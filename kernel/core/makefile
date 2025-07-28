

build: main.o vfs.o

main.o: main.c
	gcc main.c -Wincompatible-pointer-types -m64 -mcmodel=large -mlong-double-64 -mno-80387 -c main.o -I ../inc ./
	cp ./main.o ../main.o

vfs.o: vfs.c
	gcc vfs.c -m64 -mcmodel=large -Wincompatible-pointer-types -mlong-double-64 -mno-80387 -c vfs.o -I ../inc ./
	cp ./vfs.o ../vfs.o

clean: 
	rm main.o
	rm vfs.o
