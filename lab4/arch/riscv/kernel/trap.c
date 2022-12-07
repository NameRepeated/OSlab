#include "trap.h"
#include "types.h"
#include "clock.h"
#include "printk.h"
#include "proc.h"


void handler_interrupt(unsigned long scause, unsigned long sepc,unsigned long regs) 
{
    switch (scause & ~TRAP_MASK) {
        case STI:
            // printk("[S] Supervisor Mode Timer Interrupt\n");
            // clock_set_next_event();
            do_timer();
            break;
        default:
            break;
    }
}

void handler_exception(unsigned long scause, unsigned long sepc,unsigned long regs) 
{
}

void trap_handler(unsigned long scause, unsigned long sepc,unsigned long regs) 
{
    if (scause & TRAP_MASK)                    // #define TRAP_MASK (1UL << 63)
        handler_interrupt(scause, sepc,regs);
    else
        handler_exception(scause, sepc,regs);
}