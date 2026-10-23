#ifndef SF_SERVER_INCLUDED
#define SF_SERVER_INCLUDED

#define SF_SERVER_PORT      20001

/* *** Queries *** */
#define SF_SERVER_TOUCH        1
#define SF_SERVER_CONNECT      2
#define SF_SERVER_DOWNLOADFILE 3

#define SF_SERVER_MAXLAGMS  1000

#define SF_SERVER_NOGUI   0
#define SF_SERVER_GUI     1

/* *** Responses *** */
#define SF_SERVER_OK               0
#define SF_SERVER_FULL             1
#define SF_SERVER_FILEDOESNOTEXIST 2

#include "sf.h"
#include "SF_MsgBuffer.h"
#include "SF_Globals.h"

class SF_Slot;
class SF_GameEngine;

#include "SF_ServerWindow.h"
//#include "SF_FileServer.h"

class SF_Server : public S_FileServer,public SF_Globals {
public:
   int run;
   int everybodyGo;
   SF_Slot *slot;
   char *nexusName;
   char *altNexusName;
   int registerWithNexus;

   S_Mutex listMutex;
   S_StringArray list;
   S_StringArray denyReason;
   int LoadList(char * ="server.devs");
   int PlayerIsListed(char *,char *);

   int guiMode;
   int timeOutSec;
   int lagMax;
   int maxGameStateMsgs;
   int minGameStateMsgPeriodicitySec;
   int startInTray;

   S_Mutex slotMutex;
   S_Mutex logMutex;

   SF_Client client;
   SF_GameEngine *ge;
   S_Thread *thread;
   S_Thread *nexusVerifyThread;
   SF_MsgBuffer broadcastBuffer;

   int CreatePlayerList();
   S_String playerList;

   SF_ServerWindow window;
   S_Thread *runThread;
   int Run();
   int Start(char * =NULL);
   int BroadcastMsg(char *);
   int SendWarningMsg();
   int KickEverybodyOff();
   int KickPlayerOff(int);
   int Stop();

   int Init();
   SF_Server();
   int Create(char * =NULL,int=SF_SERVER_NOGUI,char * =NULL,int=SF_SERVER_PORT,
	  int=S_LOGGER_TIMESTAMP,char * =NULL,int=FALSE);
   int Destroy();
   ~SF_Server();

   virtual int NewClientDenied(S_Client *);
   int PlayerIsBanned(char *,char **);

   virtual int ProcessConnectMsg(S_Client *);
   int EngineThread();

   int RegisterWithNexus(char *,ulong,int=FALSE);

   int Log(const char *,...);
   int SLog(const char *,...);
   };

#endif

