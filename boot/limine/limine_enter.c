#include "kernel.h"
void limine_enter(void)
{
    bootloader = Limine;
    kernel_entry();
    for (;;) __asm__("hlt");
}
