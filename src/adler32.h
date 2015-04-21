/* q&d adler32 */

#pragma once

#include <stdint.h>

#define ADLER32_INIT 1

uint32_t adler32(uint32_t c, uint8_t *data, int size);
