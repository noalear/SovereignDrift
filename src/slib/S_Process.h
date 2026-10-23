/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
#ifndef S_PROCESS
#define S_PROCESS

#ifdef OS2
#define S_PROCESS_DETACHED     EXEC_BACKGROUND
#define S_PROCESS_ASYNCHRONOUS EXEC_ASYNCRESULT
#define S_PROCESS_INFINITEWAIT 0
typedef ulong HANDLE;
#endif
#ifdef WIN32
#define S_PROCESS_DETACHED     DETACHED_PROCESS
#define S_PROCESS_ASYNCHRONOUS 0
#define S_PROCESS_INFINITEWAIT INFINITE
typedef ulong PID;
#endif
#ifdef UNIX
#define S_PROCESS_DETACHED     0
#define S_PROCESS_ASYNCHRONOUS 0
#define S_PROCESS_INFINITEWAIT 0
typedef ulong PID;
typedef ulong HANDLE;
#endif

#define S_PROCESS_STARTFAILED -1
#define S_PROCESS_OK           0

class S_Process
{
 public:
  ulong flag;
  char *exe;
  char *arg;
  PID pid;
  HANDLE handle;
  
  int Start(char *,char * = NULL,ulong=S_PROCESS_DETACHED);
  int Suspend(void);
  int Resume(void);
  int Stop(void);
  int WaitFor(slong=S_PROCESS_INFINITEWAIT );
  int SetPriority(ulong);
  
# ifdef OS2
  RESULTCODES rescResults;
# endif
# ifdef WIN32
  STARTUPINFO si;
  PROCESS_INFORMATION pi;
# endif
  
  S_Process();
  S_Process(char *,char * = NULL,ulong=S_PROCESS_DETACHED);
  int Destroy();
  ~S_Process();
};

#endif
