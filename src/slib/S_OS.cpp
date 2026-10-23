/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
#include "S_OS.h"
#include "S_Process.h"

char *
stristr(char *istr,char *isubstr)
{
  
  char *rc = NULL;
  
  if ( !istr || !isubstr )
    return rc;
  
  char *str = S_strdup(istr);
  if ( str )
    {
      char *substr = S_strdup(isubstr);
      if ( substr )
	{
	  int i;
	  for ( i=0; i<(int)strlen(str); i++ )
            str[i] = tolower(str[i]);
	  for ( i=0; i<(int)strlen(substr); i++ )
            substr[i] = tolower(substr[i]);
	  rc = strstr(str,substr);
	  S_free(substr);
	}
      S_free(str);
    }
  
  return rc;
}


int
_OsOpenURL(char *url)
{
  if ( url )
    {
#     ifdef OS2
      S_Process process;
      process.Start("netscape.exe",url);
#     endif
#     ifdef WIN32
      ShellExecute(NULL,"open",url,NULL,NULL,0);
#     endif
    }
  return 0;
}


ulong 
_OsFreeMem()
{
  ulong bytes = 0;
# ifdef OS2
# endif
# ifdef WIN32
  MEMORYSTATUS ms;
  ms.dwLength = sizeof(ms);
  GlobalMemoryStatus(&ms);
  bytes = ms.dwAvailPhys + ms.dwAvailPageFile;
# endif
  return bytes;
}



char *
_OsFilename(char *filename)
{
  if ( filename )
    {
      for ( int i=0; i<(int)strlen(filename); i++ )
#     if defined(WIN32) || defined(OS2)
	if ( filename[i] == '/' )
	  filename[i] = '\\';
#     endif
#     if defined(UNIX)
      if ( filename[i] == '\\' )
	filename[i] = '/';
#     endif
    }
  return filename;
}


ulong 
_OsFileSize(char *filename)
{
  ulong fileSize = 0;
  if ( filename )
    {
      FILE *fptr = fopen(filename,"rb");
      if ( fptr )
	{
	  fseek(fptr,0,SEEK_END);
	  fileSize = ftell(fptr);
	  fclose(fptr);
	}
    }
  return fileSize;
}


ulong 
_OsFileCheckSum(char *filename,int begin,int end,ulong *psize)
{
  ulong checkSum = 0;
  if ( filename )
    {
      FILE *fptr = fopen(filename,"rb");
      if ( fptr )
	{
	  //
	  // Get the size of the file
	  //
	  fseek(fptr,0,SEEK_END);
	  ulong size = ftell(fptr);
	  if ( psize )
	    {
	      *psize = size;
	    }
	  
	  rewind(fptr);
	  
	  if ( size > 0 )
	    {
	      
	      if ( end == 0 )
		{
		  end = size;
		}
	      
	      //
	      // Allocate buffer and compute checksum
	      //	    
	      char *buf = (char *)S_malloc(size);
	      if ( buf )
		{
		  if ( fread(buf,size,1,fptr) )
		    {
		      for ( int i=begin; i<end; i++ )
			{
			  checkSum += buf[i];
			}
		    }
		  S_free(buf);
		}
	      
	    }
	  
	  fclose(fptr);
	}
    }
  return checkSum;
}


time_t 
_OsGetFileDate(char *filename,ulong options)
{
  if ( filename )
    {
#     ifdef WIN32
      void *hFile;
      FILETIME ftime;
      hFile = CreateFile(filename,GENERIC_READ,FILE_SHARE_READ,
                         NULL,OPEN_EXISTING,0,0);
      if ( hFile != (void *)0xFFFFFFFF )
	{
	  switch(options)
	    {
	    case S_OS_FILECREATIONDATE :
	      GetFileTime(hFile,&ftime,NULL,NULL);
	      break;
            case S_OS_FILEMODIFIEDDATE :
	      GetFileTime(hFile,NULL,NULL,&ftime);
	      break;
            case S_OS_FILEACCESSEDDATE :
	      GetFileTime(hFile,NULL,&ftime,NULL);
	      break;
            }
	  CloseHandle(hFile);
	  
	  LARGE_INTEGER utime0 = {0xD53E8000,0x019DB1DE};
	  LARGE_INTEGER wtime  = {ftime.dwLowDateTime,ftime.dwHighDateTime};
	  wtime.QuadPart -= utime0.QuadPart;
	  wtime.QuadPart /= 10000000;
	  
	  return wtime.LowPart;
	}
#     endif
#     ifdef OS2
#     endif
#     ifdef UNIX
      struct stat buf;
      stat(filename,&buf);
      switch(options)
	{
	case S_OS_FILECREATIONDATE :
	  return buf.st_ctime;
	case S_OS_FILEMODIFIEDDATE :
	  return buf.st_mtime;
	case S_OS_FILEACCESSEDDATE :
	  return buf.st_atime;
	}
#     endif
    }
  return 0;
}


