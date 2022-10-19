#include "print.h"
#include "sbi.h"

extern void test();

int start_kernel() {
    puti(2022);
    puts(" Hello RISC-V 3200105947\n");

    test(); // DO NOT DELETE !!!

	return 0;
}
