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

#include "eos-shard-blob.h"
#include "eos-shard-shard-file.h"

static EosShardBlob *
eos_shard_blob_new (void)
{
  EosShardBlob *blob = g_new0 (EosShardBlob, 1);
  blob->ref_count = 1;
  return blob;
}

static void
eos_shard_blob_free (EosShardBlob *blob)
{
  g_clear_object (&blob->shard_file);
  g_free (blob->content_type);
  g_free (blob);
}

EosShardBlob *
eos_shard_blob_ref (EosShardBlob *blob)
{
  blob->ref_count++;
  return blob;
}

void
eos_shard_blob_unref (EosShardBlob *blob)
{
  if (--blob->ref_count == 0)
    eos_shard_blob_free (blob);
}

/**
 * eos_shard_blob_get_content_type:
 *
 * Get the content type of the blob's data.
 *
 * Returns: the mimetype of the blob
 */
const char *
eos_shard_blob_get_content_type (EosShardBlob *blob)
{
  return (const char *) blob->content_type;
}

/**
 * eos_shard_blob_get_stream:
 *
 * Creates and returns a #GInputStream to the blob's content. If the blob is
 * compressed, the returned stream will be streamed through a
 * #GZlibDecompressor, yielding decompressed data.
 *
 * Returns: (transfer full): a new GInputStream for the blob's data
 */
GInputStream *
eos_shard_blob_get_stream (EosShardBlob *blob)
{
  g_autoptr(GInputStream) blob_stream = G_INPUT_STREAM (_eos_shard_blob_stream_new_for_blob (blob, blob->shard_file));

  if (blob->flags & EOS_SHARD_BLOB_FLAG_COMPRESSED_ZLIB) {
    g_autoptr(GZlibDecompressor) decompressor = g_zlib_decompressor_new (G_ZLIB_COMPRESSOR_FORMAT_ZLIB);
    return g_converter_input_stream_new (blob_stream, G_CONVERTER (decompressor));
  } else {
    return g_object_ref (blob_stream);
  }
}

/**
 * eos_shard_blob_get_flags:
 *
 * Currently, the only flag is one which indicates whether the content is
 * compressed. Since the two blob read methods decompress content
 * automatically, this method is really only useful internally.
 *
 * Returns: the blob's #EosShardBlobFlags
 */
EosShardBlobFlags
eos_shard_blob_get_flags (EosShardBlob *blob)
{
  return (EosShardBlobFlags) blob->flags;
}

gsize
_eos_shard_blob_get_packed_size (EosShardBlob *blob)
{
  return blob->size;
}

goffset
_eos_shard_blob_get_offset (EosShardBlob *blob)
{
  return blob->offs;
}

/**
 * eos_shard_blob_get_content_size:
 *
 * Gives the blob's uncompressed content size.
 *
 * Returns: the blob's content size in bytes
 */
gsize
eos_shard_blob_get_content_size (EosShardBlob *blob)
{
  return blob->uncompressed_size;
}

/**
 * eos_shard_blob_load_contents:
 *
 * Synchronously read and return the contents of this
 * blob as a #GBytes.
 *
 * Returns: (transfer full): the blob's data
 */
GBytes *
eos_shard_blob_load_contents (EosShardBlob  *blob,
                              GError       **error)
{
  return _eos_shard_shard_file_load_blob (blob->shard_file, blob, error);
}

EosShardBlob *
_eos_shard_blob_new_for_variant (EosShardShardFile           *shard_file,
                                 GVariant                    *blob_variant)
{
  EosShardBlob *blob = eos_shard_blob_new ();
  blob->shard_file = g_object_ref (shard_file);
  g_variant_get (blob_variant, EOS_SHARD_BLOB_ENTRY,
                 &blob->content_type,
                 &blob->flags,
                 &blob->adler32,
                 &blob->offs,
                 &blob->size,
                 &blob->uncompressed_size);
  return blob;
}

G_DEFINE_BOXED_TYPE (EosShardBlob, eos_shard_blob,
                     eos_shard_blob_ref, eos_shard_blob_unref)
