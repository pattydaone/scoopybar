files :=     main.c wayland_backend.c ../include/xdg-shell-protocol.c ../include/wlr-layer-shell-unstable-v1.c
obj_files := main.o wayland_backend.o            xdg-shell-protocol.o            wlr-layer-shell-unstable-v1.o
includes := ../include

all: bin build clean 

bin:
	mkdir -p ../bin

build: $(files)
	gcc -Wall -g -I $(includes) -c $(files)
	gcc $(obj_files) -o ../bin/main -lwayland-client -lrt

clean:
	rm -f $(obj_files)
