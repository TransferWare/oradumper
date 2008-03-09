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

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif

#if HAVE_STRING_H
#include <string.h>
#endif

#if HAVE_SYS_ERRNO_H
#include <sys/errno.h>
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
void FREE(/*@only@*//*@sef@*/ void *ptr);

/*#ifdef lint
#define FREE(x) free(x)
#else*/
#define FREE(x) do { if ((x) != NULL) free(x); } while (0)
/*#endif*/

#define min(x, y) ((x) < (y) ? (x) : (y))
#define max(x, y) ((x) > (y) ? (x) : (y))

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

#define OPTION_USERID_MANDATORY false
#define OPTION_QUERY_MANDATORY true
#define OPTION_FETCH_SIZE_MANDATORY false
#define OPTION_DBUG_OPTIONS_MANDATORY false
#define OPTION_NLS_LANGUAGE_MANDATORY false
#define OPTION_NLS_DATE_FORMAT_MANDATORY false
#define OPTION_NLS_TIMESTAMP_FORMAT_MANDATORY false
#define OPTION_NLS_NUMERIC_CHARACTERS_MANDATORY false
#define OPTION_DETAILS_MANDATORY false
#define OPTION_RECORD_DELIMITER_MANDATORY false
#define OPTION_FEEDBACK_MANDATORY false
#define OPTION_COLUMN_HEADING_MANDATORY false
#define OPTION_FIXED_COLUMN_LENGTH_MANDATORY false
#define OPTION_COLUMN_SEPARATOR_MANDATORY true
#define OPTION_ENCLOSURE_STRING_MANDATORY false
#define OPTION_OUTPUT_FILE_MANDATORY false
#define OPTION_OUTPUT_APPEND_MANDATORY false
#define OPTION_NULL_MANDATORY false

  { "userid", OPTION_USERID_MANDATORY, "Oracle connect string", NULL }, /* userid may be NULL when oradumper is used as a library */
  { "query", OPTION_QUERY_MANDATORY, "Select statement", NULL },
  { "fetch_size", OPTION_FETCH_SIZE_MANDATORY, "Array size", "10" },
  { "dbug_options", OPTION_DBUG_OPTIONS_MANDATORY, "DBUG options", "" },
  { "nls_language", OPTION_NLS_LANGUAGE_MANDATORY, "NLS language", NULL },
  { "nls_date_format", OPTION_NLS_DATE_FORMAT_MANDATORY, "NLS date format", NULL },
  { "nls_timestamp_format", OPTION_NLS_TIMESTAMP_FORMAT_MANDATORY, "NLS timestamp format", NULL },
  { "nls_numeric_characters", OPTION_NLS_NUMERIC_CHARACTERS_MANDATORY, "NLS numeric characters", NULL },
  { "details", OPTION_DETAILS_MANDATORY, "Print details about input and output values: yes, only or no", "no" },
  { "record_delimiter", OPTION_RECORD_DELIMITER_MANDATORY, "Record delimiter", "\\n" }, /* LF */
  { "feedback", OPTION_FEEDBACK_MANDATORY, "Give feedback after every fetch (0 = no feedback)", "0" },
  { "column_heading", OPTION_COLUMN_HEADING_MANDATORY, "Include column names in first line (1 = yes)", "1" },
  { "fixed_column_length", OPTION_FIXED_COLUMN_LENGTH_MANDATORY, "Fixed column length: 0 = yes (fixed), 1 = no (variable)", "0" },
  { "column_separator", OPTION_COLUMN_SEPARATOR_MANDATORY, "The column separator", NULL },
  { "enclosure_string", OPTION_ENCLOSURE_STRING_MANDATORY, "Put around a column when it is variable and it contains the column separator", "\"" },
  { "output_file", OPTION_OUTPUT_FILE_MANDATORY, "The output file", NULL },
  { "output_append", OPTION_OUTPUT_APPEND_MANDATORY, "Append to the output file (1 = yes)?", "0" },
  { "null", OPTION_NULL_MANDATORY, "Value to print for NULL values", NULL },
};

