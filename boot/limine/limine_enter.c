#include "kernel.h"
#include "limine_module.h"
void limine_enter(void)
{
    bootloader = Limine;
    kinfo.bl   = Limine;
    lmodule_init();
    kernel_entry();
    for (;;) __asm__("hlt");
}
