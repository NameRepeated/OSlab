//arch/riscv/kernel/proc.c
#include "proc.h"
#include "mm.h"
#include "clock.h"
#include "printk.h"
#include "rand.h"
#include "defs.h"
#include "vm.h"
#include "elf.h"

extern void __dummy();
extern void __switch_to(struct task_struct* prev, struct task_struct* next);
extern char uapp_start[];
extern char uapp_end[];
extern uint64 swapper_pg_dir[512];

struct task_struct* idle;           // idle process
struct task_struct* current;        // 指向当前运行线程的 `task_struct`
struct task_struct* task[NR_TASKS]; // 线程数组, 所有的线程都保存在此
//dummy 
void dummy() {
    uint64 MOD = 1000000007;
    uint64 auto_inc_local_var = 0;
    int last_counter = -1;  
    while(1) {
        if (last_counter == -1 || current->counter != last_counter) {
            last_counter = current->counter;
            auto_inc_local_var = (auto_inc_local_var + 1) % MOD;
            printk("[PID = %d] is running. thread space begin at %lx\n", current->pid, current->thread.sp);
            //printk("[PID = %d] is running. auto_inc_local_var = %d\n", current->pid, auto_inc_local_var);
        }
    }
}
//switch to
extern void __switch_to(struct task_struct* prev, struct task_struct* next);
void switch_to(struct task_struct* next) 
{
    /* YOUR CODE HERE */
    if(current == next) 
        return;
    else 
    {
        struct task_struct *prev = current;
        current = next;
        __switch_to(prev, current);
    }
}

static uint64_t load_program(struct task_struct* task, pagetable_t pgtbl) {
    Elf64_Ehdr* ehdr = (Elf64_Ehdr*)(&uapp_start);

    uint64_t phdr_start = (uint64_t)ehdr + ehdr->e_phoff;
    int phdr_cnt = ehdr->e_phnum;

    Elf64_Phdr* phdr;
    int load_phdr_cnt = 0;
    for (int i = 0; i < phdr_cnt; i++) {
        phdr = (Elf64_Phdr*)(phdr_start + sizeof(Elf64_Phdr) * i);
        if (phdr->p_type == PT_LOAD) {

            uint64_t NumPG = (phdr->p_vaddr- PGROUNDDOWN(phdr->p_vaddr) + phdr->p_memsz - 1) / PGSIZE + 1;
            uint64_t tmpPG = alloc_pages(NumPG); 
            uint64_t tmpAddr = ((uint64_t)(uapp_start) + phdr->p_offset);
            memcpy((void*)(tmpPG + phdr->p_vaddr- PGROUNDDOWN(phdr->p_vaddr)), (void*)(tmpAddr), phdr->p_memsz); 

            create_mapping(pgtbl, PGROUNDDOWN(phdr->p_vaddr), tmpPG - PA2VA_OFFSET, NumPG * PGSIZE, ((phdr->p_flags)<< 1 | (1<<4)| 1));
        }
    }

    create_mapping(pgtbl, USER_END - PGSIZE, kalloc() - PA2VA_OFFSET, PGSIZE, 23);

    task -> thread.sepc = ehdr->e_entry; 
    task -> thread.sstatus = (csr_read(sstatus)) | (1<<5) | (1<<18) & 0xFFFFFFFFFFFFFEFF;
    task -> thread.sscratch = USER_END;
}

static uint64_t load_bin(struct task_struct* task, pagetable_t pgtbl)
{
        // SPP=0 SPIE=1 SUM=1        
        uint64_t NumPG = ((uint64_t)(&uapp_end) - (uint64_t)(&uapp_start) - 1) / PGSIZE + 1; 
        uint64_t tmpPG = alloc_pages(NumPG); 
        memcpy((void*)(tmpPG), (void*)(&uapp_start), NumPG * PGSIZE); 
        uint64_t u_stack_begin = alloc_page(); 

        create_mapping(pgtbl, 0, (unsigned long)tmpPG - PA2VA_OFFSET, NumPG*PGSIZE , 31);
        create_mapping(pgtbl, USER_END - PGSIZE, u_stack_begin - PA2VA_OFFSET, PGSIZE, 23);
        task -> thread.sepc = USER_START;
        task -> thread.sstatus = (csr_read(sstatus)) | (1<<5) | (1<<18) & 0xFFFFFFFFFFFFFEFF;
        task -> thread.sscratch = USER_END;
}

