#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#define SEQUENTIAL_BIN_NAME "sequential_min_max.out"

int main(int argc, char* argv[]) {
	if (access(SEQUENTIAL_BIN_NAME, F_OK) == -1) {
		printf("Error: " SEQUENTIAL_BIN_NAME " not found\n");
		return -1;
	}

	execvp("./" SEQUENTIAL_BIN_NAME, argv);
	printf("Error: execvp() failed, errno=%d\n", errno);
}
