// #include "elf_parse.h"
#include "kernel.h"
#include "kpi.h"
#include "printk.h"
#include "tkm.h"
#include <elf.h>
#include <stdint.h>
#include <string.h>

// 最大 PCREL_HI20 重定位条目数（可调整）
#define MAX_PCREL_PAIRS 8192

// 辅助函数：I类指令立即数编码
static inline uint32_t insn_set_imm_i(uint32_t insn, int32_t imm) {
    uint32_t uimm = (uint32_t)(imm & 0xFFF);
    return (insn & ~(0xFFFUL << 20)) | (uimm << 20);
}

// 辅助函数：S类指令立即数编码
static inline uint32_t insn_set_imm_s(uint32_t insn, int32_t imm) {
    uint32_t uimm = (uint32_t)(imm & 0xFFF);
    uint32_t imm11_5 = (uimm >> 5) & 0x7F;
    uint32_t imm4_0  = uimm & 0x1F;
    return (insn & ~(0x7FUL << 25)) | (imm11_5 << 25)
         | (insn & ~(0x1FUL << 7))  | (imm4_0 << 7);
}

// 主函数：返回 0 成功，-1 失败
int elf_relocate_module(void *base, module_info *mod)
{
    plogk_info_stack[++plogk_info_ptr] = "RELOC";
    if (!base) {
        plogk("base is NULL, skipping relocation\n");
        plogk_info_ptr--;
        return -1;
    }
    plogk("base=%p, mod=%p\n", base, mod);

    Elf64_Ehdr *ehdr = (Elf64_Ehdr *)base;
    Elf64_Shdr *shdr = (Elf64_Shdr *)((char *)base + ehdr->e_shoff);

    /* 定位符号表和字符串表 */
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
    if (!symtab) {
        plogk("No symbol table found\n");
        plogk_info_ptr--;
        return -1;
    }

    // 定义 PCREL 配对结构
    struct pcrel_pair {
        uintptr_t auipc_addr;
        uintptr_t target;       // 未解析时为 0
        uint32_t  sym_idx;      // 用于调试
        int64_t   addend;
    } pairs[MAX_PCREL_PAIRS];
    int pair_count = 0;

    #define GET_SEC_BASE(idx) \
        ((idx < ehdr->e_shnum) ? \
            ((ehdr->e_type == ET_REL) ? ((uintptr_t)base + shdr[idx].sh_offset) : ((uintptr_t)base + shdr[idx].sh_addr)) : 0)

    /* 第一遍：收集所有 PCREL_HI20（无论符号是否解析） */
    for (int i = 0; i < ehdr->e_shnum; i++) {
        if (shdr[i].sh_type != SHT_RELA) continue;
        Elf64_Shdr *rela_hdr = &shdr[i];
        Elf64_Rela *rela = (Elf64_Rela *)((char *)base + rela_hdr->sh_offset);
        int count = rela_hdr->sh_size / rela_hdr->sh_entsize;
        uint32_t target_sec = rela_hdr->sh_info;
        if (target_sec >= ehdr->e_shnum) {
            plogk("First pass: invalid target_sec %u, skipping\n", target_sec);
            continue;
        }
        uintptr_t target_base = GET_SEC_BASE(target_sec);
        if (!target_base) continue;

        for (int j = 0; j < count; j++) {
            uint32_t type = ELF64_R_TYPE(rela[j].r_info);
            if (type == R_RISCV_PCREL_HI20) {
                uint32_t sym_idx = ELF64_R_SYM(rela[j].r_info);
                int64_t addend = rela[j].r_addend;
                uintptr_t sym_addr = 0;

                // 尝试解析符号（与第二遍逻辑一致）
                if (sym_idx < (uint32_t)nsyms && sym_idx > 0) {
                    Elf64_Sym *sym = &symtab[sym_idx];
                    const char *name = strtab + sym->st_name;
                    if (sym->st_shndx < ehdr->e_shnum && sym->st_shndx != SHN_UNDEF) {
                        uintptr_t sec_base = GET_SEC_BASE(sym->st_shndx);
                        if (sec_base) sym_addr = sec_base + sym->st_value;
                    } else if (sym->st_shndx == SHN_UNDEF) {
                        // 外部符号：第二阶段优先用 KPI 解析模块间导出符号
                        if (mod && mod->version == KPI_VERSION)
                            sym_addr = kpi_resolve_symbol(mod, name);
                        // KPI 解析不到或第一阶段（mod==NULL）时，回退到内核符号表
                        if (!sym_addr) {
                            for (ksym *p = ksym_table_start; p < ksym_table_end; p++)
                                if (!strcmp(p->name, name)) {
                                    sym_addr = kinfo.bootinfo.kernel_base_addr + p->offset;
                                    break;
                                }
                        }
                    } else if (sym->st_shndx == SHN_ABS) {
                        sym_addr = sym->st_value;   // 绝对值
                    }
                }

                uintptr_t auipc_loc = target_base + rela[j].r_offset;
                if (pair_count >= MAX_PCREL_PAIRS) {
                    plogk("Too many PCREL_HI20 entries (max %d)\n", MAX_PCREL_PAIRS);
                    plogk_info_ptr--;
                    return -1;
                }
                pairs[pair_count].auipc_addr = auipc_loc;
                pairs[pair_count].target = sym_addr ? (sym_addr + addend) : 0;
                pairs[pair_count].sym_idx = sym_idx;
                pairs[pair_count].addend = addend;
                pair_count++;
            }
        }
    }

    /* 第二遍：执行重定位 */
    int ret = 0;
    for (int i = 0; i < ehdr->e_shnum; i++) {
        if (shdr[i].sh_type != SHT_RELA) continue;
        Elf64_Shdr *rela_hdr = &shdr[i];
        Elf64_Rela *rela = (Elf64_Rela *)((char *)base + rela_hdr->sh_offset);
        int count = rela_hdr->sh_size / rela_hdr->sh_entsize;
        uint32_t target_sec = rela_hdr->sh_info;
        if (target_sec >= ehdr->e_shnum) {
            plogk("Second pass: invalid target_sec %u, skipping section\n", target_sec);
            continue;
        }
        uintptr_t target_base = GET_SEC_BASE(target_sec);
        if (!target_base) {
            plogk("Second pass: target_base=0 for sec %u, skipping\n", target_sec);
            continue;
        }

        for (int j = 0; j < count; j++) {
            uint32_t type = ELF64_R_TYPE(rela[j].r_info);
            uint32_t sym_idx = ELF64_R_SYM(rela[j].r_info);
            int64_t addend = rela[j].r_addend;
            uintptr_t *loc_ptr = (uintptr_t *)(target_base + rela[j].r_offset);
            uint32_t *loc32 = (uint32_t *)loc_ptr;
            uint64_t *loc64 = (uint64_t *)loc_ptr;

            if (type == R_RISCV_ALIGN || type == R_RISCV_RELAX)
                continue;
            if (type == R_RISCV_RELATIVE) {
                *loc64 = (uintptr_t)base + addend;
                continue;
            }

            if (sym_idx == 0) continue;
            if (sym_idx >= (uint32_t)nsyms) {
                plogk("Invalid sym_idx %u\n", sym_idx);
                ret = -1; goto out;
            }

            Elf64_Sym *sym = &symtab[sym_idx];
            const char *sym_name = strtab + sym->st_name;
            uintptr_t sym_addr = 0;

            /* 符号地址解析 */
            if (sym->st_shndx < ehdr->e_shnum && sym->st_shndx != SHN_UNDEF) {
                uintptr_t sec_base = GET_SEC_BASE(sym->st_shndx);
                if (!sec_base) {
                    plogk("Invalid section %d for symbol %s\n", sym->st_shndx, sym_name);
                    ret = -1; goto out;
                }
                sym_addr = sec_base + sym->st_value;
            } else if (sym->st_shndx == SHN_UNDEF) {
                // 外部符号：第二阶段优先用 KPI 解析模块间导出符号
                if (mod && mod->version == KPI_VERSION)
                    sym_addr = kpi_resolve_symbol(mod, sym_name);
                // KPI 解析不到或第一阶段（mod==NULL）时，回退到内核符号表
                if (!sym_addr) {
                    for (ksym *p = ksym_table_start; p < ksym_table_end; p++)
                        if (!strcmp(p->name, sym_name)) {
                            sym_addr = kinfo.bootinfo.kernel_base_addr + p->offset;
                            break;
                        }
                }
                if (!sym_addr) {
                    plogk("Failed to resolve external symbol '%s'\n", sym_name);
                    ret = -1; goto out;
                }
            } else if (sym->st_shndx == SHN_ABS) {
                sym_addr = sym->st_value;
            } else {
                plogk("Invalid st_shndx %d for %s\n", sym->st_shndx, sym_name);
                ret = -1; goto out;
            }

            uint32_t insn;
            uintptr_t value;

            switch (type) {
                case R_RISCV_64:
                    value = sym_addr + addend;
                    *loc64 = value;
                    break;
                case R_RISCV_32:
                    *loc32 = (uint32_t)(sym_addr + addend);
                    break;
                case R_RISCV_CALL:
                case R_RISCV_CALL_PLT: {
                    uint32_t *auipc = loc32;
                    uint32_t *jalr = loc32 + 1;
                    if ((*jalr & 0x7f) != 0x67) break;
                    int rd = (*auipc >> 7) & 0x1f;
                    int rs1 = (*jalr >> 15) & 0x1f;
                    if (rd != rs1) break;
                    int64_t delta = sym_addr + addend - (uintptr_t)auipc;
                    int32_t hi = (int32_t)((delta + 0x800) >> 12);
                    int32_t lo = (int32_t)(delta & 0xfff);
                    if (lo & 0x800) hi++;
                    *auipc = (*auipc & ~(0xfffffUL << 12)) | ((hi & 0xfffff) << 12);
                    *jalr  = (*jalr  & ~(0xfffUL << 20)) | ((lo & 0xfff) << 20);
                    break;
                }
                case R_RISCV_PCREL_HI20: {
                    int64_t delta = sym_addr + addend - (uintptr_t)loc32;
                    int32_t hi = (int32_t)((delta + 0x800) >> 12);
                    insn = *loc32;
                    insn = (insn & ~(0xfffffUL << 12)) | ((hi & 0xfffff) << 12);
                    *loc32 = insn;
                    break;
                }
                case R_RISCV_PCREL_LO12_I:
                case R_RISCV_PCREL_LO12_S: {
                    if (sym->st_shndx >= ehdr->e_shnum || sym->st_shndx == SHN_UNDEF) {
                        plogk("LO12 symbol invalid section\n");
                        ret = -1; goto out;
                    }
                    uintptr_t auipc_addr = GET_SEC_BASE(sym->st_shndx) + sym->st_value;
                    uintptr_t target = 0;
                    for (int k = 0; k < pair_count; k++) {
                        if (pairs[k].auipc_addr == auipc_addr) {
                            target = pairs[k].target;
                            break;
                        }
                    }
                    if (!target) {
                        plogk("LO12 cannot find matching HI20 or target unresolved for auipc=%p\n", (void*)auipc_addr);
                        ret = -1; goto out;
                    }
                    int32_t lo = (int32_t)(target - auipc_addr) & 0xFFF;
                    insn = *loc32;
                    if (type == R_RISCV_PCREL_LO12_I)
                        insn = insn_set_imm_i(insn, lo);
                    else
                        insn = insn_set_imm_s(insn, lo);
                    *loc32 = insn;
                    break;
                }
                case R_RISCV_HI20: {
                    uintptr_t target = sym_addr + addend;
                    int32_t hi = (int32_t)((target + 0x800) >> 12);
                    insn = *loc32;
                    insn = insn_set_imm_i(insn, hi);
                    *loc32 = insn;
                    break;
                }
                case R_RISCV_LO12_I:
                case R_RISCV_LO12_S: {
                    uintptr_t target = sym_addr + addend;
                    int32_t lo = (int32_t)(target & 0xFFF);
                    insn = *loc32;
                    if (type == R_RISCV_LO12_I)
                        insn = insn_set_imm_i(insn, lo);
                    else
                        insn = insn_set_imm_s(insn, lo);
                    *loc32 = insn;
                    break;
                }
                case R_RISCV_GOT_HI20: {
                    // 简化处理：按绝对地址的HI20 (实际应填入GOT表偏移，此处仅为兼容)
                    uintptr_t target = sym_addr + addend;
                    int32_t hi = (int32_t)((target + 0x800) >> 12);
                    insn = *loc32;
                    insn = (insn & ~(0xfffffUL << 12)) | ((hi & 0xfffff) << 12);
                    *loc32 = insn;
                    break;
                }
                case R_RISCV_BRANCH: {
                    int64_t delta = sym_addr + addend - (uintptr_t)loc32;
                    if (delta < -0x100000 || delta >= 0x100000) {
                        plogk("Branch offset out of range\n");
                        ret = -1; goto out;
                    }
                    uint32_t imm = (uint32_t)(delta & 0x1FFFFF);
                    uint32_t imm12 = (imm >> 12) & 1;
                    uint32_t imm11 = (imm >> 11) & 1;
                    uint32_t imm10_5 = (imm >> 5) & 0x3F;
                    uint32_t imm4_1 = (imm >> 1) & 0xF;
                    insn = *loc32;
                    insn = (insn & ~(0x1UL << 31)) | (imm12 << 31);
                    insn = (insn & ~(0x1UL << 7))  | (imm11 << 7);
                    insn = (insn & ~(0x3FUL << 25)) | (imm10_5 << 25);
                    insn = (insn & ~(0xFUL << 8))  | (imm4_1 << 8);
                    *loc32 = insn;
                    break;
                }
                case R_RISCV_JAL: {
                    int64_t delta = sym_addr + addend - (uintptr_t)loc32;
                    if (delta < -0x100000 || delta >= 0x100000) {
                        plogk("JAL offset out of range\n");
                        ret = -1; goto out;
                    }
                    uint32_t imm = (uint32_t)(delta & 0x1FFFFF);
                    uint32_t imm20 = (imm >> 20) & 1;
                    uint32_t imm19_12 = (imm >> 12) & 0xFF;
                    uint32_t imm11 = (imm >> 11) & 1;
                    uint32_t imm10_1 = (imm >> 1) & 0x3FF;
                    insn = *loc32;
                    insn = (insn & ~(0x1UL << 31)) | (imm20 << 31);
                    insn = (insn & ~(0xFFUL << 12)) | (imm19_12 << 12);
                    insn = (insn & ~(0x1UL << 20)) | (imm11 << 20);
                    insn = (insn & ~(0x3FFUL << 21)) | (imm10_1 << 21);
                    *loc32 = insn;
                    break;
                }
                case R_RISCV_32_PCREL:
                    *loc32 = (uint32_t)(sym_addr + addend - (uintptr_t)loc32);
                    break;
                default:
                    plogk("Unsupported relocation type %d at %p\n", type, loc32);
                    ret = -1; goto out;
            }
        }
    }

out:
    plogk_info_ptr--;
    return ret;
}