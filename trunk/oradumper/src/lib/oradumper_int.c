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

#if HAVE_LOCALE_H
#include <locale.h>
#else
#define setlocale(/*@sef@*/x, /*@sef@*/y)
#endif

#if HAVE_MALLOC_H
#include <malloc.h>
#endif

#if HAVE_STDBOOL_H
#include <stdbool.h>
#else
typedef int bool;
#define false 0
#define true 1
#endif

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif

#if HAVE_STRING_H
#include <string.h>
#endif

#if HAVE_SYS_ERRNO_H
#include <sys/errno.h>
#endif

#if HAVE_WCHAR_H
#include <wchar.h>
#endif

#if HAVE_DBUG_H
#include <dbug.h>
#endif

/* include dmalloc as last one */
#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif

#include "oradumper_int.h"

typedef /*observer*/ char *data_ptr_t;

/**
 * The maximum number of dots per line before a new line
 */
#define DOTS_PER_LINE 75

typedef enum {
  OPTION_USERID = 0,
  OPTION_SQLSTMT,
  OPTION_FETCH_SIZE,
  OPTION_DBUG_OPTIONS,
  OPTION_NLS_LANGUAGE,
  OPTION_NLS_DATE_FORMAT,
  OPTION_NLS_TIMESTAMP_FORMAT,
  OPTION_NLS_NUMERIC_CHARACTERS,
  OPTION_DETAILS,
  OPTION_RECORD_DELIMITER,
  OPTION_FEEDBACK,
  OPTION_COLUMN_HEADING,
  OPTION_FIXED_COLUMN_LENGTH,
  OPTION_COLUMN_SEPARATOR,
  OPTION_ENCLOSURE_STRING,
  OPTION_OUTPUT_FILE,
  OPTION_OUTPUT_APPEND,
  OPTION_NULL,
} option_t;

typedef struct {
  /*@null@*/ /*@observer@*/ char *userid;
  /*@observer@*/ char *sqlstmt;
  unsigned int fetch_size;
  /*@observer@*/ char *dbug_options;
  /*@null@*/ /*@observer@*/ char *nls_language;
  /*@null@*/ /*@observer@*/ char *nls_date_format;
  /*@null@*/ /*@observer@*/ char *nls_timestamp_format;
  /*@null@*/ /*@observer@*/ char *nls_numeric_characters;
  bool details;
  /*@observer@*/ char *record_delimiter;
  bool feedback;
  bool column_heading;
  bool fixed_column_length;
  /*@observer@*/ char *column_separator;
  /*@null@*/ /*@observer@*/ char *enclosure_string;
  /*@null@*/ /*@observer@*/ char *output_file;
  bool output_append;
  /*@observer@*/ char *null;
} settings_t;

#define OPTION_TRUE(opt) strcmp((opt == NULL ? "" : opt), "1") == 0;

static struct {
  /*@observer@*/ char *name; /* name */
  int mandatory;
  char *desc; /* description */
  /*@null@*/ char *def; /* default */
  /*@observer@*//*@null@*/ char *value; /* value */
} opt[] = { 
  { "userid", 0, "Oracle connect string", NULL, NULL }, /* userid may be NULL when oradumper is used as a library */
  { "sqlstmt", 1, "Select statement", NULL, NULL },
  { "fetch_size", 1, "Array size", "10", NULL },
  { "dbug_options", 1, "DBUG options", "", NULL },
  { "nls_language", 0, "NLS language", NULL, NULL },
  { "nls_date_format", 0, "NLS date format", NULL, NULL },
  { "nls_timestamp_format", 0, "NLS timestamp format", NULL, NULL },
  { "nls_numeric_characters", 0, "NLS numeric characters", NULL, NULL },
  { "details", 0, "Print details about input and output values: yes, only or no", "no", NULL },
  { "record_delimiter", 1, "Record delimiter", "\n", NULL },
  { "feedback", 1, "Give feedback after every fetch (0 = no feedback)", "0", NULL },
  { "column_heading", 1, "Include column names in first line (1 = yes)", "1", NULL },
  { "fixed_column_length", 1, "Fixed column length: 0 = yes (fixed), 1 = no (variable)", "0", NULL },
  { "column_separator", 1, "The column separator (e.g. a comma or semi-colon)", ",", NULL },
  { "enclosure_string", 0, "Characters around a character column in variable length data", NULL, NULL },
  { "output_file", 0, "The output file", NULL, NULL },
  { "output_append", 0, "Append to the output file (1 = yes)?", NULL, NULL },
  { "null", 1, "Value to print for NULL values", "NULL", NULL },
};

