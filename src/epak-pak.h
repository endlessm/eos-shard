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

#ifndef EPAK_PAK_H
#define EPAK_PAK_H

#include <gio/gio.h>
#include <stdint.h>

#include "epak-types.h"
#include "epak-format.h"

#define EPAK_TYPE_PAK             (epak_pak_get_type ())
#define EPAK_PAK(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), EPAK_TYPE_PAK, EpakPak))
#define EPAK_PAK_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass),  EPAK_TYPE_PAK, EpakPakClass))
#define EPAK_IS_PAK(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EPAK_TYPE_PAK))
#define EPAK_IS_PAK_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass),  EPAK_TYPE_PAK))
#define EPAK_PAK_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj),  EPAK_TYPE_PAK, EpakPakClass))

typedef struct _EpakPak        EpakPak;
typedef struct _EpakPakClass   EpakPakClass;

struct _EpakPak {
  GObject parent;
};

struct _EpakPakClass {
  GObjectClass parent_class;
};

GType epak_pak_get_type (void) G_GNUC_CONST;

void epak_util_raw_name_to_hex_name (char *hex_name, uint8_t *raw_name);
gboolean epak_util_hex_name_to_raw_name (uint8_t raw_name[20], char *hex_name);

EpakRecord * epak_pak_find_record_by_raw_name (EpakPak *self, uint8_t *raw_name);
EpakRecord * epak_pak_find_record_by_hex_name (EpakPak *self, char *hex_name);
GSList * epak_pak_list_records (EpakPak *self);

GBytes * _epak_pak_load_blob (EpakPak                 *self,
                              struct epak_blob_entry  *blob,
                              GError                 **error);

gsize _epak_pak_read_data (EpakPak *self, void *buf, gsize count, goffset offset);

#endif /* EPAK_PAK_H */

