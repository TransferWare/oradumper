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

#if HAVE_UNILBRK_H
#include <unilbrk.h>
#endif

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

START_TEST(test_linebreaks)
{
  DBUG_ENTER("test_linebreaks");
#if defined(WITH_DMALLOC) && defined(HAVE_SEARCH_H)
  dmalloc_track(dmalloc_track_func);
#endif

  /* u8_width_linebreaks (const uint8_t *s, size_t n, int width, int start_column, int at_end_columns, const char *override, const char *encoding, char *p) */
  fail_if(0);
  
#if defined(WITH_DMALLOC) && defined(HAVE_SEARCH_H)
  fail_if(addr_list != NULL);
  dmalloc_track(NULL);
#endif

  DBUG_LEAVE();
}
END_TEST

Suite *
options_suite(void)
{
  Suite *s = suite_create("General");

  TCase *tc_internal = tcase_create("Internal");

  tcase_add_test(tc_internal, test_linebreaks);
  suite_add_tcase(s, tc_internal);

  return s;
}

int
main(void)
{
  int number_failed;
  Suite *s = options_suite();
  SRunner *sr = srunner_create(s);

  DBUG_INIT("d,g,t,o=dbug.log", "check_unistring");  
  DBUG_ENTER("main");
  
  srcdir = (getenv("srcdir") != NULL ? getenv("srcdir") : ".");

  srunner_run_all(sr, CK_ENV); /* Use environment variable CK_VERBOSITY, which can have the values "silent", "minimal", "normal", "verbose" */
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);

  DBUG_LEAVE();
  DBUG_DONE();

  return (number_failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}
