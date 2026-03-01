#include "cpio.h"
#include "elf_parse.h"
#include "kernel.h"
#include "printk.h"
#include "stddef.h"
#include "stdint.h"
#include "tkm.h"
#include <elf.h>

int init_mod()
{
    for (size_t i = 0; i < ncfs.size; i++) {
        char *tmp = ncfs.file_list[i].name;
        for (int j = 0; tmp[j] != '\0'; j++)
            if (tmp[j] == '.' && tmp[j + 1] != '\0' && tmp[j + 2] != '\0' && tmp[j + 3] != '\0' && tmp[j + 1] == 't' && tmp[j + 2] == 'k'
                && tmp[j + 3] == 'm') {
                // test             = elf_pie_enter_parse((Elf64_Ehdr *)ncfs.file_list[i].data_ptr);
                module_info *mod = load_mod((Elf64_Ehdr *)ncfs.file_list[i].data_ptr);
                (void)mod;
                // printk(ansi_V("[ Module ]") " Load module %s.\n", mod->name);
            }
    }
    return 0;
}

module_info *load_mod(Elf64_Ehdr *base_addr)
{
    // int machine = ((Elf64_Ehdr *)ncfs.file_list[i].data_ptr)->e_machine;
    int machine = base_addr->e_machine;
    switch (machine) {
#ifdef __x86_64__
        case EM_X86_64 :
            break;
#endif
#ifdef __riscv
        case EM_RISCV :
            break;
#endif
#ifdef __loongarch64
        case EM_LOONGARCH :
            break;
#endif
        default :
            printk(ansi_V("[ Module ]") " Unsupport platform: %s.\n", machine == EM_X86_64    ? "x86_64" :
                                                                      machine == EM_RISCV     ? "RISC-V" :
                                                                      machine == EM_LOONGARCH ? "loongarch64" :
                                                                                                "Unknown");
            return NULL;
    }
    mod_enter    test = (mod_enter)(intptr_t)elf_pie_enter_parse(base_addr);
    module_info *mod  = test(&kinfo);
    printk(ansi_V("[ Module ]") " Load module %s.\n", mod->name);
    printk(ansi_V("[ Module ]") "Module returned : %d\n", mod->init());
    return mod;
}
