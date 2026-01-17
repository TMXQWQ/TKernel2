#include "cpio.h"
#include "limine_module.h"

void executable_entry(void)
{
    const char msg[] = "Logically you should use Limine to boot it instead of executing it directly, right?\n\n";
    for (;;);
    __asm__ volatile("mov $1, %%rax\n"
                     "mov $1, %%rdi\n"
                     "lea %[msg], %%rsi\n"
                     "mov %[len], %%rdx\n"
                     "syscall\n"
                     "mov $60, %%rax\n"
                     "mov $1, %%rdi\n"
                     "syscall\n"
                     :
                     : [msg] "m"(msg), [len] "r"(sizeof msg - 1)
                     : "rax", "rdi", "rsi", "rdx");
}

void kernel_entry(void)
{
    lmodule_init();
    lmodule_t *mod = get_lmodule("initrd");
    cpio_parse((newc_header *)mod->data);
    for (;;);
}