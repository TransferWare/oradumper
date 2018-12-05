#if HAVE_CONFIG_H
#include <config.h>
#endif

#if HAVE_CHECK_H
#include <check.h>
#endif

/* should always be there */
#include <stdio.h>

#if HAVE_SEARCH_H
#include <search.h>
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

#include "../src/lib/oradumper.h"
#include "../src/lib/oradumper_int.h"

#if defined(DBUG_OFF) && DBUG_OFF == 0
#undef  DBUG_OFF
#else
#define DBUG_OFF 1
#endif

#if HAVE_DBUG_H
#include <dbug.h>
#endif

/* include dmalloc as last one */
#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif

#ifndef FILENAME_MAX
#define FILENAME_MAX 1024
#endif

static char error_msg[1000+1] = "";
static unsigned int row_count;
static char *srcdir; /* environment variable set by automake */
static char output_ref[FILENAME_MAX];

static char *cmp_files(const char *file1, const char *file2)
{
  static char result[1000+1] = "";

  FILE *fin1 = fopen(file1, "r"), *fin2 = fopen(file2, "r");
  int ch1 = EOF, ch2 = EOF;
  int nr;

  if (fin1 == NULL)
    {
      (void) snprintf(result, sizeof(result), "File '%s' does not exist", file1);
    }
  else if (fin2 == NULL)
    {
      (void) snprintf(result, sizeof(result), "File '%s' does not exist", file2);
    }
  else
    {
      for (nr = 1; ; nr++)
        {
          ch1 = fgetc(fin1);
          ch2 = fgetc(fin2);

          if (ch1 == EOF || ch2 == EOF)
            {
              break;
            }
          else if (ch1 != ch2)
            {
              break;
            }
        }
    }
  
  if (fin1 != NULL)
    {
      (void) fclose(fin1);
    }
  if (fin2 != NULL)
    {
      (void) fclose(fin2);
    }

  if (!(ch1 == EOF && ch2 == EOF))
    {
      (void) snprintf(result, sizeof(result), "Files '%s' and '%s' differ at position %d", file1, file2, nr);
    }

  return result[0] == '\0' ? NULL : result;
}

#if defined(WITH_DMALLOC) && defined(HAVE_SEARCH_H)

static void *addr_list = NULL; /* a list of all addresses allocated currently */

static int addr_cmp(const void *addr1, const void *addr2)
{
  return (char *) addr1 - (char *) addr2;
}

static void dmalloc_track_func(const char *file,
                               const unsigned int line,
                               const int func_id,
                               const DMALLOC_SIZE byte_size,
                               const DMALLOC_SIZE alignment,
                               const DMALLOC_PNT old_addr,
                               const DMALLOC_PNT new_addr)
{
  char *this_file = __FILE__;

  if (file != NULL && strcmp(file, &this_file[strlen(this_file) - strlen(file)]) != 0)
    {
      dmalloc_message("checking file %s", file);

      switch(func_id)
        {
        case DMALLOC_FUNC_MALLOC:
        case DMALLOC_FUNC_CALLOC:
        case DMALLOC_FUNC_REALLOC:
        case DMALLOC_FUNC_RECALLOC:
        case DMALLOC_FUNC_MEMALIGN:
        case DMALLOC_FUNC_VALLOC:
        case DMALLOC_FUNC_STRDUP:
          if (old_addr != NULL && old_addr != new_addr) /* realloc */
            {
              dmalloc_message("deleting pointer %p", old_addr);
              (void) tdelete(old_addr, &addr_list, addr_cmp);
            }
          if (new_addr != NULL)
            {
              dmalloc_message("adding pointer %p", new_addr);
              (void) tsearch(new_addr, &addr_list, addr_cmp);
            }
          break;

        case DMALLOC_FUNC_FREE:
        case DMALLOC_FUNC_CFREE:
          if (old_addr != NULL)
            {
              dmalloc_message("deleting pointer %p", old_addr);
              (void) tdelete(old_addr, &addr_list, addr_cmp);
            }
          break;
        }
    }
}
#endif

