#include <stdio.h>
#include <stdlib.h>

#include "error.h"

void terminate(const char *message)
{
  fprintf(stderr, "%s\n", message);
  exit(EXIT_FAILURE);
}
