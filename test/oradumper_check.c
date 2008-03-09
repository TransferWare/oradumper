#if HAVE_CONFIG_H
#include <config.h>
#endif

#if HAVE_CHECK_H
#include <check.h>
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

#define DBUG_OFF 1

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

static char dbug_options[1000+1] = "dbug_options=";
static char error_msg[1000+1] = "";
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

START_TEST(test_sizes)
{
  fail_unless(sizeof(value_name_t) > 30, NULL);
  fail_unless(sizeof(character_set_name_t) > 20, NULL);
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
  error = oradumper(0, options, 0, sizeof(error_msg), error_msg);

  fail_if(NULL != error, NULL);

  /* should display error */
  error = oradumper(1, options, 0, sizeof(error_msg), error_msg);

  fail_if(NULL == error, NULL);

  DBUG_LEAVE();
}
END_TEST

START_TEST(test_output_file)
{
  const char *options[] = { "query=select * from dual", "output_file=/", "dbug_options=d,g,t" };

  DBUG_ENTER("test_output_file");

  /* should display usage but no error */
  fail_if(NULL == oradumper(3, options, 0, sizeof(error_msg), error_msg), NULL);

  DBUG_LEAVE();
}
END_TEST

START_TEST(test_enclosure_string)
{
  char userid[100+1] = "userid=";
  char enclosure_string[100+1];
  int d1, d2, d3;
  const char *options[] = {
    userid,
    dbug_options,
    "query=select * from dual where 0=1",
    enclosure_string,
    "column_heading=0"
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
	  error = oradumper(sizeof(options)/sizeof(options[0]), options, 0, sizeof(error_msg), error_msg);
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
	  fail_if(NULL == oradumper(sizeof(options)/sizeof(options[0]), options, 0, sizeof(error_msg), error_msg), "invalid hexadecimal string");
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
		  error = oradumper(sizeof(options)/sizeof(options[0]), options, 0, sizeof(error_msg), error_msg);
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
		  error = oradumper(sizeof(options)/sizeof(options[0]), options, 1, sizeof(error_msg), error_msg);
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
	  error = oradumper(sizeof(options)/sizeof(options[0]), options, 1, sizeof(error_msg), error_msg);
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
		  error = oradumper(sizeof(options)/sizeof(options[0]), options, 1, sizeof(error_msg), error_msg);
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
			  error = oradumper(sizeof(options)/sizeof(options[0]), options, 1, sizeof(error_msg), error_msg);
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
    dbug_options,
    "query=select "
  };
  char *error;

  fail_if(getenv("USERID") == NULL, "Environment variable USERID should be set");

  (void) strncat(userid, getenv("USERID"), sizeof(userid) - strlen(userid));

  fail_unless(NULL != oradumper(sizeof(options)/sizeof(options[0]), options, 1, sizeof(error_msg), error_msg), error_msg);
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
    "nls_language=AMERICAN",
    "nls_date_format=yyyy-mm-dd hh24:mi:ss",
    "nls_timestamp_format=yyyy-mm-dd hh24:mi:ss",
    "nls_numeric_characters=.,",
    dbug_options,
    "query=\
select 1234567890 as NR, unistr('\"my,string\"') as STR, to_date('1900-12-31 23:23:59') as DAY from dual \
union all \
select 2345678901, unistr('YOURSTRING'), to_date('20001231232359', 'yyyymmddhh24miss') from dual where :x is null",
    output_file,
    "fixed_column_length=0",
    "column_separator=,",
    "feedback=1",
    "enclosure_string=\\x22", /* " */
    last_option
  };
  char *error;

  (void) sprintf(output_ref, "%s/%s.ref", srcdir, output);

  fail_if(getenv("USERID") == NULL, "Environment variable USERID should be set");

  (void) strncat(last_option, getenv("USERID"), sizeof(last_option) - strlen(last_option));
  (void) strncat(output_file, output, sizeof(output_file) - strlen(output_file));

  fail_unless(NULL == oradumper(sizeof(options)/sizeof(options[0]), options, 0, sizeof(error_msg), error_msg), error_msg);
  /* skip connect, but append */
  strcpy(last_option, "output_append=1");
  fetch_size[strlen(fetch_size)-1] = '2';
  fail_unless(NULL == oradumper(sizeof(options)/sizeof(options[0]), options, 0, sizeof(error_msg), error_msg), error_msg);
  /* disconnect */
  fetch_size[strlen(fetch_size)-1] = '3';
  fail_unless(NULL == oradumper(sizeof(options)/sizeof(options[0]), options, 1, sizeof(error_msg), error_msg), error_msg);

  error = cmp_files(output, output_ref);
  fail_if(error != NULL, error);
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
    dbug_options,
    "query=\
select to_clob(rpad('0123456789', 8000, '0123456789')) as myclob from dual",
    output_file,
    "fixed_column_length=0"
  };
  char *error;

  (void) sprintf(output_ref, "%s/%s.ref", srcdir, output);

  fail_if(getenv("USERID") == NULL, "Environment variable USERID should be set");

  (void) strncat(userid, getenv("USERID"), sizeof(userid) - strlen(userid));
  (void) strncat(output_file, output, sizeof(output_file) - strlen(output_file));

  fail_unless(NULL == oradumper(sizeof(options)/sizeof(options[0]), options, 1, sizeof(error_msg), error_msg), error_msg);

  error = cmp_files(output, output_ref);
  fail_if(error != NULL, error);
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
    dbug_options,
    "query=\
select object_name, object_type from all_objects where owner = 'SYS' and object_type <> 'JAVA CLASS' and rownum <= :b1 order by object_name",
    output_file,
    "fixed_column_length=0",
    "1000" /* bind variable */
  };
  char *error;

  (void) sprintf(output_ref, "%s/%s.ref", srcdir, output);

  fail_if(getenv("USERID") == NULL, "Environment variable USERID should be set");

  (void) strncat(userid, getenv("USERID"), sizeof(userid) - strlen(userid));
  (void) strncat(output_file, output, sizeof(output_file) - strlen(output_file));

  fail_unless(NULL == oradumper(sizeof(options)/sizeof(options[0]), options, 1, sizeof(error_msg), error_msg), error_msg);

  error = cmp_files(output, output_ref);
  fail_if(error != NULL, error);
}
END_TEST

