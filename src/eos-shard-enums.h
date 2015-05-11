
#ifndef __EOS_SHARD_ENUMS_H__
#define __EOS_SHARD_ENUMS_H__

#include <glib-object.h>

#define EOS_SHARD_TYPE_ERROR                       (eos_shard_error_get_type ())

/**
 * EOS_SHARD_ERROR:
 *
 * Error domain for EosShard.
 */
#define EOS_SHARD_ERROR                            (eos_shard_error_quark ())

/**
 * EosShardError:
 * @EOS_SHARD_ERROR_SHARD_NOT_FOUND: Assertion failure
 * @EOS_SHARD_ERROR_SHARD_PATH_NOT_SET: Assertion failure
 * @EOS_SHARD_ERROR_SHARD_COULD_NOT_OPEN: Assertion failure
 * @EOS_SHARD_ERROR_SHARD_CORRUPT: Assertion failure
 * @EOS_SHARD_ERROR_BLOB_STREAM_READ: Assertion failure
 *
 * Error codes for the %EOS_SHARD_ERROR error domain.
 */
typedef enum {
  EOS_SHARD_ERROR_SHARD_NOT_FOUND,
  EOS_SHARD_ERROR_SHARD_PATH_NOT_SET,
  EOS_SHARD_ERROR_SHARD_COULD_NOT_OPEN,
  EOS_SHARD_ERROR_SHARD_CORRUPT,
  EOS_SHARD_ERROR_BLOB_STREAM_READ,

  /*< private >*/
  EOS_SHARD_ERROR_LAST
} EosShardError;

GType eos_shard_error_get_type (void);
GQuark eos_shard_error_quark (void);

#endif /* __EOS_SHARD_ENUMS_H__ */
