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

#include "eos-shard-writer.h"

#include <endian.h>
#include <fcntl.h>
#include <string.h>
#include <zlib.h>

#include "eos-shard-shard-file.h"

#define ALIGN(n) (((n) + 0x3f) & ~0x3f)

static off_t lalign(int fd)
{
  off_t off = lseek (fd, 0, SEEK_CUR);
  off = ALIGN (off);
  lseek (fd, off, SEEK_SET);
  return off;
}

struct eos_shard_writer_blob_entry
{
  GFile *file;
  char *content_type;
  uint16_t flags;
  uint32_t adler32;
  uint64_t offs;
  uint64_t size;
  uint64_t uncompressed_size;
};

struct eos_shard_writer_record_entry
{
  uint8_t raw_name[EOS_SHARD_RAW_NAME_SIZE];
  struct eos_shard_writer_blob_entry metadata;
  struct eos_shard_writer_blob_entry data;
};

struct _EosShardWriter
{
  GObject parent;

  GArray *entries;
};

G_DEFINE_TYPE (EosShardWriter, eos_shard_writer, G_TYPE_OBJECT);

static void
eos_shard_writer_finalize (GObject *object)
{
  EosShardWriter *self = EOS_SHARD_WRITER (object);
  g_array_unref (self->entries);
  G_OBJECT_CLASS (eos_shard_writer_parent_class)->finalize (object);
}

static void
eos_shard_writer_blob_entry_clear (struct eos_shard_writer_blob_entry *blob)
{
  g_clear_object (&blob->file);
  g_free (blob->content_type);
}

static void
eos_shard_writer_record_entry_clear (struct eos_shard_writer_record_entry *entry)
{
  eos_shard_writer_blob_entry_clear (&entry->metadata);
  eos_shard_writer_blob_entry_clear (&entry->data);
}

static void
eos_shard_writer_class_init (EosShardWriterClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = eos_shard_writer_finalize;
}

static void
eos_shard_writer_init (EosShardWriter *self)
{
  self->entries = g_array_new (FALSE, TRUE, sizeof (struct eos_shard_writer_record_entry));
  g_array_set_clear_func (self->entries, (GDestroyNotify) eos_shard_writer_record_entry_clear);
}

static void
fill_blob_entry_from_gfile (struct eos_shard_writer_blob_entry *blob, GFile *file)
{
  g_autoptr(GFileInfo) info = g_file_query_info (file, "standard::*", 0, NULL, NULL);
  const char *ct = g_file_info_get_content_type (info);

  blob->file = g_object_ref (file);
  blob->content_type = g_strdup (ct);
  blob->uncompressed_size = g_file_info_get_size (info);
}

/**
 * eos_shard_writer_add_record:
 * @self: an #EosShardWriter
 * @hex_name: the hexadecimal string which will act as this entry's ID
 * @metadata: a #GFile pointing to the entry's metadata on disk
 * @metadata_flags: an #EosShardBlobFlags to be applied to the metadata
 * @data: a #GFile pointing to the entry's data on disk
 * @data_flags: an #EosShardBlobFlags to be applied to the data
 *
 * Adds a data/metadata file pair to the shard file. These pairs must be added in
 * increasing hex_name order. Once all pairs have been added, call
 * eos_shard_writer_write() to save the shard file to disk.
 */
void
eos_shard_writer_add_record (EosShardWriter *self,
                             char *hex_name,
                             GFile *metadata,
                             EosShardBlobFlags metadata_flags,
                             GFile *data,
                             EosShardBlobFlags data_flags)
{
  struct eos_shard_writer_record_entry record_entry = {};

  eos_shard_util_hex_name_to_raw_name (record_entry.raw_name, hex_name);

  if (self->entries->len > 0) {
    struct eos_shard_writer_record_entry *e = &g_array_index (self->entries, struct eos_shard_writer_record_entry, self->entries->len - 1);
    g_assert (memcmp (record_entry.raw_name, e->raw_name, EOS_SHARD_RAW_NAME_SIZE) > 0);
  }

  fill_blob_entry_from_gfile (&record_entry.metadata, metadata);
  record_entry.metadata.flags = metadata_flags;

  fill_blob_entry_from_gfile (&record_entry.data, data);
  record_entry.data.flags = data_flags;

  g_array_append_val (self->entries, record_entry);
}

