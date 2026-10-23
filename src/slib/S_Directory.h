#ifndef S_DIRECTORY
#define S_DIRECTORY

#include "S_String.h"

#define S_DIRECTORY_REMOVETREEFAILED -4
#define S_DIRECTORY_CHANGEFAILED     -3
#define S_DIRECTORY_DELETEFAILED     -2
#define S_DIRECTORY_CREATEFAILED     -1
#define S_DIRECTORY_OK                0
#define S_DIRECTORY_DIRECTORIES       1
#define S_DIRECTORY_FILES             2

#if  defined(OS2) || defined(WIN32)
#  define S_DIRECTORY_SEPARATOR '\\'
#endif

#ifdef UNIX
#  define S_DIRECTORY_SEPARATOR '/'
#endif

class S_Directory {
public:
   int rc;
   S_String name;

   int CreateDir();
   int RemoveDir();
   int RemoveTree(int=S_DIRECTORY_DIRECTORIES|S_DIRECTORY_FILES);
   int Exists();
   int Dir();
   int ChangeTo();

   int Init();
   S_Directory(char * =NULL);
   int Create(char * =NULL);
   int Destroy();
   ~S_Directory();
   };

#endif
