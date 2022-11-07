#include "printk.h"
#include "sbi.h"
#include "proc.h"
#include "mm.h"

extern void test();

int start_kernel() {
    printk("Hello RISC-V\n");
    printk("idle process is running!\n");
    // mm_init();
    // task_init();
    test(); // DO NOT DELETE !!!

	return 0;
}
