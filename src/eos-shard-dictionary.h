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

GType eos_shard_dictionary_get_type (void) G_GNUC_CONST;

EosShardDictionary * eos_shard_dictionary_new_for_fd (int fd,
                                                      goffset offset,
                                                      GError **error);

EosShardDictionary * eos_shard_dictionary_ref (EosShardDictionary *dictionary);
void eos_shard_dictionary_unref (EosShardDictionary *dictionary);
char * eos_shard_dictionary_lookup_key (EosShardDictionary *dictionary,
                                        const char *key,
                                        GError **error);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (EosShardDictionary, eos_shard_dictionary_unref)
