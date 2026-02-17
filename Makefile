# =====================================================
#
#      Makefile
#      Uinxed-Kernel compile script
#
#      2024/6/23 By Rainy101112
#      Based on GPL-3.0 open source agreement
#      Copyright © 2020 ViudiraTech, based on the GPLv3 agreement.
#
# =====================================================

ifneq ($(wildcard .config),)
  include .config
else ifneq ($(wildcard .config-default),)
  include .config-default
else
  $(error No configuration file (.config or .config-default) found)
endif

ifeq ($(VERBOSE), 1)
  V=
  Q=
else
  V=@printf "\033[1;32m[ Build ]\033[0m $@ ...\n";
  Q=@
	MAKE_FLAGS	:=	--no-print-directory
endif

ifeq ($(CONFIG_KERNEL_LOG), y)
  C_CONFIG += -DKERNEL_LOG=1
else
  C_CONFIG += -DKERNEL_LOG=0
endif

ifeq ($(CONFIG_TTF_CONSOLE), y)
  C_CONFIG += -DTTF_CONSOLE=1
else
  C_CONFIG += -DTTF_CONSOLE=0
endif

ifneq ($(CONFIG_CONSOLE_FONT_SIZE),)
  C_CONFIG += -DCONSOLE_FONT_SIZE=$(CONFIG_CONSOLE_FONT_SIZE)
endif

ifneq ($(CONFIG_MAX_CPU_COUNT),)
  C_CONFIG += -DMAX_CPU_COUNT=$(CONFIG_MAX_CPU_COUNT)
endif

ifeq ($(CONFIG_ARCH), "x86_64")

ifeq ($(CONFIG_CPU_FEATURE_FPU), y)
  C_CONFIG += -DCPU_FEATURE_FPU=1
else
  C_CONFIG += -DCPU_FEATURE_FPU=0 -mno-mmx -mno-80387
endif

ifeq ($(CONFIG_CPU_FEATURE_SSE), y)
  C_CONFIG += -DCPU_FEATURE_SSE=1
else
  C_CONFIG += -DCPU_FEATURE_SSE=0 -mno-sse -mno-sse2
endif

ifeq ($(CONFIG_CPU_FEATURE_AVX), y)
  C_CONFIG += -DCPU_FEATURE_AVX=1
else
  C_CONFIG += -DCPU_FEATURE_AVX=0 -mno-avx -mno-avx2
endif

endif

ifneq ($(CONFIG_TTY_DEFAULT_DEV),)
  C_CONFIG += -DTTY_DEFAULT_DEV=\"$(CONFIG_TTY_DEFAULT_DEV)\"
endif

ifneq ($(CONFIG_TTY_BUF_SIZE),)
  C_CONFIG += -DTTY_BUF_SIZE=$(CONFIG_TTY_BUF_SIZE)
endif

ifneq ($(CONFIG_SERIAL_BAUD_RATE),)
  C_CONFIG += -DSERIAL_BAUD_RATE=$(CONFIG_SERIAL_BAUD_RATE)
endif

ifneq ($(CONFIG_SERIAL_DATA_BITS),)
  C_CONFIG += -DSERIAL_DATA_BITS=$(CONFIG_SERIAL_DATA_BITS)
endif

ifneq ($(CONFIG_SERIAL_STOP_BITS),)
  C_CONFIG += -DSERIAL_STOP_BITS=$(CONFIG_SERIAL_STOP_BITS)
endif

IMAGE_NAME	:=	TKernel-test

C_SOURCES      := $(shell find kernel -name "*.c") $(shell find boot -name "*.c") $(shell find lib -name "*.c")
MOD_SOURCES	:= $(shell find modules -name "*.c")
S_SOURCES      := $(shell find * -name "*.s")
HEADERS        := $(shell find * -name "*.h")
OBJS           := $(C_SOURCES:%.c=%.o) $(S_SOURCES:%.s=%.o)
DEPS           := $(OBJS:%.o=%.d)
LIBS           := $(wildcard libs/lib*.a)
PWD            := $(shell pwd)

QEMU           := qemu-system-$(CONFIG_ARCH)
QEMU_SERIAL	:= stdio
QEMU_BIOS	:=	assets/ovmf-code.fd
QEMU_KVM	   := --enable-kvm
QEMU_SMP	   := 2
QEMU_FLAGS     := -bios assets/ovmf-code.fd -serial $(QEMU_SERIAL) --bios $(QEMU_BIOS)

