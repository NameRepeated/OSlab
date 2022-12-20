.equ reg_size, 0x8
    .section .text.entry
    .align 2
    .globl _traps
_traps:
    addi sp, sp, -16
    sd t0, 0(sp)
    sd t1, 8(sp)

    csrr t0, sstatus
    andi t0, t0, 0x100 # take SPP bit
    addi t1, x0, 0
    bne t0, t1, _context  
    csrr t1, sscratch  
    csrw sscratch, sp
    add sp, t1, x0
_context:
    # save  registers 
    addi sp, sp, -33*reg_size
    sd x0, 0*reg_size(sp)
    sd x1, 1*reg_size(sp)
    sd x2, 2*reg_size(sp)
    sd x3, 3*reg_size(sp)
    sd x4, 4*reg_size(sp)
    sd x5, 5*reg_size(sp)
    sd x6, 6*reg_size(sp)
    sd x7, 7*reg_size(sp)
    sd x8, 8*reg_size(sp)
    sd x9, 9*reg_size(sp)
    sd x10, 10*reg_size(sp)
    sd x11, 11*reg_size(sp)
    sd x12, 12*reg_size(sp)
    sd x13, 13*reg_size(sp)
    sd x14, 14*reg_size(sp)
    sd x15, 15*reg_size(sp)
    sd x16, 16*reg_size(sp)
    sd x17, 17*reg_size(sp)
    sd x18, 18*reg_size(sp)
    sd x19, 19*reg_size(sp)
    sd x20, 20*reg_size(sp)
    sd x21, 21*reg_size(sp)
    sd x22, 22*reg_size(sp)
    sd x23, 23*reg_size(sp)
    sd x24, 24*reg_size(sp)
    sd x25, 25*reg_size(sp)
    sd x26, 26*reg_size(sp)
    sd x27, 27*reg_size(sp)
    sd x28, 28*reg_size(sp)
    sd x29, 29*reg_size(sp)
    sd x30, 30*reg_size(sp)
    sd x31, 31*reg_size(sp)
    csrr t0, sepc
    sd t0, 32*reg_size(sp)

    csrr a0, scause
    csrr a1, sepc
    mv a2, sp
    call trap_handler


    # load sepc and 31 register (x2(sp) should be loaded last)
    ld t0, 32*reg_size(sp)
    csrw sepc, t0

    ld x0, 0*reg_size(sp)
    ld x1, 1*reg_size(sp)
    ld x3, 3*reg_size(sp)
    ld x4, 4*reg_size(sp)
    ld x5, 5*reg_size(sp)
    ld x6, 6*reg_size(sp)
    ld x7, 7*reg_size(sp)
    ld x8, 8*reg_size(sp)
    ld x9, 9*reg_size(sp)
    ld x10, 10*reg_size(sp)
    ld x11, 11*reg_size(sp)
    ld x12, 12*reg_size(sp)
    ld x13, 13*reg_size(sp)
    ld x14, 14*reg_size(sp)
    ld x15, 15*reg_size(sp)
    ld x16, 16*reg_size(sp)
    ld x17, 17*reg_size(sp)
    ld x18, 18*reg_size(sp)
    ld x19, 19*reg_size(sp)
    ld x20, 20*reg_size(sp)
    ld x21, 21*reg_size(sp)
    ld x22, 22*reg_size(sp)
    ld x23, 23*reg_size(sp)
    ld x24, 24*reg_size(sp)
    ld x25, 25*reg_size(sp)
    ld x26, 26*reg_size(sp)
    ld x27, 27*reg_size(sp)
    ld x28, 28*reg_size(sp)
    ld x29, 29*reg_size(sp)
    ld x30, 30*reg_size(sp)
    ld x31, 31*reg_size(sp)
    ld t0,  32*reg_size(sp)
    ld x2, 2*reg_size(sp)
    addi sp, sp, 33*reg_size
    

    # resume original stack pointer (if trapped from U mode)
    csrr t0, sstatus
    andi t0, t0, 0x100 # take SPP bit
    addi t1, x0, 0
    bne t0, t1, _smode_trap_2  # SPP != 0: S-mode, don't need to switch stack
    csrr t1, sscratch  # switch stack pointers for U mode and S mode
    csrw sscratch, sp
    add sp, t1, x0
_smode_trap_2:

    # restore t0 and t1
    ld t0, 0(sp)
    ld t1, 8(sp)
    addi sp, sp, 16

    sret



    .global __dummy
__dummy:
    # YOUR CODE HERE
    #la t0, dummy
    #csrw sepc, t0
    #sret
    csrr t1, sscratch  # switch stack pointers for U mode and S mode
    csrw sscratch, sp
    mv sp,t1
    sret

    .globl __switch_to
__switch_to:
    # save state to prev process
    # YOUR CODE HERE
    # a0: prev
    # a1: next
    sd ra, 40(a0)
    sd sp, 48(a0)
    sd s0, 56(a0)
    sd s1, 64(a0)
    sd s2, 72(a0)
    sd s3, 80(a0)
    sd s4, 88(a0)
    sd s5, 96(a0)
    sd s6, 104(a0)
    sd s7, 112(a0)
    sd s8, 120(a0)
    sd s9, 128(a0)
    sd s10, 136(a0)
    sd s11, 144(a0)
    csrr t1, sepc
    sd t1, 152(a0)
    csrr t1, sstatus
    sd t1, 160(a0)
    csrr t1, sscratch
    sd t1, 168(a0)
    
    # restore state from next process
    ld ra, 40(a1)
    ld sp, 48(a1)
    ld s0, 56(a1)
    ld s1, 64(a1)
    ld s2, 72(a1)
    ld s3, 80(a1)
    ld s4, 88(a1)
    ld s5, 96(a1)
    ld s6, 104(a1)
    ld s7, 112(a1)
    ld s8, 120(a1)
    ld s9, 128(a1)
    ld s10, 136(a1)
    ld s11, 144(a1)
    ld t1,  152(a1)
    csrw sepc, t1
    ld t0, 160(a1)
    csrw sstatus,t0
    ld t1, 168(a1)
    csrw sscratch , t1

    # switch page table
    ld t0, 176(a1) # load new process' page table
    li t1, 0x8000000000000000
    or t0, t0, t1
    csrw satp, t0
    
    # flush tlb
    sfence.vma zero, zero
    # flush icache
    fence.i
    ret
