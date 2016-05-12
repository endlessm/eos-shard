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

#pragma once

#include <gio/gio.h>
#include <stdint.h>

#include "eos-shard-types.h"
#include "eos-shard-blob-stream.h"

typedef enum
{
  EOS_SHARD_BLOB_FLAG_NONE,
  EOS_SHARD_BLOB_FLAG_COMPRESSED_ZLIB,
} EosShardBlobFlags;

/**
 * EosShardBlob:
 *
 * A handle to either a record's data or metadata. Contains information about
 * the underlying content's MIME type, size, and whether it's compressed or
 * not.
 *
 * If the content is compressed, its #EosShardBlobStream will be automatically
 * piped through a #GZlibDecompressor.
 **/

GType eos_shard_blob_get_type (void) G_GNUC_CONST;

struct _EosShardBlob {
  /*< private >*/
  int ref_count;
  EosShardShardFile *shard_file;

  char *content_type;
  EosShardBlobFlags flags;
  uint8_t checksum[0x20];
  uint64_t offs;
  uint64_t size;
  uint64_t uncompressed_size;
};

const char * eos_shard_blob_get_content_type (EosShardBlob *blob);

EosShardBlob * _eos_shard_blob_new (void);

GBytes * eos_shard_blob_load_contents (EosShardBlob  *blob,
                                       GError       **error);
GInputStream * eos_shard_blob_get_stream (EosShardBlob *blob);
EosShardBlobFlags eos_shard_blob_get_flags (EosShardBlob *blob);
gsize eos_shard_blob_get_content_size (EosShardBlob *blob);
EosShardBlob * eos_shard_blob_ref (EosShardBlob *blob);
void eos_shard_blob_unref (EosShardBlob *blob);

gsize _eos_shard_blob_get_packed_size (EosShardBlob *blob);
goffset eos_shard_blob_get_offset (EosShardBlob *blob);

EosShardDictionary * eos_shard_blob_load_as_dictionary (EosShardBlob *blob, GError **error);
