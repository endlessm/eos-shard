
#include "epak-blob.h"
#include "epak-pak.h"

static EpakBlob *
epak_blob_new (void)
{
  EpakBlob *blob = g_new0 (EpakBlob, 1);
  blob->ref_count = 1;
  return blob;
}

static void
epak_blob_free (EpakBlob *blob)
{
  g_object_unref (blob->pak);
  g_free (blob);
}

static EpakBlob *
epak_blob_ref (EpakBlob *blob)
{
  blob->ref_count++;
  return blob;
}

static void
epak_blob_unref (EpakBlob *blob)
{
  if (--blob->ref_count == 0)
    epak_blob_free (blob);
}

/**
 * epak_blob_get_content_type:
 *
 * Get the content type of the blob's data.
 *
 * Returns: the mimetype of the blob
 */
const char *
epak_blob_get_content_type (EpakBlob *blob)
{
  switch (blob->blob->content_type) {
    case EPAK_BLOB_CONTENT_TYPE_HTML:
      return "text/html";

    case EPAK_BLOB_CONTENT_TYPE_PNG:
      return "image/png";

    case EPAK_BLOB_CONTENT_TYPE_JPG:
      return "image/jpeg";

    case EPAK_BLOB_CONTENT_TYPE_PDF:
      return "application/pdf";

    case EPAK_BLOB_CONTENT_TYPE_JSON:
      return "application/json";

    case EPAK_BLOB_CONTENT_TYPE_TEXT_PLAIN:
      return "text/plain";

    case EPAK_BLOB_CONTENT_TYPE_UNKNOWN:
    default:
      return "application/octet-stream";
  }
}


/**
 * epak_blob_get_stream:
 *
 * Creates and returns a #GInputStream to the blob's content. If the blob is
 * compressed, the returned stream will be streamed through a
 * #GZlibDecompressor, yielding decompressed data.
 *
 * Returns: (transfer full): a new GInputStream for the blob's data
 */
GInputStream *
epak_blob_get_stream (EpakBlob *blob)
{
  GInputStream *stream;
  GZlibDecompressor *decompressor;

  stream = G_INPUT_STREAM (_epak_blob_stream_new_for_blob (blob, blob->pak));
  if (blob->blob->flags & EPAK_BLOB_FLAG_COMPRESSED_ZLIB) {
    decompressor = g_zlib_decompressor_new (G_ZLIB_COMPRESSOR_FORMAT_ZLIB);
    return g_converter_input_stream_new (stream, G_CONVERTER (decompressor));
  }

  return stream;
}

EpakBlobFlags
epak_blob_get_flags (EpakBlob *blob)
{
  return (EpakBlobFlags) blob->blob->flags;
}

gsize
_epak_blob_get_packed_size (EpakBlob *blob)
{
  return blob->blob->size;
}

goffset
_epak_blob_get_offset (EpakBlob *blob)
{
  return blob->blob->offs;
}

gsize
epak_blob_get_content_size (EpakBlob *blob)
{
  return blob->blob->uncompressed_size;
}

/**
 * epak_blob_load_contents:
 *
 * Synchronously read and return the contents of this
 * blob as a #GBytes.
 *
 * Returns: (transfer full): the blob's data
 */
GBytes *
epak_blob_load_contents (EpakBlob *blob)
{
  return _epak_pak_load_blob (blob->pak, blob->blob);
}

EpakBlob *
_epak_blob_new_for_blob (EpakPak                *pak,
                         struct epak_blob_entry *blob_)
{
  EpakBlob *blob = epak_blob_new ();
  blob->pak = g_object_ref (pak);
  blob->blob = blob_;
  return blob;
}

G_DEFINE_BOXED_TYPE (EpakBlob, epak_blob,
                     epak_blob_ref, epak_blob_unref)
