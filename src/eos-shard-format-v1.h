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

#include <stdint.h>

/* The base file format consists of a uint64_t, which defines
 * the header's length. After that comes an EOS_SHARD_HEADER_ENTRY. */

#define EOS_SHARD_V1_MAGIC ("ShardV1")

/* content-type, sha256 checksum, flags, offset, size, uncompressed-size */
#define EOS_SHARD_V1_BLOB_ENTRY "(sayuttt)"

/* raw name, metadata blob, data blob */
#define EOS_SHARD_V1_RECORD_ENTRY "(ay" EOS_SHARD_V1_BLOB_ENTRY EOS_SHARD_V1_BLOB_ENTRY ")"

/* magic, array of records */
#define EOS_SHARD_V1_HEADER_ENTRY "(sa" EOS_SHARD_V1_RECORD_ENTRY ")"
