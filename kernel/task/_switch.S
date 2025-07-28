.intel_syntax noprefix
.code64
.section .text
.global save_registers
.global __switch_to
.extern timer_handle

save_registers:
    CLI
    PUSH 0 //对齐
    PUSH 0 //对齐
    PUSH R15
    PUSH R14
    PUSH R13
    PUSH R12
    PUSH R11
    PUSH R10
    PUSH R9
    PUSH R8
    PUSH RDI
    PUSH RSI
    PUSH RBP
    PUSH RDX
    PUSH RCX
    PUSH RBX
    PUSH RAX
    MOV  RAX, GS
    PUSH RAX
    MOV  RAX, FS
    PUSH RAX
    MOV  RAX, ES
    PUSH RAX
    MOV  RAX, DS
    PUSH RAX
    MOV  RDI, RSP
    CALL timer_handle
    MOV RSP, RAX
    POP RAX
    MOV DS, RAX
    POP RAX
    MOV ES, RAX
    POP RAX
    MOV FS, RAX
    POP RAX
    MOV GS, RAX
    POP RAX
    POP RBX
    POP RCX
    POP RDX
    POP RBP
    POP RSI
    POP RDI
    POP R8
    POP R9
    POP R10
    POP R11
    POP R12
    POP R13
    POP R14
    POP R15
    ADD RSP, 16 //越过对齐
    STI
    IRETQ

// void __switch_to(TaskContext* old, TaskContext* new)
// old: rdi, new: rsi
__switch_to:
    // 保存旧任务上下文
    mov [rdi+0x00], r15
    mov [rdi+0x08], r14
    mov [rdi+0x10], r13
    mov [rdi+0x18], r12
    mov [rdi+0x20], r11
    mov [rdi+0x28], r10
    mov [rdi+0x30], r9
    mov [rdi+0x38], r8
    mov [rdi+0x40], rbx
    mov [rdi+0x48], rbp
    mov [rdi+0x50], rip
    mov [rdi+0x58], rsp
    mov [rdi+0x60], rflags
    mov [rdi+0x68], rax
    mov [rdi+0x70], rcx
    mov [rdi+0x78], rdx
    mov [rdi+0x80], rdi
    mov [rdi+0x88], rsi

    // 恢复新任务上下文
    mov r15, [rsi+0x00]
    mov r14, [rsi+0x08]
    mov r13, [rsi+0x10]
    mov r12, [rsi+0x18]
    mov r11, [rsi+0x20]
    mov r10, [rsi+0x28]
    mov r9,  [rsi+0x30]
    mov r8,  [rsi+0x38]
    mov rbx, [rsi+0x40]
    mov rbp, [rsi+0x48]
    mov rax, [rsi+0x68]
    mov rcx, [rsi+0x70]
    mov rdx, [rsi+0x78]
    mov rdi, [rsi+0x80]
    mov rsi, [rsi+0x88]
    mov rsp, [rsi+0x58]
    push [rsi+0x60]   // rflags
    push [rsi+0x50]   // rip
    retfq
