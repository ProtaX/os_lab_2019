#include "netfac.h"

struct sockaddr_in create_sockaddr(uint16_t port, uint32_t s_addr) {
  struct sockaddr_in sockaddr = {
    .sin_family = AF_INET, /* Always */
    .sin_port = htons(port), /* Host to network short */
    .sin_addr.s_addr = htonl(s_addr), /* Host to network long */
    .sin_zero = {0}
  };
  return sockaddr;
}

uint64_t MultModulo(uint64_t a, uint64_t b, uint64_t mod) {
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
