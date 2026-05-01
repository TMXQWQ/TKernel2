#include "kernel.h"
// #include "printk.h"
#include "tkm.h"

/* Module Info */
#define MI_NAME "TEST2"
/* End Of Module Info */

uintptr_t module_init()
{ return 1; }
module_info *_start(kernel_info *ki)
{
    (void)ki;
    static module_info mi;
    mi.name = MI_NAME;
    mi.init = module_init;
    // printk("test\n");
    return &mi;
}
