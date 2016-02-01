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

#ifndef EOS_SHARD_WRITER_H
#define EOS_SHARD_WRITER_H

#include <gio/gio.h>
#include "eos-shard-format.h"

/**
 * EosShardWriter:
 *
 * An object for packing file into a shard file. Records are added as pairs of
 * data/metadata, and are tagged with a unique 40 character hex name.
 *
 * Files must be added in increasing order based on their name.
 */

#define EOS_SHARD_TYPE_WRITER (eos_shard_writer_get_type ())
G_DECLARE_FINAL_TYPE (EosShardWriter, eos_shard_writer, EOS_SHARD, WRITER, GObject)

typedef enum {
  EOS_SHARD_WRITER_BLOB_METADATA,
  EOS_SHARD_WRITER_BLOB_DATA,
} EosShardWriterBlob;

void eos_shard_writer_add_record (EosShardWriter *self,
                                  char *hex_name);
void eos_shard_writer_add_blob (EosShardWriter *self,
                                EosShardWriterBlob which_blob,
                                GFile *file,
                                EosShardBlobFlags flags);
void eos_shard_writer_write_to_fd (EosShardWriter *self, int fd);
void eos_shard_writer_write (EosShardWriter *self, char *path);

#endif /* EOS_SHARD_WRITER_H */
