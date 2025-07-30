#ifndef TERMINAL_EMBEDDED_FONT
#define TERMINAL_EMBEDDED_FONT
#endif
#include "ctype.h"
#include "description_table.h"
#include "hal.h"
#include "kernel.h"
#include "os_terminal.h"
#include <stdint.h>
#include <string.h>
// #include "alloc.h"
#include "bitmap.h"
#include "debug.h"
#include "frame.h"
#include "heap.h"
#include "hhdm.h"
#include "page.h"
#include "pcb.h"
#include "scheduler.h"
#include "switch.h"
#include "terminal.h"
#include "vfs.h"
#include "apic.h"

int kernel_main() {
  __asm__("cli");
  // sse_init();
  gdt_setup();
  idt_init();
  init_frame(); // 初始化内存帧
  page_init();  // 初始化内存页
  // init_hhdm();			// 初始化高半区内存映射
  init_heap(); // 初始化内存堆
  // init_terminal();
  vfs_init();
  vfs_inode_t *i = vfs_open("/");
  printks("inode\"/\"\tid:%d\tname:%s\n", i->id, i->name);
  i = vfs_creat("/test.txt", VFS_FILE);
  printks("created file:%s\n", i->name);
  char buf[256] = "12345";
  char read_buf[256] = {0};
  vfs_write(i, buf, 6, 0);
  vfs_read(i, read_buf, 6, 0);
  printks("vfs_read test:%s\n", read_buf);
  vfs_printk_ls(&root);
  printks("\n[KERNEL]\tMem Inited!\n");
  init_task();
  printks("\n[KERNEL]\tTask Inited\n");
  current_task = idle_pcb;
  enable_scheduler();
  __asm__("sti");
  __asm__("int $0x40");
  // switch_to(&(init_pcb->context0),&(idle_pcb->context0));
  while (1) {
    asm volatile("hlt");
    // __asm__("int $0x40");
    // for(int i=0;i<=114514;i++) __asm__ ("nop");
    // printks("scheduling...");
    // scheduler();
  }
  printks("\n[ERROR]kernel is going to end!!!\n");
  panic("[ERROR]Idle has been ended");
}
