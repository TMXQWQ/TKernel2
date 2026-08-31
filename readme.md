# TKernel2

*A simple operating system kernel implementation.*

[![License: GPL-3.0](https://img.shields.io/badge/License-GPL--3.0-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)

## Features

- **Multi-architecture support**: x86_64, RISC-V (riscv64)
- **Limine bootloader**: Modern UEFI and BIOS boot support
- **Kernel module system**: Dynamic loading of `.tkm` kernel modules
- **Kconfig configuration**: Linux-style configuration system
- **Hardware Abstraction Layer (HAL)**: Architecture-independent kernel design
- **TTY and Serial drivers**: Console I/O and serial port support
- **Memory management**: Paging, heap allocation, and frame management
- **Interrupt handling**: IDT, GDT, and APIC support (x86_64)
- **Device drivers**: PCI, ACPI, HPET, and video support

## Project Structure

```
TKernel2/
├── assets/          # Bootloader resources (Limine, OVMF BIOS, fonts)
├── boot/            # Boot entry code and Limine protocol handling
├── hal/             # Hardware Abstraction Layer
│   └── arch/        # Architecture-specific code (x86_64, riscv64)
│       ├── driver/  # Hardware drivers (serial, etc.)
│       ├── linker/  # Linker scripts
│       └── sync/    # Synchronization primitives (spin locks)
├── include/         # Public header files
│   └── generated/   # Auto-generated headers from Kconfig
├── kernel/          # Kernel core
│   ├── cpio/        # CPIO archive handling
│   ├── debug/       # Debug and logging facilities
│   ├── elf/         # ELF file parsing
│   ├── module/      # Kernel module management
│   ├── common.c     # Common utilities
│   └── kmain.c      # Kernel entry point
├── lib/             # Standard library
│   ├── libc/        # C standard library
│   └── utils/       # Utility functions (hash, etc.)
├── modules/         # Kernel modules
│   ├── UxTK/        # Uinxed-Kernel module (submodule)
│   ├── test/        # Test module
│   └── test2/       # Test module 2
├── scripts/         # Build and utility scripts
├── Makefile         # Main build script
├── Kconfig          # Kconfig configuration definitions
├── .config          # Active configuration (auto-generated)
└── .gitmodules      # Git submodules configuration
```

## Requirements

### Build Tools

- **GCC** or **Clang** with cross-compilation support
- **GNU Make**
- **cpio** (for initrd creation)
- **xorriso** (for ISO creation)
- **mtools**, `sgdisk`, `limine` (for HDD image creation)
- **QEMU** (for testing)

### Architecture-Specific

#### x86_64
- `x86_64-linux-gnu-gcc` or Clang with x86_64 target
- QEMU system x86_64

#### RISC-V
- `riscv64-linux-gnu-gcc` toolchain
- QEMU system riscv64
- OpenSBI firmware (included in `assets/`)

## Building

### 1. Clone the Repository

```bash
git clone <repository-url>
cd TKernel2
git submodule update --init --recursive
```

### 2. Configure the Kernel

```bash
# Interactive menu configuration
make menuconfig

# Or use the default configuration
cp .config-default .config
```

Configuration options include:

| Option | Description | Default |
|--------|-------------|---------|
| `CONFIG_ARCH` | Target architecture (`x86_64`, `riscv64`) | `x86_64` |
| `CONFIG_KERNEL_LOG` | Enable runtime logging | `y` |
| `CONFIG_CPU_MAX_COUNT` | Maximum CPU count (0 = no limit) | `0` |
| `CONFIG_CPU_FEATURE_FPU` | Enable X87 FPU/MMX (x86_64) | `n` |
| `CONFIG_CPU_FEATURE_SSE` | Enable SSE/SSE2 (x86_64) | `n` |
| `CONFIG_CPU_FEATURE_AVX` | Enable AVX/AVX2 (x86_64) | `n` |
| `CONFIG_TTY_DEFAULT_DEV` | Default TTY device | `tty0` |
| `CONFIG_TTY_BUF_SIZE` | TTY buffer size | `4096` |
| `CONFIG_SERIAL_BAUD_RATE` | Serial baud rate | `9600` |

### 3. Build the Kernel

```bash
make all
```

This will generate:
- `kernel.bin` - The kernel executable
- `TKernel-test.iso` - Bootable ISO image
- `initrd.img` - Initial RAM disk with modules

### 4. Run in QEMU

```bash
# Run with CD-ROM (default)
make run

# Run with debugging
make run_db

# Run with GDB support
make run_gdb

# Run with SMP (multi-core)
make run_smp

# Run with HDD image (UEFI)
make run_hdd_uefi
```

## Makefile Targets

| Target | Description |
|--------|-------------|
| `make all` | Build the entire project (kernel + ISO) |
| `make run` | Run the ISO in QEMU |
| `make run_db` | Run with QEMU debugging enabled |
| `make run_gdb` | Run with GDB debugging support |
| `make run_smp` | Run with SMP (2 cores by default) |
| `make run_hdd_uefi` | Run with HDD image (UEFI) |
| `make clean` | Clean all generated files |
| `make menuconfig` | Run interactive configuration |
| `make nconfig` | Run nconfig (alternative to menuconfig) |
| `make format` | Format all source files with clang-format |
| `make check` | Run clang-tidy static analysis |
| `make gen.clangd` | Generate .clangd configuration for IDEs |
| `make help` | Display help message |

## Kernel Modules

TKernel2 supports dynamic kernel modules with the `.tkm` extension.

### Creating a Module

1. Create a directory in `modules/` (e.g., `modules/mymodule/`)
2. Add a `Kconfig` file for configuration options
3. Add a `Makefile` with build rules
4. Implement your module code

### Module Configuration

```makefile
# modules/mymodule/Kconfig
config MYMODULE
    tristate "My Custom Module"
    help
      Description of your module
```

### Building Modules

Modules are built automatically when selected in Kconfig. The `.tkm` files are packed into `initrd.img` and loaded at boot time.

## Architecture Support

### x86_64

- Limine bootloader support (BIOS and UEFI)
- Memory management with paging
- Interrupt handling (IDT, GDT, APIC)
- FPU, SSE, AVX support (optional)
- KVM acceleration support

### RISC-V (riscv64)

- OpenSBI firmware support
- QEMU virt machine
- Basic interrupt and memory management
- RISC-V-specific serial driver

## Kernel Modules

TKernel2 supports dynamic kernel modules with the `.tkm` extension.

### New Module System Features

The new module system includes several improvements:

- **Dynamic CPIO Parsing**: Increased limit from 128 to 512 files
- **Automatic Dependency Resolution**: Topological sorting of module dependencies
- **Module Manifest System**: JSON-based module metadata
- **Unified Build Templates**: Simplified module development
- **Circular Dependency Detection**: Prevents loading issues

### Creating Modules

#### Simple Module (Basic Template)

```makefile
# modules/my_module/Makefile
ModuleName := my_module
ModuleType := tkm
ModuleDependencies :=
include ../../scripts/module_template.mk
```

#### Complex Module (Advanced Template)

```makefile
# modules/my_module/Makefile
ModuleName := my_module
ModuleType := tkm
ModuleDependencies :=
ModuleLibs := $(shell find lib/ -name "lib*.a")
include ../../scripts/module_template_advanced.mk
```

#### Kconfig File

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

#### Module Code

```c
#include "tkm.h"

module_info mi = {
    .name = "my_module",
    .init = module_init,
    .version = KPI_VERSION,
    .export_table = my_exports,
    .export_count = 1,
    .dependencies = NULL,
    .dep_count = 0,
};

uintptr_t module_init(void)
{
    // Module initialization code
    return 0;
}

module_info *_start(kernel_info *ki)
{
    return (module_info *)&mi;
}
```

### Building Modules

#### Build Single Module

```bash
cd modules/my_module
make all
```

#### Build All Modules

```bash
cd modules
make all
```

#### Generate Module Manifest

```bash
cd modules
make gen-manifest
```

#### Generate Loading Order

```bash
cd modules
make sort-modules
```

### Module Manifest System

The system automatically generates a `module_manifest.json` file:

```json
{
  "version": "1.0",
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

### Dependency Management

Dependencies are automatically resolved from Kconfig files:

- **Automatic Sorting**: Modules are loaded in dependency order
- **Cycle Detection**: Prevents circular dependencies
- **Error Handling**: Clear error messages for dependency issues

### Advanced Features

#### KPI Modules

Kernel Programming Interface modules can export symbols:

```c
kpi_export_sym my_exports[] = {
    {"my_function", (uintptr_t)my_function, STT_FUNC},
    {"my_variable", (uintptr_t)&my_variable, STT_OBJECT},
};
```

#### Module Development Guide

For detailed information on module development, see [docs/module_development.md](docs/module_development.md).

## Development

### Code Style

The project uses clang-format for consistent code formatting:

```bash
# Format all files
make format

# Format specific file
clang-format -i path/to/file.c
```

### Static Analysis

Run clang-tidy checks:

```bash
make check
```

### IDE Configuration

Generate `.clangd` for language server support:

```bash
make gen.clangd
```

### Debugging

Enable debug builds by changing `-O3` to `-O0` in the Makefile for better stack traces.

## Dependencies

### Git Submodules

- **UxTK** (modules/UxTK): Uinxed-Kernel module with additional drivers and features

Initialize submodules:

```bash
git submodule update --init --recursive
```

## License

This project is licensed under the GPL-3.0 License - see the LICENSE file for details.

## Credits

- [Uinxed-Kernel](https://github.com/ViudiraTech/Uinxed-Kernel) - Provided partial source code
- [Limine](https://github.com/limine-bootloader/limine) - Bootloader
- [OpenSBI](https://github.com/riscv/opensbi) - RISC-V firmware

## Contributing

Contributions are welcome! Please ensure:

1. Code follows the existing style (run `make format`)
2. Changes pass static analysis (run `make check`)
3. Documentation is updated as needed
4. Commits are well-documented

## Troubleshooting

### Build Errors

- **Missing tools**: Ensure all required tools are installed (`gcc`, `make`, `cpio`, `xorriso`, etc.)
- **Cross-compiler**: For non-x86_64 targets, install the appropriate cross-compiler toolchain

### Runtime Issues

- **Boot failure**: Check BIOS/UEFI settings and ensure proper boot order
- **Kernel panic**: Enable debug mode and check serial output
- **Module loading issues**: Verify `.tkm` files are correctly built and packed

## Resources

- [Limine Documentation](https://github.com/limine-bootloader/limine)
- [RISC-V Specification](https://riscv.org/specifications/)
- [OSDev Wiki](https://wiki.osdev.org/)

---

**Copyright © 2024 TMX. Licensed under GPL-3.0.**