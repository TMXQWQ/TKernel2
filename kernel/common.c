/*
 *
 *      common.c
 *      Common device
 *
 *      2024/6/27 By Rainy101112
 *      Based on Apache 2.0 open source license.
 *      Copyright © 2020 ViudiraTech, based on the Apache 2.0 license.
 *
 */

#include "common.h"
#include "stddef.h"
#include "stdint.h"

#ifdef __x86_64__ 

/* Port write (8 bits) */
void outb(uint16_t port, uint8_t value)
{
    __asm__ volatile("outb %1, %0" ::"dN"(port), "a"(value));
}

/* Port read (8 bits) */
uint8_t inb(uint16_t port)
{
    uint8_t ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "dN"(port));
    return ret;
}

/* Port write (16 bits) */
void outw(uint16_t port, uint16_t value)
{
    __asm__ volatile("outw %1, %0" ::"dN"(port), "a"(value));
}

/* Port read (16 bits) */
uint16_t inw(uint16_t port)
{
    uint16_t ret;
    __asm__ volatile("inw %1, %0" : "=a"(ret) : "dN"(port));
    return ret;
}

/* Port write (32 bits) */
void outl(uint16_t port, uint32_t value)
{
    __asm__ volatile("outl %1, %0" ::"dN"(port), "a"(value));
}

/* Port read (32 bits) */
uint32_t inl(uint16_t port)
{
    uint32_t ret;
    __asm__ volatile("inl %1, %0" : "=a"(ret) : "dN"(port));
    return ret;
}

/* Read data from I/O port to memory in batches (16 bits) */
void insw(uint16_t port, void *buf, size_t n)
{
    __asm__ volatile("cld; rep; insw" : "+D"(buf), "+c"(n) : "d"(port));
}

/* Write data from memory to I/O port in batches (16 bits) */
void outsw(uint16_t port, const void *buf, size_t n)
{
    __asm__ volatile("cld; rep; outsw" : "+S"(buf), "+c"(n) : "d"(port));
}

/* Read data from I/O port to memory in batches (32 bits) */
void insl(uint32_t port, void *addr, size_t cnt)
{
    __asm__ volatile("cld; repne; insl;" : "=D"(addr), "=c"(cnt) : "d"(port), "0"(addr), "1"(cnt) : "memory", "cc");
}

/* Write data from memory to I/O port in batches (32 bits) */
void outsl(uint32_t port, const void *addr, size_t cnt)
{
    __asm__ volatile("cld; repne; outsl;" : "=S"(addr), "=c"(cnt) : "d"(port), "0"(addr), "1"(cnt) : "memory", "cc");
}

/* Flushes the TLB of the specified address */
void flush_tlb(uint64_t addr)
{
    __asm__ volatile("invlpg (%0)" ::"r"(addr) : "memory");
}

/* Get the current value of the CR3 register */
uint64_t get_cr3(void)
{
    uint64_t cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
    return cr3;
}

/* Get the current value of the RSP register */
uint64_t get_rsp(void)
{
    uint64_t rsp;
    __asm__ volatile("mov %%rsp, %0" : "=r"(rsp));
    return rsp;
}

/* Get the current value of the status flag register */
uint64_t get_rflags(void)
{
    uint64_t rflags;
    __asm__ volatile("pushfq; pop %0" : "=r"(rflags)::"memory");
    return rflags;
}

/* Write a 32-bit data to the specified memory address */
void mmio_write32(uint32_t *addr, uint32_t data)
{
    *(volatile uint32_t *)addr = data;
}

/* Write a 64-bit data to the specified memory address */
void mmio_write64(void *addr, uint64_t data)
{
    *(volatile uint64_t *)addr = data;
}

/* Read a 32-bit data from the specified memory address */
uint32_t mmio_read32(void *addr)
{
    return *(volatile uint32_t *)addr;
}

/* Read a 64-bit data from the specified memory address */
uint64_t mmio_read64(void *addr)
{
    return *(volatile uint64_t *)addr;
}

/* Read msr register */
uint64_t rdmsr(uint32_t msr)
{
    uint32_t rax, rdx;
    __asm__ volatile("rdmsr" : "=a"(rax), "=d"(rdx) : "c"(msr));
    return ((uint64_t)rdx << 32) | rax;
}

/* Write to msr register */
void wrmsr(uint32_t msr, uint64_t value)
{
    uint32_t rax = (uint32_t)value;
    uint32_t rdx = value >> 32;
    __asm__ volatile("wrmsr" ::"c"(msr), "a"(rax), "d"(rdx));
}

/* Loading data atomically */
uint64_t load(uint64_t *addr)
{
    uint64_t ret = 0;
    __asm__ volatile("lock xadd %[ret], %[addr];" : [addr] "+m"(*addr), [ret] "+r"(ret)::"memory");
    return ret;
}

/* Storing data atomically */
void store(uint64_t *addr, uint32_t value)
{
    __asm__ volatile("lock xchg %[value], %[addr];" : [addr] "+m"(*addr), [value] "+r"(value)::"memory");
}

/* Basic rdtsc reading */
uint64_t rdtsc(void)
{
    uint32_t lo, hi;
    __asm__ volatile("rdtsc" : "=a"(lo), "=d"(hi) : : "memory");
    return ((uint64_t)hi << 32) | lo;
}

