#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#define N 35
#define W 1
#define R 0

int
main()
{
  int p[2];
  pipe(p);

  if(fork()!=0){
    close(p[R]);
    close(W);
    dup(p[W]);
    close(p[W]);
    int i;
    for(i=2;i!=35;i++)
      write(W, &i, 4);
    close(W);
    wait(0);
    exit(0);
  } else {
    close(0);
    dup(p[R]);
    close(p[W]);
    close(p[R]);
    while(1) {
      int p2[2];
      int i,j;
      if(read(R, &i, 4)!=0)
        printf("prime %d\n",i);
      else {
        close(1);
        exit(0);
      }
      pipe(p2);
      if(fork()==0) {
        close(0);
        dup(p2[0]);
        close(p2[0]);
        close(p2[1]);
      } else {
        close(W);
        dup(p2[W]);
        close(p2[W]);
        close(p2[R]);
        while(read(R, &j, 4)!=0) {
          if(j%i!=0)
            write(W, &j, 4);
        }
        close(W);
        wait(0);
        exit(0);
      }
    }
  }

}

