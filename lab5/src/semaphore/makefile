all: semaphore

semaphore: main.c
	gcc -o semaphore.out main.c -lpthread

printer: semaphore.c
	gcc -o printer.out semaphore.c -lpthread

clean:
	rm *.o *.out -f
