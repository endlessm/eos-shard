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

#include "eos-shard-blob-stream.h"
#include "eos-shard-blob.h"
#include "eos-shard-enums.h"
#include "eos-shard-shard-file.h"

#include <string.h>
#include <errno.h>

struct _EosShardBlobStreamPrivate
{
  goffset pos;
  EosShardBlob *blob;
  EosShardShardFile *shard_file;
};
typedef struct _EosShardBlobStreamPrivate EosShardBlobStreamPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (EosShardBlobStream, eos_shard_blob_stream, G_TYPE_INPUT_STREAM);

static gssize
eos_shard_blob_stream_read (GInputStream  *stream,
                            void          *buffer,
                            gsize          count,
                            GCancellable  *cancellable,
                            GError       **error)
{
  EosShardBlobStream *blob_stream;
  EosShardBlobStreamPrivate *priv;
  gsize actual_count, size_read;
  int read_error;
  goffset blob_offset;

  blob_stream = EOS_SHARD_BLOB_STREAM (stream);
  priv = eos_shard_blob_stream_get_instance_private (blob_stream);

  blob_offset = _eos_shard_blob_get_offset (priv->blob);
  actual_count = MIN (count, _eos_shard_blob_get_packed_size (priv->blob) - priv->pos);

  size_read = _eos_shard_shard_file_read_data (priv->shard_file, buffer, actual_count, blob_offset + priv->pos);
  read_error = errno;
  if (size_read == -1) {
    g_set_error (error, EOS_SHARD_ERROR, EOS_SHARD_ERROR_BLOB_STREAM_READ,
                 "Read failed: %s", strerror (read_error));
    return -1;
  }

  priv->pos += size_read;
  return size_read;
}

static gboolean
eos_shard_blob_stream_close (GInputStream  *stream,
                             GCancellable  *cancellable,
                             GError       **error)
{
  return TRUE;
}

static void
eos_shard_blob_stream_dispose (GObject *object)
{
  EosShardBlobStream *blob_stream;
  EosShardBlobStreamPrivate *priv;

  blob_stream = EOS_SHARD_BLOB_STREAM (object);
  priv = eos_shard_blob_stream_get_instance_private (blob_stream);

  g_clear_pointer (&priv->blob, eos_shard_blob_unref);
  g_clear_object (&priv->shard_file);

  G_OBJECT_CLASS (eos_shard_blob_stream_parent_class)->dispose (object);
}

static void
eos_shard_blob_stream_class_init (EosShardBlobStreamClass *klass)
{
  GObjectClass *gobject_class;
  GInputStreamClass *istream_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->dispose = eos_shard_blob_stream_dispose;

  istream_class = G_INPUT_STREAM_CLASS (klass);
  istream_class->read_fn  = eos_shard_blob_stream_read;
  istream_class->close_fn = eos_shard_blob_stream_close;
}

static void
eos_shard_blob_stream_init (EosShardBlobStream *blob_stream)
{
}

EosShardBlobStream *
_eos_shard_blob_stream_new_for_blob (EosShardBlob *blob,
                                     EosShardShardFile  *shard_file)
{
  EosShardBlobStream *blob_stream;
  EosShardBlobStreamPrivate *priv;

  blob_stream = g_object_new (EOS_SHARD_TYPE_BLOB_STREAM, NULL);
  priv = eos_shard_blob_stream_get_instance_private (blob_stream);

  priv->blob = eos_shard_blob_ref (blob);
  priv->shard_file = g_object_ref (shard_file);

  return blob_stream;
}
