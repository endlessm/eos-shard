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
gboolean bloom_filter_init_for_fd (struct bloom_filter *self,
                                   int fd,
                                   off_t offset,
                                   GError **error);

void bloom_filter_write_to_stream (struct bloom_filter *self, GOutputStream *out);

void bloom_filter_add (struct bloom_filter *self, const char *key);
gboolean bloom_filter_test (struct bloom_filter *self, const char *key);

void bloom_filter_dispose (struct bloom_filter *self);

#endif /* __GI_SCANNER__ */
