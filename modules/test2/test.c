#include "kernel.h"
#include "tkm.h"

uintptr_t module_init(kernel_info *);

module_info test_info = {
    "TEST2",
    0,
    module_init,
};

uintptr_t module_init(kernel_info *ki)
{
    (void)ki;
    return 1;
}
module_info *_start(void)
{
    test_info.name = "TEST2";
    test_info.init = module_init;
    return &test_info;
}
