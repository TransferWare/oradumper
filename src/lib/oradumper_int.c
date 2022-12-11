#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>

#ifndef S_SPLINT_S

#if HAVE_ASSERT_H
#include <assert.h>
#endif

#if HAVE_CTYPE_H
#include <ctype.h>
#endif

#if HAVE_LANGINFO_H
#include <langinfo.h>
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

/* include dmalloc as last one */
#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif

#endif /* #ifndef S_SPLINT_S */

#include "oradumper.h"
#include "oradumper_int.h"

/*#define DBUG_MEMORY 1*/

#if !defined(S_SPLINT_S) && !defined(DBUG_OFF)

#ifdef assert
#undef assert
#endif

#define assert(cond) do { if (!(cond)) { DBUG_PRINT("error", ("assertion \"%s\" failed: file \"%s\", line %d", #cond, __FILE__, __LINE__)); } } while (0)

#endif

extern
void FREE(/*@only@*//*@sef@*/ void *ptr);

/*#ifdef lint
#define FREE(x) free(x)
#else*/
#define FREE(x) do { if ((x) != NULL) free(x); } while (0)
/*#endif*/

#ifdef  min
#undef  min
#endif
#define min(x, y) ((x) < (y) ? (x) : (y))
#ifdef  max
#undef  max
#endif
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
  /* GJP 23-11-2009 Setting environment variable NLS_LANG=.utf8 returns UTF8 output. */
  OPTION_NLS_LANG,
  OPTION_NLS_DATE_FORMAT,
  OPTION_NLS_TIMESTAMP_FORMAT,
  OPTION_NLS_TIMESTAMP_TZ_FORMAT,
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
  OPTION_ZERO_BEFORE_DECIMAL_CHARACTER,
  OPTION_LEFT_ALIGN_NUMERIC_COLUMNS,
} option_t;

typedef struct {
  /*@null@*/ /*@only@*/ char *userid;
  /*@only@*/ char *query;
  unsigned int fetch_size;
  /*@null@*/ /*@only@*/ char *nls_lang;
  /*@null@*/ /*@only@*/ char *nls_date_format;
  /*@null@*/ /*@only@*/ char *nls_timestamp_format;
  /*@null@*/ /*@only@*/ char *nls_timestamp_tz_format;
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
  bool zero_before_decimal_character;
  bool left_align_numeric_columns;
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
#define OPTION_NLS_LANG_MANDATORY false
#define OPTION_NLS_DATE_FORMAT_MANDATORY false
#define OPTION_NLS_TIMESTAMP_FORMAT_MANDATORY false
#define OPTION_NLS_TIMESTAMP_TZ_FORMAT_MANDATORY false
#define OPTION_NLS_NUMERIC_CHARACTERS_MANDATORY false
#define OPTION_DETAILS_MANDATORY false
#define OPTION_RECORD_DELIMITER_MANDATORY false
#define OPTION_FEEDBACK_MANDATORY false
#define OPTION_COLUMN_HEADING_MANDATORY false
#define OPTION_FIXED_COLUMN_LENGTH_MANDATORY false
#define OPTION_COLUMN_SEPARATOR_MANDATORY false
#define OPTION_ENCLOSURE_STRING_MANDATORY false
#define OPTION_OUTPUT_FILE_MANDATORY false
#define OPTION_OUTPUT_APPEND_MANDATORY false
#define OPTION_NULL_MANDATORY false
#define OPTION_ZERO_BEFORE_DECIMAL_CHARACTER_MANDATORY false
#define OPTION_LEFT_ALIGN_NUMERIC_COLUMNS_MANDATORY false

  { "userid", OPTION_USERID_MANDATORY, "Oracle connect string", NULL }, /* userid may be NULL when oradumper is used as a library */
  { "query", OPTION_QUERY_MANDATORY, "Select statement", NULL },
  { "fetch_size", OPTION_FETCH_SIZE_MANDATORY, "Array size", "1000" },
  { "nls_lang", OPTION_NLS_LANG_MANDATORY, "Set NLS_LANG environment variable", NULL },
  { "nls_date_format", OPTION_NLS_DATE_FORMAT_MANDATORY, "Set NLS date format", NULL },
  { "nls_timestamp_format", OPTION_NLS_TIMESTAMP_FORMAT_MANDATORY, "Set NLS timestamp format", NULL },
  { "nls_timestamp_tz_format", OPTION_NLS_TIMESTAMP_TZ_FORMAT_MANDATORY, "Set NLS timestamp timezone format", NULL },
  { "nls_numeric_characters", OPTION_NLS_NUMERIC_CHARACTERS_MANDATORY, "Set NLS numeric characters", NULL },
  { "details", OPTION_DETAILS_MANDATORY, "Print details about input and output values (1 = yes)", "0" },
  { "record_delimiter", OPTION_RECORD_DELIMITER_MANDATORY, "Record delimiter", "\\n" }, /* LF */
  { "feedback", OPTION_FEEDBACK_MANDATORY, "Give feedback (0 = no feedback)", "1" },
  { "column_heading", OPTION_COLUMN_HEADING_MANDATORY, "Include column names in first line (1 = yes)", "1" },
  { "fixed_column_length", OPTION_FIXED_COLUMN_LENGTH_MANDATORY, "Fixed column length: 1 = yes (fixed), 0 = no (variable)", "0" },
  { "column_separator", OPTION_COLUMN_SEPARATOR_MANDATORY, "The column separator", "," },
  { "enclosure_string", OPTION_ENCLOSURE_STRING_MANDATORY, "Put around a column when it has a variable length and contains the column separator", "\"" },
  { "output_file", OPTION_OUTPUT_FILE_MANDATORY, "The output file", NULL },
  { "output_append", OPTION_OUTPUT_APPEND_MANDATORY, "Append to the output file (1 = yes)?", "0" },
  { "null", OPTION_NULL_MANDATORY, "Value to print for NULL values", NULL },
  { "zero_before_decimal_character", OPTION_ZERO_BEFORE_DECIMAL_CHARACTER_MANDATORY, "Print zero before a number starting with a decimal character (1 = yes)", "1" },
  { "left_align_numeric_columns", OPTION_LEFT_ALIGN_NUMERIC_COLUMNS_MANDATORY, "Left align numeric columns when the column length is fixed (1 = yes)", "0" },
};

