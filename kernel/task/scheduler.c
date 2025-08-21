#include "scheduler.h"
#include "acpi.h"
#include "alloc.h"
#include "apic.h"
#include "debug.h"
#include "hal_int.h"
#include "page.h"
#include "pcb.h"

int is_scheduler = 0; // 0:disable
                      // 1:enable

list_t *pcb_list = NULL;

void enable_scheduler() { is_scheduler = 1; }

void disable_scheduler() { is_scheduler = 0; }

int get_scheduler() { return is_scheduler; }

int add_task(pcb_t *new_task) {
  if (pcb_list == NULL) {
    pcb_list = (list_t *)malloc(sizeof(list_t));
    pcb_list->data = (void *)new_task;
    pcb_list->pre = pcb_list;
    pcb_list->next = pcb_list;
  } else {
    list_t *p = pcb_list;
    for (p = pcb_list; p->next != pcb_list; p = p->next) {
    }
    p->next = (list_t *)malloc(sizeof(list_t));
    p->next->data = (void *)new_task;
    p->next->next = pcb_list;
    p->next->pre = p;
  }
}

void remove_task(pcb_t *task) {}

/*
int scheduler() {
    if (is_scheduler == 0 && get_current_task()->time != 0) {
        return 1;   // 禁止抢占
    }

    // 正确初始化数据结构
    list_t* level[3] = {NULL};        // 三级队列头指针
    list_t* tail[3] = {NULL};          // 三级队列尾指针
    pcb_t* min_pcb[3] = {NULL};        // 每级最小nice的PCB
    uint64_t min_nice[3] = {100, 100, 100}; // 初始最小nice值

    // 临时保存下一个节点，避免链表破坏
    list_t* next_node = NULL;

    // 安全遍历所有进程
    for (list_t* p = pcb_list; p != NULL; p = next_node) {
        next_node = p->next; // 保存下一个节点

        pcb_t* pcb = (pcb_t*)p->data;
        int L = pcb->level;

        // 只处理有效优先级 (0-2)
        if (L < 0 || L > 2) continue;

        // 插入到对应优先级队列
        if (tail[L] == NULL) {
            // 队列为空
            level[L] = tail[L] = p;
        } else {
            // 添加到队列尾部
            tail[L]->next = p;
            tail[L] = p;
        }
        p->next = NULL; // 新节点作为尾部

        // 更新最小nice值PCB
        if (pcb->nice < min_nice[L]) {
            min_nice[L] = pcb->nice;
            min_pcb[L] = pcb;
        }
    }

    // 选择要运行的进程
    pcb_t* next_pcb = NULL;

    // 按优先级顺序选择 (0 > 1 > 2)
    for (int i = 0; i < 3; i++) {
        if (level[i] != NULL && min_pcb[i] != NULL) {
            next_pcb = min_pcb[i];
            break;
        }
    }

    // 如果没有可运行进程，切换到空闲进程
    if (next_pcb == NULL) {
        next_pcb = idle_pcb;
    }

    // 执行上下文切换
    switch_to(&(get_current_task()->context0), &(next_pcb->context0));

    // 此处的return通常不会执行
    return 0;
}
    */

// 全局就绪队列（每个优先级一个）
list_t ready_queues[4];

// 初始化就绪队列
void init_ready_queues() {
  for (int i = 0; i < 4; i++) {
    // 正确初始化头节点
    ready_queues[i].pre = &ready_queues[i];
    ready_queues[i].next = &ready_queues[i]; // 指向自身，表示空队列
    ready_queues[i].data = NULL;

    // 添加调试输出
    printks("Ready queue %d init: head=%p, next=%p\n", i, &ready_queues[i],
            ready_queues[i].next);
  }
}

// 添加任务到就绪队列
void add_task_to_ready_queue(pcb_t *task) {
  if (task == NULL || task->level < 0 || task->level >= 4) {
    printks("Invalid task for ready queue: %p\n", task);
    return;
  }

  int level = task->level;

  // 创建新节点
  list_t *new_node = (list_t *)kmalloc(sizeof(list_t));
  if (!new_node) {
    panic("Failed to allocate ready node");
  }

  // 初始化节点
  new_node->data = task;

  // 添加到队列尾部
  list_t *tail = ready_queues[level].pre;

  new_node->next = &ready_queues[level];
  new_node->pre = tail;

  tail->next = new_node;
  ready_queues[level].pre = new_node;

  task->state = 0;
}

// 从就绪队列获取下一个任务（不移除）
pcb_t *peek_next_ready_task() {
  for (int level = 0; level < 4; level++) {
    list_t *queue = &ready_queues[level];

    // 检查队列是否为空（next 指向自身）
    if (queue->next == queue) {
      printks("Queue %d is empty\n", level);
      continue;
    }

    // 获取第一个节点
    list_t *first = queue->next;

    // 安全检查：确保节点有效
    if (first == NULL || first == queue) {
      printks("Invalid first node in queue %d: %p\n", level, first);
      continue;
    }

    // 返回任务指针
    return (pcb_t *)first->data;
  }

  // 没有就绪任务，返回空闲任务
  return idle_pcb;
}
// 从就绪队列移除任务
void remove_task_from_ready_queue(pcb_t *task) {
  if (task == NULL)
    return;

  // 遍历所有队列
  for (int level = 0; level < 4; level++) {
    list_t *curr = ready_queues[level].next;

    while (curr != &ready_queues[level]) {
      if (curr->data == task) {
        // 从链表中移除
        curr->pre->next = curr->next;
        curr->next->pre = curr->pre;

        // 释放节点
        kfree(curr);

        task->state = 3;
        return;
      }
      curr = curr->next;
    }
  }
}

