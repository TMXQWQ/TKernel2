#include "kernel.h"
#include "printk.h"
#include "tkm.h"
#include <elf.h>
#include <stdint.h>
#include <string.h>

/* Module Info */
#define Mod_Name               TEST
#define __macro_to_str_impl(m) #m
#define __macro_to_str(m)      __macro_to_str_impl(m)

#define MI_NAME __macro_to_str(Mod_Name)
/* End Of Module Info */

int strcmp(const char *str1, const char *str2)
{
#if defined(__builtin_strcmp)
    return __builtin_strcmp(str1, str2);
#else
    const uint8_t *_str1 = (const uint8_t *)str1;
    const uint8_t *_str2 = (const uint8_t *)str2;
    int            c1, c2;

    do {
        c1 = *_str1++;
        c2 = *_str2++;
        if (!c1) return c1 - c2;
    } while (c1 == c2);
    return c1 - c2;
#endif
}

// typeof(printk)(*_printk);
// void printk(const char *format, ...)
// {
//     va_list args;
//     va_start(args, format);
//     _printk(format, args);
//     va_end(args);
// }

kernel_info *_kinfo;

uintptr_t module_init(void)
{
    printk("test\n");
    return 0;
}
module_info *_start(kernel_info *ki)
{
    static module_info mi;
    _kinfo  = ki;
    mi.name = MI_NAME;
    mi.init = module_init;
    // // 重定位
    // ksym     *kts = _kinfo->bootinfo.symbol_table_start;
    // ksym     *kte = _kinfo->bootinfo.symbol_table_end;
    // uintptr_t krs = _kinfo->bootinfo.kernel_base_addr;
    // ksym     *p   = kts;
    // for (; (uintptr_t)p < (uintptr_t)kte; p++)
    //     if (!strcmp(p->name, "printk")) { _printk = (typeof(_printk))((uintptr_t)(p->offset) + (uintptr_t)krs); }
    return &mi;
}