/* convert2ascii - convert a string which may contain escaped characters into a ascii string.

Escape Sequence Name    Meaning
\a      Alert           Produces an audible or visible alert.
\b      Backspace       Moves the cursor back one position (non-destructive).
\f      Form Feed       Moves the cursor to the first position of the next page.
\n      New Line        Moves the cursor to the first position of the next line.
\r      Carriage Return Moves the cursor to the first position of the current line.
\t      Horizontal Tab  Moves the cursor to the next horizontal tabular position.
\v      Vertical Tab    Moves the cursor to the next vertical tabular position.
\'                      Produces a single quote. (GJP: not used)
\"                      Produces a double quote. (GJP: not used)
\?                      Produces a question mark. (GJP: not used)
\\                      Produces a single backslash.
\0                      Produces a null character.
\ddd                    Defines one character by the octal digits (base-8 number).
                        Multiple characters may be defined in the same escape sequence,
                        but the value is implementation-specific (see examples).
\xdd                    Defines one character by the hexadecimal digit (base-16 number).

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
convert2ascii(const size_t error_msg_size, /*@out@*/ char *error_msg, char *str)
{
  size_t src, dst;
  char *error = NULL;
  int n, count;
  unsigned int ch1;
  int ch2;
  const size_t len = strlen(str);

  DBUG_ENTER("convert2ascii");
  DBUG_PRINT("input", ("str: '%s'", str));

  error_msg[0] = '\0';

  for (src = dst = 0; error == NULL && str[src] != '\0'; src++, dst++)
    {
      assert(dst <= src);

      DBUG_PRINT("input", ("str[src=%d]: '%c'", (int)src, str[src]));

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
              n = sscanf(&str[src], "%3o%n", &ch1, &count);
              assert(n == 1);
              str[dst] = (char) ch1;
              src += count - 1; /* src is incremented each loop */
              break;
              
            case 'x':
            case 'X': /* begin of a hexadecimal character */
              n = sscanf(&str[++src], "%2x%n", (unsigned int *)&ch2, &count); /* skip the x/X */
              if (n != 1) /* %n is not counted */
                {
                  (void) snprintf(error_msg, error_msg_size, "Could not convert %s to a hexadecimal number. Scanned items: %d; input count: %d", &str[src], n, count);
                  error = error_msg;
                }
              else
                {
                  str[dst] = (char) ch2;
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

      DBUG_PRINT("input", ("str[src=%d]: '%c'", (int)src, str[src]));
      DBUG_PRINT("input", ("str[dst=%d]: '%c'", (int)dst, str[dst]));
    }

  /*@-nullpass@*/
  DBUG_PRINT("info",
             ("error: %p; src=%d; original length of str: %d%s",
              error,
              (int) src,
              (int) len,
              (error != NULL || src == len ? "" : " (convert2ascii error)")));
  /*@=nullpass@*/

  DBUG_PRINT("output", ("str: '%s'", str));

  assert(error != NULL || src == len);

  if (error == NULL)
    {
      str[dst] = '\0';
    }

  DBUG_LEAVE();

  return error;
}

static
/*@null@*/ /*@observer@*/ char *
set_option(const option_t option,
           const char *value,
           const size_t error_msg_size,
           /*@out@*/ char *error_msg,
           /*@partial@*/ settings_t *settings)
{
  char *error = NULL;

  DBUG_PRINT("info", ("set_option(%d, %s, %d, %p, ...)", (int) option, value, (int)error_msg_size, error_msg));

  error_msg[0] = '\0';

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
      settings->fetch_size = max(1u, settings->fetch_size);
      break;

    case OPTION_NLS_LANG:
      FREE(settings->nls_lang);
      settings->nls_lang = strdup(value);
      break;

    case OPTION_NLS_DATE_FORMAT:
      FREE(settings->nls_date_format);
      settings->nls_date_format = strdup(value);
      break;

    case OPTION_NLS_TIMESTAMP_FORMAT:
      FREE(settings->nls_timestamp_format);
      settings->nls_timestamp_format = strdup(value);
      break;

    case OPTION_NLS_TIMESTAMP_TZ_FORMAT:
      FREE(settings->nls_timestamp_tz_format);
      settings->nls_timestamp_tz_format = strdup(value);
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

    case OPTION_ZERO_BEFORE_DECIMAL_CHARACTER:
      settings->zero_before_decimal_character = OPTION_TRUE(value);
      break;

    case OPTION_LEFT_ALIGN_NUMERIC_COLUMNS:
      settings->left_align_numeric_columns = OPTION_TRUE(value);
      break;
    }

  return error;
}

static
/*@observer@*/
char *
get_type_str(const orasql_datatype_t type)
{
  switch (type)
    {
    case ANSI_CHARACTER:
      return "ANSI_CHARACTER";
    case ANSI_CHARACTER_VARYING:
      return "ANSI_CHARACTER_VARYING";
    case ANSI_DATE:
      return "ANSI_DATE";
#ifndef HAVE_GCOV
    case ANSI_DECIMAL:
      return "ANSI_DECIMAL";
    case ANSI_DOUBLE_PRECISION:
      return "ANSI_DOUBLE_PRECISION";
    case ANSI_FLOAT:
      return "ANSI_FLOAT";
    case ANSI_INTEGER:
      return "ANSI_INTEGER";
#endif      
    case ANSI_NUMERIC:
      return "ANSI_NUMERIC";
#ifndef HAVE_GCOV
    case ANSI_REAL:
      return "ANSI_REAL";
    case ANSI_SMALLINT:
      return "ANSI_SMALLINT";
    case ORA_VARCHAR2:
      return "ORA_VARCHAR2";
    case ORA_NUMBER:
      return "ORA_NUMBER";
    case ORA_INTEGER:
      return "ORA_INTEGER";
    case ORA_FLOAT:
      return "ORA_FLOAT";
    case ORA_STRING:
      return "ORA_STRING";
    case ORA_VARNUM:
      return "ORA_VARNUM";
    case ORA_DECIMAL:
      return "ORA_DECIMAL";
#endif      
    case ORA_LONG:
      return "ORA_LONG";
#ifndef HAVE_GCOV            
    case ORA_VARCHAR:
      return "ORA_VARCHAR";
    case ORA_ROWID:
      return "ORA_ROWID";
    case ORA_DATE:
      return "ORA_DATE";
    case ORA_VARRAW:
      return "ORA_VARRAW";
#endif
    case ORA_RAW:
      return "ORA_RAW";
#ifndef HAVE_GCOV      
    case ORA_LONG_RAW:
      return "ORA_LONG_RAW";
    case ORA_UNSIGNED:
      return "ORA_UNSIGNED";
    case ORA_DISPLAY:
      return "ORA_DISPLAY";
    case ORA_LONG_VARCHAR:
      return "ORA_LONG_VARCHAR";
    case ORA_LONG_VARRAW:
      return "ORA_LONG_VARRAW";
    case ORA_CHAR:
      return "ORA_CHAR";
    case ORA_CHARZ:
      return "ORA_CHARZ";
#endif      
    case ORA_UROWID:
      return "ORA_UROWID";
    case ORA_CLOB:
      return "ORA_CLOB";
#ifndef HAVE_GCOV      
    case ORA_BLOB:
      return "ORA_BLOB";
#endif      
    case ORA_INTERVAL:
      return "ORA_INTERVAL";
    default:
      return "UNKNOWN";
    }
}

