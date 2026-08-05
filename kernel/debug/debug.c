#include "common.h"
#include "printk.h"

static void pERRk()
{
    printk("%s", "\e["
                 "1"
                 ";3"
                 "1"
                 ";4"
                 "0"
                 "m");
    printk("[ Panic ] ");
    printk("\e[0m");
}

void panic(const char *format, ...)
{
    // disable_intr();
#if KERNEL_LOG
    va_list args;
    va_start(args, format);
    pERRk();
    printk("Kernel Panic --- Not Sync.\n");
    pERRk();
    printk("\t--- ");
    vwprintf(&stdio, format, args);
    printk("\n");
    va_end(args);
    for (;;);
#else
    (void)format;
    for (;;);
#endif
}
