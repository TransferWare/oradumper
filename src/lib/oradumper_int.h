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
  ORA_CHARF = -96, /* char[n] */
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
  OPTION_B1,
  OPTION_B2,
  OPTION_B3,
  OPTION_B4,
  OPTION_B5,
  OPTION_B6,
  OPTION_B7,
  OPTION_B8,
  OPTION_B9,
  OPTION_B10
#define MAX_BIND_VARIABLES 10
} option_t;

typedef int error_t; /* sqlca.sqlcode */

#define OK 0

extern
void
process_options(const unsigned int length, const char **options);

extern
/*@null@*//*@observer@*/
const char *
get_option(const option_t option);

/* functions to be declared in the PRO*C source */
extern
error_t
sql_connect(const char *userid);

extern
error_t
sql_execute_immediate(const char *statement);

extern
error_t
sql_allocate_descriptors(const unsigned int max_array_size);

extern
error_t
sql_parse(const char *select_statement);

extern
error_t
sql_bind_variable_count(/*@out@*/ unsigned int *count);

extern
error_t
sql_bind_variable_name(const unsigned int nr, const size_t size, /*@out@*/ char *name);

extern
error_t
sql_bind_variable(const unsigned int nr, /*@null@*/ const char *value);

extern
error_t
sql_open_cursor(void);

extern
error_t
sql_column_count(/*@out@*/ unsigned int *count);

extern
error_t
sql_describe_column(const unsigned int nr,
		    const size_t size,
		    /*@out@*/ char *name,
		    /*@out@*/ int *type,
		    /*@out@*/ unsigned int *length);

extern
error_t
sql_define_column(const unsigned int nr,
		  const int type,
		  const unsigned int length,
		  const unsigned int array_size,
		  const char *data[],
		  const unsigned short ind[]);

extern
error_t
sql_fetch_rows();

extern
error_t
sql_close_cursor(void);

extern
error_t
sql_deallocate_descriptors(void);

#endif
