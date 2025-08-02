#pragma once
#include "dpi.h"


// driver *tty_open(driver *d, uint64_t index){}
BUILD_DRIVER_HANDLE(tty)

typedef struct TTY_Private{
    driver* d;  //实际上使用的驱动
    int index;
} tty_private;