static void
write_blob (int fd,
            int data_offs,
            struct eos_shard_writer_blob_entry *blob)
{
  GFileInputStream *file_stream = g_file_read (blob->file, NULL, NULL);
  g_autoptr(GInputStream) stream;

  if (blob->flags & EOS_SHARD_BLOB_FLAG_COMPRESSED_ZLIB) {
    g_autoptr(GZlibCompressor) compressor = g_zlib_compressor_new (G_ZLIB_COMPRESSOR_FORMAT_ZLIB, -1);
    stream = g_converter_input_stream_new (G_INPUT_STREAM (file_stream), G_CONVERTER (compressor));
    g_object_unref (file_stream);
  } else {
    stream = G_INPUT_STREAM (file_stream);
  }

  blob->adler32 = adler32 (0L, NULL, 0);
  blob->offs = lalign (fd) - data_offs;

  char buf[4096];
  int size, total_size = 0;
  while ((size = g_input_stream_read (stream, buf, sizeof (buf), NULL, NULL)) != 0) {
    write (fd, buf, size);
    blob->adler32 = adler32 (blob->adler32, (const Bytef *) buf, size);
    total_size += size;
  }

  blob->size = total_size;
}

static GVariant *
blob_entry_variant (struct eos_shard_writer_blob_entry *blob)
{
  return g_variant_new (EOS_SHARD_BLOB_ENTRY,
                        blob->content_type,
                        blob->flags,
                        blob->adler32,
                        blob->offs,
                        blob->size,
                        blob->uncompressed_size);
}

static GVariant *
record_entry_variant (struct eos_shard_writer_record_entry *entry)
{
  return g_variant_new ("(^ay@" EOS_SHARD_BLOB_ENTRY "@" EOS_SHARD_BLOB_ENTRY ")",
                        entry->raw_name,
                        blob_entry_variant (&entry->metadata),
                        blob_entry_variant (&entry->data));
}

static GVariant *
header_entry_variant (uint64_t data_offs, GArray *entries)
{
  GVariantBuilder builder;

  g_variant_builder_init (&builder, G_VARIANT_TYPE ("a" EOS_SHARD_RECORD_ENTRY));

  int i;
  for (i = 0; i < entries->len; i++) {
    struct eos_shard_writer_record_entry *e = &g_array_index (entries, struct eos_shard_writer_record_entry, i);
    g_variant_builder_add (&builder, "@" EOS_SHARD_RECORD_ENTRY, record_entry_variant (e));
  }

  return g_variant_new (EOS_SHARD_HEADER_ENTRY,
                        EOS_SHARD_MAGIC,
                        data_offs,
                        &builder);
}

static int
write_variant (int fd, GVariant *variant)
{
  return write (fd,
                g_variant_get_data (variant),
                g_variant_get_size (variant));
}

void
eos_shard_writer_write (EosShardWriter *self, char *path)
{
  int fd = open (path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  GVariant *variant;

  variant = header_entry_variant (0, self->entries);
  uint64_t header_size = g_variant_get_size (variant);
  header_size = htole64 (header_size);
  g_variant_unref (variant);

  uint64_t data_offs = ALIGN (4 + header_size);
  lseek (fd, data_offs, SEEK_SET);

  int i;
  for (i = 0; i < self->entries->len; i++) {
    struct eos_shard_writer_record_entry *e = &g_array_index (self->entries, struct eos_shard_writer_record_entry, i);

    write_blob (fd, data_offs, &e->metadata);
    write_blob (fd, data_offs, &e->data);
  }

  lseek (fd, 0, SEEK_SET);
  write (fd, &header_size, sizeof (header_size));
  variant = header_entry_variant (data_offs, self->entries);
  write_variant (fd, variant);
  g_variant_unref (variant);

  close (fd);
}