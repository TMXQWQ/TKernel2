#include "kernel.h"
#include "printk.h"
#include "serial.h"
#include "tkm.h"

kernel_info kinfo;

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
    printk(ansi_V("[ Kernel ]") " Boot By : %s\n", kinfo.bootinfo.bootloader == Limine ? "Limine" : "Unkown");
    printk(ansi_V("[ Kernel ]") " Kernel: %s", KERNEL_NAME);
    printk("(%s)\n", KERNEL_VERSION);
    printk(ansi_V("[ Kernel ]") " BUILD_DATE: %s, BUILD_TIME: %s\n", BUILD_DATE, BUILD_TIME);
    printk(ansi_V("[ Kernel ]") " COMPILER_NAME: %s\n", COMPILER_NAME);
    //
    ksym     *kts = kinfo.bootinfo.symbol_table_start;
    ksym     *kte = kinfo.bootinfo.symbol_table_end;
    uintptr_t krs = kinfo.bootinfo.kernel_base_addr;
    ksym     *p   = kts;
    printk("%p %p\n", kts, kte);
    for (; p != kte; p++) printk("%s %p\n", p->name, p->offset+ krs);
    //
    // init_mod();
    for (int i = 0; i <= 7; i++) { printk("\033[1;13;4%dm  \033[0m", i); }
    printk("\n");
    for (;;);
}