START_TEST(test_sizes)
{
  DBUG_ENTER("test_sizes");
  fail_unless(sizeof(value_name_t) > 30, NULL);
  fail_unless(sizeof(character_set_name_t) > 20, NULL);
  DBUG_LEAVE();
}
END_TEST

START_TEST(test_usage)
{
  const char output[] = "usage.txt";
  FILE *fout = fopen(output, "w");
  char *error;
  const char *options[] = { "any" };

  DBUG_ENTER("test_usage");

  (void) sprintf(output_ref, "%s/%s.ref", srcdir, output);

  fail_if(fout == NULL, "File '%s' could not be opened for writing: %s", output, strerror(errno));

  oradumper_usage(fout);

  fail_if(fclose(fout) != 0, "File '%s' could not be closed: %s", output, strerror(errno));
  error = cmp_files(output, output_ref);
  fail_if(error != NULL, error);

  /* should display usage but no error */
  error = oradumper(0, options, 0, sizeof(error_msg), error_msg, &row_count);

  fail_if(NULL != error, NULL);

  /* should display error */
  error = oradumper(1, options, 0, sizeof(error_msg), error_msg, &row_count);

  fail_if(NULL == error, NULL);

  DBUG_LEAVE();
}
END_TEST

START_TEST(test_output_file)
{
  const char *options[] = { "query=select * from dual", "output_file=/", "feedback=0" };

  DBUG_ENTER("test_output_file");

  /* should display usage but no error */
  fail_if(NULL == oradumper(sizeof(options)/sizeof(options[0]), options, 0, sizeof(error_msg), error_msg, &row_count), NULL);

  DBUG_LEAVE();
}
END_TEST

