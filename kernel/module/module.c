#include "cpio.h"
#include "elf_parse.h"
#include "kernel.h"
#include "kpi.h"
#include "printk.h"
#include "stddef.h"
#include "stdint.h"
#include "string.h"
#include "tkm.h"
#include <elf.h>

int init_mod()
{
    for (size_t i = 0; i < ncfs.size; i++) {
        char *tmp = ncfs.file_list[i].name;
        for (int j = 0; tmp[j] != '\0'; j++)
            if (tmp[j] == '.' && tmp[j + 1] != '\0' && tmp[j + 2] != '\0' && tmp[j + 3] != '\0' && tmp[j + 1] == 't' && tmp[j + 2] == 'k'
                && tmp[j + 3] == 'm') {
                // test             = elf_pie_enter_parse((Elf64_Ehdr *)ncfs.file_list[i].data_ptr);
                // NOLINTNEXTLINE(performance-no-int-to-ptr) : module image address stored as uintptr_t
                module_info *mod = load_mod((Elf64_Ehdr *)ncfs.file_list[i].data_ptr);
                (void)mod;
                // printk(ansi_V("[ Module ]") " Load module %s.\n", mod->name);
            }
    }
    return 0;
}

module_info *load_mod(Elf64_Ehdr *base_addr)
{
    plogk_info_stack[++plogk_info_ptr] = "Module";
    int machine = base_addr->e_machine;
    switch (machine) {
#ifdef __x86_64__
        case EM_X86_64 :
            break;
#endif
#ifdef __riscv
        case EM_RISCV :
            break;
#endif
#ifdef __loongarch64
        case EM_LOONGARCH :
            break;
#endif
        default :
            printk(ansi_V("[ Module ]") " Unsupport platform: %s.\n", machine == EM_X86_64    ? "x86_64" :
                                                                      machine == EM_RISCV     ? "RISC-V" :
                                                                      machine == EM_LOONGARCH ? "loongarch64" :
                                                                                                "Unknown");
            plogk_info_ptr--;
            return NULL;
    }
    
    // 第一阶段：重定位内核符号和内部导出符号（mod == NULL）
    elf_relocate_module(base_addr, NULL);

    // 提前获取模块入口和 module_info
    // NOLINTNEXTLINE(performance-no-int-to-ptr) : recovering a function pointer from the ELF entry address
    mod_enter test = (mod_enter)(intptr_t)elf_pie_enter_parse(base_addr);
    module_info *mod = test(&kinfo);
    if (!mod) {
        plogk("Failed to get module info\n");
        plogk_info_ptr--;
        return NULL;
    }
    plogk(" Load module %s.\n", mod->name);

    // 第二阶段：处理依赖和注册导出符号，然后统一重定位外部符号（mod != NULL）
    if (mod->version == KPI_VERSION) {
        // 1. 先处理所有依赖模块的引用计数
        for (uint32_t i = 0; i < mod->dep_count; i++) {
            const char *dep_name = mod->dependencies[i];
            module_info *dep = find_module(dep_name);
            if (dep) {
                dep->refcount++;
            } else {
                plogk("Warning: Dependency '%s' not found for module '%s'\n", dep_name, mod->name);
            }
        }
    }

    // 2. 统一重新重定位外部符号（mod != NULL）
    elf_relocate_module(base_addr, mod);

    // 3. 若是 KPI 模块，注册其导出符号（供后续模块使用）
    if (mod->version == KPI_VERSION) { kpi_register_module(mod); }

    // 4. 清零 NOBITS（.bss）段：模块镜像直接使用 cpio 文件缓冲区，
    //     而 .bss 无文件内容（sh_offset 与 .rela.text 重叠），必须手动清零，
    //     否则模块内静态变量（如自旋锁 {0,0}）会读到垃圾值导致死锁。
    {
        Elf64_Shdr *shdr = (Elf64_Shdr *)((char *)base_addr + base_addr->e_shoff);
        for (uint32_t i = 0; i < base_addr->e_shnum; i++) {
            if (shdr[i].sh_type == SHT_NOBITS) {
                memset((char *)base_addr + shdr[i].sh_offset, 0, shdr[i].sh_size);
            }
        }
    }

    // 5. 调用模块初始化
    if (mod->init) { plogk("Module init returned: %d\n", mod->init()); }
    mod->state = 2;

    plogk_info_ptr--;
    return mod; // 原代码返回 0，改为返回 mod 指针
}