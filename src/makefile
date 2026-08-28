src_files := main.c wayland_backend.c bar.c config.c ipc.c message_handler.c
src_objs  := main.o wayland_backend.o bar.o config.o ipc.o message_handler.o

util_files := ../utils/config_parser.c ../utils/log.c ../utils/misc.c ../utils/ll.c
util_objs  :=          config_parser.o          log.o          misc.o          ll.o

include_files := ../include/wlr-layer-shell-unstable-v1.c ../include/xdg-output-unstable-v1.c ../include/xdg-shell-protocol.c
include_objs  :=            wlr-layer-shell-unstable-v1.o            xdg-output-unstable-v1.o            xdg-shell-protocol.o

files := $(src_files) $(util_files) $(include_files)
objs  := $(src_objs)  $(util_objs)  $(include_objs)

includes := -I../include/ $(shell pkg-config --cflags pixman-1) -I..
libs := $(shell pkg-config --libs wayland-client) $(shell pkg-config --libs pixman-1) -lrt

debug_opts := -O0 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address

all: bin build_debug clean 
release: bin build_release clean

bin:
	mkdir -p ../bin

build_debug: $(files)
	gcc -std=gnu23 $(debug_opts) $(includes) -c $(files)
	gcc $(objs) -fsanitize=address -o ../bin/main $(libs)

build_release:
	gcc -std=gnu23 -O2 $(includes) -c $(files)
	gcc $(objs) -o ../bin/main $(libs)

clean:
	rm -f $(objs)