START_TEST(test_enclosure_string)
{
  char userid[100+1] = "userid=";
  char enclosure_string[100+1];
  int d1, d2, d3;
  const char *options[] = {
    "query=select * from dual where 0=1",
    enclosure_string,
    "column_heading=0",
    "output_file=test_enclosure_string",
    "feedback=0",
    userid
  };
  char *error;
  int cond;

  DBUG_ENTER("test_enclosure_string");

  fail_if(getenv("USERID") == NULL, "Environment variable USERID should be set");

  (void) strncat(userid, getenv("USERID"), sizeof(userid) - strlen(userid));

  /* test \a \b \f \n \r \t \v \\ (ok) and \c (wrong) */
  for (d1 = 0; d1 < 128; d1++)
    {
      switch ((char) d1)
        {
        case 'a':
        case 'b':
        case 'f':
        case 'n':
        case 'r':
        case 't':
        case 'v':
        case '\\': /* OK */
        case 'c': /* FAIL */
          sprintf(enclosure_string, "enclosure_string=\\%ca", d1);
          error = oradumper(sizeof(options)/sizeof(options[0]), options, 0, sizeof(error_msg), error_msg, &row_count);
          strcpy(userid, ""); /* no reconnect */
          cond = ((error != NULL) == (d1 == 'c'));
          DBUG_PRINT("info", ("error: %p; d1: %c; cond: %d", error, d1, cond));
          fail_unless(cond, "1 escape character");
          break;

        default:
          break;
        }
    }

  /* test hexadecimal strings */
  for (d1 = 0; d1 < 128; d1++) /* test hexadecimal boundaries: '\0' (fail), 0, 9, a, f, g (fail) */
    {
      switch ((char) d1)
        {
        case '\0':
          sprintf(enclosure_string, "enclosure_string=\\x10\\x");
          fail_if(NULL == oradumper(sizeof(options)/sizeof(options[0]), options, 0, sizeof(error_msg), error_msg, &row_count), "invalid hexadecimal string");
          break;

        case '0':
        case '9':
        case 'a':
        case 'A':
        case 'f':
        case 'F':
        case 'g':
        case 'G':
          for (d2 = 0; d2 < 128; d2++)
            {
              switch ((char) d2)
                {
                case '\0':
                  sprintf(enclosure_string, "enclosure_string=\\x%c\\x10", d1);
                  error = oradumper(sizeof(options)/sizeof(options[0]), options, 0, sizeof(error_msg), error_msg, &row_count);
                  /* if there is no error d1 must not be g or G */
                  cond = ((error != NULL) == (d1 == 'g' || d1 == 'G'));
                  DBUG_PRINT("info", ("error: %p; d1: %c; cond: %d", error, d1, cond));
                  fail_unless(cond, "1 hexadecimal digit");
                  break;

                case '0':
                case '9':
                case 'a':
                case 'A':
                case 'f':
                case 'F':
                case 'g':
                case 'G':
                  sprintf(enclosure_string, "enclosure_string=\\x%c%c", d1, d2);
                  error = oradumper(sizeof(options)/sizeof(options[0]), options, 0, sizeof(error_msg), error_msg, &row_count);
                  /* \x9G is converted to 9G */
                  cond = ((error != NULL) == (d1 == 'g' || d1 == 'G'));
                  DBUG_PRINT("info", ("error: %p; d1: %c; d2: %c; cond: %d", error, d1, d2, cond));
                  /* if there is no error d1 and d2 must not be g or G */
                  fail_unless(cond, "2 hexadecimal digits");
                  break;

                default:
                  break;
                }
            }
          break;

        default:
          break;
        }
    }

  /* test octal strings */
  for (d1 = 0; d1 < 128; d1++) /* test hexadecimal boundaries: 0, 7, 8 */
    {
      switch ((char) d1)
        {
        case '0':
        case '7': /* OK */
        case '8': /* FAIL */
          sprintf(enclosure_string, "enclosure_string=\\%ca", d1);
          error = oradumper(sizeof(options)/sizeof(options[0]), options, 0, sizeof(error_msg), error_msg, &row_count);
          cond = ((error != NULL) == (d1 == '8'));
          DBUG_PRINT("info", ("error: %p; d1: %c; cond: %d", error, d1, cond));
          fail_unless(cond, "1 octal digit");

          for (d2 = 0; d2 < 128; d2++)
            {
              switch ((char) d2)
                {
                case '0':
                case '7': /* OK */
                case '8': /* FAIL */
                  sprintf(enclosure_string, "enclosure_string=\\%c%ca", d1, d2);
                  error = oradumper(sizeof(options)/sizeof(options[0]), options, 0, sizeof(error_msg), error_msg, &row_count);
                  /* \78 is converted to 78 */
                  cond = ((error != NULL) == (d1 == '8'));
                  DBUG_PRINT("info", ("error: %p; d1: %c; d2: %c; cond: %d", error, d1, d2, cond));
                  fail_unless(cond, "2 octal digits");

                  for (d3 = 0; d3 < 128; d3++)
                    {
                      switch ((char) d3)
                        {
                        case '0':
                        case '7': /* OK */
                        case '8': /* FAIL */
                          sprintf(enclosure_string, "enclosure_string=\\%c%c%c", d1, d2, d3);
                          error = oradumper(sizeof(options)/sizeof(options[0]), options, 0, sizeof(error_msg), error_msg, &row_count);
                          /* \078 is converted to 78 */
                          cond = ((error != NULL) == (d1 == '8'));
                          DBUG_PRINT("info", ("error: %p; d1: %c; d2: %c; d3: %c; cond: %d", error, d1, d2, d3, cond));
                          fail_unless(cond, "3 octal digits");
                          break;

                        default:
                          break;
                        }
                    }
                  break;

                default:
                  break;
                }
            }
          break;

        default:
          break;
        }
    }

  DBUG_LEAVE();
}
END_TEST

START_TEST(test_query_sql_error)
{
  char userid[100+1] = "userid=";
  const char *options[] = {
    userid,
    "feedback=0",
    "query=select "
  };
  char *error;

  DBUG_ENTER("test_query_sql_error");

  fail_if(getenv("USERID") == NULL, "Environment variable USERID should be set");

  (void) strncat(userid, getenv("USERID"), sizeof(userid) - strlen(userid));

  fail_unless(NULL != oradumper(sizeof(options)/sizeof(options[0]), options, 1, sizeof(error_msg), error_msg, &row_count), error_msg);

  DBUG_LEAVE();
}
END_TEST

