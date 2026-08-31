# TKernel2 模块开发指南

本文档介绍如何在 TKernel2 中开发和使用内核模块。

## 概述

TKernel2 支持动态加载内核模块（.tkm 文件），允许在不重新编译内核的情况下扩展功能。

## 新模块系统特性

### 1. 动态 CPIO 解析
- 移除了固定大小限制（从 128 增加到 512）
- 支持更多模块和文件
- 改进的错误处理

### 2. 模块清单系统
- 自动生成 JSON 格式的模块清单
- 记录模块名称、路径、依赖关系
- 支持版本信息

### 3. 自动依赖解析
- 基于拓扑排序算法的依赖解析
- 自动检测循环依赖
- 生成正确的加载顺序

### 4. 通用构建模板
- 简化的模块 Makefile
- 支持基本模块和高级模块
- 自动集成构建流程

## 创建新模块

### 1. 基本步骤

1. 在 `modules/` 目录下创建新目录
2. 创建 `Kconfig` 文件定义模块配置
3. 创建 `Makefile` 使用构建模板
4. 编写模块代码

### 2. 创建 Kconfig 文件

```makefile
menu "My Module"
    config MY_MODULE
        tristate "My Custom Module"
        default m
        depends on MODULES
        help
            Description of my module
endmenu
```

### 3. 创建 Makefile

#### 简单模块（使用基本模板）

```makefile
# 使用通用模块构建模板
ModuleName := my_module
ModuleType := tkm
ModuleDependencies :=
include ../../scripts/module_template.mk
```

#### 复杂模块（使用高级模板）

```makefile
# 使用高级模块构建模板
ModuleName := my_module
ModuleType := tkm
ModuleDependencies :=
ModuleLibs := $(shell find lib/ -name "lib*.a")
include ../../scripts/module_template_advanced.mk
```

### 4. 编写模块代码

模块需要实现以下函数：

```c
#include "tkm.h"

// 模块信息结构
module_info mi = {
    .name = "my_module",
    .init = module_init,
    .version = KPI_VERSION,
    .export_table = my_exports,
    .export_count = 1,
    .dependencies = NULL,
    .dep_count = 0,
};

// 模块初始化函数
uintptr_t module_init(void)
{
    // 模块初始化代码
    return 0;
}

// 模块入口函数
module_info *_start(kernel_info *ki)
{
    return (module_info *)&mi;
}
```

## 构建模块

### 1. 构建单个模块

```bash
cd modules/my_module
make all
```

### 2. 构建所有模块

```bash
cd modules
make all
```

### 3. 生成模块清单

```bash
cd modules
make gen-manifest
```

### 4. 生成加载顺序

```bash
cd modules
make sort-modules
```

## 依赖管理

### 1. 定义依赖关系

在 `Kconfig` 中使用 `depends on` 指定依赖：

```makefile
config MY_MODULE
    tristate "My Module"
    depends on MODULES
    depends on ANOTHER_MODULE
    help
        This module depends on another_module
```

### 2. 自动依赖解析

系统会自动：
- 从 `Kconfig` 解析依赖关系
- 生成模块清单（`module_manifest.json`）
- 使用拓扑排序算法确定加载顺序
- 检测循环依赖

### 3. 模块加载顺序

模块按依赖顺序加载：
1. 首先加载没有依赖的模块
2. 然后加载依赖已加载模块的模块
3. 依此类推

## 模块清单文件

### 1. 清单格式

```json
{
  "version": "1.0",
  "description": "TKernel2 模块清单 - 自动生成",
  "modules": [
    {
      "name": "test",
      "path": "modules/test",
      "tkm_file": "modules/test/test.tkm",
      "dependencies": [],
      "enabled": true
    }
  ]
}
```

### 2. 清单字段说明

| 字段 | 类型 | 描述 |
|------|------|------|
| `name` | string | 模块名称 |
| `path` | string | 模块路径 |
| `tkm_file` | string | .tkm 文件路径 |
| `dependencies` | array[]string | 依赖的模块名称 |
| `enabled` | boolean | 是否启用 |

