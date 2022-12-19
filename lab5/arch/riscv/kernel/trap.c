#include "trap.h"
#include "types.h"
#include "clock.h"
#include "proc.h"
#include "printk.h"
#include "syscall.h"
#define IS_TRAP (1UL << 63)


void interrupt_handler(unsigned long scause, unsigned long sepc, struct pt_regs * regs) {
    if((scause & ~IS_TRAP) == 5){
            //printk("[S] Supervisor Mode Timer Interrupt\n");
            clock_set_next_event();
            do_timer();       
    }
    else{
        printk("Int scause:%d\n", scause);
    }
}

void exception_handler(unsigned long scause, unsigned long sepc, struct pt_regs * regs) {
    switch (scause){
            case 0x8:
                switch (regs->a7){
                    case 64:
                        if(regs -> a0 == 1){
                            char buf[1000] = {0};
                            memcpy(buf, (void *)regs -> a1, regs -> a2);
                            regs -> a0 = sys_write(buf);
                        }
                        break;
                    case 172:
                        regs -> a0 = sys_getpid();
                        break;
                }
                regs->sepc+=4;
                break;
            default:
                if(scause == 12)
                printk("Scause:%d Instruction Page Fault\n", scause);
                else
                printk("Scause:%d\n", scause);
                regs->sepc+=4;
                break;
        }
}

void trap_handler(unsigned long scause, unsigned long sepc, struct pt_regs * regs) {

    //printk("scause: 0x%lx, sepc: %lx\n", scause, sepc);
    if (scause & IS_TRAP)   
        interrupt_handler(scause, sepc, regs);
    else
        exception_handler(scause, sepc, regs);
}
