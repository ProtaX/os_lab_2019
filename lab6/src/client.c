#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <pthread.h>

#include <errno.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>

#include "libnetfac/netfac.h"

#define VERBOSE

struct client_context {
  fac_args_t start_args;
  fac_server_list_t* servers_list;
#define servers_list context.servers_list
  uint32_t servers_num;
#define servers_num context.servers_num

  uint64_t res;
} context;

static pthread_mutex_t context_mtx = PTHREAD_MUTEX_INITIALIZER;

/* Per server task */
static void server_recieve_task(fac_server_t* server) {
  int socket = server->socket;
  char response[sizeof(uint64_t)];
  if (recv(socket, response, sizeof(response), 0) < 0) {
    fprintf(stderr, "Recieve failed\n");
    return;
  }
  close(socket);

  uint64_t answer = 0;
  memcpy(&answer, response, sizeof(uint64_t));
  printf("Answer: %llu\n", answer);

  pthread_mutex_lock(&context_mtx);
  context.res = MultModulo(context.res, answer, context.start_args.mod);
  pthread_mutex_unlock(&context_mtx);
}

/* Join threads and free data */
static void finalize_tasks() {
  fac_server_list_t* iter = servers_list;
  for (int i = 0; iter != NULL; ) {
    if (i < servers_num)
      /* Some servers may be unused 
       * but we have to free memory anyway
       * */
      if (pthread_join(iter->server.thread, NULL) != 0) {
        printf("Error: cannot join %d\n", iter->server.thread);
        exit(1);
      }
#ifdef VERBOSE
    printf("Thread %d joined\n", iter->server.thread);
#endif
    fac_server_list_t* prev = iter;
    iter = iter->next;
    free(prev);
    i++;
  }
}

/* <X.X.X.X>:<PORT> */
static fac_server_list_t* read_servers_file(const char* filename, int* len) {
  if (access(filename, F_OK) == -1) {
    printf("Error: file %s does not exist\n", filename);
    return 0;
  }

  FILE* file = fopen(filename, "r");
  if (!file) {
    printf("Error: cannot open file %s\n", filename);
    return 0;
  }

  fac_server_list_t* first = NULL;
  int i;
  for (i = 0 ;; ++i) {
    fac_server_list_t* head = (fac_server_list_t*)malloc(sizeof(fac_server_list_t));
    head->next = NULL;
    int res = fscanf(file, "%s : %d", head->server.ip, &head->server.port);
    if (res != 2) {
      free(head);
      if (res == EOF)  /* No more strings */
        break;
      fclose(file);  /* Else error occured */
      return 0;
    }
#ifdef VERBOSE
    printf("Got server %s:%d\n", head->server.ip, head->server.port);
#endif
    if (!first)
      first = head;
    else {
      head->next = first->next;
      first->next = head;
    }
  }

  fclose(file);
  *len = i;
  return first;
}

static bool ConvertStringToUI64(const char *str, uint64_t *val) {
  unsigned long long i = strtoull(str, NULL, 10);
  if (errno == ERANGE) {
    fprintf(stderr, "Out of uint64_t range: %s\n", str);
    return false;
  }

  if (errno != 0)
    return false;

  *val = i;
  return true;
}

int main(int argc, char **argv) {
  uint64_t k = -1;
  uint64_t mod = -1;
  context.res = 1;
  /* Domain name max length is 255 bytes */
  /* Linux file name max length is 255 bytes */
  char servers_file[255] = {'\0'};

  while (true) {
    int current_optind = optind ? optind : 1;

    static struct option options[] = {{"k", required_argument, 0, 0},
                                      {"mod", required_argument, 0, 0},
                                      {"servers", required_argument, 0, 0},
                                      {0, 0, 0, 0}};

    int option_index = 0;
    int c = getopt_long(argc, argv, "", options, &option_index);

    if (c == -1)
      break;

    switch (c) {
    case 0: {
      switch (option_index) {
      case 0:
        if (!ConvertStringToUI64(optarg, &k)) {
          printf("Error: bad l value\n");
          return -1;
        }
        break;
      case 1:
        if (!ConvertStringToUI64(optarg, &mod)) {
          printf("Error: bad mod value\n");
          return -1;
        }
        break;
      case 2:
        if (memcpy(servers_file, optarg, strlen(optarg)) != servers_file) {
          printf("memcpy() error\n");
          return -1;
        }
        break;
      default:
        printf("Index %d is out of options\n", option_index);
      }
    } break;

    case '?':
      printf("Arguments error\n");
      break;
    default:
      fprintf(stderr, "getopt returned character code 0%o?\n", c);
    }
  }

  if (k == -1 || mod == -1 || !strlen(servers_file)) {
    fprintf(stderr, "Using: %s --k [1000] --mod 5 --servers /path/to/file\n",
            argv[0]);
    return 1;
  }
  context.start_args.begin = 1;
  context.start_args.end = k+1;
  context.start_args.mod = mod;

  /* Get servers list */
  if ((servers_list = read_servers_file(servers_file, &servers_num)) == 0) {
    printf("Error: cannot read servers file\n");
    return -1;
  }

  if (servers_num > k / 2) {
    servers_num = k / 2;
    printf("Warning: too much servers. Continue with %d\n", servers_num);
  }
#ifdef VERBOSE
  printf("Got server list, len=%d\n", servers_num);
#endif

  /* Send data and wait for results */
  float block = (float)k / servers_num;
  fac_server_list_t* servers_list_item = servers_list;
  for (int i = 0; i < servers_num; i++) {

    fac_server_t* server = &servers_list_item->server;
    /* Prepare package */
    server->args.begin = round(block * (float)i) + 1;
    server->args.end = round(block * (i + 1.f)) + 1;
    server->args.mod = mod;

    /* Prepare socket */
    struct sockaddr_in server_sockaddr = create_sockaddr(server->port, 0);
    if (!inet_aton(server->ip, &server_sockaddr.sin_addr)) {
      printf("Error: cannot translate %s into int value\n", server->ip);
      return -1;
    }

    /* Socket for every server */
    int sck = socket(AF_INET, SOCK_STREAM, 0);
    if (sck < 0) {
      fprintf(stderr, "Socket creation failed!\n");
      exit(1);
    }
    server->socket = sck;
#ifdef VERBOSE
    printf("Socket [%s:%d] created\n", server->ip, server->port);
#endif

    /* Connects socket sck to address server */
    if (connect(sck, (struct sockaddr *)&server_sockaddr, sizeof(struct sockaddr_in)) < 0) {
      fprintf(stderr, "Connection failed\n");
      exit(1);
    }

    /* Send actual data */
    if (send(sck, &server->args, sizeof(fac_args_t), 0) < 0) {
      fprintf(stderr, "Send failed\n");
      exit(1);
    }
#ifdef VERBOSE
    printf("Data sent\n");
#endif

    /* Run task to recieve result */
    if (pthread_create(&server->thread, NULL, (void*)server_recieve_task, (void*)server) != 0) {
      printf("Error: cannot create thread to recieve from server [%s:%p]\n", server->ip, server->port);
      return -1;
    }

    /* Iterate list */
    servers_list_item = servers_list_item->next; 
  }

  finalize_tasks();
  printf("Result: %lu\n", context.res);  /* Threads done - no need in mutex */
  return 0;
}
