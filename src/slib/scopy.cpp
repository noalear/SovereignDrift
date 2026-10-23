#include "S_Lib.h"

/* ************************************************************** */
/* *** Copy a file to the system directory for the purpose of *** */
/* *** knowing it will be in the execution path.              *** */
/* ************************************************************** */

main(int argc, char *argv[])
{
  
  if ( argc < 2 )
    {
      printf("Usage: scopy [filename]\n");
      exit(0);
    }
  
  char *filename = argv[1];
  S_String dest("%s/%s",_OsSystemDir(),filename);
  _OsFilename(dest.str);
  _OsCopyFile(filename,dest.str);
  
  return 0;
  
}
