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

#ifndef EOS_SHARD_SHARD_FILE_IMPL_H
#define EOS_SHARD_SHARD_FILE_IMPL_H

#include <gio/gio.h>
#include <stdint.h>

#include "eos-shard-types.h"

#define EOS_SHARD_TYPE_SHARD_FILE_IMPL (eos_shard_shard_file_impl_get_type ())
G_DECLARE_INTERFACE (EosShardShardFileImpl, eos_shard_shard_file_impl, EOS_SHARD, SHARD_FILE_IMPL, GObject)

struct _EosShardShardFileImplInterface
{
  GTypeInterface g_iface;

  EosShardRecord *  (* find_record_by_raw_name) (EosShardShardFileImpl  *self,
                                                 uint8_t                *raw_name);

  GSList *          (* list_records)            (EosShardShardFileImpl  *self);

  EosShardBlob *    (* lookup_blob)             (EosShardShardFileImpl  *self,
                                                 EosShardRecord         *record,
                                                 const char             *name);
  GSList *          (* list_blobs)              (EosShardShardFileImpl  *self,
                                                 EosShardRecord         *record);
};

#endif /* EOS_SHARD_SHARD_FILE_IMPL_H */
