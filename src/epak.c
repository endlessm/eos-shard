#include "epak.h"

#include "epak_private.h"
#include "epak_fmt.h"

struct _EpakPakPrivate
{
    struct epak_t *epak_handle;
    gchar *path;
};
typedef struct _EpakPakPrivate EpakPakPrivate;

enum
{
  PROP_0,

  PROP_PATH,

  LAST_PROP
};

static GParamSpec *obj_props[LAST_PROP] = { NULL, };

static void initable_iface_async_init (GAsyncInitableIface *iface);

G_DEFINE_TYPE_WITH_CODE (EpakPak, epak_pak, G_TYPE_OBJECT,
                         G_ADD_PRIVATE (EpakPak)
                         G_IMPLEMENT_INTERFACE (G_TYPE_ASYNC_INITABLE, initable_iface_async_init))

void
epak_pak_init_async_internal (GAsyncInitable *initable,
                                   int             io_priority,
                                   GCancellable   *cancellable,
                                   GAsyncReadyCallback callback,
                                   gpointer user_data)
{
}

gboolean
epak_pak_init_finish_internal (GAsyncInitable *initable,
                                    GAsyncResult   *res,
                                    GError        **error)
{
    return FALSE;
}

static void
epak_pak_set_property (GObject      *gobject,
                       guint         prop_id,
                       const GValue *value,
                       GParamSpec   *pspec)
{
  switch (prop_id)
    {
    case PROP_PATH:
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
    }
}

static void
epak_pak_get_property (GObject    *gobject,
                       guint       prop_id,
                       GValue     *value,
                       GParamSpec *pspec)
{
  switch (prop_id)
    {
    case PROP_PATH:
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
    }
}

static void
initable_iface_async_init (GAsyncInitableIface *iface)
{
  iface->init_async = epak_pak_init_async_internal;
  iface->init_finish = epak_pak_init_finish_internal;
}

static void
epak_pak_class_init (EpakPakClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->set_property = epak_pak_set_property;
  gobject_class->get_property = epak_pak_get_property;

  /**
   * XapianDatabase:path:
   *
   * The path to the database directory.
   */
  obj_props[PROP_PATH] =
    g_param_spec_string ("path",
                         "Path",
                         "Path to the epak file",
                         NULL,
                         (GParamFlags) (G_PARAM_READWRITE |
                                        G_PARAM_CONSTRUCT_ONLY |
                                        G_PARAM_STATIC_STRINGS));

  g_object_class_install_properties (gobject_class, LAST_PROP, obj_props);
}

static void 
epak_pak_init (EpakPak *pak)
{
}
