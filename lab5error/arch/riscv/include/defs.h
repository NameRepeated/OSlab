#ifndef _DEFS_H
#define _DEFS_H

#define USER_START (0x0000000000000000) // user space start virtual address
#define USER_END   (0x0000004000000000) // user space end virtual address

#define OPENSBI_SIZE (0x200000)

#define VM_START (0xffffffe000000000)
#define VM_END   (0xffffffff00000000)
#define VM_SIZE  (VM_END - VM_START)
#define PA2VA_OFFSET (VM_START - PHY_START)
#define PGROUNDUP(addr) ((addr + PGSIZE - 1) & (~(PGSIZE - 1)))
#define PGROUNDDOWN(addr) (addr & (~(PGSIZE - 1)))
#define PGOFFSET(addr) ((addr) - PGROUNDDOWN(addr))


#define VA2PA(x) ((x - (uint64)PA2VA_OFFSET))
#define PA2VA(x) ((x + (uint64)PA2VA_OFFSET))

#define PHY_START 0x0000000080000000
#define PHY_SIZE  128 * 1024 * 1024 // 128MB,  QEMU 默认内存大小
#define PHY_END   (PHY_START + PHY_SIZE)
  
#define PGSIZE 0x1000 // 4KB


// #define csr_read(csr)                       \
// ({                                          \
//     register uint64 __v;                    \
//     asm volatile ("csrr "#csr  ",%0 "       \
//                     : :"=r" (__v)           \
//                     : "memory");            \
//     __v;                                    \
// })
#define csr_read(csr)                       \
({                                          \
    register uint64 __v;                    \
    asm volatile ("csrr %[v], " #csr    \
    : [v] "=r" (__v)::);     \
    __v;                                    \
})

#define csr_write(csr, val)                         \
({                                                  \
    uint64 __v = (uint64)(val);                     \
    asm volatile ("csrw " #csr ", %0"               \
                    : : "r" (__v)                   \
                    : "memory");                    \
})

#endif
