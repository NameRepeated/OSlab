#pragma once

#define TRAP_MASK  0x8000000000000000
#define STI 5
struct pt_regs
{
    unsigned long reg[32];
    unsigned long sepc;
};
void handler_interrupt(unsigned long scause, unsigned long sepc,struct pt_regs* regs);
void handler_exception(unsigned long scause, unsigned long sepc,struct pt_regs* regs);
void trap_handler(unsigned long scause, unsigned long sepc,struct pt_regs* regs);
