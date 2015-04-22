#ifndef __EPAK_ENTRY_H__
#define __EPAK_ENTRY_H__

#include <gio/gio.h>

#define EPAK_TYPE_ENTRY             (epak_entry_get_type ())
#define EPAK_ENTRY(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), EPAK_TYPE_ENTRY, EpakEntry))
#define EPAK_ENTRY_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass),  EPAK_TYPE_ENTRY, EpakEntryClass))
#define EPAK_IS_ENTRY(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EPAK_TYPE_ENTRY))
#define EPAK_IS_ENTRY_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass),  EPAK_TYPE_ENTRY))
#define EPAK_ENTRY_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj),  EPAK_TYPE_ENTRY, EpakEntryClass))

typedef struct _EpakEntry        EpakEntry;
typedef struct _EpakEntryClass   EpakEntryClass;

struct _EpakEntry {
    GObject parent;
};

struct _EpakEntryClass {
    GObjectClass parent_class;
};

GType epak_entry_get_type (void) G_GNUC_CONST;

#endif /* __EPAK_ENTRY_H__ */
