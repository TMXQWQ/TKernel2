# TKernel2

*一个简易操作系统内核的实现。*

[![License: GPL-3.0](https://img.shields.io/badge/License-GPL--3.0-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)

## 功能特性

- **多架构支持**：x86_64、RISC-V (riscv64)
- **Limine 引导加载器**：支持现代 UEFI 和 BIOS 启动
- **内核模块系统**：支持动态加载 `.tkm` 内核模块
- **Kconfig 配置系统**：Linux 风格的配置方式
- **硬件抽象层 (HAL)**：架构无关的内核设计
- **TTY 和串口驱动**：控制台 I/O 和串口支持
- **内存管理**：分页、堆分配和帧管理
- **中断处理**：IDT、GDT 和 APIC 支持 (x86_64)
- **设备驱动**：PCI、ACPI、HPET 和视频支持

## 项目结构

```
TKernel2/
├── assets/          # 引导加载器资源（Limine、OVMF BIOS、字体）
├── boot/            # 启动入口代码和 Limine 协议处理
├── hal/             # 硬件抽象层
│   └── arch/        # 架构特定代码（x86_64、riscv64）
│       ├── driver/  # 硬件驱动（串口等）
│       ├── linker/  # 链接器脚本
│       └── sync/    # 同步原语（自旋锁）
├── include/         # 公共头文件
│   └── generated/   # Kconfig 自动生成的头文件
├── kernel/          # 内核核心
│   ├── cpio/        # CPIO 归档处理
│   ├── debug/       # 调试和日志设施
│   ├── elf/         # ELF 文件解析
│   ├── module/      # 内核模块管理
│   ├── common.c     # 通用工具函数
│   └── kmain.c      # 内核入口点
├── lib/             # 标准库
│   ├── libc/        # C 标准库
│   └── utils/       # 工具函数（哈希等）
├── modules/         # 内核模块
│   ├── UxTK/        # Uinxed-Kernel 模块（子模块）
│   ├── test/        # 测试模块
│   └── test2/       # 测试模块 2
├── scripts/         # 构建和工具脚本
├── Makefile         # 主构建脚本
├── Kconfig          # Kconfig 配置定义
├── .config          # 当前配置（自动生成）
└── .gitmodules      # Git 子模块配置
```

## 构建要求

### 构建工具

- **GCC** 或 **Clang**（支持交叉编译）
- **GNU Make**
- **cpio**（用于 initrd 创建）
- **xorriso**（用于 ISO 创建）
- **mtools**、`sgdisk`、`limine`（用于 HDD 镜像创建）
- **QEMU**（用于测试）

### 架构特定要求

#### x86_64
- `x86_64-linux-gnu-gcc` 或支持 x86_64 目标的 Clang
- QEMU system x86_64

#### RISC-V
- `riscv64-linux-gnu-gcc` 工具链
- QEMU system riscv64
- OpenSBI 固件（已包含在 `assets/` 目录）

## 编译构建

### 1. 克隆仓库

```bash
git clone <仓库地址>
cd TKernel2
git submodule update --init --recursive
```

### 2. 配置内核

```bash
# 交互式菜单配置
make menuconfig

# 或使用默认配置
cp .config-default .config
```

配置选项包括：

| 选项 | 描述 | 默认值 |
|------|------|--------|
| `CONFIG_ARCH` | 目标架构（`x86_64`、`riscv64`） | `x86_64` |
| `CONFIG_KERNEL_LOG` | 启用运行时日志 | `y` |
| `CONFIG_CPU_MAX_COUNT` | 最大 CPU 数量（0 = 无限制） | `0` |
| `CONFIG_CPU_FEATURE_FPU` | 启用 X87 FPU/MMX (x86_64) | `n` |
| `CONFIG_CPU_FEATURE_SSE` | 启用 SSE/SSE2 (x86_64) | `n` |
| `CONFIG_CPU_FEATURE_AVX` | 启用 AVX/AVX2 (x86_64) | `n` |
| `CONFIG_TTY_DEFAULT_DEV` | 默认 TTY 设备 | `tty0` |
| `CONFIG_TTY_BUF_SIZE` | TTY 缓冲区大小 | `4096` |
| `CONFIG_SERIAL_BAUD_RATE` | 串口波特率 | `9600` |

### 3. 编译内核

```bash
make all
```

这将生成：
- `kernel.bin` - 内核可执行文件
- `TKernel-test.iso` - 可启动的 ISO 镜像
- `initrd.img` - 包含模块的初始 RAM 盘

### 4. 在 QEMU 中运行

```bash
# 使用 CD-ROM 运行（默认）
make run

# 调试模式运行
make run_db

# 使用 GDB 调试运行
make run_gdb

# 多核运行
make run_smp

# 使用 HDD 镜像运行（UEFI）
make run_hdd_uefi
```

## Makefile 目标

| 目标 | 描述 |
|------|------|
| `make all` | 构建整个项目（内核 + ISO） |
| `make run` | 在 QEMU 中运行 ISO |
| `make run_db` | 启用 QEMU 调试运行 |
| `make run_gdb` | 启用 GDB 调试支持运行 |
| `make run_smp` | 多核运行（默认 2 核） |
| `make run_hdd_uefi` | 使用 HDD 镜像运行（UEFI） |
| `make clean` | 清理所有生成的文件 |
| `make menuconfig` | 运行交互式配置 |
| `make nconfig` | 运行 nconfig（menuconfig 的替代方案） |
| `make format` | 使用 clang-format 格式化所有源文件 |
| `make check` | 运行 clang-tidy 静态分析 |
| `make gen.clangd` | 为 IDE 生成 .clangd 配置 |
| `make help` | 显示帮助信息 |

## 内核模块

TKernel2 支持动态内核模块，使用 `.tkm` 扩展名。

### 创建模块

1. 在 `modules/` 目录下创建目录（如 `modules/mymodule/`）
2. 添加 `Kconfig` 文件用于配置选项
3. 添加构建规则的 `Makefile`
4. 实现模块代码

### 模块配置

```makefile
# modules/mymodule/Kconfig
config MYMODULE
    tristate "我的自定义模块"
    help
      模块描述信息
```

### 构建模块

模块在 Kconfig 中选中后会自动构建。`.tkm` 文件会被打包到 `initrd.img` 中，并在启动时加载。

## 架构支持

### x86_64

- Limine 引导加载器支持（BIOS 和 UEFI）
- 分页内存管理
- 中断处理（IDT、GDT、APIC）
- FPU、SSE、AVX 支持（可选）
- KVM 加速支持

### RISC-V (riscv64)

- OpenSBI 固件支持
- QEMU virt 机器
- 基本中断和内存管理
- RISC-V 特定串口驱动

## 开发

### 代码风格

项目使用 clang-format 保持代码格式一致：

```bash
# 格式化所有文件
make format

# 格式化特定文件
clang-format -i path/to/file.c
```

### 静态分析

运行 clang-tidy 检查：

```bash
make check
```

### IDE 配置

生成 `.clangd` 用于语言服务器支持：

```bash
make gen.clangd
```

### 调试

在 Makefile 中将 `-O3` 改为 `-O0` 以启用调试构建，获得更好的堆栈跟踪。

## 依赖项

### Git 子模块

- **UxTK** (modules/UxTK)：Uinxed-Kernel 模块，包含额外的驱动和功能

初始化子模块：

```bash
git submodule update --init --recursive
```

## 许可证

本项目采用 GPL-3.0 许可证 - 查看 LICENSE 文件了解详情。

## 致谢

- [Uinxed-Kernel](https://github.com/ViudiraTech/Uinxed-Kernel) - 提供部分源代码
- [Limine](https://github.com/limine-bootloader/limine) - 引导加载器
- [OpenSBI](https://github.com/riscv/opensbi) - RISC-V 固件

## 贡献

欢迎贡献！请确保：

1. 代码遵循现有风格（运行 `make format`）
2. 更改通过静态分析检查（运行 `make check`）
3. 更新相关文档
4. 提交信息清晰明确

## 故障排除

### 构建错误

- **缺少工具**：确保安装所有必需的工具（`gcc`、`make`、`cpio`、`xorriso` 等）
- **交叉编译器**：对于非 x86_64 目标，安装相应的交叉编译器工具链

### 运行时问题

- **启动失败**：检查 BIOS/UEFI 设置，确保正确的启动顺序
- **内核崩溃**：启用调试模式并检查串口输出
- **模块加载问题**：验证 `.tkm` 文件是否正确构建和打包

## 参考资料

- [Limine 文档](https://github.com/limine-bootloader/limine)
- [RISC-V 规范](https://riscv.org/specifications/)
- [OSDev Wiki](https://wiki.osdev.org/)

---

**版权 © 2024 TMX. 基于 GPL-3.0 许可证。**