#include "kernel.h"
#include "printk.h"
#include "serial.h"
#include "tkm.h"

typeof(bootloader) bootloader;

kernel_info kinfo = {
    .kernel_name = KERNEL_NAME,
    .version     = KERNEL_VERSION,
};

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
    init_serial();
    printk("[ Kernel ] Boot By : %s\n", bootloader == Limine ? "Limine" : "Unkown");
    printk("[ Kernel ] Kernel: %s (%s)\n", KERNEL_NAME, KERNEL_VERSION);
    printk("[ Kernel ] BUILD_DATE: %s, BUILD_TIME: %s\n", BUILD_DATE, BUILD_TIME);
    printk("[ Kernel ] COMPILER_NAME: %s\n", COMPILER_NAME);
    init_mod();
    for (;;);
}
