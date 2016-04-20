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

#pragma once

/* GI doesn't like the unprefixed jlist types. */
#ifndef __GI_SCANNER__

#include <stdint.h>

/* The JList format */

#pragma pack(push, 4)

#define JLIST_MAGIC "JListV1 "

/* The core format is a giant blob of key/value pairs, which are both C
 * strings (that is, delimited by NUL pointers). At the end, we have
 * regularly spaced offsets into the file, which divides the large blob
 * into a number of "blocks". This allows us to do a binary search across
 * the starting key of each "block", and then from there, do a linear scan
 * into the block to find the key we want.
 *
 * This structure is often known as a "jump list" or "skip list".
 *
 * We do this since it is otherwise difficult to binary search across
 * variable-length data.
 *
 * Additionally, in cases where we expect a lot of keys to not be found,
 * we allow embedding a bloom filter, giving us an easy win for key lookups.
 */

/* The maximum size of a key or a value stored in the JList. Since we read
 * from disk in chunks of this amount, we will not find keys if they go over
 * this limit. */
#define JLIST_MAX_KEY_SIZE 8192

struct jlist_header {
    char magic[8];

    /* Start of the offsets in the file. */
    uint64_t block_table_start;

    /* The start of the bloom filter. Is set to 0 if there is no
     * bloom filter in the file... */
    uint64_t bloom_filter_start;
};

struct jlist_block_table_entry {
    /* Offset to a block. */
    uint64_t offset;

    /* Length of a block. */
    uint64_t length;
};

struct jlist_block_table {
    uint16_t n_blocks;

    /* Immediately afterwards, block entries. */
    struct jlist_block_table_entry *blocks;
};

#pragma pack(pop)

#endif
