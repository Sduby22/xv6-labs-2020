#include <kernel/types.h>
#include <user/user.h>

#define W 1
#define R 0

void die(const char* str) {
  printf(str);
  exit(1);
}

int main(int argc, char *argv[]) {
  int p[2];
  int p2[2];
  int pid;
  pipe(p);
  pipe(p2);
  if ((pid = fork()) == 0) {
    // Child process: recv a byte and send it back.
    pid = getpid();
    char buf[1];
    close(p[W]);
    close(p2[R]);
    if(read(p[R], buf, 1) != 1) {
      die("Child failed to recv.");
    } 
    printf("%d: received ping\n", pid);
    if(write(p2[W], "A", 1) != 1) {
      die("Child failed to send.");
    } 
    close(p[R]);
    close(p2[W]);
  } else {
    // Parent: send a byte to child, and recv one after.
    pid = getpid();
    close(p[R]);
    close(p2[W]);
    char buf[1];
    if(write(p[W], "a", 1) != 1) {
      die("Parent failed to send.");
    }
    if(read(p2[R], buf, 1) != 1) {
      die("Parent failed to recv.");
    }
    printf("%d: received pong\n", pid);
    close(p[W]);
    close(p2[R]);
    wait(0);
  }
  exit(0);
}
