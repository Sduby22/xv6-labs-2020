#include <kernel/types.h>
#include <user/user.h>
#include <kernel/param.h>

#define MAXLEN 2048

int main(int argc, char* argv[]) {
  char buf[MAXLEN];
  int nargc = argc-1;
  char *nargv[MAXARG] = {};
  
  for (int i = 1; i != argc; i++) {
    nargv[i-1] = argv[i];
  }

  char *p = buf;
  int met_space = 1;
  for(char ch; read(0, &ch, 1) == 1;) {
    switch (ch) {
    case '\n':
      *p++ = 0;
      if (fork() == 0) {
        exec(nargv[0], nargv);
      } 
      wait(0);
      met_space = 1;
      nargc = argc-1;
      break;
    case ' ':
      *p++ = 0;
      met_space = 1;
      break;
    default:
      if (met_space) {
        met_space = 0;
        nargv[nargc++] = p;
      }
      *p++ = ch;
    }
  }
  exit(0);
}
