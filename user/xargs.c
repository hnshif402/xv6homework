#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"
#include "kernel/param.h"

#define DEBUG 1

void run_cmd(char *cmd, char **param)
{
  int pid;
  switch(pid=fork()) {
  case -1:
    printf("run_cmd: fork failed.\n");
    exit(1);
  case 0:
    exec(cmd, param);
    printf("run_cmd: exec failed\n");
  default:
    wait(&pid);
  }
}

int main(int argc, char *argv[])
{
  int i;
  char buf[255], *p;
  char *arg[MAXARG];

  int ps, n; /* page size */
  ps = MAXARG;
  //memset(arg, '\0',MAXARG);
  i = 1;
  p = argv[i];
  if(*p == '-') {
    p++;
    if(*p == 'n') {
      ps=atoi(argv[++i]);
    }
    i++;
  }

  for(n=0; i<argc && n<MAXARG; i++,n++) {
      arg[n] = argv[i];
    }
    
  int sz, pos, count;
  while((sz = read(0, buf, sizeof(buf)))> 0) {
    count = 1;
    i = n;
    p=buf;
    pos = 0;
    if(DEBUG) printf("parent received: %s\n", buf);
    if(*p == '"') {
      p+=1;
      pos++;
    }
    while(pos < sz) {
      if(buf[pos] == '\\') {
        if(buf[pos+1]=='n') {
          buf[pos] = '\0';
          buf[++pos] = '\0';
          if(DEBUG) printf("parent send: %s\n", p);
          char *s = malloc(strlen(p)+1);
          memcpy(s, p, strlen(p));
          s[strlen(p)] ='\0';
          arg[i] = s;
          i++;
          if(count%ps == 0) {
            run_cmd(arg[0],arg);
            i--;
          }
          count++;
          //write(fds[1], p, strlen(p));
          pos++;
          p = buf + pos;
        }
      }
      if(buf[pos] == '"') {
        buf[pos] = '\0';
        if(DEBUG) printf("parent send: %s\n", p);
        char *s = malloc(strlen(p)+1);
        memcpy(s, p, strlen(p));
        s[strlen(p)] ='\0';
        arg[i] = s;
        i++;
        if(count%ps == 0) {
          run_cmd(arg[0],arg);
          i--;
        }
        count++;
        //write(fds[1], p, strlen(p));
        pos++;
        p = buf + pos;
      }
      pos++;
    }
    if(p < buf + sz) {
      if(DEBUG) printf("parent send: %s\n", p);
      char *s = malloc(strlen(p)+1);
      memcpy(s, p, strlen(p));
      s[strlen(p)] ='\0';
      arg[i] = s;
      run_cmd(arg[0],arg);
      count++;
      //write(fds[1], p, strlen(p));
    }
  }
  exit(0);
}
