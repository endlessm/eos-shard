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

#ifdef HAVE_FALLOCATE
#include <linux/falloc.h>
#endif

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "eos-shard-shard-file.h"
#include "eos-shard-format-v2.h"

/* Rounds up n to the nearest m -- m must be a power of two. */
#define _ALIGN(n, m) (((n) + ((m)-1)) & ~((m)-1))

#define ALIGN(n) _ALIGN(n, 0x20)

struct eos_shard_writer_v2_blob_entry
{
  char *name;
  off_t offs;
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

  struct write_context ctx;

  GPtrArray *blobs;
  GArray *records;

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

static void
eos_shard_writer_v2_finalize (GObject *object)
{
  EosShardWriterV2 *self = EOS_SHARD_WRITER_V2 (object);
  constant_pool_dispose (&self->cpool);
  g_ptr_array_unref (self->blobs);
  g_array_unref (self->records);
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
  constant_pool_init (&self->cpool);

  self->blobs = g_ptr_array_new_with_free_func ((GDestroyNotify) eos_shard_writer_v2_blob_entry_free);

  self->records = g_array_new (FALSE, TRUE, sizeof (struct eos_shard_writer_v2_record_entry));
  g_array_set_clear_func (self->records, (GDestroyNotify) eos_shard_writer_v2_record_entry_clear);
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

static void
write_blob (int fd, struct eos_shard_writer_v2_blob_entry *blob, GFile *file)
{
  g_autoptr(GError) error = NULL;

  GFileInputStream *file_stream = g_file_read (file, NULL, &error);
  if (!file_stream) {
    g_error ("Could not read from %s: %s", g_file_get_path (file), error->message);
    return;
  }

  g_autoptr(GInputStream) stream;

  if (blob->sblob.flags & EOS_SHARD_BLOB_FLAG_COMPRESSED_ZLIB) {
    g_autoptr(GZlibCompressor) compressor = g_zlib_compressor_new (G_ZLIB_COMPRESSOR_FORMAT_ZLIB, -1);
    stream = g_converter_input_stream_new (G_INPUT_STREAM (file_stream), G_CONVERTER (compressor));
    g_object_unref (file_stream);
  } else {
    stream = G_INPUT_STREAM (file_stream);
  }

  off_t data_start = blob->offs + sizeof (blob->sblob);
  off_t offset = data_start;

  uint8_t buf[4096*4];
  int size;
  g_autoptr(GChecksum) checksum = g_checksum_new (G_CHECKSUM_SHA256);
  while ((size = g_input_stream_read (stream, buf, sizeof (buf), NULL, NULL)) != 0) {
    g_assert (pwrite (fd, buf, size, offset) >= 0);
    g_checksum_update (checksum, buf, size);
    offset += size;
  }

  size_t checksum_buf_len = sizeof (blob->sblob.csum);
  g_checksum_get_digest (checksum, blob->sblob.csum, &checksum_buf_len);
  g_assert (checksum_buf_len == sizeof (blob->sblob.csum));

  blob->sblob.data_start = data_start;
  blob->sblob.size = (offset - data_start);
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
  b.sblob.name_offs = constant_pool_add (&self->cpool, name);
  b.sblob.content_type_offs = constant_pool_add (&self->cpool, content_type);
  b.sblob.uncompressed_size = g_file_info_get_size (info);

  /* Position the blob in the file. */

#ifdef HAVE_FALLOCATE
  /* If we have fallocate, then use a multiple of the blocksize so that we can
   * cut out the blocks with zeroes afterwards. */
  self->ctx.offset = _ALIGN (self->ctx.offset, self->ctx.blksize);
#else
  self->ctx.offset = ALIGN (self->ctx.offset);
#endif

  b.offs = self->ctx.offset;
  self->ctx.offset += sizeof (b.sblob) + b.sblob.uncompressed_size;

  struct eos_shard_writer_v2_blob_entry *blob = g_memdup (&b, sizeof (b));
  g_ptr_array_add (self->blobs, blob);
  uint64_t index = self->blobs->len - 1;


  write_blob (self->ctx.fd, blob, file);

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
  struct eos_shard_writer_v2_record_entry e = {};
  eos_shard_writer_v2_record_entry_init (&e);
  eos_shard_util_hex_name_to_raw_name (e.raw_name, hex_name);
  g_array_append_val (self->records, e);
  uint64_t index = self->records->len - 1;

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

static void
finish_writing_blobs (EosShardWriterV2 *self, struct write_context *ctx)
{
  int i;

#ifdef HAVE_FALLOCATE

  off_t blob_offset_delta = 0;

  /* Go through and remove unused chunks from each blob. */

  for (i = 0; i < self->blobs->len; i++) {
    struct eos_shard_writer_v2_blob_entry *b = g_ptr_array_index (self->blobs, i);

    b->offs -= blob_offset_delta;
    b->sblob.data_start -= blob_offset_delta;

    off_t uncompressed_data_end = _ALIGN (b->sblob.data_start + b->sblob.uncompressed_size, ctx->blksize);
    off_t compressed_data_end = _ALIGN (b->sblob.data_start + b->sblob.size, ctx->blksize);

    off_t remove_range_start = compressed_data_end;
    off_t remove_range_length = uncompressed_data_end - compressed_data_end;

    if (remove_range_length > 0) {
      if (fallocate (ctx->fd, FALLOC_FL_COLLAPSE_RANGE, remove_range_start, remove_range_length) == 0) {
        /* We successfully collapsed this range. Adjust blob offsets from this point forward. */
        blob_offset_delta += remove_range_length;
      }
    }
  }

  ctx->offset -= blob_offset_delta;

#endif /* HAVE_FALLOCATE */

  /* Now go through and write out blob headers. */
  for (i = 0; i < self->blobs->len; i++) {
    struct eos_shard_writer_v2_blob_entry *b = g_ptr_array_index (self->blobs, i);
    g_assert (pwrite (ctx->fd, &b->sblob, sizeof (b->sblob), b->offs) >= 0);
  }
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

  finish_writing_blobs (self, &self->ctx);
  ctx->offset = ALIGN (ctx->offset);

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
