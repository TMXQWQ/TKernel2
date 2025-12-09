#include "drivers/ports/ttys.h"
#include "drivers/tty/tty.h"
#include "kernel.h"
#include <stdint.h>

void executable_entry(void)
{
    const char msg[] = "Logically you should use Limine to boot it instead of executing it directly, right?\n\n";
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
    // init_parallel();
    // write_parallel(PARALLEL_PORT_1, 'T');
    // write_serial(SERIAL_PORT_1, 'T');
    ttys_info.info.ops.install();
    // stty_info.father.ops.ioctl(STTY_WRITE, 1, 'T');
    // stty_info.father.ops.ioctl(STTY_READ, 1, 0); //clear
    for (;;) ttys_info.info.ops.ioctl(TTY_WRITE, 1, ttys_info.info.ops.ioctl(TTY_READ, 1, 0));
}