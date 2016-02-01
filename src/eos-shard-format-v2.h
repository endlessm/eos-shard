/* Copyright 2016 Endless Mobile, Inc. */

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

#ifndef EOS_SHARD_FORMAT_V2_H
#define EOS_SHARD_FORMAT_V2_H

#include <stdint.h>

#include "eos-shard-shard-file.h"

/* Unlike V1, the file immediately starts with this magic and header. */

#define EOS_SHARD_V2_MAGIC "ShardV2 "

/*
 * Implementation notes:
 *
 * We serialize C structs directly to disk. Technically a no-no, but it
 * can work well enough. We do expect sane alignment and padding from our
 * system.
 *
 * All values in shards are intended to be used in little-endian form,
 * but no effort is made to convert, since we currently ship exclusively
 * on little-endian systems.
 */

#pragma pack(push, 4)

struct eos_shard_v2_hdr {
  char magic[8];

  /* Flags about the file. If we extend the format to add new chunks,
   * we might add in cap bits in here. For now, there are no flags. */
  uint16_t flags;

  /* Number of all records in the shard. */
  uint64_t records_length;

  /* Offset to where the record table starts... */
  uint64_t records_start;

  /* Offset to the string constant table. All other offsets into the
   * string constant table are relative to this... */
  uint64_t string_constant_table_start;
};

enum {
  /* A tombstone is a special record that implies we should treat the record
   * as if it was deleted -- when loading multiple shards, a tombstone overrides
   * records in older databases. */
  EOS_SHARD_V2_RECORD_FLAG_TOMBSTONE = 0x01,
};

struct eos_shard_v2_record {
  /* Our keys are fixed-size "raw" SHA-1 hashes. Records *must* be sorted by
   * this key, otherwise our bisection will break. */
  uint8_t raw_name[EOS_SHARD_RAW_NAME_SIZE];

  /* Flags and capability bits. */
  uint32_t flags;

  /* The number of blobs this record. Capped at 255. There should
   * *always* be at least one entry. */
  uint8_t blob_table_length;

  /* Offset relative to the start of the file about where the blob table can
   * be located... */
  uint64_t blob_table_start;
};

/* Each entry in the blob table is simply a pointer to the start of the blob.
 * It's written out here as a struct to make the code clearer, with less casts. */
struct eos_shard_v2_record_blob_table_entry {
  uint64_t blob_start;
};

#define EOS_SHARD_V2_BLOB_METADATA "$metadata"
#define EOS_SHARD_V2_BLOB_DATA "$data"

#define EOS_SHARD_V2_BLOB_MAX_NAME_SIZE (255)
#define EOS_SHARD_V2_BLOB_MAX_CONTENT_TYPE_SIZE (255)

struct eos_shard_v2_blob {
  /* Flags and capability bits, along with the public EosShardBlobFlags... */
  uint16_t flags;

  /* The name of the blob. */
  uint64_t name_offs;

  /* Offset into the string constant table containing a MIME type. */
  uint64_t content_type_offs;

  /* SHA256 checksum of the file. */
  uint8_t csum[0x20];

  /* The sizes of the files. If the file is uncompressed, then the two sizes
   * should be the same... */
  uint64_t size;
  uint64_t uncompressed_size;

  /* Offset to the start of the data. */
  uint64_t data_start;
};

#pragma pack(pop)

#endif /* EOS_SHARD_FORMAT_V2_H */
