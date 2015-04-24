
#include "epak-writer.h"

#include <fcntl.h>
#include <string.h>

#include "epak-pak.h"
#include "epak_private.h"
#include "epak_fmt.h"

struct epak_writer_blob_entry
{
  GFile *file;
  struct epak_blob_entry base;
};

struct epak_writer_doc_entry
{
  uint8_t name[20];
  struct epak_writer_blob_entry metadata, data;
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
fill_blob_entry_from_gfile (struct epak_writer_blob_entry *b, GFile *file)
{
  struct epak_blob_entry *blob = &b->base;
  b->file = file;

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

  /* XXX: ekn heuristics */
  switch (blob->content_type)
    {
    case EPAK_BLOB_CONTENT_TYPE_HTML:
    case EPAK_BLOB_CONTENT_TYPE_JSON:
      blob->flags |= EPAK_BLOB_FLAG_COMPRESSED_ZLIB;
      break;
    }

  blob->uncompressed_size = g_file_info_get_size (info);

  g_object_unref (info);
}

void
epak_writer_add_entry (EpakWriter *writer,
                       char *hex_name,
                       GFile *metadata, GFile *data)
{
  EpakWriterPrivate *priv = epak_writer_get_instance_private (writer);
  struct epak_writer_doc_entry doc = {};

  epak_util_hex_name_to_raw_name (doc.name, hex_name);

  fill_blob_entry_from_gfile (&doc.metadata, metadata);
  fill_blob_entry_from_gfile (&doc.data, data);

  g_array_append_val (priv->entries, doc);
}

static void
write_blob (struct epak_writer_t *pakw,
            struct epak_blob_entry *be,
            struct epak_writer_blob_entry *wbe)
{
  GFileInputStream *file_stream = g_file_read (wbe->file, NULL, NULL);
  GInputStream *stream;

  if (wbe->base.flags & EPAK_BLOB_FLAG_COMPRESSED_ZLIB) {
    GZlibCompressor *compressor = g_zlib_compressor_new (G_ZLIB_COMPRESSOR_FORMAT_ZLIB, -1);
    stream = g_converter_input_stream_new (G_INPUT_STREAM (file_stream), G_CONVERTER (compressor));
    g_object_unref (file_stream);
  } else {
    stream = G_INPUT_STREAM (file_stream);
  }

  struct epak_blob_writer_t bw;
  *be = wbe->base;
  epak_write_blob (&bw, pakw, be, 0);

  char buf[4096];
  int size, total_size = 0;
  while ((size = g_input_stream_read (stream, buf, sizeof (buf), NULL, NULL)) != 0) {
    epak_blob_writer_write (&bw, buf, size);
    total_size += size;
  }

  be->size = total_size;

  g_object_unref (stream);
}

void
epak_writer_write (EpakWriter *writer,
                   char *path)
{
  EpakWriterPrivate *priv = epak_writer_get_instance_private (writer);
  int fd = open (path, O_WRONLY | O_CREAT | O_TRUNC);
  struct epak_t *pak;
  struct epak_writer_t pakw;

  pak = epak_new (priv->entries->len);
  epak_write (&pakw, fd, pak);

  int i;
  for (i = 0; i < priv->entries->len; i++) {
    struct epak_writer_doc_entry *e = &g_array_index (priv->entries, struct epak_writer_doc_entry, i);
    memcpy (pak->entries[i].raw_name, e->name, EPAK_RAW_NAME_SIZE);

    write_blob (&pakw, &pak->entries[i].metadata, &e->metadata);
    write_blob (&pakw, &pak->entries[i].data, &e->data);
  }

  epak_write_finish (&pakw);
  close (fd);
}
