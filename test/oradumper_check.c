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

#if HAVE_SYS_ERRNO_H
#include <sys/errno.h>
#endif

#include "../src/lib/oradumper.h"
#include "../src/lib/oradumper_int.h"

/* include dmalloc as last one */
#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif

static bool cmp_files(const char *file1, const char *file2)
{
  FILE *fin1 = NULL, *fin2 = NULL;
  bool result = false;
  char line1[1000+1], line2[1000+1];

  if ((fin1 = fopen(file1, "r")) != NULL &&
      (fin2 = fopen(file2, "r")) != NULL)
    {
      result = true;
      while (fgets(line1, sizeof(line1), fin1) != NULL &&
	     fgets(line2, sizeof(line2), fin2) != NULL) {
	/* each line read should be of equal size and equal content */
	if (strcmp(line1, line2) != 0) {
	  result = false;
	  break;
	}
      }
    }

  if (fin1 != NULL) {
    (void) fclose(fin1);
  }
  if (fin2 != NULL) {
    (void) fclose(fin2);
  }
  
  return result;
}

START_TEST(test_sizes)
{
  fail_unless(sizeof(value_name_t) > 30, NULL);
  fail_unless(sizeof(character_set_name_t) >= 20, NULL);
}
END_TEST

START_TEST(test_usage)
{
  const char usage_txt[] = "usage.txt";
  const char usage_txt_ref[] = "usage.txt.ref";
  FILE *fout = fopen(usage_txt, "w");

  fail_if(fout == NULL, "File '%s' could not be opened for writing: %s", usage_txt, strerror(errno));

  oradumper_usage(fout);

  fail_if(fclose(fout) != 0, "File '%s' could not be closed: %s", usage_txt, strerror(errno));
  fail_if(cmp_files(usage_txt, usage_txt_ref) == false,
	  "Files '%s' and '%s' are not equal", usage_txt, usage_txt_ref);
}
END_TEST

Suite *
options_suite (void)
{
  Suite *s = suite_create ("General");

  /* Core test case */
  TCase *tc_interface = tcase_create ("Interface");
  tcase_add_test (tc_interface, test_sizes);
  tcase_add_test (tc_interface, test_usage);
  suite_add_tcase (s, tc_interface);

  return s;
}

int
main (void)
{
  int number_failed;
  Suite *s = options_suite ();
  SRunner *sr = srunner_create (s);
  srunner_run_all (sr, CK_NORMAL);
  number_failed = srunner_ntests_failed (sr);
  srunner_free (sr);
  return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
