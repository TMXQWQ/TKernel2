#include "kernel.h"
#include "printk.h"
#include "tkm.h"
#include <elf.h>
#include <stdint.h>

/* Module Info */
#define Mod_Name               TEST2
#define __macro_to_str_impl(m) #m
#define __macro_to_str(m)      __macro_to_str_impl(m)

#define MI_NAME __macro_to_str(Mod_Name)
/* End Of Module Info */

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
    return (module_info*)&mi;
}