static
void
check_value_info(value_info_t *value_info, const bool bind_variable)
{
  unsigned int value_nr;
  unsigned int array_nr;

  DBUG_ENTER("check_value_info");

  DBUG_PRINT("info", ("bind_variable: %d; value_count: %u; array_count: %u; descr: %p; size: %p; align: %p; buf: %p; data: %p; ind: %p; returned_length: %p",
          (int) bind_variable,
                      value_info->value_count,
                      value_info->array_count,
                      value_info->descr,
                      value_info->size,
                      value_info->align,
                      value_info->buf,
                      value_info->data,
                      value_info->ind,
                      value_info->returned_length));

  if (!bind_variable)
    {
      assert(value_info->size != NULL);
      assert(value_info->align != NULL);
      assert(value_info->buf != NULL);
    }
  else
    {
      assert(value_info->size == NULL);
      assert(value_info->align == NULL);
      assert(value_info->buf == NULL);
    }

  for (value_nr = 0; value_nr < value_info->value_count; value_nr++)
    {
      assert(value_info->descr != NULL);
      assert(value_info->data != NULL);
      assert(value_info->ind != NULL);
      assert(value_info->returned_length != NULL);

      DBUG_PRINT("info", ("value_nr: %u; data: %p; ind: %p; returned_length: %p", value_nr, value_info->data[value_nr], value_info->ind[value_nr], value_info->returned_length[value_nr]));
      
      if (!bind_variable)
        {
          assert(value_info->size != NULL);
          assert(value_info->align != NULL);
          assert(value_info->buf != NULL);
          assert(value_info->buf[value_nr] != NULL);
          assert(value_info->align[value_nr] == 'R' || value_info->align[value_nr] == 'L');
          assert((value_info->size[value_nr]-1) >= value_info->descr[value_nr].octet_length);
          
          DBUG_PRINT("info", ("align: %c; size: %u; buf: %p", value_info->align[value_nr], value_info->size[value_nr], value_info->buf[value_nr]));
        }
      
      print_value_description(&value_info->descr[value_nr]);
        
      assert(value_info->data[value_nr] != NULL);
      assert(value_info->ind[value_nr] != NULL);
      assert(value_info->returned_length[value_nr] != NULL);

      /* length is in characters for NCHAR, bytes otherwise */
      assert(value_info->descr[value_nr].national_character != 0 || (value_info->descr[value_nr].octet_length == value_info->descr[value_nr].length));

      for (array_nr = 0; array_nr < value_info->array_count; array_nr++)
        {
          DBUG_PRINT("info", ("array_nr: %u", array_nr));
          
          DBUG_PRINT("info", ("value_count: %u; ind: %hi; returned_length: %hu", array_nr, value_info->ind[value_nr][array_nr], value_info->returned_length[value_nr][array_nr]));

#ifdef DBUG_MEMORY
          DBUG_PRINT("info", ("Dumping value_info->data[%u][%u] (%p)", value_nr, array_nr, value_info->data[value_nr][array_nr]));
          DBUG_DUMP("info", value_info->data[value_nr][array_nr],  value_info->returned_length[value_nr][array_nr] + 1);
#endif

          if (!bind_variable)
            {
              assert(value_info->size != NULL && 
                     (value_info->size[value_nr]-1) >= value_info->returned_length[value_nr][array_nr] &&
                     (size_t) (value_info->size[value_nr]-1) >= strlen((char*)value_info->data[value_nr][array_nr]));
            }
          assert(value_info->ind[value_nr][array_nr] != -1 || value_info->returned_length[value_nr][array_nr] == 0);
          if (!bind_variable)
            {
              assert(array_nr == 0 ||
                     (value_info->data != NULL &&
                      value_info->data[value_nr] != NULL &&
                      value_info->data[value_nr][array_nr] != NULL &&
                      value_info->data[value_nr][array_nr-1] != NULL &&
                      value_info->size != NULL &&
                      (value_info->data[value_nr][array_nr] - value_info->data[value_nr][array_nr-1]) == (int)value_info->size[value_nr]));
            }
          /* data must end with a terminating zero */
          assert(value_info->data[value_nr][array_nr][value_info->returned_length[value_nr][array_nr]] == (unsigned char)'\0');
        }
    }

  DBUG_LEAVE();
}

