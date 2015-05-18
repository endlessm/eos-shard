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

#include <fcntl.h>
#include <string.h>
#include <zlib.h>

#include "eos-shard-shard-file.h"
#include "eos-shard-utils.h"

struct eos_shard_writer_record_entry
{
  struct eos_shard_record_entry base;

  GFile *metadata_file;
  GFile *data_file;
};

struct _EosShardWriterPrivate
{
  GArray *entries;
};
typedef struct _EosShardWriterPrivate EosShardWriterPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (EosShardWriter, eos_shard_writer, G_TYPE_OBJECT);

static void
eos_shard_writer_finalize (GObject *object)
{
  EosShardWriter *writer;
  EosShardWriterPrivate *priv;

  writer = EOS_SHARD_WRITER (object);
  priv = eos_shard_writer_get_instance_private (writer);

  g_array_unref (priv->entries);

  G_OBJECT_CLASS (eos_shard_writer_parent_class)->finalize (object);
}

static void
eos_shard_writer_record_entry_clear (struct eos_shard_writer_record_entry *entry)
{
  g_clear_object (&entry->metadata_file);
  g_clear_object (&entry->data_file);
}

static void
eos_shard_writer_class_init (EosShardWriterClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = eos_shard_writer_finalize;
}

static void
eos_shard_writer_init (EosShardWriter *writer)
{
  EosShardWriterPrivate *priv = eos_shard_writer_get_instance_private (writer);

  priv->entries = g_array_new (FALSE, TRUE, sizeof (struct eos_shard_writer_record_entry));
  g_array_set_clear_func (priv->entries, (GDestroyNotify) eos_shard_writer_record_entry_clear);
}

static void
fill_blob_entry_from_gfile (struct eos_shard_blob_entry *blob, GFile *file)
{
  GFileInfo *info = g_file_query_info (file, "standard::*", 0, NULL, NULL);
  const char *ct = g_file_info_get_content_type (info);

  g_assert (strlen (ct) < sizeof (blob->content_type));
  strcpy (blob->content_type, ct);

  blob->uncompressed_size = g_file_info_get_size (info);

  g_object_unref (info);
}

/**
 * eos_shard_writer_add_record:
 * @writer: a EosShardWriter
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
eos_shard_writer_add_record (EosShardWriter *writer,
                             char *hex_name,
                             GFile *metadata,
                             EosShardBlobFlags metadata_flags,
                             GFile *data,
                             EosShardBlobFlags data_flags)
{
  EosShardWriterPrivate *priv = eos_shard_writer_get_instance_private (writer);
  struct eos_shard_writer_record_entry record_entry = {};

  eos_shard_util_hex_name_to_raw_name (record_entry.base.raw_name, hex_name);

  if (priv->entries->len > 0) {
    struct eos_shard_writer_record_entry *e = &g_array_index (priv->entries, struct eos_shard_writer_record_entry, priv->entries->len - 1);
    g_assert (memcmp (record_entry.base.raw_name, e->base.raw_name, EOS_SHARD_RAW_NAME_SIZE) > 0);
  }

  record_entry.metadata_file = g_object_ref (metadata);
  record_entry.base.metadata.flags = metadata_flags;
  fill_blob_entry_from_gfile (&record_entry.base.metadata, metadata);
  record_entry.data_file = g_object_ref (data);
  record_entry.base.data.flags = data_flags;
  fill_blob_entry_from_gfile (&record_entry.base.data, data);

  g_array_append_val (priv->entries, record_entry);
}

static void
write_blob (int fd,
            int data_offs,
            struct eos_shard_blob_entry *blob,
            GFile *file)
{
  GFileInputStream *file_stream = g_file_read (file, NULL, NULL);
  GInputStream *stream;

  if (blob->flags & EOS_SHARD_BLOB_FLAG_COMPRESSED_ZLIB) {
    GZlibCompressor *compressor = g_zlib_compressor_new (G_ZLIB_COMPRESSOR_FORMAT_ZLIB, -1);
    stream = g_converter_input_stream_new (G_INPUT_STREAM (file_stream), G_CONVERTER (compressor));
    g_object_unref (file_stream);
    g_object_unref (compressor);
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

  g_object_unref (stream);
}

void
eos_shard_writer_write (EosShardWriter *writer,
                        char *path)
{
  EosShardWriterPrivate *priv = eos_shard_writer_get_instance_private (writer);
  int fd = open (path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  struct eos_shard_hdr hdr;
  int entry_offs;

  memcpy (hdr.magic, EOS_SHARD_MAGIC, sizeof (hdr.magic));
  hdr.n_records = priv->entries->len;
  hdr.data_offs = ALIGN (ALIGN (sizeof (hdr)) + sizeof (struct eos_shard_record_entry) * hdr.n_records);
  write (fd, &hdr, sizeof (hdr));

  entry_offs = lalign (fd);
  lseek (fd, hdr.data_offs, SEEK_SET);

  int i;
  for (i = 0; i < priv->entries->len; i++) {
    struct eos_shard_writer_record_entry *e = &g_array_index (priv->entries, struct eos_shard_writer_record_entry, i);

    write_blob (fd, hdr.data_offs, &e->base.metadata, e->metadata_file);
    write_blob (fd, hdr.data_offs, &e->base.data, e->data_file);
  }

  lseek (fd, entry_offs, SEEK_SET);

  for (i = 0; i < priv->entries->len; i++) {
    struct eos_shard_writer_record_entry *e = &g_array_index (priv->entries, struct eos_shard_writer_record_entry, i);
    struct eos_shard_record_entry *record_entry = &e->base;
    write (fd, record_entry, sizeof (*record_entry));
  }

  close (fd);
}
