#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>

#if HAVE_ASSERT_H
#include <assert.h>
#endif

#if HAVE_CTYPE_H
#include <ctype.h>
#endif

#if HAVE_MALLOC_H
#include <malloc.h>
#endif

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif

#if HAVE_STRING_H
#include <string.h>
#endif

#if HAVE_DBUG_H
#include <dbug.h>
#endif

/* include dmalloc as last one */
#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif

#include "oradumper_int.h"

static struct {
  /*@observer@*/ char *name; /* name */
  char *desc; /* description */
  /*@null@*/ char *def; /* default */
  /*@observer@*//*@null@*/ char *value; /* value */
} opt[] = { 
  { "userid", "Oracle connect string", "scott/tiger", NULL },
  { "sqlstmt", "Select statement", NULL, NULL },
  { "arraysize", "Array size", "10", NULL }
};

static
void
usage(void)
{
  size_t i;

  (void) fprintf( stderr, "\nUsage: oradumper [OPTION]...\n\nOption:\n");
  for (i = 0; i < sizeof(opt)/sizeof(opt[0]); i++)
    {
      (void) fprintf( stderr, "%-10s\t%s", opt[i].name, opt[i].desc);
      if (opt[i].def != NULL)
	{
	  (void) fprintf( stderr, " (%s)", opt[i].def);
	}
      (void) fprintf( stderr, "\n");
    }

  exit(EXIT_FAILURE);
}

void
process_options(const unsigned int length, const char **options)
{
  size_t i, j;

  for (i = 0; i < (size_t) length; i++)
    {
      for (j = 0; j < sizeof(opt)/sizeof(opt[0]); j++)
	{
	  if (strncmp(options[i], opt[j].name, strlen(opt[j].name)) == 0 &&
	      options[i][strlen(opt[j].name)] == '=')
	    {
	      opt[j].value = (char*)(options[i] + strlen(opt[j].name) + 1);
	      break;
	    }
	}

      if (j == sizeof(opt)/sizeof(opt[0]))
	{
	  usage();
	}
    }

  /* assign defaults */
  for (j = 0; j < sizeof(opt)/sizeof(opt[0]); j++)
    {
      if (opt[j].value == NULL && opt[j].def != NULL)
	{
	  opt[j].value = opt[j].def;
	}
    }

  if (get_option(OPTION_USERID) == NULL || get_option(OPTION_SQLSTMT) == NULL )
    {
      usage();
    }
}

const char *
get_option(const option_t option)
{
  switch(option)
    {
    case OPTION_USERID:
    case OPTION_SQLSTMT:
    case OPTION_ARRAYSIZE:
      return opt[option].value;
    }

#ifndef lint
  return NULL;
#endif
}
