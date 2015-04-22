#include "epak.h"

#include "epak_private.h"
#include "epak_fmt.h"

struct _EpakPakPrivate
{
    int fd;
    struct epak_t *epak_handle;
    char *path;
};

typedef struct _EpakPakPrivate EpakPakPrivate;

enum
{
    PROP_0,

    PROP_PATH,

    LAST_PROP
};

static GParamSpec *obj_props[LAST_PROP] = { NULL, };

static void initable_iface_init (GInitableIface *iface);

G_DEFINE_TYPE_WITH_CODE (EpakPak, epak_pak, G_TYPE_OBJECT,
                         G_ADD_PRIVATE (EpakPak)
                         G_IMPLEMENT_INTERFACE (G_TYPE_INITABLE, initable_iface_init))

static gboolean
epak_pak_init_internal (GInitable *initable,
                        GCancellable *cancellable,
                        GError **error)
{
    EpakPakPrivate *priv = epak_pak_get_instance_private(EPAK_PAK(G_OBJECT(initable)));
    if (priv->path) {
        priv->fd = open(priv->path, O_RDONLY);
        int ret = epak_open(&priv->epak_handle, priv->fd);

        if (ret)
            return TRUE;
        else
            return FALSE;
    } else {
        // FIXME make an error to set
        return FALSE;
    }
}

static void
initable_iface_init (GInitableIface *iface)
{
    iface->init = epak_pak_init_internal;
}

static void
epak_pak_set_property (GObject      *gobject,
                       guint         prop_id,
                       const GValue *value,
                       GParamSpec   *pspec)
{
    EpakPakPrivate *priv = epak_pak_get_instance_private(EPAK_PAK(gobject));
    switch (prop_id) {
        case PROP_PATH:
            priv->path = g_value_dup_string(value);
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
    EpakPakPrivate *priv = epak_pak_get_instance_private(EPAK_PAK(gobject));
    switch (prop_id) {
        case PROP_PATH:
            g_value_set_string(value, priv->path);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
    }
}

static void
epak_pak_finalize (GObject *gobject)
{
    EpakPakPrivate *priv = epak_pak_get_instance_private(EPAK_PAK(gobject));

    g_clear_pointer(&priv->path, g_free);

    G_OBJECT_CLASS(epak_pak_parent_class)->finalize(gobject);
}

static void
epak_pak_dispose (GObject *gobject)
{
    EpakPakPrivate *priv = epak_pak_get_instance_private(EPAK_PAK(gobject));

    close(priv->fd);

    G_OBJECT_CLASS(epak_pak_parent_class)->dispose(gobject);
}

static void
epak_pak_class_init (EpakPakClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->finalize = epak_pak_finalize;
    gobject_class->dispose = epak_pak_dispose;
    gobject_class->set_property = epak_pak_set_property;
    gobject_class->get_property = epak_pak_get_property;

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

/**
 * epak_pak_find_entry:
 *
 * Finds a #EpakEntry for the given id
 *
 * Returns: (transfer full): the #EpakEntry with id
 */
EpakEntry *
epak_pak_find_entry (EpakPak *self, char *id)
{
    EpakPakPrivate *priv = epak_pak_get_instance_private(self);

    int i;
    for (i=0; i < priv->epak_handle->hdr.n_docs; i++) {
        g_print("HOHOHO %d\n", i);
    }

    return NULL;
}
