#!/usr/bin/env python3
"""
模块依赖排序工具

使用拓扑排序算法解析模块依赖关系，生成正确的加载顺序。

使用方法：
    python3 scripts/sort_modules.py > modules/module_list.txt
"""

import json
import sys
from pathlib import Path
from collections import defaultdict, deque

# 项目根目录
PROJECT_ROOT = Path(__file__).parent.parent
MANIFEST_FILE = PROJECT_ROOT / "modules" / "module_manifest.json"


def load_manifest(manifest_path):
    """加载模块清单"""
    if not manifest_path.exists():
        print(f"错误: 清单文件不存在: {manifest_path}", file=sys.stderr)
        print(f"请先运行: python3 scripts/gen_module_manifest.py > {manifest_path}", file=sys.stderr)
        return None

    try:
        with open(manifest_path, 'r', encoding='utf-8') as f:
            return json.load(f)
    except Exception as e:
        print(f"错误: 无法加载清单文件: {e}", file=sys.stderr)
        return None


def build_dependency_graph(modules):
    """构建依赖关系图"""
    # 模块名到模块信息的映射
    module_map = {m["name"]: m for m in modules if m.get("enabled", True)}

    # 邻接表：module -> [dependencies]
    graph = defaultdict(list)

    # 入度计数：module -> in_degree
    in_degree = defaultdict(int)

    # 初始化所有模块
    for module in module_map.values():
        in_degree[module["name"]] = 0

    # 构建依赖图
    for module in module_map.values():
        for dep in module.get("dependencies", []):
            if dep in module_map:
                graph[dep].append(module["name"])
                in_degree[module["name"]] += 1
            else:
                print(f"警告: 模块 '{module['name']}' 依赖 '{dep}'，但该模块不存在或未启用", file=sys.stderr)

    return graph, in_degree, module_map


def topological_sort(graph, in_degree, module_map):
    """拓扑排序"""
    # 队列：存储入度为 0 的节点
    queue = deque()

    # 找到所有入度为 0 的节点
    for module_name in module_map:
        if in_degree[module_name] == 0:
            queue.append(module_name)

    # 排序结果
    sorted_modules = []
    visited = set()

    # Kahn 算法
    while queue:
        # 从队列中取出一个节点
        current = queue.popleft()

        if current in visited:
            continue

        visited.add(current)
        sorted_modules.append(current)

        # 减少所有依赖当前模块的其他模块的入度
        for neighbor in graph[current]:
            in_degree[neighbor] -= 1
            if in_degree[neighbor] == 0:
                queue.append(neighbor)

    # 检查是否有循环依赖
    if len(sorted_modules) != len(module_map):
        # 找出未访问的模块（循环依赖）
        cycle_modules = [m for m in module_map if m not in visited]
        print(f"错误: 检测到循环依赖，涉及模块: {cycle_modules}", file=sys.stderr)

        # 尝试简单的循环检测
        for module in cycle_modules:
            print(f"  - {module}: 依赖 {module_map[module].get('dependencies', [])}", file=sys.stderr)

        return None

    return sorted_modules


def main():
    """主函数"""
    # 加载清单
    manifest = load_manifest(MANIFEST_FILE)
    if manifest is None:
        # 回退到读取 module_list.txt
        print(f"警告: 无法加载模块清单，使用现有 module_list.txt", file=sys.stderr)
        return

    modules = manifest.get("modules", [])

    if not modules:
        print("警告: 没有找到任何模块", file=sys.stderr)
        return

    # 构建依赖图
    graph, in_degree, module_map = build_dependency_graph(modules)

    # 拓扑排序
    sorted_modules = topological_sort(graph, in_degree, module_map)

    if sorted_modules is None:
        print("错误: 无法解析模块依赖顺序", file=sys.stderr)
        sys.exit(1)

    # 输出排序结果
    print("# 自动生成的模块加载顺序 - 由拓扑排序算法生成")
    print("# 修改此文件不会影响加载顺序，请修改 Kconfig 中的依赖关系")
    print(f"MODULE_LIST := {' '.join(f'{m}.tkm' for m in sorted_modules)}")


if __name__ == "__main__":
    main()