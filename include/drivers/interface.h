#ifndef DPI_H
#define DPI_H
#include "common.h"
#include "stdint.h"

// DPI
typedef struct DPI_OP {
        uintptr_t (*ioctl)(uintptr_t op, uintptr_t arg1, uintptr_t arg2);
        uintptr_t (*install)();
        uintptr_t (*uninstall)();
        uintptr_t (*enable)();
        uintptr_t (*disable)();
} dpi_op;

typedef struct DPI_INFO {
        union {
                uintptr_t name_reg;
#if ARCH == x86_64
                uint8_t name_char[8];
#endif
        } name;
        dpi_op ops;
} dpi_info;

// soft interrupt
#define soft_irq2vector(soft_irq) (soft_irq + 32)
#define vector2soft_irq(vector)   (vector - 32)
#define SOFTIRQ_ENABLE (1>>0)
#define SOFTIRQ_DISABLE (0>>0)
extern int is_soft_irq; // enable/disable

typedef struct SoftIrq {
        uintptr_t vector; // bind to ivt/idt/pic/apic interrupt
        uintptr_t (*handler)(uintptr_t data);
        uintptr_t flag;
} soft_riq;
extern soft_riq irqs[256];
uintptr_t do_soft_irq(uintptr_t vector, uintptr_t int_data);

soft_riq register_soft_irq(uintptr_t vector, uintptr_t (*handler)(uintptr_t));

uintptr_t unregister_soft_irq(uintptr_t vector);

#endif
