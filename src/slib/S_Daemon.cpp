/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
#include "S_OS.h"
#include "S_Daemon.h"
#include "S_Directory.h"
#include "S_Process.h"

S_Daemon *_daemon = NULL;


int
S_Daemon::Init()
{
  rc = S_DAEMON_OK;
  return 0;
}


S_Daemon::S_Daemon()
{
  _daemon = this;
  Init();
}


int
S_Daemon::Create(char *iname,char *ilabel)
{
  name.Create(iname);
  label.Create(ilabel);
  return 0;
}


int
S_Daemon::Destroy()
{
  return 0;
}


S_Daemon::~S_Daemon() {
   Destroy();
   }


int
S_Daemon::Main(int,char *[])
{
  return 0;
}


int
S_Daemon::UpdateStatus(slong state)
{
#  ifdef WIN32
  if ( _OsType() == S_OS_WINNT )
    {
      SERVICE_STATUS status;
      status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
      status.dwCurrentState = state;
      status.dwControlsAccepted = SERVICE_ACCEPT_STOP |
	SERVICE_ACCEPT_PAUSE_CONTINUE |
	SERVICE_ACCEPT_SHUTDOWN;
      if ( state == SERVICE_START_PENDING )
	status.dwControlsAccepted = 0;
      status.dwServiceSpecificExitCode = 0;
      status.dwWin32ExitCode = 0;
      status.dwCheckPoint = 0;
      status.dwWaitHint = 5000;
      SetServiceStatus((SERVICE_STATUS_HANDLE)hStatus,&status);
    }
#  endif
  return 0;
}


int
S_Daemon::HandlePauseMsg()
{
  return 0;
}


int
S_Daemon::HandleContinueMsg()
{
  return 0;
}


int
S_Daemon::HandleInterrogateMsg()
{
  return 0;
}


int
S_Daemon::HandleShutdownMsg()
{
  return 0;
}


int
S_Daemon::HandleStopMsg()
{
  return 0;
}


void
S_Daemon_ServiceCtrlHandler(ulong code)
{
# ifdef WIN32
  if ( _OsType() == S_OS_WINNT )
    {
      switch ( code )
	{
	case SERVICE_CONTROL_PAUSE :
	  _daemon->HandlePauseMsg();
	  break;
	case SERVICE_CONTROL_CONTINUE :
	  _daemon->HandleContinueMsg();
	  break;
	case SERVICE_CONTROL_INTERROGATE :
	  _daemon->HandleInterrogateMsg();
	  break;
	case SERVICE_CONTROL_SHUTDOWN :
	  _daemon->HandleShutdownMsg();
	  break;
	case SERVICE_CONTROL_STOP :
	  _daemon->HandleStopMsg();
	  break;
	}
    }
#  endif
}


void
S_Daemon_ServiceMain(int argc,char *argv[])
{
# ifdef WIN32
  if ( _OsType() == S_OS_WINNT )
    {
      if ( _daemon )
	{
	  _daemon->hStatus = (ulong)
	    RegisterServiceCtrlHandler(_daemon->name.str,
				       (LPHANDLER_FUNCTION)
				       S_Daemon_ServiceCtrlHandler);
	  if ( _daemon->hStatus )
	    {
	      _daemon->UpdateStatus(SERVICE_START_PENDING);
	      _daemon->Main(argc,argv);
	    }
	}
    }
# endif
}


int
S_Daemon::StartUp()
{
# ifdef WIN32
      if ( _OsType() == S_OS_WINNT )
	{
	  SERVICE_TABLE_ENTRY serviceTable[] =
	  {
            {name.str,(LPSERVICE_MAIN_FUNCTION)S_Daemon_ServiceMain},
            {NULL,NULL}
	  };
	  rc = S_DAEMON_STARTUPFAILED;
	  if ( StartServiceCtrlDispatcher(serviceTable) )
            rc = S_DAEMON_OK;
	}
#  endif
      return rc;
}


int
S_Daemon::Install(char *path,char *args)
{
  char *command = (char *)S_malloc(strlen(path)+strlen(args)+2);
  if ( command && path && args )
    {
      sprintf(command,"%s %s",path,args);
#     ifdef WIN32
      if ( _OsType() == S_OS_WINNT )
	{
	  rc = S_DAEMON_SCMOPENFAILED;
	  SC_HANDLE hSCM = OpenSCManager(0,0,SC_MANAGER_CREATE_SERVICE);
	  if ( hSCM )
	    {
	      rc = S_DAEMON_CREATESERVICEFAILED;
	      SC_HANDLE hService = CreateService(hSCM,
						 name.str,
						 label.str,
						 SERVICE_ALL_ACCESS,
						 SERVICE_WIN32_OWN_PROCESS,
						 SERVICE_AUTO_START,
						 SERVICE_ERROR_NORMAL,
						 command,
						 0,0,0,0,0);
	      if ( hService )
		{
                  rc = S_DAEMON_OK;
                  StartService(hService,0,NULL);
                  CloseServiceHandle(hService);
		}
	    }
	  CloseServiceHandle(hSCM);
	}
      if ( _OsType() == S_OS_WIN95 )
	{
	  char *name = "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run";
	  HKEY hkey  = HKEY_LOCAL_MACHINE;
	  
	  HKEY hnkey;
	  RegOpenKey(hkey,name,&hnkey);
	  RegSetValueEx(hnkey,label.str,0,REG_SZ,(BYTE *)command,
			strlen(command)+1);
	  
	  char exedir[1024];
	  strcpy(exedir,command);
	  
	  int i = strlen(exedir);
	  int foundEnd = FALSE;
	  while ( !foundEnd ) {
	    if ( i <= 0 )
	      foundEnd = TRUE;
	    if ( !foundEnd && exedir[i] == '\\' ) {
	      exedir[i] = 0;
	      foundEnd = TRUE;
	    }
	    i--;
	  }
	  
	  S_Directory home;
	  S_Directory dir(exedir);
	  dir.ChangeTo();
	  S_Process process;
	  process.Start(path,args);
	  home.ChangeTo();
	  
	}
#     endif
      S_free(command);
    }
  return rc;
}


int
S_Daemon::Uninstall()
{
# ifdef WIN32
  if ( _OsType() == S_OS_WINNT )
    {
      rc = S_DAEMON_SCMOPENFAILED;
      SC_HANDLE hSCM = OpenSCManager(0,0,SC_MANAGER_CREATE_SERVICE);
      if ( hSCM )
	{
	  rc = S_DAEMON_OPENSERVICEFAILED;
	  SC_HANDLE hService = OpenService(hSCM,
					   name.str,
					   SERVICE_ALL_ACCESS | DELETE);
	  if ( hService )
	    {
	      rc = S_DAEMON_UNINSTALLFAILED;
	      SERVICE_STATUS status;
	      if ( QueryServiceStatus(hService,&status) )
		{
                  rc = S_DAEMON_OK;
                  if ( status.dwCurrentState != SERVICE_STOPPED )
		    ControlService(hService,SERVICE_CONTROL_STOP,&status);
                  DeleteService(hService);
		}
	      CloseServiceHandle(hService);
	    }
	  CloseServiceHandle(hSCM);
	}
    }
  if ( _OsType() == S_OS_WIN95 )
    {
      char *name = "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run";
      HKEY hkey  = HKEY_LOCAL_MACHINE;
      
      HKEY hnkey;
      RegOpenKey(hkey,name,&hnkey);
      RegDeleteValue(hnkey,label.str);
    }
#  endif
  return rc;
}
