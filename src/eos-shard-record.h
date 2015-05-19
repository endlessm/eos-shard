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

#ifndef EOS_SHARD_RECORD_H
#define EOS_SHARD_RECORD_H

#include <gio/gio.h>
#include <stdint.h>

#include "eos-shard-types.h"

/**
 * EosShardRecord:
 *
 * A particular data/metadata pair. Generally, a record will have some content
 * stored as data, as well as some JSON-formatted metadata about that content.
 **/

GType eos_shard_record_get_type (void) G_GNUC_CONST;

struct _EosShardRecord {
  /*< private >*/
  int ref_count;
  EosShardShardFile *shard_file;
  struct eos_shard_record_entry *record_entry;

  /*< public >*/
  EosShardBlob *data;
  EosShardBlob *metadata;
};

EosShardRecord * _eos_shard_record_new_for_record_entry (EosShardShardFile *shard_file, struct eos_shard_record_entry *record_entry);

uint8_t * eos_shard_record_get_raw_name (EosShardRecord *record);
char * eos_shard_record_get_hex_name (EosShardRecord *record);

#endif /* EOS_SHARD_RECORD_H */
