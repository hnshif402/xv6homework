#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define DEBUG 0

void write_to_right(int fds_p2c[])
{
  close(fds_p2c[1]);

  int p; 
  if(read(fds_p2c[0], &p, sizeof(p)) == 0) {
    close(fds_p2c[0]);
    exit(0);
  }

  fprintf(1, "prime %d\n", p);

  int n;
  int fds_c2gc[2], pid;
  if(pipe(fds_c2gc) < 0) {
    printf("pipe() error.\n");
    exit(1);
  }
  switch(pid=fork()) {
  case -1:
    printf("fork() error.\n");
    exit(1);
  case 0:
    write_to_right(fds_c2gc);
  default:
    close(fds_c2gc[0]);
    while(read(fds_p2c[0], &n, sizeof(n)) > 0) {
      if(DEBUG) { printf("parent %d, p=%d\n", getpid(), &p);}
      if(n % p != 0) {
        write(fds_c2gc[1], &n, sizeof(n));
      }
    }
    close(fds_c2gc[1]);
    wait(0);
  }
  exit(0);
}

int main()
{
  int n, fds_p2c[2], pid;
  if(pipe(fds_p2c) < 0) {
    printf("pipe() faild.\n");
    exit(1);
  }
  switch(pid=fork()) {
  case -1:
    printf("fork() failed.\n");
  case 0:
    write_to_right(fds_p2c);
  default:
    close(fds_p2c[0]);
    for (n=2; n<=35; n++) {
      if (DEBUG) printf("DEBUG: main: %d\n", n);
      write(fds_p2c[1], &n, sizeof(n));
    }
    close(fds_p2c[1]);
    wait(0);
  }
  exit(0);
}
