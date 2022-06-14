#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

/* must create two pipes, one pipe for one direction;
 * direction1:
 *     parent  write -> pfd1[1], 
 *     child read from pfd1[0]
 *     ---------------------------------                 ------------------------------------
 *    P|pfd1[0]|pfd1[1]|pfd2[0]|pfd2[1]                 C|pfd1[0]|pfd1[1]|pfd2[0]|pfd2[1]
 *      close    |        ||    close                       |      close   close    ||
 *              write    read                              read                    write  
 *               |        ||                                |                       ||
 *               |        ||                                |                       ||
 *               |        ||                   kernel       |                       ||
 *               |        ==================|pipe:pfd2|=====|=========================
 *               |                kernel                    |
 *               |______________|pipe:pfd1|_________________|          
 * direction2:
 *     child write -> pfd2[1], 
 *     parent read from pfd2[0]
 */


char p = 'p'; /* child process sends byte 'p' to parent process */ 
char c = 'c'; /* parent process sends byte 'c' to child process */

int main()
{
  int pfd1[2], pfd2[2], pid;
  char buf[1];
  
  if (pipe(pfd1) != 0 || pipe(pfd2) != 0) {
    printf("pipe() failed.\n");
    exit(1);
  }

  if ((pid = fork()) < 0) {
    printf("fork() failed.\n");
    exit(1);
  }
  if (pid == 0) {  /* child process */
    close(pfd1[1]);
    close(pfd2[0]);
    if ((read(pfd1[0], buf, 1)) != 1) {
      printf("pipe error.\n");
      exit(1);
      }
    printf("%d: received ping\n", getpid());
    close(pfd1[0]);
    write(pfd2[1], &p, 1);
    close(pfd2[1]);

    exit(0);
  }
  /* parent process */
  close(pfd1[0]);
  close(pfd2[1]);
  write(pfd1[1], &c, 1);
  close(pfd1[1]);

  if ((read(pfd2[0], buf, 1)) != 1) {
    printf("pipe error.\n");
    exit(1);
  }
  printf("%d: received pong\n", getpid());
  close(pfd2[0]);

  wait(&pid);

  exit(0);
}