START_TEST(test_query_data_types)
{
  const char output[] = "query_data_types.lis";
  char userid[100+1] = "userid=";
  char output_file[100+1] = "output_file=";
  char output_append[] = "output_append=0";
  char query[1000+1];
  const char *options[] = {
    "fetch_size=1",
    "nls_date_format=yyyy-mm-dd hh24:mi:ss",
    "nls_timestamp_format=yyyy-mm-dd hh24:mi:ss.ff",
    "nls_timestamp_tz_format=yyyy-mm-dd hh24:mi:ss.ff tzh:tzm",
    "nls_numeric_characters=.,",
    query,
    output_file,
    "feedback=1",
    "fixed_column_length=0",
    output_append,
    userid /* userid last so the number of arguments can be decremented forcing no new connect */
  };
  char *error;
  char *columns[] = {
    "\"rowid\"",
    "\"char\"",
    "\"date\"",
    "\"float\"",
    "\"number\"",
    "\"timestamp(3)\"",
    "\"timestamp(6)\"",
    "\"timestamp(6) with time zone\"",
    "\"varchar2\"",
    "\"clob\"",
    "\"raw\"",
    "\"interval day(3) to second(0)\"",
    "\"interval day(3) to second(2)\"",
    "\"interval day(5) to second(1)\"",
    "\"interval day(9) to second(6)\"",
    "\"nchar\"",
    "\"nclob\"",
    "\"nvarchar2\""
  };
  int nr;

  DBUG_ENTER("test_query_data_types");

#if defined(WITH_DMALLOC) && defined(HAVE_SEARCH_H)
  dmalloc_track(dmalloc_track_func);
#endif

  (void) sprintf(output_ref, "%s/%s.ref", srcdir, output);

  DBUG_PRINT("info", ("output_ref: %s", output_ref));

  DBUG_PRINT("info", ("USERID: %p", getenv("USERID")));

  if (getenv("USERID") == NULL)
    {
      fail("Environment variable USERID should be set");
    }

  (void) strncat(userid, getenv("USERID"), sizeof(userid) - strlen(userid));
  (void) strncat(output_file, output, sizeof(output_file) - strlen(output_file));

  DBUG_PRINT("info", ("output_file: %s", output_file));

  for (nr = 0; nr < sizeof(columns)/sizeof(columns[0]); nr++)
    {
      (void) sprintf(query, "query=select %s from oradumper_test", columns[nr]);

      {
        int i;
        const int nr_options = sizeof(options)/sizeof(options[0]) - (nr > 0 ? 1 : 0);

        DBUG_PRINT("info", ("nr_options: %d", nr_options));

        for (i = 0; i < nr_options; i++)
          {
            DBUG_PRINT("info", ("options[%d]: %s", i, options[i]));
          }
      }

      error = oradumper(sizeof(options)/sizeof(options[0]) - (nr > 0 ? 1 : 0), /* supply userid first time */
                        options,
                        0,
                        sizeof(error_msg),
                        error_msg,
                        &row_count);

      DBUG_PRINT("info", ("error: %s", (error != NULL ? error : "none")));

      fail_unless(NULL == error, error_msg);

      (void) strcpy(output_append, "output_append=1");
    }

  (void) strcpy(query, "query=select owner,view_name, text from ALL_VIEWS where view_name='ALL_VIEWS'");

  /* final call */
  fail_unless(NULL == oradumper(sizeof(options)/sizeof(options[0]) - 1, /* do not supply userid */
                                options,
                                1, /* disconnect */
                                sizeof(error_msg),
                                error_msg,
                                &row_count), error_msg);

#if defined(WITH_DMALLOC) && defined(HAVE_SEARCH_H)
  fail_if(addr_list != NULL);
  dmalloc_track(NULL);
#endif

  error = cmp_files(output, output_ref);
  fail_if(error != NULL, error);

  DBUG_LEAVE();
}
END_TEST

