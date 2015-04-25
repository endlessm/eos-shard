
#ifndef EPAK_BLOB_H
#define EPAK_BLOB_H

#include <gio/gio.h>
#include <stdint.h>

#include "epak-types.h"
#include "epak-format.h"

GType epak_blob_get_type (void) G_GNUC_CONST;

struct _EpakBlob {
  /*< private >*/
  int ref_count;
  EpakPak *pak;
  struct epak_blob_entry *blob;
};

EpakBlob * _epak_blob_new_for_blob (EpakPak *pak, struct epak_blob_entry *blob);

EpakBlobContentType epak_blob_get_content_type (EpakBlob *blob);
EpakBlobFlags epak_blob_get_flags (EpakBlob *blob);

GBytes * epak_blob_load_contents (EpakBlob *blob);

#endif /* EPAK_BLOB_H */
