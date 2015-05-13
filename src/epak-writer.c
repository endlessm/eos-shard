/* Copyright 2015 Endless Mobile, Inc. */

/* This file is part of epak.
 *
 * epak is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * epak is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with epak.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include "epak-writer.h"

#include <fcntl.h>
#include <string.h>
#include <zlib.h>

#include "epak-pak.h"
#include "epak-utils.h"

struct epak_writer_record_entry
{
  struct epak_record_entry base;

  GFile *metadata_file;
  GFile *data_file;
};

struct _EpakWriterPrivate
{
  GArray *entries;
};
typedef struct _EpakWriterPrivate EpakWriterPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (EpakWriter, epak_writer, G_TYPE_OBJECT);

static void
epak_writer_finalize (GObject *object)
{
  EpakWriter *writer;
  EpakWriterPrivate *priv;

  writer = EPAK_WRITER (object);
  priv = epak_writer_get_instance_private (writer);

  if (priv->entries != NULL)
    g_array_unref (priv->entries);

  G_OBJECT_CLASS (epak_writer_parent_class)->finalize (object);
}

static void
epak_writer_class_init (EpakWriterClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = epak_writer_finalize;
}

static void
epak_writer_init (EpakWriter *writer)
{
  EpakWriterPrivate *priv = epak_writer_get_instance_private (writer);

  priv->entries = g_array_new (FALSE, TRUE, sizeof (struct epak_writer_record_entry));
}

static void
fill_blob_entry_from_gfile (struct epak_blob_entry *blob, GFile *file)
{
  GFileInfo *info = g_file_query_info (file, "standard::*", 0, NULL, NULL);
  const char *ct = g_file_info_get_content_type (info);

  g_assert (strlen (ct) < sizeof (blob->content_type));
  strcpy (blob->content_type, ct);

  blob->uncompressed_size = g_file_info_get_size (info);

  g_object_unref (info);
}

/**
 * epak_writer_add_record:
 * @writer: a EpakWriter
 * @hex_name: the hexadecimal string which will act as this entry's ID
 * @metadata: a #GFile pointing to the entry's metadata on disk
 * @metadata_flags: an #EpakBlobFlags to be applied to the metadata
 * @data: a #GFile pointing to the entry's data on disk
 * @data_flags: an #EpakBlobFlags to be applied to the data
 *
 * Adds a data/metadata file pair to the epak. These pairs must be added in
 * increasing hex_name order. Once all pairs have been added, call
 * epak_writer_write to save the epak to disk.
 */
void
epak_writer_add_record (EpakWriter *writer,
                       char *hex_name,
                       GFile *metadata,
                       EpakBlobFlags metadata_flags,
                       GFile *data,
                       EpakBlobFlags data_flags)
{
  EpakWriterPrivate *priv = epak_writer_get_instance_private (writer);
  struct epak_writer_record_entry record_entry = {};

  epak_util_hex_name_to_raw_name (record_entry.base.raw_name, hex_name);

  if (priv->entries->len > 0) {
    struct epak_writer_record_entry *e = &g_array_index (priv->entries, struct epak_writer_record_entry, priv->entries->len - 1);
    g_assert (memcmp (record_entry.base.raw_name, e->base.raw_name, EPAK_RAW_NAME_SIZE) > 0);
  }

  record_entry.metadata_file = metadata;
  record_entry.base.metadata.flags = metadata_flags;
  fill_blob_entry_from_gfile (&record_entry.base.metadata, metadata);
  record_entry.data_file = data;
  record_entry.base.data.flags = data_flags;
  fill_blob_entry_from_gfile (&record_entry.base.data, data);

  g_array_append_val (priv->entries, record_entry);
}

static void
write_blob (int fd,
            int data_offs,
            struct epak_blob_entry *blob,
            GFile *file)
{
  GFileInputStream *file_stream = g_file_read (file, NULL, NULL);
  GInputStream *stream;

  if (blob->flags & EPAK_BLOB_FLAG_COMPRESSED_ZLIB) {
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
epak_writer_write (EpakWriter *writer,
                   char *path)
{
  EpakWriterPrivate *priv = epak_writer_get_instance_private (writer);
  int fd = open (path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  struct epak_hdr hdr;
  int entry_offs;

  memcpy (hdr.magic, EPAK_MAGIC, sizeof (hdr.magic));
  hdr.n_records = priv->entries->len;
  hdr.data_offs = ALIGN (ALIGN (sizeof (hdr)) + sizeof (struct epak_record_entry) * hdr.n_records);
  write (fd, &hdr, sizeof (hdr));

  entry_offs = lalign (fd);
  lseek (fd, hdr.data_offs, SEEK_SET);

  int i;
  for (i = 0; i < priv->entries->len; i++) {
    struct epak_writer_record_entry *e = &g_array_index (priv->entries, struct epak_writer_record_entry, i);

    write_blob (fd, hdr.data_offs, &e->base.metadata, e->metadata_file);
    write_blob (fd, hdr.data_offs, &e->base.data, e->data_file);
  }

  lseek (fd, entry_offs, SEEK_SET);

  for (i = 0; i < priv->entries->len; i++) {
    struct epak_writer_record_entry *e = &g_array_index (priv->entries, struct epak_writer_record_entry, i);
    struct epak_record_entry *record_entry = &e->base;
    write (fd, record_entry, sizeof (*record_entry));
  }

  close (fd);
}
