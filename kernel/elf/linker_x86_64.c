#include "kernel.h"
#include <elf.h>
#include <stdint.h>
#include <string.h>
#include "printk.h"

/**
 * elf_relocate_module - 重定位一个已加载到内存的内核模块（PIE ELF）
 * @base: 模块在内存中的基地址（对应链接地址 0）
 *
 * 说明：
 * - 该函数遍历所有 SHT_RELA 节，处理 R_X86_64_RELATIVE、R_X86_64_64 和 R_X86_64_PC32 重定位。
 * - 对于内部符号，直接使用 base + st_value 计算地址。
 * - 对于外部未定义符号（如 printk、_kinfo），通过扫描内核的 _symbol_table_start/_symbol_table_end
 *   找到匹配的符号，并计算其实际地址（kinfo.bootinfo.kernel_base_addr + offset）。
 * - 若外部符号无法找到，则将该重定位地址填为 0（并可选地输出警告）。
 */
void elf_relocate_module(void *base)
{
    if (!base) return;

    Elf64_Ehdr *ehdr = (Elf64_Ehdr *)base;

    // ELF 魔数校验
    if (ehdr->e_ident[EI_MAG0] != ELFMAG0 || ehdr->e_ident[EI_MAG1] != ELFMAG1 || ehdr->e_ident[EI_MAG2] != ELFMAG2
        || ehdr->e_ident[EI_MAG3] != ELFMAG3)
        return;

    Elf64_Shdr *shdr = (Elf64_Shdr *)((char *)base + ehdr->e_shoff);
    if (ehdr->e_shstrndx == SHN_UNDEF) return;

    for (int i = 0; i < ehdr->e_shnum; i++) {
        if (shdr[i].sh_type != SHT_RELA) continue;

        // 获取重定位信息所引用的符号表（通过 sh_link）
        Elf64_Shdr *symtab_hdr = &shdr[shdr[i].sh_link];
        Elf64_Sym  *symtab     = (Elf64_Sym *)((char *)base + symtab_hdr->sh_offset);
        int         nsyms      = symtab_hdr->sh_size / symtab_hdr->sh_entsize;

        // 获取符号名字符串表（通过符号表节头的 sh_link）
        Elf64_Shdr *strtab_hdr = &shdr[symtab_hdr->sh_link];
        char       *strtab     = (char *)((char *)base + strtab_hdr->sh_offset);

        // 重定位项数组
        Elf64_Rela *rela  = (Elf64_Rela *)((char *)base + shdr[i].sh_offset);
        int         count = shdr[i].sh_size / shdr[i].sh_entsize;

        for (int j = 0; j < count; j++) {
            uint32_t type    = ELF64_R_TYPE(rela[j].r_info);
            uint32_t sym_idx = ELF64_R_SYM(rela[j].r_info);

            uintptr_t *loc      = (uintptr_t *)((char *)base + rela[j].r_offset);
            uintptr_t  sym_addr = 0;

            if (sym_idx != STN_UNDEF && (long)sym_idx < (long)nsyms) {
                Elf64_Sym *sym = &symtab[sym_idx];
                if (sym->st_shndx != SHN_UNDEF) {
                    // 本模块内部符号
                    sym_addr = (uintptr_t)base + sym->st_value;
                } else {
                    // 外部符号，查找内核符号表
                    const char *name = strtab + sym->st_name;
                    // 遍历 ksym 表
                    for (ksym *p = _symbol_table_start; p < _symbol_table_end; p++) {
                        if (strcmp(p->name, name) == 0) {
                            sym_addr = kinfo.bootinfo.kernel_base_addr + p->offset;
                            break;
                        }
                    }
                    if (sym_addr == 0) {
                        // 符号未找到，可输出警告（需要 printk 支持）
                        printk("elf_relocate: unresolved symbol '%s'\n", name);
                    }
                }
            }

            switch (type) {
                case R_X86_64_RELATIVE :
                    *loc = (uintptr_t)base + rela[j].r_addend;
                    break;
                case R_X86_64_64 :
                    *loc = sym_addr + rela[j].r_addend;
                    break;
                case R_X86_64_PC32 :
                    *loc = (uint32_t)(sym_addr + rela[j].r_addend - (uintptr_t)loc);
                    break;
                // 可根据需要添加其他重定位类型
                default :
                    break;
            }
        }
    }
}