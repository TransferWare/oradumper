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
  int mandatory;
  char *desc; /* description */
  /*@null@*/ char *def; /* default */
  /*@observer@*//*@null@*/ char *value; /* value */
} opt[] = { 
  { "userid", 1, "Oracle connect string", "scott/tiger", NULL },
  { "sqlstmt", 1, "Select statement", NULL, NULL },
  { "arraysize", 1, "Array size", "10", NULL },
  { "dbug_options", 1, "DBUG options", "", NULL },
  { "nls_date_format", 1, "NLS date format", "yyyy-mm-dd hh24:mi:ss", NULL },
  { "nls_timestamp_format", 1, "NLS timestamp format", "yyyy-mm-dd hh24:mi:ss", NULL },
  { "nls_numeric_characters", 1, "NLS numeric characters", ".,", NULL }
};

static
void
usage(void)
{
  size_t i;

  (void) fprintf( stderr, "\nUsage: oradumper [OPTION]...\n\nOPTION:\n");
  for (i = 0; i < sizeof(opt)/sizeof(opt[0]); i++)
    {
      (void) fprintf( stderr, "  %-30s  %s", opt[i].name, opt[i].desc);
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
	  (void) fprintf(stderr, "\nERROR: Option %s unknown.\n", options[i]);
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

  /* mandatory options should not be empty */
  for (j = 0; j < sizeof(opt)/sizeof(opt[0]); j++)
    {
      if (opt[j].mandatory != 0 && opt[j].value == NULL)
	{
	  (void) fprintf(stderr, "\nERROR: Option %s mandatory.\n", opt[j].name);
	  usage();
	}
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
    case OPTION_DBUG_OPTIONS:
    case OPTION_NLS_DATE_FORMAT:
    case OPTION_NLS_TIMESTAMP_FORMAT:
    case OPTION_NLS_NUMERIC_CHARACTERS:
      return opt[option].value;
    }

#ifndef lint
  return NULL;
#endif
}

int
oradumper(const unsigned int length, const char **options)
{
  int status = 1;
#define NLS_MAX_SIZE 100
  char nls_date_format_stmt[NLS_MAX_SIZE+1];
  char nls_timestamp_format_stmt[NLS_MAX_SIZE+1];
  char nls_numeric_characters_stmt[NLS_MAX_SIZE+1];
  const char *userid = get_option(OPTION_USERID);
  const char *nls_date_format = get_option(OPTION_NLS_DATE_FORMAT);
  const char *nls_timestamp_format = get_option(OPTION_NLS_TIMESTAMP_FORMAT);
  const char *nls_numeric_characters = get_option(OPTION_NLS_NUMERIC_CHARACTERS);
  const char *array_size = get_option(OPTION_ARRAYSIZE);
  const char *sqlstmt = get_option(OPTION_SQLSTMT);
	  
  process_options(length, options);

  DBUG_INIT(get_option(OPTION_DBUG_OPTIONS), "oradumper");

  do
    {
      assert(userid != NULL);

      /* Connect to the database. */
      if (sql_connect(userid) != OK)
	break;

      assert(nls_date_format != NULL);

      (void) snprintf(nls_date_format_stmt,
		      sizeof(nls_date_format_stmt),
		      "ALTER SESSION SET NLS_DATE_FORMAT = '%s'",
		      nls_date_format);

      if (sql_execute_immediate(nls_date_format_stmt) != OK)
	break;

      assert(nls_timestamp_format != NULL);

      (void) snprintf(nls_timestamp_format_stmt,
		      sizeof(nls_timestamp_format_stmt),
		      "ALTER SESSION SET NLS_TIMESTAMP_FORMAT = '%s'",
		      nls_timestamp_format);

      if (sql_execute_immediate(nls_timestamp_format_stmt) != OK)
	break;

      assert(nls_timestamp_format != NULL);

      assert(nls_numeric_characters != NULL);

      (void) snprintf(nls_numeric_characters_stmt,
		      sizeof(nls_numeric_characters_stmt),
		      "ALTER SESSION SET NLS_NUMERIC_CHARACTERS = '%s'",
		      nls_numeric_characters);
      
      if (sql_execute_immediate(nls_numeric_characters) != OK)
	break;

      assert(array_size != NULL);

      if (sql_allocate_descriptors((unsigned int) atoi(array_size)) != OK)
	break;

      assert(sqlstmt != NULL);

      if (sql_parse(sqlstmt) != OK)
	break;

      status = 0;
    }
  while (1 != 1);

  DBUG_DONE();

  return status;
}
