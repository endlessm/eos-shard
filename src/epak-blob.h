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

#ifndef EPAK_BLOB_H
#define EPAK_BLOB_H

#include <gio/gio.h>
#include <stdint.h>

#include "epak-types.h"
#include "epak-blob-stream.h"
#include "epak-format.h"

/**
 * EpakBlob:
 *
 * A handle to either a record's data or metadata. Contains information about
 * the underlying content's MIME type, size, and whether it's compressed or
 * not.
 *
 * If the content is compressed, its #EpakBlobStream will be automatically
 * piped through a #GZlibDecompressor.
 **/

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
