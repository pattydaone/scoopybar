conf:
	gcc -Wall -g -c config_parsing.c ../utils/config_parser.c ../utils/log.c
	gcc config_parsing.o config_parser.o log.o -o ../bin/test
	rm -rf *.o
conf_errs:
	gcc -Wall -g -c config_error_tests.c ../src/config.c ../src/ipc.c ../utils/log.c
	gcc config_error_tests.o config.o ipc.o log.o -o ../bin/test
	rm -rf *.o
run:
	../bin/test
