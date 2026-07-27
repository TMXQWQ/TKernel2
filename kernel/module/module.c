#include "cpio.h"
#include "elf_parse.h"
#include "kernel.h"
#include "kpi.h"
#include "printk.h"
#include "stddef.h"
#include "stdint.h"
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
    elf_relocate_module(base_addr, NULL);
    // 2. 提前获取模块入口和 module_info
    mod_enter    test = (mod_enter)(intptr_t)elf_pie_enter_parse(base_addr);
    module_info *mod  = test(&kinfo);
    if (!mod) {
        plogk("Failed to get module info\n");
        plogk_info_ptr--;
        return NULL;
    }
    plogk(" Load module %s.\n", mod->name);

    // 3. 如果是 KPI 模块，先加载所有依赖
    if (mod->version == KPI_VERSION) {
        for (uint32_t i = 0; i < mod->dep_count; i++) {
            const char  *dep_name = mod->dependencies[i];
            module_info *dep      = find_module(dep_name);
            dep->refcount++;
        }
    }
    // 4. 执行重定位（传入 mod）
    elf_relocate_module(base_addr, mod);
    
    // 5. 若是 KPI 模块，注册其导出符号
    if (mod->version == KPI_VERSION) { kpi_register_module(mod); }

    // 6. 调用模块初始化
    if (mod->init) { plogk("Module init returned: %d\n", mod->init()); }
    mod->state = 2;

    plogk_info_ptr--;
    return mod; // 原代码返回 0，改为返回 mod 指针
}