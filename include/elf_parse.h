#ifndef ELF_PARSE_H
#define ELF_PARSE_H
#include <elf.h>
#include "tkm.h"
typedef uintptr_t (*enter)();

enter elf_pie_enter_parse(Elf64_Ehdr *base);

void *elf_get_section(Elf64_Ehdr *base, char *name);

Elf64_Shdr *get_section_headers(Elf64_Ehdr *base);

Elf64_Xword get_symbol_count(Elf64_Shdr *symtab_hdr);

char *get_symbol_name(Elf64_Sym *symtab, char *strtab, int idx);

Elf64_Shdr *get_target_section(Elf64_Shdr *rel_hdr, Elf64_Shdr *shdr);

enter elf_pie_enter_parse(Elf64_Ehdr *base);

void elf_relocate_module(void *base, module_info* mod);

#endif