/* convert2ascii - convert a string which may contain escaped characters into a ascii string.

Escape Sequence	Name	Meaning
\a	Alert           Produces an audible or visible alert.
\b	Backspace	Moves the cursor back one position (non-destructive).
\f	Form Feed	Moves the cursor to the first position of the next page.
\n	New Line	Moves the cursor to the first position of the next line.
\r	Carriage Return	Moves the cursor to the first position of the current line.
\t	Horizontal Tab	Moves the cursor to the next horizontal tabular position.
\v	Vertical Tab	Moves the cursor to the next vertical tabular position.
\'	                Produces a single quote. (GJP: not used)
\"		        Produces a double quote. (GJP: not used)
\?		        Produces a question mark. (GJP: not used)
\\		        Produces a single backslash.
\0		        Produces a null character.
\ddd		        Defines one character by the octal digits (base-8 number).
                        Multiple characters may be defined in the same escape sequence,
			but the value is implementation-specific (see examples).
\xdd		        Defines one character by the hexadecimal digit (base-16 number).

Examples:

printf("\12");
Produces the decimal character 10 (x0A Hex).

printf("\xFF");
Produces the decimal character -1 or 255 (depending on sign).

printf("\x123");
Produces a single character (value is undefined). May cause errors.

printf("\0222");
Produces two characters whose values are implementation-specific.

*/
static
/*@null@*/ /*@observer@*/ char *
convert2ascii(const size_t error_msg_size, char *error_msg, char *str)
{
  size_t src, dst;
  char *error = NULL;
  int n, count;
  unsigned int ch;
  const size_t len = strlen(str);

  /* DBUG is only activated here after options have been parsed */

  /*
  DBUG_ENTER("convert2ascii");
  DBUG_PRINT("input", ("str: '%s'", str));
  */

  for (src = dst = 0; error == NULL && str[src] != '\0'; src++, dst++)
    {
      assert(dst <= src);

      /*
      DBUG_PRINT("input", ("str[src=%d]: '%c'", (int)src, str[src]));
      */

      switch(str[src])
	{
	case '\\':
	  switch(str[++src])
	    {
	    case 'a': str[dst] = '\a'; break;
	    case 'b': str[dst] = '\b'; break;
	    case 'f': str[dst] = '\f'; break;
	    case 'n': str[dst] = '\n'; break;
	    case 'r': str[dst] = '\r'; break;
	    case 't': str[dst] = '\t'; break;
	    case 'v': str[dst] = '\v'; break;
	    case '\\': str[dst] = '\\'; break;
	      
	    case '0':
	    case '1':
	    case '2':
	    case '3':
	    case '4':
	    case '5':
	    case '6':
	    case '7': /* begin of an octal character */
	      n = sscanf(&str[src], "%3o%n", &ch, &count);
	      assert(n == 1);
	      str[dst] = (char) ch;
	      src += count - 1; /* src is incremented each loop */
	      break;
	      
	    case 'x':
	    case 'X': /* begin of a hexadecimal character */
	      n = sscanf(&str[++src], "%2x%n", &ch, &count); /* skip the x/X */
	      if (n != 1) /* %n is not counted */
		{
		  (void) snprintf(error_msg, error_msg_size, "Could not convert %s to a hexadecimal number. Scanned items: %d; input count: %d", &str[src], n, count);
		  error = error_msg;
		}
	      else
		{
		  str[dst] = (char) ch;
		  src += count - 1; /* src is incremented each loop */
		}
	      break;
	      
	    default:
	      (void) snprintf(error_msg, error_msg_size, "Illegal escaped string %s", &str[src-1]);
	      error = error_msg;
	      break;
	    }
	  break;

	default:
	  str[dst] = str[src];
	  break;
	}

      /*
      DBUG_PRINT("input", ("str[src=%d]: '%c'", (int)src, str[src]));
      DBUG_PRINT("input", ("str[dst=%d]: '%c'", (int)dst, str[dst]));
      */
    }

  /*@-nullpass@*/
  /*
  DBUG_PRINT("info",
	     ("error: %p; src=%d; original length of str: %d%s",
	      error,
	      (int) src,
	      (int) len,
	      (error != NULL || src == len ? "" : " (convert2ascii error)")));
  */
  /*@=nullpass@*/

  /*
  DBUG_PRINT("output", ("str: '%s'", str));
  */

  assert(error != NULL || src == len);

  if (error == NULL)
    {
      str[dst] = '\0';
    }

  /*
  DBUG_LEAVE();
  */

  return error;
}

