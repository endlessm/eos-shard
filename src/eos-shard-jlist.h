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

#include <gio/gio.h>

#include "eos-shard-types.h"

GType eos_shard_jlist_get_type (void) G_GNUC_CONST;

EosShardJList * _eos_shard_jlist_new (int fd, off_t offset);

EosShardJList * eos_shard_jlist_ref (EosShardJList *jlist);
void eos_shard_jlist_unref (EosShardJList *jlist);
char * eos_shard_jlist_lookup_key (EosShardJList *jlist, char *key);
GHashTable * eos_shard_jlist_values (EosShardJList *jlist);
