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
QEMU_FLAGS := -bios ./assets/OVMF.fd -cdrom tkernel.iso
# QEMU_KVM := -enable-kvm -cpu host
# QEMU_reOUT := > ./qemu.log
QEMU_OUT := -serial stdio $(QEMU_reOUT)
CHECKS         := -quiet -checks=-*,clang-analyzer-*,bugprone-*,cert-*,misc-*,performance-*,portability-*,-misc-include-cleaner,-clang-analyzer-security.insecureAPI.*
C_FLAGS		:= -Wall -Wextra -g3 -O0 -m64 -fno-builtin -fno-pie -fno-stack-protector -fno-sanitize=undefined \
               -mcmodel=large -mno-red-zone -mno-80387 $(C_FPU_MMX_SSE_FLAGS) -msoft-float -I include -MMD -I ./kernel/inc ./kernel

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
	make clean -C kernel
	rm ./kernel.bin ./tkernel.iso ./vkernel.bin

tkernel.iso: kernel.bin kerneldump.log
	@mkdir -p iso
	@cp -r ./assets/rootfs/* ./iso/
	@cp ./kernel.bin ./iso/kernel
	@touch ./tkernel.iso
	xorriso $(XORRISOFLAGS) ./iso -o ./tkernel.iso \
	#  2> /dev/null
	rm -rf ./iso
	

run: tkernel.iso
	qemu-system-x86_64 -enable-kvm -cpu host $(QEMU_FLAGS) $(QEMU_OUT)

run_db: tkernel.iso
	qemu-system-x86_64 $(QEMU_KVM) $(QEMU_FLAGS) -no-reboot -d in_asm,int 

run_gdb: tkernel.iso
	qemu-system-x86_64 $(QEMU_KVM) $(QEMU_FLAGS) -no-reboot -serial stdio -S -s

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