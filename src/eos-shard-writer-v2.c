/* Copyright 2015 Endless Mobile, Inc. */

/* This file is part of eos-shard.
 *
 * eos-shard is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * eos-shard is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with eos-shard.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include "eos-shard-writer-v2.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <gio/gfiledescriptorbased.h>
#include <gio/gunixinputstream.h>

#include "eos-shard-shard-file.h"
#include "eos-shard-format-v2.h"

/* Rounds up n to the nearest m -- m must be a power of two. */
#define _ALIGN(n, m) (((n) + ((m)-1)) & ~((m)-1))

#define ALIGN(n) _ALIGN(n, 0x20)

struct eos_shard_writer_v2_blob_entry
{
  /* Offset to where the sblob is placed in the file... */
  off_t offs;

  char *name;
  struct eos_shard_v2_blob sblob;
};

struct eos_shard_writer_v2_record_entry
{
  uint8_t raw_name[EOS_SHARD_RAW_NAME_SIZE];
  GArray *blobs;

  off_t blob_table_start;
};

struct constant_pool
{
  uint64_t total_size;
  GHashTable *strings_to_offsets;
  GPtrArray *strings;
};

struct write_context
{
  int fd;
  off_t offset;
  size_t blksize;
};

enum
{
  PROP_0,
  PROP_FD,
  LAST_PROP,
};

static GParamSpec *obj_props[LAST_PROP] = { NULL, };

struct _EosShardWriterV2
{
  GObject parent;

  /* This lock applies to the members below. */
  GMutex lock;

  struct write_context ctx;
  GPtrArray *blobs;
  GArray *records;
  GHashTable *csum_to_data_start;
  struct constant_pool cpool;
};

G_DEFINE_TYPE (EosShardWriterV2, eos_shard_writer_v2, G_TYPE_OBJECT);

#define CSTRING_SIZE(S) (strlen((S)) + 1)

static void
constant_pool_init (struct constant_pool *cpool)
{
  cpool->total_size = 0;
  cpool->strings_to_offsets = g_hash_table_new_full (g_str_hash, g_str_equal,
                                                     (GDestroyNotify) g_free,
                                                     NULL);
  cpool->strings = g_ptr_array_new ();
}

static void
constant_pool_dispose (struct constant_pool *cpool)
{
  g_hash_table_destroy (cpool->strings_to_offsets);
  g_ptr_array_unref (cpool->strings);
}

static uint64_t
constant_pool_add (struct constant_pool *cpool, const char *S)
{
  if (g_hash_table_contains (cpool->strings_to_offsets, S))
    return GPOINTER_TO_UINT (g_hash_table_lookup (cpool->strings_to_offsets, S));

  uint64_t offset = cpool->total_size;
  char *our_S = g_strdup (S);
  g_hash_table_insert (cpool->strings_to_offsets, our_S, GUINT_TO_POINTER (offset));
  g_ptr_array_add (cpool->strings, our_S);
  cpool->total_size += CSTRING_SIZE (our_S);
  return offset;
}

static void
constant_pool_write (struct constant_pool *cpool, int fd, off_t offset)
{
  int i;
  for (i = 0; i < cpool->strings->len; i++) {
    char *key = g_ptr_array_index (cpool->strings, i);
    size_t size = CSTRING_SIZE (key);
    g_assert (pwrite (fd, key, size, offset) >= 0);
    offset += size;
  }
}

static guint
csum_hash (gconstpointer v)
{
  /* Simple variant of g_str_hash / the djb hash function. */
  const signed char *p, *e;
  guint32 h = 5381;
  for (p = v, e = v+0x20; p < e; p++)
    h = (h << 5) + h + *p;
  return h;
}

static gboolean
csum_equal (gconstpointer a, gconstpointer b)
{
  return (memcmp (a, b, 0x20)) == 0;
}

static void
eos_shard_writer_v2_finalize (GObject *object)
{
  EosShardWriterV2 *self = EOS_SHARD_WRITER_V2 (object);
  constant_pool_dispose (&self->cpool);
  g_ptr_array_unref (self->blobs);
  g_array_unref (self->records);
  g_hash_table_unref (self->csum_to_data_start);
  G_OBJECT_CLASS (eos_shard_writer_v2_parent_class)->finalize (object);
}

static void
eos_shard_writer_v2_blob_entry_free (struct eos_shard_writer_v2_blob_entry *blob)
{
  g_free (blob->name);
  g_free (blob);
}

static void
eos_shard_writer_v2_record_entry_clear (struct eos_shard_writer_v2_record_entry *e)
{
  g_array_unref (e->blobs);
}

