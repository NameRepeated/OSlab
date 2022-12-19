#include "syscall.h"
#include "printk.h"
#include "proc.h"

unsigned long sys_write(const char* buf) {
    return printk(buf);
}

unsigned long sys_getpid() {
    return current->pid;
}