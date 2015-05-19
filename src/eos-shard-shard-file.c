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

#include "eos-shard-shard-file.h"

#include <errno.h>
#include <endian.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "eos-shard-enums.h"
#include "eos-shard-format.h"
#include "eos-shard-blob.h"
#include "eos-shard-record.h"

struct _EosShardShardFile
{
  GObject parent;

  char *path;
  int fd;
  uint64_t data_offs;
  size_t n_records;
  EosShardRecord **records;
};

enum
{
  PROP_0,
  PROP_PATH,
  LAST_PROP,
};

static GParamSpec *obj_props[LAST_PROP] = { NULL, };

static void initable_iface_init (GInitableIface *iface);

G_DEFINE_TYPE_WITH_CODE (EosShardShardFile, eos_shard_shard_file, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (G_TYPE_INITABLE, initable_iface_init))

static gboolean
eos_shard_shard_file_init_internal (GInitable *initable,
                                    GCancellable *cancellable,
                                    GError **error)
{
  EosShardShardFile *self = EOS_SHARD_SHARD_FILE (initable);

  if (!self->path) {
    g_set_error (error, EOS_SHARD_ERROR, EOS_SHARD_ERROR_SHARD_FILE_PATH_NOT_SET,
                 "No path was given.");
    return FALSE;
  }

  self->fd = open (self->path, O_RDONLY);
  if (self->fd < 0) {
    if (errno == ENOENT)
      g_set_error (error, EOS_SHARD_ERROR, EOS_SHARD_ERROR_SHARD_FILE_NOT_FOUND,
                   "No file was found at path %s", self->path);
    else
      g_set_error (error, EOS_SHARD_ERROR, EOS_SHARD_ERROR_SHARD_FILE_COULD_NOT_OPEN,
                   "Could not open file %s: %m", self->path);

    return FALSE;
  }

  uint64_t header_size;
  read (self->fd, &header_size, sizeof (header_size));
  header_size = le64toh (header_size);

  uint8_t *header_data = g_malloc (header_size);
  read (self->fd, header_data, header_size);

  g_autoptr(GBytes) bytes = g_bytes_new_take (header_data, header_size);
  g_autoptr(GVariant) header_variant = g_variant_new_from_bytes (G_VARIANT_TYPE (EOS_SHARD_HEADER_ENTRY), bytes, FALSE);
  g_autoptr(GVariantIter) records_iter;

  const char *magic;
  g_variant_get (header_variant, "(&sta" EOS_SHARD_RECORD_ENTRY ")",
                 &magic,
                 &self->data_offs,
                 &records_iter);

  if (strcmp (magic, EOS_SHARD_MAGIC) != 0) {
    g_set_error (error, EOS_SHARD_ERROR, EOS_SHARD_ERROR_SHARD_FILE_CORRUPT,
                 "The shard file at %s is corrupt.", self->path);
    close (self->fd);
    return FALSE;
  }

  self->n_records = g_variant_iter_n_children (records_iter);
  self->records = g_new (EosShardRecord *, self->n_records);

  GVariant *child;
  int i = 0;
  while ((child = g_variant_iter_next_value (records_iter))) {
    self->records[i++] = _eos_shard_record_new_for_variant (self, child);
    g_variant_unref (child);
  }

  return TRUE;
}

static void
initable_iface_init (GInitableIface *iface)
{
  iface->init = eos_shard_shard_file_init_internal;
}

