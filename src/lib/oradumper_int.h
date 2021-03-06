#ifndef ORADUMPER_INT_H
#define ORADUMPER_INT_H

#if HAVE_STDBOOL_H
#include <stdbool.h>
#else
typedef int bool;
#define false 0
#define true 1
#endif

#ifdef HAVE_OCI_H
#include <oci.h>
#endif

/*#if HAVE_DBUG_H*/
#include <dbug.h>
/*#endif*/

typedef enum {
  ANSI_CHARACTER = 1,
  ANSI_CHARACTER_VARYING = 12, /* VARCHAR2, NVARCHAR */
  ANSI_DATE = 9, /* DATE (char 7), TIMESTAMP (char 11), TIMESTAMP WITH TIME ZONE (char 11) */
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
  ORA_RAW = -23, /* RAW: unsigned char[n] */
  ORA_LONG_RAW = -24, /* unsigned char[n] */
  ORA_UNSIGNED = -68, /* unsigned int */
  ORA_DISPLAY = -91, /* char[n] */
  ORA_LONG_VARCHAR = -94, /* char[n+4] */
  ORA_LONG_VARRAW = -95, /* unsigned char[n+4] */
  ORA_CHAR = -96, /* char[n] */
  ORA_CHARZ = -97, /* char[n+1] */
  ORA_UROWID = -104, /* UROWID: char[4] */
  ORA_CLOB = -112, /* CLOB, NCLOB */
  ORA_BLOB = -113, /* BLOB */
  ORA_INTERVAL = 10 /* unsigned char[11] */
} orasql_datatype_t;

typedef int error_t; /* sqlca.sqlcode */

/* zero terminated character arrays */
typedef char character_set_name_t[20 + 1];
typedef char value_name_t[30 + 1];
typedef unsigned int orasql_size_t;

typedef struct {
  /* Length of column to display */
  orasql_size_t display_length;
  bool is_numeric;
  /* the following fields are returned by exec sql get descriptor */
  value_name_t name;
  orasql_datatype_t type;
  orasql_datatype_t type_orig; /* type is converted to a string type so this will store the original type */
  orasql_size_t octet_length; /* length in bytes */
  orasql_size_t length; /* length in characters for NCHAR; in bytes otherwise */
  int precision;
  int scale;
  character_set_name_t character_set_name;
  /* Oracle extensions */
  unsigned int national_character;
  orasql_size_t internal_length;
} value_description_t;

/* zero terminated character array */
typedef char charz_1_t[1];
/* character array with a length indicator */
typedef struct { unsigned short len; unsigned char arr[1]; } varchar_1_t;

typedef /*@null@*/ /*@only@*/ unsigned char *byte_ptr_t;

typedef /*@observer@*/ unsigned char *value_data_t;

typedef /*@null@*/ /*@only@*/ value_data_t *value_data_ptr_t;
typedef /*@null@*/ /*@only@*/ short *short_ptr_t;
typedef /*@null@*/ /*@only@*/ orasql_size_t *orasql_size_ptr_t;

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
  /*@null@*/ /*@only@*/ orasql_size_t *size;

  /*
        NR STR        DAY
---------- ---------- --------
     78901 YOURSTRING 31-12-00
1234567890 my,string  31-12-00
  */

  /*@null@*/ /*@only@*/ char *align; /* values 'R' and 'L': a numeric fixed column aligns at the right */

  /* buffer array: buf[value_count] is the buffer for data[value_count][array_count] */
  /*@null@*/ /*@only@*/ byte_ptr_t *buf;

  /* data array: data[value_count][array_count] */
  /* data[value_count][array_count] may point to somewhere in buffer buf[value_count] */
  /*@null@*/ /*@only@*/ value_data_ptr_t *data;

  /* indicator array: ind[value_count][array_count] */
  /*@null@*/ /*@only@*/ short_ptr_t *ind;
  
  /* returned length array: returned_length[value_count][array_count] */
  /*@null@*/ /*@only@*/ orasql_size_ptr_t *returned_length;
} value_info_t;

#define OK 0

/* functions which need to be tested only are defined in the internal oradumper source */

/*@-exportlocal@*/
extern
void
print_value_description(value_description_t *value_description);

/*@-exportlocal@*/
extern
void
oradumper_usage(FILE *fout);
/*@=exportlocal@*/

/* functions defined in the PRO*C source */
extern
error_t
orasql_connect(const char *userid);

#ifdef HAVE_OCI_H
extern
error_t
orasql_register_connect(OCIExtProcContext *ctx);
#endif

extern
error_t
orasql_connected(void);

extern
error_t
orasql_cache_free_all(void);

extern
error_t
orasql_disconnect(void);

extern
error_t
orasql_execute_immediate(const char *statement);

extern
error_t
orasql_allocate_descriptor(const char *descriptor_name, const orasql_size_t max_array_size);

extern
error_t
orasql_parse(const char *select_statement);

extern
error_t
orasql_describe_input(const char *descriptor_name);

extern
error_t
orasql_value_count(const char *descriptor_name, /*@out@*/ unsigned int *count);

extern
error_t
orasql_value_get(const char *descriptor_name,
                 const unsigned int nr,
                 /*@out@*/ value_description_t *value_description);

extern
error_t
orasql_value_set(const char *descriptor_name,
                 const unsigned int nr,
                 const unsigned int array_size,
                 /*@in@*/ value_description_t *value_description,
                 const char *data,
                 const short *ind,
                 const orasql_size_t *returned_length);

extern
error_t
orasql_open_cursor(const char *descriptor_name);

extern
error_t
orasql_describe_output(const char *descriptor_name);

extern
error_t
orasql_fetch_rows(const char *descriptor_name,
                  const unsigned int array_size,
                  /*@out@*/ unsigned int *count);

extern
error_t
orasql_rows_processed(/*out@*/ unsigned int *count);

extern
error_t
orasql_close_cursor(void);

extern
error_t
orasql_deallocate_descriptor(const char *descriptor_name);

extern
void
orasql_error(/*@out@*/ orasql_size_t *length, /*@out@*/ char **msg);

#endif
