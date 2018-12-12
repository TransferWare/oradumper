#if HAVE_CONFIG_H
#include <config.h>
#endif

#ifndef S_SPLINT_S

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

#endif /* #ifndef S_SPLINT_S */

#include <dbug.h>

#include "oradumper.h"

int main( int argc, char **argv )
{
  const int disconnect = 1;
  char error_msg[1000+1];
  unsigned int row_count;
  int status;

#if defined(WITH_DMALLOC) && !defined(S_SPLINT_S)
  (void) atexit(dmalloc_shutdown);
#endif

  /* setup support for UTF-8 */
  (void) setlocale(LC_ALL, "");

  DBUG_INIT((getenv("DBUG_OPTIONS") != NULL ? getenv("DBUG_OPTIONS") : ""), "oradumper");
  DBUG_ENTER("main");
  
  if (NULL != oradumper((unsigned int)(argc - 1),
                        (const char **)(argv + 1),
                        disconnect,
                        sizeof(error_msg),
                        error_msg,
                        &row_count))
    {
      (void) fprintf(stderr, "\nERROR: %s\n", error_msg);
      status = EXIT_FAILURE;
    }
  else
    {
      status = EXIT_SUCCESS;
    }

  DBUG_LEAVE();
  
  return status;
}
