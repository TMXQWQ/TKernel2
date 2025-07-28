# VERBOSE := 1


# "-b sys/limine-bios-cd.bin -no-emul-boot -boot-info-table"
ifeq ($(VERBOSE), 1)
  V=
  Q=
else
  V=@printf "\033[1;32m[Build]\033[0m $@ ...\n";
  Q=@
endif

C_SOURCES	:= $(shell find * -name "*.c")
S_SOURCES	:= $(shell find * -name "*.s")
OBJS		:= $(C_SOURCES:%.c=%.o) $(S_SOURCES:%.s=%.o)
DEPS		:= $(OBJS:%.o=%.d)
LIBS		:= $(wildcard libs/lib*.a)

C_FPU_MMX_SSE_FLAGS :=  -mno-mmx -mno-sse -mno-sse2 

C_FLAGS		:= -Wall -Wextra -g3 -O0 -m64 -fno-builtin -fno-pie -fno-stack-protector -fno-sanitize=undefined \
               -mcmodel=large -mno-red-zone -mno-80387 $(C_FPU_MMX_SSE_FLAGS) -msoft-float -I include -MMD -I ./inc ./
LD_FLAGS	:= -static -m elf_x86_64 -T ./linker.lds --allow-multiple-definition
AS_FLAGS	:= -g --64


%.o: %.c
	$(V)$(CC) $(C_FLAGS) -c -o $@ $<

%.o: %.s
	$(V)$(AS) $(AS_FLAGS) -o $@ $<

all: tkernel.iso
	@echo "Complete!"

kernel.bin: $(OBJS) $(LIBS)
	# cp assets/*.o ./
	# ld boot.o main.o hal.o loader.o vfs.o liballoc-x86_64.o libos_terminal-embedfont-x86_64.o -b elf64-x86-64 -T ./linker.lds -z muldefs -o kernel.bin
	$(V) $(LD) $(LD_FLAGS) -r -o $@ $^
	cp ./kernel.bin ../vkernel.bin



clean:
	$(V)rm $(DEPS)
	$(V)rm $(OBJS)
	$(V)rm -rf ./iso
	$(V)rm kernel.bin
	# $(V)rm tkernel.iso

loader.o:
	make -C module/loader clean
	make -C module/loader build

rebuild:
	make clean
	make run



-include $(DEPS)