#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#define __NR_newsyscall 17

int
main (int argc, char *argv[])
{
  int n;
  int parameter = 0;

  if (argc > 1)
    parameter = atoi (argv[1]);

  n = syscall (__NR_newsyscall, parameter);

  printf ("Return %d\n", n);
}
