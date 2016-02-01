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

#include "eos-shard-writer.h"

#include <fcntl.h>
#include <string.h>

#include "eos-shard-shard-file.h"
#include "eos-shard-format-v2.h"

#define ALIGN(n) (((n) + 0x1f) & ~0x1f)

static off_t lalign(int fd)
{
  off_t off = lseek (fd, 0, SEEK_CUR);
  off = ALIGN (off);
  lseek (fd, off, SEEK_SET);
  return off;
}

struct eos_shard_writer_blob_entry
{
  GFile *file;
  off_t offs;
  EosShardBlobFlags flags;
  uint64_t uncompressed_size;
};

struct eos_shard_writer_record_entry
{
  uint8_t raw_name[EOS_SHARD_RAW_NAME_SIZE];
  int fst_length;
  off_t fst_start;
  struct eos_shard_writer_blob_entry metadata;
  struct eos_shard_writer_blob_entry data;
};

struct _EosShardWriter
{
  GObject parent;

  GArray *entries;
};

G_DEFINE_TYPE (EosShardWriter, eos_shard_writer, G_TYPE_OBJECT);

static void
eos_shard_writer_finalize (GObject *object)
{
  EosShardWriter *self = EOS_SHARD_WRITER (object);
  g_array_unref (self->entries);
  G_OBJECT_CLASS (eos_shard_writer_parent_class)->finalize (object);
}

static void
eos_shard_writer_blob_entry_clear (struct eos_shard_writer_blob_entry *blob)
{
  g_clear_object (&blob->file);
}

static void
eos_shard_writer_record_entry_clear (struct eos_shard_writer_record_entry *entry)
{
  eos_shard_writer_blob_entry_clear (&entry->metadata);
  eos_shard_writer_blob_entry_clear (&entry->data);
}

static void
eos_shard_writer_class_init (EosShardWriterClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = eos_shard_writer_finalize;
}

static void
eos_shard_writer_init (EosShardWriter *self)
{
  self->entries = g_array_new (FALSE, TRUE, sizeof (struct eos_shard_writer_record_entry));
  g_array_set_clear_func (self->entries, (GDestroyNotify) eos_shard_writer_record_entry_clear);
}

/**
 * eos_shard_writer_add_record:
 * @self: an #EosShardWriter
 * @hex_name: the hexadecimal string which will act as this entry's ID
 *
 * Adds a data/metadata file pair to the shard file. These pairs must be added in
 * increasing hex_name order. Once all pairs have been added, call
 * eos_shard_writer_write() to save the shard file to disk.
 *
 * To set the individual fields of the blobs within the record,
 * use eos_shard_writer_add_blob().
 */
void
eos_shard_writer_add_record (EosShardWriter *self,
                             char *hex_name)
{
  struct eos_shard_writer_record_entry record_entry = {};

  eos_shard_util_hex_name_to_raw_name (record_entry.raw_name, hex_name);

  if (self->entries->len > 0) {
    struct eos_shard_writer_record_entry *e = &g_array_index (self->entries, struct eos_shard_writer_record_entry, self->entries->len - 1);
    g_assert (memcmp (record_entry.raw_name, e->raw_name, EOS_SHARD_RAW_NAME_SIZE) > 0);
  }

  g_array_append_val (self->entries, record_entry);
}

static struct eos_shard_writer_blob_entry *
get_blob_entry (struct eos_shard_writer_record_entry *e,
                EosShardWriterBlob which_blob)
{
  switch (which_blob) {
  case EOS_SHARD_WRITER_BLOB_METADATA:
    return &e->metadata;
  case EOS_SHARD_WRITER_BLOB_DATA:
    return &e->data;
  default:
    g_assert_not_reached ();
  }
}

/**
 * eos_shard_writer_add_blob:
 * @self: an #EosShardWriter
 * @which_blob: Which blob in the record
 * @file: a file of contents to write into the shard
 * @flags: flags about how the data should be stored in the file
 *
 * Set the data for a blob entry in the last added record in the file.
 * Records can be added to the file with eos_shard_writer_add_record().
 */
void
eos_shard_writer_add_blob (EosShardWriter *self,
                           EosShardWriterBlob which_blob,
                           GFile *file,
                           EosShardBlobFlags flags)
{
  struct eos_shard_writer_record_entry *e = &g_array_index (self->entries, struct eos_shard_writer_record_entry, self->entries->len - 1);
  struct eos_shard_writer_blob_entry *b = get_blob_entry (e, which_blob);

  b->file = g_object_ref (file);
  b->flags = flags;

  g_autoptr(GFileInfo) info = g_file_query_info (file, "standard::*", 0, NULL, NULL);
  b->uncompressed_size = g_file_info_get_size (info);
}

