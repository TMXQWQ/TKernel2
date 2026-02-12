#include "cpio.h"
#include "elf_parse.h"
#include "tkm.h"
#include <stdint.h>
#include "printk.h"

int init_mod()
{
    for (size_t i = 0; i < ncfs.size; i++) {
        enter test;
        char *tmp = ncfs.file_list[i].name;
        for (int j = 0; tmp[j] != '\0'; j++)
            if (tmp[j] == '.' && tmp[j + 1] != '\0' && tmp[j + 2] != '\0' && tmp[j + 3] != '\0' && tmp[j + 1] == 't' && tmp[j + 2] == 'k'
                && tmp[j + 3] == 'm') {
                test = elf_pie_enter_parse((Elf64_Ehdr *)ncfs.file_list[i].data_ptr);
                char* name = load_mod((mod_enter)(uintptr_t)test)->name;
                printk("[ Module ] Load module %s.\n", name);
            }
    }
    return 0;
}

module_info *load_mod(mod_enter e)
{
    return e();
}
