#ifndef SUM_H
#define SUM_H

struct SumArgs {
  int *array;
  int begin;
  int end;
};

typedef struct SumArgs sum_args_t;

int Sum(const sum_args_t* args);

#endif