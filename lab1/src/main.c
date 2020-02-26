/* Compiling:
 * gcc main.c -lm -o rng
 * 
 * Usage:
 * rng
 * rng [NUMBER]
 * rng [NUMBER] [FILENAME]
 */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <stdint.h>

enum {
  STDOUT_MODE,
  FILE_MODE
};

int main(int argc, char* argv[]) {
  uint32_t mode;
  uint32_t rand_numbers_count = 150;
  switch (argc) {
    case 1: // rng
      mode = STDOUT_MODE;
      break;
    case 2: // rng [NUMBER]
      mode = STDOUT_MODE;
      rand_numbers_count = atoi(argv[1]);
      break;
    case 3: // rng [NUMBER] [FILENAME]
      mode = FILE_MODE;
      rand_numbers_count = atoi(argv[1]);
      if (!rand_numbers_count) {
        printf("Error while parsing or argument is zero\n");
        return -1;
      }
      break;
    default:
      printf("Error while parsing arguments: too many arguments\n");
      return -1;
  }

  //Array of raw random bytes from /dev/urandom
  uint8_t rand_bytes[rand_numbers_count * sizeof(uint32_t)];
  FILE *dev_random, *out_file;
  uint32_t i;
  if ((dev_random = fopen("/dev/urandom", "r+b")) == NULL) {
    printf("Error: cannot open /dev/urandom\n");
    return -1;
  }
  fread(rand_bytes, sizeof(uint32_t), rand_numbers_count, dev_random);
  fclose(dev_random);
  if (mode == FILE_MODE)
    if ((out_file = fopen(argv[2], "w")) == NULL) {
      printf("Error: cannot create file %s\n", argv[1]);
      return -1;
    }

  for (i = 0; i < sizeof(rand_bytes); ) {
    int32_t number = (int32_t)(rand_bytes[i++] << 24) |
                              (rand_bytes[i++] << 16) |
                              (rand_bytes[i++] << 8)  |
                              (rand_bytes[i++] << 0);
    number = number % 10;
    if (mode == STDOUT_MODE)
      printf("%d ", number);
    else
      fprintf(out_file, "%d ", number);
  }

  if (mode == FILE_MODE)
    fclose(out_file);

  return 0;
}
