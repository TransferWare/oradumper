#ifndef ORADUMPER_INT_H
#define ORADUMPER_INT_H

typedef enum {
  OPTION_USERID = 0,
  OPTION_SQLSTMT,
  OPTION_ARRAYSIZE
} option_t;

extern
void
process_options(const unsigned int length, const char **options);

extern
/*@null@*//*@observer@*/
const char *
get_option(const option_t option);

#endif
