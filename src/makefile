src_files := main.c wayland_backend.c bar.c ll.c config.c ipc.c
src_objs  := main.o wayland_backend.o bar.o ll.o config.o ipc.o

util_files := ../utils/config_parser.c ../utils/log.c ../utils/misc.c
util_objs  :=          config_parser.o          log.o          misc.o

include_files := ../include/wlr-layer-shell-unstable-v1.c ../include/xdg-output-unstable-v1.c ../include/xdg-shell-protocol.c
include_objs  :=            wlr-layer-shell-unstable-v1.o            xdg-output-unstable-v1.o            xdg-shell-protocol.o

files := $(src_files) $(util_files) $(include_files)
objs  := $(src_objs)  $(util_objs)  $(include_objs)

includes := -I ../include/ -I /usr/include/pixman-1/ -I ..
libs := -lwayland-client -lm -lrt -lpixman-1 # TODO: replace with pkg-config invokations

debug_opts := -O0 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address

all: bin build_debug clean 
release: bin build_release clean

bin:
	mkdir -p ../bin

build_debug: $(files)
	gcc -std=gnu23 $(debug_opts) $(includes) -c $(files)
	gcc $(objs) -fsanitize=address -o ../bin/main $(libs)

build_release:
	gcc --std=c2x -O2 $(includes) -c $(files)
	gcc $(objs) -o ../bin/main $(libs)

clean:
	rm -f $(objs)
