#include <stdio.h>

extern void asma(unsigned long x[]);

void check(unsigned long x[]) {
    unsigned long carry;
    carry = x[1] << 63;
    printf("carry: %lu ", carry);
    printf("check: (%lu, %lu) ",
            (x[0] >> 1) | carry, x[1] >> 1);
}

int main(int argc, const char **argv) {
    unsigned long x[2] = { strtol(argv[1], NULL, 0), strtol(argv[2], NULL, 0) };

    printf("input: (%d, %d) ", x[0], x[1]);

    check(x);

    asma(x);
    printf("asma: (%lu, %lu)\n", x[0], x[1]);

    return 0;
}
