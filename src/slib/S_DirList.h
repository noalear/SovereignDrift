/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
#ifndef S_DIRLIST
#define S_DIRLIST

#define S_DIRLIST_FILES               1
#define S_DIRLIST_DIRECTORIES         2

#if  defined(OS2) || defined(WIN32)
#  define S_DIRLIST_SEPARATOR '\\'
#endif

#ifdef UNIX
#  define S_DIRLIST_SEPARATOR '/'
#endif

#ifdef WIN32
#  define HDIR_SYSTEM 0
   typedef HANDLE HDIR;
#endif
#ifdef UNIX
#  define NO_ERROR             0
#  define ERROR_NO_MORE_FILES -1
#endif

class S_DirList {
public:
   slong incFiles;
   slong iniFiles;
   slong numFiles;
   slong maxFiles;
   char **file;
   char *dirName;

   char *FileName(int);
   char *FullPathName(int);
   char *DirName(int);

   int Init();
   int Destroy();
   int Create(char *,int=S_DIRLIST_FILES | S_DIRLIST_DIRECTORIES);
   S_DirList(char *,int=S_DIRLIST_FILES | S_DIRLIST_DIRECTORIES);
   ~S_DirList();
   };

#endif

int S_DirList_FileExists(char *);
