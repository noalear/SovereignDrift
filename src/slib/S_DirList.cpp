/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
#include "S_OS.h"
#include "S_DirList.h"


int
S_DirList::Init()
{
  dirName = NULL;
  file = NULL;
  numFiles = 0;
  maxFiles = 0;
  incFiles = 1024;
  iniFiles = 1024;
  
  return 0;
}


S_DirList::S_DirList(char *dirMask,int flag)
{
  Init();
  Create(dirMask,flag);
}


int
S_DirList::Create(char *dirMask,int flag) {

  Destroy();

#  ifdef OS2
      HDIR hdir = HDIR_SYSTEM;
      FILEFINDBUF3 ffb = {0};
      ulong ffblen = sizeof(FILEFINDBUF3);
      ulong findCount = 1;
#  endif
#  ifdef WIN32
      HANDLE hdir;
      WIN32_FIND_DATA fd;
#  endif

   int i;
   char *dir;

   dir = (char *)S_malloc(strlen(dirMask)+1);
   strcpy(dir,dirMask);
   i = strlen(dir);
   while ( dir[i] != S_DIRLIST_SEPARATOR && i >= 0 )
     dir[i--] = 0;

#  if defined (OS2) || defined(WIN32)

      int rc = ERROR_NO_MORE_FILES;;
      char *fileName = NULL;
      int isDir = 0;

      rc = ERROR_NO_MORE_FILES;
#     ifdef OS2
         rc = DosFindFirst(dirMask,&hdir,FILE_NORMAL | FILE_DIRECTORY,&ffb,
                           ffblen,&findCount,FIL_STANDARD);
         if ( rc == NO_ERROR ) {
            fileName = ffb.achName;
            isDir = ffb.attrFile & FILE_DIRECTORY;
            }
#     endif
#     ifdef WIN32
         hdir = FindFirstFile(dirMask,&fd);
         if ( hdir != INVALID_HANDLE_VALUE && hdir != (HANDLE)0xFFFFFFFF )
            rc = NO_ERROR;
         if ( rc == NO_ERROR ) {
            fileName = fd.cFileName;
            isDir = fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
            }
#     endif

      if ( rc == NO_ERROR && (
          ( isDir && flag & S_DIRLIST_DIRECTORIES) ||
          (!isDir && flag & S_DIRLIST_FILES) ) ) {
         file = (char **) S_malloc(sizeof(char *)*(iniFiles));
         maxFiles = iniFiles;
         if ( file )
	   {
	     file[numFiles]=(char *)S_malloc(strlen(fileName)+1+strlen(dir));
	     sprintf(file[numFiles],"%s%s",dir,fileName);
	     numFiles++;
	   }
	 else
	   {
             numFiles = 0;
             maxFiles = 0;
	   }
         }

      while ( rc == NO_ERROR && rc != ERROR_NO_MORE_FILES ) {
#        ifdef OS2
            rc = DosFindNext(hdir,&ffb,ffblen,&findCount);
            if ( rc == NO_ERROR ) {
               fileName = ffb.achName;
               isDir = ffb.attrFile & FILE_DIRECTORY;
               }
#        endif
#        ifdef WIN32
            rc = FindNextFile(hdir,&fd);
            if ( rc == TRUE ) {
              fileName = fd.cFileName;
               rc = NO_ERROR;
               isDir = fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
               }
            else
               rc = ERROR_FILE_NOT_FOUND;
#        endif
         if ( rc == NO_ERROR && (
             ( isDir && flag & S_DIRLIST_DIRECTORIES) ||
             (!isDir && flag & S_DIRLIST_FILES) ) ) {
            if ( !file || maxFiles == 0 )
	      {
               file = (char **) S_malloc(sizeof(char *)*iniFiles);
	       maxFiles = iniFiles;
	      }
            else if ( file && numFiles >= maxFiles )
	      {
                file = (char **) S_realloc(file,
					   sizeof(char *)*(maxFiles+incFiles));
                maxFiles += incFiles;
	      }
	    if ( file )
	      {
		file[numFiles] = (char *)
		  S_malloc(strlen(fileName)+1+strlen(dir));
		sprintf(file[numFiles],"%s%s",dir,fileName);
		numFiles++;
	      }
	    else
	      {
                numFiles = 0;
                maxFiles = 0;
	      }
            }
         }
#     ifdef OS2
         DosFindClose(hdir);
#     endif
#     ifdef WIN32
         FindClose(hdir);
#     endif
#  endif


#  ifdef UNIX
      glob_t g;
      glob(dirMask,0,NULL,&g);

      numFiles = 0;
      for ( i=0; i<(int)g.gl_pathc; i++ ) {
         struct stat sb;
         if ( lstat(g.gl_pathv[i],&sb) >= 0 ) {
            if ( ((sb.st_mode & S_IFMT) == S_IFREG && 
                  (flag & S_DIRLIST_FILES)) ||
                 ((sb.st_mode & S_IFMT) == S_IFDIR && 
                  (flag & S_DIRLIST_DIRECTORIES)) ) {
               if ( !file || maxFiles == 0 )
		 {
                  file = (char **) S_malloc(sizeof(char *)*g.gl_pathc);
		  maxFiles = g.gl_pathc;
		 }
               else if ( file && numFiles >= maxFiles )
		 {
		   file = (char **)S_realloc(file,
					     sizeof(char *)*
					     (maxFiles+incFiles));
		   maxFiles += incFiles;
		 }
	       if ( file )
		 {
		   file[numFiles] = S_strdup(g.gl_pathv[i]);
		   numFiles++;
		 }
	       else
		 {
		   numFiles = 0;
		   maxFiles = 0;
		 }
	       }
	    }
         }
      globfree(&g);
#  endif

   if ( dir )
      S_free(dir);

   return 0;
   }

char *S_DirList::FileName(int i) {
   int j;
   for ( j=strlen(file[i]); j > 0 && file[i][j] != S_DIRLIST_SEPARATOR; j-- );
   if ( file[i][j] == S_DIRLIST_SEPARATOR )
      return &file[i][j+1];
   else
      return &file[i][j];
   }

char *S_DirList::FullPathName(int i) {
   return file[i];
   }

char *S_DirList::DirName(int i) {
   if ( dirName )
      S_free(dirName);
   dirName = S_strdup(file[i]);
   int j;
   for ( j=strlen(dirName); j > 0 && dirName[j] != S_DIRLIST_SEPARATOR; j-- );
   dirName[j+1] = 0;
   return dirName;
   }

int
S_DirList::Destroy()
{
  if ( file )
    {
      for ( int i=0; i<numFiles; i++ )
	{
	  if ( file[i] )
	    {
	      S_free( file[i] );
	      file[i] = NULL;
	    }
	}
      S_free( file );
      file = NULL;
    }
  if ( dirName )
    {
      S_free(dirName);
      dirName = NULL;
    }
  Init();
  return 0;
}


S_DirList::~S_DirList()
{
  Destroy();
}


int S_DirList_FileExists(char *filename) {
   S_DirList dirList(filename);
   return dirList.numFiles > 0;
   }
