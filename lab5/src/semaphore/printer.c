#include <stdlib.h>
#include <semaphore.h>
#include <stdio.h>
#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>

#define SEM_NAME "/semaphore1"
#define VERBOSE
#define PAUSE_AFTER_SEM_POST

struct paper_list {
  char text[256];
  struct paper_list* next;
};
typedef struct paper_list paper_list_t;

/* Pointer to the top element of blank sheets stack */
static paper_list_t paper_blank_head = { "\0", NULL };
/* Pointer to the top element of printed sheets stack */
static paper_list_t paper_printed_head = { "\0", NULL };

static sem_t* sem;
static pthread_mutex_t sem_mtx = PTHREAD_MUTEX_INITIALIZER;

/* Prints on all sheets and increments the semaphore */
static void printer_task() {
  paper_list_t* iter = paper_blank_head.next;
  int counter = 1;

  while (iter) {
    pthread_mutex_lock(&sem_mtx);
    sprintf(iter->text, "Hello, list #%d!", counter++);

    /* Move to the printed stack */
    paper_blank_head.next = iter->next;
    iter->next = paper_printed_head.next;
    paper_printed_head.next = iter;

#ifdef VERBOSE
    printf("Printer printed: %s\n", iter->text);
#endif

    iter = paper_blank_head.next;
    pthread_mutex_unlock(&sem_mtx);
    sem_post(sem);

#ifdef PAUSE_AFTER_SEM_POST
    sleep(1);
#endif
  }
  sem_post(sem);  /* Signal to finish collector_task */
}

/* Takes a ready sheet and prints it on the screen */
static void collector_task() {
  while (1) {
    sem_wait(sem);  /* ??? */
    /* Signal if printer_task finished */
    if (!paper_printed_head.next) {
#ifdef VERBOSE
      printf("Printer stack is empty, collector_task finished\n");
#endif
      return;
    }

    pthread_mutex_lock(&sem_mtx);
    paper_list_t* iter = paper_printed_head.next;

    /* Iterate printer stack and print on the screen */
    while (iter) {
      printf("Collector got: %s\n", iter->text);
      iter = iter->next;
      free(paper_printed_head.next);
      paper_printed_head.next = iter;
    }
    pthread_mutex_unlock(&sem_mtx);
  }
}

/* Creates 10 sheets of paper */
static void worker_task() {
  if ((sem = sem_open(SEM_NAME, O_CREAT, 0777, 0)) == SEM_FAILED) {
    perror("sem_open");
    exit(1);
  }

  /* Not a critical section - do not lock sem_mtx */
  int i;
  for (i = 0; i < 10; i++) {
    struct paper_list* item = (struct paper_list*)malloc(sizeof(struct paper_list));
    item->next = paper_blank_head.next;
    paper_blank_head.next = item;
  }
}

int main (int argc, char* argv[]) {
  pthread_t collector, printer;

  /* Load printer */
  worker_task();
  /* Print */
  if (pthread_create(&printer, NULL, (void*)&printer_task, NULL) != 0) {
    perror("pthread_create");
    return 1;
  }
  /* Collect sheets */
  if (pthread_create(&collector, NULL, (void*)&collector_task, NULL) != 0) {
    perror("pthread_create");
    return 1;
  }

  /* Join threads */
  if (pthread_join(printer, 0) != 0) {
    perror("pthread_join");
    return 1;
  }
  if (pthread_join(collector, 0) != 0) {
    perror("pthread_join");
    return 1;
  }

  /* Close semaphore */
  if (sem_close(sem) < 0)
    perror("sem_close");
  printf("semaphore closed\n"); 
  return 0;
}
