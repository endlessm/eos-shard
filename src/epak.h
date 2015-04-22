#include <gio/gio.h>

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
