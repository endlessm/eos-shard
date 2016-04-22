
#ifndef EOS_SHARD_BLOOM_FILTER_H
#define EOS_SHARD_BLOOM_FILTER_H

#include <gio/gio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <math.h>

#include "eos-shard-types.h"
#include "eos-shard-jlist.h"

/**
 * EosShardBloomFilter:
 *
 * foo
 */

GType eos_shard_bloom_filter_get_type (void) G_GNUC_CONST;

struct _EosShardBloomFilter {
  int ref_count;

  double fp_rate;
  int n_elements;
  int n_bits;
  int n_hashes;
  int n_buckets;
  uint32_t *buckets;
  uint32_t *hashes;
};

void eos_shard_bloom_filter_testaroo (void);
EosShardJList * eos_shard_bloom_filter_test_with_jlist (EosShardBloomFilter *filter, char **words, int n_words);

void eos_shard_bloom_filter_add (EosShardBloomFilter *self, char *key);
gboolean eos_shard_bloom_filter_test (EosShardBloomFilter *self, char *key);
EosShardBloomFilter * eos_shard_bloom_filter_new_for_params (int n, double p);

#endif /* EOS_SHARD_BLOOM_FILTER_H */
