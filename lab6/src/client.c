#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

#include <errno.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/types.h>

#define VERBOSE

struct fac_server {
  char ip[255];
  int port;
};

struct fac_server_list {
  struct fac_server server;
  struct fac_server_list* next;
};

typedef struct fac_server fac_server_t;
typedef struct fac_server_list fac_server_list_t;

static fac_server_list_t* read_servers_file(const char* filename, fac_server_list_t* arr, int* len) {
  
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
    if (i == 255) {
      printf("Warning: cannot operate with more than 255 servers\n");
      break;
    }
    fac_server_list_t* head = (fac_server_list_t*)malloc(sizeof(fac_server_list_t));
    head->next = NULL;
    int res = fscanf(file, "%s : %d", head->server.ip, &head->server.port);
    if (res != 2) {
      free(head);
      /* No more strings */
      if (res == EOF)
        break;
      /* Else error occured */
      fclose(file);
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
  //arr = first;
  return first;
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
  uint64_t res = 1;
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
        if (memcpy(servers, optarg, strlen(optarg)) != servers) {
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
  fac_server_list_t* servers_list;
  if ((servers_list = read_servers_file(servers, servers_list, &servers_num)) == 0) {
    printf("Error: cannot read servers file\n");
    return -1;
  }
#ifdef VERBOSE
  printf("Got server list, len=%d\n", servers_num);
#endif
  float block = (float)k / servers_num;
  // TODO: work continiously, rewrite to make parallel
  for (int i = 0; i < servers_num; i++) {
    uint64_t begin = round(block * (float)i) + 1;
    uint64_t end = round(block * (i + 1.f)) + 1;

    fac_server_t* s = &servers_list->server;
    struct hostent *hostname = gethostbyname(s->ip);
    if (hostname == NULL) {
      fprintf(stderr, "gethostbyname failed with %s\n", s->ip);
      exit(1);
    }

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(s->port);
    server.sin_addr.s_addr = *((unsigned long *)hostname->h_addr);
    
    /* Wy do we open socket every time? */
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

    char task[sizeof(uint64_t) * 3];
    memcpy(task, &begin, sizeof(uint64_t));
    memcpy(task + sizeof(uint64_t), &end, sizeof(uint64_t));
    memcpy(task + 2 * sizeof(uint64_t), &mod, sizeof(uint64_t));

    if (send(sck, task, sizeof(task), 0) < 0) {
      fprintf(stderr, "Send failed\n");
      exit(1);
    }

    //TODO: make async
    char response[sizeof(uint64_t)];
    if (recv(sck, response, sizeof(response), 0) < 0) {
      fprintf(stderr, "Recieve failed\n");
      exit(1);
    }

    uint64_t answer = 0;
    memcpy(&answer, response, sizeof(uint64_t));
    printf("answer: %llu\n", answer);
    res *= answer;
    servers_list = servers_list->next;
    close(sck);
  }
  free(servers_list);
  
  printf("Result: %lu\n", res);
  return 0;
}
