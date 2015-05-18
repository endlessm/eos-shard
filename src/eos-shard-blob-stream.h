/* Copyright 2015 Endless Mobile, Inc. */

/* This file is part of eos-shard.
 *
 * eos-shard is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * eos-shard is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with eos-shard.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifndef EOS_SHARD_BLOB_STREAM_H
#define EOS_SHARD_BLOB_STREAM_H

#include <gio/gio.h>

#include "eos-shard-types.h"
#include "eos-shard-format.h"

#define EOS_SHARD_TYPE_BLOB_STREAM             (eos_shard_blob_stream_get_type ())
#define EOS_SHARD_BLOB_STREAM(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), EOS_SHARD_TYPE_BLOB_STREAM, EosShardBlobStream))
#define EOS_SHARD_BLOB_STREAM_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass),  EOS_SHARD_TYPE_BLOB_STREAM, EosShardBlobStreamClass))
#define EOS_SHARD_IS_BLOB_STREAM(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EOS_SHARD_TYPE_BLOB_STREAM))
#define EOS_SHARD_IS_BLOB_STREAM_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass),  EOS_SHARD_TYPE_BLOB_STREAM))
#define EOS_SHARD_BLOB_STREAM_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj),  EOS_SHARD_TYPE_BLOB_STREAM, EosShardBlobStreamClass))

/**
 * EosShardBlobStream:
 *
 * A #GInputStream to an #EosShardBlob's content.
 */

typedef struct _EosShardBlobStream        EosShardBlobStream;
typedef struct _EosShardBlobStreamClass   EosShardBlobStreamClass;

struct _EosShardBlobStream
{
  GInputStream parent;
};

struct _EosShardBlobStreamClass
{
  GInputStreamClass parent_class;
};

GType eos_shard_blob_stream_get_type (void) G_GNUC_CONST;

EosShardBlobStream * _eos_shard_blob_stream_new_for_blob (EosShardBlob *blob, EosShardShardFile *shard_file);

#endif /* EOS_SHARD_BLOB_STREAM_H */
