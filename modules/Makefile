-include ../.config

# Python 解释器（检查多个常见位置）
PYTHON := $(shell which python3 || which python || echo "python3")

# 模块清单文件
MODULE_MANIFEST := module_manifest.json
MODULE_LIST_TXT := module_list.txt

ifeq ($(VERBOSE), 1)
  V=
  Q=
else
  V=@printf "\033[1;32m[ Build ]\033[0m $@ ...\n";
  Q=@
endif

C_SOURCES      := $(shell find * -name "*.c")
tkms           := $(shell find * -name "*.tkm")
OBJS           := $(C_SOURCES:%.c=%.o) $(S_SOURCES:%.s=%.o)
DEPS           := $(OBJS:%.o=%.d)

# 检查是否需要重新生成模块清单
NEED_MANIFEST := $(shell test -f $(MODULE_MANIFEST) || echo "1")
NEED_SORT     := $(shell test -f $(MODULE_LIST_TXT) || echo "1")

# 生成模块清单（如果没有或需要更新）
ifeq ($(NEED_MANIFEST),1)
  $(info 生成模块清单...)
  _GEN_MANIFEST := $(shell $(PYTHON) ../scripts/gen_module_manifest.py > $(MODULE_MANIFEST) 2>/dev/null)
endif

# 自动生成模块加载顺序
ifeq ($(NEED_SORT),1)
  $(info 解析模块依赖并生成加载顺序...)
  _GEN_SORT := $(shell $(PYTHON) ../scripts/sort_modules.py > $(MODULE_LIST_TXT) 2>/dev/null)
endif

# 包含生成的模块列表
-include $(MODULE_LIST_TXT)

# 如果模块列表为空，使用回退方案
ifndef MODULE_LIST
  MODULE_LIST := $(shell ls *.tkm 2>/dev/null | tr '\n' ' ')
  $(info 警告: 无法生成模块列表，使用当前目录中的 .tkm 文件)
endif

# 构建所有启用的模块
.PHONY: $(MODULE_LIST)
$(MODULE_LIST): %: 
	$(V) echo "Building $@ in $(patsubst %.tkm,%,$@)"
	$(V) if [ "$@" = "TASK_MODULE.tkm" ]; then \
		echo "Special handling for TASK_MODULE.tkm"; \
		make all -C task; \
	else \
		make all -C $(patsubst %.tkm,%,$@); \
	fi

# 确保所有模块都被构建
ALL_MODULES := $(MODULE_LIST)
all-modules: $(ALL_MODULES)

# Debug target
debug:
	@echo "MODULE_LIST: $(MODULE_LIST)"
	@echo "ALL_MODULES: $(ALL_MODULES)"
	@echo "Current directory: $(pwd)"
	@echo "Files in current directory: $(wildcard *.tkm)"

initrd.img: $(ALL_MODULES)
	$(V)
	$(Q) find . -maxdepth 1 -name "*.tkm" | cpio --format=newc -o > $@ 2> /dev/null
	$(Q) cp $@ ../
	$(Q) printf "\033[1;32m[ Info  ]\033[0m initrd.img 已创建，包含 %d 个模块\n" $$(find . -maxdepth 1 -name "*.tkm" | wc -l)

clean:
	$(V) rm -f initrd.img $(OBJS) $(DEPS) $(tkms) $(MODULE_MANIFEST)

.PHONY: gen-manifest sort-modules
gen-manifest:
	$(Q) $(PYTHON) ../scripts/gen_module_manifest.py > $(MODULE_MANIFEST)
	$(Q) printf "\033[1;32m[ Done  ]\033[0m 模块清单已生成: $(MODULE_MANIFEST)\n"

sort-modules: gen-manifest
	$(Q) $(PYTHON) ../scripts/sort_modules.py > $(MODULE_LIST_TXT)
	$(Q) printf "\033[1;32m[ Done  ]\033[0m 模块加载顺序已更新: $(MODULE_LIST_TXT)\n"