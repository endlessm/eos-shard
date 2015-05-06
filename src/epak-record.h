
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
