#ifndef NETFAC_H
#define NETFAC_H

#include <stdint.h>
#include <netinet/in.h>
#include <pthread.h>

struct fac_args {
  uint64_t begin;
  uint64_t end;
  uint64_t mod;
};

struct fac_server {
  /* External */
  char ip[255];
  int port;

  /* Internal */
  pthread_t thread;
  int socket;
  struct fac_args args;
};

struct fac_server_list {
  struct fac_server server;
  struct fac_server_list* next;
};

typedef struct fac_server fac_server_t;
typedef struct fac_server_list fac_server_list_t;
typedef struct fac_args fac_args_t;

struct sockaddr_in create_sockaddr(uint16_t port, uint32_t s_addr);
uint64_t MultModulo(uint64_t a, uint64_t b, uint64_t mod);

#endif
