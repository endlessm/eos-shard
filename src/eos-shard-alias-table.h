
#ifndef EOS_SHARD_ALIAS_TABLE_H
#define EOS_SHARD_ALIAS_TABLE_H

#include <gio/gio.h>

#include "eos-shard-types.h"
#include "eos-shard-writer.h"
#include "eos-shard-shard-file.h"
#include "eos-shard-record.h"
#include "eos-shard-blob.h"
#include "eos-shard-jlist.h"
#include "eos-shard-jlist-writer.h"
#include "eos-shard-bloom-filter.h"

/**
 * EosShardAliasTable:
 *
 * foo
 */

GType eos_shard_alias_table_get_type (void) G_GNUC_CONST;

struct _EosShardAliasTable {
  int ref_count;

  GFile *filter_file;
  GFileOutputStream *filter_out;
  EosShardBloomFilter *filter;

  GFile *jlist_file;
  EosShardJListWriter *jlist_writer;

  EosShardJList *jlist;
};

char * eos_shard_alias_table_find_entry (EosShardAliasTable *self, char *key);
void eos_shard_alias_table_add_entry (EosShardAliasTable *self, char *key, char *value);
void eos_shard_alias_table_write_to_shard (EosShardAliasTable *self, EosShardWriter *writer);

GHashTable * eos_shard_alias_table_find_entries (EosShardAliasTable *self, char **keys, int n_keys);
EosShardAliasTable * eos_shard_alias_table_new_from_shard (EosShardShardFile *shard);
EosShardAliasTable * eos_shard_alias_table_new (int n_entries);

#endif /* EOS_SHARD_ALIAS_TABLE_H */
