#include "kpi.h"
#include "kernel.h"
#include "printk.h"
#include "string.h"
#include "tkm.h"

static module_info *loaded_modules[MAX_MODULES];
static uint32_t     loaded_count = 0;

// 按名称查找已加载模块
module_info *find_module(const char *name)
{
    for (uint32_t i = 0; i < loaded_count; i++) {
        if (strcmp(loaded_modules[i]->name, name) == 0) return loaded_modules[i];
    }
    return NULL;
}

// 内核符号查询（原有逻辑）
uintptr_t kernel_symbol(const char *name)
{
    for (ksym *p = _symbol_table_start; p < _symbol_table_end; p++) {
        if (strcmp(p->name, name) == 0) return kinfo.bootinfo.kernel_base_addr + p->offset;
    }
    return 0;
}

// KPI 符号解析（仅依赖模块 + 内核）
uintptr_t kpi_resolve_symbol(module_info *mod, const char *name)
{
    if (!mod || !name) return 0;

    // 1. 在直接依赖的模块中查找
    for (uint32_t i = 0; i < mod->dep_count; i++) {
        module_info *dep = find_module(mod->dependencies[i]);
        if (!dep) continue;
        for (uint32_t j = 0; j < dep->export_count; j++) {
            if (strcmp(dep->export_table[j].name, name) == 0) return dep->export_table[j].value;
        }
    }

    // 2. 回退到内核符号表
    return kernel_symbol(name);
}

void kpi_register_module(module_info *mod)
{
    plogk_info_stack[++plogk_info_ptr] = "KPI";
    if (find_module(mod->name)) {
        plogk("Module '%s' already registered\n", mod->name);
        return;
    }
    if (loaded_count >= MAX_MODULES) {
        plogk("Too many modules, cannot register %s\n", mod->name);
        return;
    }
    loaded_modules[loaded_count++] = mod;
    mod->state                     = 1;
    plogk("Module '%s' registered with %u exports\n", mod->name, mod->export_count);
    plogk_info_ptr--;
}