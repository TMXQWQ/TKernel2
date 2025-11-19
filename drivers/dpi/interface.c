#include "drivers/interface.h"

soft_riq irqs[256];

uintptr_t do_soft_irq(uintptr_t vector, uintptr_t int_data)
{
    return irqs[vector].flag == SOFTIRQ_ENABLE ? irqs[vector].handler(int_data) : 0;
}

soft_riq register_soft_irq(uintptr_t vector, uintptr_t (*handler)(uintptr_t))
{
    irqs[vector].handler = handler;
    irqs[vector].flag    = SOFTIRQ_ENABLE;
    return irqs[vector];
}

uintptr_t unregister_soft_irq(uintptr_t vector)
{
    irqs[vector].flag = SOFTIRQ_DISABLE;
    return 0;
}
