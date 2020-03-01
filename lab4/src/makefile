all: task1 task3

task1:
	make -F ./task1/makefile

task3: process_memory.c
	gcc -o process_memory.out process_memory.c

clean:
	rm -rf *.out *.o -f
