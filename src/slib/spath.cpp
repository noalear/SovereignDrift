#include "S_Lib.h"

/* ****************************************************************** */
/* *** Check the PATH environment variable for argv[1].  If it is *** */
/* *** not there return 0 else return 1.                          *** */
/* ****************************************************************** */
int
main(int argc, char *argv[])
{
  
  if ( argc < 2 )
    {
      printf("Usage: s_path [dir]\n");
      exit(0);
    }
  
  S_Directory currentDir;
  
  char *dir = argv[1];
  
# if defined(WIN32) || defined(OS2)
  S_String testDir("%c:%s",currentDir.name.str[0],dir);
  char sep = ';';
# endif
# ifdef UNIX
  S_String testDir("%s",dir);
  char sep = ':';
# endif
  
  char *systemPath = getenv("PATH");
  
  char *subPath = strstr(systemPath,testDir.str);

  if ( subPath && 
       (subPath[strlen(testDir.str)] == sep || 
	subPath[strlen(testDir.str)] == '\0') )
    {
      return 1;
    }
  
  subPath = strstr(systemPath,dir);

  if ( subPath && 
       (subPath[strlen(dir)] == sep || 
	subPath[strlen(dir)] == '\0') )
    {
      return 1;
    }
  
  return 0;
  
}