START_TEST(test_query4)
{
  const char output[] = "query4.lis";
  char userid[100+1] = "userid=";
  char output_file[100+1] = "output_file=";
  const char *options[] = {
    userid,
    "fetch_size=100",
    "nls_date_format=yyyy-mm-dd hh24:mi:ss",
    "nls_timestamp_format=yyyy-mm-dd hh24:mi:ss",
    "nls_numeric_characters=.,",
    dbug_options,
    "query=\
select cast(1234567890 as number(10, 0)) as NR, unistr('my,string') as STR, to_date('1900-12-31 23:23:59') as DAY from dual \
union \
select 2345678901, unistr('YOURSTRING'), to_date('20001231232359', 'yyyymmddhh24miss') from dual \
union all \
select 12, unistr('abc\\00e5\\00f1\\00f6'), null from dual",
    output_file,
    "fixed_column_length=1",
    "column_separator=\\040" /* space */
  };
  char *error;

  (void) sprintf(output_ref, "%s/%s.ref", srcdir, output);

  fail_if(getenv("USERID") == NULL, "Environment variable USERID should be set");

  (void) strncat(userid, getenv("USERID"), sizeof(userid) - strlen(userid));
  (void) strncat(output_file, output, sizeof(output_file) - strlen(output_file));

  fail_unless(NULL == oradumper(sizeof(options)/sizeof(options[0]), options, 1, sizeof(error_msg), error_msg), error_msg);

  error = cmp_files(output, output_ref);
  fail_if(error != NULL, error);
}
END_TEST

START_TEST(test_query5)
{
  const char output[] = "query5.lis";
  char userid[100+1] = "userid=";
  char output_file[100+1] = "output_file=";
  const char *options[] = {
    userid,
    "fetch_size=1",
    "nls_date_format=yyyy-mm-dd hh24:mi:ss",
    "nls_timestamp_format=yyyy-mm-dd hh24:mi:ss.ff",
    "nls_numeric_characters=.,",
    dbug_options,
    "query=select rowid, t.* from oradumper_test t",
    output_file,
    "fixed_column_length=0"
  };
  char *error;

  (void) sprintf(output_ref, "%s/%s.ref", srcdir, output);

  fail_if(getenv("USERID") == NULL, "Environment variable USERID should be set");

  (void) strncat(userid, getenv("USERID"), sizeof(userid) - strlen(userid));
  (void) strncat(output_file, output, sizeof(output_file) - strlen(output_file));

  fail_unless(NULL == oradumper(sizeof(options)/sizeof(options[0]), options, 1, sizeof(error_msg), error_msg), error_msg);

  error = cmp_files(output, output_ref);
  fail_if(error != NULL, error);
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

  tcase_set_timeout(tc_query, 10);
  tcase_add_test(tc_query, test_query_sql_error);
  tcase_add_test(tc_query, test_query1);
  tcase_add_test(tc_query, test_query2);
  tcase_add_test(tc_query, test_query3);
  tcase_add_test(tc_query, test_query4);
  tcase_add_test(tc_query, test_query5);
  suite_add_tcase(s, tc_query);

  return s;
}

int
main(void)
{
  int number_failed;
  Suite *s = options_suite();
  SRunner *sr = srunner_create(s);

  DBUG_INIT("d,g,t,o=oradumper_check.log", "oradumper_check");

  if (getenv("DBUG_OPTIONS") != NULL)
    (void) strcat(dbug_options, getenv("DBUG_OPTIONS"));

  srcdir = (getenv("srcdir") != NULL ? getenv("srcdir") : ".");

  srunner_run_all(sr, CK_ENV); /* Use environment variable CK_VERBOSITY, which can have the values "silent", "minimal", "normal", "verbose" */
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);

  DBUG_DONE();

  return (number_failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}