OBJDUMP	:=	$(CONFIG_ARCH)-linux-gnu-objdump

CHECKS         := -quiet -checks=-*,clang-analyzer-*,bugprone-*,cert-*,misc-*,performance-*,portability-*,-misc-include-cleaner,-clang-analyzer-security.insecureAPI.*

# If you want to get more details of `dump_stack`, you need to replace `-O3` with `-O0` or '-Os'.
# `-fno-optimize-sibling-calls` is for `dump_stack` to work properly.
# ifeq ($(CONFIG_ARCH),"x86_64")
CC	:=	$(CONFIG_ARCH)-linux-gnu-gcc
LD	:=	$(CONFIG_ARCH)-linux-gnu-ld

ifeq ($(CONFIG_ARCH), "x86_64")
LD	:=	ld
endif

C_FLAGS        := -Wall -Wextra -O0 -g3 -m64 -fpie -ffreestanding -fno-optimize-sibling-calls -fno-stack-protector -fno-omit-frame-pointer -mstackrealign -mno-red-zone -I include -MMD
LD_FLAGS       := -nostdlib -T assets/linker.ld
# endif


all: info TKernel-test.iso

%.o: %.c
	$(V)$(CC) $(C_FLAGS) $(C_CONFIG) -c -o $@ $<

%.o: %.s
	$(V)$(AS) $(AS_FLAGS) -o $@ $<

%.fmt: %
	$(Q)printf "\033[1;32m[Format]\033[0m $< ...\n"
	$(Q)clang-format -i $<

%.tidy: %
	$(Q)printf "\033[1;32m[Checks]\033[0m $< ...\n"
	$(Q)clang-tidy $< $(CHECKS) -- $(C_FLAGS)

info:
	$(Q)printf "TKernel2 Compile Script.\n"
	$(Q)printf "By Microfish and TMX.\n"
	$(Q)printf "Thanks for ViudiraTech awa.\n"
	$(Q)printf "Copyright 2020 TMX. Based on the GPLv3 license.\n"
	$(Q)printf "Based on the GPL-3.0 open source license.\n"
	$(Q)echo

kernel.bin: $(OBJS) $(LIBS)
	$(V)$(LD) $(LD_FLAGS) -o $@ $^

kerneldump.log: kernel.bin
	$(V)$(OBJDUMP) -d kernel.bin > kerneldump.log &

$(IMAGE_NAME).iso: kernel.bin initrd.img
	$(Q)echo
	$(Q)printf "\033[1;32m[ ISO   ]\033[0m Packing ISO file...\n"
	$(Q)cp -a assets/Limine iso
	$(Q)cp initrd.img iso/EFI/BOOT/
	$(Q)cp $< iso/EFI/Boot
	$(Q)xorriso -as mkisofs -R -r -J -b Limine/limine-bios-cd.bin -no-emul-boot -boot-load-size 4 \
                -boot-info-table -hfsplus -apm-block-size 2048 -efi-boot-part --efi-boot-image --protective-msdos-label \
                --efi-boot Limine/limine-uefi-cd.bin -o TKernel-test.iso iso &> /dev/null

	$(Q) $(RM) -rf iso
	$(Q) printf "\033[1;32m[ Done  ]\033[0m Compilation complete.\n"
	$(Q) printf "\033[1;32m[ INFO  ]\033[0m Code Statistics:\n"
	$(Q) cloc .

$(IMAGE_NAME).hdd:  kernel.bin initrd.img
	rm -f $(IMAGE_NAME).hdd
	dd if=/dev/zero bs=1M count=0 seek=64 of=$(IMAGE_NAME).hdd
	PATH=$$PATH:/usr/sbin:/sbin sgdisk $(IMAGE_NAME).hdd -n 1:2048 -t 1:ef00 -m 1
	limine bios-install $(IMAGE_NAME).hdd
	mformat -i $(IMAGE_NAME).hdd@@1M
	mmd -i $(IMAGE_NAME).hdd@@1M ::/EFI ::/EFI/BOOT ::/boot ::/boot/limine
	mcopy -i $(IMAGE_NAME).hdd@@1M kernel.bin ::/EFI/Boot/
	mcopy -i $(IMAGE_NAME).hdd@@1M initrd.img ::/EFI/Boot/
	mcopy -i $(IMAGE_NAME).hdd@@1M assets/Limine/Limine/limine.conf assets/Limine/Limine/limine-bios.sys ::/boot/limine
	mcopy -i $(IMAGE_NAME).hdd@@1M assets/Limine/EFI/Boot/bootx64.efi ::/EFI/BOOT
	mcopy -i $(IMAGE_NAME).hdd@@1M assets/Limine/EFI/Boot/BOOTLOONGARCH64.EFI ::/EFI/BOOT

