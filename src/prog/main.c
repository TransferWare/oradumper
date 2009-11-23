#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h> /* should always be there */

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif

#if HAVE_LOCALE_H
#include <locale.h>
#else
#define setlocale(/*@sef@*/x, /*@sef@*/y)
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
  unsigned int row_count;

#ifdef WITH_DMALLOC
  (void) atexit(dmalloc_shutdown);
#endif

  /* setup support for UTF-8 */
  (void) setlocale(LC_ALL, "");

  if (NULL != oradumper((unsigned int)(argc - 1),
                        (const char **)(argv + 1),
                        disconnect,
                        sizeof(error_msg),
                        error_msg,
                        &row_count))
    {
      (void) fprintf(stderr, "\nERROR: %s\n", error_msg);
      return EXIT_FAILURE;
    }
  else
    {
      return EXIT_SUCCESS;
    }
}
