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

struct _EosShardBlobStream
{
  GInputStream parent;

  goffset pos;
  EosShardBlob *blob;
  EosShardShardFile *shard_file;
};

static void seekable_iface_init (GSeekableIface *iface);
static goffset eos_shard_blob_stream_tell (GSeekable *seekable);
static gboolean eos_shard_blob_stream_can_seek (GSeekable *seekable);
static gboolean eos_shard_blob_stream_seek (GSeekable *seekable,
                                            goffset offset,
                                            GSeekType type,
                                            GCancellable *cancellable,
                                            GError **error);
static gboolean eos_shard_blob_stream_can_truncate (GSeekable *seekable);
static gboolean eos_shard_blob_stream_truncate (GSeekable *seekable,
                                                goffset offset,
                                                GCancellable *cancellable,
                                                GError **error);

G_DEFINE_TYPE_WITH_CODE (EosShardBlobStream, eos_shard_blob_stream, G_TYPE_INPUT_STREAM,
                         G_IMPLEMENT_INTERFACE (G_TYPE_SEEKABLE, seekable_iface_init));

static void
seekable_iface_init (GSeekableIface *iface)
{
  iface->tell = eos_shard_blob_stream_tell;
  iface->can_seek = eos_shard_blob_stream_can_seek;
  iface->seek = eos_shard_blob_stream_seek;
  iface->can_truncate = eos_shard_blob_stream_can_truncate;
  iface->truncate_fn = eos_shard_blob_stream_truncate;
}

static goffset
eos_shard_blob_stream_tell (GSeekable *seekable)
{
  EosShardBlobStream *self = EOS_SHARD_BLOB_STREAM (seekable);
  return self->pos;
}

static gboolean
eos_shard_blob_stream_can_seek (GSeekable *seekable)
{
  return TRUE;
}

// This method implementation is pretty much copied wholesale from Gio's
// GMemoryInputStream, since the internal models are basically identical
static gboolean
eos_shard_blob_stream_seek (GSeekable *seekable,
                            goffset offset,
                            GSeekType type,
                            GCancellable *cancellable,
                            GError **error)
{
  EosShardBlobStream *self;
  goffset absolute;
  gsize blob_size;

  self = EOS_SHARD_BLOB_STREAM (seekable);
  blob_size = _eos_shard_blob_get_packed_size (self->blob);

  switch (type) {
    case G_SEEK_CUR:
      absolute = self->pos + offset;
      break;
    case G_SEEK_SET:
      absolute = offset;
      break;
    case G_SEEK_END:
      absolute = blob_size + offset;
      break;
    default:
      g_set_error_literal (error,
                           G_IO_ERROR,
                           G_IO_ERROR_INVALID_ARGUMENT,
                           "Invalid GSeekType supplied");
      return FALSE;
  }

  if (absolute < 0 || absolute > blob_size) {
    g_set_error_literal (error,
                         G_IO_ERROR,
                         G_IO_ERROR_INVALID_ARGUMENT,
                         "Invalid seek request");
    return FALSE;
  }

  self->pos = absolute;

  return TRUE;
}

static gboolean
eos_shard_blob_stream_can_truncate (GSeekable *seekable)
{
  return FALSE;
}

static gboolean
eos_shard_blob_stream_truncate (GSeekable *seekable,
                                goffset offset,
                                GCancellable *cancellable,
                                GError **error)
{
  g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
                       "Truncate not allowed on input stream");
  return FALSE;
}

static gssize
eos_shard_blob_stream_read (GInputStream  *stream,
                            void          *buffer,
                            gsize          count,
                            GCancellable  *cancellable,
                            GError       **error)
{
  EosShardBlobStream *self = EOS_SHARD_BLOB_STREAM (stream);
  gsize actual_count, size_read;
  int read_error;
  goffset blob_offset;

  blob_offset = eos_shard_blob_get_offset (self->blob);
  actual_count = MIN (count, _eos_shard_blob_get_packed_size (self->blob) - self->pos);

  size_read = _eos_shard_shard_file_read_data (self->shard_file, buffer, actual_count, blob_offset + self->pos);
  read_error = errno;
  if (size_read == -1) {
    g_set_error (error, EOS_SHARD_ERROR, EOS_SHARD_ERROR_BLOB_STREAM_READ,
                 "Read failed: %s", strerror (read_error));
    return -1;
  }

  self->pos += size_read;
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
  EosShardBlobStream *self = EOS_SHARD_BLOB_STREAM (object);

  g_clear_pointer (&self->blob, eos_shard_blob_unref);
  g_clear_object (&self->shard_file);

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
  EosShardBlobStream *self = g_object_new (EOS_SHARD_TYPE_BLOB_STREAM, NULL);
  self->blob = eos_shard_blob_ref (blob);
  self->shard_file = g_object_ref (shard_file);
  return self;
}