/* Serialized rdtsc reads */
uint64_t rdtsc_serialized(void)
{
    uint32_t lo, hi;
    __asm__ volatile("mfence\n\t"
                     "rdtsc\n\t"
                     "lfence"
                     : "=a"(lo), "=d"(hi)
                     :
                     : "memory");
    return ((uint64_t)hi << 32) | lo;
}

/* Basic rdtscp reading */
uint64_t rdtscp(uint32_t *aux)
{
    uint32_t lo, hi;
    __asm__ volatile("rdtscp" : "=a"(lo), "=d"(hi), "=c"(*aux) : : "memory");
    return ((uint64_t)hi << 32) | lo;
}

/* Serialized rdtscp reads */
uint64_t rdtscp_serialized(uint32_t *aux)
{
    uint32_t lo, hi;
    __asm__ volatile("mfence\n\t"
                     "rdtscp\n\t"
                     "lfence"
                     : "=a"(lo), "=d"(hi), "=c"(*aux)
                     :
                     : "memory");
    return ((uint64_t)hi << 32) | lo;
}

/* Enable interrupt */
void enable_intr(void)
{
    __asm__ volatile("sti");
}

/* Disable interrupts */
void disable_intr(void)
{
    __asm__ volatile("cli" ::: "memory");
}

/* Kernel halt */
void krn_halt(void)
{
    disable_intr();
    while (1) __asm__ volatile("hlt");
}

/* Compiler barrier */
__attribute__((noinline)) void compiler_barrier(void)
{
    __asm__ volatile("" ::: "memory");
}

#endif

#ifdef __aarch64__

#define outb(port, value)      __compiletime_error("ARM: use mmio_write8(addr, value)")
#define inb(port)             __compiletime_error("ARM: use mmio_read8(addr)")
#define outw(port, value)     __compiletime_error("ARM: use mmio_write16(addr, value)")
#define inw(port)            __compiletime_error("ARM: use mmio_read16(addr)")
#define outl(port, value)     __compiletime_error("ARM: use mmio_write32(addr, value)")
#define inl(port)            __compiletime_error("ARM: use mmio_read32(addr)")
#define insw(port, buf, n)   __compiletime_error("ARM: use mmio_reads(addr, buf, count)")
#define outsw(port, buf, n)  __compiletime_error("ARM: use mmio_writes(addr, buf, count)")
#define insl(port, addr, cnt) __compiletime_error("ARM: use mmio_readl(addr, buf, count)")
#define outsl(port, addr, cnt) __compiletime_error("ARM: use mmio_writel(addr, buf, count)")

/* Write a 32-bit data to the specified memory address */
void mmio_write32(uint32_t *addr, uint32_t data)
{
    *(volatile uint32_t *)addr = data;
}

/* Write a 64-bit data to the specified memory address */
void mmio_write64(void *addr, uint64_t data)
{
    *(volatile uint64_t *)addr = data;
}

/* Read a 32-bit data from the specified memory address */
uint32_t mmio_read32(void *addr)
{
    return *(volatile uint32_t *)addr;
}

/* Read a 64-bit data from the specified memory address */
uint64_t mmio_read64(void *addr)
{
    return *(volatile uint64_t *)addr;
}

// CR3 → TTBR0_EL1 / TTBR1_EL1（页表基址）
inline uint64_t get_ttbr0_el1(void) {
    uint64_t val;
    __asm__ volatile("mrs %0, ttbr0_el1" : "=r"(val));
    return val;
}

inline void flush_tlb(uint64_t addr) {
    // 将虚拟地址写入 TLBI VAAE1IS（按地址，EL1，内部共享）
    __asm__ volatile("tlbi vaae1is, %0" : : "r"(addr) : "memory");
    // 同步上下文
    __asm__ volatile("dsb ish; isb" : : : "memory");
}

inline void flush_tlb_all(void) {
    __asm__ volatile("tlbi vmalle1is; dsb ish; isb" : : : "memory");
}

#define get_cr3() get_ttbr0_el1()

inline uint64_t get_sp_el0(void) {
    uint64_t sp;
    __asm__ volatile("mov %0, sp" : "=r"(sp));  // sp 是 SP_EL0
    return sp;
}
#define get_rsp() get_sp_el0()

inline void enable_intr(void) {
    // 清除 I 位（IRQ）和 F 位（FIQ）
    __asm__ volatile("msr daifclr, #2" : : : "memory"); // 2 = 清除 I (IRQ)
    // 如果需要同时开启 FIQ：msr daifclr, #3
}
inline void disable_intr(void) {
    // 设置 I 位（IRQ）
    __asm__ volatile("msr daifset, #2" : : : "memory");
}
inline void krn_halt(void) {
    disable_intr();
    while (1) {
        __asm__ volatile("wfi" : : : "memory"); // 等待中断，但中断已被禁用，等效于暂停
    }
}
// 数据内存屏障（DMB）
inline void dmb_sy(void) {
    __asm__ volatile("dmb sy" : : : "memory");
}
// 数据同步屏障（DSB）
inline void dsb_sy(void) {
    __asm__ volatile("dsb sy" : : : "memory");
}
// 指令同步屏障（ISB）
inline void isb_sy(void) {
    __asm__ volatile("isb" : : : "memory");
}

/* Compiler barrier */
__attribute__((noinline)) void compiler_barrier(void)
{
    __asm__ volatile("" ::: "memory");
}

#endif
