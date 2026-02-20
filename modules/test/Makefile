-include ../../.config

ModuleName  := Test
ifeq ($(VERBOSE), 1)
  V=
  Q=
else
  V=@printf "\033[1;32m[ Build ]\033[0m $@ ...\n";
  Q=@
endif

ifeq ($(CONFIG_$(ModuleName)), m)


CC	:=	$(CONFIG_ARCH)-linux-gnu-gcc
LD	:=	$(CONFIG_ARCH)-linux-gnu-ld

C_SOURCES      := $(shell find * -name "*.c")
C_FLAGS        := -Wall -Wextra -g3 -fpie -ffreestanding -fno-optimize-sibling-calls -fno-stack-protector -fno-omit-frame-pointer -I ../../include -MMD
OBJS           := $(C_SOURCES:%.c=%.o) $(S_SOURCES:%.s=%.o)
LD_FLAGS       := -nostdlib -e _start -T ../module_default.ld
LD					:= ld.lld


all: test.tkm

%.o: %.c
	$(V)$(CC) $(C_FLAGS) -c -o $@ $<

test.tkm:	$(OBJS)
	$(V)$(LD) $(LD_FLAGS) -o $@ $^
	$(Q) cp $@ ../

endif

ifneq ($(CONFIG_$(ModuleName)), m)

all:
	$(Q)printf "\033[1;32m[ INFO  ]\033[0m CONFIG_$(ModuleName)!=m.\n";
	$(Q)printf "\033[1;32m[ INFO  ]\033[0m Skipped Module $(ModuleName).\n";

endif
