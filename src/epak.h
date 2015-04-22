#ifndef __EPAK_PAK_H__
#define __EPAK_PAK_H__

#include <gio/gio.h>
#include <stdint.h>

#include "epak_types.h"

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

EpakEntry * epak_pak_find_entry_by_raw_name (EpakPak *self, uint8_t *raw_name);
GSList * epak_pak_list_entries (EpakPak *self);

GBytes * _epak_pak_load_blob (EpakPak *self, struct epak_blob_entry *blob);

#endif /* __EPAK_PAK_H__ */
