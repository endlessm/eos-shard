/* q&d adler32 */

#ifndef ADLER32_H
#define ADLER32_H

#include <stdint.h>

#define ADLER32_INIT 1

uint32_t adler32(uint32_t c, uint8_t *data, int size);

#endif /* ADLER32_H */
