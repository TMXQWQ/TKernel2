#ifndef TERMINAL_EMBEDDED_FONT
#define TERMINAL_EMBEDDED_FONT
#endif
#include "ctype.h"
#include "description_table.h"
#include "hal.h"
#include "os_terminal.h"
#include <stdint.h>
#include <string.h>
// #include "alloc.h"
#include "apic.h"
#include "bitmap.h"
#include "cpio.h"
#include "debug.h"
#include "frame.h"
#include "heap.h"
#include "hhdm.h"
#include "page.h"
#include "pcb.h"
#include "scheduler.h"
#include "switch.h"
#include "terminal.h"
#include "timer.h"
#include "tty.h"
#include "utils.h"
#include "vfs.h"
#include "video.h"

int kernel_main() {
  __asm__("cli");
  {
    plogk("response of limine:\n");
    plogk("\trsdp_request:%p\n", rsdp_request.response);
    plogk("\tkernel_file_request:%p\n", kernel_file_request.response);
    plogk("\tsmp_request:%p\n", smp_request.response);
    plogk("\tframebuffer_request:%p\n", framebuffer_request.response);
    plogk("\tsmbios_request:%p\n", smbios_request.response);
    plogk("\tmemmap_request:%p\n", memmap_request.response);
    plogk("\thhdm_request:%p\n", hhdm_request.response);
    plogk("\tkernel_address_request:%p\n", kernel_address_request.response);
    plogk("\tentry_point_request:%p\n", entry_point_request.response);
  }
  // sse_init();
  gdt_setup();
  idt_init();
  page_init(); // 初始化内存页
  // init_hhdm();			// 初始化高半区内存映射
  init_heap(); // 初始化内存堆
  // init_terminal();
  init_frame(); // 初始化内存帧
  printks("\n[KERNEL]\tMem Inited!\n");
  acpi_init();
  vfs_init();
  // vfs_inode_t *i = vfs_open("/");
  // printks("inode\"/\"\tid:%d\tname:%s\n", i->id, i->name);
  // i = vfs_creat("/test.txt", VFS_FILE);
  // printks("created file:%s\n", i->name);
  // char buf[256] = "12345";
  // char read_buf[256] = {0};
  // vfs_write(i, buf, 6, 0);
  // vfs_read(i, read_buf, 6, 0);
  // printks("vfs_read test:%s\n", read_buf);
  // vfs_printk_ls(&root);
  driver *d = malloc(sizeof(driver));
  // tty_write(tty_ioctl(tty_open(d, 0), 0, "set", ), NULL, "hello world\n",
  //           strlen("hello world\n"), 0);
  video_init();
  disable_scheduler();
  // plogk("[Initrd]%s:%p\n", module_request.response->modules[0]->path,
  // module_request.response->modules[0]->address); printk("\n");
  init_task();
  printks("\n[KERNEL]\tTask Inited\n");
  current_task = idle_pcb;
  enable_scheduler();
  __asm__("sti");
  __asm__("int $0x20");
  // switch_to(&(init_pcb->context0),&(idle_pcb->context0));
  while (1) {
    asm volatile("hlt");
    // __asm__("int $0x40");
    // for(int i=0;i<=114514;i++) __asm__ ("nop");
    // printks("schedule to idle\r\n");
    // msleep(10);
    // scheduler();
  }
  printks("\n[ERROR]kernel is going to end!!!\n");
  panic("[ERROR]Idle has been ended");
}