static
/*@null@*/ /*@observer@*/ char *
set_option(const option_t option,
	   const char *value,
	   const size_t error_msg_size,
	   char *error_msg,
	   /*@partial@*/ settings_t *settings)
{
  char *error = NULL;

  switch(option)
    {
    case OPTION_USERID:
      FREE(settings->userid);
      settings->userid = strdup(value);
      break;

    case OPTION_QUERY:
      FREE(settings->query);
      settings->query = strdup(value);
      break;

    case OPTION_FETCH_SIZE:
      settings->fetch_size = (unsigned int) atoi(value);
      break;

    case OPTION_DBUG_OPTIONS:
      FREE(settings->dbug_options);
      settings->dbug_options = strdup(value);
      break;

    case OPTION_NLS_LANGUAGE:
      FREE(settings->nls_language);
      settings->nls_language = strdup(value);
      break;

    case OPTION_NLS_DATE_FORMAT:
      FREE(settings->nls_date_format);
      settings->nls_date_format = strdup(value);
      break;

    case OPTION_NLS_TIMESTAMP_FORMAT:
      FREE(settings->nls_timestamp_format);
      settings->nls_timestamp_format = strdup(value);
      break;

    case OPTION_NLS_NUMERIC_CHARACTERS:
      FREE(settings->nls_numeric_characters);
      settings->nls_numeric_characters = strdup(value);
      break;

    case OPTION_DETAILS:
      settings->details = OPTION_TRUE(value);
      break;

    case OPTION_RECORD_DELIMITER:
      FREE(settings->record_delimiter);
      settings->record_delimiter = strdup(value);
      assert(settings->record_delimiter != NULL);
      error = convert2ascii(error_msg_size, error_msg, settings->record_delimiter);
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
      FREE(settings->column_separator);
      settings->column_separator = strdup(value);
      assert(settings->column_separator != NULL);
      error = convert2ascii(error_msg_size, error_msg, settings->column_separator);
      break;

    case OPTION_ENCLOSURE_STRING:
      FREE(settings->enclosure_string);
      settings->enclosure_string = strdup(value);
      assert(settings->enclosure_string != NULL);
      error = convert2ascii(error_msg_size, error_msg, settings->enclosure_string);
      break;

    case OPTION_OUTPUT_FILE:
      FREE(settings->output_file);
      settings->output_file = strdup(value);
      break;

    case OPTION_OUTPUT_APPEND:
      settings->output_append = OPTION_TRUE(value);
      break;

    case OPTION_NULL:
      FREE(settings->null);
      settings->null = strdup(value);
      break;
    }

  return error;
}

void
oradumper_usage(FILE *fout)
{
  size_t i;
  bool mandatory = true;

  (void) fputs("\nUsage: oradumper [PARAMETER=VALUE]... [BIND VALUE]...\n\n", fout);
  do
    {
      (void) fputs((mandatory ? "Mandatory parameters:\n" : "Optional parameters:\n"), fout);

      for (i = 0; i < sizeof(opt)/sizeof(opt[0]); i++)
	{
	  if ((int) opt[i].mandatory != (int) mandatory)
	    continue;

	  (void) fprintf( fout, "  %-30s  %s", opt[i].name, opt[i].desc);
	  if (opt[i].def != NULL)
	    {
	      (void) fprintf( fout, " [default = %s]", opt[i].def);
	    }
	  (void) fputs("\n", fout);
	}
      (void) fputs("\n", fout);

      mandatory = !mandatory;
    }
  while (!mandatory);
  (void) fputs("The values for record_delimiter, column_separator and enclosure_string may contain escaped characters like \\n, \\012 or \\x0a (linefeed).\n\n", fout);
  (void) fputs("The first argument which is not recognized as a PARAMETER=VALUE pair, is a bind value.\n", fout);
  (void) fputs("So abc is a bind value, but abc=xyz is a bind value as well since abc is not a known parameter.\n\n", fout);
}

