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

// NOLINTNEXTLINE(misc-use-internal-linkage) : exported public API used by modules (debug.h)
void panic(const char *format, ...)
{
    // disable_intr();
#if KERNEL_LOG
    va_list args;
    va_start(args, format);
    /* 先把内核环形缓冲（syscall 等静默日志）全部刷到串口，便于回看崩溃前轨迹 */
    klog_dump();
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
