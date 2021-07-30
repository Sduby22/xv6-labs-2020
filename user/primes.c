#include <kernel/types.h>
#include <user/user.h>

#define W 1
#define R 0

// 0 indicates fail.
// 1 ----------success.
int writeint(int pipe, int *x) {
  return write(pipe, x, sizeof(int)) == 4;
}

int readint(int pipe, int *x) {
  return read(pipe, x, sizeof(int)) == 4;
}

int main() {
  int p[2];
  int p2[2];
  pipe(p);

  for(int i = 2; i != 36; i++)
    writeint(p[W], &i);
  close(p[W]);

  int *left = p;
  int *right = p2;
  int first;
  while (readint(left[R], &first) != 0) {
    printf("prime %d\n", first);
    pipe(right);

    if (fork() == 0) {
      close(right[W]);
      int* tmp = left;
      left = right;
      right = tmp;
      continue;
    }

    close(right[R]);

    for(int next; readint(left[R], &next) != 0;)
      if (next % first != 0)
        writeint(right[W], &next);

    close(left[R]);
    close(right[W]);
    wait(0);
  }

  exit(0);
}
