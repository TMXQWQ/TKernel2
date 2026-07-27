// kernel/elf/linker_x86_64.c (来源：rebuild分支)

#include "kernel.h"
#include "elf_parse.h"
#include "printk.h"
#include <elf.h>
#include <stdint.h>
#include <string.h>
#include "tkm.h"
#include "kpi.h"

// 修改函数声明，增加 mod 参数
void elf_relocate_module(void *base, module_info *mod)
{
    plogk_info_stack[++plogk_info_ptr] = "RELOC";
    if (!base) return;
    Elf64_Ehdr *ehdr = (Elf64_Ehdr *)base;

    Elf64_Shdr *shdr = (Elf64_Shdr *)((char *)base + ehdr->e_shoff);

    for (int i = 0; i < ehdr->e_shnum; i++) {
        if (shdr[i].sh_type != SHT_RELA) continue;

        Elf64_Shdr *symtab_hdr = &shdr[shdr[i].sh_link];
        Elf64_Sym  *symtab     = (Elf64_Sym *)((char *)base + symtab_hdr->sh_offset);
        int         nsyms      = symtab_hdr->sh_size / symtab_hdr->sh_entsize;

        Elf64_Shdr *strtab_hdr = &shdr[symtab_hdr->sh_link];
        char       *strtab     = (char *)((char *)base + strtab_hdr->sh_offset);

        Elf64_Rela *rela  = (Elf64_Rela *)((char *)base + shdr[i].sh_offset);
        int         count = shdr[i].sh_size / shdr[i].sh_entsize;
        uint32_t target_sec_idx = shdr[i].sh_info;
        if (target_sec_idx >= ehdr->e_shnum) continue;

        uint64_t target_sec_base = (uint64_t)base + shdr[target_sec_idx].sh_offset;

        for (int j = 0; j < count; j++) {
            uint32_t type    = ELF64_R_TYPE(rela[j].r_info);
            uint32_t sym_idx = ELF64_R_SYM(rela[j].r_info);
            int64_t  addend  = rela[j].r_addend;

            uintptr_t *loc = (uintptr_t *)((uintptr_t)target_sec_base + rela[j].r_offset);
            uintptr_t   sym_addr = 0;
            const char *sym_name = "?";

            if (sym_idx > 0 && (uint64_t)sym_idx < (uint64_t)nsyms) {
                Elf64_Sym *sym = &symtab[sym_idx];
                sym_name       = strtab + sym->st_name;

                if (sym->st_shndx != SHN_UNDEF) {
                    // 内部符号：直接计算地址
                    uint64_t sec_base = (uint64_t)base + shdr[sym->st_shndx].sh_offset;
                    sym_addr = sec_base + sym->st_value;
                } else {
                    if (!mod) {
                        plogk("Mod=Null.Skip kpi sym.\n");
                        continue;
                    }
                    // 外部符号：使用 KPI 解析（如果可用）
                    const char *name = strtab + sym->st_name;
                    if (mod && mod->version == KPI_VERSION) {
                        // 调用 KPI 符号解析（仅限依赖模块）
                        sym_addr = kpi_resolve_symbol(mod, name);
                    } else {
                        // 兼容旧模式：只在内核符号表查找
                        for (ksym *p = _symbol_table_start; p < _symbol_table_end; p++) {
                            if (strcmp(p->name, name) == 0) {
                                sym_addr = kinfo.bootinfo.kernel_base_addr + p->offset;
                                break;
                            }
                        }
                    }
                    if (!sym_addr) {
                        plogk("FAILED to find external symbol '%s' (module %s)\n",
                              name, mod ? mod->name : "(legacy)");
                    }
                }
            }

            // 重定位类型处理（保持不变）
            uintptr_t value = 0;
            switch (type) {
                case R_X86_64_RELATIVE:
                    value = (uintptr_t)base + addend;
                    *loc  = value;
                    break;
                case R_X86_64_64:
                    value = sym_addr + addend;
                    *loc  = value;
                    break;
                case R_X86_64_PC32:
                case R_X86_64_PLT32:
                    value = (uint32_t)(sym_addr + addend - (uintptr_t)loc);
                    *(uint32_t *)loc = (uint32_t)value;
                    break;
                case R_X86_64_GOT64:
                case R_X86_64_PLTOFF64:
                    value = (uint64_t)(sym_addr + addend);
                    *(uint64_t *)loc = (uint64_t)value;
                    break;
                default:
                    plogk("Unsupported type %d at %p (sym=%s)\n", type, loc, sym_name);
                    break;
            }
        }
    }
    plogk_info_ptr--;
}