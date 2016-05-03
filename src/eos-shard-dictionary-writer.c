/* Copyright 2016 Endless Mobile, Inc. */

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

#include "eos-shard-dictionary-writer.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "eos-shard-dictionary-format.h"
#include "eos-shard-bloom-filter.h"
#include "eos-shard-enums.h"

#define CSTRING_SIZE(S) (strlen((S)) + 1)

struct _EosShardDictionaryWriter {
  int ref_count;
  GFileOutputStream *stream;

  uint32_t n_entries_total;
  uint32_t block_length;
  uint16_t n_blocks;

  uint32_t n_entries_added;
  char *last_key;

  GArray *offsets;

  struct bloom_filter bloom_filter;
};

EosShardDictionaryWriter *
eos_shard_dictionary_writer_new_for_stream (GFileOutputStream *stream, int n_entries)
{
  EosShardDictionaryWriter *self = g_new0 (EosShardDictionaryWriter, 1);
  self->ref_count = 1;

  self->stream = stream;

  self->n_entries_total = n_entries;

  /* We currently use a block length of sqrt(), which is the most ideal choice
   * to have the same number of blocks as items in each block.
   *
   * XXX: Given that bsearching for blocks and scanning across blocks have two
   * completely different performance characteristics, it remains to be seen
   * whether this is actually the most performant approach. */
  self->block_length = ceil (sqrt (n_entries));
  self->n_blocks = ceil ((double) n_entries / self->block_length);

  self->offsets = g_array_sized_new (TRUE, TRUE, sizeof (uint64_t), self->n_blocks + 1);

  /* XXX: Currently these params are hardcoded. */
  bloom_filter_init_for_params (&self->bloom_filter, n_entries, 0.01);

  return self;
}

void
eos_shard_dictionary_writer_begin (EosShardDictionaryWriter *self)
{
  /* We have to write the header at the end when we know the location of our
   * block table, so advance past its location so we can start writing entries. */
  uint64_t header_offs = sizeof (struct dictionary_header);
  g_seekable_seek (G_SEEKABLE (self->stream), header_offs, G_SEEK_SET, NULL, NULL);
}

void
eos_shard_dictionary_writer_add_entry (EosShardDictionaryWriter *self,
                                       char *key,
                                       char *value,
                                       GError **error)
{
  /* If we already have some entries, make sure this key is alphabetically after
   * the last added key.
   */
  if (self->last_key) {
    if (strcmp (key, self->last_key) < 0) {
      g_set_error (error, EOS_SHARD_ERROR,
                   EOS_SHARD_ERROR_DICTIONARY_WRITER_ENTRIES_OUT_OF_ORDER,
                   "Added key %s is less than last key %s", key, self->last_key);
      return;
    }
  }

  if ((self->n_entries_added % self->block_length) == 0) {
    uint64_t current_offset = g_seekable_tell (G_SEEKABLE (self->stream));
    g_array_append_val (self->offsets, current_offset);
  }

  /* Add the key to the bloom filter */
  bloom_filter_add (&self->bloom_filter, key);

  GOutputStream *out = G_OUTPUT_STREAM (self->stream);
  g_output_stream_write (out, key, CSTRING_SIZE (key), NULL, NULL);
  g_output_stream_write (out, value, CSTRING_SIZE (value), NULL, NULL);
  self->n_entries_added++;

  g_free (self->last_key);
  self->last_key = g_strdup (key);
}

void
eos_shard_dictionary_writer_finish (EosShardDictionaryWriter *self, GError **error)
{
  int i;

  if (self->n_entries_added != self->n_entries_total) {
    g_set_error (error, EOS_SHARD_ERROR,
                 EOS_SHARD_ERROR_DICTIONARY_WRITER_WRONG_NUMBER_ENTRIES,
                "Incorrect number of entries: got %d, expected %d",
                self->n_entries_added, self->n_entries_total);
    return;
  }
  g_assert (self->offsets->len == self->n_blocks);

  /* Create a fake offset value to calculate the last block's length. */
  uint64_t current_offset = g_seekable_tell (G_SEEKABLE (self->stream));
  g_array_append_val (self->offsets, current_offset);

  /* Write out our blocks. */
  uint64_t block_table_start = current_offset;

  GOutputStream *out = G_OUTPUT_STREAM (self->stream);

  g_output_stream_write (out, &self->n_blocks, sizeof (uint16_t), NULL, NULL);
  for (i = 0; i < self->n_blocks; i++) {
    struct dictionary_block_table_entry block = {};
    block.offset = g_array_index (self->offsets, uint64_t, i);
    block.length = g_array_index (self->offsets, uint64_t, i+1) - g_array_index (self->offsets, uint64_t, i);
    g_output_stream_write (out, &block, sizeof (struct dictionary_block_table_entry), NULL, NULL);
  }

  /* Now write out our bloom filter. */
  uint64_t bloom_filter_start = g_seekable_tell (G_SEEKABLE (self->stream));
  bloom_filter_write_to_stream (&self->bloom_filter, out);

  /* Write out our header now that we know where the block table begins. */
  struct dictionary_header header = {};
  strcpy (header.magic, DICTIONARY_MAGIC);
  header.block_table_start = block_table_start;
  header.bloom_filter_start = bloom_filter_start;
  g_seekable_seek (G_SEEKABLE (self->stream), 0, G_SEEK_SET, NULL, NULL);
  g_output_stream_write (out, &header, sizeof (struct dictionary_header), NULL, NULL);
}

static void
eos_shard_dictionary_writer_free (EosShardDictionaryWriter *self)
{
  g_free (self->last_key);
  bloom_filter_dispose (&self->bloom_filter);
  g_array_free (self->offsets, TRUE);
  g_free (self);
}

EosShardDictionaryWriter *
eos_shard_dictionary_writer_ref (EosShardDictionaryWriter *self)
{
  self->ref_count++;
  return self;
}

void
eos_shard_dictionary_writer_unref (EosShardDictionaryWriter *self)
{
  if (--self->ref_count == 0)
    eos_shard_dictionary_writer_free (self);
}

G_DEFINE_BOXED_TYPE (EosShardDictionaryWriter, eos_shard_dictionary_writer,
                     eos_shard_dictionary_writer_ref, eos_shard_dictionary_writer_unref)
