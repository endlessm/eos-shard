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

#ifndef EOS_SHARD_FORMAT_H
#define EOS_SHARD_FORMAT_H

#include <stdint.h>

/* The base file format consists of a uint64_t, which defines
 * the header's length. After that comes an EOS_SHARD_HEADER_ENTRY. */

typedef enum
{
  EOS_SHARD_BLOB_FLAG_NONE,
  EOS_SHARD_BLOB_FLAG_COMPRESSED_ZLIB,
} EosShardBlobFlags;

#define EOS_SHARD_MAGIC ("ShardV1")

/* content-type, sha256 checksum, flags, offset, size, uncompressed-size */
#define EOS_SHARD_BLOB_ENTRY "(sayuttt)"

/* raw name, metadata blob, data blob */
#define EOS_SHARD_RECORD_ENTRY "(ay" EOS_SHARD_BLOB_ENTRY EOS_SHARD_BLOB_ENTRY ")"

/* magic, array of records */
#define EOS_SHARD_HEADER_ENTRY "(sa" EOS_SHARD_RECORD_ENTRY ")"

#define EOS_SHARD_RAW_NAME_SIZE 20
#define EOS_SHARD_HEX_NAME_SIZE (EOS_SHARD_RAW_NAME_SIZE*2)

#endif /* EOS_SHARD_FORMAT_H */
