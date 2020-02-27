#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/wait.h>

void print_zombie(pid_t zombie_pid) {
  pid_t ps_pid = fork();
  if (ps_pid >= 0) {
    if (ps_pid > 0) {
      waitpid(ps_pid, 0, 0);
    }
    if (ps_pid == 0) {
      char str_pid[12];
      sprintf(str_pid, "%d", zombie_pid);
      execlp("ps", "ps", "-p", str_pid, NULL);
      printf("Error: cannot execute ps\n");
    }
  }
  else {
    printf("Error: cannot print info about zombie\n");
    return;
  }
}

int main(int argc, char* argv[]) {
  pid_t child = fork();

  if (child >= 0) {
    if (child > 0) {
      printf("Zombie pid=%d\n", child);
      // ps -p child
      print_zombie(child);
    }
    if (child == 0)
      return 0;
  }
  else {
    printf("fork() failed\n");
    return -1;
  } 
}
