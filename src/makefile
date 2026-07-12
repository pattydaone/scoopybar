files :=     main.c wayland_backend.c ../include/xdg-shell-protocol.c ../include/wlr-layer-shell-unstable-v1.c ../include/xdg-output-unstable-v1.c \
			 ../include/ll.h
obj_files := main.o wayland_backend.o            xdg-shell-protocol.o            wlr-layer-shell-unstable-v1.o            xdg-output-unstable-v1.o
includes := -I ../include/ -I /usr/include/pixman-1/ 
libs := -lwayland-client -lm -lrt -lpixman-1

all: bin build clean 

bin:
	mkdir -p ../bin

build: $(files)
	gcc -Wall -g $(includes) -c $(files)
	gcc $(obj_files) -o ../bin/main $(libs)

clean:
	rm -f $(obj_files)
