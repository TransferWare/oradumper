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
  { "details", 0, "Print details about input and output values: yes, only or no", "no", NULL },
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
    case OPTION_DETAILS:
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

  value_info_t bind_value = { 0, 0, NULL, NULL, NULL, NULL };
  unsigned int bind_variable_nr;
#define bind_variable_count bind_value.value_count
#define bind_variable_name bind_value.descr[bind_variable_nr].name
#define bind_variable_value bind_value.data[bind_variable_nr][0].charz_ptr

  value_info_t column_value;

  unsigned int column_count = 0, column_nr;
  char column_name[30+1] = "";
  int column_type;
  unsigned int column_length, column_size /* allocated size: length + 1 byte for the terminating zero */;
  int column_precision, column_scale;
  char column_character_set[20+1];
  data_ptr_t **data = NULL;
  /*@observer@*/ data_ptr_t data_ptr = NULL;
  short **ind = NULL;
  unsigned int array_nr;
  unsigned int row_count;

#ifdef WITH_DMALLOC
  unsigned long mark = 0;
#endif

  memset(&bind_value, 0, sizeof(bind_value));
  process_options(length, options);

  userid = get_option(OPTION_USERID);
  nls_date_format = get_option(OPTION_NLS_DATE_FORMAT);
  nls_timestamp_format = get_option(OPTION_NLS_TIMESTAMP_FORMAT);
  nls_numeric_characters = get_option(OPTION_NLS_NUMERIC_CHARACTERS);
  array_size_str = get_option(OPTION_ARRAYSIZE);
  sqlstmt = get_option(OPTION_SQLSTMT);

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

	case STEP_ALLOCATE_DESCRIPTORS:
	  assert(array_size_str != NULL);

	  array_size = (unsigned int) atoi(array_size_str);

	  status = sql_allocate_descriptors(array_size);
	  break;

	case STEP_PARSE:
	  assert(sqlstmt != NULL);

	  (void) fprintf(stderr, "Parsing \"%s\".\n", sqlstmt);

	  status = sql_parse(sqlstmt);
	  break;

	case STEP_BIND_VARIABLE:
	  if ((status = sql_bind_variable_count(&bind_variable_count)) != OK)
	    break;

	  bind_value.array_count = 1;

#ifdef lint
	  free(bind_value.descr);
	  free(bind_value.size);
	  free(bind_value.data);
	  free(bind_value.ind);
