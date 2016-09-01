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
#include "eos-shard-blob.h"

/**
 * EosShardWriter:
 *
 * An object for packing file into a shard file. Records are
 * tagged with a unique 40 character hex name.
 */

#define EOS_SHARD_TYPE_WRITER_V2 (eos_shard_writer_v2_get_type ())
G_DECLARE_FINAL_TYPE (EosShardWriterV2, eos_shard_writer_v2, EOS_SHARD, WRITER_V2, GObject)

EosShardWriterV2 * eos_shard_writer_v2_new_for_fd (int fd);

uint64_t eos_shard_writer_v2_add_blob (EosShardWriterV2  *self,
                                       char              *name,
                                       GFile             *file,
                                       char              *content_type,
                                       EosShardBlobFlags  flags);
uint64_t eos_shard_writer_v2_add_record (EosShardWriterV2 *self,
                                         char *hex_name);
void eos_shard_writer_v2_add_blob_to_record (EosShardWriterV2 *self,
                                             uint64_t          record_id,
                                             uint64_t          blob_id);

void eos_shard_writer_v2_finish (EosShardWriterV2 *self);
