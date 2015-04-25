
#include "epak-writer.h"

#include <fcntl.h>
#include <string.h>
#include <zlib.h>

#include "epak-pak.h"

struct epak_writer_doc_entry
{
  struct epak_doc_entry base;

  GFile *metadata_file, *data_file;
};

struct _EpakWriterPrivate
{
  GArray *entries;
};
typedef struct _EpakWriterPrivate EpakWriterPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (EpakWriter, epak_writer, G_TYPE_OBJECT);

static void
epak_writer_class_init (EpakWriterClass *klass)
{
}

static void
epak_writer_init (EpakWriter *writer)
{
  EpakWriterPrivate *priv = epak_writer_get_instance_private (writer);

  priv->entries = g_array_new (FALSE, TRUE, sizeof (struct epak_writer_doc_entry));
}

static void
fill_blob_entry_from_gfile (struct epak_blob_entry *blob, GFile *file)
{
  GFileInfo *info = g_file_query_info (file, "standard::*", 0, NULL, NULL);
  const char *ct = g_file_info_get_content_type (info);

  if (strcmp (ct, "text/html") == 0)
    blob->content_type = EPAK_BLOB_CONTENT_TYPE_HTML;
  else if (strcmp (ct, "image/png") == 0)
    blob->content_type = EPAK_BLOB_CONTENT_TYPE_PNG;
  else if (strcmp (ct, "image/jpeg") == 0)
    blob->content_type = EPAK_BLOB_CONTENT_TYPE_JPG;
  else if (strcmp (ct, "application/pdf") == 0)
    blob->content_type = EPAK_BLOB_CONTENT_TYPE_PDF;
  else if (strcmp (ct, "application/json") == 0)
    blob->content_type = EPAK_BLOB_CONTENT_TYPE_JSON;
  else if (strcmp (ct, "text/plain") == 0)
    blob->content_type = EPAK_BLOB_CONTENT_TYPE_TEXT_PLAIN;
  else
    {
      blob->content_type = EPAK_BLOB_CONTENT_TYPE_UNKNOWN;
      g_warning ("Unknown content-type %s from file: %s\n", ct, g_file_info_get_name (info));
    }

  blob->uncompressed_size = g_file_info_get_size (info);

  g_object_unref (info);
}

void
epak_writer_add_entry (EpakWriter *writer,
                       char *hex_name,
                       GFile *metadata,
                       EpakBlobFlags metadata_flags,
                       GFile *data,
                       EpakBlobFlags data_flags)
{
  EpakWriterPrivate *priv = epak_writer_get_instance_private (writer);
  struct epak_writer_doc_entry doc = {};

  epak_util_hex_name_to_raw_name (doc.base.raw_name, hex_name);

  doc.metadata_file = metadata;
  doc.base.metadata.flags = metadata_flags;
  fill_blob_entry_from_gfile (&doc.base.metadata, metadata);
  doc.data_file = data;
  doc.base.data.flags = data_flags;
  fill_blob_entry_from_gfile (&doc.base.data, data);

  g_array_append_val (priv->entries, doc);
}

#define ALIGN(n) (((n) + 0x3f) & ~0x3f)

static off_t lalign(int fd)
{
  off_t off = lseek (fd, 0, SEEK_CUR);
  off = ALIGN (off);
  lseek (fd, off, SEEK_SET);
  return off;
}

static void
write_blob (int fd, int data_offs,
            struct epak_blob_entry *blob,
            GFile *file)
{
  GFileInputStream *file_stream = g_file_read (file, NULL, NULL);
  GInputStream *stream;

  if (blob->flags & EPAK_BLOB_FLAG_COMPRESSED_ZLIB) {
    GZlibCompressor *compressor = g_zlib_compressor_new (G_ZLIB_COMPRESSOR_FORMAT_ZLIB, -1);
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
  hdr.n_docs = priv->entries->len;
  hdr.data_offs = ALIGN (ALIGN (sizeof (hdr)) + sizeof (struct epak_doc_entry) * hdr.n_docs);
  write (fd, &hdr, sizeof (hdr));

  entry_offs = lalign (fd);
  lseek (fd, hdr.data_offs, SEEK_SET);

  int i;
  for (i = 0; i < priv->entries->len; i++) {
    struct epak_writer_doc_entry *e = &g_array_index (priv->entries, struct epak_writer_doc_entry, i);

    write_blob (fd, hdr.data_offs, &e->base.metadata, e->metadata_file);
    write_blob (fd, hdr.data_offs, &e->base.data, e->data_file);
  }

  lseek (fd, entry_offs, SEEK_SET);

  for (i = 0; i < priv->entries->len; i++) {
    struct epak_writer_doc_entry *e = &g_array_index (priv->entries, struct epak_writer_doc_entry, i);
    struct epak_doc_entry *doc = &e->base;
    write (fd, doc, sizeof (*doc));
  }

  close (fd);
}
