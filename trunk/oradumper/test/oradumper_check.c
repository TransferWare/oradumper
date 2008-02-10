#if HAVE_CONFIG_H
#include <config.h>
#endif

#if HAVE_CHECK_H
#include <check.h>
#endif

#if HAVE_STDBOOL_H
#include <stdbool.h>
#else
typedef int bool;
#define false 0
#define true 1
#endif

/* should always be there */
#include <stdio.h>

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

/* include dmalloc as last one */
#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif

static char dbug_options[1000+1] = "dbug_options=";
static char error_msg[1000+1] = "";

static char *cmp_files(const char *file1, const char *file2)
{
  static char result[1000+1] = "";

  FILE *fin1 = NULL, *fin2 = NULL;
  int ch1 = EOF, ch2 = EOF;
  int nr;

  if ((fin1 = fopen(file1, "r")) != NULL &&
      (fin2 = fopen(file2, "r")) != NULL)
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
      return result;
    }
  else
    {
      return NULL;
    }
}

START_TEST(test_sizes)
{
  fail_unless(sizeof(value_name_t) > 30, NULL);
  fail_unless(sizeof(character_set_name_t) > 20, NULL);
}
END_TEST

START_TEST(test_usage)
{
  const char usage_txt[] = "usage.txt";
  const char usage_txt_ref[] = "usage.txt.ref";
  FILE *fout = fopen(usage_txt, "w");
  char *error;

  fail_if(fout == NULL, "File '%s' could not be opened for writing: %s", usage_txt, strerror(errno));

  oradumper_usage(fout);

  fail_if(fclose(fout) != 0, "File '%s' could not be closed: %s", usage_txt, strerror(errno));
  error = cmp_files(usage_txt, usage_txt_ref);
  fail_if(error != NULL, error);
}
END_TEST

START_TEST(test_query1)
{
  const char query1_lis_ref[] = "query1.lis.ref";
  const char query1_lis[] = "query1.lis";
  char fetch_size[100+1] = "fetch_size=1";
  char last_option[100+1] = "userid=";
  char output_file[100+1] = "output_file=";
  const char *options[] = {
    fetch_size,
    "nls_date_format=yyyy-mm-dd hh24:mi:ss",
    "nls_timestamp_format=yyyy-mm-dd hh24:mi:ss",
    "nls_numeric_characters=.,",
    dbug_options,
    "query=\
select 1234567890 as NR, 'mystring' as STR, to_date('1900-12-31 23:23:59') as DAY from dual \
union \
select 2345678901, 'YOURSTRING', to_date('20001231232359', 'yyyymmddhh24miss') from dual",
    output_file,
    "fixed_column_length=0",
    last_option
  };
  char *error;

  fail_if(getenv("USERID") == NULL, "Environment variable USERID should be set");

  (void) strncat(last_option, getenv("USERID"), sizeof(last_option) - strlen(last_option));
  (void) strncat(output_file, query1_lis, sizeof(output_file) - strlen(output_file));

  fail_unless(NULL == oradumper(sizeof(options)/sizeof(options[0]), options, 0, sizeof(error_msg), error_msg), error_msg);
  /* skip connect, but append */
  strcpy(last_option, "output_append=1");
  fetch_size[strlen(fetch_size)-1] = '2';
  fail_unless(NULL == oradumper(sizeof(options)/sizeof(options[0]), options, 0, sizeof(error_msg), error_msg), error_msg);
  /* disconnect */
  fetch_size[strlen(fetch_size)-1] = '3';
  fail_unless(NULL == oradumper(sizeof(options)/sizeof(options[0]), options, 1, sizeof(error_msg), error_msg), error_msg);

  error = cmp_files(query1_lis, query1_lis_ref);
  fail_if(error != NULL, error);
}
END_TEST

START_TEST(test_query2)
{
  const char query2_lis_ref[] = "query2.lis.ref";
  const char query2_lis[] = "query2.lis";
  char userid[100+1] = "userid=";
  char output_file[100+1] = "output_file=";
  const char *options[] = {
    userid,
    "fetch_size=2",
    "nls_date_format=yyyy-mm-dd hh24:mi:ss",
    "nls_timestamp_format=yyyy-mm-dd hh24:mi:ss",
    "nls_numeric_characters=.,",
    dbug_options,
    "query=\
select to_clob(rpad('0123456789', 8000, '0123456789')) as myclob from dual",
    output_file,
    "fixed_column_length=0"
  };
  char *error;

  fail_if(getenv("USERID") == NULL, "Environment variable USERID should be set");

  (void) strncat(userid, getenv("USERID"), sizeof(userid) - strlen(userid));
  (void) strncat(output_file, query2_lis, sizeof(output_file) - strlen(output_file));

  fail_unless(NULL == oradumper(sizeof(options)/sizeof(options[0]), options, 1, sizeof(error_msg), error_msg), error_msg);

  error = cmp_files(query2_lis, query2_lis_ref);
  fail_if(error != NULL, error);
}
END_TEST

START_TEST(test_query3)
{
  const char query3_lis_ref[] = "query3.lis.ref";
  const char query3_lis[] = "query3.lis";
  char userid[100+1] = "userid=";
  char output_file[100+1] = "output_file=";
  const char *options[] = {
    userid,
    "fetch_size=3",
    "nls_date_format=yyyy-mm-dd hh24:mi:ss",
    "nls_timestamp_format=yyyy-mm-dd hh24:mi:ss",
    "nls_numeric_characters=.,",
    dbug_options,
    "query=\
select object_name, object_type from all_objects where owner = 'SYS' and object_type <> 'JAVA CLASS' and rownum <= :b1 order by object_name",
    output_file,
    "fixed_column_length=0",
    "1000" /* bind variable */
  };
  char *error;

  fail_if(getenv("USERID") == NULL, "Environment variable USERID should be set");

  (void) strncat(userid, getenv("USERID"), sizeof(userid) - strlen(userid));
  (void) strncat(output_file, query3_lis, sizeof(output_file) - strlen(output_file));

  fail_unless(NULL == oradumper(sizeof(options)/sizeof(options[0]), options, 1, sizeof(error_msg), error_msg), error_msg);

  error = cmp_files(query3_lis, query3_lis_ref);
  fail_if(error != NULL, error);
}
END_TEST

Suite *
options_suite(void)
{
  Suite *s = suite_create("General");

  TCase *tc_internal = tcase_create("Internal");
  TCase *tc_interface = tcase_create("Interface");

  tcase_add_test(tc_internal, test_sizes);
  tcase_add_test(tc_internal, test_usage);
  suite_add_tcase(s, tc_internal);

  tcase_set_timeout(tc_interface, 10);
  tcase_add_test(tc_interface, test_query1);
  tcase_add_test(tc_interface, test_query2);
  tcase_add_test(tc_interface, test_query3);
  suite_add_tcase(s, tc_interface);

  return s;
}

int
main(void)
{
  int number_failed;
  Suite *s = options_suite();
  SRunner *sr = srunner_create(s);

  if (getenv("DBUG_OPTIONS") != NULL)
    {
      (void) strcat(dbug_options, getenv("DBUG_OPTIONS"));
    }

  srunner_run_all(sr, CK_ENV); /* Use environment variable CK_VERBOSITY, which can have the values "silent", "minimal", "normal", "verbose" */
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return (number_failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}
