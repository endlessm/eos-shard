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

#include "eos-shard-writer-v1.h"

#include <fcntl.h>
#include <string.h>

#include "eos-shard-shard-file.h"

#define ALIGN(n) (((n) + 0x3f) & ~0x3f)

static off_t lalign(int fd)
{
  off_t off = lseek (fd, 0, SEEK_CUR);
  off = ALIGN (off);
  lseek (fd, off, SEEK_SET);
  return off;
}

struct eos_shard_writer_v1_blob_entry
{
  GFile *file;
  char *content_type;
  uint16_t flags;
  uint8_t checksum[32];
  uint64_t offs;
  uint64_t size;
  uint64_t uncompressed_size;
};

struct eos_shard_writer_v1_record_entry
{
  uint8_t raw_name[EOS_SHARD_RAW_NAME_SIZE];
  struct eos_shard_writer_v1_blob_entry metadata;
  struct eos_shard_writer_v1_blob_entry data;
};

struct _EosShardWriterV1
{
  GObject parent;

  GArray *entries;
};

G_DEFINE_TYPE (EosShardWriterV1, eos_shard_writer_v1, G_TYPE_OBJECT);

static void
eos_shard_writer_v1_finalize (GObject *object)
{
  EosShardWriterV1 *self = EOS_SHARD_WRITER_V1 (object);
  g_array_unref (self->entries);
  G_OBJECT_CLASS (eos_shard_writer_v1_parent_class)->finalize (object);
}

static void
eos_shard_writer_v1_blob_entry_clear (struct eos_shard_writer_v1_blob_entry *blob)
{
  g_clear_object (&blob->file);
  g_free (blob->content_type);
}

static void
eos_shard_writer_v1_record_entry_clear (struct eos_shard_writer_v1_record_entry *entry)
{
  eos_shard_writer_v1_blob_entry_clear (&entry->metadata);
  eos_shard_writer_v1_blob_entry_clear (&entry->data);
}

static void
eos_shard_writer_v1_class_init (EosShardWriterV1Class *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = eos_shard_writer_v1_finalize;
}

static void
eos_shard_writer_v1_init (EosShardWriterV1 *self)
{
  self->entries = g_array_new (FALSE, TRUE, sizeof (struct eos_shard_writer_v1_record_entry));
  g_array_set_clear_func (self->entries, (GDestroyNotify) eos_shard_writer_v1_record_entry_clear);
}

static void
blob_entry_init (struct eos_shard_writer_v1_blob_entry *blob)
{
  blob->content_type = g_strdup ("");
}

static void
record_entry_init (struct eos_shard_writer_v1_record_entry *record)
{
  blob_entry_init (&record->metadata);
  blob_entry_init (&record->data);
}

/**
 * eos_shard_writer_v1_add_record:
 * @self: an #EosShardWriterV1
 * @hex_name: the hexadecimal string which will act as this entry's ID
 *
 * Adds a data/metadata file pair to the shard file. Once all pairs have been
 * added, call eos_shard_writer_v1_write() to save the shard file to disk.
 *
 * To set the individual fields of the blobs within the record,
 * use eos_shard_writer_v1_add_blob().
 */
void
eos_shard_writer_v1_add_record (EosShardWriterV1 *self,
                                char *hex_name)
{
  struct eos_shard_writer_v1_record_entry record_entry = {};
  eos_shard_util_hex_name_to_raw_name (record_entry.raw_name, hex_name);
  record_entry_init (&record_entry);
  g_array_append_val (self->entries, record_entry);
}

static struct eos_shard_writer_v1_blob_entry *
get_blob_entry (struct eos_shard_writer_v1_record_entry *e,
                EosShardWriterV1Blob which_blob)
{
  switch (which_blob) {
  case EOS_SHARD_WRITER_V1_BLOB_METADATA:
    return &e->metadata;
  case EOS_SHARD_WRITER_V1_BLOB_DATA:
    return &e->data;
  default:
    g_assert_not_reached ();
  }
}

/**
 * eos_shard_writer_v1_add_blob:
 * @self: an #EosShardWriterV1
 * @which_blob: Which blob in the record
 * @file: a file of contents to write into the shard
 * @content_type: (nullable): the content-type of the file. Pass %NULL to auto-detect.
 * @flags: flags about how the data should be stored in the file
 *
 * Set the data for a blob entry in the last added record in the file.
 * Records can be added to the file with eos_shard_writer_v1_add_record().
 */
void
eos_shard_writer_v1_add_blob (EosShardWriterV1 *self,
                              EosShardWriterV1Blob which_blob,
                              GFile *file,
                              const char *content_type,
                              EosShardBlobFlags flags)
{
  struct eos_shard_writer_v1_record_entry *e = &g_array_index (self->entries, struct eos_shard_writer_v1_record_entry, self->entries->len - 1);
  struct eos_shard_writer_v1_blob_entry *b = get_blob_entry (e, which_blob);

  b->file = g_object_ref (file);
  b->flags = flags;

  g_autoptr(GFileInfo) info = g_file_query_info (file, "standard::*", 0, NULL, NULL);
  if (!content_type)
    content_type = g_file_info_get_content_type (info);
  g_free (b->content_type);
  b->content_type = g_strdup (content_type);
  b->uncompressed_size = g_file_info_get_size (info);
}

