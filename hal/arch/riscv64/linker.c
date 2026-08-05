#include "elf_parse.h"
#include "kernel.h"
#include "kpi.h"
#include "printk.h"
#include "tkm.h"
#include <elf.h>
#include <stdint.h>
#include <string.h>

/* 辅助宏：从 32 位指令中提取/修改立即数位域 */
#define RISCV_IMM_I(val) (((val) & 0xFFF) << 20)
#define RISCV_IMM_S(val) ((((val) & 0x1F) << 7) | (((val) & 0xFE0) << 20))

static inline uint32_t insn_set_imm_i(uint32_t insn, int32_t imm)
{
    insn = (insn & ~(0xFFF << 20)) | RISCV_IMM_I(imm);
    return insn;
}

static inline uint32_t insn_set_imm_s(uint32_t insn, int32_t imm)
{
    insn = (insn & ~(0xFE0 << 20)) & ~(0x1F << 7);
    insn |= RISCV_IMM_S(imm);
    return insn;
}

/* 用于配对 PCREL_HI20 和 PCREL_LO12 的映射表 */
#define MAX_PCREL_PAIRS 128
struct pcrel_pair {
    uintptr_t auipc_addr;   // auipc 指令的运行时地址
    uintptr_t target;       // 目标符号地址 (S + A)
};

