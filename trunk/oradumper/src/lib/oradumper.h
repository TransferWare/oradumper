#ifndef ORADUMPER_H
#define ORADUMPER_H

#define TRANSFERWARE_BUG_67 1

extern
/*@null@*/ /*@observer@*/
char *
oradumper(const unsigned int nr_arguments,
    const char **arguments,
    const int disconnect,
    const size_t error_msg_size,
    /*@out@*/ char *error_msg,
    /*@out@*/ unsigned int *row_count);

#endif
