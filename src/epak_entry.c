#include "epak_entry.h"

struct _EpakEntryPrivate
{
    int foo;    
};

typedef struct _EpakEntryPrivate EpakEntryPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (EpakEntry, epak_entry, G_TYPE_OBJECT)

static void
epak_entry_class_init (EpakEntryClass *klass)
{
}

static void 
epak_entry_init (EpakEntry *entry)
{
}
