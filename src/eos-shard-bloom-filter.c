
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
  return fnv_mix (a);
}

static uint32_t
fnv_1a_b (uint32_t a)
{
  return fnv_mix (a * FNV_32_PRIME);
}

static void
bloom_filter_hash (EosShardBloomFilter *self, char *key) {
  int i;
  int a = fnv_1a (key);
  int b = fnv_1a_b (a);
  int x = a % self->header.n_bits;
  for (i=0; i < self->header.n_hashes; i++) {
    self->hashes[i] = x < 0 ? (x + self->header.n_bits) : x;
    x = (x + b) % self->header.n_bits;
  }
}

void
eos_shard_bloom_filter_add (EosShardBloomFilter *self, char *key)
{
  int i;
  bloom_filter_hash (self, key);
  for (i=0; i < self->header.n_hashes; i++) {
    int h = self->hashes[i];
    int bucket_i = floor (h / 32);
    g_assert (bucket_i < self->header.n_buckets);
    self->buckets[bucket_i] |= 1 << (h % 32);
  }
}

gboolean
eos_shard_bloom_filter_test (EosShardBloomFilter *self, char *key)
{
  int i;
  bloom_filter_hash (self, key);
  for (i=0; i < self->header.n_hashes; i++) {
    int h = self->hashes[i];
    int bucket_i = floor (h / 32);
    g_assert (bucket_i < self->header.n_buckets);
    if ((self->buckets[bucket_i] & (1 << (h % 32))) == 0)
      return FALSE;
  }
  return TRUE;
}

EosShardBloomFilter *
eos_shard_bloom_filter_new_for_params (int n, double p)
{
  EosShardBloomFilter *self = g_new0 (EosShardBloomFilter, 1);
  self->ref_count = 1;

  self->header.n_elements = n;
  self->header.fp_rate = p;

  int optimal_n_bits = ceil (-1.0 * ((double) n * log (p)) / (M_LN2 * M_LN2));
  self->header.n_bits = ((optimal_n_bits / 32) + 1) * 32; // round up to next 32 bit block
  self->header.n_buckets = self->header.n_bits/32;
  self->header.n_hashes = ceil (((double) self->header.n_bits/(double) n) * M_LN2);

  self->buckets = (uint32_t*) calloc (self->header.n_buckets, sizeof (uint32_t));
  self->hashes = (uint32_t*) calloc (self->header.n_hashes, sizeof (uint32_t));

  return self;
}

void
eos_shard_bloom_filter_write_to_stream (EosShardBloomFilter *self, GFileOutputStream *stream)
{
  GOutputStream *out = (GOutputStream*) stream;
  size_t header_size = sizeof (struct eos_shard_bloom_filter_header);
  size_t buckets_size = self->header.n_buckets * sizeof (uint32_t);

  g_output_stream_write (out, &self->header, header_size, NULL, NULL);
  g_output_stream_write (out, self->buckets, buckets_size, NULL, NULL);
}

EosShardBloomFilter *
eos_shard_bloom_filter_new_for_fd (int fd, off_t offset)
{
  EosShardBloomFilter *self = g_new0 (EosShardBloomFilter, 1);
  size_t header_size = sizeof (struct eos_shard_bloom_filter_header);
  size_t buckets_size;

  self->ref_count = 1;

  pread (fd, &self->header, header_size, offset);

  buckets_size = self->header.n_buckets * sizeof (uint32_t);
  self->buckets = (uint32_t*) calloc (self->header.n_buckets, sizeof (uint32_t));
  pread (fd, self->buckets, buckets_size, offset + header_size);

  self->hashes = (uint32_t*) calloc (self->header.n_hashes, sizeof (uint32_t));

  return self;
}

static void
eos_shard_bloom_filter_free (EosShardBloomFilter *self)
{
  g_free (self->buckets);
  g_free (self->hashes);
  g_free (self);
}

EosShardBloomFilter *
eos_shard_bloom_filter_ref (EosShardBloomFilter *self)
{
  self->ref_count++;
  return self;
}

void
eos_shard_bloom_filter_unref (EosShardBloomFilter *self)
{
  if (--self->ref_count == 0)
    eos_shard_bloom_filter_free (self);
}

G_DEFINE_BOXED_TYPE (EosShardBloomFilter, eos_shard_bloom_filter,
                     eos_shard_bloom_filter_ref, eos_shard_bloom_filter_unref)
