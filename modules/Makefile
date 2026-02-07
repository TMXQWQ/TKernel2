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
-include module_list.txt

test.tkm: test/test.c
	$(V) make test.tkm -C test

test2.tkm: test2/test.c
	$(V) make test2.tkm -C test2

initrd.img: $(C_SOURCES) $(MODULE_LIST)
	$(V)
	find . -maxdepth 1 -name "*.tkm" | cpio --format=newc -o > $@
	cp $@ ../

clean: initrd.img
	rm -f initrd.img $(OBJS) $(DEPS) $(tkms)
