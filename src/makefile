files :=     main.c wayland_backend.c ../include/xdg-shell-protocol.c
obj_files := main.o wayland_backend.o xdg-shell-protocol.o
includes := ../include

all: bin build clean 

bin:
	mkdir -p ../bin

build: $(files)
	gcc -Wall -g -I $(includes) -c $(files)
	gcc $(obj_files) -o ../bin/main -lwayland-client -lrt

clean:
	rm -f $(obj_files)
