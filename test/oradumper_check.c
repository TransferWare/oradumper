#include <stdlib.h>
#include <check.h>
#include "../src/lib/oradumper.h"
#include "../src/lib/oradumper_int.h"

START_TEST (test_sizes)
{
  fail_unless (sizeof(value_name_t) == 31, 
	       "Size of value_name_t must be 31.");
  fail_unless (sizeof(character_set_name_t) == 21,
	       "Size of character_set_name_t must be 21.");
}
END_TEST

Suite *
options_suite (void)
{
  Suite *s = suite_create ("General");

  /* Core test case */
  TCase *tc_interface = tcase_create ("Interface");
  tcase_add_test (tc_interface, test_sizes);
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