// __attribute__((interrupt)) void timer_handle(interrupt_frame_t *frame) {
//   static regs_t *tmp;
//   __asm__("cli");
//   save_regs();
//   __asm__ __volatile__("mov %%rsp,%0" : "=r"(tmp)::);
//   if (is_scheduler == 0) {
//     send_eoi();
//     restore_regs();
//     return;
//   }
//   // printkf("frame addr:%p\r\n", frame);
//   current_task->context0.rip = frame->rip;
//   scheduler(frame, tmp);
//   // printkf("%p\n", frame->rip);
//   send_eoi();
//   restore_regs();
//   return;
// }

void timer_handle_c(regs_t *reg) {
  interrupt_frame_t *frame = &((_regs_t*)reg)->auto_regs.frame;
  // printkf("frame:%p\r\n", frame);
  // printkf("frame->rip:%p\r\n", frame->rip);
  // printkf("frame->rsp:%p\r\n", frame->rsp);
  // printkf("frame->ss:%p\r\n", frame->ss);
  // printkf("reg:%p\r\n", reg);
  if (is_scheduler == 0) {
    goto end;
  }
  if (current_task->flag & PCB_FLAGS_SWITCH_TO_USER != 0)
  {
    frame->rip = current_task->context0.rip;
    current_task->flag ^= PCB_FLAGS_SWITCH_TO_USER;
    current_task->flag ^= PCB_FLAGS_KTHREAD;
    frame->cs = 0x20;
    frame->ss = 0x18;
    goto end;
  } else {
    current_task->context0.rip = frame->rip;
  }
  scheduler(frame, reg);
end:
  send_eoi();
  return;
}

__asm__(
  ".globl timer_handle\n\t"
  "timer_handle:\n\t"
  _save_regs_asm_
  "mov %rsp, %rdi\n\t"
  "call timer_handle_c\n\t"
  _restore_regs_asm_
  "sti\n\t"
  "iretq\n\t"
);

//简易轮转调度
int scheduler(interrupt_frame_t *frame, regs_t *regs) {
  static uint32_t count = 0;
  count++;
  // printks("%d",count);
  if (is_scheduler == 0) {
    return 1;
  }

  list_t *next = pcb_list;
  for (; ((pcb_t *)(next->data)) != current_task; next = next->next) {
  }
  next = next->next;
  pcb_t *now = current_task;
  current_task = ((pcb_t *)(next->data));
  // printk("\nscheduling from pid %d to pid %d\n", now->pid,
  //         ((pcb_t *)(next->data))->pid);
  // printk("pid %d context rip:%p\r\n",now->pid);
  // printk("pid %d context rip:%p\r\n",((pcb_t*)(next->data))->pid);
  switch_to(now, current_task, frame, regs);
  return 0;
}

// __attribute__((force_align_arg_pointer))
void switch_to(pcb_t *source, pcb_t *target, interrupt_frame_t *frame,
               regs_t *regs) {
  switch_page_directory(target->page_dir);
  TaskContext *old = &(source->context0), *new = &(target->context0);
  old->r15 = regs->r15;
  old->r14 = regs->r14;
  old->r13 = regs->r13;
  old->r12 = regs->r12;
  old->r11 = regs->r11;
  old->r10 = regs->r10;
  old->r9 = regs->r9;
  old->r8 = regs->r8;
  old->rax = regs->rax;
  old->rbx = regs->rbx;
  old->rcx = regs->rcx;
  old->rdx = regs->rdx;
  old->rbp = regs->rbp;
  old->rsi = regs->rsi;
  old->rdi = regs->rdi;
  old->rsp = frame->rsp;
  // 创建新任务寄存器上下文
  regs_t new_regs = (regs_t){
      // 段寄存器
      .ds = 0x10,
      .es = 0x10,
      .fs = 0x10,
      .gs = 0x10,

      // 通用寄存器
      .rax = new->rax,
      .rbx = new->rbx,
      .rcx = new->rcx,
      .rdx = new->rdx,
      .rbp = new->rbp,
      .rsi = new->rsi,
      .rdi = new->rdi,
      .r8 = new->r8,
      .r9 = new->r9,
      .r10 = new->r10,
      .r11 = new->r11,
      .r12 = new->r12,
      .r13 = new->r13,
      .r14 = new->r14,
      .r15 = new->r15,

      // 中断信息
      .vector = 0,   // 中断向量号
      .err_code = 0, // 错误代码

      // CPU 自动保存部分
      .rip = new->rip, // 指令指针
      .cs = 0x8,       // 内核代码段选择子
      .rflags = new->rflags,
      .rsp = new->rsp, // 栈指针
      .ss = 0x10       // 内核数据段选择子
  };
  if (target->flag & PCB_FLAGS_KTHREAD == 0)
  {
    new_regs.cs = 0x20;
    new_regs.ss = 0x18;
  }
  frame->rip = new_regs.rip;
  frame->cs = new_regs.cs;
  frame->rflags = new_regs.rflags;
  if (frame->cs == 0x8) {
    frame->rsp = new_regs.rsp;
    frame->ss = new_regs.ss;
  }
  // 伪造寄存器上下文
  // CHEAT_REGS((&new_regs));
  regs->r15 = new->r15;
  regs->r14 = new->r14;
  regs->r13 = new->r13;
  regs->r12 = new->r12;
  regs->r11 = new->r11;
  regs->r10 = new->r10;
  regs->r9 = new->r9;
  regs->r8 = new->r8;
  regs->rax = new->rax;
  regs->rbx = new->rbx;
  regs->rcx = new->rcx;
  regs->rdx = new->rdx;
  regs->rbp = new->rbp;
  regs->rsi = new->rsi;
  regs->rdi = new->rdi;
}
