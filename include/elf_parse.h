#ifndef ELF_PARSE_H
#define ELF_PARSE_H
#include <elf.h>
typedef uintptr_t (*enter)(uint64_t *);

enter elf_pie_enter_parse(Elf64_Ehdr *base);
#endif
