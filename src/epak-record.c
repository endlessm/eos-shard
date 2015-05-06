/* Copyright 2015 Endless Mobile, Inc. */

/* This file is part of epak.
 *
 * epak is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * epak is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with epak.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include "epak-record.h"

#include <stdio.h>

#include "epak-pak.h"
#include "epak-blob.h"
#include "epak-format.h"

static EpakRecord *
epak_record_new (void)
{
  EpakRecord *record = g_new0 (EpakRecord, 1);
  record->ref_count = 1;
  return record;
}

static void
epak_record_free (EpakRecord *record)
{
  if (record->pak != NULL)
    g_object_unref (record->pak);
  if (record->data != NULL)
    epak_blob_unref (record->data);
  if (record->metadata != NULL)
    epak_blob_unref (record->metadata);
  g_free (record);
}

static EpakRecord *
epak_record_ref (EpakRecord *record)
{
  record->ref_count++;
  return record;
}

static void
epak_record_unref (EpakRecord *record)
{
  if (--record->ref_count == 0)
    epak_record_free (record);
}

/**
 * epak_record_get_raw_name:
 *
 * Get the raw name of an record, which is a series of 20
 * bytes, which represent a SHA-1 hash.
 *
 * Returns: (transfer none): the name
 */
uint8_t *
epak_record_get_raw_name (EpakRecord *record)
{
  return (uint8_t *) record->record_entry->raw_name;
}

/**
 * epak_record_get_hex_name:
 *
 * Returns a debug name of an record, which is the same as
 * a SHA-1 hash but as a readable hex string.
 *
 * Returns: (transfer full): the debug name
 */
char *
epak_record_get_hex_name (EpakRecord *record)
{
  char *hex_name = g_malloc (41);
  epak_util_raw_name_to_hex_name (hex_name, record->record_entry->raw_name);
  hex_name[EPAK_HEX_NAME_SIZE] = '\0';
  return hex_name;
}

EpakRecord *
_epak_record_new_for_record_entry (EpakPak *pak, struct epak_record_entry *record_entry)
{
  EpakRecord *record = epak_record_new ();
  record->pak = g_object_ref (pak);
  record->record_entry = record_entry;
  record->metadata = _epak_blob_new_for_blob (pak, &record_entry->metadata);
  record->data = _epak_blob_new_for_blob (pak, &record_entry->data);
  return record;
}

G_DEFINE_BOXED_TYPE (EpakRecord, epak_record,
                     epak_record_ref, epak_record_unref)
