#include <stdio.h>
#include "oradumper.h"

int main( int argc, char **argv )
{
  return epc__main( argc, argv, &ifc_plsdbug );
}
