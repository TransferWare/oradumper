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
  { "userid", 0, "Oracle connect string", NULL, NULL }, /* userid may be NULL when oradumper is used as a library */
  { "sqlstmt", 1, "Select statement", NULL, NULL },
  { "arraysize", 1, "Array size", "10", NULL },
  { "dbug_options", 1, "DBUG options", "", NULL },
  { "nls_date_format", 1, "NLS date format", "yyyy-mm-dd hh24:mi:ss", NULL },
  { "nls_timestamp_format", 1, "NLS timestamp format", "yyyy-mm-dd hh24:mi:ss", NULL },
  { "nls_numeric_characters", 1, "NLS numeric characters", ".,", NULL },
  { "b1", 0, "Bind variable 1", NULL, NULL },
  { "b2", 0, "Bind variable 2", NULL, NULL },
  { "b3", 0, "Bind variable 3", NULL, NULL },
  { "b4", 0, "Bind variable 4", NULL, NULL },
  { "b5", 0, "Bind variable 5", NULL, NULL },
  { "b6", 0, "Bind variable 6", NULL, NULL },
  { "b7", 0, "Bind variable 7", NULL, NULL },
  { "b8", 0, "Bind variable 8", NULL, NULL },
  { "b9", 0, "Bind variable 9", NULL, NULL },
  { "b10", 0, "Bind variable 10", NULL, NULL },
};

static
void
usage(void)
{
  size_t i;

  (void) fprintf( stderr, "\nUsage: oradumper [OPTION=VALUE]...\n\nOPTION:\n");
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
	  (void) fprintf(stderr, "\nERROR: OPTION in OPTION=VALUE (%s) unknown.\n", options[i]);
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
    case OPTION_B1:
    case OPTION_B2:
    case OPTION_B3:
    case OPTION_B4:
    case OPTION_B5:
    case OPTION_B6:
    case OPTION_B7:
    case OPTION_B8:
    case OPTION_B9:
    case OPTION_B10:
      return opt[option].value;
    }

#ifndef lint
  return NULL;
#endif
}

