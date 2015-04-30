#include "epak-blob-stream.h"
#include "epak-blob.h"
#include "epak-pak.h"

struct _EpakBlobStreamPrivate
{
  goffset pos;
  EpakBlob *blob;
  EpakPak *pak;
};
typedef struct _EpakBlobStreamPrivate EpakBlobStreamPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (EpakBlobStream, epak_blob_stream, G_TYPE_INPUT_STREAM);

static gssize
epak_blob_stream_read (GInputStream  *stream,
                       void          *buffer,
                       gsize          count,
                       GCancellable  *cancellable,
                       GError       **error)
{
  EpakBlobStream *blob_stream;
  EpakBlobStreamPrivate *priv;
  gsize actual_count, size_read;
  goffset blob_offset;

  blob_stream = EPAK_BLOB_STREAM (stream);
  priv = epak_blob_stream_get_instance_private (blob_stream);

  blob_offset = _epak_blob_get_offset (priv->blob);
  actual_count = MIN (count, _epak_blob_get_packed_size (priv->blob) - priv->pos);

  size_read = _epak_pak_read_data (priv->pak, buffer, actual_count, blob_offset + priv->pos);
  priv->pos += size_read;
  return size_read;
}

static gboolean
epak_blob_stream_close (GInputStream  *stream,
                        GCancellable  *cancellable,
                        GError       **error)
{
  return TRUE;
}

static void
epak_blob_stream_dispose (GObject *object)
{
  EpakBlobStream *blob_stream;
  EpakBlobStreamPrivate *priv;

  blob_stream = EPAK_BLOB_STREAM (object);
  priv = epak_blob_stream_get_instance_private (blob_stream);

  g_clear_pointer (&priv->blob, epak_blob_unref);
  g_clear_object (&priv->pak);

  G_OBJECT_CLASS (epak_blob_stream_parent_class)->dispose (object);
}

static void
epak_blob_stream_class_init (EpakBlobStreamClass *klass)
{
  GObjectClass *gobject_class;
  GInputStreamClass *istream_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->dispose = epak_blob_stream_dispose;

  istream_class = G_INPUT_STREAM_CLASS (klass);
  istream_class->read_fn  = epak_blob_stream_read;
  istream_class->close_fn = epak_blob_stream_close;
}

static void
epak_blob_stream_init (EpakBlobStream *blob_stream)
{
}

EpakBlobStream *
_epak_blob_stream_new_for_blob (EpakBlob *blob,
                                EpakPak  *pak)
{
  EpakBlobStream *blob_stream;
  EpakBlobStreamPrivate *priv;

  blob_stream = g_object_new (EPAK_TYPE_BLOB_STREAM, NULL);
  priv = epak_blob_stream_get_instance_private (blob_stream);

  priv->blob = epak_blob_ref (blob);
  priv->pak = g_object_ref (pak);

  return blob_stream;
}
