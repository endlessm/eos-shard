
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

EpakBlobContentType
epak_blob_get_content_type (EpakBlob *blob)
{
  return (EpakBlobContentType) blob->blob->content_type;
}

EpakBlobFlags
epak_blob_get_flags (EpakBlob *blob)
{
  return (EpakBlobFlags) blob->blob->flags;
}

/**
 * epak_blob_load_contents:
 *
 * Synchronously read and return the contents of this
 * blob as a #GBytes.
 */
GBytes *
epak_blob_load_contents (EpakBlob *blob)
{
  return _epak_pak_load_blob (blob->pak, blob->blob);
}

EpakBlob *
_epak_blob_new_for_blob (EpakPak *pak, struct epak_blob_entry *blob_)
{
  EpakBlob *blob = epak_blob_new ();
  blob->pak = g_object_ref (pak);
  blob->blob = blob_;
  return blob;
}

G_DEFINE_BOXED_TYPE (EpakBlob, epak_blob,
                     epak_blob_ref, epak_blob_unref)
