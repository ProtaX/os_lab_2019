all: semaphore

semaphore: main.c
	gcc -o semaphore.out main.c -lpthread

printer: printer.c
	gcc -o printer.out printer.c -lpthread

clean:
	rm *.o *.out -f
