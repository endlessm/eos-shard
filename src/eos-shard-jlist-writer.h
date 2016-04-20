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

GType eos_shard_jlist_writer_get_type (void) G_GNUC_CONST;

EosShardJListWriter * eos_shard_jlist_writer_new_for_stream (GFileOutputStream *stream, int n_entries);

void eos_shard_jlist_writer_begin (EosShardJListWriter *self);
void eos_shard_jlist_writer_add_entry (EosShardJListWriter *self, char *key, char *value);
void eos_shard_jlist_writer_finish (EosShardJListWriter *self);
