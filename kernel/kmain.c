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
    for (;;);
}

void kernel_entry(void)
{
    init_serial();
    printk(ansi_V("[ Kernel ]") " Boot By : %s\n", bootloader == Limine ? "Limine" : "Unkown");
    printk(ansi_V("[ Kernel ]") " Kernel: %s (%s)\n", KERNEL_NAME, KERNEL_VERSION);
    printk(ansi_V("[ Kernel ]") " BUILD_DATE: %s, BUILD_TIME: %s\n", BUILD_DATE, BUILD_TIME);
    printk(ansi_V("[ Kernel ]") " COMPILER_NAME: %s\n", COMPILER_NAME);
    init_mod();
    for (int i = 0; i <= 7; i++) { printk("\033[1;13;4%dm  \033[0m", i); }
    printk("\n");
    for (;;);
}
