#include <stdlib.h>
#include <semaphore.h>
#include <stdio.h>
#include <fcntl.h>
#include <pthread.h>

#include <sys/types.h>
#include <sys/stat.h>

#define SEM_NAME "/semaphore1"

struct paper_list {
  char text[256];
  struct paper_list* next;
  struct paper_list* prev;
};

static struct paper_list paper_list_head = { "\0", NULL };
static struct paper_list* paper_list_done = NULL;
static sem_t* sem;
static pthread_mutex_t sem_mtx = PTHREAD_MUTEX_INITIALIZER;

/* Prints on all sheets and increments the semaphore */
void printer_task() {
  struct paper_list* iter = paper_list_head.next;
  int counter = 1;

  while (iter) {
    pthread_mutex_lock(&sem_mtx);
    sprintf(iter->text, "Hello, list #%d!", counter++);
    /* Move item to done queue */
    struct paper_list* next = iter->next;
    if (!paper_list_done) {
      paper_list_done = iter;
      iter->next->prev = iter->prev;
      iter->prev->next = iter->next;
      paper_list_done->next = NULL;
      paper_list_done->prev = NULL;
    }
    else {
      iter->next = paper_list_done->next;
      iter->prev = paper_list_done;
      iter->next->prev = iter;
      iter->prev->next = iter;
    }

    printf("Printer printed: %s\n", iter->text);
    iter = next;
    pthread_mutex_unlock(&sem_mtx);

    sem_post(sem);
    //sleep(1);
  }
}

/* Takes a ready sheet and prints it on the screen */
void collector_task() {
  int i;
  struct paper_list* iter;
  while(1) {
    sem_wait(sem);
    pthread_mutex_lock(&sem_mtx);
    iter = paper_list_done;

    printf("Collectpr\n");

    if (!iter)
      continue;
    while (iter) {
      printf("Got sheet: %s\n", iter->text);
      /* Remove printed */
      if (iter->next)
        iter->next->prev = iter->prev;
      if (iter->prev)
        iter->prev->next = iter->next;
      /* Iterate */
      iter = iter->next;
    }
    pthread_mutex_unlock(&sem_mtx);
  }
}

/* Creates 10 sheets of paper */
void worker_task() {
  if ((sem = sem_open(SEM_NAME, O_CREAT, 0777, 0)) == SEM_FAILED) {
    perror("sem_open");
    exit(1);
  }

  /* Not a critical section - do not lock sem_mtx */
  int i;
  for (i = 0; i < 10; i++) {
    struct paper_list* item = (struct paper_list*)malloc(sizeof(struct paper_list));
    item->next = paper_list_head.next;
    item->prev = &paper_list_head;
    paper_list_head.next = item;
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
