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

#include "eos-shard-dictionary.h"

#include <stdint.h>
#include <string.h>

#include "eos-shard-bloom-filter.h"
#include "eos-shard-enums.h"

/* Details of the format and algorithm are given in here. */
#include "eos-shard-dictionary-format.h"

#define CSTRING_SIZE(S) (strlen((S)) + 1)

typedef struct _EosShardDictionary {
  int ref_count;

  int fd;
  int offset;
  struct dictionary_header header;

  struct bloom_filter _bloom_filter, *bloom_filter;
} EosShardDictionary;

static gboolean
dictionary_open (struct dictionary_header *header, int fd, int offset)
{
  ssize_t len = pread (fd, header, sizeof (*header), offset);

  if (len < -1 || memcmp (header->magic, DICTIONARY_MAGIC, sizeof (header->magic)) != 0)
    return FALSE;

  return TRUE;
}

/* Find the block for a given key with a binary search. */
static gboolean
dictionary_find_block (EosShardDictionary *dictionary,
                       char *key,
                       struct dictionary_block_table_entry *block_out,
                       GError **error)
{
  ssize_t len;
  int fd = dictionary->fd;

  int key_size = CSTRING_SIZE (key);

  struct dictionary_block_table tbl;
  len = pread (fd, &tbl, sizeof(tbl), dictionary->offset + dictionary->header.block_table_start);

  if (len < 0) {
    g_set_error (error, EOS_SHARD_ERROR, EOS_SHARD_ERROR_DICTIONARY_CORRUPT,
                 "The dictionary is corrupt.");
    return FALSE;
  }

  struct dictionary_block_table_entry blocks[tbl.n_blocks];
  len = pread (fd, blocks, sizeof(blocks), dictionary->offset + dictionary->header.block_table_start + 2);

  if (len < 0) {
    g_set_error (error, EOS_SHARD_ERROR, EOS_SHARD_ERROR_DICTIONARY_CORRUPT,
                 "The dictionary is corrupt.");
    return FALSE;
  }

  /* bsearch */
  int lo = 0, hi = tbl.n_blocks - 1;
  while (lo <= hi) {
    int mid = (lo + hi) / 2;
    uint64_t block_offs = blocks[mid].offset;

    char chunk_key[key_size];
    memset (chunk_key, 0, key_size);
    len = pread (fd, chunk_key, key_size, dictionary->offset + block_offs);

    if (len < 0) {
      g_set_error (error, EOS_SHARD_ERROR, EOS_SHARD_ERROR_DICTIONARY_CORRUPT,
                   "The dictionary is corrupt.");
      return FALSE;
    }

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
dictionary_lookup_key_in_block (EosShardDictionary *dictionary,
                                struct dictionary_block_table_entry block,
                                char *key,
                                GError **error)
{
  ssize_t len;
  int fd = dictionary->fd;
  uint64_t offs = block.offset, end = offs + block.length;

  while (offs < end) {
    /* Now do a linear search for the key... */
    char chunk[DICTIONARY_MAX_KEY_SIZE] = {};
    int chunk_size = sizeof (chunk) - 1;

    len = pread (fd, chunk, chunk_size, dictionary->offset + offs);

    if (len < 0) {
      g_set_error (error, EOS_SHARD_ERROR, EOS_SHARD_ERROR_DICTIONARY_CORRUPT,
                   "The dictionary is corrupt.");
      return 0;
    }

    char *str = chunk;
    while (1) {
      char *chunk_key = str;
      uint64_t key_offs = offs;

#define ADVANCE_CHUNK()                                                 \
      offs += CSTRING_SIZE (str);                                        \
      str += CSTRING_SIZE (str);                                         \
      /* If we're truncated and reached the end of the chunk, then read again. */ \
      if (str >= chunk + chunk_size) {                                  \
        offs = key_offs;                                                \
        break;                                                          \
      }

      /* We've read the key, now advance... */
      ADVANCE_CHUNK ();

      if (strcmp (chunk_key, key) == 0)
        return offs;

      /* Skip value. */
      ADVANCE_CHUNK ();
    }
  }

  return 0;
}

static uint64_t
dictionary_lookup_key (EosShardDictionary *dictionary, char *key, GError **error)
{
  if (dictionary->bloom_filter)
    if (!bloom_filter_test (dictionary->bloom_filter, key))
      return 0;

  struct dictionary_block_table_entry block;

  if (!dictionary_find_block (dictionary, key, &block, error))
    return 0;

  return dictionary_lookup_key_in_block (dictionary, block, key, error);
}

EosShardDictionary *
eos_shard_dictionary_new_for_fd (int fd, goffset offset, GError **error)
{
  struct dictionary_header header;

  if (!dictionary_open (&header, fd, offset)) {
    g_set_error (error, EOS_SHARD_ERROR, EOS_SHARD_ERROR_DICTIONARY_CORRUPT,
                 "The dictionary is corrupt.");
    return NULL;
  }

  EosShardDictionary *dictionary = g_new0 (EosShardDictionary, 1);
  dictionary->ref_count = 1;
  dictionary->fd = fd;
  dictionary->offset = offset;
  dictionary->header = header;

  if (dictionary->header.bloom_filter_start != 0) {
    dictionary->bloom_filter = &dictionary->_bloom_filter;
    if (!bloom_filter_init_for_fd (dictionary->bloom_filter, fd, offset + dictionary->header.bloom_filter_start, error))
      return NULL;
  }

  return dictionary;
}

static void
eos_shard_dictionary_free (EosShardDictionary *dictionary)
{
  if (dictionary->bloom_filter)
    bloom_filter_dispose (dictionary->bloom_filter);

  g_free (dictionary);
}

EosShardDictionary *
eos_shard_dictionary_ref (EosShardDictionary *dictionary)
{
  dictionary->ref_count++;
  return dictionary;
}

void
eos_shard_dictionary_unref (EosShardDictionary *dictionary)
{
  if (--dictionary->ref_count == 0)
    eos_shard_dictionary_free (dictionary);
}

static char *
read_cstring (int fd, int offset)
{
  GString *string = g_string_new (NULL);
  ssize_t len;

  while (1) {
    char chunk[8192] = {};
    len = pread (fd, chunk, sizeof (chunk) - 1, offset);
    g_string_append_len (string, chunk, len);
    if (strlen (chunk) < len)
      break;
  }

  return g_string_free (string, FALSE);
}

/* Find a key within the dictionary, returning the value stored at key if found */
char *
eos_shard_dictionary_lookup_key (EosShardDictionary *dictionary, char *key, GError **error)
{
  uint64_t value_offset = dictionary_lookup_key (dictionary, key, error);
  if (value_offset == 0)
    return NULL;
  return read_cstring (dictionary->fd, dictionary->offset + value_offset);
}

G_DEFINE_BOXED_TYPE (EosShardDictionary, eos_shard_dictionary,
                     eos_shard_dictionary_ref, eos_shard_dictionary_unref)
