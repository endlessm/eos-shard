
#include "eos-shard-enums.h"

#define EOS_SHARD_DEFINE_ENUM_VALUE(value,nick) \
  { value, #value, nick },

#define EOS_SHARD_DEFINE_ENUM_TYPE(TypeName,type_name,values) \
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

G_DEFINE_QUARK (eos-shard-error-quark, eos_shard_error)

EOS_SHARD_DEFINE_ENUM_TYPE (EosShardError, eos_shard_error,
  EOS_SHARD_DEFINE_ENUM_VALUE (EOS_SHARD_ERROR_SHARD_FILE_NOT_FOUND, "shard-file-not-found")
  EOS_SHARD_DEFINE_ENUM_VALUE (EOS_SHARD_ERROR_SHARD_FILE_PATH_NOT_SET, "shard-file-path-not-set")
  EOS_SHARD_DEFINE_ENUM_VALUE (EOS_SHARD_ERROR_SHARD_FILE_COULD_NOT_OPEN, "shard-file-could-not-open")
  EOS_SHARD_DEFINE_ENUM_VALUE (EOS_SHARD_ERROR_SHARD_FILE_CORRUPT, "shard-file-corrupt")
  EOS_SHARD_DEFINE_ENUM_VALUE (EOS_SHARD_ERROR_BLOB_CHECKSUM_MISMATCH, "blob-checksum-mismatch")
  EOS_SHARD_DEFINE_ENUM_VALUE (EOS_SHARD_ERROR_BLOB_STREAM_READ, "blob-stream-read")
  EOS_SHARD_DEFINE_ENUM_VALUE (EOS_SHARD_ERROR_LAST, "type-last"))
