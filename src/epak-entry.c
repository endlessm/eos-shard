
#include "epak-entry.h"

#include <stdio.h>

#include "epak-pak.h"
#include "epak-blob.h"
#include "epak_fmt.h"

static EpakEntry *
epak_entry_new (void)
{
  EpakEntry *entry = g_new0 (EpakEntry, 1);
  entry->ref_count = 1;
  return entry;
}

static void
epak_entry_free (EpakEntry *entry)
{
  g_object_unref (entry->pak);
  g_free (entry);
}

static EpakEntry *
epak_entry_ref (EpakEntry *entry)
{
  entry->ref_count++;
  return entry;
}

static void
epak_entry_unref (EpakEntry *entry)
{
  if (--entry->ref_count == 0)
    epak_entry_free (entry);
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
  return (uint8_t*) entry->doc->raw_name;
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
  char *hex_name = g_malloc (41);
  epak_util_raw_name_to_hex_name (hex_name, entry->doc->raw_name);
  hex_name[EPAK_HEX_NAME_SIZE] = '\0';
  return hex_name;
}

EpakEntry *
_epak_entry_new_for_doc (EpakPak *pak, struct epak_doc_entry *doc)
{
  EpakEntry *entry = epak_entry_new ();
  entry->pak = g_object_ref (pak);
  entry->doc = doc;
  entry->metadata = _epak_blob_new_for_blob (pak, &doc->metadata);
  entry->data = _epak_blob_new_for_blob (pak, &doc->data);
  return entry;
}

G_DEFINE_BOXED_TYPE (EpakEntry, epak_entry,
                     epak_entry_ref, epak_entry_unref)
