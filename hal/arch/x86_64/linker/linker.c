#include "debug.h"
#include "elf_parse.h"
#include "kernel.h"
#include "kpi.h"
#include "printk.h"
#include "tkm.h"
#include <elf.h>
#include <stdint.h>
#include <string.h>

// 通过内核符号表解析内核导出符号
static uintptr_t resolve_kernel_symbol(const char *name)
{
    for (ksym *p = ksym_table_start; p < ksym_table_end; p++) {
        if (strcmp(p->name, name) == 0) return kinfo.bootinfo.kernel_base_addr + p->offset;
    }
    return 0;
}

// 两阶段模块重定位：
//   - mod == NULL：第一阶段，只重定位内部符号（模块内）+ 内核导出符号（ksym 表）
//   - mod != NULL：第二阶段，统一重定位外部符号（模块间 KPI 导出，回退到内核符号表）
void elf_relocate_module(void *base, module_info *mod)
{
    plogk_info_stack[++plogk_info_ptr] = "RELOC";
    if (!base) goto END;
    Elf64_Ehdr *ehdr = (Elf64_Ehdr *)base;

    Elf64_Shdr *shdr = (Elf64_Shdr *)((char *)base + ehdr->e_shoff);

    for (int i = 0; i < ehdr->e_shnum; i++) {
        if (shdr[i].sh_type != SHT_RELA) continue;

        Elf64_Shdr *symtab_hdr = &shdr[shdr[i].sh_link];
        Elf64_Sym  *symtab     = (Elf64_Sym *)((char *)base + symtab_hdr->sh_offset);
        int         nsyms      = symtab_hdr->sh_size / symtab_hdr->sh_entsize;

        Elf64_Shdr *strtab_hdr = &shdr[symtab_hdr->sh_link];
        char       *strtab     = (char *)((char *)base + strtab_hdr->sh_offset);

        Elf64_Rela *rela           = (Elf64_Rela *)((char *)base + shdr[i].sh_offset);
        int         count          = shdr[i].sh_size / shdr[i].sh_entsize;
        uint32_t    target_sec_idx = shdr[i].sh_info;
        if (target_sec_idx >= ehdr->e_shnum) continue;

        uint64_t target_sec_base = (uint64_t)base + shdr[target_sec_idx].sh_offset;

        for (int j = 0; j < count; j++) {
            uint32_t type    = ELF64_R_TYPE(rela[j].r_info);
            uint32_t sym_idx = ELF64_R_SYM(rela[j].r_info);
            int64_t  addend  = rela[j].r_addend;

            uintptr_t  *loc      = (uintptr_t *)((uintptr_t)target_sec_base + rela[j].r_offset);
            uintptr_t   sym_addr = 0;
            const char *sym_name = "?";

            if (sym_idx > 0 && (uint64_t)sym_idx < (uint64_t)nsyms) {
                Elf64_Sym *sym = &symtab[sym_idx];
                sym_name       = strtab + sym->st_name;

                if (sym->st_shndx != SHN_UNDEF) {
                    // 内部符号：直接计算地址
                    uint64_t sec_base = (uint64_t)base + shdr[sym->st_shndx].sh_offset;
                    sym_addr          = sec_base + sym->st_value;
                } else {
                    // 外部符号
                    const char *name = strtab + sym->st_name;
                    if (mod) {
                        // 第二阶段：优先用 KPI 解析模块间导出符号
                        if (mod->version == KPI_VERSION) {
                            sym_addr = kpi_resolve_symbol(mod, name);
                        }
                        // KPI 解析不到或非 KPI 模块时，回退到内核符号表
                        if (!sym_addr) sym_addr = resolve_kernel_symbol(name);
                    } else {
                        // 第一阶段：只解析内核导出符号
                        sym_addr = resolve_kernel_symbol(name);
                    }
                }
            }

            // 重定位类型处理
            uintptr_t value = 0;
            switch (type) {
                case R_X86_64_RELATIVE :
                    value = (uintptr_t)base + addend;
                    *loc  = value;
                    break;
                case R_X86_64_64 :
                    value = sym_addr + addend;
                    *loc  = value;
                    break;
                case R_X86_64_PC32 :
                case R_X86_64_PLT32 :
                case R_X86_64_GOTPCREL :
                    value            = (uint32_t)(sym_addr + addend - (uintptr_t)loc);
                    *(uint32_t *)loc = (uint32_t)value;
                    break;
                case R_X86_64_GOT64 :
                case R_X86_64_PLTOFF64 :
                    value            = (uint64_t)(sym_addr + addend);
                    *(uint64_t *)loc = (uint64_t)value;
                    break;
                default :
                    plogk("Unsupported type %d at %p (sym=%s)\n", type, loc, sym_name);
                    break;
            }
        }
    }
END:
    plogk_info_ptr--;
}
