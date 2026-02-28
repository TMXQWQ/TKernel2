// #include "elf.h"
// #include "elf_parse.h"
// #include "printk.h"

// /**
//  * 对指定重定位节应用重定位
//  * @param base        ELF 文件基地址
//  * @param rel_shdr    重定位节头（.rela.text 等）
//  * @param symtab      符号表内容指针
//  * @param strtab      字符串表内容指针
//  * @param symbol_values 预先计算好的符号最终地址数组（与符号表索引对应）
//  */
// void relocate_section(Elf64_Ehdr *base, Elf64_Shdr *rel_shdr, Elf64_Sym *symtab, char *strtab, Elf64_Addr *symbol_values)
// {
//     // 获取目标节头及其内容
//     Elf64_Shdr *target_shdr    = get_target_section(rel_shdr, get_section_headers(base));
//     char       *target_content = (char *)base + target_shdr->sh_offset; // 目标节内容

//     // 重定位条目数量
//     int         rel_count = rel_shdr->sh_size / rel_shdr->sh_entsize;
//     Elf64_Rela *rela      = (Elf64_Rela *)((char *)base + rel_shdr->sh_offset);

//     for (int i = 0; i < rel_count; i++) {
//         // 提取符号索引和重定位类型
//         int sym_idx  = ELF64_R_SYM(rela[i].r_info);
//         int rel_type = ELF64_R_TYPE(rela[i].r_info);

//         // 符号的最终地址
//         Elf64_Addr sym_addr = symbol_values[sym_idx];

//         // 需要修正的位置在目标节内的偏移
//         Elf64_Off offset = rela[i].r_offset;
//         // 该位置在最终内存中的地址（假设目标节起始地址为 target_shdr->sh_addr）
//         Elf64_Addr place = target_shdr->sh_addr + offset;
//         // 加数
//         Elf64_Sxword addend = rela[i].r_addend;

//         // 根据类型计算修正值
//         Elf64_Addr value = 0;
//         switch (rel_type) {
//             case R_X86_64_64 : // S + A
//                 value                                    = sym_addr + addend;
//                 *(Elf64_Addr *)(target_content + offset) = value;
//                 break;
//             case R_X86_64_PC32 : // S + A - P
//                 value                                    = sym_addr + addend - place;
//                 *(Elf32_Word *)(target_content + offset) = (Elf32_Word)value;
//                 break;
//             case R_X86_64_32 : // S + A (32位，截断)
//                 value                                    = sym_addr + addend;
//                 *(Elf32_Word *)(target_content + offset) = (Elf32_Word)value;
//                 break;
//             case R_X86_64_GOTPC64 :
//             case R_X86_64_GOTOFF64 :
//             case R_X86_64_PLTOFF64 :
//             // 可根据需要添加更多类型
//             default :
//                 printk("Unsupported relocation type %d\n", rel_type);
//                 break;
//         }
//     }
// }