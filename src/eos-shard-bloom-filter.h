
#ifndef EOS_SHARD_BLOOM_FILTER_H
#define EOS_SHARD_BLOOM_FILTER_H

#include <gio/gio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <math.h>

#include "eos-shard-types.h"

/**
 * EosShardBloomFilter:
 *
 * foo
 */

GType eos_shard_bloom_filter_get_type (void) G_GNUC_CONST;

struct eos_shard_bloom_filter_header {
  double fp_rate;
  uint32_t n_elements;
  uint32_t n_bits;
  uint32_t n_hashes;
  uint32_t n_buckets;
};

struct _EosShardBloomFilter {
  int ref_count;

  struct eos_shard_bloom_filter_header header;
  uint32_t *buckets;
  uint32_t *hashes;
};

void eos_shard_bloom_filter_add (EosShardBloomFilter *self, char *key);
gboolean eos_shard_bloom_filter_test (EosShardBloomFilter *self, char *key);
EosShardBloomFilter * eos_shard_bloom_filter_new_for_params (int n, double p);

void eos_shard_bloom_filter_write_to_stream (EosShardBloomFilter *self, GFileOutputStream *stream);
EosShardBloomFilter * eos_shard_bloom_filter_new_for_fd (int fd, off_t offset);

#endif /* EOS_SHARD_BLOOM_FILTER_H */
