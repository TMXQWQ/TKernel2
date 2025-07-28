#include "ctype.h"
#include "hal.h"

#define close_interrupt __asm__ volatile("cli" ::: "memory")
#define open_interrupt __asm__ volatile("sti" ::: "memory")

void io_out8(uint16_t port, uint8_t data) {
  __asm__ volatile("outb %b0, %w1" : : "a"(data), "Nd"(port));
}

uint8_t io_in8(uint16_t port) {
  uint8_t data;
  __asm__ volatile("inb %w1, %b0" : "=a"(data) : "Nd"(port));
  return data;
}

uint16_t io_in16(uint16_t port) {
  uint16_t data;
  __asm__ volatile("inw %w1, %w0" : "=a"(data) : "Nd"(port));
  return data;
}

void io_out16(uint16_t port, uint16_t data) {
  __asm__ volatile("outw %w0, %w1" : : "a"(data), "Nd"(port));
}

uint32_t io_in32(uint16_t port) {
  uint32_t data;
  __asm__ volatile("inl %1, %0" : "=a"(data) : "Nd"(port));
  return data;
}

void io_out32(uint16_t port, uint32_t data) {
  __asm__ volatile("outl %0, %1" : : "a"(data), "Nd"(port));
}

void flush_tlb(uint64_t addr) {
  __asm__ volatile("invlpg (%0)" ::"r"(addr) : "memory");
}

uint64_t get_cr0(void) {
  uint64_t cr0;
  __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
  return cr0;
}

void set_cr0(uint64_t cr0) { __asm__ volatile("mov %0, %%cr0" : : "r"(cr0)); }

uint64_t get_cr3(void) {
  uint64_t cr0;
  __asm__ volatile("mov %%cr3, %0" : "=r"(cr0));
  return cr0;
}

uint64_t get_rsp(void) {
  uint64_t rsp;
  __asm__ volatile("mov %%rsp, %0" : "=r"(rsp));
  return rsp;
}

uint64_t get_rflags() {
  uint64_t rflags;
  __asm__ volatile("pushfq\n"
                   "pop %0\n"
                   : "=r"(rflags)
                   :
                   : "memory");
  return rflags;
}

void insl(uint32_t port, uint32_t *addr, int cnt) {
  __asm__ volatile("cld\n\t"
                   "repne\n\t"
                   "insl\n\t"
                   : "=D"(addr), "=c"(cnt)
                   : "d"(port), "0"(addr), "1"(cnt)
                   : "memory", "cc");
}

void mmio_write32(uint32_t *addr, uint32_t data) {
  *(volatile uint32_t *)addr = data;
}

uint32_t mmio_read32(void *addr) { return *(volatile uint32_t *)addr; }

uint64_t mmio_read64(void *addr) { return *(volatile uint64_t *)addr; }

void mmio_write64(void *addr, uint64_t data) {
  *(volatile uint64_t *)addr = data;
}

uint64_t rdmsr(uint32_t msr) {
  uint32_t eax, edx;
  __asm__ volatile("rdmsr" : "=a"(eax), "=d"(edx) : "c"(msr));
  return ((uint64_t)edx << 32) | eax;
}

void wrmsr(uint32_t msr, uint64_t value) {
  uint32_t eax = (uint32_t)value;
  uint32_t edx = value >> 32;
  __asm__ volatile("wrmsr" : : "c"(msr), "a"(eax), "d"(edx));
}

uint64_t load(uint64_t *addr) {
  uint64_t ret = 0;
  __asm__ volatile("lock xadd %[ret], %[addr];"
                   : [addr] "+m"(*addr), [ret] "+r"(ret)
                   :
                   : "memory");
  return ret;
}

void store(uint64_t *addr, uint32_t value) {
  __asm__ volatile("lock xchg %[value], %[addr];"
                   : [addr] "+m"(*addr), [value] "+r"(value)
                   :
                   : "memory");
}

// uint8_t io_in8(uint16_t port){
//     uint8_t data;
//     asm volatile ("inb %1, %0" : "=a" (data) : "Nd" (port));
//     return data;
// }

// uint16_t io_in16(uint16_t port){
//     uint16_t data;
//     asm volatile ("inw %1, %0" : "=a" (data) : "Nd" (port));
//     return data;
// }

// uint32_t io_in32(uint16_t port){
//     uint32_t data;
//     asm volatile ("inl %1, %0" : "=a" (data) : "Nd" (port));
//     return data;
// }

// void io_out8(uint16_t port, uint8_t msg){
//     asm volatile ("outb %0, %1" : : "a" (msg), "Nd" (port));
// }

// void io_out16(uint16_t port, uint16_t msg){
//     asm volatile ("outw %0, %1" : : "a" (msg), "Nd" (port));
// }

// void io_out32(uint16_t port, uint32_t msg){
//     asm volatile ("outl %0, %1" : : "a" (msg), "Nd" (port));
// }

void io_printf(const char *string) {
  if (io_printf_mode == 0) {
    for (int32_t i = 0; string[i] != '\0' && i <= MAX_STRING; i++) {
      putc_serial(string[i]);
    }
  }
}

int32_t io_scanf(int32_t max_size, char *str) {
  if (io_scanf_mode == 0) {
    for (int32_t i = 0; i < max_size - 1; i++) {
      str[i] = gets_serial();
      if (str[i] == '\n')
        break;
    }
    str[max_size - 1] = '\0';
  }
}

void io_hlt() { __asm__ __volatile__("hlt"); }

/*  串口    */
void init_serial() {
  // 初始化串口，设置波特率
  io_out8(0x3F8 + 1, 0x00);
  io_out8(0x3F8 + 3, 0x80);
  io_out8(0x3F8 + 0, 0x03);
  io_out8(0x3F8 + 1, 0x00);
  io_out8(0x3F8 + 3, 0x03);
  io_out8(0x3F8 + 2, 0xC7);
  io_out8(0x3F8 + 4, 0x0B);
}
void putc_serial(char c) {
  // 等待串口
  while ((io_in8(0x3F8 + 5) & 0x20) == 0)
    ;
  // 发送字符到串口
  io_out8(0x3F8, c);
}

char gets_serial() {
  uint8_t result; // 定义 result 变量
  __asm__ __volatile__("check_rx_buffer:        \n\t"
                       "movw $0x3FD, %%dx       \n\t" // 串口状态端口
                       "inb %%dx, %%al          \n\t" // 读取状态
                       "testb $0x01, %%al       \n\t" // 检查数据是否就绪
                       "jz check_rx_buffer      \n\t" // 未就绪则循环等待
                       "movw $0x3F8, %%dx       \n\t" // 串口数据端口
                       "inb %%dx, %%al          \n\t" // 读取数据到 AL
                       : "=a"(result)   // 输出：AL 寄存器 → result
                       :                // 无输入操作数
                       : "dx", "memory" // 破坏列表：DX 寄存器和内存
  );
  return result;
}
