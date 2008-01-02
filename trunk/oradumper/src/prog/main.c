#include "oradumper.h"

int main( int argc, char **argv )
{
  return oradumper((unsigned int)(argc - 1), argv + 1);
}