#endif

	  bind_value.descr =
	    (value_description_t *) calloc((size_t) bind_value.value_count, sizeof(bind_value.descr[0]));
	  assert(bind_value.descr != NULL);
	  bind_value.size =
	    (size_t *) calloc((size_t) bind_value.value_count, sizeof(bind_value.size[0]));
	  assert(bind_value.size != NULL);
	  bind_value.data =
	    (value_data_t **) calloc((size_t) bind_value.value_count, sizeof(bind_value.data[0]));
	  assert(bind_value.data != NULL);
	  bind_value.ind =
	    (short **) calloc((size_t) bind_value.value_count, sizeof(bind_value.ind[0]));
	  assert(bind_value.ind != NULL);

	  for (bind_variable_nr = 0;
	       bind_variable_nr < bind_variable_count;
	       bind_variable_nr++)
	    {
	      /* get the bind variable name */
	      if ((status = sql_bind_variable_name(bind_variable_nr + 1,
						   sizeof(bind_variable_name),
						   bind_variable_name)) != OK)
		break;

	      bind_value.ind[bind_variable_nr] = NULL; /* no bind variable indicators needed */
	      bind_value.size[bind_variable_nr] =
		sizeof(bind_value.data[bind_variable_nr][0].charz_ptr);
	      assert(bind_value.size[bind_variable_nr] % 4 == 0);
	      bind_value.data[bind_variable_nr] =
		(value_data_t *) calloc((size_t) bind_value.array_count, sizeof(bind_value.size[bind_variable_nr]));
	      assert(bind_value.data[bind_variable_nr] != NULL);

	      if (bind_variable_nr < MAX_BIND_VARIABLES)
		{
		  bind_variable_value = (char *) get_option((option_t)((unsigned int)OPTION_B1 + bind_variable_nr));
		}
	      else
		{
		  bind_variable_value = NULL;
		}

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

	case STEP_FETCH_ROWS:
	  if ((status = sql_column_count(&column_count)) != OK)
	    break;

	  data = (data_ptr_t **) calloc((size_t) column_count, sizeof(*data));
	  assert(data != NULL);

	  ind = (short **) calloc((size_t) column_count, sizeof(*ind));
	  assert(ind != NULL);

	  for (column_nr = 0;
	       column_nr < column_count;
	       column_nr++)
	    {
	      if ((status = sql_describe_column(column_nr + 1,
						sizeof(column_name),
						column_name,
						&column_type,
						&column_length,
						&column_precision,
						&column_scale,
						column_character_set)) != OK)
		break;

	      DBUG_PRINT("info",
			 ("column: %u; name: %s; type: %d; length: %u; precision: %d; scale: %d; character set: %s",
			  column_nr + 1,
			  column_name,
			  column_type,
			  column_length, 
			  column_precision,
			  column_scale,
			  column_character_set));

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
		  break;

		case ANSI_DECIMAL:
		case ORA_DECIMAL:
		case ANSI_FLOAT:
		case ORA_FLOAT:
		case ANSI_DOUBLE_PRECISION:
		case ANSI_REAL:
		  column_length = 100;
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

		case ORA_VARNUM:
		case ORA_VARRAW:
		case ORA_DISPLAY:
		case ORA_LONG_VARCHAR:
		case ORA_LONG_VARRAW:
		case ORA_CHAR:
		case ORA_CHARZ:

		  break;
		}

	      column_type = ANSI_CHARACTER_VARYING;
	      /* add 1 byte for a terminating zero */
	      column_size = column_length + 1;

	      /* column_size must be a multiple of 4 (word boundary): 
		 column_size % 4 == 0 => column_size
		 column_size % 4 == 1 => column_size += 3
		 column_size % 4 == 2 => column_size += 2
		 column_size % 4 == 3 => column_size += 1
	      */
	      column_size = ((unsigned int) ((column_size + 3) / 4)) * 4;
	      column_length = column_size - 1;

	      data[column_nr] = (data_ptr_t *) calloc((size_t) array_size, sizeof(**data));
	      assert(data[column_nr] != NULL);

	      /* data[column_nr][array_nr] points to memory in data[column_nr][0] */
	      data[column_nr][0] = (data_ptr_t) calloc((size_t) array_size, column_size * sizeof(***data));
	      assert(data[column_nr][0] != NULL);

	      DBUG_PRINT("info", ("data[%u][0]= %p", column_nr, data[column_nr][0]));

	      for (array_nr = 0, data_ptr = data[column_nr][0];
		   array_nr < array_size;
		   array_nr++, data_ptr += column_size)
		{
		  /*@-modobserver@*/
		  data[column_nr][array_nr] = data_ptr;
		  /*@=modobserver@*/

#ifdef DBUG_MEMORY
		  DBUG_PRINT("info", ("Dumping data[%u][%u] (%p)", column_nr, array_nr, data[column_nr][array_nr]));
		  DBUG_DUMP("info", data[column_nr][array_nr], (unsigned int) column_size);
#endif
		  assert(array_nr == 0 ||
			 ((char*)data[column_nr][array_nr] - (char*)data[column_nr][array_nr-1]) == (int)column_size);
		}

	      ind[column_nr] = (short *) calloc((size_t) array_size, sizeof(**ind));
	      assert(ind[column_nr] != NULL);

#ifdef DBUG_MEMORY
	      DBUG_PRINT("info", ("Dumping data[%u] (%p)", column_nr, data[column_nr]));
	      DBUG_DUMP("info", data[column_nr], (unsigned int)(array_size * sizeof(**data)));
	      DBUG_PRINT("info", ("Dumping ind[%u] (%p)", column_nr, ind[column_nr]));
	      DBUG_DUMP("info", ind[column_nr], (unsigned int)(array_size * sizeof(**ind)));
#endif

	      if ((status = sql_define_column(column_nr + 1,
					      column_type,
					      column_length,
					      array_size,
					      data[column_nr][0],
					      ind[column_nr])) != OK)
		break;
	    }

	  (void) printf("\n"); /* column heading end */