START_TEST(test_query1)
{
  const char output[] = "query1.lis";
  char fetch_size[100+1] = "fetch_size=1";
  char last_option[100+1] = "userid=";
  char output_file[100+1] = "output_file=";
  const char *options[] = {
    fetch_size,
    "null=NULL",
    "nls_lang=AMERICAN",
    "nls_date_format=yyyy-mm-dd hh24:mi:ss",
    "nls_timestamp_format=yyyy-mm-dd hh24:mi:ss",
    "nls_numeric_characters=.,",
    /*    
    "query=\
select 1234567890 as NR, unistr('\"my,string\"') as STR, to_date('1900-12-31 23:23:59') as DAY from dual \
union all \
select 2345678901, unistr('YOURSTRING'), to_date('20001231232359', 'yyyymmddhh24miss') from dual where :x is null",
    */
    "query=\
select 1234567890 as NR, '\"my,string\"' as STR, to_date('1900-12-31 23:23:59') as DAY from dual \
union all \
select 2345678901, 'YOURSTRING', to_date('20001231232359', 'yyyymmddhh24miss') from dual where :x is null",
    output_file,
    "fixed_column_length=0",
    "column_separator=,",
    "feedback=0",
    "enclosure_string=\\x22", /* " */
    last_option
  };
  char *error;

  DBUG_ENTER("test_query1");

  (void) sprintf(output_ref, "%s/%s.ref", srcdir, output);

  fail_if(getenv("USERID") == NULL, "Environment variable USERID should be set");

  (void) strncat(last_option, getenv("USERID"), sizeof(last_option) - strlen(last_option));
  (void) strncat(output_file, output, sizeof(output_file) - strlen(output_file));

  fail_unless(NULL == oradumper(sizeof(options)/sizeof(options[0]), options, 0, sizeof(error_msg), error_msg, &row_count), error_msg);
  /* skip connect, but append */
  strcpy(last_option, "output_append=1");
  strcpy(fetch_size, "fetch_size=10");
  fail_unless(NULL == oradumper(sizeof(options)/sizeof(options[0]), options, 0, sizeof(error_msg), error_msg, &row_count), error_msg);
  /* disconnect */
  strcpy(fetch_size, "fetch_size=100");
  fail_unless(NULL == oradumper(sizeof(options)/sizeof(options[0]), options, 1, sizeof(error_msg), error_msg, &row_count), error_msg);

  error = cmp_files(output, output_ref);
  fail_if(error != NULL, error);

  DBUG_LEAVE();
}
END_TEST

START_TEST(test_query2)
{
  const char output[] = "query2.lis";
  char userid[100+1] = "userid=";
  char output_file[100+1] = "output_file=";
  const char *options[] = {
    userid,
    "fetch_size=2",
    "nls_date_format=yyyy-mm-dd hh24:mi:ss",
    "nls_timestamp_format=yyyy-mm-dd hh24:mi:ss",
    "nls_numeric_characters=.,",
    "feedback=0",
    "query=\
select to_clob(rpad('0123456789', 8000, '0123456789')) as myclob from dual",
    output_file,
    "fixed_column_length=0"
  };
  char *error;

  DBUG_ENTER("test_query2");

  (void) sprintf(output_ref, "%s/%s.ref", srcdir, output);

  fail_if(getenv("USERID") == NULL, "Environment variable USERID should be set");

  (void) strncat(userid, getenv("USERID"), sizeof(userid) - strlen(userid));
  (void) strncat(output_file, output, sizeof(output_file) - strlen(output_file));

  fail_unless(NULL == oradumper(sizeof(options)/sizeof(options[0]), options, 1, sizeof(error_msg), error_msg, &row_count), error_msg);

  error = cmp_files(output, output_ref);
  fail_if(error != NULL, error);

  DBUG_LEAVE();
}
END_TEST

