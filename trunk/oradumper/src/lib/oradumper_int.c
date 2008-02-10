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

extern
char *strdup(const char *str);

typedef /*observer*/ char *data_ptr_t;

/**
 * The maximum number of dots per line before a new line
 */
#define DOTS_PER_LINE 75

typedef enum {
  OPTION_USERID = 0,
  OPTION_QUERY,
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
  /*@null@*/ /*@only@*/ char *userid;
  /*@only@*/ char *query;
  unsigned int fetch_size;
  /*@only@*/ char *dbug_options;
  /*@null@*/ /*@only@*/ char *nls_language;
  /*@null@*/ /*@only@*/ char *nls_date_format;
  /*@null@*/ /*@only@*/ char *nls_timestamp_format;
  /*@null@*/ /*@only@*/ char *nls_numeric_characters;
  bool details;
  /*@only@*/ char *record_delimiter;
  bool feedback;
  bool column_heading;
  bool fixed_column_length;
  /*@only@*/ char *column_separator;
  /*@null@*/ /*@only@*/ char *enclosure_string;
  /*@null@*/ /*@only@*/ char *output_file;
  bool output_append;
  /*@only@*/ char *null;
} settings_t;

#define OPTION_TRUE(opt) strcmp((opt == NULL ? "" : opt), "1") == 0;

static const struct {
  /*@observer@*/ char *name; /* name */
  bool mandatory;
  char *desc; /* description */
  /*@null@*/ char *def; /* default */
} opt[] = { 
  { "userid", false, "Oracle connect string", NULL }, /* userid may be NULL when oradumper is used as a library */
  { "query", true, "Select statement", NULL },
  { "fetch_size", true, "Array size", "10" },
  { "dbug_options", true, "DBUG options", "" },
  { "nls_language", false, "NLS language", NULL },
  { "nls_date_format", false, "NLS date format", NULL },
  { "nls_timestamp_format", false, "NLS timestamp format", NULL },
  { "nls_numeric_characters", false, "NLS numeric characters", NULL },
  { "details", false, "Print details about input and output values: yes, only or no", "no" },
  { "record_delimiter", true, "Record delimiter (hexadecimal)", "0d" },
  { "feedback", true, "Give feedback after every fetch (0 = no feedback)", "0" },
  { "column_heading", true, "Include column names in first line (1 = yes)", "1" },
  { "fixed_column_length", true, "Fixed column length: 0 = yes (fixed), 1 = no (variable)", "0" },
  { "column_separator", true, "The column separator (hexadecimal)", "2c" },
  { "enclosure_string", false, "Characters around a character column in variable length data (hexadecimal)", NULL },
  { "output_file", false, "The output file", NULL },
  { "output_append", false, "Append to the output file (1 = yes)?", NULL },
  { "null", true, "Value to print for NULL values", "NULL" },
};


/* hex2ascii - convert a hexadecimal string into a ascii string */
/* "0a0d" becomes "\r\n" and returns value NULL */
/* "0axx" returns an error message (non-hexadecimal) */
/* "0a0" returns an error message (odd number of characters) */
static
/*@null@*/ /*@observer@*/ char *
hex2ascii(char *str)
{
  size_t i;
  int ch;
  char *error = NULL;

  for (i = 0, ch = 0; error == NULL && str[i] != '\0'; )
    {
      switch(str[i])
	{
	case '0': ch +=  0; break;
	case '1': ch +=  1; break;
	case '2': ch +=  2; break;
	case '3': ch +=  3; break;
	case '4': ch +=  4; break;
	case '5': ch +=  5; break;
	case '6': ch +=  6; break;
	case '7': ch +=  7; break;
	case '8': ch +=  8; break;
	case '9': ch +=  9; break;
	case 'a':
	case 'A': ch += 10; break;
	case 'b':
	case 'B': ch += 11; break;
	case 'c':
	case 'C': ch += 12; break;
	case 'd':
	case 'D': ch += 13; break;
	case 'e':
	case 'E': ch += 14; break;
	case 'f':
	case 'F': ch += 15; break;
	default:
	  error = "Found a non-hexadecimal character";
	  continue;
	}

      if ((++i) % 2 == 0)
	{
	  /* two hexadecimal characters occupy one ascii character */
	  /* i == 2: characters at position 0 and 1 become character at position 0 */
	  /* i == 4: characters at position 2 and 3 become character at position 1 */
	  str[(i-2)/2] = (char) ch;
	}
      else
	{
	  ch *= 16;
	}
    }

  assert(error != NULL || i == strlen(str));

  if (error == NULL)
    {
      if (i % 2 == 0)
	{
	  str[i / 2] = '\0';
	}
      else
	{
	  error = "A hexadecimal string must have an even number of characters";
	}
    }

  return error;
}

