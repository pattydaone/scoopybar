conf:
	gcc -Wall -g -c config_parsing.c ../utils/config_parser.c ../utils/log.c
	gcc config_parsing.o config_parser.o log.o -o ../bin/test
	rm -rf *.o
run:
	../bin/test
