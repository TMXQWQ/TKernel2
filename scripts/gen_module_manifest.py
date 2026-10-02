#!/usr/bin/env python3
"""
生成模块清单文件 (module_manifest.json)

该脚本扫描 modules/ 目录，自动生成模块的元数据清单，包括：
- 模块名称
- 模块路径
- 依赖关系
- Kconfig 中的配置信息

使用方法：
    python3 scripts/gen_module_manifest.py > modules/module_manifest.json
"""

import os
import json
import sys
from pathlib import Path

# 项目根目录
PROJECT_ROOT = Path(__file__).parent.parent
MODULES_DIR = PROJECT_ROOT / "modules"

# Kconfig 配置符号 -> 模块目录名 的映射。
# Kconfig 里 `depends on` 用的是配置符号（如 TASK_MODULE / LINUX_SYSCALL），
# 而模块目录名是 task / linux_syscall，二者不一致。必须在生成清单时把依赖翻译
# 成目录名，否则 sort_modules.py 的拓扑排序认不出依赖，加载顺序就是错的。
# （在 generate_manifest() 里预扫描填充。）
SYMBOL_TO_DIR = {}


def parse_kconfig_symbol(kconfig_path):
    """从 Kconfig 中提取 `config <SYMBOL>` 的符号名；失败返回 None"""
    if not kconfig_path.exists():
        return None
    try:
        with open(kconfig_path, 'r', encoding='utf-8') as f:
            for line in f.read().split('\n'):
                line = line.strip()
                if line.startswith('config '):
                    parts = line.split()
                    if len(parts) > 1:
                        return parts[1]
    except Exception:
        return None
    return None


def parse_kconfig_dependencies(kconfig_path):
    """解析 Kconfig 文件，提取依赖关系（返回模块目录名列表）"""
    dependencies = []
    if not kconfig_path.exists():
        return dependencies

    try:
        with open(kconfig_path, 'r', encoding='utf-8') as f:
            content = f.read()

        # 查找 dependencies 或 depends on 语句
        for line in content.split('\n'):
            line = line.strip()
            # 只提取模块依赖，忽略通用的 depends on 语句
            if line.startswith('depends on'):
                # 'depends on X'.split() == ['depends', 'on', 'X']，
                # 依赖名是 parts[2]（此前误写成 parts[1]，取到的是 'on'，
                # 会被下面的过滤条件吞掉，导致所有模块依赖恒为空）。
                parts = line.split()
                if len(parts) > 2:
                    dep = parts[2]
                    # 过滤掉非模块依赖（m/y/n/on 等）
                    if dep in ['m', 'y', 'n', 'on']:
                        continue
                    # 把配置符号翻译成模块目录名；翻不到的（如 CPU_FEATURE_*）
                    # 说明不是模块依赖，直接忽略。
                    dep_dir = SYMBOL_TO_DIR.get(dep)
                    if dep_dir and dep_dir not in dependencies:
                        dependencies.append(dep_dir)
    except Exception as e:
        print(f"警告: 解析 {kconfig_path} 失败: {e}", file=sys.stderr)

    return dependencies

def get_module_info(module_path):
    """获取模块信息"""
    module_name = module_path.name

    # 检查是否存在 .tkm 文件或 Makefile
    tkm_files = list(module_path.glob("*.tkm"))
    makefile = module_path / "Makefile"
    kconfig = module_path / "Kconfig"

    if not makefile.exists():
        return None

    # 解析依赖
    dependencies = parse_kconfig_dependencies(kconfig)

    # 查找主要的 .tkm 文件
    main_tkm = None
    if tkm_files:
        # 优先选择与模块名同名的文件
        for tkm in tkm_files:
            if tkm.stem == module_name:
                main_tkm = tkm
                break
        if main_tkm is None:
            main_tkm = tkm_files[0]

    return {
        "name": module_name,
        "path": str(module_path.relative_to(PROJECT_ROOT)),
        "tkm_file": str(main_tkm.relative_to(PROJECT_ROOT)) if main_tkm else None,
        "dependencies": dependencies,
        "enabled": True  # 默认启用，实际状态从 .config 读取
    }

def generate_manifest():
    """生成模块清单"""
    manifest = {
        "version": "1.0",
        "description": "TKernel2 模块清单 - 自动生成",
        "modules": []
    }

    # 扫描模块目录
    if not MODULES_DIR.exists():
        print(f"错误: 模块目录不存在: {MODULES_DIR}", file=sys.stderr)
        return manifest

    # 预扫描：建立 配置符号 -> 模块目录名 的映射，供依赖翻译使用
    SYMBOL_TO_DIR.clear()
    for item in MODULES_DIR.iterdir():
        if item.is_dir() and not item.name.startswith('.') and (item / "Makefile").exists():
            symbol = parse_kconfig_symbol(item / "Kconfig")
            if symbol:
                SYMBOL_TO_DIR[symbol] = item.name

    for item in MODULES_DIR.iterdir():
        if item.is_dir() and not item.name.startswith('.'):
            # 检查是否是有效的模块目录（有 Makefile）
            makefile = item / "Makefile"
            if not makefile.exists():
                continue
                
            # 跳过子模块仓库（如 UxTK），除非它们是启用的模块
            if (item / '.git').exists():
                # 检查是否是启用的模块（有对应的 CONFIG_选项）
                config_name = f"CONFIG_{item.name}"
                try:
                    with open(PROJECT_ROOT / '.config', 'r') as config_file:
                        config_content = config_file.read()
                        if f"{config_name}=m" not in config_content and f"{config_name}=y" not in config_content:
                            continue  # 未启用的子模块，跳过
                except Exception:
                    continue  # 无法读取配置文件，跳过

            module_info = get_module_info(item)
            if module_info:
                manifest["modules"].append(module_info)

    # 按名称排序
    manifest["modules"].sort(key=lambda x: x["name"])

    return manifest

def main():
    """主函数"""
    manifest = generate_manifest()
    print(json.dumps(manifest, indent=2, ensure_ascii=False))

if __name__ == "__main__":
    main()