#include "drivers/tty/tty.h"
#include <stdint.h>

tty_info vtty_list[4];

uintptr_t vtty_ioctl(uintptr_t op, uintptr_t arg1, uintptr_t arg2)
{
    (void)arg1, (void)arg2;
    tty_info *t = (tty_info *)arg1;
    switch (op) {
        case TTY_INIT :
            t->info.ops.ioctl(TTY_OPEN, 0, 0);
            break;
        case TTY_OPEN :
            t->info.ops.ioctl(TTY_OPEN, *((uintptr_t *)arg2), *((uintptr_t *)arg2 + 1));
            break;
        case TTY_READ :
            t = &vtty_list[arg1];
            return t->vtty->info.ops.ioctl(TTY_READ, arg2, 0);
        case TTY_WRITE :
            t = &vtty_list[arg1];
            t->vtty->info.ops.ioctl(TTY_WRITE, *((uintptr_t *)arg2), *((uintptr_t *)arg2 + 1));
            break;
        case TTY_BIND :
            t       = &vtty_list[arg1];
            t->vtty = (tty_info *)arg2; 
            break;
        case TTY_GET_INFO :
            return (uintptr_t)(vtty_list + arg1);
        default :
            break;
    }
    return 0;
}
uintptr_t vtty_install();
uintptr_t vtty_uninstall();
uintptr_t vtty_enable();
uintptr_t vtty_disable();
