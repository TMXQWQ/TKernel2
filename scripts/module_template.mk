# TKernel2 通用模块构建模板
# 
# 使用方法：
# 1. 在模块目录中创建一个简单的 Makefile，包含：
#    ModuleName := your_module_name
#    ModuleType := tkm
#    ModuleDependencies :=
#    include ../../scripts/module_template.mk
#
# 2. 创建 Kconfig 文件定义模块配置
# 3. 编译模块时会自动生成 .tkm 文件

# 包含全局配置
-include ../../.config

# 模块配置
ModuleName := $(ModuleName)
ModuleType := tkm
ModuleDependencies :=

# 编译器配置
ifeq ($(VERBOSE), 1)
  V=
  Q=
else
  V=@printf "\033[1;32m[ Build ]\033[0m $@ ...\n";
  Q=@
endif

# 工具链配置
CC := $(CONFIG_ARCH)-linux-gnu-gcc
ifeq ($(CONFIG_ARCH), "x86_64")
  LD := ld
else
  LD := ld.lld
endif

# 源文件
C_SOURCES := $(shell find * -name "*.c")
S_SOURCES := $(shell find * -name "*.s")
OBJS := $(C_SOURCES:%.c=%.o) $(S_SOURCES:%.s=%.o)
DEPS := $(OBJS:%.o=%.d)

# 链接器脚本
LD_SCRIPT := $(wildcard *.ld)
ifeq ($(LD_SCRIPT),)
  LD_SCRIPT := ../module_default.ld
endif

# 编译和链接标志
C_FLAGS = -Wall -fno-pie -fno-pic -Wextra -ffreestanding -fno-optimize-sibling-calls \
          -fno-stack-protector -fno-omit-frame-pointer -I ../../include -MMD

ifeq ($(CONFIG_ARCH), "x86_64")
  C_FLAGS += -mcmodel=large
endif

LD_FLAGS = -nostdlib -e _start -T $(LD_SCRIPT)

# 模块文件名
MODULE_FILE := $(ModuleName).tkm

# 默认目标
all: $(MODULE_FILE)

# 检查模块是否启用
# 使用通用的 CONFIG_$(ModuleName) 变量
ifeq ($(CONFIG_$(ModuleName)), m)

# 编译目标
%.o: %.c
	$(V)$(CC) $(C_FLAGS) -c -o $@ $<

# 链接目标
$(MODULE_FILE): $(OBJS)
	$(V)$(LD) $(LD_FLAGS) -r -o $@ $^
	$(Q) cp $@ ../
	$(Q) printf "\033[1;32m[ Done  ]\033[0m 模块 $(MODULE_FILE) 已生成\n"

# 清理目标
clean:
	$(V) rm -f $(MODULE_FILE) $(OBJS) $(DEPS)

# 显示帮助信息
help:
	$(Q)echo "模块 $(ModuleName) 构建选项:"
	$(Q)echo "  all        - 编译模块（默认）"
	$(Q)echo "  clean      - 清理编译文件"
	$(Q)echo "  help       - 显示此帮助信息"

# 依赖文件包含
-include $(DEPS)

else

# 模块未启用，不编译
all:
	$(Q)printf "\033[1;33m[ Skip  ]\033[0m 模块 $(ModuleName) 未启用\n"

clean:

help:
	$(Q)echo "模块 $(ModuleName) 未启用，请检查 .config"

endif

# 防止文件名冲突
.PHONY: all clean help