#include "printk.h"
#include "sbi.h"
#include "mm.h"
#include "proc.h"
#include "defs.h"

extern void test();

int start_kernel() {
    printk("[S-MODE] Hello RISC-V\n");
    schedule();

    test();

	return 0;
}

