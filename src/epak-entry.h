
#ifndef EPAK_ENTRY_H
#define EPAK_ENTRY_H

#include <gio/gio.h>
#include <stdint.h>

#include "epak-types.h"

GType epak_entry_get_type (void) G_GNUC_CONST;

struct _EpakEntry {
  /*< private >*/
  int ref_count;
  EpakPak *pak;
  struct epak_doc_entry *doc;
};

EpakEntry * _epak_entry_new_for_doc (EpakPak *pak, struct epak_doc_entry *doc);

uint8_t * epak_entry_get_raw_name (EpakEntry *entry);
char * epak_entry_get_hex_name (EpakEntry *entry);

GBytes * epak_entry_read_metadata (EpakEntry *entry);
GBytes * epak_entry_read_data (EpakEntry *entry);

#endif /* EPAK_ENTRY_H */
