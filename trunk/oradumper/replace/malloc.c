#if HAVE_CONFIG_H
# include <config.h>
#endif

#ifdef malloc
#undef malloc
#endif

#ifdef WITH_DMALLOC
# include "dmalloc.h"
#else
# include <stdlib.h>
#endif

/* Allocate an N-byte block of memory from the heap.
   If N is zero, allocate a 1-byte block.  */

void *
rpl_malloc (size_t n)
{
  if (n == 0)
    n = 1;
  return malloc (n);
}
