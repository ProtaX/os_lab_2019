#include "revert_string.h"
#include <string.h>

void char_swap(char* l, char* r) {
  char buf = *l;
  *l = *r;
  *r = buf;
}

void RevertString(char *str)
{
  int len = strlen(str);
  int mid = len / 2;
  int i;
  for (i = 0; i < mid; i++)
    char_swap(&str[i], &str[len - i - 1]);
}

