
#ifndef EPAK_BLOB_H
#define EPAK_BLOB_H

#include <gio/gio.h>
#include <stdint.h>

#include "epak-types.h"
#include "epak-blob-stream.h"
#include "epak-format.h"

GType epak_blob_get_type (void) G_GNUC_CONST;

struct _EpakBlob {
  /*< private >*/
  int ref_count;
  EpakPak *pak;
  struct epak_blob_entry *blob;
};

const char * epak_blob_get_content_type (EpakBlob *blob);
GBytes * epak_blob_load_contents (EpakBlob *blob);
GInputStream * epak_blob_get_stream (EpakBlob *blob);
EpakBlobFlags epak_blob_get_flags (EpakBlob *blob);
gsize epak_blob_get_content_size (EpakBlob *blob);
EpakBlob * epak_blob_ref (EpakBlob *blob);
void epak_blob_unref (EpakBlob *blob);

EpakBlob * _epak_blob_new_for_blob (EpakPak *pak, struct epak_blob_entry *blob);
gsize _epak_blob_get_packed_size (EpakBlob *blob);
goffset _epak_blob_get_offset (EpakBlob *blob);

#endif /* EPAK_BLOB_H */
