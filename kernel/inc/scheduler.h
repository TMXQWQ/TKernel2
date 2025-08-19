#include "pcb.h"
#include "hal_int.h"


int add_task(pcb_t* new_task);

void remove_task(pcb_t* task);

void enable_scheduler();

void disable_scheduler();

int get_scheduler();

int scheduler(interrupt_frame_t *frame, regs_t* regs);

__attribute__((interrupt))
void timer_handle(interrupt_frame_t *frame);

// 切换上下文：保存 old，恢复 new
void switch_to(pcb_t* source, pcb_t* target,interrupt_frame_t *frame, regs_t* regs);


// 伪造中断现场 - 修正顺序
#define cheat_from_intr(ret_addr, cs, rsp) __asm__ __volatile__( \
    "mov %%ss,%%ax\n\t"  \
    "pushq %%rax\n\t"  \
    "pushq %2\n\t"  \
    "pushfq\n\t"    \
    "pushq %1\n\t"  \
    "pushq %0\n\t"  \
    ::"r"(ret_addr),"r"((uint64_t)(cs)), "r"(rsp):"rax" \
)

// 伪造中断现场
// #define _cheat_from_intr() __asm__ __volatile__( \
//     "pushq %%rsp\n\t"   \
//     "pushfq\n\t"    \
//     "mov %%cs, %%ax\n\t"  \
//     "pushq %%rax\n\t"  \
//     "pushq 1f\n\t"  \
//     "1:"  \
//     :::"rax"\
// )

// 伪造中断返回帧
// #define CHEAT_INTR_FRAME(rip_val, cs_val, rflags_val, rsp_val, ss_val) do { \
//     __asm__ __volatile__("pushq %0" :: "r"(rflags_val)); /* RFLAGS */ \
//     __asm__ __volatile__("pushq %0" :: "r"(cs_val));     /* CS */ \
//     __asm__ __volatile__("pushq %0" :: "r"(rip_val));    /* RIP */ \
// } while (0)

// 伪造寄存器上下文
#define CHEAT_REGS(regs_ptr) do { \
    __asm__ __volatile__(\
        "sub $0x10,%%rsp\n\t" \
        "pushq %0\n\t" \
        "pushq %1\n\t" \
        "pushq %2\n\t" \
        "pushq %3\n\t" \
        "pushq %4\n\t" \
        "pushq %5\n\t" \
        "pushq %6\n\t"  \
        "pushq %7\n\t"  \
        "pushq %8\n\t" \
        "pushq %9\n\t" \
        "pushq %10\n\t" \
        "pushq %11\n\t" \
        "pushq %12\n\t" \
        "pushq %13\n\t" \
        "pushq %14\n\t" \
        "mov %%gs,%%ax\n\t"\
        "pushq %%rax\n\t" \
        "mov %%fs,%%ax\n\t"\
        "pushq %%rax\n\t" \
        "mov %%es,%%ax\n\t"\
        "pushq %%rax\n\t" \
        "mov %%ds,%%ax\n\t"\
        "pushq %%rax\n\t" \
    ::"g"(regs_ptr->r15), "g"(regs_ptr->r14), "g"(regs_ptr->r13), "g"(regs_ptr->r12), \
    "g"(regs_ptr->r11), "r"(regs_ptr->r10), "r"(regs_ptr->r9), "r"(regs_ptr->r8), \
    "r"(regs_ptr->rdi), "r"(regs_ptr->rsi), "r"(regs_ptr->rbp), "r"(regs_ptr->rdx), \
    "r"(regs_ptr->rcx), "r"(regs_ptr->rbx), "r"(regs_ptr->rax) :"rax"\
);  \
} while (0)

// 保存寄存器上下文
#define save_regs() do { \
    __asm__ __volatile__(\
        "sub $0x10,%rsp\n\t" \
        "pushq %r15\n\t" \
        "pushq %r14\n\t" \
        "pushq %r13\n\t" \
        "pushq %r12\n\t" \
        "pushq %r11\n\t" \
        "pushq %r10\n\t" \
        "pushq %r9\n\t"  \
        "pushq %r8\n\t"  \
        "pushq %rdi\n\t" \
        "pushq %rsi\n\t" \
        "pushq %rbp\n\t" \
        "pushq %rdx\n\t" \
        "pushq %rcx\n\t" \
        "pushq %rbx\n\t" \
        "pushq %rax\n\t" \
        "mov %gs,%ax\n\t"\
        "pushq %rax\n\t" \
        "mov %fs,%ax\n\t"\
        "pushq %rax\n\t" \
        "mov %es,%ax\n\t"\
        "pushq %rax\n\t" \
        "mov %ds,%ax\n\t"\
        "pushq %rax\n\t" \
    );    \
} while (0)

