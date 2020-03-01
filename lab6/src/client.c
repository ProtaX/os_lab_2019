#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <errno.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/types.h>

struct Server {
  char ip[255];
  int port;
};

typedef struct {
  server_t server;
  server_list_t* next;
} server_list_t;

typedef struct Server server_t;

static int read_servers_file(const char* filename, server_list_t* arr, int* len) {
  if (access(filename, F_OK) == -1) {
    printf("Error: file %s does not exist\n", filename);
    return -1;
  }

  FILE* file = fopen(filename, "rt");
  if (!file) {
    printf("Error: cannot open file %s\n", filename);
    return -1;
  }
 
  server_list_t* head = arr;
  int i;
  for (i = 0 ;; ++i) {
    if (i == 255) {
      printf("Warning: cannot operate with more than 255 servers\n");
      break;
    }
    head = (server_list_t*)malloc(sizeof(server_list_t));
    head->next = NULL;
    if (fscanf(filename, "%s:%d\n", head->server.ip, head->server.port) != 2) {
      free(head);
      break;
    }
    head->next = arr->next;
    arr->next = head;
  }

  fclose(file);
  *len = i;
  return 0;
}

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
  /* Netmask is 255.255.255.0 */
  /* Should`nt it be 254 ? */
  char servers[255] = {'\0'};

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
        char* res = memcpy(servers, optarg, strlen(optarg));
        if (res != servers) {
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

  if (k == -1 || mod == -1 || !strlen(servers)) {
    fprintf(stderr, "Using: %s --k 1000 --mod 5 --servers /path/to/file\n",
            argv[0]);
    return 1;
  }

  // TODO: for one server here, rewrite with servers from file
  unsigned int servers_num;
  server_list_t* servers_list;
  if (read_server_file(servers, servers_list, &servers_num) == -1) {
    printf("Error: cannot read servers file\n");
    return -1;
  }

  // TODO: work continiously, rewrite to make parallel
  for (int i = 0; i < servers_num; i++) {
    server_t s* = &servers_list[i].server;
    struct hostent *hostname = gethostbyname(s.ip);
    if (hostname == NULL) {
      fprintf(stderr, "gethostbyname failed with %s\n", s.ip);
      exit(1);
    }

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(s.port);
    server.sin_addr.s_addr = *((unsigned long *)hostname->h_addr);

    int sck = socket(AF_INET, SOCK_STREAM, 0);
    if (sck < 0) {
      fprintf(stderr, "Socket creation failed!\n");
      exit(1);
    }

    /* Connects socket sck to address server */
    // TODO: if failed, continue?
    if (connect(sck, (struct sockaddr *)&server, sizeof(server)) < 0) {
      fprintf(stderr, "Connection failed\n");
      exit(1);
    }

    // TODO: for one server
    // parallel between servers
    uint64_t begin = 1;
    uint64_t end = k;

    char task[sizeof(uint64_t) * 3];
    memcpy(task, &begin, sizeof(uint64_t));
    memcpy(task + sizeof(uint64_t), &end, sizeof(uint64_t));
    memcpy(task + 2 * sizeof(uint64_t), &mod, sizeof(uint64_t));

    if (send(sck, task, sizeof(task), 0) < 0) {
      fprintf(stderr, "Send failed\n");
      exit(1);
    }

    char response[sizeof(uint64_t)];
    if (recv(sck, response, sizeof(response), 0) < 0) {
      fprintf(stderr, "Recieve failed\n");
      exit(1);
    }

    // TODO: from one server
    // unite results
    uint64_t answer = 0;
    memcpy(&answer, response, sizeof(uint64_t));
    printf("answer: %llu\n", answer);

    close(sck);
  }
  free(servers_list);

  return 0;
}
