#ifndef DPI_H
#define DPI_H
#include "common.h"
#include "stdint.h"

typedef struct DPI_OP {
        uintptr_t (*ioctl)(uintptr_t op, uintptr_t arg1, uintptr_t arg2);
        uintptr_t (*install)();
        uintptr_t (*uninstall)();
        uintptr_t (*enable)();
        uintptr_t (*disable)();
} dpi_op;

typedef struct DPI_INFO {
        union name {
                uintptr_t name_reg;
#if ARCH == x86_64
                uint8_t name_char[8];
#endif
        };
        dpi_op ops;
} dpi_info;

#endif