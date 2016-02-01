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

#include "eos-shard-shard-file-impl-v2.h"

#include <errno.h>
#include <endian.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "eos-shard-shard-file.h"
#include "eos-shard-shard-file-impl.h"
#include "eos-shard-enums.h"
#include "eos-shard-blob.h"
#include "eos-shard-record.h"

#include "eos-shard-format-v2.h"

struct _EosShardShardFileImplV2
{
  GObject parent;

  EosShardShardFile *shard_file;
  int fd;

  struct eos_shard_v2_hdr hdr;
  struct eos_shard_v2_record *records;
};

static void shard_file_impl_init (EosShardShardFileImplInterface *iface);

G_DEFINE_TYPE_WITH_CODE (EosShardShardFileImplV2, eos_shard_shard_file_impl_v2, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (EOS_SHARD_TYPE_SHARD_FILE_IMPL, shard_file_impl_init))

static void
eos_shard_shard_file_impl_v2_finalize (GObject *object)
{
  EosShardShardFileImplV2 *self = EOS_SHARD_SHARD_FILE_IMPL_V2 (object);

  g_clear_pointer (&self->records, g_free);

  G_OBJECT_CLASS (eos_shard_shard_file_impl_v2_parent_class)->finalize (object);
}

static void
eos_shard_shard_file_impl_v2_class_init (EosShardShardFileImplV2Class *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->finalize = eos_shard_shard_file_impl_v2_finalize;
}

static void
eos_shard_shard_file_impl_v2_init (EosShardShardFileImplV2 *shard_file)
{
}

EosShardShardFileImpl *
_eos_shard_shard_file_impl_v2_new (EosShardShardFile *shard_file,
                                   int fd,
                                   GError **error)
{
  EosShardShardFileImplV2 *self = g_object_new (EOS_SHARD_TYPE_SHARD_FILE_IMPL_V2, NULL);
  self->fd = fd;
  self->shard_file = shard_file;

  read (self->fd, &self->hdr, sizeof (self->hdr));

  if (memcmp (self->hdr.magic, EOS_SHARD_V2_MAGIC, sizeof (self->hdr.magic)) != 0) {
    g_set_error (error, EOS_SHARD_ERROR, EOS_SHARD_ERROR_SHARD_FILE_CORRUPT,
                 "The shard file is corrupt.");
    g_object_unref (self);
    return NULL;
  }

  int buf_size = self->hdr.records_length * sizeof (*self->records);
  self->records = g_malloc (buf_size);
  pread (self->fd, self->records, buf_size, self->hdr.records_start);

  return EOS_SHARD_SHARD_FILE_IMPL (self);
}

static EosShardBlob *
blob_new (EosShardShardFile *shard_file, struct eos_shard_v2_blob *sblob, int offs)
{
  /* If the size is 0, then we have a blob with no data. Return null as
   * a special case here. */
  if (sblob->uncompressed_size == 0)
    return NULL;

  EosShardBlob *blob = _eos_shard_blob_new ();
  blob->shard_file = g_object_ref (shard_file);
  blob->flags = sblob->flags;
  memcpy (blob->checksum, sblob->csum, sizeof (blob->checksum));
  blob->size = sblob->size;
  blob->uncompressed_size = sblob->uncompressed_size;
  blob->offs = sblob->data_start;
  return blob;
}

static gboolean
find_fst_entry (EosShardShardFileImpl *impl, struct eos_shard_v2_record *srecord, const char *name,
                struct eos_shard_v2_record_fst_entry *entry_out)
{
  EosShardShardFileImplV2 *self = EOS_SHARD_SHARD_FILE_IMPL_V2 (impl);
  int i;

  /* Do a linear search because we don't expect many FST entries per record... */
  for (i = 0; i < srecord->fst_length; i++) {
    struct eos_shard_v2_record_fst_entry entry;
    pread (self->fd, &entry, sizeof (entry), srecord->fst_start + i*sizeof (entry));

    if (strncmp ((char *) entry.name, name, sizeof (entry.name)) == 0) {
      *entry_out = entry;
      return TRUE;
    }
  }

  return FALSE;
}

static EosShardBlob *
read_blob (EosShardShardFileImpl *impl, struct eos_shard_v2_record *srecord, const char *name)
{
  EosShardShardFileImplV2 *self = EOS_SHARD_SHARD_FILE_IMPL_V2 (impl);

  struct eos_shard_v2_record_fst_entry entry;
  if (!find_fst_entry (impl, srecord, name, &entry))
    return NULL;

  struct eos_shard_v2_blob sblob;
  pread (self->fd, &sblob, sizeof (sblob), entry.blob_start);

  return blob_new (self->shard_file, &sblob, entry.blob_start);
}

static inline gboolean
record_is_tombstone (struct eos_shard_v2_record *record)
{
  /* Tombstones indicate that records have been deleted.
   * They're indicated with by a special flag in the record. */
  return (record->flags & EOS_SHARD_V2_RECORD_FLAG_TOMBSTONE) != 0;
}

static EosShardRecord *
record_new (EosShardShardFileImpl *impl, struct eos_shard_v2_record *srecord)
{
  EosShardShardFileImplV2 *self = EOS_SHARD_SHARD_FILE_IMPL_V2 (impl);

  if (record_is_tombstone (srecord))
    return NULL;

  EosShardRecord *record = _eos_shard_record_new ();
  record->shard_file = g_object_ref (self->shard_file);
  record->raw_name = srecord->raw_name;
  record->metadata = read_blob (impl, srecord, EOS_SHARD_V2_FST_METADATA);
  record->data = read_blob (impl, srecord, EOS_SHARD_V2_FST_DATA);
  return record;
}

static int
find_record_by_raw_name_compar (const void *a, const void *b)
{
  const struct eos_shard_v2_record *rec_a = a;
  const struct eos_shard_v2_record *rec_b = b;
  return memcmp (rec_a->raw_name, rec_b->raw_name, EOS_SHARD_RAW_NAME_SIZE);
}

static EosShardRecord *
find_record_by_raw_name (EosShardShardFileImpl *impl, uint8_t *raw_name)
{
  EosShardShardFileImplV2 *self = EOS_SHARD_SHARD_FILE_IMPL_V2 (impl);
  struct eos_shard_v2_record key, *res;

  memcpy (key.raw_name, raw_name, EOS_SHARD_RAW_NAME_SIZE);

  res = bsearch (&key, self->records, self->hdr.records_length, sizeof (*self->records),
                 find_record_by_raw_name_compar);

  if (res == NULL)
    return NULL;
  if (record_is_tombstone (res))
    return NULL;

  return record_new (impl, res);
}

static GSList *
list_records (EosShardShardFileImpl *impl)
{
  EosShardShardFileImplV2 *self = EOS_SHARD_SHARD_FILE_IMPL_V2 (impl);
  GSList *l = NULL;

  int i = self->hdr.records_length;
  while (i-- > 0) {
    struct eos_shard_v2_record *srecord = &self->records[i];
    if (record_is_tombstone (srecord))
      continue;

    EosShardRecord *record = record_new (impl, srecord);
    l = g_slist_prepend (l, record);
  }

  return l;
}

static gsize
read_data (EosShardShardFileImpl *impl, void *buf, gsize count, goffset offset)
{
  EosShardShardFileImplV2 *self = EOS_SHARD_SHARD_FILE_IMPL_V2 (impl);
  return pread (self->fd, buf, count, offset);
}

static void
shard_file_impl_init (EosShardShardFileImplInterface *iface)
{
  iface->find_record_by_raw_name = find_record_by_raw_name;
  iface->list_records = list_records;
  iface->read_data = read_data;
}
