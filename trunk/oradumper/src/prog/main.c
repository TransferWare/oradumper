#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h> /* should always be there */

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
  char error_msg[1000+1];

#ifdef WITH_DMALLOC
  atexit(dmalloc_shutdown);
#endif

  if (NULL != oradumper((unsigned int)(argc - 1),
			(const char **)(argv + 1),
			disconnect,
			sizeof(error_msg),
			error_msg))
    {
      (void) fputs(error_msg, stderr);
      return EXIT_FAILURE;
    }
  else
    {
      return EXIT_SUCCESS;
    }
}
