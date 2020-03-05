#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <getopt.h>
#include <math.h>

static uint64_t res = 1;
static pthread_mutex_t fac_mtx = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
  pthread_t thread;
  uint64_t begin;
  uint64_t end;
  uint32_t mod;
} fac_thread_t;

static uint64_t MultModulo(uint64_t a, uint64_t b, uint64_t mod) {
  uint64_t result = 0;
  a = a % mod;

  while (b > 0) {
    if (b % 2 == 1)
      result = (result + a) % mod;
    a = (a * 2) % mod;
    b /= 2;
  }

  return result % mod;
}

static void fac_threaded(fac_thread_t* f) {
  printf("Thread: id=%lu from %lu to %lu\n", f->thread, f->begin, f->end - 1);
  for (uint64_t i = f->begin; i < f->end; i++) {
    pthread_mutex_lock(&fac_mtx);
    res = MultModulo(res, i, f->mod);
    if (!res) {
      printf("Error: uint64_t overflow (i=%lu)\n", i);
      pthread_mutex_unlock(&fac_mtx);
      return;
    }
    pthread_mutex_unlock(&fac_mtx);
  }
}

int main(int argc, char* argv[]) {
  uint64_t k = 0;
  uint32_t pnum = 0;
  uint32_t mod = 0;

  while (1) {
    int current_optind = optind ? optind : 1;

    static struct option options[] = {{"k", required_argument, 0, 0},
                                      {"pnum", required_argument, 0, 0},
                                      {"mod", required_argument, 0, 0},
                                      {}};

    int option_index = 0;
    int c = getopt_long(argc, argv, "", options, &option_index);

    if (c == -1) break;

    switch (c) {
      case 0:
        switch (option_index) {
          case 0:
            k = atoi(optarg);
            if (!k) {
              printf("Error: bad k value\n");
              return -1;
            }
            break;
          case 1:
            pnum = atoi(optarg);
            if (!pnum) {
              printf("Error: bad pnum value\n");
              return -1;
            }
            break;
          case 2:
            mod = atoi(optarg);
            if (!mod) {
              printf("Error: bad mod value\n");
              return -1;
            }
            break;
          default:
            printf("Index %d is out of options\n", option_index);
        }
        break;
      case '?':
        break;

      default:
        printf("getopt returned character code 0%o?\n", c);
    }
  }

  if (optind < argc) {
    printf("Has at least one no option argument\n");
    return 1;
  }

  if (k == -1 || pnum == -1 || mod == -1) {
    printf("Usage: %s --k \"num\" --mod \"num\" --pnum \"num\" \n",
           argv[0]);
    return 1;
  }

  if (pnum > k / 2) {
    pnum = k / 2;
    printf("Warning: too much threads. Continue with %d\n", pnum);
  }

  float block = (float)k / pnum;
  fac_thread_t thread_pool[pnum];

  for (uint32_t i = 0; i < pnum; i++) {
    int begin = round(block * (float)i) + 1;
    int end = round(block * (i + 1.f)) + 1;

    thread_pool[i].begin = begin;
    thread_pool[i].end = end;
    thread_pool[i].mod = mod;

    if (pthread_create(&thread_pool[i].thread, NULL, (void *)fac_threaded, (void*)&thread_pool[i]) != 0) {
      printf("Error: cannot create new pthread\n");
      return -1;
    }
  }

  for (uint32_t i = 0; i < pnum; i++)
    if (pthread_join(thread_pool[i].thread, 0) != 0) {
      printf("Error: cannot join thread %d\n", i);
      return -1;
    }

  if (res)
    printf("%lu! mod %d = %lu\n", k, mod, res);
}