static
/*@null@*//*@observer@*/
char *
get_option(const option_t option)
{
  switch(option)
    {
    case OPTION_USERID:
    case OPTION_SQLSTMT:
    case OPTION_FETCH_SIZE:
    case OPTION_DBUG_OPTIONS:
    case OPTION_NLS_LANGUAGE:
    case OPTION_NLS_DATE_FORMAT:
    case OPTION_NLS_TIMESTAMP_FORMAT:
    case OPTION_NLS_NUMERIC_CHARACTERS:
    case OPTION_DETAILS:
    case OPTION_RECORD_DELIMITER:
    case OPTION_FEEDBACK:
    case OPTION_COLUMN_HEADING:
    case OPTION_FIXED_COLUMN_LENGTH:
    case OPTION_COLUMN_SEPARATOR:
    case OPTION_ENCLOSURE_STRING:
    case OPTION_OUTPUT_FILE:
    case OPTION_OUTPUT_APPEND:
    case OPTION_NULL:
      return opt[option].value;
    }

#ifndef lint
  return NULL;
#endif
}

void
oradumper_usage(FILE *fout)
{
  size_t i;

  (void) fprintf( fout, "\nUsage: oradumper [OPTION=VALUE]... [VALUE]...\n\nOPTION:\n");
  for (i = 0; i < sizeof(opt)/sizeof(opt[0]); i++)
    {
      (void) fprintf( fout, "  %-30s  %s", opt[i].name, opt[i].desc);
      if (opt[i].def != NULL)
	{
	  (void) fprintf( fout, " (%s)", opt[i].def);
	}
      (void) fprintf( fout, "\n");
    }
}

unsigned int
oradumper_process_arguments(const unsigned int nr_arguments, const char **arguments)
{
  size_t i, j;

  for (i = 0; i < (size_t) nr_arguments; i++)
    {
      for (j = 0; j < sizeof(opt)/sizeof(opt[0]); j++)
	{
	  if (strncmp(arguments[i], opt[j].name, strlen(opt[j].name)) == 0 &&
	      arguments[i][strlen(opt[j].name)] == '=')
	    {
	      opt[j].value = (char*)(arguments[i] + strlen(opt[j].name) + 1);
	      break;
	    }
	}

      if (j == sizeof(opt)/sizeof(opt[0])) /* arguments[i] not an option */
	{
	  break;
	}
    }

  for (j = 0; j < sizeof(opt)/sizeof(opt[0]); j++)
    {
      /* assign defaults */
      if (opt[j].value == NULL && opt[j].def != NULL)
	{
	  opt[j].value = opt[j].def;
	}
      /* mandatory options should not be empty */
      if (opt[j].mandatory != 0 && opt[j].value == NULL)
	{
	  (void) fprintf(stderr, "\nERROR: Option %s mandatory.\n", opt[j].name);
	  oradumper_usage(stderr);
	  i = 0; /* number of options should be at least 1: the statement */
	  break;
	}
    }

  return (unsigned int) i; /* number of options found */
}