static void
write_blob (int fd, struct eos_shard_writer_blob_entry *blob)
{
  /* If we don't have any file, that's normal... */
  if (!blob->file)
    return;

  g_autoptr(GError) error = NULL;
  GFileInputStream *file_stream = g_file_read (blob->file, NULL, &error);
  if (!file_stream) {
    g_error ("Could not read from %s: %s", g_file_get_path (blob->file), error->message);
    return;
  }

  g_autoptr(GInputStream) stream;

  if (blob->flags & EOS_SHARD_BLOB_FLAG_COMPRESSED_ZLIB) {
    g_autoptr(GZlibCompressor) compressor = g_zlib_compressor_new (G_ZLIB_COMPRESSOR_FORMAT_ZLIB, -1);
    stream = g_converter_input_stream_new (G_INPUT_STREAM (file_stream), G_CONVERTER (compressor));
    g_object_unref (file_stream);
  } else {
    stream = G_INPUT_STREAM (file_stream);
  }

  struct eos_shard_v2_blob sblob = { };
  sblob.flags = blob->flags;

  blob->offs = lseek (fd, 0, SEEK_CUR);
  int data_offs = ALIGN (blob->offs + sizeof (sblob));
  lseek (fd, data_offs, SEEK_SET);

  /* Make room for the blob entry. */

  uint8_t buf[4096*4];
  int size, total_size = 0;
  g_autoptr(GChecksum) checksum = g_checksum_new (G_CHECKSUM_SHA256);
  while ((size = g_input_stream_read (stream, buf, sizeof (buf), NULL, NULL)) != 0) {
    g_assert (write (fd, buf, size) >= 0);
    g_checksum_update (checksum, buf, size);
    total_size += size;
  }
  size_t checksum_buf_len = sizeof (sblob.csum);
  g_checksum_get_digest (checksum, sblob.csum, &checksum_buf_len);
  g_assert (checksum_buf_len == sizeof (sblob.csum));

  sblob.size = total_size;
  sblob.uncompressed_size = blob->uncompressed_size;
  sblob.data_start = data_offs;

  /* Go back and patch our blob header... */
  int old_pos = lseek (fd, 0, SEEK_CUR);
  lseek (fd, blob->offs, SEEK_SET);
  write (fd, &sblob, sizeof (sblob));
  lseek (fd, old_pos, SEEK_SET);
}

static void
write_fst_entry (int fd,
                 struct eos_shard_writer_record_entry *e,
                 struct eos_shard_writer_blob_entry *b,
                 const char *name)
{
  if (!b->file)
    return;

  e->fst_length++;
  if (e->fst_length == 1)
    e->fst_start = lseek (fd, 0, SEEK_CUR);

  struct eos_shard_v2_record_fst_entry fst = { };
  strncpy ((char *) fst.name, name, sizeof (fst.name));
  fst.blob_start = b->offs;
  write (fd, &fst, sizeof (fst));
}

static void
write_record (int fd,
              struct eos_shard_writer_record_entry *e)
{
  struct eos_shard_v2_record srecord = { };
  memcpy (srecord.raw_name, e->raw_name, EOS_SHARD_RAW_NAME_SIZE);
  srecord.fst_length = e->fst_length;
  srecord.fst_start = e->fst_start;
  write (fd, &srecord, sizeof (srecord));
}

void
eos_shard_writer_write_to_fd (EosShardWriter *self, int fd)
{
  int i;

  struct eos_shard_v2_hdr hdr = { };

  memcpy (hdr.magic, EOS_SHARD_V2_MAGIC, sizeof (hdr.magic));
  hdr.records_length = self->entries->len;

  /* We do this backwards so we can do it in one parse... */
  lseek (fd, ALIGN (sizeof (hdr)), SEEK_SET);

  /* First, do data... */

  for (i = 0; i < self->entries->len; i++) {
    struct eos_shard_writer_record_entry *e = &g_array_index (self->entries, struct eos_shard_writer_record_entry, i);

    write_blob (fd, &e->metadata);
    write_blob (fd, &e->data);
  }

  lalign (fd);

  /* Now write out FST entries... */
  for (i = 0; i < self->entries->len; i++) {
    struct eos_shard_writer_record_entry *e = &g_array_index (self->entries, struct eos_shard_writer_record_entry, i);

    write_fst_entry (fd, e, &e->data, EOS_SHARD_V2_FST_DATA);
    write_fst_entry (fd, e, &e->metadata, EOS_SHARD_V2_FST_METADATA);
  }

  lalign (fd);

  /* Now for records... */
  hdr.records_start = lalign (fd);
  for (i = 0; i < self->entries->len; i++) {
    struct eos_shard_writer_record_entry *e = &g_array_index (self->entries, struct eos_shard_writer_record_entry, i);

    write_record (fd, e);
  }

  lseek (fd, 0, SEEK_SET);
  write (fd, &hdr, sizeof (hdr));
}

/**
 * eos_shard_writer_write:
 * @self: An #EosShardWriter
 * @path: The file path to write the shard to.
 *
 * This finalizes the shard and writes the contents to the file path
 * specified. Meant as the final step in compiling a shard file together.
 */
void
eos_shard_writer_write (EosShardWriter *self, char *path)
{
  int fd = open (path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  eos_shard_writer_write_to_fd (self, fd);
  close (fd);
}
