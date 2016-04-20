/* Copyright 2016 Endless Mobile, Inc. */

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

#include "eos-shard-jlist.h"

#include <stdint.h>
#include <string.h>

#include "eos-shard-bloom-filter.h"

/* Details of the format and algorithm are given in here. */
#include "eos-shard-jlist-format.h"

#define CSTRING_SIZE(S) (strlen((S)) + 1)

typedef struct _EosShardJList {
  int ref_count;

  int fd;
  int offset;
  struct jlist_header header;

  struct bloom_filter _bloom_filter, *bloom_filter;
} EosShardJList;

static gboolean
jlist_open (struct jlist_header *header, int fd, int offset)
{
  pread (fd, header, sizeof (*header), offset);

  if (memcmp (header->magic, JLIST_MAGIC, sizeof (header->magic)) != 0)
    return FALSE;

  return TRUE;
}

/* Find the block for a given key with a binary search. */
static gboolean
jlist_find_block (EosShardJList *jlist, char *key,
                  struct jlist_block_table_entry *block_out)
{
  int fd = jlist->fd;

  int key_size = CSTRING_SIZE(key);

  struct jlist_block_table tbl;
  pread (fd, &tbl, sizeof(tbl), jlist->offset + jlist->header.block_table_start);

  struct jlist_block_table_entry blocks[tbl.n_blocks];
  pread (fd, blocks, sizeof(blocks), jlist->offset + jlist->header.block_table_start + 2);

  /* bsearch */
  int lo = 0, hi = tbl.n_blocks - 1;
  while (lo <= hi) {
    int mid = (lo + hi) / 2;
    uint64_t block_offs = blocks[mid].offset;

    char chunk_key[key_size];
    memset (chunk_key, 0, key_size);
    pread (fd, chunk_key, key_size, jlist->offset + block_offs);

    /* We want to find the first block where the chunk_key is greater than the key,
     * since the block before that has the value we want. */
    int p = (strncmp (chunk_key, key, key_size) > 0);

    if (p)
      hi = mid - 1;
    else
      lo = mid + 1;
  }

  /* We didn't find the item. */
  if (lo <= 0 || hi > tbl.n_blocks - 1)
    return FALSE;

  uint64_t block_idx = lo - 1;
  *block_out = blocks[block_idx];
  return TRUE;
}

/* Given a block, do the linear scan into it. */
static uint64_t
jlist_lookup_key_in_block (EosShardJList *jlist,
                           struct jlist_block_table_entry block,
                           char *key)
{
  int fd = jlist->fd;
  uint64_t offs = block.offset, end = offs + block.length;

  while (offs < end) {
    /* Now do a linear search for the key... */
    char chunk[JLIST_MAX_KEY_SIZE] = {};
    int chunk_size = sizeof (chunk) - 1;

    pread (fd, chunk, chunk_size, jlist->offset + offs);

    char *str = chunk;
    while (1) {
      char *chunk_key = str;
      uint64_t key_offs = offs;

#define ADVANCE_CHUNK()                                                 \
      offs += CSTRING_SIZE(str);                                        \
      str += CSTRING_SIZE(str);                                         \
      /* If we're truncated and reached the end of the chunk, then read again. */ \
      if (str >= chunk + chunk_size) {                                  \
        offs = key_offs;                                                \
        break;                                                          \
      }

      /* We've read the key, now advance... */
      ADVANCE_CHUNK();

      if (strcmp (chunk_key, key) == 0)
        return offs;

      /* Skip value. */
      ADVANCE_CHUNK();
    }
  }

  return 0;
}

static uint64_t
jlist_lookup_key (EosShardJList *jlist, char *key)
{
  if (jlist->bloom_filter)
    if (bloom_filter_test (jlist->bloom_filter, key))
      return 0;

  struct jlist_block_table_entry block;

  if (!jlist_find_block (jlist, key, &block))
    return 0;

  return jlist_lookup_key_in_block (jlist, block, key);
}

EosShardJList *
eos_shard_jlist_new_for_fd (int fd, off_t offset)
{
  struct jlist_header header;

  if (!jlist_open (&header, fd, offset))
    return NULL;

  EosShardJList *jlist = g_new0 (EosShardJList, 1);
  jlist->ref_count = 1;
  jlist->fd = fd;
  jlist->offset = offset;
  jlist->header = header;

  if (jlist->header.bloom_filter_start != 0) {
    jlist->bloom_filter = &jlist->_bloom_filter;
    bloom_filter_init_for_fd (jlist->bloom_filter, fd, offset + jlist->header.bloom_filter_start);
  }

  return jlist;
}

static void
eos_shard_jlist_free (EosShardJList *jlist)
{
  if (jlist->bloom_filter)
    bloom_filter_dispose (jlist->bloom_filter);

  g_free (jlist);
}

EosShardJList *
eos_shard_jlist_ref (EosShardJList *jlist)
{
  jlist->ref_count++;
  return jlist;
}

void
eos_shard_jlist_unref (EosShardJList *jlist)
{
  if (--jlist->ref_count == 0)
    eos_shard_jlist_free (jlist);
}

static char *
read_cstring (int fd, int offset)
{
  GString *string = g_string_new (NULL);

  while (1) {
    char chunk[8192] = {};
    int len = pread (fd, chunk, sizeof (chunk) - 1, offset);
    g_string_append_len (string, chunk, len);
    if (strlen (chunk) < len)
      break;
  }

  return g_string_free (string, FALSE);
}

char *
eos_shard_jlist_lookup_key (EosShardJList *jlist,
                            char          *key)
{
  uint64_t value_offset = jlist_lookup_key (jlist, key);
  if (value_offset == 0)
    return NULL;
  return read_cstring (jlist->fd, jlist->offset + value_offset);
}

G_DEFINE_BOXED_TYPE (EosShardJList, eos_shard_jlist,
                     eos_shard_jlist_ref, eos_shard_jlist_unref)
