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

/* ESD1 file format */

#pragma pack(push,1)

/* "ESD" stands for "EOS SharD". If you have a better acronym,
 * please let me know. This is the best I could come up with for
 * three letters... */
#define EOS_SHARD_MAGIC ("ESD1")

struct eos_shard_hdr
{
  char magic[4];
  uint32_t n_records;
  uint64_t data_offs;
};

typedef enum eos_shard_blob_flags
{
  EOS_SHARD_BLOB_FLAG_NONE,
  EOS_SHARD_BLOB_FLAG_COMPRESSED_ZLIB,
} EosShardBlobFlags;

struct eos_shard_blob_entry
{
  char content_type[64];
  uint16_t flags;
  uint32_t adler32;
  uint64_t offs;
  uint64_t size;
  uint64_t uncompressed_size;
};

#define EOS_SHARD_RAW_NAME_SIZE 20
#define EOS_SHARD_HEX_NAME_SIZE (EOS_SHARD_RAW_NAME_SIZE*2)

struct eos_shard_record_entry
{
  uint8_t raw_name[EOS_SHARD_RAW_NAME_SIZE];
  struct eos_shard_blob_entry metadata;
  struct eos_shard_blob_entry data;
};

#pragma pack(pop)

#endif /* EOS_SHARD_FORMAT_H */
