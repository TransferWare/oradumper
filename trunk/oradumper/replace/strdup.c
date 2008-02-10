#if HAVE_CONFIG_H
# include <config.h>
#else
# define HAVE_STRING_H 1
# define HAVE_STDLIB_H 1
#endif

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif

#if HAVE_STRING_H
#include <string.h>
#endif

#ifdef malloc
#undef malloc
#endif

#ifdef WITH_DMALLOC
# include "dmalloc.h"
#endif

char *
strdup(const char *s)
{
  size_t len = strlen(s) + 1;
  void *new = malloc(len);
  if (new == NULL)
    return NULL;
  return (char *) memcpy(new, s, len);
}
