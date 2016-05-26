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
#include "eos-shard-format-v1.h"
#include "eos-shard-blob.h"

/**
 * EosShardWriterV1:
 *
 * An object for packing file into a shard file. Records are added as pairs of
 * data/metadata, and are tagged with a unique 40 character hex name.
 *
 * Files must be added in increasing order based on their name.
 */

#define EOS_SHARD_TYPE_WRITER_V1 (eos_shard_writer_v1_get_type ())
G_DECLARE_FINAL_TYPE (EosShardWriterV1, eos_shard_writer_v1, EOS_SHARD, WRITER_V1, GObject)

typedef enum {
  EOS_SHARD_WRITER_V1_BLOB_METADATA,
  EOS_SHARD_WRITER_V1_BLOB_DATA,
} EosShardWriterV1Blob;

void eos_shard_writer_v1_add_record (EosShardWriterV1 *self,
                                     char *hex_name);
void eos_shard_writer_v1_add_blob (EosShardWriterV1 *self,
                                   EosShardWriterV1Blob which_blob,
                                   GFile *file,
                                   const char *content_type,
                                   EosShardBlobFlags flags);
void eos_shard_writer_v1_write_to_fd (EosShardWriterV1 *self, int fd);
void eos_shard_writer_v1_write (EosShardWriterV1 *self, char *path);
