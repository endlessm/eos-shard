
#include "epak.h"

#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "adler32.h"
#include "epak_private.h"
#include "epak_entry.h"
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
 * epak_util_raw_name_to_hex_name:
 * @hex_name: (out caller-allocates) (array fixed-size=41 zero-terminated):
 *   Storage for a hexidecimal name, which must be 41 bytes long.
 * @raw_name: The raw name to convert.
 *
 * Converts a raw SHA-1 hash name into a hexadecimal string.
 */
void
epak_util_raw_name_to_hex_name (char *hex_name, uint8_t *raw_name)
{
    int i;

    for (i = 0; i < EPAK_RAW_NAME_SIZE; i++)
        sprintf (&hex_name[i*2], "%02x", raw_name[i]);
    hex_name[EPAK_HEX_NAME_SIZE] = '\0';

    return;
}

/**
 * epak_util_hex_name_to_raw_name:
 * @raw_name: (out caller-allocates) (array fixed-size=20):
 *   Storage for a raw name, which must be 20 bytes long.
 * @hex_name: The hexidecimal name to convert.
 *
 * Converts a raw SHA-1 hash name into a hexadecimal string. If
 * we could not convert this name, then this function returns %FALSE.
 */
gboolean
epak_util_hex_name_to_raw_name (uint8_t raw_name[20], char *hex_name)
{
    int n = strlen (hex_name);
    if (n < EPAK_HEX_NAME_SIZE)
        return FALSE;

    int i = 0;
    for (i = 0; i < EPAK_RAW_NAME_SIZE; i++) {
        char a = hex_name[i*2];
        char b = hex_name[i*2+1];
        if (!g_ascii_isxdigit (a) || !g_ascii_isxdigit (b))
            return FALSE;
        raw_name[i] = (g_ascii_xdigit_value (a) << 8) | g_ascii_xdigit_value (b);
    }

    return TRUE;
}

static int
epak_doc_entry_cmp (const void *a_, const void *b_)
{
    const struct epak_doc_entry *a = a_;
    const struct epak_doc_entry *b = b_;
    return memcmp (a->raw_name, b->raw_name, EPAK_RAW_NAME_SIZE);
}

/**
 * epak_pak_find_entry_by_raw_name:
 *
 * Finds a #EpakEntry for the given raw name
 *
 * Returns: (transfer full): the #EpakEntry with the given raw name
 */
EpakEntry *
epak_pak_find_entry_by_raw_name (EpakPak *self, uint8_t *raw_name)
{
    EpakPakPrivate *priv = epak_pak_get_instance_private(self);
    struct epak_t *pak = priv->epak_handle;
    return bsearch (raw_name,
                    pak->entries, pak->hdr.n_docs,
                    sizeof (struct epak_doc_entry),
                    epak_doc_entry_cmp);
}

/**
 * epak_pak_list_entries:
 *
 * List all entries inside @self.
 *
 * Returns: (transfer full) (element-type EpakEntry): a list of #EpakEntry
 */
GSList *
epak_pak_list_entries (EpakPak *self)
{
    EpakPakPrivate *priv = epak_pak_get_instance_private (self);
    GSList *l = NULL;
    int i;

    for (i = priv->epak_handle->hdr.n_docs - 1; i >= 0; i--) {
        struct epak_doc_entry *entry = &priv->epak_handle->entries[i];
        l = g_slist_prepend (l, _epak_entry_new_for_doc (self, entry));
    }

    return l;
}

GBytes *
_epak_pak_load_blob (EpakPak *self, struct epak_blob_entry *blob)
{
    EpakPakPrivate *priv = epak_pak_get_instance_private(self);
    uint8_t *buf = g_malloc (blob->size);
    GBytes *bytes;

    pread (priv->fd, buf, blob->size, priv->epak_handle->hdr.data_offs + blob->offs);
    bytes = g_bytes_new_take (buf, blob->size);

    uint32_t csum = adler32 (ADLER32_INIT, buf, blob->size);
    if (csum != blob->adler32) {
        g_clear_pointer (&bytes, g_bytes_unref);
        g_warning ("Could not load blob: checksum did not match");
    }

    return bytes;
}
