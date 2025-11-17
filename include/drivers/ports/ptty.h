#ifndef PTTY_H
#define PTTY_H
#include "drivers/tty/tty.h"
#define PTTY_INIT  (uintptr_t)0
#define PTTY_OPEN  (uintptr_t)1
#define PTTY_READ  (uintptr_t)2
#define PTTY_WRITE (uintptr_t)3
#define PTTY_CLOSE (uintptr_t)4

extern tty_info ptty_info;
extern int is_ptty_enable;

uintptr_t ptty_ioctl(uintptr_t op, uintptr_t arg1, uintptr_t arg2);
uintptr_t ptty_install();
uintptr_t ptty_uninstall();
uintptr_t ptty_enable();
uintptr_t ptty_disable();
#endif