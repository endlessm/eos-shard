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
#include "eos-shard-shard-file-impl.h"

#define EOS_SHARD_RAW_NAME_SIZE 20
#define EOS_SHARD_HEX_NAME_SIZE (EOS_SHARD_RAW_NAME_SIZE*2)

/**
 * EosShardShardFile:
 *
 * A handle to a shard file. Allows for fast lookups of #EosShardRecords based on
 * their hex name.
 */

#define EOS_SHARD_TYPE_SHARD_FILE (eos_shard_shard_file_get_type ())
G_DECLARE_FINAL_TYPE (EosShardShardFile, eos_shard_shard_file, EOS_SHARD, SHARD_FILE, GObject)

void eos_shard_util_raw_name_to_hex_name (char *hex_name, const uint8_t *raw_name);
gboolean eos_shard_util_hex_name_to_raw_name (uint8_t raw_name[20], const char *hex_name);

EosShardRecord * eos_shard_shard_file_find_record_by_raw_name (EosShardShardFile *self, uint8_t *raw_name);
EosShardRecord * eos_shard_shard_file_find_record_by_hex_name (EosShardShardFile *self, char *hex_name);
GSList * eos_shard_shard_file_list_records (EosShardShardFile *self);
void eos_shard_shard_file_records_foreach (EosShardShardFile *self, EosShardRecordsForeachFunc func, gpointer user_data);

GBytes * _eos_shard_shard_file_load_blob (EosShardShardFile            *self,
                                          EosShardBlob                 *blob,
                                          GError                      **error);
EosShardDictionary * _eos_shard_shard_file_new_dictionary (EosShardShardFile *self,
                                                           EosShardBlob *blob,
                                                           GError **error);

gsize _eos_shard_shard_file_read_data (EosShardShardFile *self, void *buf, gsize count, goffset offset);
GSList * _eos_shard_shard_file_list_blobs (EosShardShardFile *self, EosShardRecord *record);

EosShardBlob * _eos_shard_shard_file_lookup_blob (EosShardShardFile *self, EosShardRecord *record, const char *name);
