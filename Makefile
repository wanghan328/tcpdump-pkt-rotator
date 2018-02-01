all:
	gcc -g -Wall -Wextra main.c ring_buffer.c -o pkt-rotator
