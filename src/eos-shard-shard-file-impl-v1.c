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

#include "config.h"

#include "eos-shard-shard-file-impl-v1.h"

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "eos-shard-shard-file.h"
#include "eos-shard-shard-file-impl.h"
#include "eos-shard-enums.h"
#include "eos-shard-blob.h"
#include "eos-shard-record.h"
#include "eos-shard-format-v1.h"

struct _EosShardShardFileImplV1
{
  GObject parent;

  EosShardShardFile *shard_file;
  int fd;

  GVariant *header_variant;
};

static void shard_file_impl_init (EosShardShardFileImplInterface *iface);

G_DEFINE_TYPE_WITH_CODE (EosShardShardFileImplV1, eos_shard_shard_file_impl_v1, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (EOS_SHARD_TYPE_SHARD_FILE_IMPL, shard_file_impl_init))

static void
eos_shard_shard_file_impl_v1_finalize (GObject *object)
{
  EosShardShardFileImplV1 *self = EOS_SHARD_SHARD_FILE_IMPL_V1 (object);

  g_clear_pointer (&self->header_variant, g_variant_unref);

  G_OBJECT_CLASS (eos_shard_shard_file_impl_v1_parent_class)->finalize (object);
}

static void
eos_shard_shard_file_impl_v1_class_init (EosShardShardFileImplV1Class *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->finalize = eos_shard_shard_file_impl_v1_finalize;
}

static void
eos_shard_shard_file_impl_v1_init (EosShardShardFileImplV1 *shard_file)
{
}

/* XXX: Yes, I know this really should be using GInitable. */
EosShardShardFileImpl *
_eos_shard_shard_file_impl_v1_new (EosShardShardFile *shard_file,
                                   int fd,
                                   GError **error)
{
  EosShardShardFileImplV1 *self = g_object_new (EOS_SHARD_TYPE_SHARD_FILE_IMPL_V1, NULL);
  self->fd = fd;
  self->shard_file = shard_file;

  uint64_t header_size;
  g_assert (read (self->fd, &header_size, sizeof (header_size)) >= 0);
  header_size = GUINT64_FROM_LE (header_size);

  uint8_t *header_data = g_malloc (header_size);
  g_assert (read (self->fd, header_data, header_size) >= 0);

  g_autoptr(GBytes) bytes = g_bytes_new_take (header_data, header_size);
  self->header_variant = g_variant_new_from_bytes (G_VARIANT_TYPE (EOS_SHARD_V1_HEADER_ENTRY), bytes, FALSE);

  const char *magic;
  g_variant_get (self->header_variant, "(&sa" EOS_SHARD_V1_RECORD_ENTRY ")",
                 &magic, NULL);

  if (strcmp (magic, EOS_SHARD_V1_MAGIC) != 0) {
    g_set_error (error, EOS_SHARD_ERROR, EOS_SHARD_ERROR_SHARD_FILE_CORRUPT,
                 "The shard file is corrupt.");
    g_object_unref (self);
    return NULL;
  }

  return EOS_SHARD_SHARD_FILE_IMPL (self);
}

static EosShardBlob *
blob_new_for_variant (EosShardShardFile *shard_file,
                      GVariant          *blob_variant)
{
  g_autoptr(EosShardBlob) blob = _eos_shard_blob_new ();
  g_autoptr(GVariant) checksum_variant;
  size_t n_elts;
  const void *checksum;

  blob->shard_file = g_object_ref (shard_file);
  g_variant_get (blob_variant, "(s@ayuttt)",
                 &blob->content_type,
                 &checksum_variant,
                 &blob->flags,
                 &blob->offs,
                 &blob->size,
                 &blob->uncompressed_size);
  blob->hdr_size = g_variant_get_size (blob_variant);
  if (!blob->offs)
    return NULL;
  checksum = g_variant_get_fixed_array (checksum_variant, &n_elts, 1);
  if (n_elts != 32)
    return NULL;
  memcpy (blob->checksum, checksum, sizeof (blob->checksum));
  return g_steal_pointer (&blob);
}

