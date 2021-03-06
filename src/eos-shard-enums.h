
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
 * @EOS_SHARD_ERROR_SHARD_FILE_NOT_FOUND: Assertion failure
 * @EOS_SHARD_ERROR_SHARD_FILE_PATH_NOT_SET: Assertion failure
 * @EOS_SHARD_ERROR_SHARD_FILE_COULD_NOT_OPEN: Assertion failure
 * @EOS_SHARD_ERROR_SHARD_FILE_CORRUPT: Assertion failure
 * @EOS_SHARD_ERROR_BLOB_STREAM_READ: Assertion failure
 * @EOS_SHARD_ERROR_DICTIONARY_WRITER_WRONG_NUMBER_ENTRIES: Assertion failure
 * @EOS_SHARD_ERROR_DICTIONARY_WRITER_ENTRIES_OUT_OF_ORDER: Assertion failure
 *
 * Error codes for the %EOS_SHARD_ERROR error domain.
 */
typedef enum {
  EOS_SHARD_ERROR_SHARD_FILE_NOT_FOUND,
  EOS_SHARD_ERROR_SHARD_FILE_PATH_NOT_SET,
  EOS_SHARD_ERROR_SHARD_FILE_COULD_NOT_OPEN,
  EOS_SHARD_ERROR_SHARD_FILE_CORRUPT,
  EOS_SHARD_ERROR_BLOB_STREAM_READ,
  EOS_SHARD_ERROR_BLOB_CHECKSUM_MISMATCH,
  EOS_SHARD_ERROR_BLOOM_FILTER_CORRUPT,
  EOS_SHARD_ERROR_DICTIONARY_CORRUPT,
  EOS_SHARD_ERROR_DICTIONARY_WRITER_WRONG_NUMBER_ENTRIES,
  EOS_SHARD_ERROR_DICTIONARY_WRITER_ENTRIES_OUT_OF_ORDER,

  /*< private >*/
  EOS_SHARD_ERROR_LAST
} EosShardError;

GType eos_shard_error_get_type (void);
GQuark eos_shard_error_quark (void);

#endif /* __EOS_SHARD_ENUMS_H__ */
