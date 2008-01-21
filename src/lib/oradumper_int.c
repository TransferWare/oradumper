#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <locale.h>
#include <wchar.h>

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

typedef /*observer*/ char *data_ptr_t;

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
  { "details", 0, "Print details about input and output values: yes, only or no", "no", NULL },
};

static
void
usage(void)
{
  size_t i;

  (void) fprintf( stderr, "\nUsage: oradumper [OPTION=VALUE]... [VALUE]...\n\nOPTION:\n");
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

unsigned int
process_arguments(const unsigned int nr_arguments, const char **arguments)
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
	  usage();
	}
    }

  return (unsigned int) i; /* number of options found */
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
    case OPTION_DETAILS:
      return opt[option].value;
    }

#ifndef lint
  return NULL;
#endif
}

int
oradumper(const unsigned int nr_arguments, const char **arguments)
{
  unsigned int nr_options;
  typedef enum {
    STEP_CONNECT = 0,
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
  char nls_date_format_stmt[NLS_MAX_SIZE+1];
  char nls_timestamp_format_stmt[NLS_MAX_SIZE+1];
  char nls_numeric_characters_stmt[NLS_MAX_SIZE+1];
  const char *userid;
  const char *nls_date_format;
  const char *nls_timestamp_format;
  const char *nls_numeric_characters;
  const char *array_size_str;
  unsigned int array_size = 0;
  const char *sqlstmt;

  value_info_t bind_value = { 0, 0, "", NULL, NULL, NULL, NULL, NULL };
  unsigned int bind_variable_nr;

  value_info_t column_value = { 0, 0, "", NULL, NULL, NULL, NULL, NULL };
  unsigned int column_nr;
  unsigned int array_nr;
#define column_count column_value.value_count
#define column_name column_value.descr[column_nr].name
#define column_type column_value.descr[column_nr].type
#define column_length column_value.descr[column_nr].length
#define column_precision column_value.descr[column_nr].precision
#define column_scale column_value.descr[column_nr].scale
#define column_size column_value.size[column_nr]
#define column_character_set_name column_value.descr[column_nr].character_set_name

  /*@observer@*/ data_ptr_t data_ptr = NULL;
  unsigned int row_count;

#ifdef WITH_DMALLOC
  unsigned long mark = 0;
#endif

  (void) setlocale(LC_ALL, "");

  memset(&bind_value, 0, sizeof(bind_value));
  (void) strcpy(bind_value.descriptor_name, "input");
  memset(&column_value, 0, sizeof(column_value));
  (void) strcpy(column_value.descriptor_name, "output");
  nr_options = process_arguments(nr_arguments, arguments);

  userid = get_option(OPTION_USERID);
  nls_date_format = get_option(OPTION_NLS_DATE_FORMAT);
  nls_timestamp_format = get_option(OPTION_NLS_TIMESTAMP_FORMAT);
  nls_numeric_characters = get_option(OPTION_NLS_NUMERIC_CHARACTERS);
  array_size_str = get_option(OPTION_ARRAYSIZE);
  sqlstmt = get_option(OPTION_SQLSTMT);

  assert(array_size_str != NULL);

  array_size = (unsigned int) atoi(array_size_str);

  DBUG_INIT(get_option(OPTION_DBUG_OPTIONS), "oradumper");
  DBUG_ENTER("oradumper");

  for (step = (step_t) 0; step <= STEP_MAX && status == OK; step++)
    {
      switch((step_t) step)
	{
	case STEP_CONNECT:
	  if (userid != NULL)
	    {
	      (void) fprintf(stderr, "Connecting.\n");
	      status = sql_connect(userid);
	    }
	  break;

	case STEP_NLS_DATE_FORMAT:
#ifdef WITH_DMALLOC
	  mark = dmalloc_mark();
#endif

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
      
	  status = sql_execute_immediate(nls_numeric_characters_stmt);
	  break;

	case STEP_ALLOCATE_DESCRIPTOR_IN:
	  bind_value.array_count = 1;
	  status = sql_allocate_descriptor(bind_value.descriptor_name, 1); /* no input bind array */
	  break;

	case STEP_ALLOCATE_DESCRIPTOR_OUT:
	  column_value.array_count = array_size;
	  status = sql_allocate_descriptor(column_value.descriptor_name, array_size);
	  break;

	case STEP_PARSE:
	  assert(sqlstmt != NULL);

	  (void) fprintf(stderr, "Parsing \"%s\".\n", sqlstmt);

	  status = sql_parse(sqlstmt);
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
	  if ((status = sql_value_count(column_value.descriptor_name, &column_count)) != OK)
	    break;

#ifdef lint
	  free(column_value.descr);
	  free(column_value.size);
	  free(column_value.buf);
	  free(column_value.data);
	  free(column_value.ind);
#endif

	  column_value.descr =
	    (value_description_t *) calloc((size_t) column_count, sizeof(*column_value.descr));
	  assert(column_value.descr != NULL);

	  column_value.size =
	    (sql_size_t *) calloc((size_t) column_value.value_count, sizeof(*column_value.size));
	  assert(column_value.size != NULL);

	  column_value.buf =
	    (byte_ptr_t *) calloc((size_t) column_value.value_count, sizeof(*column_value.buf));
	  assert(column_value.buf != NULL);

	  column_value.data = (value_data_ptr_t *) calloc((size_t) column_count, sizeof(*column_value.data));
	  assert(column_value.data != NULL);

	  column_value.ind = (short_ptr_t *) calloc((size_t) column_count, sizeof(*column_value.ind));
	  assert(column_value.ind != NULL);

	  for (column_nr = 0;
	       column_nr < column_count;
	       column_nr++)
	    {
	      if ((status = sql_value_get(column_value.descriptor_name,
					  column_nr + 1,
					  &column_value.descr[column_nr])) != OK)
		break;

	      /* print the column headings */
	      printf("%s%s", (column_nr > 0 ? "," : ""), column_name);

	      switch (column_type)
		{
		case ANSI_NUMERIC:
		case ORA_NUMBER:
		case ANSI_SMALLINT:
		case ANSI_INTEGER:
		case ORA_INTEGER:
		case ORA_UNSIGNED:
		  column_length = 45;
		  column_type = ANSI_CHARACTER_VARYING;
		  /* add 1 byte for a terminating zero */
		  column_size = column_length + 1;
		  break;

		case ANSI_DECIMAL:
		case ORA_DECIMAL:
		case ANSI_FLOAT:
		case ORA_FLOAT:
		case ANSI_DOUBLE_PRECISION:
		case ANSI_REAL:
		  column_length = 100;
		  column_type = ANSI_CHARACTER_VARYING;
		  /* add 1 byte for a terminating zero */
		  column_size = column_length + 1;
		  break;

		case ORA_LONG:
		  column_length = 2000;
		  break;

		case ORA_ROWID:
		  column_length = 18;
		  break;

		case ANSI_DATE:
		case ORA_DATE:
		  column_length = 25;
		  column_type = ANSI_CHARACTER_VARYING;
		  /* add 1 byte for a terminating zero */
		  column_size = column_length + 1;
		  break;

		case ORA_RAW:
		  column_length = ( column_length == 0 ? 512U : column_length );
		  break;

		case ORA_LONG_RAW:
		  column_length = 2000;
		  break;

		case ANSI_CHARACTER:
		case ANSI_CHARACTER_VARYING:
		case ORA_VARCHAR2:
		case ORA_STRING:
		case ORA_VARCHAR:
		  column_type = ANSI_CHARACTER_VARYING;
		  /* add 1 byte for a terminating zero */
		  column_size = column_length + 1;
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
		  column_type = ANSI_CHARACTER_VARYING;
		  /* add 1 byte for a terminating zero */
		  column_size = column_length + 1;
		  break;

		case ORA_BLOB:
		  column_size = column_length;
		  break;

#ifndef lint
		default:
		  column_type = ANSI_CHARACTER_VARYING;
		  /* add 1 byte for a terminating zero */
		  column_size = column_length + 1;
#endif
		}

	      /* column_value.data[column_nr][array_nr] points to memory in column_value.buf[column_nr] */
	      column_value.buf[column_nr] = (byte_ptr_t) calloc((size_t) array_size, (size_t) column_size);
	      assert(column_value.buf[column_nr] != NULL);

	      column_value.data[column_nr] =
		(value_data_ptr_t) calloc((size_t) array_size,
					  sizeof(column_value.data[column_nr][0]));
	      assert(column_value.data[column_nr] != NULL);

	      DBUG_PRINT("info", ("column_value.buf[%u]= %p", column_nr, column_value.buf[column_nr]));

	      for (array_nr = 0, data_ptr = (char *) column_value.buf[column_nr];
		   array_nr < array_size;
		   array_nr++, data_ptr += column_size)
		{
		  /*@-observertrans@*/
		  /*@-dependenttrans@*/
		  column_value.data[column_nr][array_nr] = (value_data_t) data_ptr;
		  /*@=observertrans@*/
		  /*@=dependenttrans@*/

#define DBUG_MEMORY 1
#ifdef DBUG_MEMORY
		  DBUG_PRINT("info", ("Dumping column_value.data[%u][%u] (%p)", column_nr, array_nr, column_value.data[column_nr][array_nr]));
		  DBUG_DUMP("info", column_value.data[column_nr][array_nr], (unsigned int) column_size);
#endif
		  assert(array_nr == 0 ||
			 (column_value.data[column_nr][array_nr] - column_value.data[column_nr][array_nr-1]) == (int)column_size);
		}

	      column_value.ind[column_nr] = (short *) calloc((size_t) array_size, sizeof(**column_value.ind));
	      assert(column_value.ind[column_nr] != NULL);

#ifdef DBUG_MEMORY
	      DBUG_PRINT("info", ("Dumping column_value.data[%u] (%p)", column_nr, column_value.data[column_nr]));
	      DBUG_DUMP("info", column_value.data[column_nr], (unsigned int)(array_size * sizeof(**column_value.data)));
	      DBUG_PRINT("info", ("Dumping column_value.ind[%u] (%p)", column_nr, column_value.ind[column_nr]));
	      DBUG_DUMP("info", column_value.ind[column_nr], (unsigned int)(array_size * sizeof(**column_value.ind)));
#endif

	      if ((status = sql_value_set(column_value.descriptor_name,
					  column_nr + 1,
					  column_value.array_count,
					  &column_value.descr[column_nr],
					  (char *) column_value.data[column_nr][0],
					  column_value.ind[column_nr])) != OK)
		break;

	      /* get descriptor info again */
	      if ((status = sql_value_get(column_value.descriptor_name,
					  column_nr + 1,
					  &column_value.descr[column_nr])) != OK)
		break;
	    }

	  (void) printf("\n"); /* column heading end */

#ifdef DBUG_MEMORY
	  DBUG_PRINT("info", ("Dumping column_value.data (%p)", column_value.data));
	  DBUG_DUMP("info", column_value.data, (unsigned int)(column_count * sizeof(*column_value.data)));
	  DBUG_PRINT("info", ("Dumping column_value.ind (%p)", column_value.ind));
	  DBUG_DUMP("info", column_value.ind, (unsigned int)(column_count * sizeof(*column_value.ind)));
#endif
	  row_count = 0;

	  do
	    {
	      if ((status = sql_fetch_rows(column_value.descriptor_name, column_value.array_count, &row_count)) != OK)
		{
		  break;
		}

	      DBUG_PRINT("info", ("rows fetched: %u", row_count));

	      for (array_nr = 0; array_nr < array_size && array_nr < row_count; array_nr++)
		{
		  DBUG_PRINT("info", ("array_nr: %u", array_nr));

		  for (column_nr = 0; column_nr < column_count; column_nr++)
		    {
		      assert(column_value.data[column_nr] != NULL);
		      assert(column_value.ind[column_nr] != NULL);

#ifdef DBUG_MEMORY
		      assert(column_value.data[column_nr][array_nr] != NULL);

		      DBUG_PRINT("info", ("Dumping column_value.data[%u][%u] (%p) after fetch", column_nr, array_nr, column_value.data[column_nr][array_nr]));
		      DBUG_DUMP("info",
				column_value.data[column_nr][array_nr],
				(unsigned int) column_size);
#endif

		      DBUG_PRINT("info", ("column_value.data[%u][%u]: %s", column_nr, array_nr, (char *) column_value.data[column_nr][array_nr]));

		      (void) printf("%s%s",
				    (column_nr > 0 ? "," : ""),
				    (column_value.ind[column_nr][array_nr] == -1 ? "NULL" : (char *) column_value.data[column_nr][array_nr]));
		    }
		  (void) printf("\n"); /* line end */
		}
	    }
	  while (status == OK && row_count == array_size); /* row_count < array_size means nothing more to fetch */

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
       column_nr < column_count;
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
    case STEP_CONNECT:
      break;
    }

#ifdef WITH_DMALLOC
  dmalloc_log_changed(mark, 1, 0, 0);
  /*  assert(dmalloc_count_changed(mark, 1, 0) == 0);*/
#endif

  DBUG_LEAVE();
  DBUG_DONE();

  return status;
}
