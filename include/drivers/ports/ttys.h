#ifndef STTY_H
#define STTY_H
#include "drivers/tty/tty.h"

// NOT Support
// #define STTY_INIT  (uintptr_t)0
// #define STTY_OPEN  (uintptr_t)1
// #define STTY_READ  (uintptr_t)2
// #define STTY_WRITE (uintptr_t)3
// #define STTY_CLOSE (uintptr_t)4
// // Write Buf -> WB
// #define STTY_WB       (uintptr_t)5
// #define STTY_WRITEBUF STTY_WB

extern tty_info ttys_info;
extern int      is_stty_enable;

uintptr_t ttys_ioctl(uintptr_t op, uintptr_t arg1, uintptr_t arg2);
uintptr_t ttys_install();
uintptr_t ttys_uninstall();
uintptr_t ttys_enable();
uintptr_t ttys_disable();
#endif