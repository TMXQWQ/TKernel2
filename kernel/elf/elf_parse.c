#include "elf_parse.h"
#include "stddef.h"
#include "stdint.h"
#include "string.h"
#include <elf.h>
#include "printk.h"

Elf64_Shdr* get_section_headers(Elf64_Ehdr *base) {
    return (Elf64_Shdr *)((char *)base + base->e_shoff);
}

int get_symbol_count(Elf64_Shdr *symtab_hdr) {
    return symtab_hdr->sh_size / symtab_hdr->sh_entsize;
}

char* get_symbol_name(Elf64_Sym *symtab, char *strtab, int idx) {
    return strtab + symtab[idx].st_name;
}

Elf64_Shdr* get_target_section(Elf64_Shdr *rel_hdr, Elf64_Shdr *shdr) {
    return &shdr[rel_hdr->sh_info];
}

uint64_t get_symbol_address(void *base, const char *name) {
    Elf64_Ehdr *ehdr = (Elf64_Ehdr *)base;
    Elf64_Shdr *shdr = (Elf64_Shdr *)(base + ehdr->e_shoff);
    int shnum = ehdr->e_shnum;

    // 查找符号表
    Elf64_Shdr *symtab_hdr = NULL;
    char *strtab = NULL;
    for (int i = 0; i < shnum; i++) {
        if (shdr[i].sh_type == SHT_SYMTAB) {
            symtab_hdr = &shdr[i];
            Elf64_Shdr *strtab_hdr = &shdr[symtab_hdr->sh_link];
            strtab = (char *)base + strtab_hdr->sh_offset;
            break;
        }
    }
    if (!symtab_hdr) return 0;

    Elf64_Sym *sym = (Elf64_Sym *)(base + symtab_hdr->sh_offset);
    int symcount = symtab_hdr->sh_size / sizeof(Elf64_Sym);

    for (int i = 0; i < symcount; i++) {
        if (strcmp(strtab + sym[i].st_name, name) == 0) {
            uint16_t shndx = sym[i].st_shndx;
            if (shndx != SHN_UNDEF && shndx < SHN_LORESERVE && shndx < shnum) {
                // 使用 sh_offset 计算节的运行时基址，而不是依赖 sh_addr
                uint64_t sec_base = (uint64_t)base + shdr[shndx].sh_offset;
                uint64_t addr = sec_base + sym[i].st_value;
                printk("Symbol %s: shndx=%d, sh_offset=%lx, st_value=%lx, base=%lx, addr=%lx\n",
                       name, shndx, shdr[shndx].sh_offset, sym[i].st_value, base, addr);
                return sec_base + sym[i].st_value;
            } else {
                // 处理 ABS 等特殊节
                uint64_t sec_base = (uint64_t)base + shdr[shndx].sh_offset; // 使用 sh_offset
                uint64_t addr = sec_base + sym[i].st_value;
                printk("Symbol %s: shndx=%d, sh_offset=%lx, st_value=%lx, base=%lx, addr=%lx\n",
                       name, shndx, shdr[shndx].sh_offset, sym[i].st_value, base, addr);
                return sym[i].st_value;
            }
        }
    }
    return 0;
}

enter elf_pie_enter_parse(Elf64_Ehdr *base)
{
    // return (enter)(((uintptr_t)(base->e_entry) + (uintptr_t)elf_get_section(base, ".text")));
    return (enter)(uintptr_t)get_symbol_address(base,"_start");
}

void *elf_get_section(Elf64_Ehdr *base, char *name)
{
    if (!base || !name) return NULL;

    // 1. 定位节头表和程序头表
    Elf64_Shdr *shdr = (Elf64_Shdr *)((char *)base + base->e_shoff);
    Elf64_Phdr *phdr = (Elf64_Phdr *)((char *)base + base->e_phoff);

    // 2. 获取节头字符串表
    if (base->e_shstrndx == SHN_UNDEF) return NULL; // 无节名表
    Elf64_Shdr *shstr_shdr = &shdr[base->e_shstrndx];
    char       *shstrtab   = (char *)base + shstr_shdr->sh_offset;

    // 3. 遍历所有节头，查找目标节
    for (int i = 0; i < base->e_shnum; i++) {
        char *sec_name = shstrtab + shdr[i].sh_name;
        if (strcmp(sec_name, name) != 0) continue;

        // 找到目标节，记录其虚拟地址和文件偏移
        Elf64_Addr target_vaddr  = shdr[i].sh_addr;
        Elf64_Off  target_offset = shdr[i].sh_offset;

        // 4. 遍历程序头，查找包含该虚拟地址的 PT_LOAD 段
        for (int j = 0; j < base->e_phnum; j++) {
            if (phdr[j].p_type != PT_LOAD) continue;

            Elf64_Addr seg_start = phdr[j].p_vaddr;
            Elf64_Addr seg_end   = seg_start + phdr[j].p_memsz;
            if (target_vaddr >= seg_start && target_vaddr < seg_end) {
                // 该段在内存中的实际地址 = base + p_offset
                char *seg_base = (char *)base + phdr[j].p_offset;
                // 节的实际地址 = 段基址 + (sh_addr - p_vaddr)
                return seg_base + (target_vaddr - seg_start);
            }
        }
        return (char *)base + target_offset;
    }

    return NULL; // 找不到喵
}
