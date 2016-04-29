
#include "eos-shard-alias-table.h"

#define FILTER_RECORD_NAME "d307a0144b2a9bfc5560eb1f7db921f0ca939e0c"
#define JLIST_RECORD_NAME "c307a0144b2a9bfc5560eb1f7db921f0ca939e0c"
#define MIMETYPE "application/octet-stream"
#define FLAGS EOS_SHARD_BLOB_FLAG_NONE

char *
eos_shard_alias_table_find_entry (EosShardAliasTable *self, char *key)
{
  if (eos_shard_bloom_filter_test (self->filter, key)) {
    return eos_shard_jlist_lookup_key (self->jlist, key);
  }
  return NULL;
}

/**
 * eos_shard_alias_table_find_entries:
 * @keys: (array length=n_keys):
 *
 * Returns: (transfer container) (element-type utf8 utf8): table
 */
GHashTable *
eos_shard_alias_table_find_entries (EosShardAliasTable *self, char **keys, int n_keys)
{
  GHashTable *table = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
  int i;
  for (i=0; i<n_keys; i++)
    {
      char *value = eos_shard_alias_table_find_entry (self, keys[i]);
      g_hash_table_insert (table, g_strdup(keys[i]), value);
    }
  return table;
}

void
eos_shard_alias_table_add_entry (EosShardAliasTable *self, char *key, char *value)
{
  eos_shard_jlist_writer_add_entry (self->jlist_writer, key, value);
  eos_shard_bloom_filter_add (self->filter, key);
}

void
eos_shard_alias_table_write_to_shard (EosShardAliasTable *self, EosShardWriter *writer)
{
  eos_shard_jlist_writer_finish (self->jlist_writer);
  eos_shard_bloom_filter_write_to_stream (self->filter, self->filter_out);

  eos_shard_writer_add_record (writer, FILTER_RECORD_NAME);
  eos_shard_writer_add_blob (writer, EOS_SHARD_WRITER_BLOB_DATA, self->filter_file, MIMETYPE, FLAGS);
  eos_shard_writer_add_blob (writer, EOS_SHARD_WRITER_BLOB_METADATA, self->filter_file, MIMETYPE, FLAGS);

  eos_shard_writer_add_record (writer, JLIST_RECORD_NAME);
  eos_shard_writer_add_blob (writer, EOS_SHARD_WRITER_BLOB_DATA, self->jlist_file, MIMETYPE, FLAGS);
  eos_shard_writer_add_blob (writer, EOS_SHARD_WRITER_BLOB_METADATA, self->jlist_file, MIMETYPE, FLAGS);
}

EosShardAliasTable *
eos_shard_alias_table_new_from_shard (EosShardShardFile *shard)
{
  EosShardRecord *filter_record, *jlist_record;
  EosShardAliasTable *self;
  EosShardJList *jlist;
  EosShardBloomFilter *filter;

  filter_record = eos_shard_shard_file_find_record_by_hex_name (shard, FILTER_RECORD_NAME);
  if (filter_record == NULL) {
    g_print("no filter man\n");
    return NULL;
  }
  filter = eos_shard_bloom_filter_new_for_fd (shard->fd, filter_record->data->offs);

  jlist_record = eos_shard_shard_file_find_record_by_hex_name (shard, JLIST_RECORD_NAME);
  if (jlist_record == NULL) {
    g_print("no jlist man\n");
    return NULL;
  }
  jlist = eos_shard_jlist_new_for_fd (shard->fd, jlist_record->data->offs);

  self = g_new0 (EosShardAliasTable, 1);
  self->ref_count = 1;
  self->jlist = jlist;
  self->filter = filter;
  return self;
}

EosShardAliasTable *
eos_shard_alias_table_new (int n_entries)
{
  EosShardAliasTable *self = g_new0 (EosShardAliasTable, 1);

  // TODO free these
  GFileIOStream *jlist_io, *filter_io;
  GFileOutputStream *jlist_out;

  self->ref_count = 1;
  self->filter = eos_shard_bloom_filter_new_for_params (n_entries, 0.01);

  // TODO clean up tmpfiles (when?)
  self->jlist_file = g_file_new_tmp("shard_jlistXXXXXX", &jlist_io, NULL);
  jlist_out = (GFileOutputStream*) g_io_stream_get_output_stream ((GIOStream*) jlist_io);
  self->jlist_writer = eos_shard_jlist_writer_new_for_stream (jlist_out, n_entries);

  self->filter_file = g_file_new_tmp("shard_filterXXXXXX", &filter_io, NULL);
  self->filter_out = (GFileOutputStream*) g_io_stream_get_output_stream ((GIOStream*)filter_io);

  eos_shard_jlist_writer_begin(self->jlist_writer);

  return self;
}

static void
eos_shard_alias_table_free (EosShardAliasTable *self)
{
  g_clear_object (&self->filter_file);
  g_clear_object (&self->jlist_file);
  g_free (self);
}

EosShardAliasTable *
eos_shard_alias_table_ref (EosShardAliasTable *self)
{
  self->ref_count++;
  return self;
}

void
eos_shard_alias_table_unref (EosShardAliasTable *self)
{
  if (--self->ref_count == 0)
    eos_shard_alias_table_free (self);
}

G_DEFINE_BOXED_TYPE (EosShardAliasTable, eos_shard_alias_table,
                     eos_shard_alias_table_ref, eos_shard_alias_table_unref)
