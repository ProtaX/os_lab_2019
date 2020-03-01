#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <math.h>

#include <sys/file.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <getopt.h>

#include "find_min_max.h"
#include "utils.h"

#define VERBOSE

typedef struct pid_list {
  pid_t pid;
  struct pid_list* next;
}pid_list_t;

pid_list_t pid_list_head = {0, NULL};

void alarm_handler(int signum) {
  pid_list_t* iter = pid_list_head.next;
  pid_list_t* prev = &pid_list_head;

  while(iter) {
    int res = kill(iter->pid, SIGKILL);
#ifdef VERBOSE
    if (res)
      printf("kill() failed, proccess is already dead\n");
    else
      printf("killed %d on timeout\n", iter->pid);
#endif
    prev = iter;
    iter = iter->next;
    free(prev);
  }

  pid_list_head.next = NULL;
}

int main(int argc, char **argv) {
  int seed = -1;
  int array_size = -1;
  int pnum = -1;
  bool with_files = false;
  int timeout = 0;

  while (true) {
    int current_optind = optind ? optind : 1;

    static struct option options[] = {{"seed", required_argument, 0, 0},
                                      {"array_size", required_argument, 0, 0},
                                      {"pnum", required_argument, 0, 0},
                                      {"by_files", no_argument, 0, 'f'},
                                      {"timeout", required_argument, 0, 't'},
                                      {0, 0, 0, 0}};

    int option_index = 0;
    int c = getopt_long(argc, argv, "f", options, &option_index);

    if (c == -1) break;

    switch (c) {
      case 0:
        switch (option_index) {
          case 0:
            seed = atoi(optarg);
            if (!seed) {
							printf("Error: bad seed value\n");
							return -1;
						}
            break;
          case 1:
            array_size = atoi(optarg);
            if (!array_size) {
							printf("Error: bad array size value\n");
							return -1;
						}
            break;
          case 2:
            pnum = atoi(optarg);
            if (!pnum) {
							printf("Error: bad pnum value\n");
							return -1;
						}
            break;
          case 3:
            with_files = true;
            break;

          default:
            printf("Index %d is out of options\n", option_index);
        }
        break;
      case 'f':
        with_files = true;
        break;
      case 't':
        timeout = atoi(optarg);
        if (!timeout) {
          printf("Error: bad timeout value\n");
          return -1;
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

  if (seed == -1 || array_size == -1 || pnum == -1) {
    printf("Usage: %s --seed \"num\" --array_size \"num\" --pnum \"num\" \n",
           argv[0]);
    return 1;
  }

  int *array = malloc(sizeof(int) * array_size);
  GenerateArray(array, array_size, seed);
  int active_child_processes = 0;
  float block = (float)array_size / pnum;
  struct timeval start_time;
  gettimeofday(&start_time, NULL);

	if (pnum > array_size / 2) {
		pnum = ceil(array_size / 2.f);
		printf("Warning: pnum value is too big. Using %d instead\n", pnum);
	}

	FILE *file_min, *file_max;  // direct readonly access to file/pipe
	int min_max_pipe[2][2];  // 0 - min, 1 - max
	if (!with_files) {
		if (pipe(min_max_pipe[0]) < 0 || pipe(min_max_pipe[1]) < 0) {
			printf("Error: cannot create pipe, error code %d\n", errno);
			return -1;
		}
		file_min = fdopen(min_max_pipe[0][0], "r");
		file_max = fdopen(min_max_pipe[1][0], "r");
	}
	
  int min_max_file[2];	// 0 - min, 1 - max
	if (with_files) {
		file_min = fopen("/tmp/parallel_min", "a");
		file_max = fopen("/tmp/parallel_max", "a");
		min_max_file[0] = fileno(file_min);
		min_max_file[1] = fileno(file_max);
		if (!min_max_file[0] || !min_max_file[1]) {
			printf("Error: cannot create output file\n");
			return -1;
		}
	}
  
  signal(SIGALRM, &alarm_handler);  
  alarm((unsigned)timeout);

  for (int i = 0; i < pnum; i++) {
    pid_t child_pid = fork();
    if (child_pid >= 0) {
      if (child_pid > 0) {
        // create new pid list element
        pid_list_t* pid_list_item = (pid_list_t*)malloc(sizeof(pid_list_t));
        pid_list_item->pid = child_pid;
        pid_list_item->next = pid_list_head.next;
        pid_list_head.next = pid_list_item;
#ifdef VERBOSE
        printf("Process %d added %d to kill queue\n", getpid(), child_pid);
#endif
      }

      active_child_processes += 1;
      if (child_pid == 0) {
        // child always ingore its own SIGALRM
        signal(SIGALRM, SIG_IGN);
        int begin = i * round(block);
        int end = begin + round(block);
        if (end > array_size)
          end = array_size;
        if (begin == array_size)
          end = array_size;

        struct MinMax mm = GetMinMax(array, begin, end);
#ifdef VERBOSE
				printf("Process %d, begin=%d end=%d min=%d max=%d\n", getpid(), begin, end, mm.min, mm.max); 
#endif
        char min_to_str[12];
				char max_to_str[12];
				sprintf(min_to_str, "%d ", mm.min);
				sprintf(max_to_str, "%d ", mm.max);

        if (with_files) {
					flock(min_max_file[0], LOCK_EX);
					write(min_max_file[0], (void*)min_to_str, strlen(min_to_str));
					flock(min_max_file[0], LOCK_UN);
					
					flock(min_max_file[1], LOCK_EX);
					write(min_max_file[1], (void*)max_to_str, strlen(max_to_str));
					flock(min_max_file[1], LOCK_UN);
				} else {
          flock(min_max_pipe[0][1], LOCK_EX);
					write(min_max_pipe[0][1], (void*)min_to_str, strlen(min_to_str));
					flock(min_max_pipe[0][1], LOCK_UN);

					flock(min_max_pipe[1][1], LOCK_EX);
					write(min_max_pipe[1][1], (void*)max_to_str, strlen(max_to_str));
					flock(min_max_pipe[1][1], LOCK_UN);
        }
        free(array);
        return 0;
      }

    } else {
      printf("Fork failed!\n");
      return 1;
    }
  }
 
  int status;
  while (active_child_processes > 0) {
    pid_t child = waitpid(0, &status, WNOHANG);
    if (child && (WIFEXITED(status) || WIFSIGNALED(status)))
      active_child_processes--;
  } 
  
  // free memory anyway
  alarm_handler(0);

#ifdef VERBOSE
  printf("All childs are terminated\n");
#endif

  struct MinMax min_max;
  min_max.min = INT_MAX;
  min_max.max = INT_MIN;

	if (with_files) {
		freopen("/tmp/parallel_min", "r", file_min);
		freopen("/tmp/parallel_max", "r", file_max);
	}
	else {
		close(min_max_pipe[0][1]);
		close(min_max_pipe[1][1]);
	}

  for (int i = 0; i < pnum; i++) {
    int min = INT_MAX;
    int max = INT_MIN;

		// Using created FILE structs
		int res = fscanf(file_max, "%d ", &max);
		res += fscanf(file_min, "%d ", &min);

    if (res != 2) {
      printf("Error: cannot read out files or pipes\n");
      return -1;
    }

    if (min < min_max.min) min_max.min = min;
    if (max > min_max.max) min_max.max = max;
  }

	fclose(file_min);
	fclose(file_max);

	if (with_files) {
		remove("/tmp/parallel_min");
		remove("/tmp/parallel_max");
	}

  struct timeval finish_time;
  gettimeofday(&finish_time, NULL);

  double elapsed_time = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
  elapsed_time += (finish_time.tv_usec - start_time.tv_usec) / 1000.0;

  free(array);

  printf("Min: %d\n", min_max.min);
  printf("Max: %d\n", min_max.max);
  printf("Elapsed time: %fms\n", elapsed_time);
  fflush(NULL);
  return 0;
}
