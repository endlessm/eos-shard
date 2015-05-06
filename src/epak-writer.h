/* Copyright 2015 Endless Mobile, Inc. */

/* This file is part of epak.
 *
 * epak is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * epak is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with epak.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifndef EPAK_WRITER_H
#define EPAK_WRITER_H

#include <gio/gio.h>
#include "epak-format.h"

#define EPAK_TYPE_WRITER             (epak_writer_get_type ())
#define EPAK_WRITER(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), EPAK_TYPE_WRITER, EpakWriter))
#define EPAK_WRITER_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass),  EPAK_TYPE_WRITER, EpakWriterClass))
#define EPAK_IS_WRITER(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EPAK_TYPE_WRITER))
#define EPAK_IS_WRITER_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass),  EPAK_TYPE_WRITER))
#define EPAK_WRITER_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj),  EPAK_TYPE_WRITER, EpakWriterClass))

typedef struct _EpakWriter        EpakWriter;
typedef struct _EpakWriterClass   EpakWriterClass;

struct _EpakWriter
{
  GObject parent;
};

struct _EpakWriterClass
{
  GObjectClass parent_class;
};

GType epak_writer_get_type (void) G_GNUC_CONST;

void epak_writer_add_record (EpakWriter *writer,
                             char *hex_name,
                             GFile *metadata,
                             EpakBlobFlags metadata_flags,
                             GFile *data,
                             EpakBlobFlags data_flags);
void epak_writer_write (EpakWriter *writer,
                        char *path);

#endif /* EPAK_WRITER_H */