void elf_relocate_module(void *base, module_info *mod)
{
    plogk_info_stack[++plogk_info_ptr] = "RELOC";
    if (!mod) {
        plogk("Mod=Null.Skip kpi sym.\n");
    }
    if (!base) return;

    Elf64_Ehdr *ehdr = (Elf64_Ehdr *)base;
    Elf64_Shdr *shdr = (Elf64_Shdr *)((char *)base + ehdr->e_shoff);

    /* 先扫描符号表，以便后续使用 */
    Elf64_Shdr *symtab_hdr = NULL, *strtab_hdr = NULL;
    Elf64_Sym  *symtab     = NULL;
    char       *strtab     = NULL;
    int         nsyms      = 0;

    for (int i = 0; i < ehdr->e_shnum; i++) {
        if (shdr[i].sh_type == SHT_SYMTAB) {
            symtab_hdr = &shdr[i];
            symtab     = (Elf64_Sym *)((char *)base + symtab_hdr->sh_offset);
            nsyms      = symtab_hdr->sh_size / symtab_hdr->sh_entsize;
            strtab_hdr = &shdr[symtab_hdr->sh_link];
            strtab     = (char *)((char *)base + strtab_hdr->sh_offset);
            break;
        }
    }

    /* 存储 PCREL_HI20 配对信息 */
    struct pcrel_pair pairs[MAX_PCREL_PAIRS];
    int pair_count = 0;

    /* 第一遍：处理所有 RELA 节，记录 HI20 配对 */
    for (int i = 0; i < ehdr->e_shnum; i++) {
        if (shdr[i].sh_type != SHT_RELA) continue;

        Elf64_Shdr *rela_hdr = &shdr[i];
        Elf64_Rela *rela = (Elf64_Rela *)((char *)base + rela_hdr->sh_offset);
        int count = rela_hdr->sh_size / rela_hdr->sh_entsize;
        uint32_t target_sec_idx = rela_hdr->sh_info;
        if (target_sec_idx >= ehdr->e_shnum) continue;
        uint64_t target_sec_base = (uint64_t)base + shdr[target_sec_idx].sh_offset;

        for (int j = 0; j < count; j++) {
            uint32_t type = ELF64_R_TYPE(rela[j].r_info);
            if (type == R_RISCV_PCREL_HI20) {
                uint32_t sym_idx = ELF64_R_SYM(rela[j].r_info);
                int64_t  addend  = rela[j].r_addend;
                uintptr_t sym_addr = 0;

                if (sym_idx > 0 && (uint64_t)sym_idx < (uint64_t)nsyms) {
                    Elf64_Sym *sym = &symtab[sym_idx];
                    if (sym->st_shndx != SHN_UNDEF) {
                        uint64_t sec_base = (uint64_t)base + shdr[sym->st_shndx].sh_offset;
                        sym_addr = sec_base + sym->st_value;
                    } else {
                        const char *name = strtab + sym->st_name;
                        if (mod && mod->version == KPI_VERSION) {
                            sym_addr = kpi_resolve_symbol(mod, name);
                        } else {
                            for (ksym *p = _symbol_table_start; p < _symbol_table_end; p++) {
                                if (strcmp(p->name, name) == 0) {
                                    sym_addr = kinfo.bootinfo.kernel_base_addr + p->offset;
                                    break;
                                }
                            }
                        }
                        if (!sym_addr) {
                            plogk("FAILED to find external symbol '%s' (module %s)\n", name, mod ? mod->name : "(legacy)");
                        }
                    }
                }

                uintptr_t target = sym_addr + addend;
                uintptr_t auipc_loc = (uintptr_t)target_sec_base + rela[j].r_offset;  // 修正点：使用目标节基址
                if (pair_count < MAX_PCREL_PAIRS) {
                    pairs[pair_count].auipc_addr = auipc_loc;
                    pairs[pair_count].target     = target;
                    pair_count++;
                } else {
                    plogk("PCREL_HI20: too many pairs, increase MAX_PCREL_PAIRS\n");
                }
            }
        }
    }

    /* 第二遍：执行实际重定位 */
    for (int i = 0; i < ehdr->e_shnum; i++) {
        if (shdr[i].sh_type != SHT_RELA) continue;

        Elf64_Shdr *rela_hdr = &shdr[i];
        Elf64_Rela *rela = (Elf64_Rela *)((char *)base + rela_hdr->sh_offset);
        int count = rela_hdr->sh_size / rela_hdr->sh_entsize;
        uint32_t target_sec_idx = rela_hdr->sh_info;
        if (target_sec_idx >= ehdr->e_shnum) continue;
        uint64_t target_sec_base = (uint64_t)base + shdr[target_sec_idx].sh_offset;

        for (int j = 0; j < count; j++) {
            uint32_t type    = ELF64_R_TYPE(rela[j].r_info);
            uint32_t sym_idx = ELF64_R_SYM(rela[j].r_info);
            int64_t  addend  = rela[j].r_addend;

            uintptr_t  *loc = (uintptr_t *)((uintptr_t)target_sec_base + rela[j].r_offset);
            uintptr_t   sym_addr = 0;
            const char *sym_name = "?";

            if (sym_idx > 0 && (uint64_t)sym_idx < (uint64_t)nsyms) {
                Elf64_Sym *sym = &symtab[sym_idx];
                sym_name = strtab + sym->st_name;

                if (sym->st_shndx != SHN_UNDEF) {
                    uint64_t sec_base = (uint64_t)base + shdr[sym->st_shndx].sh_offset;
                    sym_addr = sec_base + sym->st_value;
                } else {
                    if (!mod) continue;
                    const char *name = strtab + sym->st_name;
                    if (mod && mod->version == KPI_VERSION) {
                        sym_addr = kpi_resolve_symbol(mod, name);
                    } else {
                        for (ksym *p = _symbol_table_start; p < _symbol_table_end; p++) {
                            if (strcmp(p->name, name) == 0) {
                                sym_addr = kinfo.bootinfo.kernel_base_addr + p->offset;
                                break;
                            }
                        }
                    }
                    if (!sym_addr) {
                        plogk("FAILED to find external symbol '%s' (module %s)\n", name, mod ? mod->name : "(legacy)");
                    }
                }
            }

            uintptr_t value = 0;
            uint32_t  insn;

            switch (type) {
                /* ---- 绝对地址 ---- */
                case R_RISCV_64:
                    value = sym_addr + addend;
                    *(uint64_t *)loc = (uint64_t)value;
                    break;
                case R_RISCV_32:
                    value = (uint32_t)(sym_addr + addend);
                    *(uint32_t *)loc = (uint32_t)value;
                    break;

                /* ---- 基址相对 ---- */
                case R_RISCV_RELATIVE:
                    value = (uintptr_t)base + addend;
                    *loc = value;
                    break;

                /* ---- PC 相对调用（auipc + jalr） ---- */
                case R_RISCV_CALL:
                case R_RISCV_CALL_PLT: {
                    uint32_t *auipc_insn = (uint32_t *)loc;
                    uint32_t *jalr_insn  = (uint32_t *)((uintptr_t)loc + 4);

                    if ((*jalr_insn & 0x7f) != 0x67) {
                        plogk("R_RISCV_CALL: expected jalr after auipc at %p\n", loc);
                        break;
                    }
                    int rd  = (*auipc_insn >> 7) & 0x1f;
                    int rs1 = (*jalr_insn >> 15) & 0x1f;
                    if (rd != rs1) {
                        plogk("R_RISCV_CALL: rd != rs1 at %p\n", loc);
                        break;
                    }

                    int64_t delta = sym_addr + addend - (uintptr_t)loc;
                    int32_t hi = (int32_t)((delta + 0x800) >> 12);
                    int32_t lo = (int32_t)(delta & 0xfff);
                    if (lo & 0x800) hi += 1;

                    *auipc_insn = (*auipc_insn & ~(0xfffff << 12)) | ((hi & 0xfffff) << 12);
                    *jalr_insn  = (*jalr_insn & ~(0xfff << 20)) | ((lo & 0xfff) << 20);
                    break;
                }

                /* ---- PCREL HI20/LO12 配对（用于全局数据访问） ---- */
                case R_RISCV_PCREL_HI20: {
                    uintptr_t target = sym_addr + addend;
                    int64_t delta = target - (uintptr_t)loc;
                    int32_t hi = (int32_t)((delta + 0x800) >> 12);
                    insn = *(uint32_t *)loc;
                    insn = (insn & ~(0xfffff << 12)) | ((hi & 0xfffff) << 12);
                    *(uint32_t *)loc = insn;
                    break;
                }
                case R_RISCV_PCREL_LO12_I:
                case R_RISCV_PCREL_LO12_S: {
                    Elf64_Sym *sym = &symtab[sym_idx];
                    // 检查 st_shndx 有效性
                    if (sym->st_shndx >= ehdr->e_shnum || sym->st_shndx == SHN_UNDEF) {
                        plogk("PCREL_LO12: invalid st_shndx %d for symbol %s\n", sym->st_shndx, sym_name);
                        break;
                    }
                    uintptr_t auipc_addr = (uintptr_t)base + shdr[sym->st_shndx].sh_offset + sym->st_value;

                    uintptr_t target = 0;
                    int found = 0;
                    for (int k = 0; k < pair_count; k++) {
                        if (pairs[k].auipc_addr == auipc_addr) {
                            target = pairs[k].target;
                            found = 1;
                            break;
                        }
                    }
                    if (!found) {
                        plogk("PCREL_LO12: no matching HI20 for auipc at %p\n", (void*)auipc_addr);
                        break;
                    }
                    int32_t lo = (int32_t)(target - auipc_addr) & 0xFFF;
                    insn = *(uint32_t *)loc;
                    if (type == R_RISCV_PCREL_LO12_I)
                        insn = insn_set_imm_i(insn, lo);
                    else
                        insn = insn_set_imm_s(insn, lo);
                    *(uint32_t *)loc = insn;
                    break;
                }

                /* ---- HI20/LO12 绝对寻址 ---- */
                case R_RISCV_HI20: {
                    uintptr_t target = sym_addr + addend;
                    int32_t hi = (int32_t)((target + 0x800) >> 12);
                    insn = *(uint32_t *)loc;
                    insn = insn_set_imm_i(insn, hi);
                    *(uint32_t *)loc = insn;
                    break;
                }
                case R_RISCV_LO12_I:
                case R_RISCV_LO12_S: {
                    uintptr_t target = sym_addr + addend;
                    int32_t lo = (int32_t)(target & 0xFFF);
                    insn = *(uint32_t *)loc;
                    if (type == R_RISCV_LO12_I)
                        insn = insn_set_imm_i(insn, lo);
                    else
                        insn = insn_set_imm_s(insn, lo);
                    *(uint32_t *)loc = insn;
                    break;
                }

                /* ---- GOT 相关（仅做简单处理） ---- */
                case R_RISCV_GOT_HI20: {
                    int64_t delta = sym_addr + addend - (uintptr_t)loc;
                    int32_t hi = (int32_t)((delta + 0x800) >> 12);
                    insn = *(uint32_t *)loc;
                    insn = insn_set_imm_i(insn, hi);
                    *(uint32_t *)loc = insn;
                    break;
                }

                /* ---- 分支和跳转（占位） ---- */
                case R_RISCV_BRANCH:
                case R_RISCV_JAL:
                    plogk("Warning: R_RISCV_BRANCH/JAL not fully handled at %p (sym=%s)\n", loc, sym_name);
                    break;

                /* ---- 忽略的标记类型 ---- */
                case R_RISCV_ALIGN:
                case R_RISCV_RELAX:
                    break;

                /* ---- 32位PC相对 ---- */
                case R_RISCV_32_PCREL:
                    value = (uint32_t)(sym_addr + addend - (uintptr_t)loc);
                    *(uint32_t *)loc = (uint32_t)value;
                    break;

                default:
                    plogk("Unsupported RISC-V relocation type %d at %p (sym=%s)\n", type, loc, sym_name);
                    break;
            }
        }
    }

    plogk_info_ptr--;
}