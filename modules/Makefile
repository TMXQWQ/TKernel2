-include ../.config
-include module_list.txt

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

# test.tkm: $(C_SOURCES)
# 	$(V) make all -C test

# test2.tkm: $(C_SOURCES)
# 	$(V) make all -C test2

%.tkm: $(shell find $* -name "*.c")
	$(V) make all -C $*

initrd.img: $(C_SOURCES) $(MODULE_LIST)
	$(V)
	$(Q) find . -maxdepth 1 -name "*.tkm" | cpio --format=newc -o > $@ 2> /dev/null
	$(Q) cp $@ ../

clean: initrd.img
	$(V) rm -f initrd.img $(OBJS) $(DEPS) $(tkms)
