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

#include "eos-shard-types.h"
#include "eos-shard-shard-file.h"

/**
 * EosShardBlobStream:
 *
 * A #GInputStream to an #EosShardBlob's content.
 */

#define EOS_SHARD_TYPE_BLOB_STREAM (eos_shard_blob_stream_get_type ())
G_DECLARE_FINAL_TYPE (EosShardBlobStream, eos_shard_blob_stream, EOS_SHARD, BLOB_STREAM, GInputStream)

EosShardBlobStream * _eos_shard_blob_stream_new_for_blob (EosShardBlob *blob, EosShardShardFile *shard_file);
