#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

static pthread_mutex_t mtx_first = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mtx_second = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
  int very_complex_data;
} complex_data_t;

static void some_dumbass_func(complex_data_t* data) {
  printf("Executing some dumbass function...\n");
  pthread_mutex_lock(&mtx_first);
  for (int i = data->very_complex_data + 1; i != data->very_complex_data; )
    i++;
  pthread_mutex_lock(&mtx_second);
  printf("Half of some dumbass function is done\n");
  for (int i = data->very_complex_data - 1; i != data->very_complex_data; )
    i--;
  pthread_mutex_unlock(&mtx_second);
  pthread_mutex_unlock(&mtx_first);
  printf("Some dumbass function done (HOW)\n");
}

static void another_dumbass_func(complex_data_t* data) {
  printf("Executing another dumbass function...\n");
  pthread_mutex_lock(&mtx_second);
  for (int i = data->very_complex_data + 1; i != data->very_complex_data; )
    i++;
  pthread_mutex_lock(&mtx_first);
  printf("Half of another dumbass function is done\n");
  for (int i = data->very_complex_data - 1; i != data->very_complex_data; )
    i--;
  pthread_mutex_unlock(&mtx_first);
  pthread_mutex_unlock(&mtx_second);
  printf("Some another function done (HOW)\n");
}

int main(int argc, char* argv[]) {
  pthread_t t1, t2;
  complex_data_t data = {1};

  if (pthread_create(&t1, NULL, (void*)some_dumbass_func, (void*)&data) != 0) {
    printf("Error: cannot create first thread\n");
    return -1;
  }
  if (pthread_create(&t2, NULL, (void*)another_dumbass_func, (void*)&data) != 0) {
    printf("Error: cannot create second thread\n");
    return -1;
  }

  if (pthread_join(t1, 0) != 0) {
    printf("Error: cannot join first thread\n");
    return -1;
  }

  if (pthread_join(t2, 0) != 0) {
    printf("Error: cannot join second thread\n");
    return -1;
  }
}