/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */
/*
 * eos-shard-file.h
 *
 * Copyright (C) 2016 Endless Mobile, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Author: Juan Pablo Ugarte <ugarte@endlessm.com>
 *
 */

#pragma once

#include <gio/gio.h>
#include "eos-shard-blob.h"

G_BEGIN_DECLS

#define EOS_SHARD_TYPE_FILE (eos_shard_file_get_type ())
G_DECLARE_FINAL_TYPE (EosShardFile, eos_shard_file, EOS_SHARD, FILE, GObject)

/**
 * eos_shard_file_new:
 * @uri: The URI to create this #GFile at.
 * @uri_scheme: The URI scheme to use.
 * @blob: An #EosShardBlob to read for this file (transfer full)
 *
 * Creates a new #GFile with support for reading #EosShardBlob
 * with the URI of uri.
 *
 * Returns: (transfer full): A #GFile implementation to read the shard
 */
GFile *eos_shard_file_new (const gchar *uri,
                           const gchar *uri_scheme,
                           EosShardBlob *blob);

G_END_DECLS
