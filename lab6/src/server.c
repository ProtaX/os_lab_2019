#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <errno.h>
#include <pthread.h>

#include <getopt.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "libnetfac/netfac.h"

static uint64_t Factorial(const fac_args_t* args) {
  uint64_t ans = args->begin;

  for (uint64_t i = args->begin + 1; i < args->end; i++)
    ans = MultModulo(ans, i, args->mod);

  return ans;
}

static void *ThreadFactorial(void *args) {
  fac_args_t* fargs = (fac_args_t*)args;
  return (void *)(uint64_t *)Factorial(fargs);
}

int main(int argc, char **argv) {
  int tnum = -1;
  int port = -1;

  while (true) {
    int current_optind = optind ? optind : 1;

    static struct option options[] = {{"port", required_argument, 0, 0},
                                      {"tnum", required_argument, 0, 0},
                                      {0, 0, 0, 0}};

    int option_index = 0;
    int c = getopt_long(argc, argv, "", options, &option_index);

    if (c == -1)
      break;

    switch (c) {
    case 0: {
      switch (option_index) {
      case 0:
        port = atoi(optarg);
        if (!port) {
          printf("Error: bad port value\n");
          return -1;
        }
        break;
      case 1:
        tnum = atoi(optarg);
        if (!tnum) {
          printf("Error: bad tnum value\n");
          return -1;
        }
        break;
      default:
        printf("Index %d is out of options\n", option_index);
      }
    } break;

    case '?':
      printf("Unknown argument\n");
      break;
    default:
      fprintf(stderr, "getopt returned character code 0%o?\n", c);
    }
  }

  if (port == -1 || tnum == -1) {
    fprintf(stderr, "Using: %s --port 20001 --tnum 4\n", argv[0]);
    return 1;
  }

  int server_fd = socket(AF_INET, SOCK_STREAM, 0);  
  /* AF_INET - IPv4, SOCK_STREAM - bidirectional, 0 - use 
   *  AF_INET option to determine protocol
   * byte streams, supports connections
   *  */
  if (server_fd < 0) {
    fprintf(stderr, "Can not create server socket!");
    return 1;
  }

  struct sockaddr_in server = create_sockaddr(port, INADDR_ANY);

  int opt_val = 1;
  /* Set socket flags:
   * server_fd - socket descriptor
   * SOL_SOCKET - socket level for flags
   * SO_REUSEADDR - address supplied to bind() should 
   *  allow reuse of local addresses, if suppored
   * &opt_val - set logical flag
   * sizeof flag
   * */
  setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof(opt_val));

  /* Binds socket to local address (gives a name) */
  int err = bind(server_fd, (struct sockaddr *)&server, sizeof(server));
  if (err < 0) {
    if (errno == 13)
      fprintf(stderr, "Try sudo %s\n", argv[0]);
    fprintf(stderr, "Can not bind to socket!, errno=%d\n", errno);
    return 1;
  }

  /* Marks the socket server_fd as a passive that
   *  will be used to accept incoming connections
   * works only with SOCK_STREAM and SOCK_SEQPACKET
   * 
   * 128 - maximum connection queue length
   * */
  err = listen(server_fd, 128);
  if (err < 0) {
    fprintf(stderr, "Could not listen on socket\n");
    return 1;
  }

  printf("Server listening at %d\n", port);

  while (true) {
    struct sockaddr_in client;
    socklen_t client_len = sizeof(client);
    /* accept() is used only with listen() call
     * accept() extracts the first connection request from queue and 
     *  creates a new socket and returns its descriptor
     * original socket server_fd is unaffected by this call
     *
     * after accept() filled client struct, it will fill the exact client_len
     * we are using a blocking socket, so accept() will wait for connections
     * */
    int client_fd = accept(server_fd, (struct sockaddr *)&client, &client_len);

    if (client_fd < 0) {
      fprintf(stderr, "Could not establish new connection\n");
      continue;
    }

    /* After we got a fd for a client, we can read from it 
     * For every client message calculate factorial 
     *  then send it back
     * */
    while (true) {
      size_t buffer_size = sizeof(fac_args_t);
      char from_client[buffer_size];

      /* Recieves a message from a socket
       * if flags are not set, read() call is the same
       * if no messages are available in the socket, wait for a one (nonblocking)
       * */
      int read = recv(client_fd, from_client, buffer_size, 0);

      /* No data recieved */
      if (!read)
        break;
      if (read < 0) {
        fprintf(stderr, "Client %d read failed, errno=%d\n", client_fd, errno);
        break;
      }
      if (read < buffer_size) {
        fprintf(stderr, "Client send wrong data format\n");
        break;
      }

      pthread_t threads[tnum];
      uint64_t begin = 0;
      uint64_t end = 0;
      uint64_t mod = 0;
      memcpy(&begin, from_client, sizeof(uint64_t));
      memcpy(&end, from_client + sizeof(uint64_t), sizeof(uint64_t));
      memcpy(&mod, from_client + 2 * sizeof(uint64_t), sizeof(uint64_t));

      fprintf(stdout, "Receive: %llu %llu %llu\n", begin, end, mod);

      if (tnum > (end - begin) / 2) {
        tnum = (end - begin) / 2;
        printf("Warning: too much threads. Continue with %d\n", tnum);
      }
      fac_args_t args[tnum];

      /* Start threads (why?) */
      float block = (float)(end - begin) / tnum;
      for (uint32_t i = 0; i < tnum; i++) {
        args[i].begin = begin + round(block * (float)i);
        args[i].end = begin + round(block * (i + 1.f));
        args[i].mod = mod;

        if (pthread_create(&threads[i], NULL, ThreadFactorial,
                           (void *)&args[i])) {
          printf("Error: pthread_create failed!\n");
          return 1;
        }
      }

      /* Join threads */
      uint64_t total = 1;
      for (uint32_t i = 0; i < tnum; i++) {
        uint64_t result = 0;
        pthread_join(threads[i], (void **)&result);
        /* We are using thread return value (why no shared data?) */
        total = MultModulo(total, result, mod);
      }

      printf("Total: %llu\n", total);

      /* Send back result */
      char buffer[sizeof(total)];
      memcpy(buffer, &total, sizeof(total));

      /* Send a message to socket
       * send() can be used only if socket is Connected
       * accept() creates a Connected socket */
      err = send(client_fd, buffer, sizeof(total), 0);
      if (err < 0) {
        fprintf(stderr, "Can't send data to client\n");
        break;
      }
    }

    /* On both sides, receptions and transmissions are disallowed */
    shutdown(client_fd, SHUT_RDWR);
    close(client_fd);
  }

  return 0;
}
