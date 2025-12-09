#ifndef TTY_H
#define TTY_H

#include "drivers/interface.h"
#include <stdint.h>
#define TTY_INIT  (uintptr_t)0
#define TTY_OPEN  (uintptr_t)1
#define TTY_READ  (uintptr_t)2
#define TTY_WRITE (uintptr_t)3
#define TTY_CLOSE (uintptr_t)4
// Write Buf -> WB
#define TTY_WB       (uintptr_t)5
#define TTY_WRITEBUF TTY_WB
#define TTY_BIND     (uintptr_t)6
#define TTY_GET_INFO (uintptr_t)7

typedef struct TTY_INFO {
        dpi_info         info;
        struct TTY_INFO *vtty;
        uintptr_t        tty_id;
} tty_info;

// vtty
extern tty_info vtty_list[4];
uintptr_t       vtty_ioctl(uintptr_t op, uintptr_t arg1, uintptr_t arg2);
uintptr_t       vtty_install();
uintptr_t       vtty_uninstall();
uintptr_t       vtty_enable();
uintptr_t       vtty_disable();

#endif