START_TEST(test_query3)
{
  const char output[] = "query3.lis";
  char userid[100+1] = "userid=";
  char output_file[100+1] = "output_file=";
  const char *options[] = {
    userid,
    "fetch_size=3",
    "nls_date_format=yyyy-mm-dd hh24:mi:ss",
    "nls_timestamp_format=yyyy-mm-dd hh24:mi:ss",
    "nls_numeric_characters=.,",
    "feedback=1", /* to print the dot after each fetch */
    "query=\
select object_name, object_type \
from all_objects \
where owner = 'SYS' \
and object_name like 'UTL\\_%' escape '\\' \
and object_type='PACKAGE' \
and object_name in ('UTL_COLL','UTL_COMPRESS','UTL_ENCODE','UTL_FILE','UTL_GDK','UTL_HTTP','UTL_I18N','UTL_INADDR','UTL_LMS') \
and rownum <= :b1 \
order by object_name",
    output_file,
    "fixed_column_length=0",
    "10" /* bind value */
  };
  char *error;

  DBUG_ENTER("test_query3");

  (void) sprintf(output_ref, "%s/%s.ref", srcdir, output);

  fail_if(getenv("USERID") == NULL, "Environment variable USERID should be set");

  (void) strncat(userid, getenv("USERID"), sizeof(userid) - strlen(userid));
  (void) strncat(output_file, output, sizeof(output_file) - strlen(output_file));

  fail_unless(NULL == oradumper(sizeof(options)/sizeof(options[0]), options, 1, sizeof(error_msg), error_msg, &row_count), error_msg);

  error = cmp_files(output, output_ref);
  fail_if(error != NULL, error);

  DBUG_LEAVE();
}
END_TEST

START_TEST(test_query4)
{
  const char output[] = "query4.lis";
  char userid[100+1] = "userid=";
  char output_file[100+1] = "output_file=";
  const char *options[] = {
    userid,
    "fetch_size=1", /* 100 */
    "feedback=0",
    "nls_lang=.utf8",
    "nls_date_format=yyyy-mm-dd hh24:mi:ss",
    "nls_timestamp_format=yyyy-mm-dd hh24:mi:ss",
    "nls_numeric_characters=.,",
    "query=\
select cast(1234567890 as number(10, 0)) as NR, unistr('my,string') as STR, to_date('1900-12-31 23:23:59') as DAY from dual \
union \
select 2345678901, unistr('YOURSTRING'), to_date('20001231232359', 'yyyymmddhh24miss') from dual \
union all \
select 12, unistr('abc\\00e5\\00f1\\00f6\\0142'), null from dual",
    output_file,
    "fixed_column_length=1",
    "column_separator=\\040" /* space */
  };
  char *error;

  DBUG_ENTER("test_query4");

  (void) sprintf(output_ref, "%s/%s.ref", srcdir, output);

  fail_if(getenv("USERID") == NULL, "Environment variable USERID should be set");

  (void) strncat(userid, getenv("USERID"), sizeof(userid) - strlen(userid));
  (void) strncat(output_file, output, sizeof(output_file) - strlen(output_file));

  fail_unless(NULL == oradumper(sizeof(options)/sizeof(options[0]), options, 1, sizeof(error_msg), error_msg, &row_count), error_msg);

  error = cmp_files(output, output_ref);
  fail_if(error != NULL, error);

  DBUG_LEAVE();
}
END_TEST

START_TEST(test_query5)
{
  /* Test that fixed column length output has the following properties:
     column display size is maximum of data size, column heading (if specified) and length of null display (if specified)
  */
  const char output[] = "query5.lis";
  char userid[100+1] = "userid=";
  char output_file[100+1] = "output_file=";
  const char *options[] = {
    userid,
    "feedback=0",
    "query=\
select  cast('query5' as varchar2(30)) as name\
,       cast(1 as number(10)) as default_retention_period\
,       cast(null as varchar2(1)) as nl\
        from dual",
    output_file,
    "fixed_column_length=1",
    "column_heading=0",
    "column_separator=\\040\\040",
    "null=null"
  };
  char *error;

  DBUG_ENTER("test_query5");

  (void) sprintf(output_ref, "%s/%s.ref", srcdir, output);

  fail_if(getenv("USERID") == NULL, "Environment variable USERID should be set");

  (void) strncat(userid, getenv("USERID"), sizeof(userid) - strlen(userid));
  (void) strncat(output_file, output, sizeof(output_file) - strlen(output_file));

  fail_unless(NULL == oradumper(sizeof(options)/sizeof(options[0]), options, 1, sizeof(error_msg), error_msg, &row_count), error_msg);

  error = cmp_files(output, output_ref);
  fail_if(error != NULL, error);

  DBUG_LEAVE();
}
END_TEST