static void
eos_shard_shard_file_set_property (GObject      *object,
                                   guint         prop_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  EosShardShardFile *self = EOS_SHARD_SHARD_FILE (object);

  switch (prop_id) {
  case PROP_PATH:
    self->path = g_value_dup_string (value);
    break;

  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
eos_shard_shard_file_get_property (GObject    *object,
                                   guint       prop_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  EosShardShardFile *self = EOS_SHARD_SHARD_FILE (object);

  switch (prop_id) {
  case PROP_PATH:
    g_value_set_string (value, self->path);
    break;

  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
eos_shard_shard_file_finalize (GObject *object)
{
  EosShardShardFile *self = EOS_SHARD_SHARD_FILE (object);

  close (self->fd);
  int i;
  for (i = 0; i < self->n_records; i++)
    eos_shard_record_unref (self->records[i]);
  g_free (self->records);
  g_clear_pointer (&self->path, g_free);

  G_OBJECT_CLASS (eos_shard_shard_file_parent_class)->finalize (object);
}

static void
eos_shard_shard_file_class_init (EosShardShardFileClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->finalize = eos_shard_shard_file_finalize;
  gobject_class->set_property = eos_shard_shard_file_set_property;
  gobject_class->get_property = eos_shard_shard_file_get_property;

  obj_props[PROP_PATH] =
    g_param_spec_string ("path",
                         "Path",
                         "Path to the shard file",
                         NULL,
                         (GParamFlags) (G_PARAM_READWRITE |
                                        G_PARAM_CONSTRUCT_ONLY |
                                        G_PARAM_STATIC_STRINGS));

  g_object_class_install_properties (gobject_class, LAST_PROP, obj_props);
}

static void 
eos_shard_shard_file_init (EosShardShardFile *shard_file)
{
}

/**
 * eos_shard_util_raw_name_to_hex_name:
 * @hex_name: (out caller-allocates) (array fixed-size=41 zero-terminated=1):
 *   Storage for a hexidecimal name, which must be 41 bytes long.
 * @raw_name: The raw name to convert.
 *
 * Converts a raw SHA-1 hash name into a hexadecimal string.
 */
void
eos_shard_util_raw_name_to_hex_name (char *hex_name, uint8_t *raw_name)
{
  int i;

  for (i = 0; i < EOS_SHARD_RAW_NAME_SIZE; i++)
    sprintf (&hex_name[i*2], "%02x", raw_name[i]);
  hex_name[EOS_SHARD_HEX_NAME_SIZE] = '\0';
}

/**
 * eos_shard_util_hex_name_to_raw_name:
 * @raw_name: (out caller-allocates) (array fixed-size=20):
 *   Storage for a raw name, which must be 20 bytes long.
 * @hex_name: The hexidecimal name to convert.
 *
 * Converts a raw SHA-1 hash name into a hexadecimal string. If
 * we could not convert this name, then this function returns %FALSE.
 */
gboolean
eos_shard_util_hex_name_to_raw_name (uint8_t raw_name[20], char *hex_name)
{
  int n = strlen (hex_name);
  if (n < EOS_SHARD_HEX_NAME_SIZE)
    return FALSE;

  int i;
  for (i = 0; i < EOS_SHARD_RAW_NAME_SIZE; i++) {
    char a = hex_name[i*2];
    char b = hex_name[i*2+1];
    if (!g_ascii_isxdigit (a) || !g_ascii_isxdigit (b))
      return FALSE;
    raw_name[i] = (g_ascii_xdigit_value (a) << 4) | g_ascii_xdigit_value (b);
  }

  return TRUE;
}

static int
eos_shard_record_entry_cmp (const void *a_, const void *b_)
{
  const EosShardRecord *a = * (const EosShardRecord **) a_;
  const EosShardRecord *b = * (const EosShardRecord **) b_;
  return memcmp (a->raw_name, b->raw_name, EOS_SHARD_RAW_NAME_SIZE);
}

/**
 * eos_shard_shard_file_find_record_by_raw_name:
 *
 * Finds a #EosShardRecord for the given raw name
 *
 * Returns: (transfer full): the #EosShardRecord with the given raw name
 */
EosShardRecord *
eos_shard_shard_file_find_record_by_raw_name (EosShardShardFile *self, uint8_t *raw_name)
{
  EosShardRecord key = { .raw_name = raw_name };
  EosShardRecord *keyp = &key;

  return bsearch (&keyp,
                  self->records, self->n_records,
                  sizeof (EosShardRecord *),
                  eos_shard_record_entry_cmp);
}

/**
 * eos_shard_shard_file_find_record_by_hex_name:
 *
 * Finds a #EosShardRecord for the given hex name
 *
 * Returns: (transfer full): the #EosShardRecord with the given hex name
 */
EosShardRecord *
eos_shard_shard_file_find_record_by_hex_name (EosShardShardFile *self, char *hex_name)
{
  uint8_t raw_name[EOS_SHARD_RAW_NAME_SIZE];

  if (!eos_shard_util_hex_name_to_raw_name (raw_name, hex_name))
    return NULL;

  return eos_shard_shard_file_find_record_by_raw_name (self, raw_name);
}

/**
 * eos_shard_shard_file_list_records:
 *
 * List all entries inside @self.
 *
 * Returns: (transfer full) (element-type EosShardRecord): a list of #EosShardRecord
 */
GSList *
eos_shard_shard_file_list_records (EosShardShardFile *self)
{
  GSList *l = NULL;
  int i;

  for (i = self->n_records - 1; i >= 0; i--)
    l = g_slist_prepend (l, eos_shard_record_ref (self->records[i]));

  return l;
}

GBytes *
_eos_shard_shard_file_load_blob (EosShardShardFile *self, EosShardBlob *blob, GError **error)
{
  uint8_t *buf = g_malloc (blob->size);

  _eos_shard_shard_file_read_data (self, buf, blob->size, blob->offs);
  GBytes *bytes = g_bytes_new_take (buf, blob->size);

  g_autoptr(GChecksum) checksum = g_checksum_new (G_CHECKSUM_SHA256);
  uint8_t checksum_buf[64];
  g_checksum_update (checksum, buf, blob->size);
  size_t checksum_buf_len = sizeof (checksum_buf);
  g_checksum_get_digest (checksum, checksum_buf, &checksum_buf_len);

  if (memcmp (checksum_buf, blob->checksum, sizeof (checksum_buf) != 0)) {
    g_clear_pointer (&bytes, g_bytes_unref);
    g_set_error (error, EOS_SHARD_ERROR, EOS_SHARD_ERROR_BLOB_CHECKSUM_MISMATCH,
                 "Could not load blob: checksum did not match");
    return NULL;
  }

  if (blob->flags & EOS_SHARD_BLOB_FLAG_COMPRESSED_ZLIB) {
    g_autoptr(GInputStream) bytestream;
    g_autoptr(GZlibDecompressor) decompressor;
    g_autoptr(GInputStream) out_stream;

    decompressor = g_zlib_decompressor_new (G_ZLIB_COMPRESSOR_FORMAT_ZLIB);
    bytestream = g_memory_input_stream_new_from_bytes (bytes);
    g_bytes_unref (bytes);
    out_stream = g_converter_input_stream_new (G_INPUT_STREAM (bytestream), G_CONVERTER (decompressor));

    g_autoptr(GError) err = NULL;
    bytes = g_input_stream_read_bytes (out_stream, blob->uncompressed_size, NULL, &err);
    if (err != NULL)
      g_warning ("Could not decompress stream: %s", err->message);
  }

  return bytes;
}

gsize
_eos_shard_shard_file_read_data (EosShardShardFile *self, void *buf, gsize count, goffset offset)
{
  return pread (self->fd, buf, count, self->data_offs + offset);
}