int
_OsSetFileDate(char *filename,time_t utctime,ulong options)
{
  if ( filename )
    {
#     ifdef WIN32
      void *hFile;
      
      LARGE_INTEGER utctime0 = {0xD53E8000,0x019DB1DE};
      LARGE_INTEGER wtime = {utctime,0};
      wtime.QuadPart *= 10000000;
      wtime.QuadPart += utctime0.QuadPart;
      
      FILETIME ftime = { wtime.LowPart,wtime.HighPart };
      
      hFile = CreateFile(filename,GENERIC_READ,FILE_SHARE_READ,
                         NULL,OPEN_EXISTING,0,0);
      switch(options)
	{
	case S_OS_FILECREATIONDATE :
	  SetFileTime(hFile,&ftime,NULL,NULL);
	  break;
	case S_OS_FILEMODIFIEDDATE :
	  SetFileTime(hFile,NULL,NULL,&ftime);
	  break;
	case S_OS_FILEACCESSEDDATE :
	  SetFileTime(hFile,NULL,&ftime,NULL);
	  break;
	}
      CloseHandle(hFile);
#     endif
#     ifdef OS2
#     endif
#     ifdef UNIX
      struct utimbuf ubuf;
      struct stat sbuf;
      stat(filename,&sbuf);
      switch ( options )
	{
	case S_OS_FILEMODIFIEDDATE :
	  ubuf.modtime = utctime;
	  ubuf.actime = sbuf.st_atime;
	  utime(filename,&ubuf);
	  break;
	case S_OS_FILEACCESSEDDATE :
	  ubuf.modtime = sbuf.st_mtime;
	  ubuf.actime = utctime;
	  utime(filename,&ubuf);
	  break;
	}
#     endif
    }
  return 0;
}


int
_OsFileExists(char *filename)
{
  if ( filename )
    {
      FILE *fptr = fopen(filename,"rb");
      if ( fptr )
	{
	  fclose(fptr);
	  return TRUE;
	}
    }
  return FALSE;
}


int
_OsSleep(ulong usec)
{
# ifdef WIN32
  Sleep(usec/1000);
# endif
# ifdef OS2
  DosSleep(usec/1000);
# endif
# ifdef UNIX
  usleep(usec);
# endif
  return 0;
}


int
_OsGetNumCPUs()
{
# ifdef WIN32
  SYSTEM_INFO si;
  GetSystemInfo(&si);
  return si.dwNumberOfProcessors;
# endif
# ifdef OS2
  return 1;
# endif
# ifdef UNIX
  return 1;
# endif
}

int
_OsSystemDisk()
{
# ifdef OS2
  ulong disknum;
  DosQuerySysInfo(QSV_BOOT_DRIVE,QSV_BOOT_DRIVE,&disknum,sizeof(ulong));
  return 'A'+disknum-1;
# endif
# ifdef WIN32
  char systemDir[1024];
  GetSystemDirectory(systemDir,1024);
  return systemDir[0];
# endif
# ifdef UNIX
  return 0;
# endif
}


char *
_OsSystemDir()
{
  static char systemDir[1024];
# ifdef OS2
  sprintf(systemDir,"%c:\\OS2",_OsSystemDisk());
# endif
# ifdef WIN32
  GetSystemDirectory(systemDir,1024);
# endif
# ifdef UNIX
  sprintf(systemDir,"/usr/bin");
# endif
  return systemDir;
}


int
_OsSetFileReadOnly(char *file,int readOnly)
{
  int rc = S_OS_OK;
# ifdef OS2
# endif
# ifdef WIN32
  if ( !readOnly )
    {
      rc = SetFileAttributes(file,FILE_ATTRIBUTE_ARCHIVE);
      if ( rc == 0 )
	{
	  rc = S_OS_FILEATTRFAILED;
	}
    }
  else
    {
      rc = SetFileAttributes(file,
			     FILE_ATTRIBUTE_ARCHIVE|FILE_ATTRIBUTE_READONLY);
      if ( rc == 0 )
	{
	  rc = S_OS_FILEATTRFAILED;
	}
    }
# endif
# ifdef UNIX
# endif
  return rc;
}


