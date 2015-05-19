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

#ifndef EOS_SHARD_WRITER_H
#define EOS_SHARD_WRITER_H

#include <gio/gio.h>
#include "eos-shard-format.h"

#define EOS_SHARD_TYPE_WRITER             (eos_shard_writer_get_type ())
#define EOS_SHARD_WRITER(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), EOS_SHARD_TYPE_WRITER, EosShardWriter))
#define EOS_SHARD_WRITER_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass),  EOS_SHARD_TYPE_WRITER, EosShardWriterClass))
#define EOS_SHARD_IS_WRITER(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EOS_SHARD_TYPE_WRITER))
#define EOS_SHARD_IS_WRITER_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass),  EOS_SHARD_TYPE_WRITER))
#define EOS_SHARD_WRITER_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj),  EOS_SHARD_TYPE_WRITER, EosShardWriterClass))

/**
 * EosShardWriter:
 *
 * An object for packing file into a shard file. Records are added as pairs of
 * data/metadata, and are tagged with a unique 40 character hex name.
 *
 * Files must be added in increasing order based on their name.
 */

typedef struct _EosShardWriter        EosShardWriter;
typedef struct _EosShardWriterClass   EosShardWriterClass;

struct _EosShardWriter
{
  GObject parent;
};

struct _EosShardWriterClass
{
  GObjectClass parent_class;
};

GType eos_shard_writer_get_type (void) G_GNUC_CONST;

void eos_shard_writer_add_record (EosShardWriter *writer,
                                  char *hex_name,
                                  GFile *metadata,
                                  EosShardBlobFlags metadata_flags,
                                  GFile *data,
                                  EosShardBlobFlags data_flags);
void eos_shard_writer_write (EosShardWriter *writer,
                             char *path);

#endif /* EOS_SHARD_WRITER_H */
