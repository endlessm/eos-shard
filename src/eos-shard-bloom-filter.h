
#pragma once

/* GI doesn't like the unprefixed bloom filter types. */
#ifndef __GI_SCANNER__

#include <stdint.h>
#include <gio/gio.h>

struct bloom_filter_header {
  uint32_t n_bits;
  uint32_t n_buckets;
  uint32_t n_hashes;
};

struct bloom_filter {
  struct bloom_filter_header header;
  uint32_t *buckets;
};

void bloom_filter_init_for_params (struct bloom_filter *self, int n, double p);
void bloom_filter_init_for_fd (struct bloom_filter *self, int fd, off_t offset);

void bloom_filter_write_to_stream (struct bloom_filter *self, GOutputStream *out);

void bloom_filter_add (struct bloom_filter *self, char *key);
gboolean bloom_filter_test (struct bloom_filter *self, char *key);

void bloom_filter_dispose (struct bloom_filter *self);

#endif /* __GI_SCANNER__ */