int
_OsCopyFile(char *file,char *dest)
{
  int rc = S_OS_OK;
  if ( file && dest )
    {
#     ifdef OS2
      if ( DosCopy(file,dest,DCPY_EXISTING) != NO_ERROR )
	{
	  rc = S_OS_FILECOPYFAILED;
	}
#     endif
#     ifdef WIN32
      if ( !CopyFile(file,dest,FALSE) )
	{
	  rc = S_OS_FILECOPYFAILED;
	}
#     endif
#     ifdef UNIX
      rc = S_OS_FILECOPYFAILED;
      FILE *rfptr = fopen(file,"rb");
      if ( rfptr )
	{
	  FILE *wfptr = fopen(dest,"wb");
	  if ( wfptr )
	    {
	      int bytesRead = 0;
	      char buffer[1024];
	      do
		{
		  bytesRead = fread(buffer,1,1024,rfptr);
		  if ( bytesRead > 0 )
		    {
		      fwrite(buffer,1,bytesRead,wfptr);
		    }
		} while ( bytesRead > 0 );
	      
              rc = S_OS_OK;
	      
	      fclose(wfptr);
	    }
	  fclose(rfptr);
	}
#     endif
    }
  return rc;
}


int
_OsDeleteSelf(char *filename,char *dirname)
{
# ifdef WIN32
  FILE *fptr = fopen("deleteme.bat","w");
  if ( fptr )
    {
      fprintf(fptr,"rem @echo off\n");
      fprintf(fptr,":tryfile\n");
      fprintf(fptr,"del \"%s\"\n",filename);
      fprintf(fptr,"if exist \"%s\" goto tryfile\n",filename);
      if ( dirname )
	{
	  fprintf(fptr,":trydir\n");
	  fprintf(fptr,"rd \"%s\"\n",dirname);
	  fprintf(fptr,"if exist \"%s\" goto dirfile\n",dirname);
	}
      fprintf(fptr,"del deleteme.bat\n");
      fclose(fptr);
    }
  
  STARTUPINFO si;
  PROCESS_INFORMATION pi;
  ZeroMemory(&si,sizeof(STARTUPINFO));
  ZeroMemory(&pi,sizeof(PROCESS_INFORMATION));
  si.cb = sizeof(STARTUPINFO);
  si.dwFlags = STARTF_USESHOWWINDOW;
  si.wShowWindow = SW_HIDE;
  if ( CreateProcess(NULL,"deleteme.bat",NULL,NULL,FALSE,
		     IDLE_PRIORITY_CLASS,NULL,NULL,&si,&pi) )
    {
      CloseHandle(pi.hThread);
      CloseHandle(pi.hProcess);
    }
# endif
  return 0;
}


int
_OsDeleteFile(char *filename)
{
  int rc = S_OS_OK;
  if ( filename )
    {
#     ifdef OS2
      if ( DosDelete(filename) != NO_ERROR )
	{
	  rc = S_OS_FILEDELETEFAILED;
	}
#     endif
#     ifdef WIN32
      if ( !DeleteFile(filename) )
	{
	  rc = S_OS_FILEDELETEFAILED;
	}
      SHChangeNotify(SHCNE_DELETE,SHCNF_PATH,filename,NULL);
#     endif
    }
  return rc;
}


int
_OsRemoveDir(char *filename)
{
  int rc = S_OS_OK;
  if ( filename )
    {
#     ifdef OS2
      if ( DosDeleteDir(filename) != NO_ERROR )
	{
	  rc = S_OS_DIRDELETEFAILED;
	}
#     endif
#     ifdef WIN32
      if ( !RemoveDirectory(filename) )
	{
	  rc = S_OS_DIRDELETEFAILED;
	}
      SHChangeNotify(SHCNE_RMDIR,SHCNF_PATH,filename,NULL);
#     endif
    }
  return rc;
}


int
_OsGetProcessID()
{
# ifdef OS2
  PTIB ptib;
  PPIB ppib;
  DosGetInfoBlocks(&ptib,&ppib);
  return ppib->pib_ulpid;
# endif
# ifdef WIN32
  return GetCurrentProcessId();
# endif
# ifdef UNIX
  return getpid();
# endif
}


int
_OsGetThreadID()
{
# ifdef OS2
  PTIB ptib;
  PPIB ppib;
  DosGetInfoBlocks(&ptib,&ppib);
  return ptib->tib_ptib2->tib2_ultid;
# endif
# ifdef WIN32
  return GetCurrentThreadId();
# endif
# ifdef UNIX
  return getpid();
# endif
}


