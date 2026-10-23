#ifndef S_STARTMENUFOLDER
#define S_STARTMENUFOLDER

#include "S_String.h"
#include "S_Directory.h"

#define S_STARTMENUFOLDER_ALLUSERSSTARTMENU    0
#define S_STARTMENUFOLDER_ALLUSERSPROGRAMS     1
#define S_STARTMENUFOLDER_ALLUSERSSTARTUP      2
#define S_STARTMENUFOLDER_STARTMENU            3
#define S_STARTMENUFOLDER_PROGRAMS             4
#define S_STARTMENUFOLDER_STARTUP              5

class S_StartMenuFolder : public S_Directory {
public:

   S_String allUsersStartMenu;
   S_String allUsersProgramsFolder;
   S_String allUsersStartupFolder;
   S_String userStartMenu;
   S_String userProgramsFolder;
   S_String userStartupFolder;

   int CreateFileLink(char *,char *,char * =NULL);
   int CreateURLLink(char *,char *);

   int Init();
   S_StartMenuFolder(S_StartMenuFolder * = NULL,char * = NULL);
   S_StartMenuFolder(int,char * = NULL);
   int Create(S_StartMenuFolder * = NULL,char * = NULL);
   int Create(int,char * = NULL);
   int Destroy();
   ~S_StartMenuFolder();
   };

#endif
