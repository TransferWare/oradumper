#ifndef ORADUMPER_INT_H
#define ORADUMPER_INT_H

typedef enum {
  ANSI_CHARACTER = 1,
  ANSI_CHARACTER_VARYING = 12,
  ANSI_DATE = 9,
  ANSI_DECIMAL = 3,
  ANSI_DOUBLE_PRECISION = 8,
  ANSI_FLOAT = 6,
  ANSI_INTEGER = 4,
  ANSI_NUMERIC = 2,
  ANSI_REAL = 7,
  ANSI_SMALLINT = 5,
  ORA_VARCHAR2 = -1, /* char[n] */
  ORA_NUMBER = -2, /* char[n] ( n <= 22) */
  ORA_INTEGER = -3, /* int */
  ORA_FLOAT = -4, /* float */
  ORA_STRING = -5, /* char[n+1] */
  ORA_VARNUM = -6, /* char[n] (n <= 22) */
  ORA_DECIMAL = -7, /* float */
  ORA_LONG = -8, /* char[n] */
  ORA_VARCHAR = -9, /* char[n+2] */
  ORA_ROWID = -11, /* char[n] */
  ORA_DATE = -12, /* char[n] */
  ORA_VARRAW = -15, /* char[n] */
  ORA_RAW = -23, /* unsigned char[n] */
  ORA_LONG_RAW = -24, /* unsigned char[n] */
  ORA_UNSIGNED = -68, /* unsigned int */
  ORA_DISPLAY = -91, /* char[n] */
  ORA_LONG_VARCHAR = -94, /* char[n+4] */
  ORA_LONG_VARRAW = -95, /* unsigned char[n+4] */
  ORA_CHAR = -96, /* char[n] */
  ORA_CHARZ = -97, /* char[n+1] */
} sql_datatype_t;

typedef enum {
  OPTION_USERID = 0,
  OPTION_SQLSTMT,
  OPTION_ARRAYSIZE,
  OPTION_DBUG_OPTIONS,
  OPTION_NLS_DATE_FORMAT,
  OPTION_NLS_TIMESTAMP_FORMAT,
  OPTION_NLS_NUMERIC_CHARACTERS,
  OPTION_DETAILS,
} option_t;

typedef int error_t; /* sqlca.sqlcode */

/* zero terminated character arrays */
typedef char character_set_name_t[20 + 1];
typedef char value_name_t[30 + 1];
typedef unsigned int sql_size_t;

typedef struct {
  /* the following fields are returned by exec sql get descriptor */
  value_name_t name;
  sql_datatype_t type;
  sql_size_t octet_length; /* length in bytes */
  sql_size_t length; /* length in characters */
  int precision;
  int scale;
  character_set_name_t character_set_name;
} value_description_t;

/* zero terminated character array */
typedef char charz_1_t[1];
/* character array with a length indicator */
typedef struct { unsigned short len; unsigned char arr[1]; } varchar_1_t;

typedef union {
  /*@observer@*/ char *charz_ptr;
  /*@only@*/ char *charz_alloc_ptr;
  charz_1_t charz;
  varchar_1_t varchar;
} value_data_t;

typedef /*@null@*/ /*@only@*/ value_data_t *value_data_ptr_t;
typedef /*@null@*/ /*@only@*/ short *short_ptr_t;

/* a structure which contains info about (arrays of) input and output values */
typedef struct {
  unsigned int value_count; /* number of values (bind variables or columns) */
  unsigned int array_count; /* each value is actually an array of this size */
  value_name_t descriptor_name;
  /* descr[value_count] */
  /*@null@*/ /*@only@*/ value_description_t *descr;

  /* this is the allocated size of value[x].data[y] */
  /* must be a multiple of 4 */
  /* size[value_count] */
  /*@null@*/ /*@only@*/ sql_size_t *size;

  /* data array: data[value_count][array_count] */
  /*@null@*/ /*@only@*/ value_data_ptr_t *data;

  /* indicator array: ind[value_count][array_count] */
  /*@null@*/ /*@only@*/ short_ptr_t *ind;
} value_info_t;

#define OK 0

/*@-exportlocal@*/
extern
unsigned int
process_arguments(const unsigned int nr_arguments, const char **arguments);

extern
/*@null@*//*@observer@*/
const char *
get_option(const option_t option);
/*@=exportlocal@*/

/* functions to be declared in the PRO*C source */
extern
error_t
sql_connect(const char *userid);

extern
error_t
sql_execute_immediate(const char *statement);

extern
error_t
sql_allocate_descriptor(const char *descriptor_name, const sql_size_t max_array_size);

extern
error_t
sql_parse(const char *select_statement);

extern
error_t
sql_describe_input(const char *descriptor_name);

extern
error_t
sql_value_count(const char *descriptor_name, /*@out@*/ unsigned int *count);

extern
error_t
sql_value_get(const char *descriptor_name, const unsigned int nr, /*@out@*/ value_description_t *value_description);

extern
error_t
sql_value_set(const char *descriptor_name, const unsigned int nr, const unsigned int array_size, /*@in@*/ value_description_t *value_description, const char *data, const short *ind);

extern
error_t
sql_open_cursor(const char *descriptor_name);

extern
error_t
sql_describe_output(const char *descriptor_name);

extern
error_t
sql_fetch_rows(const char *descriptor_name, const unsigned int array_size, /*@out@*/ unsigned int *count);

extern
error_t
sql_rows_processed(/*out@*/ unsigned int *count);

extern
error_t
sql_close_cursor(void);

extern
error_t
sql_deallocate_descriptor(const char *descriptor_name);

extern
void
sql_error(/*@out@*/ sql_size_t *length, /*@out@*/ char **msg);

#endif
