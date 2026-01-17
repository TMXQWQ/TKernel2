#include "elf_parse.h"
#include <elf.h>
#include <stdint.h>

enter elf_pie_enter_parse(Elf64_Ehdr *base)
{
    return (enter)(((uintptr_t)base) + ((uintptr_t)(base->e_entry) - 0x400000));
}