void task_init() {

    // 1. 调用 kalloc() 为 idle 分配一个物理页
    // 2. 设置 state 为 TASK_RUNNING;
    // 3. 由于 idle 不参与调度 可以将其 counter / priority 设置为 0
    // 4. 设置 idle 的 pid 为 0
    // 5. 将 current 和 task[0] 指向 idle
    idle = (struct task_struct *)kalloc();
    idle -> state = TASK_RUNNING;
    idle -> counter = 0xFFFFFFFFFFFFFFFF;
    idle -> priority = 0;
    idle -> pid = 0;
    current = idle;
    task[0] = idle;
    task[0] -> thread.sp = (uint64)task[0] + PGSIZE;

    // 1. 参考 idle 的设置, 为 task[1] ~ task[NR_TASKS - 1] 进行初始化
    // 2. 其中每个线程的 state 为 TASK_RUNNING, counter 为 0, priority 使用 rand() 来设置, pid 为该线程在线程数组中的下标。
    // 3. 为 task[1] ~ task[NR_TASKS - 1] 设置 `thread_struct` 中的 `ra` 和 `sp`,
    // 4. 其中 `ra` 设置为 __dummy （见 4.3.2）的地址,  `sp` 设置为 该线程申请的物理页的高地址
    for(int i = 1; i < NR_TASKS; i++){
        task[i] = (struct task_struct *)kalloc();
        task[i] -> state = TASK_RUNNING;
        task[i] -> counter = 0;
        task[i] -> priority = rand();
        task[i] -> pid = i;
        task[i] -> thread.ra = (uint64)__dummy;
        task[i] -> thread.sp = (uint64)task[i] + PGSIZE;

        pagetable_t pgtbl = (pagetable_t)kalloc(); 
        memcpy(pgtbl, swapper_pg_dir, sizeof(swapper_pg_dir)/sizeof(char));
        //   delete // down to change load mode 

        //load_program(task[i],pgtbl);
        load_bin(task[i],pgtbl);
        task[i] -> pgd = (pagetable_t) (((unsigned long)pgtbl - PA2VA_OFFSET) >> 12);
    }
    printk("...proc_init done!\n");
}
//do timer
void do_timer(void) 
{
    // 1. 如果当前线程是 idle 线程 直接进行调度
    if (current == idle) 
    {
        schedule();
    }
    // 2. 如果当前线程不是 idle 对当前线程的运行剩余时间减1 若剩余时间仍然大于0 则直接返回 否则进行调度
    else 
    {
        current->counter--;
        if (current->counter > 0) return;
        else schedule();
    }
    /* YOUR CODE HERE */
}
//schedule
void schedule(void) {
    /* YOUR CODE HERE */
#ifdef SJF
    uint64 next = 0, min_time = 0xFFFFFFFFFFFFFFFF;
    for(uint64 i = NR_TASKS - 1; i > 0; i--) 
    {
        if (!task[i]) continue;
        if(task[i]->state == TASK_RUNNING && task[i]->counter < min_time && task[i]->counter) 
        {
            min_time = task[i]->counter;
            next = i;
        }
    }
    if(!next) 
    {
        printk("\n");
        for(uint64 i = 1; i < NR_TASKS; i++)
        {
            task[i]->counter = rand();
            //printk("SET [PID = %d COUNTER = %d]\n", task[i]->pid, task[i]->counter);
        }
        schedule();
        return;
    } 
    else 
    {
        //printk("switch to [PID = %d COUNTER = %d]\n", task[next]->pid, task[next]->counter);
        switch_to(task[next]);
    }
#endif
//the same as linux open code
#ifdef PRIORITY    
    uint64 next = 0;
    uint64 max_counter = 0;
    while(1) {
        max_counter = next = 0;
        for(uint64 i = NR_TASKS - 1; i > 0; i--)
        {
            if (!task[i]) continue;
            if(task[i]->state == TASK_RUNNING && task[i]->counter > max_counter) 
            {
                max_counter = task[i]->counter;
                next = i;
            }
        }

        if(next) break;
        printk("\n");
        for(uint64 i = 1; i < NR_TASKS; i++) 
        {
            if (task[i]) 
            {
                task[i]->counter = (task[i]->counter >> 1) + task[i]->priority;
                //printk("SET [PID = %d PRIORITY = %d COUNTER = %d]\n", task[i]->pid, task[i]->priority, task[i]->counter);
            }
        }
    }
    //printk("\n");
    //printk("switch to [PID = %d PRIORITY = %d COUNTER = %d]\n", task[next]->pid, task[next]->priority, task[next]->counter);
    switch_to(task[next]);
#endif

}