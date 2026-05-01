-include ../.config

ModuleName  := Test
ifeq ($(VERBOSE), 1)
  V=
  Q=
else
  V=@printf "\033[1;32m[ Build ]\033[0m $@ ...\n";
  Q=@
endif

CC	:=	$(CONFIG_ARCH)-linux-gnu-gcc
LD	:=	$(CONFIG_ARCH)-linux-gnu-ld

C_SOURCES      := $(shell find arch/$(CONFIG_ARCH) -name "*.c") $(shell find generic -name "*.c")
C_FLAGS        := -Wall -Wextra -fpie -ffreestanding -fno-optimize-sibling-calls -fno-stack-protector -fno-omit-frame-pointer -I ../include -MMD
OBJS           := $(C_SOURCES:%.c=%.o) $(S_SOURCES:%.s=%.o)
LD_FLAGS       := -nostdlib -r
LD					:= ld.lld
DEPS           := $(OBJS:%.o=%.d)

all: libhal-$(CONFIG_ARCH).a

libhal-$(CONFIG_ARCH).a: $(OBJS)
	$(V)$(LD) $(LD_FLAGS) -o $@ $^
	$(Q) cp $@ ../

%.o: %.c
	$(V)$(CC) $(C_FLAGS) -c -o $@ $<

clean: 
	$(Q)$(RM) $(OBJS) $(DEPS) libhal-*.a
