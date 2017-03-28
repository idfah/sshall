#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "ioredir.h"

int main(int narg, char *arg[])
{
  fprintf(stdout, "1. to stdout\n");
  fprintf(stderr, "2. to stderr\n");

  ioredir_desc iod = ioredir_set_err(STDOUT_FILENO);

  printf("%d %d\n", iod.cur, iod.orig);
  printf("%d %d\n", STDOUT_FILENO, STDERR_FILENO);

  fprintf(stdout, "3. to stdout\n");
  fprintf(stderr, "4. to stderr\n");

  ioredir_restore(iod);

  printf("%d %d\n", STDOUT_FILENO, STDERR_FILENO);

  fprintf(stdout, "5. to stdout\n");
  fprintf(stderr, "6. to stderr\n");

  return 0;
}
