#include "kernel/types.h"
#include "kernel/memlayout.h"
#include "user/user.h"

int main() {
  uint64 sz = (PHYSTOP-KERNBASE) / 3 * 2;
  sbrk(sz);
  exit(0);
}
