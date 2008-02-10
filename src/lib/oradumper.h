#ifndef ORADUMPER_H
#define ORADUMPER_H

extern
/*@null@*/ /*@observer@*/
char *
oradumper(const unsigned int nr_arguments,
	  const char **arguments,
	  const int disconnect,
	  const size_t error_msg_size,
	  char *error_msg);

#endif
