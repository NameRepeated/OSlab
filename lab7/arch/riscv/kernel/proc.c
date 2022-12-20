//arch/riscv/kernel/proc.c
#include "proc.h"
#include "mm.h"
#include "clock.h"
#include "printk.h"
#include "rand.h"
#include "defs.h"
#include "elf.h"
#include "string.h"
#include "vm.h"
void SJF_schedule();
void Priority_schedule();
extern void __dummy();
extern void __switch_to(struct task_struct* prev, struct task_struct* next);


extern char uapp_start[];
extern char uapp_end[];
extern uint64 swapper_pg_dir[512];

struct task_struct* idle;           // idle process
struct task_struct* current;        // 指向当前运行线程的 `task_struct`
struct task_struct* task[NR_TASKS]; // 线程数组, 所有的线程都保存在此

static uint64 load_elf_program(struct task_struct* task) {
    Elf64_Ehdr* ehdr = (Elf64_Ehdr*)(&uapp_start);

    uint64 phdr_start = (uint64)ehdr + ehdr->e_phoff;
    int phdr_cnt = ehdr->e_phnum;

    Elf64_Phdr* phdr;
    int load_phdr_cnt = 0;
    for (int i = 0; i < phdr_cnt; i++) {
        phdr = (Elf64_Phdr*)(phdr_start + sizeof(Elf64_Phdr) * i);
        if (phdr->p_type == PT_LOAD) {
            // do mapping
            // compute # of pages for code
            uint64 pg_num = (PGOFFSET(phdr->p_vaddr) + phdr->p_memsz - 1) / PGSIZE + 1;
            uint64 uapp_new = alloc_pages(pg_num); // allocate new space for copied code
            uint64 load_addr = ((uint64)(&uapp_start) + phdr->p_offset);
            memcpy((void*)(uapp_new + PGOFFSET(phdr->p_vaddr)), (void*)(load_addr), phdr->p_memsz); // copy code
            // note we should open the U-bit switch
            create_mapping((uint64*)PA2VA((uint64)task->pgd), PGROUNDDOWN(phdr->p_vaddr), VA2PA(uapp_new), pg_num, phdr->p_flags | 0x8,1);
        }
    }

    // allocate user stack and do mapping
    uint64 u_stack_begin = alloc_page(); // allocate U-mode stack
    create_mapping((uint64*)PA2VA((uint64)task->pgd), USER_END - PGSIZE, VA2PA(u_stack_begin), 1, 11,1);

    // following code has been written for you
    // set user stack
    // pc for the user program
    task->thread.sepc = ehdr->e_entry; // the program starting address
    // sstatus bits set
    task->thread.sstatus = (1 << 18) | (1 << 5);
    // user stack for user program
    task->thread.sscratch = USER_END;
}

// static uint64 load_binary_program(struct task_struct* task) 
// {
//     // copy the user code to a new page
//     uint64 pg_num = ((uint64)(&uapp_end) - (uint64)(&uapp_start) - 1) / PGSIZE + 1; // compute # of pages for code
//     uint64 uapp_new = alloc_pages(pg_num); // allocate new space for copied code
//     memcpy((void*)(uapp_new), (void*)(&uapp_start), pg_num * PGSIZE); // copy code
//     uint64 u_stack_begin = alloc_page(); // allocate U-mode stack

//     // note the U bits for the following PTEs are set to 1
//     // mapping of user text segment
//     create_mapping((uint64*)PA2VA((uint64)task->pgd), 0, VA2PA(uapp_new), pg_num*PGSIZE, 7,1);
//     // mapping of user stack segment
//     create_mapping((uint64*)PA2VA((uint64)task->pgd), USER_END - PGSIZE, VA2PA(u_stack_begin), PGSIZE, 5,1);

//     // set CSRs
//     task->thread.sepc = USER_START; // set sepc at user space
//     task->thread.sstatus = (csr_read(sstatus)) | (1 << 18) | (1 << 5) & 0xFFFFFFFFFFFFFEFF; // set SPP = 0, SPIE = 1, SUM = 1
//     task->thread.sscratch = USER_END; // U-mode stack end (initial sp)
// }
static uint64 load_binary_program(struct task_struct* task, pagetable_t pgtbl){
        // SPP=0 SPIE=1 SUM=1        
        uint64 NumPG = ((uint64)(&uapp_end) - (uint64)(&uapp_start) - 1) / PGSIZE + 1; 
        uint64 tmpPG = alloc_pages(NumPG); 
        memcpy((void*)(tmpPG), (void*)(&uapp_start), NumPG * PGSIZE); 
        uint64 u_stack_begin = alloc_page(); 

        create_mapping(pgtbl, 0, (unsigned long)tmpPG - PA2VA_OFFSET, NumPG*PGSIZE , 7,1);
        create_mapping(pgtbl, USER_END - PGSIZE, u_stack_begin - PA2VA_OFFSET, 1*PGSIZE, 3,1);
        task -> thread.sepc = USER_START;
        task -> thread.sstatus = (csr_read(sstatus)) | (1<<5) | (1<<18) & 0xFFFFFFFFFFFFFEFF;
        task -> thread.sscratch = USER_END;
}

