
#ifndef __EPAK_ENUMS_H__
#define __EPAK_ENUMS_H__

#include <glib-object.h>

#define EPAK_TYPE_ERROR                       (epak_error_get_type ())

/**
 * EPAK_ERROR:
 *
 * Error domain for Epak.
 */
#define EPAK_ERROR                            (epak_error_quark ())

/**
 * EpakError:
 * @EPAK_ERROR_PAK_NOT_FOUND: Assertion failure
 * @EPAK_ERROR_PAK_PATH_NOT_SET: Assertion failure
 * @EPAK_ERROR_PAK_COULD_NOT_OPEN: Assertion failure
 * @EPAK_ERROR_PAK_CORRUPT: Assertion failure
 * @EPAK_ERROR_BLOB_STREAM_READ: Assertion failure
 *
 * Error codes for the %EPAK_ERROR error domain.
 */
typedef enum {
  EPAK_ERROR_PAK_NOT_FOUND,
  EPAK_ERROR_PAK_PATH_NOT_SET,
  EPAK_ERROR_PAK_COULD_NOT_OPEN,
  EPAK_ERROR_PAK_CORRUPT,
  EPAK_ERROR_BLOB_STREAM_READ,

  /*< private >*/
  EPAK_ERROR_LAST
} EpakError;

GType epak_error_get_type (void);
GQuark epak_error_quark (void);

#endif /* __EPAK_ENUMS_H__ */
