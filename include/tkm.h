#ifndef TKM_H
#define TKM_H
#include "elf.h"
#include "kernel.h"
#include <stdint.h>

// 在现有结构体中扩展，不破坏兼容性
typedef struct module_info {
        char *name;
        uintptr_t (*init)();

        // === KPI 扩展字段 ===
        uint32_t               version;      // 设为 KPI_VERSION 启用新功能
        struct kpi_export_sym *export_table; // 导出符号数组
        uint32_t               export_count; // 导出符号数量
        char                 **dependencies; // 依赖的模块名列表（NULL结尾）
        uint32_t               dep_count;    // 依赖数量

        // 运行时状态（加载器维护，模块无需初始化）
        uint32_t state;    // 0=未加载 1=已注册 2=已初始化
        uint32_t refcount; // 被其他模块引用计数
} module_info;

// 新增：单个导出符号描述
typedef struct kpi_export_sym {
        char     *name;
        uintptr_t value;
        uint32_t  type;
} kpi_export_sym;

// 新增：KPI 版本号
#define KPI_VERSION 1

typedef module_info *(*mod_enter)(kernel_info *);

int init_mod();

module_info *load_mod(Elf64_Ehdr *);

#endif
