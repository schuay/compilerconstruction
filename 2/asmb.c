#include <stddef.h>

void asmb(unsigned long x[], size_t n) {
    unsigned long carry = 0;
    unsigned long next_carry;
    long i;
    for (i = n - 1; i >= 0; i--) {
        next_carry = x[i] << 63;
        x[i] = (x[i] >> 1) | carry;
        carry = next_carry;
    }
}
