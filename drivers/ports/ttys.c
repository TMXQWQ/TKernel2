#include "drivers/ports/ttys.h"
#include "drivers/ports/serial.h"
#include "drivers/tty/tty.h"
#include "stdint.h"

int is_stty_enable = 0;

tty_info ttys_info = {
    .info = {
#if ARCH == x86_64
               .name.name_char = {"TTYS   "},
#endif
               .ops = {
            .ioctl     = ttys_ioctl,
            .install   = ttys_install,
            .uninstall = ttys_uninstall,
            .enable    = ttys_enable,
            .disable   = ttys_disable,
        },
    }
};

uintptr_t ttys_ioctl(uintptr_t op, uintptr_t arg1, uintptr_t arg2)
{
    (void)arg1, (void)arg2;
    switch (op) {
        case TTY_INIT :
            init_serial();
            read_serial(SERIAL_PORT_1 + arg1 - 1); // clear
            break;
        case TTY_WRITE :
            write_serial(SERIAL_PORT_1 + arg1 - 1, (uint8_t)arg2);
            break;
        case TTY_READ :
            return read_serial(SERIAL_PORT_1 + arg1 - 1);
            break;
        case TTY_WB : // test! will be remove in future awa
            for (uintptr_t i = 0; *((uint8_t *)arg2 + i) != '\0'; i++) write_serial(SERIAL_PORT_1 + arg1 - 1, *((uint8_t *)arg2 + i));
            break;
        case TTY_GET_INFO:
            return (uintptr_t)&ttys_info;
        default :
            break;
    }
    return 0;
}
uintptr_t ttys_install()
{
    ttys_ioctl(TTY_INIT, 0, 0);
    return 0;
}
uintptr_t ttys_uninstall()
{
    ttys_ioctl(TTY_CLOSE, 0, 0);
    return 0;
}
uintptr_t ttys_enable()
{
    is_stty_enable = 1;
    return 0;
}
uintptr_t ttys_disable()
{
    is_stty_enable = 0;
    return 0;
}
