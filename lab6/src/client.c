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

#include "libnetfac/netfac.h"

#define VERBOSE

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
  uint64_t res = 1;
  /* Domain name max length is 255 bytes */
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
    fprintf(stderr, "Using: %s --k 1000 --mod 5 --servers /path/to/file\n",
            argv[0]);
    return 1;
  }

  unsigned int servers_num;
  fac_server_list_t* servers_list;
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

  float block = (float)k / servers_num;
  int i = 0;
  for (fac_server_list_t* servers_list_item = servers_list; servers_list_item != NULL; ) {

    fac_args_t package = {
      .begin = round(block * (float)i) + 1,
      .end = round(block * (i + 1.f)) + 1,
      .mod = mod
    };

    fac_server_t* s_data = &servers_list_item->server;
    struct hostent *hostname = gethostbyname(s_data->ip);
    if (hostname == NULL) {
      fprintf(stderr, "gethostbyname failed with %s\n", s_data->ip);
      exit(1);
    }

    struct sockaddr_in server = {
      .sin_family = AF_INET, /* Always */
      .sin_port = htons(s_data->port), /* Host to network short */
      .sin_addr.s_addr = *((unsigned long *)hostname->h_addr), /* Why no htonl? */
      .sin_zero = {0}
    };

    /* Wy do we open socket every time? */
    int sck = socket(AF_INET, SOCK_STREAM, 0);
    if (sck < 0) {
      fprintf(stderr, "Socket creation failed!\n");
      exit(1);
    }
#ifdef VERBOSE
    printf("Socket [%s:%d] created\n", s_data->ip, s_data->port);
#endif

    /* Connects socket sck to address server */
    // TODO: if failed, continue?
    if (connect(sck, (struct sockaddr *)&server, sizeof(server)) < 0) {
      fprintf(stderr, "Connection failed\n");
      exit(1);
    }

    if (send(sck, &package, sizeof(package), 0) < 0) {
      fprintf(stderr, "Send failed\n");
      exit(1);
    }
#ifdef VERBOSE
    printf("Data sent\n");
#endif

    //TODO: make async
    /* No address check? */
    char response[sizeof(uint64_t)];
    if (recv(sck, response, sizeof(response), 0) < 0) {
      fprintf(stderr, "Recieve failed\n");
      exit(1);
    }

    uint64_t answer = 0;
    memcpy(&answer, response, sizeof(uint64_t));
    printf("answer: %llu\n", answer);
    res = MultModulo(res, answer, mod);

    /* End of for statement */
    fac_server_list_t* prev_server = servers_list_item;
    servers_list_item = servers_list_item->next;
    free(prev_server);
    i++;

    close(sck);
  }

  printf("Result: %lu\n", res);
  return 0;
}
