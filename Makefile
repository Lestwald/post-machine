post-machine.exe : post-machine.o
	gcc -o post-machine post-machine.o

post-machine.o : post-machine.c
	gcc -std=c11 -pedantic -Wall -Wextra -c -o post-machine.o post-machine.c