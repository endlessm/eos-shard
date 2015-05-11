
#include "epak-enums.h"

#define EPAK_DEFINE_ENUM_VALUE(value,nick) \
  { value, #value, nick },

#define EPAK_DEFINE_ENUM_TYPE(TypeName,type_name,values) \
GType \
type_name ## _get_type (void) \
{ \
  static volatile gsize g_define_id__volatile = 0; \
  if (g_once_init_enter (&g_define_id__volatile)) \
    { \
      static const GEnumValue v[] = { \
        values \
        { 0, NULL, NULL }, \
      }; \
      GType g_define_id = g_enum_register_static (g_intern_static_string (#TypeName), v); \
      g_once_init_leave (&g_define_id__volatile, g_define_id); \
    } \
  return g_define_id__volatile; \
}

G_DEFINE_QUARK (epak-error-quark, epak_error)

EPAK_DEFINE_ENUM_TYPE (EpakError, epak_error,
  EPAK_DEFINE_ENUM_VALUE (EPAK_ERROR_PAK_NOT_FOUND, "pak-not-found")
  EPAK_DEFINE_ENUM_VALUE (EPAK_ERROR_PAK_PATH_NOT_SET, "pak-path-not-set")
  EPAK_DEFINE_ENUM_VALUE (EPAK_ERROR_PAK_COULD_NOT_OPEN, "pak-could-not-open")
  EPAK_DEFINE_ENUM_VALUE (EPAK_ERROR_PAK_CORRUPT, "pak-corrupt")
  EPAK_DEFINE_ENUM_VALUE (EPAK_ERROR_BLOB_STREAM_READ, "blob-stream-read")
  EPAK_DEFINE_ENUM_VALUE (EPAK_ERROR_LAST, "type-last"))
