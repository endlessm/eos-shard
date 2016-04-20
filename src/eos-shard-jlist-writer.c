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

#include "eos-shard-jlist-writer.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "eos-shard-jlist-format.h"
#include "eos-shard-bloom-filter.h"

#define CSTRING_SIZE(S) (strlen((S)) + 1)

struct _EosShardJListWriter {
  int ref_count;
  GFileOutputStream *stream;

  uint32_t n_entries_total;
  uint32_t block_length;
  uint16_t n_blocks;

  uint32_t n_entries_added;

  GArray *offsets;

  struct bloom_filter bloom_filter;
};

EosShardJListWriter *
eos_shard_jlist_writer_new_for_stream (GFileOutputStream *stream, int n_entries)
{
  EosShardJListWriter *self = g_new0 (EosShardJListWriter, 1);
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
eos_shard_jlist_writer_begin (EosShardJListWriter *self)
{
  /* We have to write the header at the end when we know the location of our
   * block table, so advance past its location so we can start writing entries. */
  uint64_t header_offs = sizeof (struct jlist_header);
  g_seekable_seek (G_SEEKABLE (self->stream), header_offs, G_SEEK_SET, NULL, NULL);
}

void
eos_shard_jlist_writer_add_entry (EosShardJListWriter *self, char *key, char *value)
{
  if ((self->n_entries_added % self->block_length) == 0) {
    uint64_t current_offset = g_seekable_tell (G_SEEKABLE (self->stream));
    g_array_append_val (self->offsets, current_offset);
  }

  GOutputStream *out = G_OUTPUT_STREAM (self->stream);
  g_output_stream_write (out, key, CSTRING_SIZE (key), NULL, NULL);
  g_output_stream_write (out, value, CSTRING_SIZE (value), NULL, NULL);
  self->n_entries_added++;
}

void
eos_shard_jlist_writer_finish (EosShardJListWriter *self)
{
  int i;

  g_assert (self->n_entries_added == self->n_entries_total);
  g_assert (self->offsets->len == self->n_blocks);

  /* Create a fake offset value to calculate the last block's length. */
  uint64_t current_offset = g_seekable_tell (G_SEEKABLE (self->stream));
  g_array_append_val (self->offsets, current_offset);

  /* Write out our blocks. */
  uint64_t block_table_start = current_offset;

  GOutputStream *out = G_OUTPUT_STREAM (self->stream);

  g_output_stream_write (out, &self->n_blocks, sizeof (uint16_t), NULL, NULL);
  for (i = 0; i < self->n_blocks; i++) {
    struct jlist_block_table_entry block = {};
    block.offset = g_array_index (self->offsets, uint64_t, i);
    block.length = g_array_index (self->offsets, uint64_t, i+1) - g_array_index (self->offsets, uint64_t, i);
    g_output_stream_write (out, &block, sizeof (struct jlist_block_table_entry), NULL, NULL);
  }

  /* Now write out our bloom filter. */
  uint64_t bloom_filter_start = g_seekable_tell (G_SEEKABLE (self->stream));
  bloom_filter_write_to_stream (&self->bloom_filter, out);

  /* Write out our header now that we know where the block table begins. */
  struct jlist_header header = {};
  strcpy (header.magic, JLIST_MAGIC); 
  header.block_table_start = block_table_start;
  header.bloom_filter_start = bloom_filter_start;
  g_seekable_seek ((GSeekable*) self->stream, 0, G_SEEK_SET, NULL, NULL);
  g_output_stream_write (out, &header, sizeof (struct jlist_header), NULL, NULL);
}

static void
eos_shard_jlist_writer_free (EosShardJListWriter *self)
{
  bloom_filter_dispose (&self->bloom_filter);
  g_array_free (self->offsets, TRUE);
  g_free (self);
}

EosShardJListWriter *
eos_shard_jlist_writer_ref (EosShardJListWriter *self)
{
  self->ref_count++;
  return self;
}

void
eos_shard_jlist_writer_unref (EosShardJListWriter *self)
{
  if (--self->ref_count == 0)
    eos_shard_jlist_writer_free (self);
}

G_DEFINE_BOXED_TYPE (EosShardJListWriter, eos_shard_jlist_writer,
                     eos_shard_jlist_writer_ref, eos_shard_jlist_writer_unref)
