#include "cpio.h"
#include "kernel.h"
#include "limine_module.h"

// NOLINTNEXTLINE(misc-use-internal-linkage) : exported via limine_request.c
void limine_enter(void)
{
    // bootloader = Limine;
    // kinfo.bl   = Limine;
    kinfo.bootinfo.bootloader         = Limine;
    kinfo.bootinfo.kernel_base_addr   = kernel_address_request.response->virtual_base;
    kinfo.bootinfo.symbol_table_start = (ksym *)ksym_table_start;
    kinfo.bootinfo.symbol_table_end   = (ksym *)ksym_table_end;
    lmodule_init();
    lmodule_t      *mod  = get_lmodule("initrd");
    newc_filesystem cpio = cpio_parse((newc_header *)mod->data);
    ncfs                 = cpio;
    kernel_entry();
    for (;;);
}