static
int
prepare_fetch(const unsigned int fetch_size, value_info_t *column_value)
{
  int status;
  unsigned int column_nr;
  /*@observer@*/ data_ptr_t data_ptr = NULL;
  unsigned int array_nr;
  
  do
    {
      if ((status = sql_value_count(column_value->descriptor_name, &column_value->value_count)) != OK)
	break;

#ifdef lint
      free(column_value->descr);
      free(column_value->size);
      free(column_value->buf);
      free(column_value->data);
      free(column_value->ind);
#endif

      column_value->descr =
	(value_description_t *) calloc((size_t) column_value->value_count, sizeof(*column_value->descr));
      assert(column_value->descr != NULL);

      column_value->size =
	(sql_size_t *) calloc((size_t) column_value->value_count, sizeof(*column_value->size));
      assert(column_value->size != NULL);

      column_value->buf =
	(byte_ptr_t *) calloc((size_t) column_value->value_count, sizeof(*column_value->buf));
      assert(column_value->buf != NULL);

      column_value->data = (value_data_ptr_t *) calloc((size_t) column_value->value_count, sizeof(*column_value->data));
      assert(column_value->data != NULL);

      column_value->ind = (short_ptr_t *) calloc((size_t) column_value->value_count, sizeof(*column_value->ind));
      assert(column_value->ind != NULL);

      for (column_nr = 0;
	   column_nr < column_value->value_count;
	   column_nr++)
	{
	  if ((status = sql_value_get(column_value->descriptor_name,
				      column_nr + 1,
				      &column_value->descr[column_nr])) != OK)
	    break;

	  switch (column_value->descr[column_nr].type)
	    {
	    case ANSI_NUMERIC:
	    case ORA_NUMBER:
	    case ANSI_SMALLINT:
	    case ANSI_INTEGER:
	    case ORA_INTEGER:
	    case ORA_UNSIGNED:
	      column_value->descr[column_nr].length =
		(sql_size_t) (column_value->descr[column_nr].precision <= 0
			      ? 38
			      : column_value->descr[column_nr].precision);
	      /* Add one character for the decimal dot (or comma) */
	      if (column_value->descr[column_nr].precision > 0)
		column_value->descr[column_nr].length++;

	      column_value->descr[column_nr].type = ANSI_CHARACTER_VARYING;
	      column_value->descr[column_nr].octet_length = column_value->descr[column_nr].length;
	      /* add 1 byte for a terminating zero */
	      column_value->size[column_nr] = column_value->descr[column_nr].length + 1;
	      break;

	    case ANSI_DECIMAL:
	    case ORA_DECIMAL:
	    case ANSI_FLOAT:
	    case ORA_FLOAT:
	    case ANSI_DOUBLE_PRECISION:
	    case ANSI_REAL:
	      column_value->descr[column_nr].length =
		(sql_size_t) (column_value->descr[column_nr].precision <= 0
			      ? 38
			      : column_value->descr[column_nr].precision);
	      /* Add one character for the decimal dot (or comma) */
	      column_value->descr[column_nr].length++;
	      /* Add the mantisse and so on */
	      column_value->descr[column_nr].length += 5;

	      column_value->descr[column_nr].type = ANSI_CHARACTER_VARYING;
	      column_value->descr[column_nr].octet_length = column_value->descr[column_nr].length;
	      /* add 1 byte for a terminating zero */
	      column_value->size[column_nr] = column_value->descr[column_nr].length + 1;
	      break;

	    case ORA_LONG:
	      column_value->descr[column_nr].length = 2000;
	      column_value->descr[column_nr].octet_length = column_value->descr[column_nr].length;
	      break;

	    case ORA_ROWID:
	      column_value->descr[column_nr].length = 18;
	      column_value->descr[column_nr].octet_length = column_value->descr[column_nr].length;
	      break;

	    case ANSI_DATE:
	    case ORA_DATE:
	      column_value->descr[column_nr].length = 25;
	      column_value->descr[column_nr].type = ANSI_CHARACTER_VARYING;
	      column_value->descr[column_nr].octet_length = column_value->descr[column_nr].length;
	      /* add 1 byte for a terminating zero */
	      column_value->size[column_nr] = column_value->descr[column_nr].length + 1;
	      break;

	    case ORA_RAW:
	      column_value->descr[column_nr].length = ( column_value->descr[column_nr].length == 0 ? 512U : column_value->descr[column_nr].length );
	      column_value->descr[column_nr].octet_length = column_value->descr[column_nr].length;
	      break;

	    case ORA_LONG_RAW:
	      column_value->descr[column_nr].length = 2000;
	      column_value->descr[column_nr].octet_length = column_value->descr[column_nr].length;
	      break;

	    case ANSI_CHARACTER:
	    case ANSI_CHARACTER_VARYING:
	    case ORA_VARCHAR2:
	    case ORA_STRING:
	    case ORA_VARCHAR:
	      column_value->descr[column_nr].type = ANSI_CHARACTER_VARYING;
	      /* add 1 byte for a terminating zero */
	      column_value->size[column_nr] = column_value->descr[column_nr].length + 1;
	      break;

	    case ORA_VARNUM:
	    case ORA_VARRAW:
	    case ORA_DISPLAY:
	    case ORA_LONG_VARCHAR:
	    case ORA_LONG_VARRAW:
	    case ORA_CHAR:
	    case ORA_CHARZ:
	      break;

	    case ORA_UROWID:
	    case ORA_CLOB:
	    case ORA_INTERVAL:
	      column_value->descr[column_nr].type = ANSI_CHARACTER_VARYING;
	      /* add 1 byte for a terminating zero */
	      column_value->size[column_nr] = column_value->descr[column_nr].length + 1;
	      break;

	    case ORA_BLOB:
	      column_value->size[column_nr] = column_value->descr[column_nr].length;
	      break;

#ifndef lint
	    default:
	      column_value->descr[column_nr].type = ANSI_CHARACTER_VARYING;
	      /* add 1 byte for a terminating zero */
	      column_value->size[column_nr] = column_value->descr[column_nr].length + 1;
#endif
	    }

	  /* column_value->data[column_nr][array_nr] points to memory in column_value->buf[column_nr] */
	  column_value->buf[column_nr] = (byte_ptr_t) calloc((size_t) fetch_size, (size_t) column_value->size[column_nr]);
	  assert(column_value->buf[column_nr] != NULL);

	  column_value->data[column_nr] =
	    (value_data_ptr_t) calloc((size_t) fetch_size,
				      sizeof(column_value->data[column_nr][0]));
	  assert(column_value->data[column_nr] != NULL);

	  DBUG_PRINT("info", ("column_value->buf[%u]= %p", column_nr, column_value->buf[column_nr]));

	  for (array_nr = 0, data_ptr = (char *) column_value->buf[column_nr];
	       array_nr < fetch_size;
	       array_nr++, data_ptr += column_value->size[column_nr])
	    {
	      /*@-observertrans@*/
	      /*@-dependenttrans@*/
	      column_value->data[column_nr][array_nr] = (value_data_t) data_ptr;
	      /*@=observertrans@*/
	      /*@=dependenttrans@*/

#define DBUG_MEMORY 1
#ifdef DBUG_MEMORY
	      DBUG_PRINT("info", ("Dumping column_value->data[%u][%u] (%p)", column_nr, array_nr, column_value->data[column_nr][array_nr]));
	      DBUG_DUMP("info", column_value->data[column_nr][array_nr], (unsigned int) column_value->size[column_nr]);
#endif
	      assert(array_nr == 0 ||
		     (column_value->data[column_nr][array_nr] - column_value->data[column_nr][array_nr-1]) == (int)column_value->size[column_nr]);
	    }

	  column_value->ind[column_nr] = (short *) calloc((size_t) fetch_size, sizeof(**column_value->ind));
	  assert(column_value->ind[column_nr] != NULL);

#ifdef DBUG_MEMORY
	  DBUG_PRINT("info", ("Dumping column_value->data[%u] (%p)", column_nr, column_value->data[column_nr]));
	  DBUG_DUMP("info", column_value->data[column_nr], (unsigned int)(fetch_size * sizeof(**column_value->data)));
	  DBUG_PRINT("info", ("Dumping column_value->ind[%u] (%p)", column_nr, column_value->ind[column_nr]));
	  DBUG_DUMP("info", column_value->ind[column_nr], (unsigned int)(fetch_size * sizeof(**column_value->ind)));
#endif

	  if ((status = sql_value_set(column_value->descriptor_name,
				      column_nr + 1,
				      column_value->array_count,
				      &column_value->descr[column_nr],
				      (char *) column_value->data[column_nr][0],
				      column_value->ind[column_nr])) != OK)
	    break;

	  /* get descriptor info again */
	  if ((status = sql_value_get(column_value->descriptor_name,
				      column_nr + 1,
				      &column_value->descr[column_nr])) != OK)
	    break;
	}
    } while (0);

  return status;
}

