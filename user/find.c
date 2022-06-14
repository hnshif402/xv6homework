#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

#define DEBUG 0

char *fmtname(char *path)
{
  //if(DEBUG) fprintf(2, "fmtname: path = %s\n", path);
  static char base[DIRSIZ+1];
  char *p;

  for(p=path+strlen(path); p >= path && *p != '/'; p--)
    ;
  p++;
  //if(DEBUG) fprintf(2, "fmtname: filename %s\n", p);

  if(strlen(p) >= DIRSIZ)
    return p;
  memmove(base, p, strlen(p));
  memset(base+strlen(p), '\0', DIRSIZ-strlen(p));
  return base;
}

void find(char *path, char *file)
{
  char buf[512], *p, *s;
  int fd;
  struct dirent de;
  struct stat st;

  if((fd = open(path, 0)) < 0) {
    fprintf(2, "find: cannot open %s\n", path);
    return;
  }

  if(fstat(fd, &st) < 0) {
    fprintf(2, "find: cannot stat %s\n", path);
    return;
  }
  //if(DEBUG) fprintf(2, "find: %s %d\n", path, st.type);
  switch(st.type) {
  case T_FILE:
    s = fmtname(path);
    if(strcmp(s, file) == 0) {
      fprintf(1, "%s\n", path);
    }
    break;
  case T_DIR:
    if(strlen(path) + 1 + DIRSIZ + 1 > sizeof(buf)) {
        fprintf(2, "find: path too long\n");
        break;
      }
      strcpy(buf, path);
      p = buf+strlen(buf);
      *p++ = '/';
      while(read(fd, &de, sizeof(de)) == sizeof(de) && strcmp(de.name, "") != 0) {
        if(DEBUG) fprintf(2, "find: %s de.name %s\n", buf, de.name);
        if(strcmp(de.name,".") == 0 || strcmp(de.name, "..") == 0)
          continue;
        memmove(p, de.name, DIRSIZ);
        p[DIRSIZ] = 0;
        //if(DEBUG)fprintf(2, "find: find %s %s\n", buf, file);
        find(buf, file);
        memset(p, '\0', DIRSIZ);
        memset(&de, '\0', sizeof(de));
      }
      break;
  }
  close(fd);
}

int main(int argc, char *argv[])
{
  if(argc != 3) {
    fprintf(2, "usages: find dir file\n");
    exit(1);
  }
  find(argv[1], argv[2]);
  exit(0);
}
