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

#include "eos-shard-record.h"

#include <stdio.h>

#include "eos-shard-shard-file.h"
#include "eos-shard-blob.h"
#include "eos-shard-format.h"

static EosShardRecord *
eos_shard_record_new (void)
{
  EosShardRecord *record = g_new0 (EosShardRecord, 1);
  record->ref_count = 1;
  return record;
}

static void
eos_shard_record_free (EosShardRecord *record)
{
  g_clear_object (&record->shard_file);
  g_clear_pointer (&record->data, eos_shard_blob_unref);
  g_clear_pointer (&record->metadata, eos_shard_blob_unref);
  g_free (record->raw_name);
  g_free (record);
}

EosShardRecord *
eos_shard_record_ref (EosShardRecord *record)
{
  record->ref_count++;
  return record;
}

void
eos_shard_record_unref (EosShardRecord *record)
{
  if (--record->ref_count == 0)
    eos_shard_record_free (record);
}

/**
 * eos_shard_record_get_raw_name:
 *
 * Get the raw name of an record, which is a series of 20
 * bytes, which represent a SHA-1 hash.
 *
 * Returns: (transfer none): the name
 */
uint8_t *
eos_shard_record_get_raw_name (EosShardRecord *record)
{
  return (uint8_t *) record->raw_name;
}

/**
 * eos_shard_record_get_hex_name:
 *
 * Returns a debug name of an record, which is the same as
 * a SHA-1 hash but as a readable hex string.
 *
 * Returns: (transfer full): the debug name
 */
char *
eos_shard_record_get_hex_name (EosShardRecord *record)
{
  char *hex_name = g_malloc (41);
  eos_shard_util_raw_name_to_hex_name (hex_name, record->raw_name);
  hex_name[EOS_SHARD_HEX_NAME_SIZE] = '\0';
  return hex_name;
}

EosShardRecord *
_eos_shard_record_new_for_variant (EosShardShardFile *shard_file, GVariant *record_variant)
{
  EosShardRecord *record = eos_shard_record_new ();
  g_autoptr(GVariant) raw_name_variant;
  g_autoptr(GVariant) metadata_variant;
  g_autoptr(GVariant) data_variant;
  size_t n_elts;
  const void *raw_name;

  record->shard_file = g_object_ref (shard_file);
  g_variant_get (record_variant, "(@ay@" EOS_SHARD_BLOB_ENTRY "@" EOS_SHARD_BLOB_ENTRY ")",
                 &raw_name_variant,
                 &metadata_variant,
                 &data_variant);
  raw_name = g_variant_get_fixed_array (raw_name_variant, &n_elts, 1);
  g_assert (n_elts == EOS_SHARD_RAW_NAME_SIZE);
  record->raw_name = g_memdup (raw_name, EOS_SHARD_RAW_NAME_SIZE);
  record->metadata = _eos_shard_blob_new_for_variant (shard_file, metadata_variant);
  record->data = _eos_shard_blob_new_for_variant (shard_file, data_variant);
  return record;
}

G_DEFINE_BOXED_TYPE (EosShardRecord, eos_shard_record,
                     eos_shard_record_ref, eos_shard_record_unref)
