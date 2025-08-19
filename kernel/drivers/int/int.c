#include "apic.h"
#include "debug.h"
#include "hal.h"
#include "hal_int.h"
#include "page.h"
#include "scheduler.h"
#include <stdint.h>
#include "idt.h"

__attribute__((interrupt)) void
page_fault_handle(interrupt_frame_t *frame, uint64_t error_code);

void set_intr_gate(unsigned int vector, uint8_t ist, void *handler) {
  _set_gate(&IDT_Table[vector], 0x8E, ist, handler); // 0x8E=中断门，DPL=0
}

/*

*/

inline void set_trap_gate(unsigned int n, unsigned char ist, void *addr) {
  _set_gate(IDT_Table + n, 0x8F, ist, addr); // P,DPL=0,TYPE=F
}

/*

*/

inline void set_system_gate(unsigned int n, unsigned char ist, void *addr) {
  _set_gate(IDT_Table + n, 0xEF, ist, addr); // P,DPL=3,TYPE=F
}

/*

*/

inline void set_system_intr_gate(unsigned int n, unsigned char ist,
                                 void *addr) // int3
{
  _set_gate(IDT_Table + n, 0xEE, ist, addr); // P,DPL=3,TYPE=E
}

/*

*/

void set_tss64(unsigned long rsp0, unsigned long rsp1, unsigned long rsp2,
               unsigned long ist1, unsigned long ist2, unsigned long ist3,
               unsigned long ist4, unsigned long ist5, unsigned long ist6,
               unsigned long ist7) {
  *(unsigned long *)(TSS64_Table + 1) = rsp0;
  *(unsigned long *)(TSS64_Table + 3) = rsp1;
  *(unsigned long *)(TSS64_Table + 5) = rsp2;

  *(unsigned long *)(TSS64_Table + 9) = ist1;
  *(unsigned long *)(TSS64_Table + 11) = ist2;
  *(unsigned long *)(TSS64_Table + 13) = ist3;
  *(unsigned long *)(TSS64_Table + 15) = ist4;
  *(unsigned long *)(TSS64_Table + 17) = ist5;
  *(unsigned long *)(TSS64_Table + 19) = ist6;
  *(unsigned long *)(TSS64_Table + 21) = ist7;
}

__attribute__((interrupt)) void
default_irq_handler(struct interrupt_frame *frame) {
  // uint8_t vector = read_isr(); // 从APIC读取中断向量
  // panic("Unhandled IRQ");
  send_eoi();
  return;
}

__attribute__((interrupt)) void
divide_error_handler(struct interrupt_frame *frame) {
  // io_printf("divide_error_handler");
  panic("#DE error reached!");
}

__attribute__((interrupt)) void
double_fault_handler(struct interrupt_frame *frame) {
  panic("#DF has been reached!");
}

__attribute__((interrupt)) void gp_irq_handler(struct interrupt_frame *frame) {
  panic("#GP has been reached!");
}

// __attribute__((interrupt))
// void page_fault_handler(struct interrupt_frame *frame) {
//     uint64_t faulting_address;
//     uint32_t error_code;
//     __asm__ __volatile__("pop %%rcx" : "=a"(error_code)::"rcx");
//     __asm__ __volatile__("mov %%cr2, %0" : "=r"(faulting_address));  // 读取
//     CR2
//     // while (1)
//     // {
//     //     __asm__("cli");
//     //     __asm__("hlt");
//     // }   //断点

//     // 检查错误类型
//     if (error_code & 0x1) {
//         // 页面存在但权限错误（如写只读页）
//         io_printf("Page Fault: Permission Denied at CR2\n");
//     } else {
//         // 页面不存在（缺页）
//         io_printf("Page Fault: Page Not Present at CR2\n");

//         // 分配物理内存并映射
//         uint64_t physical_address = (uint64_t)page_alloc(1);  //
//         假设已实现物理页分配函数 map_page(faulting_address, physical_address,
//         PTE_WRITEABLE || PTE_USER);   // 映射为可读写页
//     }

//     // 恢复执行
//     return;
// }

void idt_init(void) {
  // set_tss64(
  //     (unsigned long)0xFFFF800000007C00,   // rsp0 (内核栈)
  //     (unsigned long)0,                    // rsp1
  //     (unsigned long)0,                    // rsp2
  //     (unsigned long)0xFFFF800000007C00,   // ist1
  //     (unsigned long)0,                    // ist2
  //     (unsigned long)0,                    // ist3
  //     (unsigned long)0,                    // ist4
  //     (unsigned long)0,                    // ist5
  //     (unsigned long)0,                    // ist6
  //     (unsigned long)0                     // ist7
  // );

  // 2. 设置异常处理（0-31号向量）
  set_intr_gate(0, 0, divide_error_handler); // 除零错误
  set_intr_gate(1, 0, default_irq_handler);  // 调试异常
  set_intr_gate(2, 0, default_irq_handler);  // NMI
  set_intr_gate(0xd, 0, gp_irq_handler);
  set_intr_gate(0xe, 0, page_fault_handle);

  // 3. 设置硬件中断（32-255号向量）
  for (unsigned int i = 32; i < 256; i++) {
    set_intr_gate(i, 0, default_irq_handler); // 默认使用IST0
  }

  set_intr_gate(0, 0, divide_error_handler); // 除零错误 (IST0)
  set_intr_gate(1, 0, default_irq_handler);  // 调试异常
  set_intr_gate(2, 1, default_irq_handler);  // NMI使用IST1
  set_intr_gate(8, 1, double_fault_handler); // 双重错误使用IST1

  // 5. 加载IDT
  struct {
    unsigned short limit;
    unsigned long base;
  } __attribute__((packed)) idtr;

  idtr.limit = sizeof(struct gate_struct) * GDT_IDT_MAX - 1;
  idtr.base = (unsigned long)IDT_Table;

  __asm__ __volatile__("lidt %0" : : "m"(idtr));
}