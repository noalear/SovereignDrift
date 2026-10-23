#include "S_OS.h"
#include "S_StartMenuFolder.h"


int
S_StartMenuFolder::Init()
{
  S_Directory::Init();

# ifdef WIN32
  char startUpFolder[MAX_PATH];
  char startMenu[MAX_PATH];
  char programsFolder[MAX_PATH];
  char allStartUpFolder[MAX_PATH];
  char allStartMenu[MAX_PATH];
  char allProgramsFolder[MAX_PATH];

  LPITEMIDLIST pidl;
  LPMALLOC pShellMalloc;
  if ( SUCCEEDED(SHGetMalloc(&pShellMalloc)) )
    {

      if ( SUCCEEDED(SHGetSpecialFolderLocation(NULL,
						CSIDL_COMMON_PROGRAMS,
						&pidl)) )
	{
	  if ( pidl )
	    SHGetPathFromIDList(pidl,allProgramsFolder);
	  pShellMalloc->Free(pidl);
	}
 
      if ( SUCCEEDED(SHGetSpecialFolderLocation(NULL,
						CSIDL_PROGRAMS,
						&pidl)) )
	{
	  if ( pidl )
	    SHGetPathFromIDList(pidl,programsFolder);
	  pShellMalloc->Free(pidl);
	}

      if ( SUCCEEDED(SHGetSpecialFolderLocation(NULL,
						CSIDL_COMMON_STARTMENU,
						&pidl)) )
	{
	  if ( pidl )
	    SHGetPathFromIDList(pidl,allStartMenu);
	  pShellMalloc->Free(pidl);
	}

      if ( SUCCEEDED(SHGetSpecialFolderLocation(NULL,
						CSIDL_STARTMENU,
						&pidl)) )
	{
	  if ( pidl )
	    SHGetPathFromIDList(pidl,startMenu);
	  pShellMalloc->Free(pidl);
	}

      if ( SUCCEEDED(SHGetSpecialFolderLocation(NULL,
						CSIDL_COMMON_STARTUP,
						&pidl)) )
	{
	  if ( pidl )
	    SHGetPathFromIDList(pidl,allStartUpFolder);
	  pShellMalloc->Free(pidl);
	}

      if ( SUCCEEDED(SHGetSpecialFolderLocation(NULL,
						CSIDL_STARTUP,
						&pidl)) )
	{
	  if ( pidl )
	    SHGetPathFromIDList(pidl,startUpFolder);
	  pShellMalloc->Free(pidl);
	}

    }

  if ( _OsType() == S_OS_WIN95 )
    {
      strcpy(allStartMenu,startMenu);
      strcpy(allProgramsFolder,programsFolder);
      strcpy(allStartUpFolder,startUpFolder);
    }

  userStartMenu.Create(startMenu);
  allUsersStartMenu.Create(allStartMenu);
  userProgramsFolder.Create(programsFolder);
  allUsersProgramsFolder.Create(allProgramsFolder);
  userStartupFolder.Create(startUpFolder);
  allUsersStartupFolder.Create(allStartUpFolder);
# endif

  return 0;
}

S_StartMenuFolder::S_StartMenuFolder(S_StartMenuFolder *iparent,
                                     char *ifolderName) {
   Init();
   Create(iparent,ifolderName);
   }

S_StartMenuFolder::S_StartMenuFolder(int iparentID,char *ifolderName) {
   Init();
   Create(iparentID,ifolderName);
   }


int
S_StartMenuFolder::Create(S_StartMenuFolder *iparent,char *ifolderName) {

   char path[1024];
   if ( iparent )
      sprintf(path,"%s",iparent->name.str);

   if ( ifolderName )
      sprintf(path,"%s\\%s",path,ifolderName);

   S_Directory::Create(path);

   return 0;
   }


int
S_StartMenuFolder::Create(int iparentID,char *ifolderName) {

   char path[1024];

   switch ( iparentID ) {
      case S_STARTMENUFOLDER_ALLUSERSSTARTMENU :
         sprintf(path,"%s",allUsersStartMenu.str);
         break;
      case S_STARTMENUFOLDER_STARTMENU :
         sprintf(path,"%s",userStartMenu.str);
         break;
      case S_STARTMENUFOLDER_ALLUSERSPROGRAMS :
         sprintf(path,"%s",allUsersProgramsFolder.str);
         break;
      case S_STARTMENUFOLDER_PROGRAMS :
         sprintf(path,"%s",userProgramsFolder.str);
         break;
      case S_STARTMENUFOLDER_ALLUSERSSTARTUP :
         sprintf(path,"%s",allUsersStartupFolder.str);
         break;
      case S_STARTMENUFOLDER_STARTUP :
         sprintf(path,"%s",userStartupFolder.str);
         break;
      default :
         sprintf(path,"%s",allUsersStartMenu.str);
         break;
      }

   if ( ifolderName )
      sprintf(path,"%s\\%s",path,ifolderName);

   S_Directory::Create(path);

   return 0;
   }


int
S_StartMenuFolder::Destroy() {
   S_Directory::Destroy();
   return 0;
   }


S_StartMenuFolder::~S_StartMenuFolder() {
   Destroy();
   }


int
S_StartMenuFolder::CreateURLLink(char *url,char *desc) {
#  ifdef WIN32
      char link[MAX_PATH];
      sprintf(link,"%s\\%s.url",name.str,desc);
      FILE *fptr = fopen(link,"w");
      if ( fptr ) {
         fprintf(fptr,"[InternetShortcut]\n");
         fprintf(fptr,"URL=%s\n",url);
         fclose(fptr);
         }
#  endif
   return 0;
   }


int
S_StartMenuFolder::CreateFileLink(char *path,char *desc,char *args) {
#  ifdef WIN32
      char link[MAX_PATH];
      sprintf(link,"%s\\%s.lnk",name.str,desc);

      HRESULT hres; 
      IShellLink* psl; 
      CoInitialize(NULL);
      hres = CoCreateInstance(CLSID_ShellLink, NULL, 
                              CLSCTX_INPROC_SERVER,
                              IID_IShellLink, (LPVOID *)&psl); 
      if ( hres == S_OK ) { 
         IPersistFile* ppf;

         psl->SetPath(path);
         psl->SetDescription(desc);
         if ( args )
            psl->SetArguments(args);

         char wdir[MAX_PATH];
         strcpy(wdir,path);
         for ( int i=strlen(wdir); wdir[i]!='\\'&& i>0; i-- );
         wdir[i+1] = 0;
         psl->SetWorkingDirectory(wdir);

         hres = psl->QueryInterface(IID_IPersistFile,(LPVOID *)&ppf); 
         if ( hres == S_OK ) { 
            WORD wsz[MAX_PATH];
            MultiByteToWideChar(CP_ACP, 0,link,-1,
                                (LPWSTR)wsz,MAX_PATH); 
            ppf->Save(wsz, TRUE);
            ppf->Release(); 
            SHChangeNotify(SHCNE_CREATE,SHCNF_PATH,link,NULL);
            }
         psl->Release(); 
         }
 # endif
   return 0;
   }

