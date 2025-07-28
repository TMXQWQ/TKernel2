

build: boot.o

# boot.o:
# 	gcc boot.S -I ./ -m64 -c boot.o
# 	cp ./boot.o ../boot.o

clean: boot.o
	rm boot.o

boot.o: boot.c
	gcc boot.c -I ./ -c -m64 -mcmodel=large boot.o
	cp ./boot.o ../boot.o
