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

def parse_kconfig_dependencies(kconfig_path):
    """解析 Kconfig 文件，提取依赖关系"""
    dependencies = []
    if not kconfig_path.exists():
        return dependencies

    try:
        with open(kconfig_path, 'r', encoding='utf-8') as f:
            content = f.read()

        # 查找 dependencies 或 depends on 语句
        lines = content.split('\n')
        for line in lines:
            line = line.strip()
            # 只提取模块依赖，忽略通用的 depends on 语句
            if line.startswith('depends on'):
                # 提取依赖的模块名
                parts = line.split()
                if len(parts) > 1:
                    dep = parts[1]
                    # 过滤掉非模块依赖
                    if dep not in ['m', 'y', 'n', 'on']:
                        if dep not in dependencies:
                            dependencies.append(dep)
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