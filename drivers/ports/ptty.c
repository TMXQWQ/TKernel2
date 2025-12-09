#include "drivers/ports/ptty.h"
#include "drivers/ports/parallel.h"
#include "stdint.h"

int is_ptty_enable = 0;

tty_info ptty_info = {
    .info = {
#if ARCH == x86_64
               .name.name_char = {"PTTY   "},
#endif
               .ops = {
            .ioctl     = ptty_ioctl,
            .install   = ptty_install,
            .uninstall = ptty_uninstall,
            .enable    = ptty_enable,
            .disable   = ptty_disable,
        },
    }
};

uintptr_t ptty_ioctl(uintptr_t op, uintptr_t arg1, uintptr_t arg2)
{
    (void)arg1, (void)arg2;
    switch (op) {
        case PTTY_INIT :
            init_parallel();
            break;
        case PTTY_WRITE :
            write_parallel(PARALLEL_PORT_1 + arg1 - 1, (uint8_t)arg2);
            break;
        default :
            break;
    }
    return 0;
}
uintptr_t ptty_install()
{
    ptty_ioctl(PTTY_INIT, 0, 0);
    return 0;
}
uintptr_t ptty_uninstall()
{
    ptty_ioctl(PTTY_CLOSE, 0, 0);
    return 0;
}
uintptr_t ptty_enable()
{
    is_ptty_enable = 1;
    return 0;
}
uintptr_t ptty_disable()
{
    is_ptty_enable = 0;
    return 0;
}
