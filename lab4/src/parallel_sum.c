#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <math.h>
#include <pthread.h>
#include "task1/utils.h"

struct SumArgs {
  int *array;
  int begin;
  int end;
};

int Sum(const struct SumArgs *args) {
  int sum = 0;
  for (int i = args->begin; i < args->end; i++)
    sum += args->array[i];
  //printf("thread sum=%d from %d to %d\n", sum, args->begin, args->end);
  return sum;
}

void *ThreadSum(void *args) {
  struct SumArgs *sum_args = (struct SumArgs *)args;
  return (void *)(size_t)Sum(sum_args);
}

int main(int argc, char* argv[]) {
  uint32_t threads_num = 2;
  uint32_t array_size = 10;
  uint32_t seed = 1;
  pthread_t threads[threads_num];
 
  //TODO: parse command line args
  /*if (argc > 1)
    seed = atoi(argv[1]);
  if (argc > 2)
    array_size = atoi(argv[2]);
  if (argc > 3)
    threads_num = atoi(argv[3]);
  else
    printf("bad args\n");
  */
  int *array = malloc(sizeof(int) * array_size);
  GenerateArray(array, array_size, seed);

  struct SumArgs args[threads_num];
  for (uint32_t i = 0; i < threads_num; i++) {
    args[i].begin = i * floor((float)array_size / threads_num);
    args[i].end = args[i].begin + ceil((float)array_size / threads_num);
    args[i].array = array;

    if (pthread_create(&threads[i], NULL, ThreadSum, (void *)&args[i])) {
      printf("Error: pthread_create failed!\n");
      return 1;
    }
  }

  int total_sum = 0;
  for (uint32_t i = 0; i < threads_num; i++) {
    int sum = 0;
    pthread_join(threads[i], (void **)&sum);
    total_sum += sum;
  }

  free(array);
  printf("Total: %d\n", total_sum);
  return 0;
}
