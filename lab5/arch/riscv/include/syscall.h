#ifndef _SYSCALL_H_
#define _SYSCALL_H_

#include "stdint.h"

#define SYS_WRITE   64
#define SYS_GETPID  172

unsigned long sys_write(const char* buf);

unsigned long sys_getpid();

#endif