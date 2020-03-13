all: semaphore

semaphore: main.c
	gcc -o semaphore.out main.c -lpthread

printer: printer.c
	gcc -o printer.out -g -O0 printer.c -lpthread

clean:
	rm *.o *.out -f