static
/*@null@*/ /*@observer@*/ char *
oradumper_process_arguments(const unsigned int nr_arguments,
			    const char **arguments,
			    const size_t error_msg_size,
			    char *error_msg,
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
	  error = set_option((option_t) j, opt[j].def, error_msg_size, error_msg, settings);
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
	      if ((error = set_option((option_t) j,
				      (char*)(arguments[i] + strlen(opt[j].name) + 1),
				      error_msg_size,
				      error_msg,
				      settings)) != NULL)
		{
		  (void) snprintf(error_msg,
				  error_msg_size,
				  "%s (%s=%s)",
				  error,
				  opt[j].name,
				  (char*)(arguments[i] + strlen(opt[j].name) + 1));
		  error = error_msg;
		}
	      break;
	    }
	}

      if (j == sizeof(opt)/sizeof(opt[0])) /* arguments[i] not an option */
	{
	  break;
	}
    }

  *nr_options = (unsigned int) i; /* number of options found */

  /* check mandatory options: only when there is no error and there are options found */
  for (j = 0; error == NULL && j < sizeof(opt)/sizeof(opt[0]); j++)
    {
      if (opt[j].mandatory)
	{
	  bool result = true;

	  switch((option_t) j)
	    {
#if OPTION_USERID_MANDATORY
	    case OPTION_USERID: 
	      result = settings->userid != NULL;
	      break;
#endif
		  
#if OPTION_QUERY_MANDATORY
	    case OPTION_QUERY:
	      result = settings->query != NULL;
	      break;
#endif

#if OPTION_FETCH_SIZE_MANDATORY
	    case OPTION_FETCH_SIZE:
	      break;
#endif

#if OPTION_DBUG_OPTIONS_MANDATORY
	    case OPTION_DBUG_OPTIONS:
	      result = settings->dbug_options != NULL;
	      break;
#endif

#if OPTION_NLS_LANGUAGE_MANDATORY
	    case OPTION_NLS_LANGUAGE:
	      result = settings->nls_language != NULL;
	      break;
#endif
		  
#if OPTION_NLS_DATE_FORMAT_MANDATORY
	    case OPTION_NLS_DATE_FORMAT:
	      result = settings->nls_date_format != NULL;
	      break;
#endif
		  
#if OPTION_NLS_TIMESTAMP_FORMAT_MANDATORY
	    case OPTION_NLS_TIMESTAMP_FORMAT:
	      result = settings->nls_timestamp_format != NULL;
	      break;
#endif
		  
#if OPTION_NLS_NUMERIC_CHARACTERS_MANDATORY
	    case OPTION_NLS_NUMERIC_CHARACTERS:
	      result = settings->nls_numeric_characters != NULL;
	      break;
#endif

#if OPTION_DETAILS_MANDATORY
	    case OPTION_DETAILS:
	      break;
#endif

#if OPTION_RECORD_DELIMITER_MANDATORY
	    case OPTION_RECORD_DELIMITER:
	      result = settings->record_delimiter != NULL;
	      break;
#endif

#if OPTION_FEEDBACK_MANDATORY
	    case OPTION_FEEDBACK:
	      break;
#endif

#if OPTION_COLUMN_HEADING_MANDATORY
	    case OPTION_COLUMN_HEADING:
	      break;
#endif

#if OPTION_FIXED_COLUMN_LENGTH_MANDATORY
	    case OPTION_FIXED_COLUMN_LENGTH:
	      break;
#endif

#if OPTION_COLUMN_SEPARATOR_MANDATORY
	    case OPTION_COLUMN_SEPARATOR:
	      if (settings->column_separator == NULL)
		{
		  settings->column_separator = strdup(settings->fixed_column_length ? " " : ",");
		}
	      result = settings->column_separator != NULL;
	      break;
#endif

#if OPTION_ENCLOSURE_STRING_MANDATORY
	    case OPTION_ENCLOSURE_STRING:
	      result = settings->enclosure_string != NULL;
	      break;
#endif

#if OPTION_OUTPUT_FILE_MANDATORY
	    case OPTION_OUTPUT_FILE:
	      result = settings->output_file != NULL;
	      break;
#endif

#if OPTION_OUTPUT_APPEND_MANDATORY
	    case OPTION_OUTPUT_APPEND:
	      break;
#endif

#if OPTION_NULL_MANDATORY
	    case OPTION_NULL:
	      result = settings->null != NULL;
	      break;
#endif
	    default:
	      break;
	    }

	  if (!result)
	    {
	      (void) snprintf(error_msg,
			      error_msg_size,
			      "Option %s mandatory", opt[j].name);
	      error = error_msg;
	      break;
	    }
	}
    }

  if (error != NULL)
    {
      (void) snprintf(error_msg + strlen(error_msg),
		      error_msg_size - strlen(error_msg),
		      "\n\nRun oradumper without arguments for help.\n");
    }

  return error;
}

