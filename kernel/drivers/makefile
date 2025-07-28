ifeq ($(VERBOSE), 1)
  V=
  Q=
else
  V=@printf "\033[1;32m[Build]\033[0m $@ ...\n";
  Q=@
endif

cflags := -I ../inc -c -mgeneral-regs-only -mno-sse -mno-mmx -mno-80387 -mno-red-zone -fno-pic -mcmodel=large
C_SOURCES	:= $(shell find * -name "*.c")
OBJS		:= $(C_SOURCES:%.c=%.o) $(S_SOURCES:%.s=%.o)
LD_FLAGS	:= -static -m elf_x86_64 --allow-multiple-definition -r

build: hal.o

hal.o: $(OBJS)
	# gcc -m64 int.c io.c mem.c -I ./inc -c
	# touch hal.o
	ld $(LD_FLAGS) $(OBJS) -o hal.o
	cp ./hal.o ./libhal.a

clean:
	rm $(OBJS)
%.o: %.c
	$(V)$(CC) $(cflags) -c -o $@ $<

# int.o: int.c 
# 	gcc int.c $(cflags) -m64 -I ./inc -c int.o 

# io.o: io.c 
# 	gcc io.c $(cflags) -m64 -I ./inc -c io.o

# mem.o: bitmap.c  page.c hhdm.c 
# 	gcc mem.c $(cflags) -m64 -DAMD64 -I ./inc -c mem.o

# platform.o: platform.c
# 	gcc platform.c $(cflags) -m64 -I ./inc -c platform.o

# heap.o: heap.c
# 	gcc heap.c $(cflags) -m64 -I ../inc -c heap.o

-include $(DEPS)