## 调试和故障排除

### 1. 常见问题

#### 模块未加载
- 检查 `.config` 中模块是否启用
- 确认模块编译成功
- 检查依赖关系

#### 依赖冲突
- 检查 `module_manifest.json` 中的依赖关系
- 使用 `make sort-modules` 重新生成加载顺序
- 检查是否有循环依赖

#### 构建失败
- 检查工具链是否正确安装
- 确认所有依赖库存在
- 检查代码语法错误

### 2. 调试技巧

#### 启用调试日志
在 Kconfig 中启用日志：
```makefile
config KERNEL_LOG
    bool "Kernel runtime logging output"
    default y
```

#### 检查模块加载顺序
```bash
cat modules/module_list.txt
```

#### 手动构建测试
```bash
cd modules/my_module
make clean
make all
```

## 高级特性

### 1. KPI 模块

KPI（Kernel Programming Interface）模块可以导出符号供其他模块使用：

```c
// 导出符号表
kpi_export_sym my_exports[] = {
    {"my_function", (uintptr_t)my_function, STT_FUNC},
    {"my_variable", (uintptr_t)&my_variable, STT_OBJECT},
};

module_info mi = {
    // ...
    .export_table = my_exports,
    .export_count = 2,
    .version = KPI_VERSION,  // 标记为 KPI 模块
};
```

### 2. 模块卸载

虽然当前版本不支持动态卸载，但可以预留接口：

```c
// 模块卸载函数
void module_cleanup(void)
{
    // 清理资源
}
```

### 3. 版本兼容性

使用版本标记确保兼容性：

```c
#define MODULE_VERSION "1.0.0"

module_info mi = {
    // ...
    .version = KPI_VERSION,
    .module_version = MODULE_VERSION,
};
```

## 最佳实践

### 1. 模块设计

- 保持模块功能单一
- 避免循环依赖
- 使用 KPI 接口而不是直接访问内核数据
- 正确处理错误情况

### 2. 代码组织

- 将相关代码放在同一个目录
- 使用清晰的文件命名
- 添加适当的注释
- 遵循代码风格指南

### 3. 测试

- 编写单元测试
- 测试模块加载和卸载
- 测试依赖关系
- 测试错误处理

### 4. 文档

- 在 Kconfig 中添加详细说明
- 在代码中添加注释
- 更新模块清单的描述

## 示例模块

### 1. 简单模块示例

```c
// modules/simple/simple.c
#include "tkm.h"
#include "kernel.h"
#include "printk.h"

module_info mi = {
    .name = "simple",
    .init = simple_init,
    .version = 0,
    .export_table = NULL,
    .export_count = 0,
    .dependencies = NULL,
    .dep_count = 0,
};

uintptr_t simple_init(void)
{
    printk("Simple module loaded\n");
    return 0;
}

module_info *_start(kernel_info *ki)
{
    return (module_info *)&mi;
}
```

### 2. KPI 模块示例

```c
// modules/kpi_example/kpi_example.c
#include "tkm.h"
#include "kernel.h"
#include "printk.h"

// 导出函数
uintptr_t my_function(void)
{
    return 42;
}

// 导出变量
int my_variable = 100;

// 导出符号表
kpi_export_sym my_exports[] = {
    {"my_function", (uintptr_t)my_function, STT_FUNC},
    {"my_variable", (uintptr_t)&my_variable, STT_OBJECT},
};

module_info mi = {
    .name = "kpi_example",
    .init = kpi_example_init,
    .version = KPI_VERSION,
    .export_table = my_exports,
    .export_count = 2,
    .dependencies = NULL,
    .dep_count = 0,
};

uintptr_t kpi_example_init(void)
{
    printk("KPI example module loaded\n");
    return 0;
}

module_info *_start(kernel_info *ki)
{
    return (module_info *)&mi;
}
```

---

**作者**: TKernel Team  
**日期**: 2024-08-27  
**版本**: 1.0