static
int
prepare_fetch(/*@in@*/ const settings_t *settings, value_info_t *column_value)
{
  int status;
  unsigned int column_nr;
  /*@observer@*/ data_ptr_t data_ptr = NULL;
  unsigned int array_nr;
  
  do
    {
      if ((status = orasql_value_count(column_value->descriptor_name, &column_value->value_count)) != OK)
	break;

      FREE(column_value->descr);
      FREE(column_value->size);
      FREE(column_value->display_size);
      FREE(column_value->align);
      FREE(column_value->buf);
      FREE(column_value->data);
      FREE(column_value->ind);

      column_value->descr =
	(value_description_t *) calloc((size_t) column_value->value_count, sizeof(*column_value->descr));
      assert(column_value->descr != NULL);

      column_value->size =
	(orasql_size_t *) calloc((size_t) column_value->value_count, sizeof(*column_value->size));
      assert(column_value->size != NULL);

      column_value->display_size =
	(orasql_size_t *) calloc((size_t) column_value->value_count, sizeof(*column_value->display_size));
      assert(column_value->display_size != NULL);

      column_value->align =
	(char *) calloc((size_t) column_value->value_count, sizeof(*column_value->align));
      assert(column_value->align != NULL);

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

	  switch (column_value->descr[column_nr].type_orig = column_value->descr[column_nr].type)
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
	      /* Add one character for the negative sign */
	      column_value->descr[column_nr].length++;
	      /* Add one character for the decimal dot (or comma) */
	      if (column_value->descr[column_nr].scale > 0)
		column_value->descr[column_nr].length++;

	      column_value->descr[column_nr].type = ANSI_CHARACTER_VARYING;
	      column_value->descr[column_nr].octet_length = column_value->descr[column_nr].length;
	      column_value->align[column_nr] = 'R';
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
	      /* Add one character for the negative sign */
	      column_value->descr[column_nr].length++;
	      /* Add one character for the decimal dot (or comma). These are floats */
	      column_value->descr[column_nr].length++;
	      /* Add the mantisse and so on */
	      column_value->descr[column_nr].length += 5;

	      column_value->descr[column_nr].type = ANSI_CHARACTER_VARYING;
	      column_value->descr[column_nr].octet_length = column_value->descr[column_nr].length;
	      column_value->align[column_nr] = 'R';
	      break;

	    case ORA_LONG:
	      column_value->descr[column_nr].length = 2000;
	      column_value->descr[column_nr].octet_length = column_value->descr[column_nr].length;
	      column_value->align[column_nr] = 'L';
	      break;

	    case ORA_ROWID:
	      column_value->descr[column_nr].length = 18;
	      column_value->descr[column_nr].octet_length = column_value->descr[column_nr].length;
	      column_value->align[column_nr] = 'L';
	      break;

	    case ANSI_DATE:
	    case ORA_DATE:
	      column_value->descr[column_nr].length = 25;
	      column_value->descr[column_nr].type = ANSI_CHARACTER_VARYING;
	      column_value->descr[column_nr].octet_length = column_value->descr[column_nr].length;
	      column_value->align[column_nr] = 'L';
	      break;

	    case ORA_RAW:
	      column_value->descr[column_nr].length =
		( column_value->descr[column_nr].length == 0
		  ? 512U
		  : column_value->descr[column_nr].length );
	      column_value->descr[column_nr].octet_length = column_value->descr[column_nr].length;
	      column_value->align[column_nr] = 'L';
	      break;

	    case ORA_LONG_RAW:
	      column_value->descr[column_nr].length = 2000;
	      column_value->descr[column_nr].octet_length = column_value->descr[column_nr].length;
	      column_value->align[column_nr] = 'L';
	      break;

	    case ANSI_CHARACTER:
	    case ANSI_CHARACTER_VARYING:
	    case ORA_VARCHAR2:
	    case ORA_STRING:
	    case ORA_VARCHAR:
	      column_value->descr[column_nr].type = ANSI_CHARACTER_VARYING;
	      column_value->align[column_nr] = 'L';
	      break;

	    case ORA_VARNUM:
	    case ORA_VARRAW:
	    case ORA_DISPLAY:
	    case ORA_LONG_VARCHAR:
	    case ORA_LONG_VARRAW:
	    case ORA_CHAR:
	    case ORA_CHARZ:
	      column_value->align[column_nr] = 'L';
	      break;

	    case ORA_UROWID:
	    case ORA_CLOB:
	    case ORA_INTERVAL:
	      column_value->descr[column_nr].type = ANSI_CHARACTER_VARYING;
	      column_value->align[column_nr] = 'L';
	      break;

	    case ORA_BLOB:
	      column_value->align[column_nr] = 'L';
	      break;

#ifndef lint
	    default:
	      column_value->descr[column_nr].type = ANSI_CHARACTER_VARYING;
	      column_value->align[column_nr] = 'L';
#endif
	    }

	  /* Do not receive columns in Unicode but in UTF8 */
	  if (column_value->descr[column_nr].unicode)
	    {
	      column_value->descr[column_nr].length = column_value->descr[column_nr].octet_length;
	    }
	  (void) strcpy(column_value->descr[column_nr].character_set_name, "UTF8");

	  /* add 1 byte for a terminating zero */
	  column_value->size[column_nr] = column_value->descr[column_nr].octet_length + 1;
	  column_value->display_size[column_nr] = 
	    max(
		max(column_value->descr[column_nr].octet_length,
		    (orasql_size_t) strlen(column_value->descr[column_nr].name)),
		(settings->null != NULL ? (orasql_size_t) strlen(settings->null) : (orasql_size_t) 0)
		);

	  /* column_value->data[column_nr][array_nr] points to memory in column_value->buf[column_nr] */
	  column_value->buf[column_nr] = (byte_ptr_t) calloc((size_t) settings->fetch_size, (size_t) column_value->size[column_nr]);
	  assert(column_value->buf[column_nr] != NULL);

	  column_value->data[column_nr] =
	    (value_data_ptr_t) calloc((size_t) settings->fetch_size,
				      sizeof(column_value->data[column_nr][0]));
	  assert(column_value->data[column_nr] != NULL);

	  DBUG_PRINT("info", ("column_value->buf[%u]= %p", column_nr, column_value->buf[column_nr]));

	  for (array_nr = 0, data_ptr = (char *) column_value->buf[column_nr];
	       array_nr < settings->fetch_size;
	       array_nr++, data_ptr += column_value->size[column_nr])
	    {
	      /*@-observertrans@*/
	      /*@-dependenttrans@*/
	      column_value->data[column_nr][array_nr] = (value_data_t) data_ptr;
	      /*@=observertrans@*/
	      /*@=dependenttrans@*/

/*#define DBUG_MEMORY 1*/
#ifdef DBUG_MEMORY
	      DBUG_PRINT("info", ("Dumping column_value->data[%u][%u] (%p)", column_nr, array_nr, column_value->data[column_nr][array_nr]));
	      DBUG_DUMP("info", column_value->data[column_nr][array_nr], (unsigned int) column_value->size[column_nr]);
#endif
	      assert(array_nr == 0 ||
		     (column_value->data[column_nr][array_nr] - column_value->data[column_nr][array_nr-1]) == (int)column_value->size[column_nr]);
	    }

	  column_value->ind[column_nr] = (short *) calloc((size_t) settings->fetch_size, sizeof(**column_value->ind));
	  assert(column_value->ind[column_nr] != NULL);

#ifdef DBUG_MEMORY
	  DBUG_PRINT("info", ("Dumping column_value->data[%u] (%p)", column_nr, column_value->data[column_nr]));
	  DBUG_DUMP("info", column_value->data[column_nr], (unsigned int)(settings->fetch_size * sizeof(**column_value->data)));
	  DBUG_PRINT("info", ("Dumping column_value->ind[%u] (%p)", column_nr, column_value->ind[column_nr]));
	  DBUG_DUMP("info", column_value->ind[column_nr], (unsigned int)(settings->fetch_size * sizeof(**column_value->ind)));
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

	  /* UTF8 should have been used */
	  assert(!column_value->descr[column_nr].unicode);
	}
    } while (0);

  return status;
}

