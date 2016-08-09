/* Copyright 2015 Endless Mobile, Inc. */

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

#include "config.h"

#include "eos-shard-bloom-filter.h"
#include "eos-shard-enums.h"

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

gboolean
bloom_filter_init_for_fd (struct bloom_filter *self, int fd, off_t offset, GError **error)
{
  size_t buckets_size;
  ssize_t len;

  len = pread (fd, &self->header, sizeof (self->header), offset);

  if (len < 0) {
    g_set_error (error, EOS_SHARD_ERROR, EOS_SHARD_ERROR_BLOOM_FILTER_CORRUPT,
                 "The bloom filter is corrupt.");
    return FALSE;
  }

  buckets_size = self->header.n_buckets * sizeof (uint32_t);
  self->buckets = (uint32_t*) calloc (self->header.n_buckets, sizeof (uint32_t));

  len = pread (fd, self->buckets, buckets_size, offset + sizeof (self->header));

  if (len < 0) {
    g_set_error (error, EOS_SHARD_ERROR, EOS_SHARD_ERROR_BLOOM_FILTER_CORRUPT,
                 "The bloom filter is corrupt.");
    return FALSE;
  }

  return TRUE;
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

/*
 * FNV Hash implementation
 *
 * FNV is a very fast, non-cryptographic hash function, and thus is a good
 * choice for bloom filters. Although bloom filters require a variable number
 * of different hash functions (inversely proportional to the desired false
 * positive rate), we can fake this with only two hash-like functions (fnv_1a
 * and fnv_1a_b) using a technique outlined here:
 *
 * http://willwhim.wpengine.com/2011/09/03/producing-n-hash-functions-by-hashing-only-once/
 */

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

/*
 * A cheap implementation of a "modified" fnv_1a. Basically just mixes the bits
 * around of a hashed value, sourced from:
 * https://web.archive.org/web/20131019013225/http://home.comcast.net/~bretm/hash/6.html
 */
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

/*
 * bloom_filter_add:
 * @self: the bloom filter
 * @key: string value to add to the bloom filter
 *
 * Hashes the key and adds it to the bloom filter.
 */
void
bloom_filter_add (struct bloom_filter *self, char *key)
{
  if (self->header.n_bits == 0)
    return;

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

/*
 * bloom_filter_test:
 * @self: the bloom filter
 * @key: string value which will be sought in the filter
 *
 * Hashes the key and tests the results against the filter. Note that this
 * function will always return FALSE if key isn't in the filter, but may return
 * TRUE even if the key isn't in the filter. In other words, this function may
 * return false positives but never false negatives.
 *
 * The chance of hitting a false positive is defined when the bloom filter
 * is created.
 *
 * Returns: FALSE if key is definitely not present, TRUE if it probably is
 */
gboolean
bloom_filter_test (struct bloom_filter *self, char *key)
{
  if (self->header.n_bits == 0)
    return FALSE;

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
