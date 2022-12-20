#include "printk.h"
#include "sbi.h"
#include "mm.h"
#include "proc.h"
#include "defs.h"

extern void test();

int start_kernel() 
{
    printk("Hello RISC-V\n");
    printk("idle process is running!\n");
    printk("[S-MODE] Hello RISC-V\n");
    schedule();

    test();

	return 0;
}

