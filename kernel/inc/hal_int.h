#ifndef HAL_INT_H
#define HAL_INT_H
#define GDT_IDT_MAX 256
#include "idt.h"

struct interrupt_frame {
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
} __attribute__((packed));

struct idt_gate {
    uint16_t offset_low;     // 偏移低16位
    uint16_t selector;       // 代码段选择子
    uint8_t  ist;            // IST 索引（低3位）
    uint8_t  type_attr;      // 类型和属性（P|DPL|TYPE）
    uint16_t offset_mid;     // 偏移中间16位
    uint32_t offset_high;    // 偏移高32位
    uint32_t reserved;       // 保留
} __attribute__((packed));

/*  Int     */
#define Build_IRQ(nr) void IRQ##nr##_interruptI(void);
void init_interrupt();

// void do_IRQ(struct pt_regs * regs,unsigned long nr);
void idt_init(void);
#define SaveReg				\
	"cli;			\n\t"		\
	"pushq	%rax;		\n\t"		\
	"pushq	%rax;		\n\t"		\
	"movq	%es,	%rax;	\n\t"		\
	"pushq	%rax;		\n\t"		\
	"movq	%ds,	%rax;	\n\t"		\
	"pushq	%rax;		\n\t"		\
	"xorq	%rax,	%rax;	\n\t"		\
	"pushq	%rbp;		\n\t"		\
	"pushq	%rdi;		\n\t"		\
	"pushq	%rsi;		\n\t"		\
	"pushq	%rdx;		\n\t"		\
	"pushq	%rcx;		\n\t"		\
	"pushq	%rbx;		\n\t"		\
	"pushq	%r8;		\n\t"		\
	"pushq	%r9;		\n\t"		\
	"pushq	%r10;		\n\t"		\
	"pushq	%r11;		\n\t"		\
	"pushq	%r12;		\n\t"		\
	"pushq	%r13;		\n\t"		\
	"pushq	%r14;		\n\t"		\
	"pushq	%r15;		\n\t"		\
	"movq	$0x10,	%rdx;	\n\t"		\
	"movq	%rdx,	%ds;	\n\t"		\
	"movq	%rdx,	%es;	\n\t"

#define _set_gate(gate_ptr, type, ist, code_addr) \
do { \
    struct idt_gate *gate = (struct idt_gate *)(gate_ptr); \
    uint64_t offset = (uint64_t)(code_addr); \
    gate->offset_low   = (uint16_t)(offset & 0xFFFF); \
    gate->selector     = 0x8;  /* 内核代码段选择子 */ \
    gate->ist          = (ist) & 0x7; \
    gate->type_attr    = (type); \
    gate->offset_mid   = (uint16_t)((offset >> 16) & 0xFFFF); \
    gate->offset_high  = (uint32_t)(offset >> 32); \
    gate->reserved     = 0; \
} while(0)

struct desc_struct 
{
	unsigned char x[8];
};

struct gate_struct
{
	unsigned char x[16];
};
struct desc_struct GDT_Table[GDT_IDT_MAX];
struct gate_struct IDT_Table[GDT_IDT_MAX];
unsigned int TSS64_Table[256] __attribute__((aligned(4096)));;

void set_intr_gate(unsigned int vector, uint8_t ist, void *handler);


#define disable_intr() __asm__("cli")
#define enable_intr() __asm__("sti")

#endif