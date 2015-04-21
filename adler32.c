/* q&d adler32 */

#include "adler32.h"
#include <unistd.h>

uint32_t adler32(uint32_t c, uint8_t *data, int size)
{
    uint32_t a, b;
    off_t i;
    const uint32_t mod = 65521;

    b = c >> 16;
    a = c & 0xFFFF;

    for (i = 0; i < size; i++) {
        a = (a + data[i]) % mod;
        b = (b + a) % mod;
    }

    return (b << 16) | a;
}
