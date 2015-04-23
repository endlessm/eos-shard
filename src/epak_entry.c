
#include "epak_entry.h"

#include <stdio.h>

#include "epak.h"
#include "epak_fmt.h"

struct _EpakEntryPrivate {
  EpakPak *pak;
  struct epak_doc_entry *doc;
};

typedef struct _EpakEntryPrivate EpakEntryPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (EpakEntry, epak_entry, G_TYPE_OBJECT)

static void
epak_entry_dispose (GObject *object)
{
  EpakEntry *entry = EPAK_ENTRY (object);
  EpakEntryPrivate *priv = epak_entry_get_instance_private (entry);

  g_clear_object (&priv->pak);

  G_OBJECT_CLASS (epak_entry_parent_class)->dispose (object);
}

static void
epak_entry_class_init (EpakEntryClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = epak_entry_dispose;
}

static void 
epak_entry_init (EpakEntry *entry)
{
}

/**
 * epak_entry_get_raw_name:
 *
 * Get the raw name of an entry, which is a series of 20
 * bytes, which represent a SHA-1 hash.
 *
 * Returns: (transfer none): the name
 */
uint8_t *
epak_entry_get_raw_name (EpakEntry *entry)
{
  EpakEntryPrivate *priv = epak_entry_get_instance_private (entry);
  return (uint8_t*) priv->doc->raw_name;
}

/**
 * epak_entry_get_hex_name:
 *
 * Returns a debug name of an entry, which is the same as
 * a SHA-1 hash but as a readable hex string.
 *
 * Returns: (transfer full): the debug name
 */
char *
epak_entry_get_hex_name (EpakEntry *entry)
{
  EpakEntryPrivate *priv = epak_entry_get_instance_private (entry);
  char *hex_name = g_malloc (41);
  epak_util_raw_name_to_hex_name (hex_name, priv->doc->raw_name);
  hex_name[40] = '\0';
  return hex_name;
}

/**
 * epak_entry_read_metadata:
 *
 * Synchronously read and return the metadata blob as a #GBytes.
 */
GBytes *
epak_entry_read_metadata (EpakEntry *entry)
{
  EpakEntryPrivate *priv = epak_entry_get_instance_private (entry);
  return _epak_pak_load_blob (priv->pak, &priv->doc->metadata);
}

/**
 * epak_entry_read_data:
 *
 * Synchronously read and return the data blob as a #GBytes.
 */
GBytes *
epak_entry_read_data (EpakEntry *entry)
{
  EpakEntryPrivate *priv = epak_entry_get_instance_private (entry);
  return _epak_pak_load_blob (priv->pak, &priv->doc->data);
}

EpakEntry *
_epak_entry_new_for_doc (EpakPak *pak, struct epak_doc_entry *doc)
{
  EpakEntry *entry = g_object_new (EPAK_TYPE_ENTRY, NULL);
  EpakEntryPrivate *priv = epak_entry_get_instance_private (entry);
  priv->pak = g_object_ref (pak);
  priv->doc = doc;
  return entry;
}
