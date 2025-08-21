#include "pcb.h"
#include "alloc.h"
#include "cpu.h"
#include "debug.h"
#include "heap.h"
#include "hhdm.h"
#include "klibc.h"
#include "kshell.h"
#include "page.h"
#include "scheduler.h"
#include "timer.h"

uint32_t now_pid = 0;
pcb_t *idle_pcb;
// list_t* current_task_ls;
// #define current_task ((pcb_t*)(current_task_ls->data))
pcb_t *current_task;
pcb_t *init_pcb;

pcb_t *get_current_task() { return current_task; }

_Noreturn void process_exit() {
  uint64_t rax = 0;
  __asm__("movq %%rax,%0" ::"r"(rax) :);
  printks("Kernel Process exit, Code: %d\n", rax);
  // kill_proc(get_current_task());
  infinite_loop;
}

/**
 * 创建一个进程
 * @param 新进程名
 * @return 进程控制块指针
 */
pcb_t *create_process(char *name) {
  pcb_t *new_pgb = (pcb_t *)malloc(sizeof(pcb_t));
  new_pgb->pid = now_pid++;
  strcpy(new_pgb->name, name);
  new_pgb->page_dir = get_kernel_pagedir();
  new_pgb->init_nice = 0;
  new_pgb->level = 2;
  new_pgb->sources = NULL;
  new_pgb->time = 0;
  new_pgb->state = 1;
  new_pgb->father = NULL;
  new_pgb->childs = NULL;
  printks("new process:%d", now_pid - 1);
  return new_pgb;
}

#define C_F_CLONE_ADDRESS ((uint8_t)1 << 0) //克隆/共享虚拟地址空间
#define C_F_CLONE_SOURCES ((uint8_t)1 << 1) //克隆/共享资源

/**
 * 创建一个进程(特有的clone函数)
 * @param 新进程名
 * @param 父进程
 * @param 标志位
 * @return 进程控制块指针
 */
pcb_t *clone(char *name, pcb_t *father, uint8_t flags) {
  pcb_t *new_p = create_process(name);
  new_p->father = father;
  if (flags & C_F_CLONE_ADDRESS) {
    new_p->page_dir = father->page_dir;
  }
  if (flags & C_F_CLONE_SOURCES) {
    new_p->sources = father->sources;
  }
  return new_p;
}

void switch_to_user_mode(void* func) {
  __asm__("cli");
  get_current_task()->context0.rflags = (0 << 12 | 0b10 | 1 << 9);
  current_task->flag ^= PCB_FLAGS_KTHREAD;
  current_task->flag |= PCB_FLAGS_SWITCH_TO_USER;
  current_task->context0.rip = (uint64_t)func;
  __asm__("sti");
  for(;;) ;
}

pcb_t *create_kernel_thread(int (*_start)(void *arg), void *args, char *name) {
  __asm__("cli");
  int s = get_scheduler();
  disable_scheduler();
  pcb_t *new_task = (pcb_t *)calloc(1, KERNEL_ST_SZ);
  // printks("[Kernel_thread]new_task address:\t%p\r\n",new_task);
  // printks("[Kernel_thread]name:\t%s\r\n",name);
  // printks("[Kernel_thread]name address:\t%p\r\n",name);
  // printks("[Kernel_thread]new task name address:\t%p\r\n",new_task->name);
  if (new_task == NULL) {
    panic("No enough Memory\r\n");
  }
  memset(new_task, 0, sizeof(pcb_t));
  new_task->name = (char *)malloc(strlen(name) * sizeof(char));
  new_task->level = 0;
  new_task->time = 100;
  // new_task->cpu_timer  = 0;
  // new_task->mem_usage  = get_all_memusage();
  // new_task->cpu_id     = get_current_cpuid();
  // memcpy(new_task->name, name, strlen(name) + 1);
  // printks("-----Before call strcpy-----");
  // printks("[Kernel_thread]name:\t%s\r\n", name);

  strcpy(new_task->name, name);
  uint64_t *stack_top = (uint64_t *)((uint64_t)new_task + STACK_SIZE);
  *(--stack_top) = (uint64_t)args;
  *(--stack_top) = (uint64_t)process_exit;
  *(--stack_top) = (uint64_t)_start;
  new_task->context0.rflags = 0x202;
  new_task->context0.rip = (uint64_t)_start;
  new_task->context0.rsp =
      (uint64_t)new_task + STACK_SIZE - sizeof(uint64_t) * 3; // 设置上下文
  new_task->context0.rdi = (uint64_t)args;
  new_task->kernel_stack = (new_task->context0.rsp &= ~0xF); // 栈16字节对齐
  new_task->user_stack =
      new_task->kernel_stack; // 内核级线程没有用户态的部分,
                              // 所以用户栈句柄与内核栈句柄统一
  new_task->pid = now_pid++;
  new_task->page_dir = get_kernel_pagedir();
  new_task->cr3 = read_cr3();
  new_task->flag = 0 | PCB_FLAGS_KTHREAD;
  add_task(new_task);
  // printks("create kernel thread:%d\n", new_task->pid);
  if (s == 1) {
    enable_scheduler();
  }

  __asm__("sti");
  return new_task;
}

int idle_thread() {} //其实压根不会用到

int init_user_main() {
  printkf("Hello World!\r\n");
  for (;;) {
  }
}

int init_kmain(int *test) {
  printk("[INFO]Init process is running. test=%d\n", *test);
  enable_scheduler();
  int a = 0;
  // 创建内核线程kshell
  // create_kernel_thread(kshell, NULL, "Kernel shell");
  // 加载init进程到内存中
  // page_map_to(get_current_directory, 0x405840, virt_to_phys(&init_user_main),
  //             PTE_USER || PTE_PRESENT || PTE_WRITEABLE);
  switch_to_user_mode(init_user_main);
  // TODO
  // 跳转到init进程
  // switch_to_user_mode(init_user_main);
  enable_intr();
  while (1) {
    // __asm__("int $0x40");
    // plogk("schedule to init,a=%d\r\n", a);
    msleep(10);
    // __asm__("hlt");
  }
  return 0; // nerver get
}

pcb_t *init_task() {
  idle_pcb = create_kernel_thread(idle_thread, NULL, "System(idle)");
  idle_pcb->level = 3;
  int *p = (int*)malloc(sizeof(int));
  *p=114514;
  init_pcb = create_kernel_thread(init_kmain, p, "init");
  current_task = idle_pcb;
  printks("idle stack: %p\tinit stack:%p\n\t", idle_pcb->context0.rsp,
          init_pcb->context0.rsp);
  return init_pcb;
}