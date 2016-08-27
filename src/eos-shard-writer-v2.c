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
  GFile *file;
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

struct _EosShardWriterV2
{
  GObject parent;

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
  g_clear_object (&blob->file);
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
eos_shard_writer_v2_class_init (EosShardWriterV2Class *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = eos_shard_writer_v2_finalize;
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

  b.file = g_object_ref (file);
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

  g_ptr_array_add (self->blobs, g_memdup (&b, sizeof (b)));
  int index = self->blobs->len - 1;
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
void
eos_shard_writer_v2_add_record (EosShardWriterV2 *self,
                                char *hex_name)
{
  struct eos_shard_writer_v2_record_entry e = {};
  eos_shard_writer_v2_record_entry_init (&e);
  eos_shard_util_hex_name_to_raw_name (e.raw_name, hex_name);
  g_array_append_val (self->records, e);
}

/**
 * eos_shard_writer_v2_add_blob_to_record:
 * @self: an #EosShardWriterV2
 * @blob_id: An opaque blob ID, retrieved from eos_shard_writer_v2_add_blob().
 *
 * Adds the referenced blob to the last added record.
 */
void
eos_shard_writer_v2_add_blob_to_record (EosShardWriterV2 *self,
                                        uint64_t          blob_id)
{
  struct eos_shard_writer_v2_record_entry *e = &g_array_index (self->records, struct eos_shard_writer_v2_record_entry, self->records->len - 1);
  struct eos_shard_writer_v2_blob_entry *b = g_ptr_array_index (self->blobs, blob_id);
  g_array_append_val (e->blobs, b);
}

struct write_context
{
  int fd;
  off_t offset;
};

static void
write_blob (int fd, struct eos_shard_writer_v2_blob_entry *blob)
{
  g_autoptr(GError) error = NULL;

  GFileInputStream *file_stream = g_file_read (blob->file, NULL, &error);
  if (!file_stream) {
    g_error ("Could not read from %s: %s", g_file_get_path (blob->file), error->message);
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

#ifdef HAVE_FALLOCATE

struct write_blob_thread_data
{
  int fd;
  struct eos_shard_writer_v2_blob_entry *blob;
};

static void
write_blob_thread (GTask *task,
                   gpointer source_object,
                   gpointer task_data,
                   GCancellable *cancellable)
{
  struct write_blob_thread_data *data = task_data;
  write_blob (data->fd, data->blob);
  g_task_return_int (task, 0);
}

struct parallel_write_blob_data
{
  int n_left;
};

static void
write_blob_callback (GObject      *source_object,
                     GAsyncResult *result,
                     gpointer      user_data)
{
  struct parallel_write_blob_data *data = user_data;
  data->n_left--;
}

/* This is the fancy algorithm we use on Linux that can be multi-threaded. */

static gboolean
blob_is_compressed (struct eos_shard_writer_v2_blob_entry *blob)
{
  return (blob->sblob.flags & EOS_SHARD_BLOB_FLAG_COMPRESSED_ZLIB);
}

static gint
compare_blobs_by_offset (gconstpointer a, gconstpointer b)
{
  struct eos_shard_writer_v2_blob_entry *blob_a = * (struct eos_shard_writer_v2_blob_entry **) a;
  struct eos_shard_writer_v2_blob_entry *blob_b = * (struct eos_shard_writer_v2_blob_entry **) b;

  if (blob_a->offs > blob_b->offs)
    return 1;
  if (blob_a->offs == blob_b->offs)
    return 0;
  if (blob_a->offs < blob_b->offs)
    return -1;

  g_assert_not_reached ();
}

static void
write_blobs (EosShardWriterV2 *self, struct write_context *ctx)
{
  int i;

  off_t offset = ctx->offset;

  struct stat stbuf;
  fstat (ctx->fd, &stbuf);

  size_t blksize = stbuf.st_blksize;

#define BLOB_ALIGN(n) (_ALIGN(n, blksize))

  /* Writing blob data is a bit tricksy, since we use several methods for speed.
   * First, we write out all blobs which are not "CPU intensive" in a serial manner. */

  for (i = 0; i < self->blobs->len; i++) {
    struct eos_shard_writer_v2_blob_entry *b = g_ptr_array_index (self->blobs, i);

    if (blob_is_compressed (b))
      continue;

    b->offs = offset = ALIGN (offset);
    offset += sizeof (b->sblob) + b->sblob.uncompressed_size;
  }

  /* Next, we schedule the position for each blob that is CPU intensive using its
   * *uncompressed data size*, but page aligned. After that, do the CPU-intensive work
   * in parallel at the recommended offsets. After that, if supported, we throw out
   * the pages we don't care about and adjust the data offsets... */

  for (i = 0; i < self->blobs->len; i++) {
    struct eos_shard_writer_v2_blob_entry *b = g_ptr_array_index (self->blobs, i);

    if (!blob_is_compressed (b))
      continue;

    b->offs = offset = BLOB_ALIGN (offset);
    offset += sizeof (b->sblob) + b->sblob.uncompressed_size;
  }

  /* Now that all blobs are scheduled, sort them by offset. This is required
   * for us to calculate the deltas to fallocate below... */
  g_ptr_array_sort (self->blobs, compare_blobs_by_offset);

  /* Write in parallel... */
  GMainContext *context = g_main_context_new ();
  g_main_context_push_thread_default (context);

  struct parallel_write_blob_data parallel_data = {
    .n_left = self->blobs->len,
  };

  for (i = 0; i < self->blobs->len; i++) {
    struct eos_shard_writer_v2_blob_entry *b = g_ptr_array_index (self->blobs, i);
    struct write_blob_thread_data thread_data = { .fd = ctx->fd, .blob = b };
    g_autoptr(GTask) task = g_task_new (NULL, NULL, write_blob_callback, &parallel_data);
    g_task_set_task_data (task, g_memdup (&thread_data, sizeof (thread_data)), (GDestroyNotify) g_free);
    g_task_run_in_thread (task, write_blob_thread);
  }

  while (parallel_data.n_left > 0)
    g_main_context_iteration (context, TRUE);

  g_main_context_pop_thread_default (context);
  g_main_context_unref (context);

  off_t blob_offset_delta = 0;

  /* Go through and remove unused chunks from each blob. */

  for (i = 0; i < self->blobs->len; i++) {
    struct eos_shard_writer_v2_blob_entry *b = g_ptr_array_index (self->blobs, i);

    b->offs -= blob_offset_delta;
    b->sblob.data_start -= blob_offset_delta;

    if (!blob_is_compressed (b))
      continue;

    off_t uncompressed_data_end = BLOB_ALIGN (b->offs + b->sblob.uncompressed_size);
    off_t compressed_data_end = BLOB_ALIGN (b->offs + b->sblob.size);

    off_t remove_range_start = compressed_data_end;
    off_t remove_range_length = uncompressed_data_end - compressed_data_end;

    if (remove_range_length > 0) {
      /* If we're the last blob, then the zeroes probably haven't been written out to disk -- let's just stomp over them. */
      gboolean is_last_blob = (i == self->blobs->len - 1);

      if (is_last_blob || fallocate (ctx->fd, FALLOC_FL_COLLAPSE_RANGE, remove_range_start, remove_range_length) == 0) {
        /* We successfully collapsed this range. Adjust blob offsets from this point forward. */
        blob_offset_delta += remove_range_length;
      }
    }
  }

  offset -= blob_offset_delta;

  /* Now go through and write out blob headers. */
  for (i = 0; i < self->blobs->len; i++) {
    struct eos_shard_writer_v2_blob_entry *b = g_ptr_array_index (self->blobs, i);
    g_assert (pwrite (ctx->fd, &b->sblob, sizeof (b->sblob), b->offs) >= 0);
  }

  ctx->offset = BLOB_ALIGN (offset);

#undef BLOB_ALIGN
}

#else

static void
write_blobs (EosShardWriterV2 *self, struct write_context *ctx)
{
  int i;

  for (i = 0; i < self->blobs->len; i++) {
    struct eos_shard_writer_v2_blob_entry *b = g_ptr_array_index (self->blobs, i);

    b->offs = ctx->offset = ALIGN (ctx->offset);
    write_blob (ctx->fd, b);
    g_assert (pwrite (ctx->fd, &b->sblob, sizeof (b->sblob), b->offs) >= 0);
    ctx->offset += b->sblob.size + sizeof (b->sblob);
  }
}

#endif /* HAVE_FALLOCATE */

void
eos_shard_writer_v2_write_to_fd (EosShardWriterV2 *self, int fd)
{
  int i;

  /* Sort our records to allow for binary searches on retrieval. */
  g_array_sort (self->records, &compare_records);

  struct write_context _ctx = {
    .fd = fd,
  };
  struct write_context *ctx = &_ctx;

  struct eos_shard_v2_hdr hdr = { };

  memcpy (hdr.magic, EOS_SHARD_V2_MAGIC, sizeof (hdr.magic));
  hdr.records_length = self->records->len;

  ctx->offset = sizeof (hdr);

  write_blobs (self, ctx);
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

/**
 * eos_shard_writer_v2_write:
 * @self: An #EosShardWriterV2
 * @path: The file path to write the shard to.
 *
 * This finalizes the shard and writes the contents to the file path
 * specified. Meant as the final step in compiling a shard file together.
 */
void
eos_shard_writer_v2_write (EosShardWriterV2 *self, char *path)
{
  int fd = open (path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  eos_shard_writer_v2_write_to_fd (self, fd);
  close (fd);
}