static EosShardRecord *
record_new_for_variant (EosShardShardFile *shard_file, GVariant *record_variant)
{
  g_autoptr(EosShardRecord) record = _eos_shard_record_new ();
  g_autoptr(GVariant) raw_name_variant;
  g_autoptr(GVariant) metadata_variant;
  g_autoptr(GVariant) data_variant;
  size_t n_elts;
  const void *raw_name;

  record->shard_file = g_object_ref (shard_file);
  g_variant_get (record_variant, "(@ay@" EOS_SHARD_V1_BLOB_ENTRY "@" EOS_SHARD_V1_BLOB_ENTRY ")",
                 &raw_name_variant,
                 &metadata_variant,
                 &data_variant);
  raw_name = g_variant_get_fixed_array (raw_name_variant, &n_elts, 1);
  if (n_elts != EOS_SHARD_RAW_NAME_SIZE)
    return NULL;

  record->raw_name = raw_name;
  record->metadata = blob_new_for_variant (shard_file, metadata_variant);
  record->data = blob_new_for_variant (shard_file, data_variant);
  return g_steal_pointer (&record);
}

typedef struct {
  GVariant *records;
  uint8_t raw_name[EOS_SHARD_RAW_NAME_SIZE];
} FindRecordByNameKey;

static int
find_record_by_raw_name_compar (const void *key_, const void *item)
{
  const FindRecordByNameKey *key = key_;
  unsigned index = GPOINTER_TO_UINT (item) - 1;
  g_autoptr(GVariant) raw_name_variant;
  size_t n_elts;
  const uint8_t *raw_name;

  g_variant_get_child (key->records, index,
                       "(@ay@" EOS_SHARD_V1_BLOB_ENTRY "@" EOS_SHARD_V1_BLOB_ENTRY ")",
                       &raw_name_variant, NULL, NULL);
  raw_name = g_variant_get_fixed_array (raw_name_variant, &n_elts, 1);

  /* If we have a corrupt shard, make the lookup return NULL. */
  if (n_elts != EOS_SHARD_RAW_NAME_SIZE)
    return -1;

  return memcmp (key->raw_name, raw_name, EOS_SHARD_RAW_NAME_SIZE);
}

static EosShardRecord *
find_record_by_raw_name (EosShardShardFileImpl *impl, uint8_t *raw_name)
{
  EosShardShardFileImplV1 *self = EOS_SHARD_SHARD_FILE_IMPL_V1 (impl);
  FindRecordByNameKey key;
  void *res;

  memcpy (key.raw_name, raw_name, EOS_SHARD_RAW_NAME_SIZE);
  g_variant_get (self->header_variant, "(&s@a" EOS_SHARD_V1_RECORD_ENTRY ")",
                 NULL, &key.records);

  /* This is a bit disgusting, but we're basically using dummy pointers to bsearch
   * through the GVariant's children. The "pointer" is actually just a 1-based index
   * of the child. We use 1-based indexes since NULL represents "no match", so we
   * need to distinguish the first child from no match.
   */
  res = bsearch (&key,
                 GUINT_TO_POINTER (1), g_variant_n_children (key.records), 1,
                 find_record_by_raw_name_compar);

  if (res == NULL)
    return NULL;

  int idx = GPOINTER_TO_UINT (res) - 1;
  g_autoptr(GVariant) child = g_variant_get_child_value (key.records, idx);
  g_variant_unref (key.records);
  return record_new_for_variant (self->shard_file, child);
}

static GSList *
list_records (EosShardShardFileImpl *impl)
{
  EosShardShardFileImplV1 *self = EOS_SHARD_SHARD_FILE_IMPL_V1 (impl);
  GSList *l = NULL;

  g_autoptr(GVariantIter) records_iter;
  g_variant_get (self->header_variant, "(&sa" EOS_SHARD_V1_RECORD_ENTRY ")",
                 NULL, &records_iter);

  GVariant *child;
  while ((child = g_variant_iter_next_value (records_iter))) {
    EosShardRecord *record = record_new_for_variant (self->shard_file, child);
    if (!record)
      return NULL;

    l = g_slist_prepend (l, record);
    g_variant_unref (child);
  }

  return g_slist_reverse (l);
}

static EosShardBlob *
lookup_blob (EosShardShardFileImpl *impl, EosShardRecord *record, const char *name)
{
  return NULL;
}

static void
shard_file_impl_init (EosShardShardFileImplInterface *iface)
{
  iface->find_record_by_raw_name = find_record_by_raw_name;
  iface->list_records = list_records;
  iface->lookup_blob = lookup_blob;
}
