#pragma once

#include <stdint.h>

//============== 控制寄存器操作 ==============//
uintptr_t read_cr0(void);
uintptr_t read_cr2(void);
uintptr_t read_cr3(void);
uintptr_t read_cr4(void);
void write_cr0(uintptr_t value);
void write_cr3(uintptr_t value);
void write_cr4(uintptr_t value);

//============== 中断控制 ==============//
void cli(void);
void sti(void);
void hlt(void);

//============== 端口IO ==============//
uint8_t inb(uint16_t port);
void outb(uint16_t port, uint8_t value);
uint16_t inw(uint16_t port);
void outw(uint16_t port, uint16_t value);
uint32_t ind(uint16_t port);
void outd(uint16_t port, uint32_t value);

//============== 特殊寄存器 ==============//
uint64_t rdtsc(void);
uint64_t rdmsr(uint32_t msr);
void wrmsr(uint32_t msr, uint64_t value);

//============== 内存屏障 ==============//
void mfence(void);
void sfence(void);
void lfence(void);

//============== 上下文切换 ==============//
void switch_context(uint64_t* old_rsp, uint64_t new_rsp);

