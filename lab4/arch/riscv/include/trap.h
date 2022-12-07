#pragma once

#define TRAP_MASK  0x8000000000000000
#define STI 5
void handler_interrupt(unsigned long scause, unsigned long sepc,unsigned long regs);
void handler_exception(unsigned long scause, unsigned long sepc,unsigned long regs);
void trap_handler(unsigned long scause, unsigned long sepc,unsigned long regs);
