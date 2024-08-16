make: 
	gcc -o fsh fsh.c io.c util.c -Wall -Werror -g

v:
	valgrind -s --leak-check=full ./fsh

run:
	./fsh