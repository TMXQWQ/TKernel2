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
                plogk(" Load module %s.\n", mod->name);
            }
    }
    return 0;
}

/* 模块 ELF 目标架构名（仅用于日志） */
static const char *module_arch_name(int machine)
{
    switch (machine) {
        case EM_X86_64 : return "x86_64";
        case EM_RISCV : return "RISC-V";
        case EM_LOONGARCH : return "loongarch64";
        default : return "Unknown";
    }
}

/* 判断模块目标架构是否受支持。
 * 说明：riscv64 的 linker 重定位已就绪，但模块加载框架尚未适配，
 * 因此模块暂时先不支持 riscv。 */
static int module_arch_supported(int machine)
{
#ifdef __x86_64__
    if (machine == EM_X86_64) return 1;
#endif
#ifdef __loongarch64
    if (machine == EM_LOONGARCH) return 1;
#endif
    return 0;
}

/* 为 NOBITS(.bss) 节分配零块：优先用 kpi->kmalloc（UxTK 初始化后可用），
 * 不可用时回退到内核静态兜底池（仅早期模块如 UxTK 在无 heap 时使用）。
 * 分配的 .bss 在模块生命周期内持续存在，不可释放。 */
#ifndef MOD_BSS_POOL_SZ
#define MOD_BSS_POOL_SZ (2 * 1024 * 1024)
#endif
static uint8_t g_mod_bss_pool[MOD_BSS_POOL_SZ] __attribute__((aligned(16)));
static size_t  g_mod_bss_pool_used = 0;

static void *mod_bss_alloc(size_t sz)
{
    if (kinfo.kpi.kmalloc) return kinfo.kpi.kmalloc(sz);
    size_t aligned = (sz + 15) & ~(size_t)15;
    if (g_mod_bss_pool_used + aligned > MOD_BSS_POOL_SZ) return NULL;
    void *p = &g_mod_bss_pool[g_mod_bss_pool_used];
    g_mod_bss_pool_used += aligned;
    return p;
}

module_info *load_mod(Elf64_Ehdr *base_addr)
{
    plogk_info_stack[++plogk_info_ptr] = "Module";
    /* 目标架构判断：只接受内核同架构、且模块框架已支持的 ELF。 */
    if (!module_arch_supported(base_addr->e_machine)) {
        plogk(" Unsupport platform: %s.\n", module_arch_name(base_addr->e_machine));
        plogk_info_ptr--;
        return NULL;
    }

    // 为 NOBITS(.bss) 节分配独立零块并重定向 sh_offset。
    // 原因：模块镜像直接使用 cpio 文件缓冲区，而 .bss 无文件内容、其 sh_offset
    // 往往与 .symtab/.strtab/.rela 文件区重叠。若仍按 `memset(base+sh_offset,0,sh_size)`
    // 在原地清零，范围会越界覆盖符号表，导致 get_symbol_address 找不到 _start
    // -> enter==0 -> 取指 0x0 -> PAGE_FAULT（IPC 等“小体量大 .bss”模块必崩）。
    // 修复：用 kpi->kmalloc(优先) 或内核静态兜底池分配零块，把 sh_offset 重定向为
    // (buf - base)，使 elf_parse/linker 中 `base + sh_offset` 寻址自动落到新块，
    // 符号表区域不再被触碰。无需复制整个模块，也无需改动 elf_parse/linker。
    {
        Elf64_Shdr *shdr = (Elf64_Shdr *)((char *)base_addr + base_addr->e_shoff);
        for (uint32_t i = 0; i < base_addr->e_shnum; i++) {
            if (shdr[i].sh_type != SHT_NOBITS) continue;
            if (shdr[i].sh_size == 0) continue;
            void *bss = mod_bss_alloc(shdr[i].sh_size);
            if (!bss) {
                plogk(" Failed to allocate .bss (%u bytes) for module\n", shdr[i].sh_size);
                plogk_info_ptr--;
                return NULL;
            }
            memset(bss, 0, shdr[i].sh_size);
            // 重定向：让 base + sh_offset 解析到新分配的零块（64 位回绕亦正确）
            shdr[i].sh_offset = (Elf64_Off)((uintptr_t)bss - (uintptr_t)base_addr);
        }
    }
    // 第一阶段：重定位内核符号和内部导出符号（mod == NULL）
    if (elf_relocate_module(base_addr, NULL) != 0) {
        plogk("Phase-1 relocation failed\n");
        plogk_info_ptr--;
        return NULL;
    }

    // 提前获取模块入口和 module_info
    // NOLINTNEXTLINE(performance-no-int-to-ptr) : recovering a function pointer from the ELF entry address
    mod_enter    enter = (mod_enter)(intptr_t)elf_pie_enter_parse(base_addr);
    module_info *mod   = enter(&kinfo);
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
            const char  *dep_name = mod->dependencies[i];
            module_info *dep      = find_module(dep_name);
            if (dep) {
                dep->refcount++;
            } else {
                plogk("Warning: Dependency '%s' not found for module '%s'\n", dep_name, mod->name);
            }
        }
    }

    // 2. 统一重新重定位外部符号（mod != NULL）
    if (elf_relocate_module(base_addr, mod) != 0) {
        plogk("Phase-2 relocation failed\n");
        plogk_info_ptr--;
        return NULL;
    }

    // 3. 若是 KPI 模块，注册其导出符号（供后续模块使用）
    if (mod->version == KPI_VERSION) { kpi_register_module(mod); }

    // 5. 调用模块初始化
    if (mod->init) { plogk("Module init returned: %d\n", mod->init()); }
    mod->state = 2;

    plogk_info_ptr--;
    return mod; // 原代码返回 0，改为返回 mod 指针
}