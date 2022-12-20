#include "trap.h"
#include "types.h"
#include "clock.h"
#include "printk.h"
#include "proc.h"
#include "syscall.h"

void handler_interrupt(unsigned long scause, unsigned long sepc,struct pt_regs* regs) 
{
    switch (scause & ~TRAP_MASK) {
        case STI:
            // printk("[S] Supervisor Mode Timer Interrupt\n");
            // clock_set_next_event();
            do_timer();
            clock_set_next_event();
            break;
        default:
            printk("Int scause:%d\n", scause);
            break;
    }
}

void handler_exception(unsigned long scause, unsigned long sepc,struct pt_regs* regs) 
{
    unsigned long ecall_from_umode = 0x8;
        if (scause == ecall_from_umode) 
        { // it's environmental call from U-mode
            switch (regs->reg[17]) 
            { // x17: ecall number, x9: a0 (return value)
                case 64:
                    regs->reg[10] = sys_write(regs->reg[10], (const char*)regs->reg[11], regs->reg[12]);
                    break;
                case 172:
                    regs->reg[10] = sys_getpid();
                    break;
            }
        regs->sepc += 4; // return to the next inst after ecall
        }
        else
        {
            if(scause == 12)
            {

                printk("Scause:%d Instruction Page Fault\n", scause);
                regs->sepc+=4;
                for (unsigned int i = 0; i < 0x4FFFFFFF; i++);
                
            }
            else
            {
                printk("Scause:%d\n", scause);
                regs->sepc+=4;
            }
                
        }
}

void trap_handler(unsigned long scause, unsigned long sepc,struct pt_regs* regs) 
{
    if (scause & TRAP_MASK)                    
        handler_interrupt(scause, sepc,regs);
    else
        handler_exception(scause, sepc,regs);
}