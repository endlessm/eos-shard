/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */
/*
 * eos-shard-file-input-stream-wrapper.h
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
 */

#pragma once

#include <gio/gio.h>

#define EOS_SHARD_TYPE_FILE_INPUT_STREAM_WRAPPER (eos_shard_file_input_stream_wrapper_get_type ())
G_DECLARE_FINAL_TYPE (EosShardFileInputStreamWrapper, eos_shard_file_input_stream_wrapper, EOS_SHARD, FILE_INPUT_STREAM_WRAPPER, GFileInputStream)

GFileInputStream *_eos_shard_file_input_stream_wrapper_new (GFile        *file,
                                                            GInputStream *stream);