static
void
print_heading(/*@in@*/ const settings_t *settings, /*@in@*/ value_info_t *column_value, FILE *fout)
/*@requires notnull column_value->descr, column_value->size @*/
{
  unsigned int column_nr;

  if (settings->column_heading)
    {
      /* print the column headings */
      for (column_nr = 0;
	   column_nr < column_value->value_count;
	   column_nr++)
	{
	  if (column_nr > 0 && settings->column_separator != NULL)
	    (void) fputs(settings->column_separator, fout);
	  if (settings->fixed_column_length)
	    {
	      (void) fprintf(fout,
			     "%-*.*s",
			     (int) column_value->size[column_nr],
			     (int) column_value->size[column_nr],
			     column_value->descr[column_nr].name);
	    }
	  else
	    {
	      (void) fputs(column_value->descr[column_nr].name, fout);
	    }
	}
      if (settings->record_delimiter != NULL)
	{
	  (void) fputs(settings->record_delimiter, fout); /* column heading end */
	}
    }
}

static
void
print_data(/*@in@*/ const settings_t *settings,
	   const unsigned int row_count,
	   const unsigned int total_fetch_size,
	   /*@in@*/ value_info_t *column_value,
	   FILE *fout)
/*@requires notnull column_value->data, column_value->ind, column_value->size @*/
{
  unsigned int column_nr, array_nr;

  for (array_nr = 0; array_nr < row_count; array_nr++)
    {
      DBUG_PRINT("info", ("array_nr: %u", array_nr));

      for (column_nr = 0; column_nr < column_value->value_count; column_nr++)
	{
	  assert(column_value->data[column_nr] != NULL);
	  assert(column_value->ind[column_nr] != NULL);

#ifdef DBUG_MEMORY
	  assert(column_value->data[column_nr][array_nr] != NULL);

	  DBUG_PRINT("info",
		     ("Dumping column_value->data[%u][%u] (%p) after fetch",
		      column_nr,
		      array_nr,
		      column_value->data[column_nr][array_nr]));
	  DBUG_DUMP("info",
		    column_value->data[column_nr][array_nr],
		    (unsigned int) column_value->size[column_nr]);
#endif

	  DBUG_PRINT("info",
		     ("column_value->data[%u][%u]: %s",
		      column_nr,
		      array_nr,
		      (char *) column_value->data[column_nr][array_nr]));

	  if (column_nr > 0 && settings->column_separator != NULL)
	    (void) fputs(settings->column_separator, fout);
	  if (column_value->ind[column_nr][array_nr] != -1) /* not a NULL value? */
	    {
	      if (settings->fixed_column_length)
		{
		  (void) fprintf(fout,
				 "%-*.*s",
				 (int) column_value->size[column_nr],
				 (int) column_value->size[column_nr],
				 (char *) column_value->data[column_nr][array_nr]);
		}
	      else
		{
		  const char *data = (char *) column_value->data[column_nr][array_nr];

		  /* only enclose character data of variable length
		     containing the column separator */
		  if (settings->column_separator != NULL &&
		      settings->column_separator[0] != '\0' &&
		      settings->enclosure_string != NULL &&
		      settings->enclosure_string[0] != '\0' &&
		      strstr(data, settings->column_separator) != NULL)
		    {
		      char *ptr1;
		      char *ptr2;

		      (void) fputs(settings->enclosure_string, fout);

		      /* Add each enclosure string twice.
			 That is what Excel does and SQL*Loader expects. */

		      for (ptr1 = (char *) data;
			   (ptr2 = strstr(ptr1, settings->enclosure_string)) != NULL;
			   ptr1 = ptr2 + strlen(settings->enclosure_string))
			{
			  (void) fprintf(fout, "%*.*s", ptr2 - ptr1, ptr2 - ptr1, ptr1);
			  (void) fputs(settings->enclosure_string, fout);
			  (void) fputs(settings->enclosure_string, fout);
			}

		      (void) fputs(ptr1, fout); /* the remainder */
		      (void) fputs(settings->enclosure_string, fout);
		    }
		  else
		    {
		      (void) fputs(data, fout);
		    }
		}
	    }
	  else
	    {
	      /* print a NULL value */
	      if (settings->fixed_column_length)
		{
		  (void) fprintf(fout,
				 "%-*.*s",
				 (int) column_value->size[column_nr],
				 (int) column_value->size[column_nr],
				 (settings->null != NULL ? settings->null : ""));
		}
	    }
	}
      if (settings->record_delimiter != NULL)
	{
	  (void) fputs(settings->record_delimiter, fout); /* column heading end */
	}
    }

  if (settings->feedback)
    {
      if ((total_fetch_size / settings->fetch_size) % DOTS_PER_LINE == 0)
	{
	  (void) fputs(".\n", stderr);
	}
      else
	{
	  (void) fputs(".", stderr);
	}
    }
}

