ifeq ($(VERBOSE), 1)
  V=
  Q=
else
  V=@printf "\033[1;32m[ Build ]\033[0m $@ ...\n";
  Q=@
endif

C_SOURCES      := $(shell find * -name "*.c")
tkms      := $(shell find * -name "*.tkm")
OBJS           := $(C_SOURCES:%.c=%.o) $(S_SOURCES:%.s=%.o)
DEPS           := $(OBJS:%.o=%.d)

test.tkm: test/test.c
	make $@ -C test

initrd.img: $(C_SOURCES) test.tkm
	find . -maxdepth 1 -name "*.tkm" | cpio --format=newc -o > $@
	cp $@ ../

clean: initrd.img
	rm initrd.img $(OBJS) $(DEPS) $(tkms)
