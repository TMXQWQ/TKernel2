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

uintptr_t module_init(void);
void test();

char *deps[] = { "TEST", NULL };

module_info mi = {
    .name         = MI_NAME,
    .init         = module_init,
    .version      = KPI_VERSION,
    .export_table = NULL,
    .export_count = 0,
    .dependencies = deps,
    .dep_count    = 1,
};

uintptr_t module_init(void)
{
    printk("test\n");
    test();
    return 0;
}
module_info *_start(kernel_info *ki)
{
    (void)ki;
    return (module_info *)&mi;
}
