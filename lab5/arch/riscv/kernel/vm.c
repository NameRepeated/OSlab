#include "defs.h"
#include "types.h"
#include "printk.h"
#include "string.h"
#include "mm.h"

extern char _stext[];
extern char _etext[];
extern char _srodata[];
extern char _erodata[];
extern char _sdata[];

#define VPN0MASK ((uint64) 0x1FF << 12)
#define VPN1MASK ((uint64) 0x1FF << 21)
#define VPN2MASK ((uint64) 0x1FF << 30)

#define PPN0MASK ((uint64) 0x1FF << 12)
#define PPN1MASK ((uint64) 0x1FF << 21)
#define PPN2MASK ((uint64) 0x3FFFFFF << 30)

#define PTEMASK (((uint64) 0xFFFFFFFFFFF) << 10)

/* early_pgtbl: ⽤于 setup_vm 进⾏ 1GB 的 映射。 */
unsigned long early_pgtbl[512] __attribute__((__aligned__(0x1000)));
unsigned long swapper_pg_dir[512] __attribute__((__aligned__(0x1000)));

void setup_vm(void) {
/*
1. 由于是进⾏ 1GB 的映射 这⾥不需要使⽤多级⻚表
2. 将 va 的 64bit 作为如下划分： | high bit | 9 bit | 30 bit |
high bit 可以忽略
中间9 bit 作为 early_pgtbl 的 index
低 30 bit 作为 ⻚内偏移 这⾥注意到 30 = 9 + 9 + 12， 即我们只使⽤根⻚表， 根⻚表的每个 entry 都对应 1GB 的区
域。
3. Page Table Entry 的权限 V | R | W | X 位设置为 1
*/
    uint64 index;
    memset(early_pgtbl, 0x0, PGSIZE);
    uint64 addr_tmp = PHY_START;
    uint64 phy_index = (addr_tmp & PPN2MASK) >> 30;
    index = phy_index;
    early_pgtbl[index] = (phy_index << 28) | 0xF;

    addr_tmp = VM_START;
    index = (addr_tmp & VPN2MASK) >> 30;
    early_pgtbl[index] = (phy_index << 28) | 0xF;
}
void create_mapping(uint64 *pgtbl, uint64 va, uint64 pa, uint64 sz, int perm) {
    /*
    pgtbl 为根页表的基地址
    va, pa 为需要映射的虚拟地址、物理地址
    sz 为映射的大小
    perm 为映射的读写权限

    创建多级页表的时候可以使用 kalloc() 来获取一页作为页表目录
    可以使用 V bit 来判断页表项是否存在
    */
    uint64 index_two, addr_tmp, index_one, index_zero;
    uint64 * fir_pgtbl, * zero_pgtbl;
    for(uint64 offset = 0; offset < sz; offset += PGSIZE)
    {
        addr_tmp =  va + offset;
        index_two = (addr_tmp & VPN2MASK) >> 30;
        index_one = (addr_tmp & VPN1MASK) >> 21;
        index_zero = (addr_tmp & VPN0MASK) >> 12;

        if(pgtbl[index_two] & 0x1)
        {
            fir_pgtbl = (uint64*) (((uint64)(pgtbl[index_two] & PTEMASK) >> 10 << 12) + PA2VA_OFFSET);
            if(fir_pgtbl[index_one] & 0x1)
            {
                zero_pgtbl = (uint64*) (((uint64)(fir_pgtbl[index_one] & PTEMASK) >> 10 << 12) + PA2VA_OFFSET);
                if((zero_pgtbl[index_zero] & 0x1))
                {
                    continue;
                }
                else{
                    zero_pgtbl[index_zero] = (((pa + offset) >> 12) << 10) | perm;
                }
            }
            else
            {
                zero_pgtbl = (uint64 *)kalloc(); 
                fir_pgtbl[index_one] = ((((uint64)zero_pgtbl - PA2VA_OFFSET) >> 12) << 10) | 0x1;
                zero_pgtbl[index_zero] = (((pa + offset) >> 12) << 10)  | perm;
            }
        }
        else
        {
            fir_pgtbl = (uint64 *)kalloc();
            zero_pgtbl = (uint64 *)kalloc();
            pgtbl[index_two] = ((((uint64)fir_pgtbl - PA2VA_OFFSET) >> 12) << 10) | 0x1;
            fir_pgtbl[index_one] = ((((uint64)zero_pgtbl - PA2VA_OFFSET) >> 12) << 10) | 0x1;
            zero_pgtbl[index_zero] = (((pa + offset) >> 12) << 10) | perm;
        }
    }
}
/* swapper_pg_dir: kernel pagetable 根⽬录， 在 setup_vm_final 进⾏映射。*/

void setup_vm_final(void) {
    memset(swapper_pg_dir, 0x0, PGSIZE);
    uint64 addr_start;
    uint64 addr_end;
    // No OpenSBI mapping required  

    // mapping kernel text X|-|R|V
    addr_start = (uint64)_stext;
    addr_end = (uint64)_etext;
    create_mapping(swapper_pg_dir, addr_start, addr_start - PA2VA_OFFSET, addr_end - addr_start, 11);

    // mapping kernel rodata -|-|R|V
    addr_start = (uint64)_srodata;
    addr_end = (uint64)_erodata;
    create_mapping(swapper_pg_dir, addr_start, addr_start - PA2VA_OFFSET, addr_end - addr_start, 3);

    // mapping other memory -|W|R|V
    addr_start = (uint64)_sdata;
    addr_end = (uint64)(VM_START + PHY_SIZE);
    create_mapping(swapper_pg_dir, addr_start, addr_start - PA2VA_OFFSET, addr_end - addr_start, 7);
    
    // set satp with swapper_pg_dir
    csr_write(satp, ((((uint64)(swapper_pg_dir) - PA2VA_OFFSET) >> 12) | ((uint64) 0x8 << 60)));
    // flush TLB
    asm volatile("sfence.vma zero, zero");
    //printk("..setup_vm_final done!\n");
    // test for write in rodata
    // unsigned long int*p = 0xffffffe000202000;
    // unsigned long int raw=*p;
    // *p=raw;

    return;
}