static void
write_blob (int fd, struct eos_shard_writer_v1_blob_entry *blob)
{
  g_autoptr(GError) error = NULL;

  if (!blob->file)
    return;

  GFileInputStream *file_stream = g_file_read (blob->file, NULL, &error);
  if (!file_stream) {
    g_error ("Could not read from %s: %s", g_file_get_path (blob->file), error->message);
    return;
  }

  g_autoptr(GInputStream) stream = NULL;

  if (blob->flags & EOS_SHARD_BLOB_FLAG_COMPRESSED_ZLIB) {
    g_autoptr(GZlibCompressor) compressor = g_zlib_compressor_new (G_ZLIB_COMPRESSOR_FORMAT_ZLIB, -1);
    stream = g_converter_input_stream_new (G_INPUT_STREAM (file_stream), G_CONVERTER (compressor));
    g_object_unref (file_stream);
  } else {
    stream = G_INPUT_STREAM (file_stream);
  }

  blob->offs = lalign (fd);

  uint8_t buf[4096*4];
  int size, total_size = 0;
  g_autoptr(GChecksum) checksum = g_checksum_new (G_CHECKSUM_SHA256);
  while ((size = g_input_stream_read (stream, buf, sizeof (buf), NULL, NULL)) != 0) {
    g_assert (write (fd, buf, size) >= 0);
    g_checksum_update (checksum, buf, size);
    total_size += size;
  }
  size_t checksum_buf_len = sizeof (blob->checksum);
  g_checksum_get_digest (checksum, blob->checksum, &checksum_buf_len);
  g_assert (checksum_buf_len == sizeof (blob->checksum));

  blob->size = total_size;
}

static GVariant *
blob_entry_variant (struct eos_shard_writer_v1_blob_entry *blob)
{
  return g_variant_new ("(s@ayuttt)",
                        blob->content_type,
                        g_variant_new_fixed_array (G_VARIANT_TYPE ("y"), blob->checksum,
                                                   sizeof (blob->checksum), sizeof (*blob->checksum)),
                        blob->flags,
                        blob->offs,
                        blob->size,
                        blob->uncompressed_size);
}

static GVariant *
record_entry_variant (struct eos_shard_writer_v1_record_entry *entry)
{
  return g_variant_new ("(@ay@" EOS_SHARD_V1_BLOB_ENTRY "@" EOS_SHARD_V1_BLOB_ENTRY ")",
                        g_variant_new_fixed_array (G_VARIANT_TYPE ("y"), entry->raw_name,
                                                   sizeof (entry->raw_name), sizeof (*entry->raw_name)),
                        blob_entry_variant (&entry->metadata),
                        blob_entry_variant (&entry->data));
}

static GVariant *
header_entry_variant (GArray *entries)
{
  GVariantBuilder builder;

  g_variant_builder_init (&builder, G_VARIANT_TYPE ("a" EOS_SHARD_V1_RECORD_ENTRY));

  int i;
  for (i = 0; i < entries->len; i++) {
    struct eos_shard_writer_v1_record_entry *e = &g_array_index (entries, struct eos_shard_writer_v1_record_entry, i);
    g_variant_builder_add (&builder, "@" EOS_SHARD_V1_RECORD_ENTRY, record_entry_variant (e));
  }

  return g_variant_new (EOS_SHARD_V1_HEADER_ENTRY,
                        EOS_SHARD_V1_MAGIC,
                        &builder);
}

static int
write_variant (int fd, GVariant *variant)
{
  return write (fd,
                g_variant_get_data (variant),
                g_variant_get_size (variant));
}

static gint
compare_records (gconstpointer a, gconstpointer b)
{
  struct eos_shard_writer_v1_record_entry *r_a, *r_b;
  r_a = (struct eos_shard_writer_v1_record_entry*) a;
  r_b = (struct eos_shard_writer_v1_record_entry*) b;
  return memcmp (r_a->raw_name, r_b->raw_name, EOS_SHARD_RAW_NAME_SIZE);
}

void
eos_shard_writer_v1_write_to_fd (EosShardWriterV1 *self, int fd)
{
  GVariant *variant;

  /* Sort our records to allow for binary searches on retrieval. */
  g_array_sort (self->entries, &compare_records);

  variant = header_entry_variant (self->entries);
  uint64_t header_size = g_variant_get_size (variant);
  header_size = GUINT64_TO_LE (header_size);
  g_variant_unref (variant);

  lseek (fd, ALIGN (sizeof (header_size) + header_size), SEEK_SET);

  int i;
  for (i = 0; i < self->entries->len; i++) {
    struct eos_shard_writer_v1_record_entry *e = &g_array_index (self->entries, struct eos_shard_writer_v1_record_entry, i);

    write_blob (fd, &e->metadata);
    write_blob (fd, &e->data);
  }

  lseek (fd, 0, SEEK_SET);
  g_assert (write (fd, &header_size, sizeof (header_size)) >= 0);
  variant = header_entry_variant (self->entries);
  write_variant (fd, variant);
  g_variant_unref (variant);
}

/**
 * eos_shard_writer_v1_write:
 * @self: An #EosShardWriterV1
 * @path: The file path to write the shard to.
 *
 * This finalizes the shard and writes the contents to the file path
 * specified. Meant as the final step in compiling a shard file together.
 */
void
eos_shard_writer_v1_write (EosShardWriterV1 *self, char *path)
{
  int fd = open (path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  eos_shard_writer_v1_write_to_fd (self, fd);
  close (fd);
}
