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
OIB			:= $(shell find -name oib)
UNAME		:= $(shell command -v uname)


info: 
	@printf "\033[1;32m[INFO]\033[0m";
	@printf "Thanks for using Tkernel!\n"
	@printf "\033[1;32m[INFO]\033[0m";
	@printf "Version:v2.0.0\n"
	@printf "\033[1;32m[INFO]\033[0m";
	@printf "TMX 保留所有权利\n"


vkernel.bin: $(C_SOURCES) $(S_SOURCES)
	make kernel.bin -C kernel

kernel.bin: vkernel.bin
	ld vkernel.bin $(LD_FLAGS) -o kernel.bin

servers: $(C_SOURCES) $(S_SOURCES)
	make servers -C servers

apps: $(C_SOURCES) $(S_SOURCES)
	make build -C apps

kerneldump.log: kernel.bin
	$(V)objdump -d kernel.bin > kerneldump.log & #耗时太久，后台写入

clean:	
	$(V)make clean -C kernel
	$(V)rm initrd.img
	$(V)rm  -f ./kernel.bin ./tkernel.iso ./vkernel.bin

tkernel.iso: kernel.bin kerneldump.log initrd.img
	$(Q)mkdir -p iso
	$(Q)cp -r ./assets/rootfs/* ./iso/
	$(Q)cp ./kernel.bin ./iso/kernel
	$(Q)cp ./initrd.img ./iso
	$(Q)touch ./tkernel.iso
	$(Q)xorriso $(XORRISOFLAGS) ./iso -o ./tkernel.iso \
	  2> /dev/null
	$(V)rm -rf ./iso
	

run: tkernel.iso
	qemu-system-x86_64 -enable-kvm -cpu host $(QEMU_FLAGS) $(QEMU_OUT)

run_db: tkernel.iso
	qemu-system-x86_64 $(QEMU_KVM) $(QEMU_FLAGS) -no-reboot -d in_asm,int -D qemu.log

run_gdb: tkernel.iso
	qemu-system-x86_64 $(QEMU_KVM) $(QEMU_FLAGS) -no-reboot -serial stdio -S -s -d in_asm,int -D qemu.log

format: $(C_SOURCES:%=%.fmt) $(S_SOURCES:%=%.fmt) $(HEADERS:%=%.fmt)
	$(Q)printf "\033[1;32m[Done]\033[0m Code Format complete.\n\n"


%.fmt: %
	$(Q)printf "\033[1;32m[Format]\033[0m $< ...\n"
	$(Q)clang-format -i $<

%.tidy: %
	$(Q)printf "\033[1;32m[Checks]\033[0m $< ...\n"
	$(Q)clang-tidy $< $(CHECKS) -- $(C_FLAGS)


check: $(C_SOURCES:%=%.tidy) $(S_SOURCES:%=%.tidy) $(HEADERS:%=%.tidy)
	$(Q)printf "\033[1;32m[Done]\033[0m Code Checks complete.\n\n"

initrd.img: $(C_SOURCES)
	$(Q)mkdir -p ./initrd
	$(V)find ./initrd -depth | cpio -ov -H newc > initrd.img
	$(Q)rm -r ./initrd


# tools/bin/oib:
# 	mkdir -p ./tools/bin
# ifndef OIB
# 	$(Q)printf "\033[1;32m[ERROR]\033[0m No OIB installed! ...\n";
# 	$(Q)echo "now try download from internet"
# 	$(Q)mkdir -p tmp
# ifndef UNAME
# 	$(Q)curl https://github.com/wenxuanjun/oib/releases/download/v0.3.0/oib-x86_64-pc-windows-msvc.zip -o ./tmp/oib.zip
# 	$(Q)7z x ./tmp/oib.zip
# else
# 	$(Q)curl -L -v https://github.com/wenxuanjun/oib/releases/download/v0.3.0/oib-x86_64-unknown-linux-gnu.tar.gz -A "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36" -o ./tmp/oib.tar.gz
# 	$(Q)tar -zxvf ./tmp/oib.tar.gz oib-x86_64-unknown-linux-gnu/oib
# 	$(Q)cp ./oib-x86_64-unknown-linux-gnu/oib ./tools/bin
# 	$(Q)rm -r oib-x86_64-unknown-linux-gnu
# 	$(Q)rm -r tmp
# 	$(Q)chmod +x tools/bin/oib
# 	OIB = $(shell find -name oib)
# endif
# endif
