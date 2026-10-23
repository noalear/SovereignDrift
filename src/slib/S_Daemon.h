/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
#ifndef S_DAEMON
#define S_DAEMON

#define S_DAEMON_OK                   0
#define S_DAEMON_SCMOPENFAILED       -1
#define S_DAEMON_CREATESERVICEFAILED -2
#define S_DAEMON_OPENSERVICEFAILED   -3
#define S_DAEMON_UNINSTALLFAILED     -4
#define S_DAEMON_STARTUPFAILED       -5

#ifdef OS2
#  define S_DAEMON_RUNNING 0
#endif
#ifdef WIN32
#  define S_DAEMON_RUNNING SERVICE_RUNNING
#endif
#ifdef UNIX
#  define S_DAEMON_RUNNING 0
#endif

#include "S_String.h"

class S_Daemon {
public:
   int rc;

   S_String name;
   S_String label;

   ulong hStatus;

   virtual int Main(int,char *[]);
   virtual int HandlePauseMsg();
   virtual int HandleContinueMsg();
   virtual int HandleInterrogateMsg();
   virtual int HandleShutdownMsg();
   virtual int HandleStopMsg();

   int StartUp();
   int Install(char *,char * ="");
   int Uninstall();

   int UpdateStatus(slong);

   int Init();
   S_Daemon();
   int Create(char *,char *);
   int Destroy();
   virtual ~S_Daemon();
   };

void S_Daemon_ServiceCtrlHandler(int);
void S_Daemon_ServiceMain(int,char *[]);

#endif