static
/*@null@*/ /*@observer@*/ char *
set_option(const option_t option, const char *value, /*@partial@*/ settings_t *settings)
{
  char *error = NULL;

  switch(option)
    {
    case OPTION_USERID:
#ifdef lint
      if (settings->userid != NULL)
#endif
	free(settings->userid);
      settings->userid = strdup(value);
      break;

    case OPTION_QUERY:
#ifdef lint
      if (settings->query != NULL)
#endif
	free(settings->query);
      settings->query = strdup(value);
      break;

    case OPTION_FETCH_SIZE:
      settings->fetch_size = (unsigned int) atoi(value);
      break;

    case OPTION_DBUG_OPTIONS:
#ifdef lint
      if (settings->dbug_options != NULL)
#endif
	free(settings->dbug_options);
      settings->dbug_options = strdup(value);
      break;

    case OPTION_NLS_LANGUAGE:
#ifdef lint
      if (settings->nls_language != NULL)
#endif
	free(settings->nls_language);
      settings->nls_language = strdup(value);
      break;

    case OPTION_NLS_DATE_FORMAT:
#ifdef lint
      if (settings->nls_date_format != NULL)
#endif
	free(settings->nls_date_format);
      settings->nls_date_format = strdup(value);
      break;

    case OPTION_NLS_TIMESTAMP_FORMAT:
#ifdef lint
      if (settings->nls_timestamp_format != NULL)
#endif
	free(settings->nls_timestamp_format);
      settings->nls_timestamp_format = strdup(value);
      break;

    case OPTION_NLS_NUMERIC_CHARACTERS:
#ifdef lint
      if (settings->nls_numeric_characters != NULL)
#endif
	free(settings->nls_numeric_characters);
      settings->nls_numeric_characters = strdup(value);
      break;

    case OPTION_DETAILS:
      settings->details = OPTION_TRUE(value);
      break;

    case OPTION_RECORD_DELIMITER:
#ifdef lint
      if (settings->record_delimiter != NULL)
#endif
	free(settings->record_delimiter);
      settings->record_delimiter = strdup(value);
      error = hex2ascii(settings->record_delimiter);
      break;

    case OPTION_FEEDBACK:
      settings->feedback = OPTION_TRUE(value);
      break;

    case OPTION_COLUMN_HEADING:
      settings->column_heading = OPTION_TRUE(value);
      break;

    case OPTION_FIXED_COLUMN_LENGTH:
      settings->fixed_column_length = OPTION_TRUE(value);
      break;

    case OPTION_COLUMN_SEPARATOR:
#ifdef lint
      if (settings->column_separator != NULL)
#endif
	free(settings->column_separator);
      settings->column_separator = strdup(value);
      error = hex2ascii(settings->column_separator);
      break;

    case OPTION_ENCLOSURE_STRING:
#ifdef lint
      if (settings->enclosure_string != NULL)
#endif
	free(settings->enclosure_string);
      settings->enclosure_string = strdup(value);
      error = hex2ascii(settings->enclosure_string);
      break;

    case OPTION_OUTPUT_FILE:
#ifdef lint
      if (settings->output_file != NULL)
#endif
	free(settings->output_file);
      settings->output_file = strdup(value);
      break;

    case OPTION_OUTPUT_APPEND:
      settings->output_append = OPTION_TRUE(value);
      break;

    case OPTION_NULL:
#ifdef lint
      if (settings->null != NULL)
#endif
	free(settings->null);
      settings->null = strdup(value);
      break;
    }

  return error;
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

static
void
oradumper_process_arguments(const unsigned int nr_arguments,
			    const char **arguments,
			    /*@out@*/ unsigned int *nr_options,
			    /*@out@*/ settings_t *settings)
{
  size_t i = 0, j = 0;
  char *error = NULL;

  (void) memset(settings, 0, sizeof(*settings));

  /* set defaults */
  for (j = 0; error == NULL && j < sizeof(opt)/sizeof(opt[0]); j++)
    {
      if (opt[j].def != NULL)
	{
	  error = set_option((option_t) j, opt[j].def, settings);
	}
    }

  /* error != NULL, means that a default is not correct, i.e. a programming error */
  assert(error == NULL);

  for (i = 0; error == NULL && i < (size_t) nr_arguments; i++)
    {
      for (j = 0; error == NULL && j < sizeof(opt)/sizeof(opt[0]); j++)
	{
	  if (strncmp(arguments[i], opt[j].name, strlen(opt[j].name)) == 0 &&
	      arguments[i][strlen(opt[j].name)] == '=')
	    {
	      if ((error = set_option((option_t) j, (char*)(arguments[i] + strlen(opt[j].name) + 1), settings)) != NULL)
		{
		  (void) fprintf(stderr,
				 "\nERROR: %s (%s=%s)\n",
				 error,
				 opt[j].name,
				 (char*)(arguments[i] + strlen(opt[j].name) + 1));
		}
	      break;
	    }
	}

      if (j == sizeof(opt)/sizeof(opt[0])) /* arguments[i] not an option */
	{
	  break;
	}
    }

  /* mandatory options should not be empty */
  for (j = 0; error == NULL && j < sizeof(opt)/sizeof(opt[0]); j++)
    {
      if (opt[j].mandatory)
	{
	  bool result = true;

	  switch((option_t) j)
	    {
	    case OPTION_USERID: 
	      result = settings->userid != NULL;
	      break;
		  
	    case OPTION_QUERY:
	      result = settings->query != NULL;
	      break;

	    case OPTION_FETCH_SIZE:
	      break;

	    case OPTION_DBUG_OPTIONS:
	      result = settings->dbug_options != NULL;
	      break;

	    case OPTION_NLS_LANGUAGE:
	      result = settings->nls_language != NULL;
	      break;
		  
	    case OPTION_NLS_DATE_FORMAT:
	      result = settings->nls_date_format != NULL;
	      break;
		  
	    case OPTION_NLS_TIMESTAMP_FORMAT:
	      result = settings->nls_timestamp_format != NULL;
	      break;
		  
	    case OPTION_NLS_NUMERIC_CHARACTERS:
	      result = settings->nls_numeric_characters != NULL;
	      break;

	    case OPTION_DETAILS:
	      break;

	    case OPTION_RECORD_DELIMITER:
	      result = settings->record_delimiter != NULL;
	      break;

	    case OPTION_FEEDBACK:
	      break;

	    case OPTION_COLUMN_HEADING:
	      break;

	    case OPTION_FIXED_COLUMN_LENGTH:
	      break;

	    case OPTION_COLUMN_SEPARATOR:
	      result = settings->column_separator != NULL;
	      break;

	    case OPTION_ENCLOSURE_STRING:
	      result = settings->enclosure_string != NULL;
	      break;

	    case OPTION_OUTPUT_FILE:
	      result = settings->output_file != NULL;
	      break;

	    case OPTION_OUTPUT_APPEND:
	      break;

	    case OPTION_NULL:
	      result = settings->null != NULL;
	      break;
	    }

	  if (!result)
	    {
	      error = "Option mandatory";
	      (void) fprintf(stderr, "\nERROR: %s (%s)\n", error, opt[j].name);
	      break;
	    }
	}
    }

  if (error != NULL)
    {
      oradumper_usage(stderr);
      i = 0;
    }

  *nr_options = (unsigned int) i; /* number of options found */
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
      if ((status = orasql_value_count(column_value->descriptor_name, &column_value->value_count)) != OK)
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
	(orasql_size_t *) calloc((size_t) column_value->value_count, sizeof(*column_value->size));
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
	  if ((status = orasql_value_get(column_value->descriptor_name,
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
		(orasql_size_t) (column_value->descr[column_nr].precision <= 0
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
		(orasql_size_t) (column_value->descr[column_nr].precision <= 0
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

	  if ((status = orasql_value_set(column_value->descriptor_name,
					 column_nr + 1,
					 column_value->array_count,
					 &column_value->descr[column_nr],
					 (char *) column_value->data[column_nr][0],
					 column_value->ind[column_nr])) != OK)
	    break;

	  /* get descriptor info again */
	  if ((status = orasql_value_get(column_value->descriptor_name,
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
oradumper(const unsigned int nr_arguments, const char **arguments, const int disconnect)
{
  unsigned int nr_options;
  typedef enum {
    STEP_OPEN_OUTPUT_FILE = 0,
    STEP_CONNECT,
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
  int status = EXIT_SUCCESS;
  int sqlcode = OK;
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

  oradumper_process_arguments(nr_arguments, arguments, &nr_options, &settings);
  if (nr_options == 0)
    {
      status = EXIT_FAILURE;
    }
#ifndef DBUG_OFF
  (void)dbug_init(settings.dbug_options, "oradumper");
  (void)dbug_enter(__FILE__, "oradumper", __LINE__, NULL);
#endif

  for (step = (step_t) 0; step <= STEP_MAX && status == EXIT_SUCCESS && sqlcode == OK; step++)
    {
      switch((step_t) step)
	{
	case STEP_OPEN_OUTPUT_FILE:
	  if (settings.output_file != NULL &&
	      (fout = fopen(settings.output_file, (settings.output_append ? "a" : "w"))) == NULL)
	    {
	      fprintf(stderr, "\nERROR: Could not write to file %s: %s.\n", settings.output_file, strerror(errno));
	      status = EXIT_FAILURE;
	    }
	  assert(fout != NULL);
	  break;

	case STEP_CONNECT:
	  if (settings.userid != NULL)
	    {
	      (void) fprintf(stderr, "Connecting.\n");
	      sqlcode = orasql_connect(settings.userid);
	    }
	  else if ((sqlcode = orasql_connected()) != OK)
	    {
	      /* not connected */
	      char userid[100+1];
	      (void) fputs("Enter userid (e.g. username/password@tns): ", stdout);
	      if (fgets(userid, (int) sizeof(userid), stdin) != NULL)
		{
		  /* strip newline */
		  char *nl = strchr(userid, '\n');

		  if (nl != NULL)
		    {
		      *nl = '\0';
		    }

		  (void) fprintf(stderr, "Connecting.\n");
		  sqlcode = orasql_connect(userid);
		}
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

	  sqlcode = orasql_execute_immediate(nls_language_stmt);
	  break;

	case STEP_NLS_DATE_FORMAT:
	  if (settings.nls_date_format == NULL)
	    break;

	  (void) snprintf(nls_date_format_stmt,
			  sizeof(nls_date_format_stmt),
			  "ALTER SESSION SET NLS_DATE_FORMAT = '%s'",
			  settings.nls_date_format);

	  sqlcode = orasql_execute_immediate(nls_date_format_stmt);
	  break;

	case STEP_NLS_TIMESTAMP_FORMAT:
	  if (settings.nls_timestamp_format == NULL)
	    break;

	  (void) snprintf(nls_timestamp_format_stmt,
			  sizeof(nls_timestamp_format_stmt),
			  "ALTER SESSION SET NLS_TIMESTAMP_FORMAT = '%s'",
			  settings.nls_timestamp_format);

	  sqlcode = orasql_execute_immediate(nls_timestamp_format_stmt);
	  break;

	case STEP_NLS_NUMERIC_CHARACTERS:
	  if (settings.nls_numeric_characters == NULL)
	    break;
	
	  (void) snprintf(nls_numeric_characters_stmt,
			  sizeof(nls_numeric_characters_stmt),
			  "ALTER SESSION SET NLS_NUMERIC_CHARACTERS = '%s'",
			  settings.nls_numeric_characters);
      
	  sqlcode = orasql_execute_immediate(nls_numeric_characters_stmt);
	  break;

	case STEP_ALLOCATE_DESCRIPTOR_IN:
	  bind_value.array_count = 1;
	  sqlcode = orasql_allocate_descriptor(bind_value.descriptor_name, 1); /* no input bind array */
	  break;

	case STEP_ALLOCATE_DESCRIPTOR_OUT:
	  column_value.array_count = settings.fetch_size;
	  sqlcode = orasql_allocate_descriptor(column_value.descriptor_name, settings.fetch_size);
	  break;

	case STEP_PARSE:
	  assert(settings.query != NULL);

	  (void) fprintf(stderr, "Parsing \"%s\".\n", settings.query);

	  sqlcode = orasql_parse(settings.query);
	  break;

	case STEP_DESCRIBE_INPUT:
	  sqlcode = orasql_describe_input(bind_value.descriptor_name);
	  break;

	case STEP_BIND_VARIABLE:
	  if ((sqlcode = orasql_value_count(bind_value.descriptor_name, &bind_value.value_count)) != OK)
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
	      if ((sqlcode = orasql_value_get(bind_value.descriptor_name,
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
	      bind_value.descr[bind_variable_nr].type = ANSI_CHARACTER_VARYING;
	      bind_value.descr[bind_variable_nr].length = (orasql_size_t) strlen((char *)bind_value.data[bind_variable_nr][0]);

	      DBUG_PRINT("info",
			 ("bind variable %u has name %s and value %s",
			  bind_variable_nr + 1,
			  bind_value.descr[bind_variable_nr].name,
			  bind_value.data[bind_variable_nr][0]));

	      if ((sqlcode = orasql_value_set(bind_value.descriptor_name,
					      bind_variable_nr + 1,
					      bind_value.array_count,
					      &bind_value.descr[bind_variable_nr],
					      (char *) bind_value.data[bind_variable_nr][0],
					      &bind_value.ind[bind_variable_nr][0])) != OK)
		break;
	    }
	  break;

	case STEP_OPEN_CURSOR:
	  sqlcode = orasql_open_cursor(bind_value.descriptor_name);
	  break;

	case STEP_DESCRIBE_OUTPUT:
	  sqlcode = orasql_describe_output(column_value.descriptor_name);
	  break;

	case STEP_FETCH_ROWS:
	  if ((sqlcode = prepare_fetch(settings.fetch_size, &column_value)) != OK)
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
	      if ((sqlcode = orasql_fetch_rows(column_value.descriptor_name, column_value.array_count, &row_count)) != OK)
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
	  while (sqlcode == OK && row_count == settings.fetch_size); 

	  if ((sqlcode = orasql_rows_processed(&row_count)) != OK)
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

  if (sqlcode != OK)
    {
      unsigned int msg_length;
      char *msg;

      orasql_error(&msg_length, &msg);

      (void) fprintf(stderr, "\nERROR: %*s\n", (int) msg_length, msg);
    }

  /* When everything is OK, step should be STEP_MAX + 1 and we should start cleanup at STEP_MAX (i.e. fetch rows) */
  /* when not everything is OK, step - 1 failed so we must start at step - 2  */
  switch((step_t) (step - (status == EXIT_SUCCESS && sqlcode == OK ? 1 : 2)))
    {
    case STEP_FETCH_ROWS:
      /*@fallthrough@*/
    case STEP_DESCRIBE_OUTPUT:
      /*@fallthrough@*/
    case STEP_OPEN_CURSOR:
      (void) orasql_close_cursor();
      /*@fallthrough@*/
    case STEP_BIND_VARIABLE:
      /*@fallthrough@*/
    case STEP_DESCRIBE_INPUT:
      /*@fallthrough@*/
    case STEP_PARSE:
      /*@fallthrough@*/
    case STEP_ALLOCATE_DESCRIPTOR_OUT:
      (void) orasql_deallocate_descriptor(column_value.descriptor_name);
      /*@fallthrough@*/
    case STEP_ALLOCATE_DESCRIPTOR_IN:
      (void) orasql_deallocate_descriptor(bind_value.descriptor_name);
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
      if (disconnect != 0)
	{
	  (void) orasql_disconnect();
	}
      /*@fallthrough@*/
    case STEP_OPEN_OUTPUT_FILE:
      if (settings.output_file != NULL)
	(void) fclose(fout);
      break;
    }

#ifdef lint
  if (settings.userid != NULL)
#endif
    free(settings.userid);
#ifdef lint
  if (settings.query != NULL)
#endif
    free(settings.query);
#ifdef lint
  if (settings.dbug_options != NULL)
#endif
    free(settings.dbug_options);
#ifdef lint
  if (settings.nls_language != NULL)
#endif
    free(settings.nls_language);
#ifdef lint
  if (settings.nls_date_format != NULL)
#endif
    free(settings.nls_date_format);
#ifdef lint
  if (settings.nls_timestamp_format != NULL)
#endif
    free(settings.nls_timestamp_format);
#ifdef lint
  if (settings.nls_numeric_characters != NULL)
#endif
    free(settings.nls_numeric_characters);
#ifdef lint
  if (settings.record_delimiter != NULL)
#endif
    free(settings.record_delimiter);
#ifdef lint
  if (settings.column_separator != NULL)
#endif
    free(settings.column_separator);
#ifdef lint
  if (settings.enclosure_string != NULL)
#endif
    free(settings.enclosure_string);
#ifdef lint
  if (settings.output_file != NULL)
#endif
    free(settings.output_file);
#ifdef lint
  if (settings.null != NULL)
#endif
    free(settings.null);

#ifdef WITH_DMALLOC
  dmalloc_log_changed(mark, 1, 0, 0);
  /*  assert(dmalloc_count_changed(mark, 1, 0) == 0);*/
#endif

#ifndef DBUG_OFF
  (void)dbug_leave(__LINE__, NULL);
  (void)dbug_done();
#endif

  return status == EXIT_SUCCESS && sqlcode == OK ? EXIT_SUCCESS : EXIT_FAILURE;
}