static
void
print_heading(/*@in@*/ const settings_t *settings, /*@in@*/ value_info_t *column_value, FILE *fout)
/*@requires notnull column_value->descr, column_value->display_size, column_value->align @*/
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
	    {
	      (void) fputs(settings->column_separator, fout);
	    }

	  if (settings->fixed_column_length)
	    {
	      if (column_value->align[column_nr] == 'L')
		{
		  (void) fprintf(fout, "%-*s",
				 (int) column_value->display_size[column_nr],
				 column_value->descr[column_nr].name);
		}
	      else
		{
		  (void) fprintf(fout, "%*s",
				 (int) column_value->display_size[column_nr],
				 column_value->descr[column_nr].name);
		}
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
/*@requires notnull column_value->descr, column_value->data, column_value->ind, column_value->size, column_value->display_size, column_value->align @*/
{
  unsigned int column_nr, array_nr;
  int n;
  char *data;
  size_t len;
  char *ptr1;
  char *ptr2;

  DBUG_ENTER("print_data");

  for (array_nr = 0; array_nr < row_count; array_nr++)
    {
      DBUG_PRINT("info", ("array_nr: %u", array_nr));

      for (column_nr = 0; column_nr < column_value->value_count; column_nr++)
	{
	  assert(column_value->data[column_nr] != NULL);
	  assert(column_value->ind[column_nr] != NULL);

	  n = 0; /* characters printed */
	  data = 
	    (column_value->ind[column_nr][array_nr] != -1 /* not a NULL value? */ ?
	     (char *) column_value->data[column_nr][array_nr] :
	     (settings->null != NULL ? settings->null : "")
	     );


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

	  if (column_nr > 0 && settings->column_separator != NULL)
	    {
	      (void) fputs(settings->column_separator, fout);
	    }

	  if (settings->fixed_column_length)
	    {
	      /* fixed length column */
	      if (column_value->align[column_nr] == 'R') /* numeric fields do not need to be enclosed */
		{
		  n += fprintf(fout, "%*s", (int) column_value->display_size[column_nr], data);
		}
	      else
		{
		  n += fprintf(fout, "%-*s", (int) column_value->display_size[column_nr], data);
		}
	    }
	  else if (data[0] == '\0')
	    {
	      ; /* do not print an empty string when the column has variable length */
	    }
	  else if (column_value->align[column_nr] == 'R') /* numeric fields do not need to be enclosed */
	    {
	      n += fprintf(fout, "%s", data);
	    }
	  else /* variable length strings */
	    {
	      /* only enclose character data of variable length
		 containing the column separator */
	      if (settings->column_separator != NULL &&
		  settings->column_separator[0] != '\0' &&
		  settings->enclosure_string != NULL &&
		  settings->enclosure_string[0] != '\0' &&
		  strstr(data, settings->column_separator) != NULL)
		{
		  /* assume fprintf does not return an error */
		  len = (size_t) fprintf(fout, "%s", settings->enclosure_string);

		  n += len;

		  /* Add each enclosure string twice.
		     That is what Excel does and SQL*Loader expects. */

		  for (ptr1 = (char *) data;
		       (ptr2 = strstr(ptr1, settings->enclosure_string)) != NULL;
		       ptr1 = ptr2 + len)
		    {
		      /* assume fprintf returns >= 0 */
		      n += fprintf(fout, "%*.*s%s%s",
				   ptr2 - ptr1,
				   ptr2 - ptr1,
				   ptr1,
				   settings->enclosure_string,
				   settings->enclosure_string);
		    }

		  /* print the remainder */
		  /* assume fprintf returns 0 */
		  n += fprintf(fout, "%s%s", ptr1, settings->enclosure_string);
		}
	      else
		{
		  n += fprintf(fout, "%s", data);
		}
	    }
	  DBUG_PRINT("info", ("characters printed for this column: %d", n));
	}

      /* newline */
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

  DBUG_LEAVE();
}

/*@null@*/ /*@observer@*/
char *
oradumper(const unsigned int nr_arguments,
	  const char **arguments,
	  const int disconnect,
	  const size_t error_msg_size,
	  char *error_msg)
{
  /*@observer@*/ /*@null@*/ char *error = NULL;

  if (nr_arguments == 0)
    {
      oradumper_usage(stderr);
    }
  else
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
      int sqlcode = OK;
#define NLS_MAX_SIZE 100
      char nls_language_stmt[NLS_MAX_SIZE+1];
      char nls_date_format_stmt[NLS_MAX_SIZE+1];
      char nls_timestamp_format_stmt[NLS_MAX_SIZE+1];
      char nls_numeric_characters_stmt[NLS_MAX_SIZE+1];
      unsigned int total_fetch_size = 0;
      value_info_t bind_value = { 0, 0, "", NULL, NULL, NULL, NULL, NULL, NULL, NULL };
      unsigned int bind_variable_nr;
      value_info_t column_value = { 0, 0, "", NULL, NULL, NULL, NULL, NULL, NULL, NULL };
      unsigned int column_nr;
      unsigned int row_count;
      FILE *fout = stdout;
      settings_t settings;

      /*
      (void) setlocale(LC_ALL, ""); TBD: document in README because this is a library
      */

      memset(&bind_value, 0, sizeof(bind_value));
      (void) strcpy(bind_value.descriptor_name, "input");
      memset(&column_value, 0, sizeof(column_value));
      (void) strcpy(column_value.descriptor_name, "output");

      error = oradumper_process_arguments(nr_arguments, arguments, error_msg_size, error_msg, &nr_options, &settings);

#ifndef DBUG_OFF
      if (error == NULL)
	{
	  (void)dbug_init(settings.dbug_options, "oradumper");
	  (void)dbug_enter(__FILE__, "oradumper", __LINE__, NULL);
	}
#endif

      for (step = (step_t) 0; step <= STEP_MAX && error == NULL; step++)
	{
	  switch((step_t) step)
	    {
	    case STEP_OPEN_OUTPUT_FILE:
	      if (settings.output_file != NULL &&
		  (fout = fopen(settings.output_file, (settings.output_append ? "a" : "w"))) == NULL)
		{
		  (void) snprintf(error_msg, error_msg_size, "Could not write to file %s: %s", settings.output_file, strerror(errno));
		  error = error_msg;
		}
	      break;

	    case STEP_CONNECT:
	      if (settings.userid != NULL)
		{
		  (void) fputs("Connecting.\n", stderr);
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

		      (void) fputs("Connecting.\n", stderr);
		      sqlcode = orasql_connect(userid);
		    }
		}
	      break;

	    case STEP_NLS_LANGUAGE:
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

	      FREE(bind_value.descr);
	      FREE(bind_value.size);
	      FREE(bind_value.display_size);
	      FREE(bind_value.align);
	      FREE(bind_value.buf);
	      FREE(bind_value.data);
	      FREE(bind_value.ind);

	      bind_value.descr =
		(value_description_t *) calloc((size_t) bind_value.value_count, sizeof(bind_value.descr[0]));
	      assert(bind_value.descr != NULL);

	      /* bind_value.data[x][y] will point to an argument, hence no allocation is necessary */
	      bind_value.size = NULL;
	      assert(bind_value.size == NULL);
	      bind_value.display_size = NULL;
	      assert(bind_value.display_size == NULL);
	      bind_value.align = NULL;
	      assert(bind_value.align == NULL);
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
	      if ((sqlcode = prepare_fetch(&settings, &column_value)) != OK)
		break;

	      assert(column_value.descr != NULL);
	      assert(column_value.size != NULL);
	      assert(column_value.display_size != NULL);
	      assert(column_value.align != NULL);
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
		  sqlcode = orasql_fetch_rows(column_value.descriptor_name, column_value.array_count, &row_count);

		  DBUG_PRINT("info", ("sqlcode: %d; rows fetched: %u", sqlcode, row_count));

		  if (sqlcode != OK
#ifdef HAVE_GCOV
		      || row_count == 0
#endif
		      )
		    break;


		  total_fetch_size += min(row_count, settings.fetch_size);
		  /*@-nullstate@*/
		  print_data(&settings, min(row_count, settings.fetch_size), total_fetch_size, &column_value, fout);
		  /*@=nullstate@*/
		}
	      /* row_count < settings.fetch_size means nothing more to fetch */
	      while (sqlcode == OK 
		     /* orasql_fetch_rows() must be called when no data is found to get full code coverage in oradumper.pc */
#ifndef HAVE_GCOV
		     && row_count == settings.fetch_size
#endif
		     );

	      if ((sqlcode = orasql_rows_processed(&row_count)) != OK)
		break;
	  
	      (void) fprintf(stderr, "\n%u row(s) processed.\n", row_count);

	      break;
	    }

	  if (sqlcode != OK)
	    {
	      unsigned int msg_length;
	      char *msg;

	      orasql_error(&msg_length, &msg);
	  
	      (void) snprintf(error_msg, error_msg_size, "%*s", (int) msg_length, msg);
	      error = error_msg;
	    }
	}

      /* When everything is OK, step should be STEP_MAX + 1 and we should start cleanup at STEP_MAX (i.e. fetch rows) */
      /* when not everything is OK, step - 1 failed so we must start at step - 2  */
      switch((step_t) (step - (error == NULL ? 1 : 2)))
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
	  (void) orasql_cache_free_all();
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

#ifndef DBUG_OFF
      if (settings.dbug_options != NULL && settings.dbug_options[0] != '\0')
	{
	  (void)dbug_leave(__LINE__, NULL);
	  (void)dbug_done();
	}
#endif

      FREE(settings.userid);
      FREE(settings.query);
      FREE(settings.dbug_options);
      FREE(settings.nls_language);
      FREE(settings.nls_date_format);
      FREE(settings.nls_timestamp_format);
      FREE(settings.nls_numeric_characters);
      FREE(settings.record_delimiter);
      FREE(settings.column_separator);
      FREE(settings.enclosure_string);
      FREE(settings.output_file);
      FREE(settings.null);

      assert(bind_value.buf == NULL);
      /*
      if (bind_value.buf != NULL)
	{
	  for (bind_variable_nr = 0;
	       bind_variable_nr < bind_value.value_count;
	       bind_variable_nr++)
	    {
	      FREE(bind_value.buf[bind_variable_nr]);
	    }
	  FREE(bind_value.buf);
	}
      */
      if (bind_value.data != NULL)
	{
	  for (bind_variable_nr = 0;
	       bind_variable_nr < bind_value.value_count;
	       bind_variable_nr++)
	    {
	      /*@-modobserver@*/
	      FREE(bind_value.data[bind_variable_nr]);
	      /*@=modobserver@*/
	    }
	  FREE(bind_value.data);
	}
      if (bind_value.ind != NULL)
	{
	  for (bind_variable_nr = 0;
	       bind_variable_nr < bind_value.value_count;
	       bind_variable_nr++)
	    {
	      FREE(bind_value.ind[bind_variable_nr]);
	    }
	  FREE(bind_value.ind);
	}
      FREE(bind_value.descr);
      FREE(bind_value.size);
      FREE(bind_value.display_size);
      FREE(bind_value.align);

      for (column_nr = 0;
	   column_nr < column_value.value_count;
	   column_nr++)
	{
	  if (column_value.buf != NULL)
	    FREE(column_value.buf[column_nr]);
	  if (column_value.data != NULL)
	    {
	      /*@-modobserver@*/
	      FREE(column_value.data[column_nr]);
	      /*@=modobserver@*/
	    }

	  if (column_value.ind != NULL)
	    FREE(column_value.ind[column_nr]);
	}

      FREE(column_value.descr);
      FREE(column_value.size);
      FREE(column_value.display_size);
      FREE(column_value.align);
      FREE(column_value.buf);
      FREE(column_value.data);
      FREE(column_value.ind);
    }

  return error;
}