#ifdef DBUG_MEMORY
	  DBUG_PRINT("info", ("Dumping data (%p)", data));
	  DBUG_DUMP("info", data, (unsigned int)(column_count * sizeof(*data)));
	  DBUG_PRINT("info", ("Dumping ind (%p)", ind));
	  DBUG_DUMP("info", ind, (unsigned int)(column_count * sizeof(*ind)));
#endif
	  row_count = 0;

	  do
	    {
	      if ((status = sql_fetch_rows(array_size, &row_count)) != OK)
		{
		  break;
		}

	      DBUG_PRINT("info", ("rows fetched: %u", row_count));

	      for (array_nr = 0; array_nr < array_size && array_nr < row_count; array_nr++)
		{
		  DBUG_PRINT("info", ("array_nr: %u", array_nr));

		  for (column_nr = 0; column_nr < column_count; column_nr++)
		    {
#ifdef DBUG_MEMORY
		      if (array_nr == 0)
			{
			  DBUG_PRINT("info", ("Dumping data[%u] (%p)", column_nr, data[column_nr]));
			  DBUG_DUMP("info",
				    data[column_nr],
				    (unsigned int)(array_size * sizeof(**data)));
			  DBUG_PRINT("info", ("Dumping ind[%u] (%p)", column_nr, ind[column_nr]));
			  DBUG_DUMP("info",
				    ind[column_nr],
				    (unsigned int)(array_size * sizeof(**ind)));
			}

		      DBUG_PRINT("info",
				 ("Dumping data[%u][%u] (%p)",
				  column_nr,
				  array_nr,
				  data[column_nr][array_nr]));
		      DBUG_DUMP("info",
				data[column_nr][array_nr],
				(unsigned int) ((char*)data[column_nr][1] - (char*)data[column_nr][0]));
#endif

		      DBUG_PRINT("info", ("ind[%u][%u]= %d", column_nr, array_nr, ind[column_nr][array_nr]));
		      DBUG_PRINT("info", ("data[%u][%u]= %s", column_nr, array_nr, data[column_nr][array_nr]));

		      assert(data[column_nr] != NULL);
		      assert(ind[column_nr] != NULL);

		      printf("%s%s",
			     (column_nr > 0 ? "," : ""),
			     (ind[column_nr][array_nr] == -1 ? "NULL" : data[column_nr][array_nr]));
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
  if (bind_value.data != NULL)
    {
      for (bind_variable_nr = 0;
	   bind_variable_nr < bind_variable_count;
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
	   bind_variable_nr < bind_variable_count;
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

  if (data != NULL && ind != NULL)
    {
      for (column_nr = 0;
	   column_nr < column_count;
	   column_nr++)
	{
	  free(data[column_nr][0]);
	  free(data[column_nr]);
	  free(ind[column_nr]);
	}
      free(data);
      free(ind);
    }

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
    case STEP_OPEN_CURSOR:
      (void) sql_close_cursor();
      /*@fallthrough@*/
    case STEP_BIND_VARIABLE:
      /*@fallthrough@*/
    case STEP_PARSE:
      /*@fallthrough@*/
    case STEP_ALLOCATE_DESCRIPTORS:
      (void) sql_deallocate_descriptors();
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