void task_init() {
    // 1. 调用 kalloc() 为 idle 分配一个物理页
    idle = (struct task_struct*)kalloc();
    // 2. 设置 state 为 TASK_RUNNING;
    idle->state = TASK_RUNNING;
    // 3. 由于 idle 不参与调度 可以将其 counter / priority 设置为 0
    idle->counter = 0;
    idle->priority = 0;
    // 4. 设置 idle 的 pid 为 0
    idle->pid = 0;
    idle->pgd = swapper_pg_dir;
    // 5. 将 current 和 task[0] 指向 idle
    current = idle;
    task[0] = idle;
    task[0] -> thread.sp = (uint64)task[0] + PGSIZE;
    // printk("idle->pid = %d,idle->counter = %d\n",idle->pid,idle->counter);
    /* YOUR CODE HERE */

    // 1. 参考 idle 的设置, 为 task[1] ~ task[NR_TASKS - 1] 进行初始化
    // 2. 其中每个线程的 state 为 TASK_RUNNING, counter 为 0, priority 使用 rand() 来设置, pid 为该线程在线程数组中的下标。
    // 3. 为 task[1] ~ task[NR_TASKS - 1] 设置 `thread_struct` 中的 `ra` 和 `sp`,
    // 4. 其中 `ra` 设置为 __dummy （见 4.3.2）的地址,  `sp` 设置为 该线程申请的物理页的高地址
    for(int i = 1; i < NR_TASKS; i++)
    {
        task[i] = (struct task_struct *)kalloc();
        task[i] -> state = TASK_RUNNING;
        task[i] -> counter = 0;
        // task[i]->counter = rand()%(COUNTER_MAX-COUNTER_MIN+1)+COUNTER_MIN;
        task[i] -> priority = rand();
        task[i] -> pid = i;
        pagetable_t tmp = (pagetable_t)alloc_page();
        memcpy((void*)tmp, (void*)swapper_pg_dir, PGSIZE);
        //load_elf_program(task[i]);
        load_binary_program(task[i],tmp);
        task[i] -> thread.ra = (uint64)__dummy;
        task[i] -> thread.sp = (uint64)task[i] + PGSIZE;
        // printk("task[%d]->pid = %d, counter = %d\n",i,task[i]->pid,task[i]->counter);
        //task[i]->pgd = (pagetable_t)alloc_page();
        task[i]->pgd = (pagetable_t) (((unsigned long)tmp - PA2VA_OFFSET) >> 12);
    }
    /* YOUR CODE HERE */

    printk("...proc_init done!\n");
}
void switch_to(struct task_struct* next) 
{
    /* YOUR CODE HERE */
    //printk("come in switch_to!");
    if(next->pid!=current->pid)
    {
        struct task_struct * prev = current;
        current = next;
        __switch_to(prev, next);
    }
    //printk("back");
}

// arch/riscv/kernel/proc.c
void dummy() 
{
    // printk("dummy in!");
    uint64 MOD = 1000000007;
    uint64 auto_inc_local_var = 0;
    int last_counter = -1;
    while(1) 
    {
        if (last_counter == -1 || current->counter != last_counter) 
        {
            last_counter = current->counter;
            auto_inc_local_var = (auto_inc_local_var + 1) % MOD;
            //printk("[PID = %d] is running. auto_inc_local_var = %d\n", current->pid, auto_inc_local_var);
            printk("[PID = %d] is running at %lx\n",current->pid,current->thread.sp);
        }
    }
}
void do_timer(void) {
    
    //printk("do_timer running\n");
    // printk("%d",2);
    // printk("current->pid = %d,current->counter = %d\n",idle->pid,idle->counter);
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
void schedule() 
{
    // printk("laji jianmo");
#ifdef SJF
    SJF_schedule();
#endif

#ifdef PRIORITY
    Priority_schedule();
#endif
}

void SJF_schedule() 
{
    // printk("SJF_schedule in!");
    struct task_struct* next = current;
    uint64 min_counter = COUNTER_MAX+1;
    while (1) 
    {
        for (int i = 1; i < NR_TASKS; i++) 
        {
            if (task[i]->counter == 0) continue;
            if (task[i]->counter < min_counter) {
                min_counter = task[i]->counter;
                next = task[i];
            }
        }

        if (min_counter == COUNTER_MAX+1) 
        {
            printk("Reset all counters!\n");
            for (int i = 1; i < NR_TASKS; i++) 
            {
                task[i]->counter = rand()%COUNTER_MAX+COUNTER_MIN;
                printk("SET [PID = %d PRIORITY = %d COUNTER = %d]\n", task[i]->pid, task[i]->priority, task[i]->counter);
            }
            
        }
        else break;
    }
    // printk("current->pid = %d",current->pid);
    // printk("next->pid = %d",next->pid);
    printk("switch to [PID = %d PRIORITY = %d COUNTER = %d]\n",next->pid,next->priority,next->counter);
    switch_to(next);
}

void Priority_schedule() 
{
    struct task_struct* next = current;
    uint64 max_priority = PRIORITY_MIN-1;
    while (1) 
    {
        for (int i = 1; i < NR_TASKS; i++) 
        {
            if (task[i]->counter == 0) continue;
            if (task[i]->priority > max_priority) 
            {
                max_priority = task[i]->priority;
                next = task[i];
            }
        }

        if (max_priority == PRIORITY_MIN-1) 
        {
            printk("Reset all priority!\n");
            for (int i = 1; i < NR_TASKS; i++) 
            {
                task[i]->counter = rand()%COUNTER_MAX+COUNTER_MIN;
                printk("SET [PID = %d PRIORITY = %d COUNTER = %d]\n", task[i]->pid, task[i]->priority, task[i]->counter);
            }
        }
        else break;
    }
    printk("switch to [PID = %d PRIORITY = %d COUNTER = %d]\n",next->pid,next->priority,next->counter);
    switch_to(next);
}