#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  if (argc==1){
    char *err = "argument missing.\n";
    write(1, err, strlen(err));
  } else {
    int num=0;
    num=atoi(argv[1]);
    sleep(num);
  }
  exit(0);
}
