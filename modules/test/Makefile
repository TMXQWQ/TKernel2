-include ../../.config

ModuleName  := test
ifeq ($(VERBOSE), 1)
  V=
  Q=
else
  V=@printf "\033[1;32m[ Build ]\033[0m $@ ...\n";
  Q=@
endif



CC	:=	$(CONFIG_ARCH)-linux-gnu-gcc
LD	:=	$(CONFIG_ARCH)-linux-gnu-ld

C_SOURCES      := $(shell find * -name "*.c")
C_FLAGS        = -Wall -fno-pie -Wextra -ffreestanding -fno-optimize-sibling-calls -fno-stack-protector -fno-omit-frame-pointer -I ../../include -MMD -mcmodel=large
OBJS           := $(C_SOURCES:%.c=%.o) $(S_SOURCES:%.s=%.o)
LD_FLAGS       := -nostdlib -e _start -T ../module_default.ld
LD					:= ld.lld
DEPS           := $(OBJS:%.o=%.d)

ifeq ($(CONFIG_$(ModuleName)), m)

all: $(ModuleName).tkm

%.o: %.c
	$(V)$(CC) $(C_FLAGS) -c -o $@ $<

$(ModuleName).tkm:	$(OBJS)
	$(V)$(LD) $(LD_FLAGS) -r -o $@ $^
	$(Q) cp $@ ../

else 
all:

endif
