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
  OPTION_NLS_NUMERIC_CHARACTERS
} option_t;

extern
void
process_options(const unsigned int length, const char **options);

extern
/*@null@*//*@observer@*/
const char *
get_option(const option_t option);

#endif
