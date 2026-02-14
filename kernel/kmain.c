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
    printk(ansi_color("[ Kernel ]", ansi_black, ansi_green, ansi_bold; ansi_underline) " Boot By : %s\n",
           bootloader == Limine ? "Limine" : "Unkown");
    printk(ansi_color("[ Kernel ]", ansi_black, ansi_green, ansi_bold; ansi_underline) " Kernel: %s (%s)\n", KERNEL_NAME, KERNEL_VERSION);
    printk(ansi_color("[ Kernel ]", ansi_black, ansi_green, ansi_bold; ansi_underline) " BUILD_DATE: %s, BUILD_TIME: %s\n", BUILD_DATE,
           BUILD_TIME);
    printk(ansi_color("[ Kernel ]", ansi_black, ansi_green, ansi_bold; ansi_underline) " COMPILER_NAME: %s\n", COMPILER_NAME);
    init_mod();
    for (;;);
}
