#include <stdlib.h>
#include <semaphore.h>
#include <stdio.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>

//#include "resourses.h"
#define SEM_NAME "/semaphore1"

int main (int argc, char* argv[]) {
  sem_t* sem;
  if (argc == 2) {
    if ((sem = sem_open(SEM_NAME, 0)) == SEM_FAILED){
      perror("sem_open");
      return -1;
    }
    sem_post(sem);
    printf("semaphore waked up\n");
    return 0; 
  }

  //Another console
  if ((sem = sem_open("/semaphore1", O_CREAT, 0777, 0)) == SEM_FAILED) {
    printf("Error: cannot create semaphore\n");
    return 0;
  }

  printf("semaphore is taken\n");
  if (sem_wait(sem) < 0)
    perror("sem_wait");
  if (sem_close(sem) < 0)
    perror("sem_close");
  printf("semaphore closed\n"); 
  return 0;
}
