#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern void asmb(unsigned long x[], size_t n);

void check(unsigned long x[], size_t n) {
    unsigned long carry = 0;
    unsigned long next_carry;
    long i;
    for (i = n - 1; i >= 0; i--) {
        next_carry = x[i] << 63;
        x[i] = (x[i] >> 1) | carry;
        carry = next_carry;
    }
}

void test(unsigned long x[], size_t n) {
    unsigned long *x_check, *x_asm;
    int err = 0;
    int i;

    x_check = malloc(n * sizeof(unsigned long));
    x_asm = malloc(n * sizeof(unsigned long));

    memcpy(x_check, x, n * sizeof(unsigned long));
    memcpy(x_asm, x, n * sizeof(unsigned long));

    check(x_check, n);
    asmb(x_asm, n);

    printf("ASM: ");
    for (i = 0; i < n; i++) {
        if (x_asm[i] != x_check[i]) {
            err = 1;
        }
        printf("%lu, ", x_asm[i]);
    }
    printf("\n");

    printf("CHK: ");
    for (i = 0; i < n; i++) {
        printf("%lu, ", x_check[i]);
    }
    printf("\n");

    if (err) {
        printf("ERR\n");
    }

    free(x_check);
    free(x_asm);
}

#define sizeofarray(x) (sizeof(x) / sizeof(x[0]))

int main(int argc, const char **argv) {

    int i;
    unsigned long x1[] = { -1, 0, 1, 2, 3, 4, 5 };
    unsigned long x2[] = { 1 };
    unsigned long x3[] = { 0xffffffffffffffff, 0xffffffffffffffff, 0xffffffffffffffff };
    unsigned long x4[] = { 3, 3, 3, 3, 3 };
    unsigned long x5[] = { 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0 };

    test(x1, sizeofarray(x1));
    test(x2, sizeofarray(x2));
    test(x2, 0);
    test(x3, sizeofarray(x3));
    test(x4, sizeofarray(x4));
    test(x5, sizeofarray(x5));

    return 0;
}
