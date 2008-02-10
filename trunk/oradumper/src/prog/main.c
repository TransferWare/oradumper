#if HAVE_CONFIG_H
#include <config.h>
#endif

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif

/* include dmalloc as last one */
#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif

#include "oradumper.h"

int main( int argc, char **argv )
{
  const int disconnect = 1;

#ifdef WITH_DMALLOC
  atexit(dmalloc_shutdown);
#endif

  int status = oradumper((unsigned int)(argc - 1), (const char **)(argv + 1), disconnect);

  return status;
}