void
print_value_description(value_description_t *value_description)
{
  DBUG_ENTER("print_value_description");
  DBUG_PRINT("info", ("name: %s; type: %d (%s); type_orig: %d (%s); octet_length: %d; length: %d; display_length: %d; precision: %d; scale: %d; character_set_name: %s; is_numeric: %d; national_character: %u; internal_length: %u",
                      value_description->name,
                      (int) value_description->type,
                      get_type_str(value_description->type),
                      (int) value_description->type_orig,
                      get_type_str(value_description->type_orig),
                      (int) value_description->octet_length,
                      (int) value_description->length,
                      (int) value_description->display_length,
                      (int) value_description->precision,
                      (int) value_description->scale,
                      value_description->character_set_name,
                      (int) value_description->is_numeric,
                      value_description->national_character,
                      value_description->internal_length));
  DBUG_LEAVE();
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
                            /*@out@*/ char *error_msg,
                            /*@out@*/ unsigned int *nr_options,
                            /*@out@*/ settings_t *settings)
{
  size_t i = 0, j = 0;
  char *error = NULL;
  
  DBUG_ENTER("oradumper_process_arguments");

  error_msg[0] = '\0';

  (void) memset(settings, 0, sizeof(*settings));

  /* set defaults */
  for (j = 0; error == NULL && j < sizeof(opt)/sizeof(opt[0]); j++)
    {
      if (opt[j].def != NULL)
        {
          DBUG_PRINT("info", ("option %d; name: %s; mandatory: %d; description: %s; default: '%s'", (int) j, opt[j].name, opt[j].mandatory, opt[j].desc, opt[j].def));
          
          error = set_option((option_t) j, opt[j].def, error_msg_size, error_msg, settings);

          if (error != NULL) { DBUG_PRINT("info", ("error: %s", error)); } /* one line for gcoverage */
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
            case OPTION_USERID: 
#if OPTION_USERID_MANDATORY
              result = settings->userid != NULL;
#endif
              break;
                  
            case OPTION_QUERY:
#if OPTION_QUERY_MANDATORY
              result = settings->query != NULL;
#endif
              break;

            case OPTION_FETCH_SIZE:
#if OPTION_FETCH_SIZE_MANDATORY
#endif
              break;

            case OPTION_NLS_LANG:
#if OPTION_NLS_LANG_MANDATORY
              result = settings->nls_lang != NULL;
#endif
              break;
                  
            case OPTION_NLS_DATE_FORMAT:
#if OPTION_NLS_DATE_FORMAT_MANDATORY
              result = settings->nls_date_format != NULL;
#endif
              break;
                  
            case OPTION_NLS_TIMESTAMP_FORMAT:
#if OPTION_NLS_TIMESTAMP_FORMAT_MANDATORY
              result = settings->nls_timestamp_format != NULL;
#endif
              break;
                  
            case OPTION_NLS_TIMESTAMP_TZ_FORMAT:
#if OPTION_NLS_TIMESTAMP_TZ_FORMAT_MANDATORY
              result = settings->nls_timestamp_tz_format != NULL;
#endif
              break;
                  
            case OPTION_NLS_NUMERIC_CHARACTERS:
#if OPTION_NLS_NUMERIC_CHARACTERS_MANDATORY
              result = settings->nls_numeric_characters != NULL;
#endif
              break;

            case OPTION_DETAILS:
#if OPTION_DETAILS_MANDATORY
#endif
              break;

            case OPTION_RECORD_DELIMITER:
#if OPTION_RECORD_DELIMITER_MANDATORY
              result = settings->record_delimiter != NULL;
#endif
              break;

            case OPTION_FEEDBACK:
#if OPTION_FEEDBACK_MANDATORY
#endif
              break;

            case OPTION_COLUMN_HEADING:
#if OPTION_COLUMN_HEADING_MANDATORY
#endif
              break;

            case OPTION_FIXED_COLUMN_LENGTH:
#if OPTION_FIXED_COLUMN_LENGTH_MANDATORY
#endif
              break;

            case OPTION_COLUMN_SEPARATOR:
#if OPTION_COLUMN_SEPARATOR_MANDATORY
              if (settings->column_separator == NULL)
                {
                  settings->column_separator = strdup(settings->fixed_column_length ? " " : ",");
                }
              result = settings->column_separator != NULL;
#endif
              break;

            case OPTION_ENCLOSURE_STRING:
#if OPTION_ENCLOSURE_STRING_MANDATORY
              result = settings->enclosure_string != NULL;
#endif
              break;

            case OPTION_OUTPUT_FILE:
#if OPTION_OUTPUT_FILE_MANDATORY
              result = settings->output_file != NULL;
#endif
              break;

            case OPTION_OUTPUT_APPEND:
#if OPTION_OUTPUT_APPEND_MANDATORY
#endif
              break;

            case OPTION_NULL:
#if OPTION_NULL_MANDATORY
              result = settings->null != NULL;
#endif
              break;

            case OPTION_ZERO_BEFORE_DECIMAL_CHARACTER:
#if OPTION_ZERO_BEFORE_DECIMAL_CHARACTER_MANDATORY
#endif
              break;

            case OPTION_LEFT_ALIGN_NUMERIC_COLUMNS:
#if OPTION_LEFT_ALIGN_NUMERIC_COLUMNS_MANDATORY
#endif
              break;

#if !defined(HAVE_GCOV) || defined(S_SPLINT_S)
            default:
              break;
#endif              
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

  DBUG_LEAVE();
  
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

  DBUG_ENTER("prepare_fetch");

  if ((status = orasql_value_count(column_value->descriptor_name, &column_value->value_count)) == OK)
    {
      FREE(column_value->descr);
      FREE(column_value->size);
      FREE(column_value->align);
      FREE(column_value->buf);
      FREE(column_value->data);
      FREE(column_value->ind);
      FREE(column_value->returned_length);

      assert(column_value->value_count > 0);

      column_value->descr =
        (value_description_t *) calloc((size_t) column_value->value_count, sizeof(*column_value->descr));
      column_value->size =
        (orasql_size_t *) calloc((size_t) column_value->value_count, sizeof(*column_value->size));
      column_value->align =
        (char *) calloc((size_t) column_value->value_count, sizeof(*column_value->align));
      column_value->buf =
        (byte_ptr_t *) calloc((size_t) column_value->value_count, sizeof(*column_value->buf));
      column_value->data = (value_data_ptr_t *) calloc((size_t) column_value->value_count, sizeof(*column_value->data));
      column_value->ind = (short_ptr_t *) calloc((size_t) column_value->value_count, sizeof(*column_value->ind));
      column_value->returned_length = (orasql_size_ptr_t *) calloc((size_t) column_value->value_count, sizeof(*column_value->returned_length));

      assert(column_value->descr != NULL);
      assert(column_value->size != NULL);
      assert(column_value->align != NULL);
      assert(column_value->buf != NULL);
      assert(column_value->data != NULL);
      assert(column_value->ind != NULL);
      assert(column_value->returned_length != NULL);

      for (column_nr = 0;
           status == OK && column_nr < column_value->value_count;
           column_nr++)
        {
          assert(column_value->descr != NULL);
          assert(column_value->size != NULL);
          assert(column_value->align != NULL);
          assert(column_value->data != NULL);
          assert(column_value->buf != NULL);
          assert(column_value->ind != NULL);
          assert(column_value->returned_length != NULL);

          if ((status = orasql_value_get(column_value->descriptor_name,
                                         column_nr + 1,
                                         &column_value->descr[column_nr])) == OK)
            {
              if (settings->details)
                {
                  (void) fprintf(stderr,
                                 "column[%u] name: %s; type: %d; byte length: %d; precision: %d; scale: %d; character set: %s\n",
                                 column_nr,
                                 column_value->descr[column_nr].name,
                                 column_value->descr[column_nr].type,
                                 (int) column_value->descr[column_nr].octet_length,
                                 column_value->descr[column_nr].precision,
                                 column_value->descr[column_nr].scale,
                                 column_value->descr[column_nr].character_set_name);
                }

/*

SQL*Plus

set long 2000000000 -- 2.000.000.000
set linesize 32767 

SQL> desc oradumper_test
 Name                         Type                                              Display size (linesize 132) Display size (linesize 32767)
 ---------------------------- ------------------------------------------------- --------------------------- ---------------------------
 rowid                        ROWID                                             18                          18
 blob                         BLOB                                              132                         160
 char                         CHAR(2000 CHAR)                                   132                         2000
 clob                         CLOB                                              80                          80
 date                         DATE                                              9                           9
 float                        FLOAT(126)                                        10                          10
 interval day(3) to second(0) INTERVAL DAY(3) TO SECOND(0)                      75                          75
 interval day(3) to second(2) INTERVAL DAY(3) TO SECOND(2)                      75                          75
 interval day(5) to second(1) INTERVAL DAY(5) TO SECOND(1)                      75                          75
 interval day(9) to second(6) INTERVAL DAY(9) TO SECOND(6)                      75                          75
 nchar                        NCHAR(1000)                                       132                         1000
 nclob                        NCLOB                                             80                          80
 number                       NUMBER(10,3)                                      10                          10
 nvarchar2                    NVARCHAR2(2000)                                   132                         2000
 raw                          RAW(2000)                                         132                         255
 timestamp(3)                 TIMESTAMP(3)                                      75                          75
 timestamp(6)                 TIMESTAMP(6)                                      75                          75
 timestamp(6) with time zone  TIMESTAMP(6) WITH TIME ZONE                       75                          75
 varchar2                     VARCHAR2(4000 CHAR)                               132                         4000
  
*/

              DBUG_PRINT("info", ("type: %d (%s)", (int)column_value->descr[column_nr].type, get_type_str(column_value->descr[column_nr].type)));

              column_value->descr[column_nr].is_numeric = false; /* the default */
              switch (column_value->descr[column_nr].type_orig = column_value->descr[column_nr].type)
                {
                case ANSI_NUMERIC:
                case ORA_NUMBER:
                case ANSI_SMALLINT:
                case ANSI_INTEGER:
                case ORA_INTEGER:
                case ORA_UNSIGNED:
                  column_value->descr[column_nr].is_numeric = true;
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
                  column_value->align[column_nr] = (settings->left_align_numeric_columns ? 'L' : 'R');
                  break;

                case ANSI_DECIMAL:
                case ORA_DECIMAL:
                case ANSI_FLOAT:
                case ORA_FLOAT:
                case ANSI_DOUBLE_PRECISION:
                case ANSI_REAL:
                  /* When gcoverage is on, some datatypes are not checked */
#if defined(lint) || !defined(HAVE_GCOV)
                  column_value->descr[column_nr].is_numeric = true;
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
                  column_value->align[column_nr] = (settings->left_align_numeric_columns ? 'L' : 'R');
#endif
                  break;

                case ORA_LONG:
                case ORA_LONG_RAW:
                  column_value->descr[column_nr].type = ANSI_CHARACTER_VARYING;
                  column_value->descr[column_nr].length = 8000;
                  column_value->align[column_nr] = 'L';
                  break;

                case ORA_ROWID:
                case ORA_UROWID:
                  column_value->descr[column_nr].type = ANSI_CHARACTER_VARYING;
                  column_value->descr[column_nr].length = 18;
                  column_value->align[column_nr] = 'L';
                  break;

                case ANSI_DATE:
                case ORA_DATE:
                  column_value->descr[column_nr].type = ANSI_CHARACTER_VARYING;
                  column_value->descr[column_nr].length = 25;
                  column_value->align[column_nr] = 'L';
                  break;

                case ORA_RAW:
                  column_value->descr[column_nr].type = ANSI_CHARACTER_VARYING;
                  column_value->descr[column_nr].length =
                    ( column_value->descr[column_nr].length == 0
                      ? 512U
                      : column_value->descr[column_nr].length );
                  column_value->align[column_nr] = 'L';
                  break;

                case ANSI_CHARACTER:
                case ANSI_CHARACTER_VARYING:
                case ORA_VARCHAR2:
                case ORA_STRING:
                case ORA_VARCHAR:
                case ORA_VARNUM:
                case ORA_VARRAW:
                case ORA_DISPLAY:
                case ORA_LONG_VARCHAR:
                case ORA_LONG_VARRAW:
                case ORA_CHAR:
                case ORA_CHARZ:
                  column_value->descr[column_nr].type = ANSI_CHARACTER_VARYING;
                  column_value->align[column_nr] = 'L';
                  break;

                case ORA_CLOB:
                case ORA_INTERVAL:
                  column_value->descr[column_nr].type = ANSI_CHARACTER_VARYING;
                  column_value->align[column_nr] = 'L';
                  break;

                case ORA_BLOB:
#if defined(lint) || !defined(HAVE_GCOV)
                  column_value->descr[column_nr].type = ANSI_CHARACTER_VARYING;
                  column_value->align[column_nr] = 'L';
#endif
                  break;

#if !defined(lint) && !defined(HAVE_GCOV)
                default:
                  column_value->descr[column_nr].type = ANSI_CHARACTER_VARYING;
                  column_value->align[column_nr] = 'L';
#endif
                }
 
              column_value->descr[column_nr].display_length = column_value->descr[column_nr].length;

              assert(column_value->descr[column_nr].national_character <= 2);
              
              /* Change non national character colums to string columns with character set AL32UTF8 */
              switch (column_value->descr[column_nr].national_character) /* If 2, NCHAR or NVARCHAR2. If 1, character. If 0, non-character. */
                {
                case 1:
                  /* no change whatsoever */
                  column_value->descr[column_nr].display_length = column_value->descr[column_nr].display_length / 4;
                  column_value->descr[column_nr].octet_length = max(column_value->descr[column_nr].octet_length, column_value->descr[column_nr].length);
                  break;
              
                case 0:
                  /*@fallthrough@*/
                case 2:
                  (void) strcpy(column_value->descr[column_nr].character_set_name, "AL32UTF8");
                  /* multiply the length by 4 since AL32UTF8 may need 4 bytes for a character and
                     since national_character equals 0, length is in bytes */
                  column_value->descr[column_nr].length = column_value->descr[column_nr].length * 4;
                  /* set octet_length */
                  column_value->descr[column_nr].octet_length = column_value->descr[column_nr].length;
                  break;
                }

              /* add 1 byte for a terminating zero */
              column_value->size[column_nr] = column_value->descr[column_nr].octet_length + 1;
              /* GJP 23-11-2009 Only set display size to description if there is a heading */
              column_value->descr[column_nr].display_length = 
                max(
                    max(column_value->descr[column_nr].display_length,
                        (settings->column_heading ? (orasql_size_t) strlen(column_value->descr[column_nr].name) : (orasql_size_t) 0)),
                    (settings->null != NULL ? (orasql_size_t) strlen(settings->null) : (orasql_size_t) 0)
                    );

              /* column_value->data[column_nr][array_nr] points to memory in column_value->buf[column_nr] */
              assert(settings->fetch_size > 0);
              assert(column_value->size[column_nr] > 0);
              column_value->buf[column_nr] =
                (byte_ptr_t) calloc((size_t) settings->fetch_size,
                                    (size_t) column_value->size[column_nr]);
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

                  DBUG_PRINT("info", ("column_value->data[%u][%u] (%p)", column_nr, array_nr, column_value->data[column_nr][array_nr]));
              
                  assert(array_nr == 0 ||
                         (column_value->data[column_nr][array_nr] - column_value->data[column_nr][array_nr-1]) == (int)column_value->size[column_nr]);
                }

              column_value->ind[column_nr] =
                (short *) calloc((size_t) settings->fetch_size, sizeof(**column_value->ind));
          
              DBUG_PRINT("info", ("fetch_size: %d; column_value->ind[%u]: %p", (int)settings->fetch_size, column_nr, column_value->ind[column_nr]));
          
              assert(column_value->ind[column_nr] != NULL);

              column_value->returned_length[column_nr] =
                (orasql_size_t *) calloc((size_t) settings->fetch_size, sizeof(**column_value->returned_length));

              DBUG_PRINT("info", ("fetch_size: %d; column_value->returned_length[%u]: %p", (int)settings->fetch_size, column_nr, column_value->returned_length[column_nr]));
          
              assert(column_value->returned_length[column_nr] != NULL);

              if ((status = orasql_value_set(column_value->descriptor_name,
                                             column_nr + 1,
                                             column_value->array_count,
                                             &column_value->descr[column_nr],
                                             (char *) column_value->data[column_nr][0],
                                             column_value->ind[column_nr],
                                             column_value->returned_length[column_nr])) == OK)
                {
                  /* get descriptor info again */
                  if ((status = orasql_value_get(column_value->descriptor_name,
                                                 column_nr + 1,
                                                 &column_value->descr[column_nr])) == OK)
                    {
                      if (settings->details)
                        {
                          (void) fprintf(stderr,
                                         "column[%u] name: %s; type: %d; byte length: %d; precision: %d; scale: %d; character set: %s\n",
                                         column_nr,
                                         column_value->descr[column_nr].name,
                                         column_value->descr[column_nr].type,
                                         (int) column_value->descr[column_nr].octet_length,
                                         column_value->descr[column_nr].precision,
                                         column_value->descr[column_nr].scale,
                                         column_value->descr[column_nr].character_set_name);
                        }
                    }
                }
            } /* if ((status = orasql_value_get(column_value->descriptor_name, */
        }
    } /* if (status == OK) */

      /* test at the end */
  check_value_info(column_value, false);

  DBUG_LEAVE();
  
  return status;
}

static
void
print_heading(/*@in@*/ const settings_t *settings, /*@in@*/ value_info_t *column_value, FILE *fout)
/*@requires notnull column_value->descr, column_value->align @*/
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
                                 (int) column_value->descr[column_nr].display_length,
                                 column_value->descr[column_nr].name);
                }
              else
                {
                  (void) fprintf(fout, "%*s",
                                 (int) column_value->descr[column_nr].display_length,
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
/*@requires notnull column_value->descr, column_value->data, column_value->ind, column_value->size, column_value->align @*/
{
  unsigned int column_nr, array_nr;
  int n;
  char *data;
  static char empty[1] = ""; /* to prevent a lint error */
  int display_size;
  size_t len;
  char *ptr1;
  char *ptr2;
  /* print 
     1) .45 as 0.45
     2) -.45 as -0.45
     3) +.45 as +0.45
  */
  char data_prefix[2+1] = "XX"; /* important: last character must (and will) be the zero terminator */

  DBUG_ENTER("print_data");
  DBUG_PRINT("input", ("row_count: %u; total_fetch_size: %u", row_count, total_fetch_size));

  for (array_nr = 0; array_nr < row_count; array_nr++)
    {
      for (column_nr = 0; column_nr < column_value->value_count; column_nr++)
        {
          DBUG_PRINT("info", ("row %u; column %u", array_nr+1, column_nr+1));
          
          assert(column_value->data[column_nr] != NULL);
          assert(column_value->ind[column_nr] != NULL);

          n = 0; /* characters printed */
          display_size = (int) column_value->descr[column_nr].display_length;
          if (column_value->ind[column_nr][array_nr] != -1)
            {
              /* not a NULL value? */
              data = (char *) column_value->data[column_nr][array_nr];
            }
          else if (settings->null != NULL)
            {
              data = settings->null;
            }
          else
            {
              data = empty;
            }

          data_prefix[0] = '\0';
          if (column_value->descr[column_nr].is_numeric /* non numeric fields will never get a leading zero */
              && settings->zero_before_decimal_character)
            {
              /* the first character in nls_numeric_characters is the decimal character */
              if (data[0] == (settings->nls_numeric_characters == NULL ? '.' : settings->nls_numeric_characters[0]))
                {
                  data_prefix[0] = '0';
                  data_prefix[1] = '\0';
                  
                  DBUG_PRINT("info", ("setting data_prefix to %s", data_prefix));
                }
              else if ((data[0] == '-' || data[0] == '+') &&
                       data[1] == (settings->nls_numeric_characters == NULL ? '.' : settings->nls_numeric_characters[0]))
                {
                  /* the first data character is the sign which must be copied to data_prefix */
                  data_prefix[0] = data[0];
                  data_prefix[1] = '0';
                  /* let data point to the second character, i.e. the decimal character */
                  data++;
                  
                  DBUG_PRINT("info", ("setting data_prefix to %s", data_prefix));
                }
            }

          DBUG_PRINT("info", ("data: %p", data));
          DBUG_PRINT("info", ("data_prefix: %s; data: %s", data_prefix, data));

#ifdef DBUG_MEMORY
          assert(column_value->data[column_nr][array_nr] != NULL);

          DBUG_PRINT("info",
                     ("Dumping column_value->data[%u][%u] (%p) after fetch",
                      column_nr,
                      array_nr,
                      column_value->data[column_nr][array_nr]));
    /* print returned length plus null terminator */
          DBUG_DUMP("info",
                    column_value->data[column_nr][array_nr],
                    (column_value->returned_length[column_nr][array_nr]+1));
#endif

          if (column_nr > 0 && settings->column_separator != NULL)
            {
              (void) fputs(settings->column_separator, fout);
            }

          if (settings->fixed_column_length)
            {
              /* fixed length column */
              if (column_value->align[column_nr] == 'R')
                {
                  n += fprintf(fout, "%*s%s", (int) (display_size - strlen(data)), data_prefix, data);
                }
              else
                {
                  n += fprintf(fout, "%s%-*s", data_prefix, (int) (display_size - strlen(data_prefix)), data);
                }
            }
          else if (data[0] == '\0')
            {
              ; /* do not print an empty string when the column has variable length */
            }
          else if (column_value->descr[column_nr].is_numeric) /* numeric fields do not need to be enclosed */
            {
              n += fprintf(fout, "%s%s", data_prefix, data);
            }
          else /* variable length strings */
            {
              /* 
                 GJP 23-11-2009

                 Only enclose character data of variable length
                 containing the column separator OR the enclosure string.
                 Add each enclosure string twice.

                 Examples ('"' is the enclosure string and ';' is the column separator):
                 1) '"' becomes '""""'
                 2) ';' becomes '";"'
              */
              if (settings->column_separator != NULL &&
                  settings->column_separator[0] != '\0' &&
                  settings->enclosure_string != NULL &&
                  settings->enclosure_string[0] != '\0' &&
                  /* data not empty */
                  data[0] != '\0' &&
                  (strstr(data, settings->column_separator) != NULL ||
                   strstr(data, settings->enclosure_string) != NULL ||
                   /* The implementation of enclosing CSV columns is not correct. */
                   /* 1 - if the data contains a carriage return or line feed the enclosure string must be added */
                   /* 2 - if the data begins or ends with a space the enclosure string must be added */
                   strchr(data, '\r') != NULL ||
                   strchr(data, '\n') != NULL ||
                   data[0] == ' ' ||
                   data[strlen(data)-1] == ' '))
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
                                   (int) (ptr2 - ptr1),
                                   (int) (ptr2 - ptr1),
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

          DBUG_PRINT("info", ("#characters printed for this column: %d; display size: %d", n, display_size));
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
          char *error_msg,
          unsigned int *row_count)
{
  /*@observer@*/ /*@null@*/ char *error = NULL;

  DBUG_ENTER("oradumper");
  DBUG_PRINT("input", ("nr_arguments: %u; disconnect: %d; error_msg_size: %u", nr_arguments, disconnect, (unsigned int)error_msg_size)); 
  
  *row_count = 0;
  error_msg[0] = '\0';

  if (nr_arguments == 0)
    {
      oradumper_usage(stderr);
    }
  else
    {
      unsigned int nr_options;
      typedef enum {
        STEP_OPEN_OUTPUT_FILE = 0,
        STEP_NLS_LANG, /* environment variable needs to be set before a connection is started */
        STEP_CONNECT,
        STEP_NLS_DATE_FORMAT,
        STEP_NLS_TIMESTAMP_FORMAT,
        STEP_NLS_TIMESTAMP_TZ_FORMAT,
        STEP_NLS_NUMERIC_CHARACTERS,
        STEP_ALLOCATE_DESCRIPTOR_IN,
        STEP_ALLOCATE_DESCRIPTOR_OUT,
        STEP_PARSE,
        STEP_DESCRIBE_INPUT,
        STEP_BIND_VALUE,
        STEP_OPEN_CURSOR,
        STEP_DESCRIBE_OUTPUT,
        STEP_FETCH_ROWS

#define STEP_MAX ((int) STEP_FETCH_ROWS)
      } step_t;
      int step;
      int sqlcode = OK;
#define NLS_MAX_SIZE 100
      char nls_lang_stmt[NLS_MAX_SIZE+1];
      int ret;
      char nls_date_format_stmt[NLS_MAX_SIZE+1];
      char nls_timestamp_format_stmt[NLS_MAX_SIZE+1];
      char nls_timestamp_tz_format_stmt[NLS_MAX_SIZE+1];
      char nls_numeric_characters_stmt[NLS_MAX_SIZE+1];
      unsigned int total_fetch_size = 0;
      value_info_t bind_value = { 0, 0, "", NULL, NULL, NULL, NULL, NULL, NULL };
      unsigned int bind_value_nr;
      value_info_t column_value = { 0, 0, "", NULL, NULL, NULL, NULL, NULL, NULL };
      unsigned int column_nr;
      FILE *fout = stdout;
      settings_t settings;

      memset(&bind_value, 0, sizeof(bind_value));
      (void) strcpy(bind_value.descriptor_name, "input");
      memset(&column_value, 0, sizeof(column_value));
      (void) strcpy(column_value.descriptor_name, "output");

      error = oradumper_process_arguments(nr_arguments, arguments, error_msg_size, error_msg, &nr_options, &settings);

#ifndef DBUG_OFF
      if (error == NULL)
        {
#if defined(HAVE_LANGINFO_H) && HAVE_LANGINFO_H != 0 && !defined(S_SPLINT_S)
          DBUG_PRINT("info", ("language info: %s", nl_langinfo(CODESET)));
#endif
        }
#endif

      for (step = (step_t) 0; step <= STEP_MAX && error == NULL; step++)
        {
          switch((step_t) step)
            {
            case STEP_OPEN_OUTPUT_FILE:
              if (settings.output_file != NULL)
                {
                  const char *mode = settings.output_append ? "a" : "w";

                  if ((fout = fopen(settings.output_file, mode)) == NULL)
                    {
                      (void) snprintf(error_msg, error_msg_size, "Could not write to file %s: %s", settings.output_file, strerror(errno));
                      error = error_msg;
                    }
                }
              break;

            case STEP_NLS_LANG:
              if (settings.nls_lang == NULL)
                break;

#if defined(HAVE_SETENV) && HAVE_SETENV != 0
              ret = setenv("NLS_LANG", settings.nls_lang, 1);
#else
              (void) snprintf(nls_lang_stmt,
                              sizeof(nls_lang_stmt),
                              "NLS_LANG=%s",
                              settings.nls_lang);
              ret = putenv(nls_lang_stmt);
#endif

#ifndef HAVE_GCOV              
              if (ret != 0)
                {
                  (void) snprintf(error_msg,
                                  error_msg_size,
                                  "Could not set environment variable NLS_LANG to %s: %s",
                                  settings.nls_lang,
                                  strerror(errno));
                  error = error_msg;
                }
#endif              
              break;

            case STEP_CONNECT:
              if (settings.userid == NULL &&
                  (sqlcode = orasql_connected()) != OK)
                {
                  /* not connected */
                  char userid[100+1];

                  (void) fputs("Enter userid (e.g. username/password@tns): ", stderr);
                  if (fgets(userid, (int) sizeof(userid), stdin) != NULL)
                    {
                      /* strip newline */
                      char *nl = strchr(userid, '\n');

                      if (nl != NULL)
                        {
                          *nl = '\0';
                        }

                      (void) set_option(OPTION_USERID, userid, error_msg_size, error_msg, &settings);
                    }
                }

              if (settings.userid != NULL)
                {
                  if (settings.feedback)
                    {
                      (void) fputs("Connecting.\n", stderr);
                    }
                  sqlcode = orasql_connect(settings.userid);
                }
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

            case STEP_NLS_TIMESTAMP_TZ_FORMAT:
              if (settings.nls_timestamp_tz_format == NULL)
                break;

              (void) snprintf(nls_timestamp_tz_format_stmt,
                              sizeof(nls_timestamp_tz_format_stmt),
                              "ALTER SESSION SET NLS_TIMESTAMP_TZ_FORMAT = '%s'",
                              settings.nls_timestamp_tz_format);

              sqlcode = orasql_execute_immediate(nls_timestamp_tz_format_stmt);
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

              if (settings.feedback)
                {
                  (void) fprintf(stderr, "Parsing \"%s\".\n", settings.query);
                }

              sqlcode = orasql_parse(settings.query);
              break;

            case STEP_DESCRIBE_INPUT:
              sqlcode = orasql_describe_input(bind_value.descriptor_name);
              break;

            case STEP_BIND_VALUE:
              if ((sqlcode = orasql_value_count(bind_value.descriptor_name, &bind_value.value_count)) == OK)
                {
                  FREE(bind_value.descr);
                  FREE(bind_value.size);
                  FREE(bind_value.align);
                  FREE(bind_value.buf);
                  FREE(bind_value.data);
                  FREE(bind_value.ind);
                  FREE(bind_value.returned_length);

                  if (bind_value.value_count == 0)
                    {
                      bind_value.descr = NULL;
                      bind_value.data = NULL;
                      bind_value.ind = NULL;
                      bind_value.returned_length = NULL;
                    }
                  else
                    {
                      bind_value.descr =
                        (value_description_t *) calloc((size_t) bind_value.value_count, sizeof(bind_value.descr[0]));
                      bind_value.data =
                        (value_data_t **) calloc((size_t) bind_value.value_count, sizeof(bind_value.data[0]));
                      bind_value.ind =
                        (short_ptr_t *) calloc((size_t) bind_value.value_count, sizeof(bind_value.ind[0]));
                      bind_value.returned_length =
                        (orasql_size_ptr_t *) calloc((size_t) bind_value.value_count, sizeof(bind_value.returned_length[0]));
                    }
                  assert(bind_value.value_count == 0 || bind_value.descr != NULL);
                  assert(bind_value.value_count == 0 || bind_value.data != NULL);
                  assert(bind_value.value_count == 0 || bind_value.ind != NULL);
                  assert(bind_value.value_count == 0 || bind_value.returned_length != NULL);

                  /* bind_value.data[x][y] will point to an argument, hence no allocation is necessary */
                  bind_value.size = NULL;
                  assert(bind_value.size == NULL);
                  bind_value.align = NULL;
                  assert(bind_value.align == NULL);
                  bind_value.buf = NULL;
                  assert(bind_value.buf == NULL);

                  for (bind_value_nr = 0;
                       sqlcode == OK && bind_value_nr < bind_value.value_count;
                       bind_value_nr++)
                    {
                      assert(bind_value.descr != NULL);
                      assert(bind_value.data != NULL);
                      assert(bind_value.ind != NULL);
                      assert(bind_value.returned_length != NULL);
                      /* get the bind variable name */
                      if ((sqlcode = orasql_value_get(bind_value.descriptor_name,
                                                      bind_value_nr + 1,
                                                      &bind_value.descr[bind_value_nr])) == OK)
                        {
                          if (settings.details)
                            {
                              (void) fprintf(stderr,
                                             "bind value[%u] name: %s; type: %d; byte length: %d; precision: %d; scale: %d; character set: %s\n",
                                             bind_value_nr,
                                             bind_value.descr[bind_value_nr].name,
                                             bind_value.descr[bind_value_nr].type,
                                             (int) bind_value.descr[bind_value_nr].octet_length,
                                             bind_value.descr[bind_value_nr].precision,
                                             bind_value.descr[bind_value_nr].scale,
                                             bind_value.descr[bind_value_nr].character_set_name);
                            }

                          assert(bind_value.array_count > 0);

                          bind_value.data[bind_value_nr] =
                            (value_data_ptr_t) calloc((size_t) bind_value.array_count,
                                                      sizeof(**bind_value.data));
                          assert(bind_value.data[bind_value_nr] != NULL);
                          bind_value.ind[bind_value_nr] =
                            (short_ptr_t) calloc((size_t) bind_value.array_count,
                                                 sizeof(**bind_value.ind));
                          assert(bind_value.ind[bind_value_nr] != NULL);
                          bind_value.returned_length[bind_value_nr] =
                            (orasql_size_ptr_t) calloc((size_t) bind_value.array_count,
                                                       sizeof(**bind_value.returned_length));
                          assert(bind_value.returned_length[bind_value_nr] != NULL);

                          if (nr_options + bind_value_nr < nr_arguments)
                            {
                              bind_value.ind[bind_value_nr][0] = 0;
                              bind_value.data[bind_value_nr][0] = (value_data_t) arguments[nr_options + bind_value_nr];
                            }
                          else
                            {
                              bind_value.ind[bind_value_nr][0] = -1;
                              bind_value.data[bind_value_nr][0] = (value_data_t) "";
                            }
                          bind_value.returned_length[bind_value_nr][0] = (orasql_size_t) strlen((char*)bind_value.data[bind_value_nr][0]);
                          bind_value.descr[bind_value_nr].type = ANSI_CHARACTER_VARYING;
                          bind_value.descr[bind_value_nr].length = (orasql_size_t) strlen((char *)bind_value.data[bind_value_nr][0]);
                          bind_value.descr[bind_value_nr].octet_length = bind_value.descr[bind_value_nr].length;

                          DBUG_PRINT("info",
                                     ("bind variable %u has name %s and value %s",
                                      bind_value_nr + 1,
                                      bind_value.descr[bind_value_nr].name,
                                      bind_value.data[bind_value_nr][0]));

                          sqlcode = orasql_value_set(bind_value.descriptor_name,
                                                     bind_value_nr + 1,
                                                     bind_value.array_count,
                                                     &bind_value.descr[bind_value_nr],
                                                     (char *) bind_value.data[bind_value_nr][0],
                                                     &bind_value.ind[bind_value_nr][0],
                                                     &bind_value.returned_length[bind_value_nr][0]);
                        }
                    }
                  check_value_info(&bind_value, true);
                }
              break;

            case STEP_OPEN_CURSOR:
              sqlcode = orasql_open_cursor(bind_value.descriptor_name);
              break;

            case STEP_DESCRIBE_OUTPUT:
              sqlcode = orasql_describe_output(column_value.descriptor_name);
              break;

            case STEP_FETCH_ROWS:
              if ((sqlcode = prepare_fetch(&settings, &column_value)) == OK)
                {
                  assert(column_value.descr != NULL);
                  assert(column_value.size != NULL);
                  assert(column_value.align != NULL);
                  assert(column_value.buf != NULL);
                  assert(column_value.data != NULL);
                  assert(column_value.ind != NULL);
                  assert(column_value.returned_length != NULL);

                  /*@-nullstate@*/
                  print_heading(&settings, &column_value, fout);
                  /*@=nullstate@*/

                  do
                    {
                      sqlcode = orasql_fetch_rows(column_value.descriptor_name, column_value.array_count, row_count);

                      DBUG_PRINT("info", ("sqlcode: %d; rows fetched: %u", sqlcode, *row_count));

                      check_value_info(&column_value, false);

                      if (sqlcode != OK
#ifdef HAVE_GCOV
                          || *row_count == 0
#endif
                          )
                        break;

                      total_fetch_size += min(*row_count, settings.fetch_size);
                      /*@-nullstate@*/
                      print_data(&settings, min(*row_count, settings.fetch_size), total_fetch_size, &column_value, fout);
                      /*@=nullstate@*/
                    }
                  /* *row_count < settings.fetch_size means nothing more to fetch */
                  while (sqlcode == OK 
                         /* orasql_fetch_rows() must be called when no data is found to get full code coverage in oradumper.pc */
#ifndef HAVE_GCOV
                         && *row_count == settings.fetch_size
#endif
                         );

                  if ((sqlcode = orasql_rows_processed(row_count)) == OK)
                    {
                      if (settings.feedback)
                        {
                          (void) fprintf(stderr, "\n%u row(s) processed.\n", *row_count);
                        }
                    }
                }
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
        case STEP_BIND_VALUE:
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
        case STEP_NLS_TIMESTAMP_TZ_FORMAT:
          /*@fallthrough@*/
        case STEP_NLS_TIMESTAMP_FORMAT:
          /*@fallthrough@*/
        case STEP_NLS_DATE_FORMAT:
          /*@fallthrough@*/
        case STEP_CONNECT:
          /*@fallthrough@*/
        case STEP_NLS_LANG:
          (void) orasql_cache_free_all();
          if (disconnect != 0)
            {
              (void) orasql_disconnect();
            }
          /*@fallthrough@*/
        case STEP_OPEN_OUTPUT_FILE:
          if (settings.output_file != NULL && fout != NULL)
            (void) fclose(fout);
          break;
        }

      FREE(settings.userid);
      FREE(settings.query);
      FREE(settings.nls_lang);
      FREE(settings.nls_date_format);
      FREE(settings.nls_timestamp_format);
      FREE(settings.nls_timestamp_tz_format);
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
          for (bind_value_nr = 0;
               bind_value_nr < bind_value.value_count;
               bind_value_nr++)
            {
              FREE(bind_value.buf[bind_value_nr]);
            }
          FREE(bind_value.buf);
        }
      */
      if (bind_value.data != NULL)
        {
          for (bind_value_nr = 0;
               bind_value_nr < bind_value.value_count;
               bind_value_nr++)
            {
              /*@-modobserver@*/
              FREE(bind_value.data[bind_value_nr]);
              /*@=modobserver@*/
            }
          FREE(bind_value.data);
        }
      if (bind_value.ind != NULL)
        {
          for (bind_value_nr = 0;
               bind_value_nr < bind_value.value_count;
               bind_value_nr++)
            {
              FREE(bind_value.ind[bind_value_nr]);
            }
          FREE(bind_value.ind);
        }
      if (bind_value.returned_length != NULL)
        {
          for (bind_value_nr = 0;
               bind_value_nr < bind_value.value_count;
               bind_value_nr++)
            {
              FREE(bind_value.returned_length[bind_value_nr]);
            }
          FREE(bind_value.returned_length);
        }
      FREE(bind_value.descr);
      FREE(bind_value.size);
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
          
          if (column_value.returned_length != NULL)
            FREE(column_value.returned_length[column_nr]);
        }

      FREE(column_value.descr);
      FREE(column_value.size);
      FREE(column_value.align);
      FREE(column_value.buf);
      FREE(column_value.data);
      FREE(column_value.ind);
      FREE(column_value.returned_length);
    }

  DBUG_PRINT("output", ("row_count: %u; return: %s", *row_count, (error != NULL ? error : "")));
  DBUG_LEAVE();

  return error;
}
