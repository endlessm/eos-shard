
#include "eos-shard-jlist-writer.h"

EosShardJListWriter *
eos_shard_jlist_writer_new_for_stream (GFileOutputStream *stream, int n_entries)
{
  EosShardJListWriter *self = g_new0 (EosShardJListWriter, 1);
  self->ref_count = 1;
  self->stream = stream;
  self->entries_added = 0;

  // Ideally we want the number of blocks to equal the number of entries per block
  self->block_size = ceil (sqrt (n_entries));
  self->offsets = (uint32_t*) calloc (self->block_size, sizeof (uint32_t));
  self->offsets_i = 0;

  return self;
}

void eos_shard_jlist_writer_begin (EosShardJListWriter *self)
{
  // We have to write the header at the end when we know the location of our
  // block table, so advance past its location so we can start writing entries.
  uint64_t hdr_offs = sizeof (struct jlist_hdr);
  g_seekable_seek ((GSeekable*) self->stream, hdr_offs, G_SEEK_SET, NULL, NULL);
}

void eos_shard_jlist_writer_add_entry (EosShardJListWriter *self, char *key, char *value)
{
  GOutputStream *out = (GOutputStream*) self->stream;
  uint64_t current_offset;

  // At each block, note the current offset to be listed in the block table.
  if (self->entries_added % self->block_size == 0)
    {
      current_offset = g_seekable_tell ((GSeekable*) self->stream);
      self->offsets[self->offsets_i++] = current_offset;
    }

  g_output_stream_write (out, key, CSTRING_SIZE(key), NULL, NULL);
  g_output_stream_write (out, value, CSTRING_SIZE(value), NULL, NULL);
  self->entries_added++;
}

void eos_shard_jlist_writer_finish (EosShardJListWriter *self)
{
  int i;
  GOutputStream *out = (GOutputStream*) self->stream;
  struct jlist_hdr hdr = {};
  uint16_t n_blocks = self->offsets_i;
  struct jlist_block_table_entry blocks[n_blocks];
  uint64_t current_offset;

  // Create a fake offset value to calculate the last block's length
  current_offset = g_seekable_tell ((GSeekable*) self->stream);
  self->offsets[self->offsets_i] = current_offset;

  for (i=0; i < self->offsets_i; i++) 
    {
      struct jlist_block_table_entry block = {};
      block.offset = self->offsets[i];
      block.length = self->offsets[i+1] - self->offsets[i];
      blocks[i] = block;
    }

  // First write our header now that we know where the block table begins.
  strcpy (hdr.magic, JLIST_MAGIC); 
  hdr.block_table_start = current_offset;
  g_seekable_seek ((GSeekable*) self->stream, 0, G_SEEK_SET, NULL, NULL);
  g_output_stream_write (out, &hdr, sizeof (struct jlist_hdr), NULL, NULL);

  // Then advance forward to where the block table starts to write it.
  g_seekable_seek ((GSeekable*) self->stream, current_offset, G_SEEK_SET, NULL, NULL);
  g_output_stream_write (out, &n_blocks, sizeof (uint16_t), NULL, NULL);
  g_output_stream_write (out, blocks, n_blocks * sizeof (struct jlist_block_table_entry), NULL, NULL);
}

static void
eos_shard_jlist_writer_free (EosShardJListWriter *self)
{
  g_free (self->offsets);
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
