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

#ifndef EOS_SHARD_SHARD_FILE_IMPL_V2_H
#define EOS_SHARD_SHARD_FILE_IMPL_V2_H

#include "eos-shard-types.h"
#include "eos-shard-shard-file-impl.h"

#define EOS_SHARD_TYPE_SHARD_FILE_IMPL_V2 (eos_shard_shard_file_impl_v2_get_type ())
G_DECLARE_FINAL_TYPE (EosShardShardFileImplV2, eos_shard_shard_file_impl_v2, EOS_SHARD, SHARD_FILE_IMPL_V2, GObject)

EosShardShardFileImpl *
_eos_shard_shard_file_impl_v2_new (EosShardShardFile *self,
                                   int fd,
                                   GError **error);

#endif /* EOS_SHARD_SHARD_FILE_IMPL_V2_H */

