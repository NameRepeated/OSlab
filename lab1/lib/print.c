#include "print.h"
#include "sbi.h"
#include "types.h"

void puts(char *s) {
    // unimplemented
    int i = 0;
    while(s[i] != '\0') {
        sbi_ecall(SBI_PUTCHAR, 0x0, (int)s[i++], 0, 0, 0, 0, 0);
    }
}

void puti(int x) {
    // unimplemented
    int foo;
    if (x > 0) {
        foo = x%10;
        puti(x/10);
        sbi_ecall(SBI_PUTCHAR, 0x0, 0x30+foo, 0, 0, 0, 0, 0);
    }
}
