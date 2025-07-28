#include "ctypes.h"

//============== 控制寄存器操作 ==============//
uintptr_t read_cr0(void) {
  uintptr_t value;
  __asm__ volatile("mov %%cr0, %0" : "=r"(value));
  return value;
}

uintptr_t read_cr2(void) {
  uintptr_t value;
  __asm__ volatile("mov %%cr2, %0" : "=r"(value));
  return value;
}

uintptr_t read_cr3(void) {
  uintptr_t value;
  __asm__ volatile("mov %%cr3, %0" : "=r"(value));
  return value;
}

uintptr_t read_cr4(void) {
  uintptr_t value;
  __asm__ volatile("mov %%cr4, %0" : "=r"(value));
  return value;
}

void write_cr0(uintptr_t value) {
  __asm__ volatile("mov %0, %%cr0" ::"r"(value) : "memory");
}

void write_cr3(uintptr_t value) {
  __asm__ volatile("mov %0, %%cr3" ::"r"(value) : "memory");
}

void write_cr4(uintptr_t value) {
  __asm__ volatile("mov %0, %%cr4" ::"r"(value) : "memory");
}

//============== 中断控制 ==============//
void cli(void) { __asm__ volatile("cli"); }

void sti(void) { __asm__ volatile("sti"); }

void hlt(void) { __asm__ volatile("hlt"); }

//============== 端口IO ==============//
uint8_t inb(uint16_t port) {
  uint8_t ret;
  __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
  return ret;
}

void outb(uint16_t port, uint8_t value) {
  __asm__ volatile("outb %0, %1" ::"a"(value), "Nd"(port));
}

uint16_t inw(uint16_t port) {
  uint16_t ret;
  __asm__ volatile("inw %1, %0" : "=a"(ret) : "Nd"(port));
  return ret;
}

void outw(uint16_t port, uint16_t value) {
  __asm__ volatile("outw %0, %1" ::"a"(value), "Nd"(port));
}

uint32_t ind(uint16_t port) {
  uint32_t ret;
  __asm__ volatile("inl %1, %0" : "=a"(ret) : "Nd"(port));
  return ret;
}

void outd(uint16_t port, uint32_t value) {
  __asm__ volatile("outl %0, %1" ::"a"(value), "Nd"(port));
}

//============== 特殊寄存器 ==============//
uint64_t rdtsc(void) {
  uint32_t low, high;
  __asm__ volatile("rdtsc" : "=a"(low), "=d"(high));
  return ((uint64_t)high << 32) | low;
}

uint64_t rdmsr(uint32_t msr) {
  uint32_t low, high;
  __asm__ volatile("rdmsr" : "=a"(low), "=d"(high) : "c"(msr));
  return ((uint64_t)high << 32) | low;
}

void wrmsr(uint32_t msr, uint64_t value) {
  uint32_t low = value & 0xFFFFFFFF;
  uint32_t high = value >> 32;
  __asm__ volatile("wrmsr" ::"a"(low), "d"(high), "c"(msr));
}

//============== 内存屏障 ==============//
void mfence(void) { __asm__ volatile("mfence" ::: "memory"); }

void sfence(void) { __asm__ volatile("sfence" ::: "memory"); }

void lfence(void) { __asm__ volatile("lfence" ::: "memory"); }

// #define SAVE_REGS \
//     "cli\n"                  // 关闭中断
//     "pushq $0\n"             // 对齐
//     "pushq $0\n"             // 对齐
//     "pushq %r15\n"          \
//     "pushq %r14\n"          \
//     "pushq %r13\n"         \
//     "pushq %r12\n"          \
//     "pushq %r11\n"          \
//     "pushq %r10\n"          \
//     "pushq %r9\n"           \
//     "pushq %r8\n"           \
//     "pushq %rdi\n"          \
//     "pushq %rsi\n"          \
//     "pushq %rbp\n"          \
//     "pushq %rdx\n"          \
//     "pushq %rcx\n"          \
//     "pushq %rbx\n"          \
//     "pushq %rax\n"          \
//     "movq %gs, %rax\n"      \
//     "pushq %rax\n"          \
//     "movq %fs, %rax\n"      \
//     "pushq %rax\n"          \
//     "movq %es, %rax\n"      \
//     "pushq %rax\n"          \
//     "movq %ds, %rax\n"      \
//     "pushq %rax\n"          \
//     "movq %rsp, %rdi\n"      // 将 RSP 作为参数传递给 timer_handle
//     "movq %rax, %rsp\n"      // 恢复 RSP
//     "popq %rax\n"           \
//     "movq %rax, %ds\n"      \
//     "popq %rax\n"           \
//     "movq %rax, %es\n"      \
//     "popq %rax\n"           \
//     "movq %rax, %fs\n"      \
//     "popq %rax\n"           \
//     "movq %rax, %gs\n"      \
//     "popq %rax\n"           \
//     "popq %rbx\n"           \
//     "popq %rcx\n"           \
//     "popq %rdx\n"           \
//     "popq %rbp\n"           \
//     "popq %rsi\n"           \
//     "popq %rdi\n"           \
//     "popq %r8\n"            \
//     "popq %r9\n"            \
//     "popq %r10\n"           \
//     "popq %r11\n"           \
//     "popq %r12\n"           \
//     "popq %r13\n"           \
//     "popq %r14\n"           \
//     "popq %r15\n"           \
//     "addq $16, %rsp\n"       // 越过对齐
//     "sti\n"                  // 打开中断
//     "iretq\n"                // 中断返回

// 内联汇编实现的上下文切换
void switch_context(uint64_t *old_rsp, uint64_t new_rsp) {
  __asm__ volatile("pushq %rbp\n" // 保存当前基址指针
                   "pushq %rbx\n" // 保存当前寄存器
                   "pushq %r12\n"
                   "pushq %r13\n"
                   "pushq %r14\n"
                   "pushq %r15\n"
                   "movq %rsp, (%rdi)\n" // 保存当前栈指针到 old_rsp
                   "movq %rsi, %rsp\n"   // 加载新栈指针
                   "popq %r15\n"         // 恢复新线程的寄存器
                   "popq %r14\n"
                   "popq %r13\n"
                   "popq %r12\n"
                   "popq %rbx\n"
                   "popq %rbp\n"
                   "ret\n" // 返回到新线程的 rip
  );
  //     __asm__ volatile(
  //         SAVE_REGS
  //     )
}

void flush_tlb(uint64_t addr) {
  __asm__ __volatile__("invlpg (%0)" ::"r"(addr) : "memory");
}

void sse_init(void) {
  __asm__ volatile("movq %cr0, %rax\n\t"
                   "and  $0xFFFB, %ax\n\t"
                   "or   $0x2,%ax\n\t"
                   "movq %rax,%cr0\n\t"
                   "movq %cr4,%rax\n\t"
                   "or   $(3<<9),%ax\n\t"
                   "movq %rax,%cr4\n\t");
}
