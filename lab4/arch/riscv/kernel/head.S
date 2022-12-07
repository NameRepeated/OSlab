.extern start_kernel
#include "defs.h"
.extern setup_vm
.extern early_pgtbl
.extern setup_vm_final
.extern mm_init
.extern task_init

    .section .text.init
    .globl _start
    .align 2
_start:
    # YOUR CODE HERE
    # ------------------
    # set stvec = _trap
    la sp,boot_stack_top
    
    call setup_vm
    call relocate
    call mm_init
    call setup_vm_final
    la t0,_trap
    csrw stvec,t0
    #set SIE[STIE]=1
    csrr t0,sie
    ori t0,t0,0x20
    csrw sie,t0
    #set first time interrupt
    rdtime t0
    li t1, 10000000
    add a0, t0, t1
    add a6, zero, zero
    add a7, zero, zero
    ecall
    #set sstatus[SIE] = 1
    csrr t0, sstatus
    ori t0, t0, 0x2
    csrw sstatus, t0
    call task_init
    j start_kernel
    # ------------------
    relocate:
    # set ra = ra + PA2VA_OFFSET
    # set sp = sp + PA2VA_OFFSET (If you have set the sp before)
    li t0, PA2VA_OFFSET
    add ra, ra, t0
    add sp, sp, t0

    # set satp with early_pgtbl
    la t0, early_pgtbl
    srli t0, t0, 12
    li t1, 0x8000000000000000
    or t0, t0, t1
    csrw satp, t0

    # flush tlb
    sfence.vma zero, zero
    # flush icache
    fence.i
    ret
    
    .section .bss.stack
    .globl boot_stack
boot_stack:
    .space 4096 # <-- change to your stack size

    .globl boot_stack_top
boot_stack_top:
