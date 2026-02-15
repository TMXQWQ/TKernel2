-include ../../.config

ModuleName  := test2
ifeq ($(VERBOSE), 1)
  V=
  Q=
else
  V=@printf "\033[1;32m[ Build ]\033[0m $@ ...\n";
  Q=@
endif

ifeq ($(CONFIG_$(ModuleName)), m)


C_SOURCES      := $(shell find * -name "*.c")
C_FLAGS        := -Wall -Wextra -O0 -g3 -m64 -fpie -ffreestanding -fno-optimize-sibling-calls -fno-stack-protector -fno-omit-frame-pointer -mstackrealign -mno-red-zone -I ../../include -MMD
OBJS           := $(C_SOURCES:%.c=%.o) $(S_SOURCES:%.s=%.o)
LD_FLAGS       := -nostdlib -m elf_x86_64 -e _start
LD					:= ld


all: $(ModuleName).tkm

%.o: %.c
	$(V)$(CC) $(C_FLAGS) -c -o $@ $<

$(ModuleName).tkm:	$(OBJS)
	$(V)$(LD) $(LD_FLAGS) -o $@ $^
	$(Q) cp $@ ../

endif

ifneq ($(CONFIG_$(ModuleName)), m)

all:
	$(Q)printf "\033[1;32m[ INFO  ]\033[0m CONFIG_$(ModuleName)!=m.\n";
	$(Q)printf "\033[1;32m[ INFO  ]\033[0m Skipped Module $(ModuleName).\n";

endif