.PHONY: help run clean gen.clangd menuconfig format check

help:
	$(Q)printf "TKernel Makefile Usage:\n"
	$(Q)printf "  make all         - Build the entire project.\n"
	$(Q)printf "  make run         - Run the Uinxed-x64.iso in QEMU.\n"
	$(Q)printf "  make clean       - Clean all generated files.\n"
	$(Q)printf "  make gen.clangd  - Generate .clangd configuration file.\n"
	$(Q)printf "  make menuconfig  - Run menuconfig to configure the kernel.\n"
	$(Q)printf "  make format      - Format all source files using clang-format.\n"
	$(Q)printf "  make check       - Run static code checks using clang-tidy.\n"
	$(Q)printf "  make help        - Display this help message.\n\n"

run: run_cdrom

run_cdrom: TKernel-test.iso
	$(Q) printf "\033[1;32m[ INFO  ]\033[0m Qemu Serial Output:"
	$(Q) lines=$$(tput lines); \
	for i in $$(seq 1 $$lines); do echo; done	# 避免qemu串口重定向覆盖编译日志
	$(Q) $(QEMU) $(QEMU_FLAGS) $(QEMU_KVM) -cdrom $<
	$(Q) echo

run_db: TKernel-test.iso kerneldump.log
	$(Q) printf "\033[1;32m[ INFO  ]\033[0m Qemu Serial Output:"
	$(Q) lines=$$(tput lines); \
	for i in $$(seq 1 $$lines); do echo; done	# 避免qemu串口重定向覆盖编译日志
	$(Q) $(QEMU) $(QEMU_FLAGS) -no-reboot -d in_asm,int -D qemu.log -cdrom $<
	$(Q) echo

run_gdb: TKernel-test.iso kerneldump.log
	$(Q) printf "\033[1;32m[ INFO  ]\033[0m Qemu Serial Output:"
	$(Q) lines=$$(tput lines); \
	for i in $$(seq 1 $$lines); do echo; done	# 避免qemu串口重定向覆盖编译日志
	$(Q) $(QEMU) $(QEMU_FLAGS) -no-reboot -d in_asm,int -D qemu.log -S -s -cdrom $<
	$(Q) echo

run_smp: TKernel-test.iso
	$(Q) qemu-system-x86_64 $(QEMU_FLAGS) $(QEMU_KVM) -smp $(QEMU_SMP) -cdrom $<

run_hdd_uefi: $(IMAGE_NAME).hdd
	qemu-system-$(CONFIG_ARCH) \
		-hda $(IMAGE_NAME).hdd \
		$(QEMU_FLAGS)

clean:
	$(Q)make clean $(MAKE_FLAGS) -C modules
	$(Q)$(RM) $(OBJS) $(DEPS) kernel.bin TKernel-test.iso initrd.img
	$(Q)printf "\033[1;32m[ Done  ]\033[0m Clean completed.\n\n"

gen.clangd:
	$(Q)$(RM) -f .clangd
	$(Q)echo "# Generated by Makefile" >> .clangd
	$(Q)sed "s/\$${workspaceFolder}/$(subst /,\/,${PWD})/g" .clangd_template >> .clangd
	$(Q)printf "\033[1;32m[ Done  ]\033[0m .clangd configuration generated.\n\n"

menuconfig:
	$(Q)assets/kconfig/mconf Kconfig

nconfig:
	$(Q)assets/kconfig/nconf Kconfig

format: $(C_SOURCES:%=%.fmt) $(S_SOURCES:%=%.fmt) $(HEADERS:%=%.fmt)
	$(Q)printf "\033[1;32m[ Done  ]\033[0m Code Format complete.\n\n"

check: $(C_SOURCES:%=%.tidy) $(S_SOURCES:%=%.tidy) $(HEADERS:%=%.tidy)
	$(Q)printf "\033[1;32m[ Done  ]\033[0m Code Checks complete.\n\n"

-include $(DEPS)

initrd.img: $(MOD_SOURCES)
	$(Q)printf "\033[1;32m[ Build ]\033[0m Building initrd.img ....\n\n"
	$(Q)make initrd.img $(MAKE_FLAGS) -C modules
	
remake: clean TKernel-test.iso

remake_run: remake run
