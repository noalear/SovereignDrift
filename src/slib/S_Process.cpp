/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
#include "S_OS.h"
#include "S_Process.h"


S_Process::S_Process()
{
  pid = 0;
  handle = 0;
  flag = 0;
  exe = NULL;
  arg = NULL;
}


S_Process::S_Process(char *iexe,char *iarg,ulong iflag)
{
  flag = iflag;
  exe = NULL;
  arg = NULL;
  Start(iexe,iarg,iflag);
}


int
S_Process::Destroy()
{
  if ( flag != S_PROCESS_DETACHED )
    Stop();
# ifdef WIN32
  if ( handle )
    {
      CloseHandle(handle);
      handle = NULL;
    }
# endif
  if ( exe )
    {
      S_free(exe);
      exe = NULL;
    }
  if ( arg )
    {
      S_free(arg);
      arg = NULL;
    }
  return 0;
}


S_Process::~S_Process()
{
  Destroy();
}


int
S_Process::Start(char *iexe,char *iarg,ulong iflag)
{
  
  int rc = S_PROCESS_STARTFAILED;
  
  flag = iflag;
  
  if ( handle || pid )
    return 0;
  
  if ( exe )
    {
      S_free(exe);
      exe = NULL;
    }
   if ( iexe )
     exe = S_strdup(iexe);
   
   if ( arg )
     {
       S_free(arg);
       arg = NULL;
     }
   if ( iarg )
     arg = S_strdup(iarg);
   
   int len = 0;
   if ( exe )
     {
       len = strlen(exe)+4;
       if ( arg )
         len = strlen(exe)+strlen(arg)+4;
     }
   
   char *command = NULL;
   if ( len > 0 )
     command = (char *)S_malloc(len*sizeof(char));
   
   if ( command )
     {
#      ifdef OS2
       char achFailName[128];
       sprintf(command,"%s%c%c",exe,0,0);
       if ( arg )
	 sprintf(command,"%s%c%s%c%c",exe,0,arg,0,0);
       rc = DosExecPgm(achFailName,128,flag,command,NULL,&rescResults,exe);
       rc = S_PROCESS_OK;
#      endif
#      ifdef WIN32
       ZeroMemory(&si,sizeof(STARTUPINFO));
       ZeroMemory(&pi,sizeof(PROCESS_INFORMATION));
       si.cb = sizeof(STARTUPINFO);
       sprintf(command,"%s",exe);
       if ( arg )
	 sprintf(command,"%s %s",exe,arg);
       rc=CreateProcess(NULL,command,NULL,NULL,TRUE,flag,NULL,NULL,&si,&pi);
       CloseHandle(pi.hThread);
       handle = pi.hProcess;
       pid = pi.dwProcessId;
       if ( rc && handle != 0 && pid >= 0 )   /* port: was "handle > 0" */
	 rc = S_PROCESS_OK;
#      endif
       S_free(command);
     }
   return rc;
}


int
S_Process::Stop() {
# ifdef OS2
  if ( pid )
    {
      DosKillProcess(DKP_PROCESS,pid);
      handle = NULL;
      pid = 0;
    }
# endif
# ifdef WIN32
  if ( handle )
    {
      TerminateProcess(handle,0);
      CloseHandle(handle);
      handle = NULL;
      pid = 0;
    }
# endif
  return 0;
}


int
S_Process::WaitFor(slong timeOut)
{
# ifdef OS2
  if ( pid )
    {
      RESULTCODES rescResults;
      DosWaitChild(DCWA_PROCESS,DCWW_WAIT,&rescResults,&pid,pid);
    }
# endif
# ifdef WIN32
  if ( handle )
    {
      if ( timeOut == S_PROCESS_INFINITEWAIT )
	WaitForSingleObject(handle,timeOut);
      else
	WaitForSingleObject(handle,timeOut/1000);
      CloseHandle(handle);
    }
# endif
  handle = (HANDLE)NULL;
  pid = 0;
  return 0;
}