static void
eos_shard_writer_v2_record_entry_init (struct eos_shard_writer_v2_record_entry *e)
{
  e->blobs = g_array_new (FALSE, TRUE, sizeof (struct eos_shard_writer_v2_blob_entry *));
}

static void
open_write_context (struct write_context *ctx, int fd)
{
  ctx->fd = fd;

  struct stat stbuf;
  fstat (ctx->fd, &stbuf);
  ctx->blksize = stbuf.st_blksize;

  ctx->offset = sizeof (struct eos_shard_v2_hdr);
}

static void
eos_shard_writer_v2_set_property (GObject      *object,
                                  guint         prop_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  EosShardWriterV2 *self = EOS_SHARD_WRITER_V2 (object);

  switch (prop_id) {
  case PROP_FD:
    open_write_context (&self->ctx, g_value_get_uint64 (value));
    break;

  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
eos_shard_writer_v2_get_property (GObject    *object,
                                  guint       prop_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  EosShardWriterV2 *self = EOS_SHARD_WRITER_V2 (object);

  switch (prop_id) {
  case PROP_FD:
    g_value_set_uint64 (value, self->ctx.fd);
    break;

  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
eos_shard_writer_v2_class_init (EosShardWriterV2Class *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = eos_shard_writer_v2_finalize;
  gobject_class->set_property = eos_shard_writer_v2_set_property;
  gobject_class->get_property = eos_shard_writer_v2_get_property;

  obj_props[PROP_FD] =
    g_param_spec_uint64 ("fd", "", "", 0, G_MAXUINT64, 0,
                         (GParamFlags) (G_PARAM_READWRITE |
                                        G_PARAM_CONSTRUCT_ONLY |
                                        G_PARAM_STATIC_STRINGS));

  g_object_class_install_properties (gobject_class, LAST_PROP, obj_props);
}

static void
eos_shard_writer_v2_init (EosShardWriterV2 *self)
{
  g_mutex_init (&self->lock);

  constant_pool_init (&self->cpool);

  self->blobs = g_ptr_array_new_with_free_func ((GDestroyNotify) eos_shard_writer_v2_blob_entry_free);

  self->records = g_array_new (FALSE, TRUE, sizeof (struct eos_shard_writer_v2_record_entry));
  g_array_set_clear_func (self->records, (GDestroyNotify) eos_shard_writer_v2_record_entry_clear);

  self->csum_to_data_start = g_hash_table_new (csum_hash, csum_equal);
}

/**
 * eos_shard_writer_v2_new_for_fd:
 *
 * Returns: (transfer full):
 */
EosShardWriterV2 *
eos_shard_writer_v2_new_for_fd (int fd)
{
  return EOS_SHARD_WRITER_V2 (g_object_new (EOS_SHARD_TYPE_WRITER_V2, "fd", fd, NULL));
}

static int
compress_blob_to_tmp (GInputStream *file_stream)
{
  char tmpl[] = "/tmp/shardXXXXXX";
  int fd = mkstemp (tmpl);
  unlink (tmpl);

  /* Compress the given GInputStream to the tmpfile. */

  g_autoptr(GZlibCompressor) compressor = g_zlib_compressor_new (G_ZLIB_COMPRESSOR_FORMAT_ZLIB, -1);
  g_autoptr(GInputStream) stream = g_converter_input_stream_new (G_INPUT_STREAM (file_stream), G_CONVERTER (compressor));

  uint8_t buf[4096*4];
  int size;
  while ((size = g_input_stream_read (stream, buf, sizeof (buf), NULL, NULL)) != 0) {
    g_assert (write (fd, buf, size) >= 0);
  }

  return fd;
}

/**
 * eos_shard_writer_v2_add_blob:
 * @self: an #EosShardWriterV2
 * @name: the name of the blob to store.
 * @file: a file of contents to write into the shard
 * @content_type: (allow-none): The MIME type of the blob. Pass %NULL to
 *   autodetect using Gio.
 * @flags: flags about how the data should be stored in the file
 *
 * Adds the blob at the specified file path to the shard.
 *
 * Returns some opaque identifier for the blob, to be passed to
 * eos_shard_writer_v2_add_blob_to_record().
 */
uint64_t
eos_shard_writer_v2_add_blob (EosShardWriterV2  *self,
                              char              *name,
                              GFile             *file,
                              char              *content_type,
                              EosShardBlobFlags  flags)
{
  struct eos_shard_writer_v2_blob_entry b = {};
  g_autoptr(GFileInfo) info = NULL;

  b.name = g_strdup (name);

  if (content_type == NULL) {
    info = g_file_query_info (file, "standard::size,standard::content-type", 0, NULL, NULL);
    content_type = (char *) g_file_info_get_content_type (info);
  } else {
    info = g_file_query_info (file, "standard::size", 0, NULL, NULL);
  }

  g_return_val_if_fail (strlen (name) <= EOS_SHARD_V2_BLOB_MAX_NAME_SIZE, 0);
  g_return_val_if_fail (strlen (content_type) <= EOS_SHARD_V2_BLOB_MAX_CONTENT_TYPE_SIZE, 0);

  b.sblob.flags = flags;
  b.sblob.uncompressed_size = g_file_info_get_size (info);

  /* Lock around the cpool. */
  g_mutex_lock (&self->lock);
  b.sblob.name_offs = constant_pool_add (&self->cpool, name);
  b.sblob.content_type_offs = constant_pool_add (&self->cpool, content_type);
  g_mutex_unlock (&self->lock);

  /* Now deal with blob contents. */

  g_autoptr(GError) error = NULL;
  GFileInputStream *file_stream = g_file_read (file, NULL, &error);
  if (!file_stream) {
    g_error ("Could not read from %s: %s", g_file_get_path (file), error->message);
    return -1;
  }

  int blob_fd;
  if (b.sblob.flags & EOS_SHARD_BLOB_FLAG_COMPRESSED_ZLIB) {
    blob_fd = compress_blob_to_tmp (G_INPUT_STREAM (file_stream));
  } else {
    blob_fd = g_file_descriptor_based_get_fd (G_FILE_DESCRIPTOR_BASED (file_stream));
  }

  /* Find the size by seeking... */
  uint64_t blob_size = lseek (blob_fd, 0, SEEK_END);
  g_assert (blob_size >= 0);
  g_assert (lseek (blob_fd, 0, SEEK_SET) >= 0);

  /* Checksum the blob. */
  g_autoptr(GChecksum) checksum = g_checksum_new (G_CHECKSUM_SHA256);
  uint8_t buf[4096*4];
  int size;
  while ((size = read (blob_fd, buf, sizeof (buf))) != 0)
    g_checksum_update (checksum, buf, size);

  size_t checksum_buf_len = sizeof (b.sblob.csum);
  g_checksum_get_digest (checksum, b.sblob.csum, &checksum_buf_len);
  g_assert (checksum_buf_len == sizeof (b.sblob.csum));

  /* Add the blob entry to the file. */
  g_mutex_lock (&self->lock);

  /* Look for a checksum in our table to return early if we have it... */
  off_t data_start = GPOINTER_TO_UINT (g_hash_table_lookup (self->csum_to_data_start, &b.sblob.csum));

  struct eos_shard_writer_v2_blob_entry *blob = g_memdup (&b, sizeof (b));
  g_ptr_array_add (self->blobs, blob);
  uint64_t index = self->blobs->len - 1;

  /* If the blob data isn't already in the file, write it in. */
  if (data_start == 0) {
    /* Position the blob in the file, and add it to the csum table. */
    int shard_fd = self->ctx.fd;
    data_start = self->ctx.offset;
    self->ctx.offset = ALIGN (self->ctx.offset + blob_size);
    g_hash_table_insert (self->csum_to_data_start, &blob->sblob.csum, GUINT_TO_POINTER (data_start));

    /* Unlock before writing data to the file. */
    g_mutex_unlock (&self->lock);

    off_t offset = data_start;

    lseek (blob_fd, 0, SEEK_SET);
    while ((size = read (blob_fd, buf, sizeof (buf))) != 0) {
      g_assert (pwrite (shard_fd, buf, size, offset) >= 0);
      offset += size;
    }
  } else {
    g_mutex_unlock (&self->lock);
  }

  blob->sblob.data_start = data_start;
  blob->sblob.size = blob_size;

  g_assert (close (blob_fd) == 0 || errno == EINTR);

  return index;
}

/**
 * eos_shard_writer_v2_add_record:
 * @self: an #EosShardWriterV2
 * @hex_name: the hexadecimal string which will act as this entry's ID
 *
 * Adds a new record to the shard file. Once all pairs have been
 * added, call eos_shard_writer_v2_write() to save the shard file to disk.
 *
 * To set the individual fields of the blobs within the record,
 * use eos_shard_writer_v2_add_blob().
 */
uint64_t
eos_shard_writer_v2_add_record (EosShardWriterV2 *self,
                                char *hex_name)
{
  g_mutex_lock (&self->lock);

  struct eos_shard_writer_v2_record_entry e = {};
  eos_shard_writer_v2_record_entry_init (&e);
  eos_shard_util_hex_name_to_raw_name (e.raw_name, hex_name);
  g_array_append_val (self->records, e);
  uint64_t index = self->records->len - 1;

  g_mutex_unlock (&self->lock);

  return index;
}

/**
 * eos_shard_writer_v2_add_blob_to_record:
 * @self: an #EosShardWriterV2
 * @record_id: An opaque record ID, retrieved from eos_shard_writer_v2_add_record().
 * @blob_id: An opaque blob ID, retrieved from eos_shard_writer_v2_add_blob().
 *
 * Adds the referenced blob to the specified record.
 */
void
eos_shard_writer_v2_add_blob_to_record (EosShardWriterV2 *self,
                                        uint64_t          record_id,
                                        uint64_t          blob_id)
{
  struct eos_shard_writer_v2_record_entry *e = &g_array_index (self->records, struct eos_shard_writer_v2_record_entry, record_id);
  struct eos_shard_writer_v2_blob_entry *b = g_ptr_array_index (self->blobs, blob_id);
  g_array_append_val (e->blobs, b);
}

static gint
compare_blob_table_entries (gconstpointer a, gconstpointer b)
{
  struct eos_shard_writer_v2_blob_entry *blob_a = * (struct eos_shard_writer_v2_blob_entry **) a;
  struct eos_shard_writer_v2_blob_entry *blob_b = * (struct eos_shard_writer_v2_blob_entry **) b;
  return strcmp (blob_a->name, blob_b->name);
}

static void
write_blob_table (struct write_context *ctx, struct eos_shard_writer_v2_record_entry *e)
{
  e->blob_table_start = ctx->offset;

  g_array_sort (e->blobs, compare_blob_table_entries);

  int i;
  for (i = 0; i < e->blobs->len; i++) {
    struct eos_shard_writer_v2_blob_entry *b = g_array_index (e->blobs, struct eos_shard_writer_v2_blob_entry *, i);
    struct eos_shard_v2_record_blob_table_entry be = { .blob_start = b->offs };
    g_assert (pwrite (ctx->fd, &be, sizeof (be), ctx->offset) >= 0);
    ctx->offset += sizeof (be);
  }
}

static void
write_record (struct write_context *ctx, struct eos_shard_writer_v2_record_entry *e)
{
  struct eos_shard_v2_record srecord = {};
  memcpy (srecord.raw_name, e->raw_name, EOS_SHARD_RAW_NAME_SIZE);
  srecord.blob_table_start = e->blob_table_start;
  srecord.blob_table_length = e->blobs->len;
  g_assert (pwrite (ctx->fd, &srecord, sizeof (srecord), ctx->offset) >= 0);
  ctx->offset += sizeof (srecord);
}

static gint
compare_records (gconstpointer a, gconstpointer b)
{
  struct eos_shard_writer_v2_record_entry *r_a, *r_b;
  r_a = (struct eos_shard_writer_v2_record_entry*) a;
  r_b = (struct eos_shard_writer_v2_record_entry*) b;
  return memcmp (r_a->raw_name, r_b->raw_name, EOS_SHARD_RAW_NAME_SIZE);
}

void
eos_shard_writer_v2_finish (EosShardWriterV2 *self)
{
  int i;

  /* Sort our records to allow for binary searches on retrieval. */
  g_array_sort (self->records, &compare_records);

  struct write_context *ctx = &self->ctx;

  struct eos_shard_v2_hdr hdr = { };

  memcpy (hdr.magic, EOS_SHARD_V2_MAGIC, sizeof (hdr.magic));
  hdr.records_length = self->records->len;

  /* Now go through and write out blob headers. */
  for (i = 0; i < self->blobs->len; i++) {
    struct eos_shard_writer_v2_blob_entry *b = g_ptr_array_index (self->blobs, i);
    b->offs = ctx->offset;
    g_assert (pwrite (ctx->fd, &b->sblob, sizeof (b->sblob), ctx->offset) >= 0);
    ctx->offset += sizeof (b->sblob);
  }

  /* Now write out blob tables... */
  for (i = 0; i < self->records->len; i++) {
    struct eos_shard_writer_v2_record_entry *e = &g_array_index (self->records, struct eos_shard_writer_v2_record_entry, i);
    write_blob_table (ctx, e);
  }

  /* Now for records... */
  hdr.records_start = ctx->offset = ALIGN (ctx->offset);
  for (i = 0; i < self->records->len; i++) {
    struct eos_shard_writer_v2_record_entry *e = &g_array_index (self->records, struct eos_shard_writer_v2_record_entry, i);
    write_record (ctx, e);
  }

  /* Now for the string constant table... */
  hdr.string_constant_table_start = ALIGN (ctx->offset);
  constant_pool_write (&self->cpool, ctx->fd, hdr.string_constant_table_start);

  g_assert (pwrite (ctx->fd, &hdr, sizeof (hdr), 0) >= 0);
}
