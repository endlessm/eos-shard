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
  GVariant *header_variant;

  GList *init_results;
  GError *init_error;
  guint init_state;

  char *path;
  int fd;
};

enum
{
  PROP_0,
  PROP_PATH,
  LAST_PROP,
};

enum
{
   NOT_INITIALIZED,
   INITIALIZING,
   INITIALIZED
};

static GParamSpec *obj_props[LAST_PROP] = { NULL, };

static void initable_iface_init (GInitableIface *iface);
static void async_initable_iface_init (GAsyncInitableIface *iface);

G_DEFINE_TYPE_WITH_CODE (EosShardShardFile, eos_shard_shard_file, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (G_TYPE_ASYNC_INITABLE, async_initable_iface_init)
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
  self->header_variant = g_variant_new_from_bytes (G_VARIANT_TYPE (EOS_SHARD_HEADER_ENTRY), bytes, FALSE);

  const char *magic;
  g_variant_get (self->header_variant, "(&sa" EOS_SHARD_RECORD_ENTRY ")",
                 &magic, NULL);

  if (strcmp (magic, EOS_SHARD_MAGIC) != 0) {
    close (self->fd);
    g_set_error (error, EOS_SHARD_ERROR, EOS_SHARD_ERROR_SHARD_FILE_CORRUPT,
                 "The shard file at %s is corrupt.", self->path);
    return FALSE;
  }

  return TRUE;
}

static void
eos_shard_shard_file_init_async_task_fn (GTask        *task,
                                         gpointer      object,
                                         gpointer      task_data,
                                         GCancellable *cancellable)
{
  GError *error = NULL;

  if (eos_shard_shard_file_init_internal (G_INITABLE (object), cancellable, &error))
    g_task_return_boolean (task, TRUE);
  else
    g_task_return_error (task, error);
}

static void
eos_shard_shard_file_init_async_ready (GObject      *source,
                                       GAsyncResult *result,
                                       gpointer      user_data)
{
  EosShardShardFile *self = EOS_SHARD_SHARD_FILE (source);
  GList *l;
  self->init_state = INITIALIZED;
  g_task_propagate_boolean (G_TASK (result), &self->init_error);

  for (l = self->init_results; l != NULL; l = l->next)
    {
      GTask *task = l->data;

      if (self->init_error)
        g_task_return_error (task, g_error_copy (self->init_error));
      else
        g_task_return_boolean (task, TRUE);
      g_object_unref (task);
    }
  g_list_free (self->init_results);
  self->init_results = NULL;
}

static void
eos_shard_shard_file_init_async_internal (GAsyncInitable      *initable,
                                          int                 io_priority,
                                          GCancellable       *cancellable,
                                          GAsyncReadyCallback callback,
                                          gpointer            user_data)
{
  EosShardShardFile *self = EOS_SHARD_SHARD_FILE (initable);
  GTask *task = g_task_new (initable, cancellable, callback, user_data);
  GTask *init_task;

  switch (self->init_state)
    {
      case NOT_INITIALIZED:
        self->init_results = g_list_append (self->init_results,
                                            task);
        self->init_state = INITIALIZING;
        init_task = g_task_new (initable, cancellable, eos_shard_shard_file_init_async_ready, NULL);
        g_task_run_in_thread (init_task, eos_shard_shard_file_init_async_task_fn);
        g_object_unref (init_task);
        break;
      case INITIALIZING:
        self->init_results = g_list_append (self->init_results,
                                            task);
        break;
      case INITIALIZED:
        if (self->init_error)
          g_task_return_error (task, g_error_copy (self->init_error));
        else
          g_task_return_boolean (task, TRUE);
        g_object_unref (task);
        break;
    }
}

static gboolean
eos_shard_shard_file_init_finish_internal (GAsyncInitable       *initable,
                                           GAsyncResult         *result,
                                           GError              **error)
{
  g_return_val_if_fail (g_task_is_valid (result, initable), FALSE);

  return g_task_propagate_boolean (G_TASK (result), error);
}

static void
initable_iface_init (GInitableIface *iface)
{
  iface->init = eos_shard_shard_file_init_internal;
}

static void
async_initable_iface_init (GAsyncInitableIface *iface)
{
  iface->init_async = eos_shard_shard_file_init_async_internal;
  iface->init_finish = eos_shard_shard_file_init_finish_internal;
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
  g_clear_pointer (&self->path, g_free);
  g_clear_pointer (&self->header_variant, g_variant_unref);
  g_clear_error (&self->init_error);
  g_list_free_full (self->init_results, g_object_unref);

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
eos_shard_util_raw_name_to_hex_name (char *hex_name, const uint8_t *raw_name)
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
eos_shard_util_hex_name_to_raw_name (uint8_t raw_name[20], const char *hex_name)
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
                       "(@ay@" EOS_SHARD_BLOB_ENTRY "@" EOS_SHARD_BLOB_ENTRY ")",
                       &raw_name_variant, NULL, NULL);
  raw_name = g_variant_get_fixed_array (raw_name_variant, &n_elts, 1);

  /* If we have a corrupt shard, make the lookup return NULL. */
  if (n_elts != EOS_SHARD_RAW_NAME_SIZE)
    return -1;

  return memcmp (key->raw_name, raw_name, EOS_SHARD_RAW_NAME_SIZE);
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
  FindRecordByNameKey key;
  void *res;

  memcpy (key.raw_name, raw_name, EOS_SHARD_RAW_NAME_SIZE);
  g_variant_get (self->header_variant, "(&s@a" EOS_SHARD_RECORD_ENTRY ")",
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
  return _eos_shard_record_new_for_variant (self, child);
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

  g_autoptr(GVariantIter) records_iter;
  g_variant_get (self->header_variant, "(&sa" EOS_SHARD_RECORD_ENTRY ")",
                 NULL, &records_iter);

  GVariant *child;
  while ((child = g_variant_iter_next_value (records_iter))) {
    EosShardRecord *record = _eos_shard_record_new_for_variant (self, child);
    if (!record)
      return NULL;

    l = g_slist_prepend (l, record);
    g_variant_unref (child);
  }

  return g_slist_reverse (l);
}

GBytes *
_eos_shard_shard_file_load_blob (EosShardShardFile *self, EosShardBlob *blob, GError **error)
{
  uint8_t *buf = g_malloc (blob->size);

  size_t size_read = _eos_shard_shard_file_read_data (self, buf, blob->size, blob->offs);
  int read_error = errno;
  if (size_read == -1) {
    g_set_error (error, EOS_SHARD_ERROR, EOS_SHARD_ERROR_BLOB_STREAM_READ,
                 "Read failed: %s", strerror (read_error));
    return NULL;
  }

  GBytes *bytes = g_bytes_new_take (buf, blob->size);

  g_autoptr(GChecksum) checksum = g_checksum_new (G_CHECKSUM_SHA256);
  uint8_t checksum_buf[32];
  g_checksum_update (checksum, buf, blob->size);
  size_t checksum_buf_len = sizeof (checksum_buf);
  g_checksum_get_digest (checksum, checksum_buf, &checksum_buf_len);
  g_assert (checksum_buf_len == sizeof (checksum_buf));

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
  return pread (self->fd, buf, count, offset);
}
