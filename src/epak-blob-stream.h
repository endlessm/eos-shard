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

#ifndef EPAK_BLOB_STREAM_H
#define EPAK_BLOB_STREAM_H

#include <gio/gio.h>

#include "epak-types.h"
#include "epak-format.h"

#define EPAK_TYPE_BLOB_STREAM             (epak_blob_stream_get_type ())
#define EPAK_BLOB_STREAM(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), EPAK_TYPE_BLOB_STREAM, EpakBlobStream))
#define EPAK_BLOB_STREAM_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass),  EPAK_TYPE_BLOB_STREAM, EpakBlobStreamClass))
#define EPAK_IS_BLOB_STREAM(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EPAK_TYPE_BLOB_STREAM))
#define EPAK_IS_BLOB_STREAM_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass),  EPAK_TYPE_BLOB_STREAM))
#define EPAK_BLOB_STREAM_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj),  EPAK_TYPE_BLOB_STREAM, EpakBlobStreamClass))

typedef struct _EpakBlobStream        EpakBlobStream;
typedef struct _EpakBlobStreamClass   EpakBlobStreamClass;

struct _EpakBlobStream
{
  GInputStream parent;
};

struct _EpakBlobStreamClass
{
  GInputStreamClass parent_class;
};

GType epak_blob_stream_get_type (void) G_GNUC_CONST;

EpakBlobStream * _epak_blob_stream_new_for_blob (EpakBlob *blob, EpakPak *pak);

#endif /* EPAK_BLOB_STREAM_H */