int
_OsRunProcess(char *exe,char *par)
{
  if ( exe )
    {
      char *command;
      int len = strlen(exe)+strlen(par)+4;
      command = (char *)S_malloc(len*sizeof(char));
      if ( command )
	{
#         ifdef OS2
	  RESULTCODES rescResults;
	  char achFailName[128];
	  sprintf(command,"%s%c%s%c%c",exe,0,par,0,0);
	  DosExecPgm(achFailName,128,EXEC_BACKGROUND,
		     command,NULL,&rescResults,exe);
#         endif
#         ifdef WIN32
	  STARTUPINFO si;
	  PROCESS_INFORMATION pi;
	  ZeroMemory(&si,sizeof(STARTUPINFO));
	  ZeroMemory(&pi,sizeof(PROCESS_INFORMATION));
	  si.cb = sizeof(STARTUPINFO);
	  sprintf(command,"%s %s",exe,par);
	  CreateProcess(NULL,command,NULL,NULL,FALSE,
			DETACHED_PROCESS | CREATE_NEW_PROCESS_GROUP,
			NULL,NULL,&si,&pi);
#         endif
	  S_free(command);
	}
    }
  return 0;
}


int
_OsMinorVersion(void)
{
# ifdef OS2
  return 0;
# endif
# ifdef WIN32
  return (GetVersion() & 0xFF00) >> 8;
# endif
# ifdef LINUX
  return 2;
# endif
  return 0;
}


int
_OsVersion(void)
{
# ifdef OS2
  return 4;
# endif
# ifdef WIN32
  return GetVersion() & 0xFF;
# endif
# ifdef LINUX
  return 2;
# endif
  return 0;
}


int
_OsType(void)
{
# ifdef OS2
  return S_OS_OS2;
# endif
# ifdef WIN32
  DWORD ver = GetVersion();
  if ( ver>>31 == 0 )
    return S_OS_WINNT;
  else
    return S_OS_WIN95;
# endif
# ifdef LINUX
  return S_OS_LINUX;
# endif
  return S_OS_UNKNOWN;
}


char *
_OsString(void)
{
  static char osString[6];
# ifdef OS2
  strcpy(osString,"OS/2");
# endif
# ifdef WIN32
  
  strcpy(osString,"MSWin");
  
  if ( _OsType() == S_OS_WINNT )
    {
      
      //
      // Windows NT
      //
      if ( _OsVersion() <= 4 )
	{
	  strcpy(osString,"WinNT");
	}
      
      //
      // Windows 2000 and XP (2001)
      //
      if ( _OsVersion() > 4 )
	{
	  
	  switch ( _OsMinorVersion() )
	    {
	    case 0 :
	      strcpy(osString,"Win2k");
	      break;
	    case 1 :
	      strcpy(osString,"WinXP");
	      break;
	    case 2 :
	      strcpy(osString,"Win2003");
	      break;
	    default :
	      strcpy(osString,">Win2003");
	    }
	}
      
    }
  
  if ( _OsType() == S_OS_WIN95 && _OsVersion() >= 4 )
    {
      
      if ( _OsMinorVersion() == 0 )
	{
	  strcpy(osString,"Win95");
	}
      
      if ( _OsMinorVersion() > 0 && _OsMinorVersion() < 90 )
	{
	  strcpy(osString,"Win98");
	}
      
      if ( _OsMinorVersion() >= 90 )
	{
	  strcpy(osString,"WinMe");
	}
      
    }
  
# endif
# ifdef LINUX
  strcpy(osString,"Linux");
# endif
  return osString;
}


static int _tcpipEnabled = FALSE;

int
_OsTcpipInit(void)
{
  
  if ( _tcpipEnabled )
    return 0;
  
  int rc = 0;
  /* *** Initialize TCP/IP stacks *** */
# if defined(OS2) && defined(IBMTCPIP)
  rc = sock_init();
# endif
# ifdef WIN32
  WSADATA stWSAData;
  rc = WSAStartup(WSA_VERSION, &stWSAData);
# endif
# ifdef UNIX
  signal(SIGPIPE, SIG_IGN);
  rc = 0;
# endif
  if ( rc != 0 )
    return -1;
  
  _tcpipEnabled = TRUE;
  return 0;
}


char *
_OsROT13(char *str)
{

  if ( !str )
    {
      return str;
    }

  for ( int i=0; i<(int)strlen(str); i++ )
    {
      //
      // Upper case
      //
      if ( str[i] >= 65 && str[i] <= 90 )
	{
	  int index = str[i] - 65;
	  index = (index + 13) % 26;
	  str[i] = index + 65;
	}

      //
      // Lower case
      //
      if ( str[i] >= 97 && str[i] <= 122 )
	{
	  int index = str[i] - 97;
	  index = (index + 13) % 26;
	  str[i] = index + 97;
	}
    }

  return str;
}
