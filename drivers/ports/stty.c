#include "drivers/ports/stty.h"
#include "drivers/ports/serial.h"
#include "stdint.h"

int is_stty_enable = 0;

tty_info stty_info = {
    .father = {
#if ARCH == x86_64
               .name.name_char = {"STTY   "},
#endif
               .ops = {
            .ioctl     = stty_ioctl,
            .install   = stty_install,
            .uninstall = stty_uninstall,
            .enable    = stty_enable,
            .disable   = stty_disable,
        },
    }
};

uintptr_t stty_ioctl(uintptr_t op, uintptr_t arg1, uintptr_t arg2)
{
    (void)arg1, (void)arg2;
    switch (op) {
        case STTY_INIT :
            init_serial();
            read_serial(SERIAL_PORT_1 + arg1 - 1); // clear
            break;
        case STTY_WRITE :
            write_serial(SERIAL_PORT_1 + arg1 - 1, (uint8_t)arg2);
            break;
        case STTY_READ :
            return read_serial(SERIAL_PORT_1 + arg1 - 1);
            break;
        case STTY_WB : // test! will be remove in future awa
            for (uintptr_t i = 0; *((uint8_t *)arg2 + i) != '\0'; i++) write_serial(SERIAL_PORT_1 + arg1 - 1, *((uint8_t *)arg2 + i));
            break;
        default :
            break;
    }
    return 0;
}
uintptr_t stty_install()
{
    stty_ioctl(STTY_INIT, 0, 0);
    return 0;
}
uintptr_t stty_uninstall()
{
    stty_ioctl(STTY_CLOSE, 0, 0);
    return 0;
}
uintptr_t stty_enable()
{
    is_stty_enable = 1;
    return 0;
}
uintptr_t stty_disable()
{
    is_stty_enable = 0;
    return 0;
}