int
oradumper(const unsigned int length, const char **options)
{
  typedef enum {
    STEP_CONNECT = 0,
    STEP_NLS_DATE_FORMAT,
    STEP_NLS_TIMESTAMP_FORMAT,
    STEP_NLS_NUMERIC_CHARACTERS,
    STEP_ALLOCATE_DESCRIPTORS,
    STEP_PARSE,
    STEP_BIND_VARIABLE,
    STEP_OPEN_CURSOR,
    STEP_COLUMN,
    STEP_CLOSE_CURSOR,
    STEP_DEALLOCATE_DESCRIPTORS

#define STEP_MAX ((int) STEP_DEALLOCATE_DESCRIPTORS)
  } step_t;
  int step;

  int status = OK;
#define NLS_MAX_SIZE 100
  char nls_date_format_stmt[NLS_MAX_SIZE+1];
  char nls_timestamp_format_stmt[NLS_MAX_SIZE+1];
  char nls_numeric_characters_stmt[NLS_MAX_SIZE+1];
  const char *userid = get_option(OPTION_USERID);
  const char *nls_date_format = get_option(OPTION_NLS_DATE_FORMAT);
  const char *nls_timestamp_format = get_option(OPTION_NLS_TIMESTAMP_FORMAT);
  const char *nls_numeric_characters = get_option(OPTION_NLS_NUMERIC_CHARACTERS);
  const char *array_size_str = get_option(OPTION_ARRAYSIZE);
  unsigned int array_size = 0;
  const char *sqlstmt = get_option(OPTION_SQLSTMT);
  unsigned int bind_variable_count = 0, bind_variable_nr;
  char bind_variable_name[30+1] = "";
  char *bind_variable_value;
  unsigned int column_count = 0, column_nr;
  char column_name[30+1] = "";
  int column_type;
  unsigned int column_length;
  char ***data = NULL;
  unsigned short **ind = NULL;
	  
  process_options(length, options);

  DBUG_INIT(get_option(OPTION_DBUG_OPTIONS), "oradumper");

  for (step = (step_t) 0; step <= STEP_MAX && status == OK; step++)
    {
      switch((step_t) step)
	{
	case STEP_CONNECT:
	  if (userid != NULL)
	    {
	      status = sql_connect(userid);
	    }
	  break;

	case STEP_NLS_DATE_FORMAT:
	  assert(nls_date_format != NULL);

	  (void) snprintf(nls_date_format_stmt,
			  sizeof(nls_date_format_stmt),
			  "ALTER SESSION SET NLS_DATE_FORMAT = '%s'",
			  nls_date_format);

	  status = sql_execute_immediate(nls_date_format_stmt);
	  break;

	case STEP_NLS_TIMESTAMP_FORMAT:
	  assert(nls_timestamp_format != NULL);

	  (void) snprintf(nls_timestamp_format_stmt,
			  sizeof(nls_timestamp_format_stmt),
			  "ALTER SESSION SET NLS_TIMESTAMP_FORMAT = '%s'",
			  nls_timestamp_format);

	  status = sql_execute_immediate(nls_timestamp_format_stmt);
	  break;

	case STEP_NLS_NUMERIC_CHARACTERS:
	  assert(nls_numeric_characters != NULL);
	
	  (void) snprintf(nls_numeric_characters_stmt,
			  sizeof(nls_numeric_characters_stmt),
			  "ALTER SESSION SET NLS_NUMERIC_CHARACTERS = '%s'",
			  nls_numeric_characters);
      
	  status = sql_execute_immediate(nls_numeric_characters);
	  break;

	case STEP_ALLOCATE_DESCRIPTORS:
	  assert(array_size_str != NULL);

	  array_size = (unsigned int) atoi(array_size_str);

	  status = sql_allocate_descriptors(array_size);
	  break;

	case STEP_PARSE:
	  assert(sqlstmt != NULL);

	  status = sql_parse(sqlstmt);
	  break;

	case STEP_BIND_VARIABLE:
	  if ((status = sql_bind_variable_count(&bind_variable_count)) != OK)
	    break;

	  for (bind_variable_nr = 0;
	       bind_variable_nr < bind_variable_count && bind_variable_nr < MAX_BIND_VARIABLES;
	       bind_variable_nr++)
	    {
	      if ((status = sql_bind_variable_name(bind_variable_nr + 1,
						   sizeof(bind_variable_name),
						   bind_variable_name)) != OK)
		break;

	      bind_variable_value = get_option((option_t)((unsigned int)OPTION_B1 + bind_variable_nr));

	      DBUG_PRINT("info",
			 ("bind variable %u has name %s and value %s",
			  bind_variable_nr + 1,
			  bind_variable_name,
			  bind_variable_value));

	      if ((status = sql_bind_variable(bind_variable_nr + 1, bind_variable_value)) != OK)
		break;
	    }
	  break;

	case STEP_OPEN_CURSOR:
	  status = sql_open_cursor();
	  break;

	case STEP_COLUMN:
	  if ((status = sql_column_count(&column_count)) != OK)
	    break;

	  for (column_nr = 0;
	       column_nr < column_count;
	       column_nr++)
	    {
	      if ((status = sql_describe_column(column_nr + 1,
						sizeof(column_name),
						column_name,
						&column_type,
						&column_length)) != OK)
		break;

	      DBUG_PRINT("info",
			 ("column %u has name %s, type %d and length %u",
			  column_nr + 1,
			  column_name,
			  column_type,
			  &column_length));

	      switch (column_type)
		{
		default:
		  column_type = ORA_STRING;
		}

	      if ((status = sql_define_column(column_nr + 1,
					      column_type,
					      column_length,
					      array_size,
					      data[column_nr],
					      ind[column_nr])) != OK)
		break;
	    }
	  break;

	  /**/
	case STEP_CLOSE_CURSOR:
	  status = sql_close_cursor();
	  break;

	case STEP_DEALLOCATE_DESCRIPTORS:
	  status = sql_deallocate_descriptors();
	  break;
	}
    }

  DBUG_DONE();

  return status;
}
