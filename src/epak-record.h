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

#ifndef EPAK_RECORD_H
#define EPAK_RECORD_H

#include <gio/gio.h>
#include <stdint.h>

#include "epak-types.h"

GType epak_record_get_type (void) G_GNUC_CONST;

struct _EpakRecord {
  /*< private >*/
  int ref_count;
  EpakPak *pak;
  struct epak_record_entry *record_entry;

  /*< public >*/
  EpakBlob *data;
  EpakBlob *metadata;
};

EpakRecord * _epak_record_new_for_record_entry (EpakPak *pak, struct epak_record_entry *record_entry);

uint8_t * epak_record_get_raw_name (EpakRecord *record);
char * epak_record_get_hex_name (EpakRecord *record);

#endif /* EPAK_RECORD_H */
