
#include "eos-shard-bloom-filter.h"

#define FNV_32_PRIME 16777619
#define FNV_32_OFFS_BASIS 2166136261LL

static uint32_t
fnv_mix (uint32_t a) {
  a += a << 13;
  a ^= a >> 7;
  a += a << 3;
  a ^= a >> 17;
  a += a << 5;
  return a;
}

static uint32_t
fnv_1a (char *v)
{
  uint32_t a = FNV_32_OFFS_BASIS;
  char *c;
  for (c = v; *c; c++) {
    a = (a ^ *c) * FNV_32_PRIME;
  }
  return fnv_mix(a);
}

static uint32_t
fnv_1a_b (uint32_t a)
{
  return fnv_mix(a * FNV_32_PRIME);
}

static void
bloom_filter_hash (EosShardBloomFilter *self, char *key) {
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
  bloom_filter_hash(self, key);
  for (i=0; i < self->n_hashes; i++) {
    int h = self->hashes[i];
    int bucket_i = floor(h / 32);
    g_assert (bucket_i < self->n_buckets);
    self->buckets[bucket_i] |= 1 << (h % 32);
  }
}

gboolean
eos_shard_bloom_filter_test (EosShardBloomFilter *self, char *key)
{
  int i;
  bloom_filter_hash(self, key);
  for (i=0; i < self->n_hashes; i++) {
    int h = self->hashes[i];
    int bucket_i = floor(h / 32);
    g_assert (bucket_i < self->n_buckets);
    if ((self->buckets[bucket_i] & (1 << (h % 32))) == 0)
      return FALSE;
  }
  return TRUE;
}

/**
 * eos_shard_bloom_filter_test_with_jlist:
 * @filter:
 * @words: (array zero-terminated=0):
 * @n_words:
 *
 * Returns: (transfer full): table
 */
EosShardJList *
eos_shard_bloom_filter_test_with_jlist (EosShardBloomFilter *filter, char **words, int n_words)
{
  int i;
  int fd = open("out.jlist", O_RDONLY); 
  double pct_members = 0.1;
  EosShardJList *jlist = _eos_shard_jlist_new(fd, 0);
  int kb = filter->n_bits / (8 * 1024);
  g_print("%d members total, %.2f%% false positive rate, %dKB\n", filter->n_elements, filter->fp_rate * 100.0, kb);

  g_test_timer_start();
  for (i=0; i<n_words; i++) {
    eos_shard_bloom_filter_test(filter, words[i]);
  }
  g_print("just bloom filter tests %fs\n", g_test_timer_elapsed());

  int false_negatives = 0;
  g_test_timer_start();
  for (i=0; i<n_words; i++) {
    if (eos_shard_bloom_filter_test(filter, words[i])) {
      char *v = eos_shard_jlist_lookup_key(jlist, words[i]);
      if (v == NULL) {
        false_negatives++;
      }
      free(v);
    } else {
      false_negatives++;
    }
  }
  g_print("all members %fs\n", g_test_timer_elapsed());
  g_assert(false_negatives == 0);

  int false_positives = 0;
  g_test_timer_start();
  for (i=0; i<n_words; i++) {
    // 1% of our lookups should be real
    double r=((double)rand()/(double)RAND_MAX);
    if (r < 1 - pct_members) {
      words[i][0]++;
    }

    if (eos_shard_bloom_filter_test(filter, words[i])) {
      char *v = eos_shard_jlist_lookup_key(jlist, words[i]);
      if (v == NULL) {
        false_positives++;
      }
      free(v);
    }
  }

  g_print("%.0f%% members %fs\n", pct_members * 100.0, g_test_timer_elapsed());
  g_print("measured false positive rate %.2f%%\n", 100.0 * (double)false_positives/(double)n_words);
  return jlist;
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

  filter->n_elements = n;
  filter->fp_rate = p;

  int optimal_n_bits = ceil(-1.0 * ((double)n * log(p)) / (M_LN2 * M_LN2));
  filter->n_bits = ((optimal_n_bits / 32) + 1) * 32; // round up to next 32 bit block
  filter->n_buckets = filter->n_bits/32;
  filter->n_hashes = ceil(((double)filter->n_bits/(double)n) * M_LN2);

  filter->buckets = (uint32_t*)calloc(filter->n_buckets, sizeof(uint32_t));
  filter->hashes = (uint32_t*)calloc(filter->n_hashes, sizeof(uint32_t));

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