// 恢复寄存器
#define restore_regs() __asm__ __volatile__( \
    /* 恢复段寄存器 */ \
    "popq %rax\n\t"         \
    "mov %ax, %ds\n\t"     /* 恢复 DS */ \
    "popq %rax\n\t"         \
    "mov %ax, %es\n\t"     /* 恢复 ES */ \
    "popq %rax\n\t"         \
    "mov %ax, %fs\n\t"     /* 恢复 FS */ \
    "popq %rax\n\t"         \
    "mov %ax, %gs\n\t"     /* 恢复 GS */ \
    \
    "popq %rax\n\t"         /* 恢复 RAX */ \
    "popq %rbx\n\t"         /* 恢复 RBX */ \
    "popq %rcx\n\t"         /* 恢复 RCX */ \
    "popq %rdx\n\t"         /* 恢复 RDX */ \
    "popq %rbp\n\t"         /* 恢复 RBP */ \
    "popq %rsi\n\t"         /* 恢复 RSI */ \
    "popq %rdi\n\t"         /* 恢复 RDI */ \
    "popq %r8\n\t"          /* 恢复 R8 */ \
    "popq %r9\n\t"          /* 恢复 R9 */ \
    "popq %r10\n\t"         /* 恢复 R10 */ \
    "popq %r11\n\t"         /* 恢复 R11 */ \
    "popq %r12\n\t"         /* 恢复 R12 */ \
    "popq %r13\n\t"         /* 恢复 R13 */ \
    "popq %r14\n\t"         /* 恢复 R14 */ \
    "popq %r15\n\t"         /* 恢复 R15 */ \
    \
    "add $0x10,%rsp\n\t"         /* 跳过error code,v */ \
    )

#define restore_regs_asm  \
    /* 恢复段寄存器 */ \
    "popq %%rax\n\t"         \
    "mov %%ax, %%ds\n\t"     /* 恢复 DS */ \
    "popq %%rax\n\t"         \
    "mov %%ax, %%es\n\t"     /* 恢复 ES */ \
    "popq %%rax\n\t"         \
    "mov %%ax, %%fs\n\t"     /* 恢复 FS */ \
    "popq %%rax\n\t"         \
    "mov %%ax, %%gs\n\t"     /* 恢复 GS */ \
    \
    "popq %%rax\n\t"         /* 恢复 RAX */ \
    "popq %%rbx\n\t"         /* 恢复 RBX */ \
    "popq %%rcx\n\t"         /* 恢复 RCX */ \
    "popq %%rdx\n\t"         /* 恢复 RDX */ \
    "popq %%rbp\n\t"         /* 恢复 RBP */ \
    "popq %%rsi\n\t"         /* 恢复 RSI */ \
    "popq %%rdi\n\t"         /* 恢复 RDI */ \
    "popq %%r8\n\t"          /* 恢复 R8 */ \
    "popq %%r9\n\t"          /* 恢复 R9 */ \
    "popq %%r10\n\t"         /* 恢复 R10 */ \
    "popq %%r11\n\t"         /* 恢复 R11 */ \
    "popq %%r12\n\t"         /* 恢复 R12 */ \
    "popq %%r13\n\t"         /* 恢复 R13 */ \
    "popq %%r14\n\t"         /* 恢复 R14 */ \
    "popq %%r15\n\t"         /* 恢复 R15 */ \
    \
    "add $0x10,%%rsp\n\t"         /* 跳过error code,v */ 


#define ret_from_intr() __asm__ __volatile__( \
    "nop\n\t" \
    "mov -0x8(%rbp),%rax\n\t" \
    "leave\n\t" \
    "sti\n\t" \
    "iretq" \
)

// 伪造的中断返回不需要恢复栈帧
#define cheat_ret_from_intr() __asm__ __volatile__( \
    "sti\n\t" \
    "iretq" \
)

