#ifndef NETFAC_H
#define NETFAC_H

#include <stdint.h>
#include <netinet/in.h>

struct fac_server {
  char ip[255];
  int port;
};

struct fac_server_list {
  struct fac_server server;
  struct fac_server_list* next;
};

struct FactorialArgs {
  uint64_t begin;
  uint64_t end;
  uint64_t mod;
};

typedef struct fac_server fac_server_t;
typedef struct fac_server_list fac_server_list_t;
typedef struct FactorialArgs fac_args_t;

struct sockaddr_in create_sockaddr(uint16_t port, uint32_t s_addr);
uint64_t MultModulo(uint64_t a, uint64_t b, uint64_t mod);

#endif
