# VERBOSE	:= 1

ifeq ($(VERBOSE), 1)
  V=
  Q=
else
  V=@printf "\033[1;32m[Build]\033[0m $@ ...\n";
  Q=@
endif

C_SOURCES	:= $(shell find * -name "*.c")
S_SOURCES	:= $(shell find * -name "*.s")
LD_FLAGS	:= -static -m elf_x86_64 -T ./kernel/linker.lds --allow-multiple-definition
XORRISOFLAGS = -as mkisofs --efi-boot limine/limine-uefi-cd.bin
QEMU_FLAGS := -bios ./assets/OVMF.fd -cdrom tkernel.iso --machine q35 -m 1G 
# QEMU_KVM := -enable-kvm -cpu host
# QEMU_reOUT := > ./qemu.log
QEMU_OUT := -serial stdio $(QEMU_reOUT)
CHECKS         := -quiet -checks=-*,clang-analyzer-*,bugprone-*,cert-*,misc-*,performance-*,portability-*,-misc-include-cleaner,-clang-analyzer-security.insecureAPI.*
C_FLAGS		:= -Wall -Wextra -g3 -O0 -m64 -fno-builtin -fno-pie -fno-stack-protector -fno-sanitize=undefined \
               -mcmodel=large -mno-red-zone -mno-80387 $(C_FPU_MMX_SSE_FLAGS) -msoft-float -I include -MMD -I ./kernel/inc ./kernel
OIB			:= ../tools/bin/oib
UNAME		:= $(shell command -v uname)


initrd.fd: $(C_SOURCES)
	$(Q)$(OIB)

%.o: %.c
	$(V)$(CC) $(C_FLAGS) -c -o $@ $<

%.o: %.s
	$(V)$(AS) $(AS_FLAGS) -o $@ $<
