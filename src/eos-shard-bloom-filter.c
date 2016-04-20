
#include "eos-shard-bloom-filter.h"

#define FNV_32_PRIME 16777619
#define FNV_32_OFFS_BASIS 2166136261LL

uint32_t fnv_mix (uint32_t a) {
  a += a << 13;
  a ^= a >> 7;
  a += a << 3;
  a ^= a >> 17;
  a += a << 5;
  return a;
}

uint32_t
fnv_1a (char *v)
{
  uint32_t a = FNV_32_OFFS_BASIS;
  char *c;
  for (c = v; *c; c++) {
    a = (a ^ *c) * FNV_32_PRIME;
  }
  return fnv_mix(a);
}

uint32_t
fnv_1a_b (uint32_t a)
{
  return fnv_mix(a * FNV_32_PRIME);
}

void
eos_shard_bloom_filter_hash (EosShardBloomFilter *self, char *key) {
  int i;
  int a = fnv_1a(key);
  int b = fnv_1a_b(a);
  int x = a % self->n_bits;
  for (i=0; i < self->n_hashes; i++) {
    self->hashes[i] = x < 0 ? (x + self->n_bits) : x;
    x = (x + b) % self->n_bits;
  }
}

void
eos_shard_bloom_filter_add (EosShardBloomFilter *self, char *key)
{
  int i;
  eos_shard_bloom_filter_hash(self, key);
  for (i=0; i < self->n_hashes; i++) {
    int h = self->hashes[i];
    int bucket_i = floor(h / 32);
    g_assert (bucket_i < self->n_buckets);
    self->buckets[bucket_i] |= 1 << h;
  }
}

gboolean
eos_shard_bloom_filter_test (EosShardBloomFilter *self, char *key)
{
  int i;
  eos_shard_bloom_filter_hash(self, key);
  for (i=0; i < self->n_hashes; i++) {
    int h = self->hashes[i];
    int bucket_i = floor(h / 32);
    g_assert (bucket_i < self->n_buckets);
    if ((self->buckets[bucket_i] & (1 << h)) == 0)
      return FALSE;
  }
  return TRUE;
}

void
eos_shard_bloom_filter_testaroo (void)
{
  int i;
  int n = 1000000;
  double p = 0.001;
  EosShardBloomFilter *filter = eos_shard_bloom_filter_new_for_params(n, p);

  g_test_timer_start();
  for (i=0; i<n; i++) {
    char s[20] = {};
    sprintf(s, "%d", i);
    eos_shard_bloom_filter_add(filter, s);
  }
  g_print("insert %f\n", g_test_timer_elapsed());

  int false_positives = 0;
  int false_negatives = 0;

  g_test_timer_start();
  for (i=0; i<n; i++) {
    char s[20] = {};
    sprintf(s, "butt %d", i);
    if (eos_shard_bloom_filter_test(filter, s)) {
      false_positives++;
    }
  }
  g_print("test1 %f\n", g_test_timer_elapsed());

  g_test_timer_start();
  for (i=0; i<n; i++) {
    char s[20] = {};
    sprintf(s, "%d", i);
    if (!eos_shard_bloom_filter_test(filter, s)) {
      false_negatives++;
    }
  }
  g_print("test2 %f\n", g_test_timer_elapsed());

  g_print("false positives %f\n", (double)false_positives/(double)n);
  g_print("false negatives %f\n", (double)false_negatives/(double)n);
}

EosShardBloomFilter *
eos_shard_bloom_filter_new_for_params (int n, double p)
{
  EosShardBloomFilter *filter = g_new0 (EosShardBloomFilter, 1);
  filter->ref_count = 1;

  int optimal_n_bits = ceil(-1.0 * ((double)n * log(p)) / (M_LN2 * M_LN2));
  filter->n_bits = ((optimal_n_bits / 32) + 1) * 32; // round up to next 32 bit block
  filter->n_buckets = filter->n_bits/32;
  filter->n_hashes = ceil(((double)filter->n_bits/(double)n) * M_LN2);

  filter->buckets = (uint32_t*)calloc(filter->n_buckets, sizeof(uint32_t));
  filter->hashes = (uint32_t*)calloc(filter->n_hashes, sizeof(uint32_t));

  g_print("actual m: %d actual k: %d\n", filter->n_bits, filter->n_hashes);

  return filter;
}

static void
eos_shard_bloom_filter_free (EosShardBloomFilter *bloom_filter)
{
  // TODO actually free shit
  g_free (bloom_filter);
}

EosShardBloomFilter *
eos_shard_bloom_filter_ref (EosShardBloomFilter *bloom_filter)
{
  bloom_filter->ref_count++;
  return bloom_filter;
}

void
eos_shard_bloom_filter_unref (EosShardBloomFilter *bloom_filter)
{
  if (--bloom_filter->ref_count == 0)
    eos_shard_bloom_filter_free (bloom_filter);
}

G_DEFINE_BOXED_TYPE (EosShardBloomFilter, eos_shard_bloom_filter,
                     eos_shard_bloom_filter_ref, eos_shard_bloom_filter_unref)
