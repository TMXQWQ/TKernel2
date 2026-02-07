#include "cpio.h"
#include "kernel.h"
#include "limine_module.h"

void limine_enter(void)
{
    bootloader = Limine;
    kinfo.bl   = Limine;
    lmodule_init();
    lmodule_t      *mod  = get_lmodule("initrd");
    newc_filesystem cpio = cpio_parse((newc_header *)mod->data);
    ncfs                 = cpio;
    kernel_entry();
    for (;;) __asm__("hlt");
}
