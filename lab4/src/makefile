all: task1 task3

task5: ./task1/utils.c parallel_sum.c
	gcc parallel_sum.c ./task1/utils.c -std=gnu99 -o parallel_sum.out -lm -lpthread -I./task1

task1:
	make -F ./task1/makefile

task3: process_memory.c
	gcc -o process_memory.out process_memory.c

clean:
	rm -rf *.out *.o -f
