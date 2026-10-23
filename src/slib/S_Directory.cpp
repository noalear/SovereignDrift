#include "S_OS.h"
#include "S_Directory.h"
#include "S_DirList.h"


int 
S_Directory::Init() {
   rc = S_DIRECTORY_OK;
   return 0;
   }


S_Directory::S_Directory(char *iname) {
   Init();
   Create(iname);
   }


int
S_Directory::Create(char *iname) {
   Destroy();
   if ( iname ) {
      name.Create(iname);
      }
   else {
      char tdir[1025];
#     ifdef OS2
         char dir[1025];
         ulong num,mask,len=1024;
         DosQueryCurrentDisk(&num,&mask);
         DosQueryCurrentDir(num,dir,&len);
         sprintf(tdir,"%c:\\%s",'a'+num-1,dir);
#     endif
#     ifdef WIN32
         GetCurrentDirectory(1024,tdir);
#     endif
#     ifdef UNIX
         getcwd(tdir,1024);
#     endif
      name.Create(tdir);
      }
   return 0;
   }


int
_CreateDir(char *name)
{
  int rc = S_DIRECTORY_OK;
# ifdef OS2
  int wrc = DosCreateDir(name,NULL);
  if ( wrc != NO_ERROR && 
       wrc != ERROR_ACCESS_DENIED && 
       wrc != ERROR_PATH_NOT_FOUND )
    {
      rc = S_DIRECTORY_CREATEFAILED;
    }
# endif
# ifdef WIN32
  if ( !CreateDirectory(name,NULL) )
    {
      ulong wrc = GetLastError();
      if ( wrc != ERROR_SUCCESS && wrc != ERROR_ALREADY_EXISTS )
	{
          rc = S_DIRECTORY_CREATEFAILED;
	}
    }
  SHChangeNotify(SHCNE_MKDIR,SHCNF_PATH,name,NULL);
# endif
# ifdef UNIX
  if ( mkdir(name,0) != 0 )
    {
      rc = S_DIRECTORY_CREATEFAILED;
    }
# endif
  return rc;
}


int
_RemoveDir(char *name)
{
  int rc = S_DIRECTORY_OK;
# ifdef OS2
  if ( DosDeleteDir(name) != NO_ERROR )
    {
      rc = S_DIRECTORY_DELETEFAILED;
    }
# endif
# ifdef WIN32
  if ( !RemoveDirectory(name) )
    {
      rc = S_DIRECTORY_DELETEFAILED;
    }
  SHChangeNotify(SHCNE_RMDIR,SHCNF_PATH,name,NULL);
# endif
# ifdef UNIX
  if ( rmdir(name) != 0 )
    {
      rc = S_DIRECTORY_DELETEFAILED;
    }
# endif
  return rc;
}


int
_DeleteAll(char *dir,int options) {
   int i;
   int rc = S_DIRECTORY_OK;
   char *mask = (char *)S_malloc(strlen(dir)+strlen("\\*.*")+1);
   sprintf(mask,"%s\\*.*",dir);

   if ( options & S_DIRECTORY_FILES ) {
      S_DirList fileList(mask,S_DIRLIST_FILES);
      for ( i=0; i<fileList.numFiles; i++ )
         rc |=_OsDeleteFile(fileList.FullPathName(i));
      }

   S_DirList dirList(mask,S_DIRLIST_DIRECTORIES);
   for ( i=0; i<dirList.numFiles; i++ ) {
      if ( !strstr(dirList.FileName(i),"..") &&
           !strstr(dirList.FileName(i),".") ) {
         rc |=_DeleteAll(dirList.FullPathName(i),options);
         if ( options & S_DIRECTORY_DIRECTORIES )
            rc |=_RemoveDir(dirList.FullPathName(i));
         }
      }

   S_free(mask);
   return rc;
   }
      


int
S_Directory::RemoveTree(int options) {
   rc = _DeleteAll(name.str,options);
   if ( options & S_DIRECTORY_DIRECTORIES )
      rc = _RemoveDir(name.str);
   if ( rc != S_DIRECTORY_OK )
     {
       rc = S_DIRECTORY_REMOVETREEFAILED;
     }
   return rc;
   }


int
S_Directory::RemoveDir() {
   rc = _RemoveDir(name.str);
   return rc;
   }


int
S_Directory::CreateDir() {
   int i=0;
   int len = strlen(name.str);
   while ( name.str[i] != S_DIRECTORY_SEPARATOR && name.str[i] != 0 ) i++;
   while ( i < len ) {
      if ( name.str[i] == S_DIRECTORY_SEPARATOR ) {
         name.str[i] = '\0';
         rc = _CreateDir(name.str);
         name.str[i] = S_DIRECTORY_SEPARATOR;
         }
      i++;
      }
   rc = _CreateDir(name.str);
   return rc;
   }


int
S_Directory::ChangeTo()
{
  rc = S_DIRECTORY_OK;

  if ( !name.str )
    {
      rc = S_DIRECTORY_CHANGEFAILED;
      return rc;
    }

# ifdef OS2
  if ( name.str[1] == ':' )
    {
      if ( DosSetDefaultDisk(toupper(name.str[0])-'A'+1) != NO_ERROR )
	{
          rc = S_DIRECTORY_CHANGEFAILED;
	}
    }
  if ( DosSetCurrentDir(name.str) != NO_ERROR )
    {
      rc = S_DIRECTORY_CHANGEFAILED;
    }
# endif
# ifdef WIN32
  if ( !SetCurrentDirectory(name.str) )
    {
      rc = S_DIRECTORY_CHANGEFAILED;
    }
# endif
# ifdef UNIX
  if ( chdir(name.str) != 0 )
    {
      rc = S_DIRECTORY_CHANGEFAILED;
    }
# endif
  return rc;
}


int
S_Directory::Destroy() {
   return 0;
   }


S_Directory::~S_Directory() {
   Destroy();
   }