START_TEST(test_query6)
{
  /* Test that fixed column length output has the following properties:
     column display size is maximum of data size, column heading (if specified) and length of null display (if specified).
  */
  const char output[] = "query6.lis";
  char userid[100+1] = "userid=";
  char output_file[100+1] = "output_file=";
  const char *options[] = {
    userid,
    "feedback=0",
    "query=\
select  cast('query6' as varchar2(30)) as name\
,       cast(1 as number(10)) as default_retention_period\
,       cast(null as varchar2(1)) as nl\
        from dual",
    output_file,
    "fixed_column_length=1",
    "column_heading=1",
    "column_separator=\\040\\040"
  };
  char *error;

  DBUG_ENTER("test_query6");

  (void) sprintf(output_ref, "%s/%s.ref", srcdir, output);

  fail_if(getenv("USERID") == NULL, "Environment variable USERID should be set");

  (void) strncat(userid, getenv("USERID"), sizeof(userid) - strlen(userid));
  (void) strncat(output_file, output, sizeof(output_file) - strlen(output_file));

  fail_unless(NULL == oradumper(sizeof(options)/sizeof(options[0]), options, 1, sizeof(error_msg), error_msg, &row_count), error_msg);

  error = cmp_files(output, output_ref);
  fail_if(error != NULL, error);

  DBUG_LEAVE();
}
END_TEST

START_TEST(test_query7)
{
  /* Test that fixed column length output has the following properties:
     column display size is maximum of data size, column heading (if specified) and length of null display (if specified).
  */
  const char output[] = "query7.lis";
  char userid[100+1] = "userid=";
  char output_file[100+1] = "output_file=";
  const char *options[] = {
    userid,
    "feedback=0",
    "query=\
select  '\"' as enclosure_string\
,       ';' as column_separator\
        from dual",
    output_file,
    "fixed_column_length=0",
    "column_heading=0",
    "column_separator=;",
    "enclosure_string=\""
  };
  char *error;

  DBUG_ENTER("test_query7");

  (void) sprintf(output_ref, "%s/%s.ref", srcdir, output);

  fail_if(getenv("USERID") == NULL, "Environment variable USERID should be set");

  (void) strncat(userid, getenv("USERID"), sizeof(userid) - strlen(userid));
  (void) strncat(output_file, output, sizeof(output_file) - strlen(output_file));

  fail_unless(NULL == oradumper(sizeof(options)/sizeof(options[0]), options, 1, sizeof(error_msg), error_msg, &row_count), error_msg);

  error = cmp_files(output, output_ref);
  fail_if(error != NULL, error);

  DBUG_LEAVE();
}
END_TEST

Suite *
options_suite(void)
{
  Suite *s = suite_create("General");

  TCase *tc_internal = tcase_create("Internal");
  TCase *tc_options = tcase_create("Options");
  TCase *tc_query = tcase_create("query");

  tcase_add_test(tc_internal, test_sizes);
  tcase_add_test(tc_internal, test_usage);
  suite_add_tcase(s, tc_internal);

  tcase_set_timeout(tc_options, 60);
  tcase_add_test(tc_internal, test_output_file);
  tcase_add_test(tc_options, test_enclosure_string);
  suite_add_tcase(s, tc_options);

  tcase_set_timeout(tc_query, 60);
  tcase_add_test(tc_query, test_query_sql_error);
  tcase_add_test(tc_query, test_query_data_types);
  tcase_add_test(tc_query, test_query1);
  tcase_add_test(tc_query, test_query2);
  tcase_add_test(tc_query, test_query3);
  tcase_add_test(tc_query, test_query4);
  tcase_add_test(tc_query, test_query5);
  tcase_add_test(tc_query, test_query6);
  tcase_add_test(tc_query, test_query7);
  suite_add_tcase(s, tc_query);

  return s;
}

int
main(void)
{
  int number_failed;
  Suite *s = options_suite();
  SRunner *sr = srunner_create(s);

  DBUG_INIT("d,g,t,o=dbug.log", "oradumper_check");  
  DBUG_ENTER("main");
  
  srcdir = (getenv("srcdir") != NULL ? getenv("srcdir") : ".");

  srunner_run_all(sr, CK_ENV); /* Use environment variable CK_VERBOSITY, which can have the values "silent", "minimal", "normal", "verbose" */
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);

  DBUG_LEAVE();
  DBUG_DONE();

  return (number_failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}