int
oradumper(const unsigned int nr_arguments, const char **arguments)
{
  unsigned int nr_options;
  typedef enum {
    STEP_CONNECT = 0,
    STEP_NLS_LANGUAGE,
    STEP_NLS_DATE_FORMAT,
    STEP_NLS_TIMESTAMP_FORMAT,
    STEP_NLS_NUMERIC_CHARACTERS,
    STEP_ALLOCATE_DESCRIPTOR_IN,
    STEP_ALLOCATE_DESCRIPTOR_OUT,
    STEP_PARSE,
    STEP_DESCRIBE_INPUT,
    STEP_BIND_VARIABLE,
    STEP_OPEN_CURSOR,
    STEP_DESCRIBE_OUTPUT,
    STEP_FETCH_ROWS

#define STEP_MAX ((int) STEP_FETCH_ROWS)
  } step_t;
  int step;
  int status = OK;
#define NLS_MAX_SIZE 100
  char nls_language_stmt[NLS_MAX_SIZE+1];
  char nls_date_format_stmt[NLS_MAX_SIZE+1];
  char nls_timestamp_format_stmt[NLS_MAX_SIZE+1];
  char nls_numeric_characters_stmt[NLS_MAX_SIZE+1];
  unsigned int total_fetch_size = 0;
  value_info_t bind_value = { 0, 0, "", NULL, NULL, NULL, NULL, NULL };
  unsigned int bind_variable_nr;
  value_info_t column_value = { 0, 0, "", NULL, NULL, NULL, NULL, NULL };
  unsigned int column_nr;
  unsigned int row_count;
  FILE *fout = stdout;
  settings_t settings;

#ifdef WITH_DMALLOC
  unsigned long mark = 0;
#endif

  (void) setlocale(LC_ALL, "");

  memset(&bind_value, 0, sizeof(bind_value));
  (void) strcpy(bind_value.descriptor_name, "input");
  memset(&column_value, 0, sizeof(column_value));
  (void) strcpy(column_value.descriptor_name, "output");
  nr_options = oradumper_process_arguments(nr_arguments, arguments);

  if (nr_options == 0)
    {
#ifdef lint
      free(bind_value.descr);
      free(bind_value.size);
      free(bind_value.buf);
      free(bind_value.data);
      free(bind_value.ind);

      free(column_value.descr);
      free(column_value.size);
      free(column_value.buf);
      free(column_value.data);
      free(column_value.ind);
#endif
      return EXIT_FAILURE;
    }

  settings.userid = get_option(OPTION_USERID);
  settings.sqlstmt = get_option(OPTION_SQLSTMT);
  settings.fetch_size = (unsigned int) atoi(get_option(OPTION_FETCH_SIZE) == NULL ? "100" : get_option(OPTION_FETCH_SIZE));
  settings.dbug_options = get_option(OPTION_DBUG_OPTIONS);
  settings.nls_language = get_option(OPTION_NLS_LANGUAGE);
  settings.nls_date_format = get_option(OPTION_NLS_DATE_FORMAT);
  settings.nls_timestamp_format = get_option(OPTION_NLS_TIMESTAMP_FORMAT);
  settings.nls_numeric_characters = get_option(OPTION_NLS_NUMERIC_CHARACTERS);
  settings.details = OPTION_TRUE(get_option(OPTION_DETAILS));
  settings.record_delimiter = get_option(OPTION_RECORD_DELIMITER);
  settings.feedback = OPTION_TRUE(get_option(OPTION_FEEDBACK));
  settings.column_heading = OPTION_TRUE(get_option(OPTION_COLUMN_HEADING));
  settings.fixed_column_length = OPTION_TRUE(get_option(OPTION_FIXED_COLUMN_LENGTH));
  settings.column_separator = get_option(OPTION_COLUMN_SEPARATOR);
  settings.enclosure_string = get_option(OPTION_ENCLOSURE_STRING);
  settings.output_file = get_option(OPTION_OUTPUT_FILE);
  settings.output_append = OPTION_TRUE(get_option(OPTION_OUTPUT_APPEND));
  settings.null = get_option(OPTION_NULL);

  if (settings.output_file != NULL &&
      (fout = fopen(settings.output_file, (settings.output_append ? "a" : "w"))) == NULL)
    {
      fprintf(stderr, "Could not write to file %s: %s", settings.output_file, strerror(errno));
#ifdef lint
      free(bind_value.descr);
      free(bind_value.size);
      free(bind_value.buf);
      free(bind_value.data);
      free(bind_value.ind);

      free(column_value.descr);
      free(column_value.size);
      free(column_value.buf);
      free(column_value.data);
      free(column_value.ind);
#endif
      return EXIT_FAILURE;
    }

  assert(fout != NULL);

  DBUG_INIT(settings.dbug_options, "oradumper");
  DBUG_ENTER("oradumper");

  for (step = (step_t) 0; step <= STEP_MAX && status == OK; step++)
    {
      switch((step_t) step)
	{
	case STEP_CONNECT:
	  if (settings.userid != NULL)
	    {
	      (void) fprintf(stderr, "Connecting.\n");
	      status = sql_connect(settings.userid);
	    }
	  break;

	case STEP_NLS_LANGUAGE:
#ifdef WITH_DMALLOC
	  mark = dmalloc_mark();
#endif
	  if (settings.nls_language == NULL)
	    break;

	  (void) snprintf(nls_language_stmt,
			  sizeof(nls_language_stmt),
			  "ALTER SESSION SET NLS_LANGUAGE = '%s'",
			  settings.nls_language);

	  status = sql_execute_immediate(nls_language_stmt);
	  break;

	case STEP_NLS_DATE_FORMAT:
	  if (settings.nls_date_format == NULL)
	    break;

	  (void) snprintf(nls_date_format_stmt,
			  sizeof(nls_date_format_stmt),
			  "ALTER SESSION SET NLS_DATE_FORMAT = '%s'",
			  settings.nls_date_format);

	  status = sql_execute_immediate(nls_date_format_stmt);
	  break;

	case STEP_NLS_TIMESTAMP_FORMAT:
	  if (settings.nls_timestamp_format == NULL)
	    break;

	  (void) snprintf(nls_timestamp_format_stmt,
			  sizeof(nls_timestamp_format_stmt),
			  "ALTER SESSION SET NLS_TIMESTAMP_FORMAT = '%s'",
			  settings.nls_timestamp_format);

	  status = sql_execute_immediate(nls_timestamp_format_stmt);
	  break;

	case STEP_NLS_NUMERIC_CHARACTERS:
	  if (settings.nls_numeric_characters == NULL)
	    break;
	
	  (void) snprintf(nls_numeric_characters_stmt,
			  sizeof(nls_numeric_characters_stmt),
			  "ALTER SESSION SET NLS_NUMERIC_CHARACTERS = '%s'",
			  settings.nls_numeric_characters);
      
	  status = sql_execute_immediate(nls_numeric_characters_stmt);
	  break;

	case STEP_ALLOCATE_DESCRIPTOR_IN:
	  bind_value.array_count = 1;
	  status = sql_allocate_descriptor(bind_value.descriptor_name, 1); /* no input bind array */
	  break;

	case STEP_ALLOCATE_DESCRIPTOR_OUT:
	  column_value.array_count = settings.fetch_size;
	  status = sql_allocate_descriptor(column_value.descriptor_name, settings.fetch_size);
	  break;

	case STEP_PARSE:
	  assert(settings.sqlstmt != NULL);

	  (void) fprintf(stderr, "Parsing \"%s\".\n", settings.sqlstmt);

	  status = sql_parse(settings.sqlstmt);
	  break;

	case STEP_DESCRIBE_INPUT:
	  status = sql_describe_input(bind_value.descriptor_name);
	  break;

	case STEP_BIND_VARIABLE:
	  if ((status = sql_value_count(bind_value.descriptor_name, &bind_value.value_count)) != OK)
	    break;

#ifdef lint
	  free(bind_value.descr);
	  free(bind_value.size);
	  free(bind_value.buf);
	  free(bind_value.data);
	  free(bind_value.ind);
#endif

	  bind_value.descr =
	    (value_description_t *) calloc((size_t) bind_value.value_count, sizeof(bind_value.descr[0]));
	  assert(bind_value.descr != NULL);

	  /* bind_value.data[x][y] will point to an argument, hence no allocation is necessary */
	  bind_value.size = NULL;
	  assert(bind_value.size == NULL);
	  bind_value.buf = NULL;
	  assert(bind_value.buf == NULL);

	  bind_value.data =
	    (value_data_t **) calloc((size_t) bind_value.value_count, sizeof(bind_value.data[0]));
	  assert(bind_value.data != NULL);
	  bind_value.ind =
	    (short_ptr_t *) calloc((size_t) bind_value.value_count, sizeof(bind_value.ind[0]));
	  assert(bind_value.ind != NULL);

	  for (bind_variable_nr = 0;
	       bind_variable_nr < bind_value.value_count;
	       bind_variable_nr++)
	    {
	      /* get the bind variable name */
	      if ((status = sql_value_get(bind_value.descriptor_name,
					  bind_variable_nr + 1,
					  &bind_value.descr[bind_variable_nr])) != OK)
		break;

	      bind_value.data[bind_variable_nr] =
		(value_data_ptr_t) calloc((size_t) bind_value.array_count,
					  sizeof(**bind_value.data));
	      assert(bind_value.data[bind_variable_nr] != NULL);
	      bind_value.ind[bind_variable_nr] =
		(short_ptr_t) calloc((size_t) bind_value.array_count,
				     sizeof(**bind_value.ind));
	      assert(bind_value.ind[bind_variable_nr] != NULL);

	      if (nr_options + bind_variable_nr < nr_arguments)
		{
		  bind_value.ind[bind_variable_nr][0] = 0;
		  bind_value.data[bind_variable_nr][0] = (value_data_t) arguments[nr_options + bind_variable_nr];
		}
	      else
		{
		  bind_value.ind[bind_variable_nr][0] = -1;
		  bind_value.data[bind_variable_nr][0] = (value_data_t) "";
		}

	      DBUG_PRINT("info",
			 ("bind variable %u has name %s and value %s",
			  bind_variable_nr + 1,
			  bind_value.descr[bind_variable_nr].name,
			  bind_value.data[bind_variable_nr][0]));

	      if ((status = sql_value_set(bind_value.descriptor_name,
					  bind_variable_nr + 1,
					  bind_value.array_count,
					  &bind_value.descr[bind_variable_nr],
					  (char *) bind_value.data[bind_variable_nr][0],
					  &bind_value.ind[bind_variable_nr][0])) != OK)
		break;
	    }
	  break;

	case STEP_OPEN_CURSOR:
	  status = sql_open_cursor(bind_value.descriptor_name);
	  break;

	case STEP_DESCRIBE_OUTPUT:
	  status = sql_describe_output(column_value.descriptor_name);
	  break;

	case STEP_FETCH_ROWS:
	  if ((status = prepare_fetch(settings.fetch_size, &column_value)) != OK)
	    break;

	  assert(column_value.descr != NULL);
	  assert(column_value.size != NULL);
	  assert(column_value.buf != NULL);
	  assert(column_value.data != NULL);
	  assert(column_value.ind != NULL);

	  /*@-nullstate@*/
	  print_heading(&settings, &column_value, fout);
	  /*@=nullstate@*/

#ifdef DBUG_MEMORY
	  DBUG_PRINT("info", ("Dumping column_value.data (%p)", column_value.data));
	  DBUG_DUMP("info", column_value.data, (unsigned int)(column_value.value_count * sizeof(*column_value.data)));
	  DBUG_PRINT("info", ("Dumping column_value.ind (%p)", column_value.ind));
	  DBUG_DUMP("info", column_value.ind, (unsigned int)(column_value.value_count * sizeof(*column_value.ind)));
#endif
	  row_count = 0;

	  do
	    {
	      if ((status = sql_fetch_rows(column_value.descriptor_name, column_value.array_count, &row_count)) != OK)
		{
		  break;
		}

	      DBUG_PRINT("info", ("rows fetched: %u", row_count));

#define min(x, y) ((x) < (y) ? (x) : (y))

	      total_fetch_size += min(row_count, settings.fetch_size);
	      /*@-nullstate@*/
	      print_data(&settings, min(row_count, settings.fetch_size), total_fetch_size, &column_value, fout);
	      /*@=nullstate@*/
	    }
	  /* row_count < settings.fetch_size means nothing more to fetch */
	  while (status == OK && row_count == settings.fetch_size); 

	  if ((status = sql_rows_processed(&row_count)) != OK)
	    break;
	  
	  (void) fprintf(stderr, "\n%u row(s) processed.\n", row_count);

	  break;
	}
    }
  
  /* Cleanup all resources but do not disconnect */
  if (bind_value.buf != NULL)
    {
      for (bind_variable_nr = 0;
	   bind_variable_nr < bind_value.value_count;
	   bind_variable_nr++)
	{
	  if (bind_value.buf[bind_variable_nr] != NULL)
	    free(bind_value.buf[bind_variable_nr]);
	}
      free(bind_value.buf);
    }
  if (bind_value.data != NULL)
    {
      for (bind_variable_nr = 0;
	   bind_variable_nr < bind_value.value_count;
	   bind_variable_nr++)
	{
	  if (bind_value.data[bind_variable_nr] != NULL)
	    free(bind_value.data[bind_variable_nr]);
	}
      free(bind_value.data);
    }
  if (bind_value.ind != NULL)
    {
      for (bind_variable_nr = 0;
	   bind_variable_nr < bind_value.value_count;
	   bind_variable_nr++)
	{
	  if (bind_value.ind[bind_variable_nr] != NULL)
	    free(bind_value.ind[bind_variable_nr]);
	}
      free(bind_value.ind);
    }
  if (bind_value.descr != NULL)
    free(bind_value.descr);
  if (bind_value.size != NULL)
    free(bind_value.size);

  for (column_nr = 0;
       column_nr < column_value.value_count;
       column_nr++)
    {
      if (column_value.buf != NULL && column_value.buf[column_nr] != NULL)
	free(column_value.buf[column_nr]);

      if (column_value.data != NULL && column_value.data[column_nr] != NULL)
	free(column_value.data[column_nr]);

      if (column_value.ind != NULL && column_value.ind[column_nr] != NULL)
	free(column_value.ind[column_nr]);
    }

  if (column_value.descr != NULL)
    free(column_value.descr);
  if (column_value.size != NULL)
    free(column_value.size);
  if (column_value.buf != NULL)
    free(column_value.buf);
  if (column_value.data != NULL)
    free(column_value.data);
  if (column_value.ind != NULL)
    free(column_value.ind);

  if (status != OK)
    {
      unsigned int msg_length;
      char *msg;

      sql_error(&msg_length, &msg);

      (void) fprintf(stderr, "%*s\n", (int) msg_length, msg);
    }

  /* When status is OK, step should be STEP_MAX + 1 and we should start cleanup at STEP_MAX (i.e. fetch rows) */
  /* when status is not OK, step - 1 failed so we must start at step - 2  */
  switch((step_t) (step - (status == OK ? 1 : 2)))
    {
    case STEP_FETCH_ROWS:
      /*@fallthrough@*/
    case STEP_DESCRIBE_OUTPUT:
      /*@fallthrough@*/
    case STEP_OPEN_CURSOR:
      (void) sql_close_cursor();
      /*@fallthrough@*/
    case STEP_BIND_VARIABLE:
      /*@fallthrough@*/
    case STEP_DESCRIBE_INPUT:
      /*@fallthrough@*/
    case STEP_PARSE:
      /*@fallthrough@*/
    case STEP_ALLOCATE_DESCRIPTOR_OUT:
      (void) sql_deallocate_descriptor(column_value.descriptor_name);
      /*@fallthrough@*/
    case STEP_ALLOCATE_DESCRIPTOR_IN:
      (void) sql_deallocate_descriptor(bind_value.descriptor_name);
      /*@fallthrough@*/
    case STEP_NLS_NUMERIC_CHARACTERS:
      /*@fallthrough@*/
    case STEP_NLS_TIMESTAMP_FORMAT:
      /*@fallthrough@*/
    case STEP_NLS_DATE_FORMAT:
      /*@fallthrough@*/
    case STEP_NLS_LANGUAGE:
      /*@fallthrough@*/
    case STEP_CONNECT:
      break;
    }

  if (settings.output_file != NULL)
    (void) fclose(fout);

#ifdef WITH_DMALLOC
  dmalloc_log_changed(mark, 1, 0, 0);
  /*  assert(dmalloc_count_changed(mark, 1, 0) == 0);*/
#endif

  DBUG_LEAVE();
  DBUG_DONE();

  return status == OK ? EXIT_SUCCESS : EXIT_FAILURE;
}
