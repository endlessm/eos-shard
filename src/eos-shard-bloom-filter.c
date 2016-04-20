
#include "eos-shard-bloom-filter.h"

#include <math.h>
#include <stdlib.h>

void
bloom_filter_init_for_params (struct bloom_filter *self, int n, double p)
{
  int optimal_n_bits = ceil (-1.0 * ((double) n * log (p)) / (M_LN2 * M_LN2));

  /* Round up to the nearest group of 32, since each bucket is 32 bits wide. */
  self->header.n_bits = (optimal_n_bits + 0x1f) & ~0x1f;
  self->header.n_buckets = self->header.n_bits / 32;
  self->header.n_hashes = ceil (((double) self->header.n_bits / (double) n) * M_LN2);

  self->buckets = (uint32_t*) calloc (self->header.n_buckets, sizeof (uint32_t));
}

void
bloom_filter_init_for_fd (struct bloom_filter *self, int fd, off_t offset)
{
  size_t buckets_size;

  pread (fd, &self->header, sizeof (self->header), offset);

  buckets_size = self->header.n_buckets * sizeof (uint32_t);
  self->buckets = (uint32_t*) calloc (self->header.n_buckets, sizeof (uint32_t));

  pread (fd, self->buckets, buckets_size, offset + sizeof (self->header));
}

void
bloom_filter_dispose (struct bloom_filter *self)
{
  free (self->buckets);
}

void
bloom_filter_write_to_stream (struct bloom_filter *self, GOutputStream *out)
{
  g_output_stream_write (out, &self->header, sizeof (self->header), NULL, NULL);

  size_t buckets_size = self->header.n_buckets * sizeof (uint32_t);
  g_output_stream_write (out, self->buckets, buckets_size, NULL, NULL);
}

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
compute_hashes (char *key, uint32_t *hashes, size_t n_hashes, uint32_t n_bits)
{
  uint32_t a = fnv_1a (key);
  uint32_t b = fnv_1a_b (a);
  uint32_t x = a % n_bits;

  int i;
  for (i = 0; i < n_hashes; i++) {
    hashes[i] = x;
    x = (x + b) % n_bits;
  }
}

void
bloom_filter_add (struct bloom_filter *self, char *key)
{
  uint32_t hashes[self->header.n_hashes];
  compute_hashes (key, hashes, self->header.n_hashes, self->header.n_bits);

  int i;
  for (i = 0; i < self->header.n_hashes; i++) {
    uint32_t h = hashes[i];
    int bucket_i = h / 32;
    g_assert (bucket_i < self->header.n_buckets);
    self->buckets[bucket_i] |= 1 << (h % 32);
  }
}

gboolean
bloom_filter_test (struct bloom_filter *self, char *key)
{
  uint32_t hashes[self->header.n_hashes];
  compute_hashes (key, hashes, self->header.n_hashes, self->header.n_bits);

  int i;
  for (i = 0; i < self->header.n_hashes; i++) {
    uint32_t h = hashes[i];
    int bucket_i = h / 32;
    g_assert (bucket_i < self->header.n_buckets);
    if ((self->buckets[bucket_i] & (1 << (h % 32))) == 0)
      return FALSE;
  }

  return TRUE;
}
