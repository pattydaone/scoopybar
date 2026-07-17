files :=     main.c wayland_backend.c ../include/xdg-shell-protocol.c ../include/wlr-layer-shell-unstable-v1.c ../include/xdg-output-unstable-v1.c \
			 ll.c ../utils/log.c      ../utils/config_parser.c
obj_files := main.o wayland_backend.o            xdg-shell-protocol.o            wlr-layer-shell-unstable-v1.o            xdg-output-unstable-v1.o \
 			 ll.o          log.o               config_parser.o
includes := -I ../include/ -I /usr/include/pixman-1/
libs := -lwayland-client -lm -lrt -lpixman-1 # TODO: replace with pkg-config invokations

all: bin build clean 

bin:
	mkdir -p ../bin

build: $(files)
	gcc -Wall -g $(includes) -c $(files)
	gcc $(obj_files) -o ../bin/main $(libs)

clean:
	rm -f $(obj_files)
