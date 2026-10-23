#ifndef SF_ENGINE_INCLUDED
#define SF_ENGINE_INCLUDED

#define SF_ENGINE_OK 0
#define SF_ENGINE_CONNECTFAILED -1
#define SF_ENGINE_SERVERFULL    -2
#define SF_ENGINE_WRONGVERSION  -3
#define SF_ENGINE_OLDVERSION    -4
#define SF_ENGINE_LOSTCONTACT   -5
#define SF_ENGINE_READFAILED    -6
#define SF_ENGINE_WRITEFAILED   -7
#define SF_ENGINE_SERVERKICKED  -8

class SF_Client;
class SF_NetEngine;
class SF_GameEngine;
class SF_DisplayEngine;
class SF_SpriteList;

class SF_Engine {
public:
   int rc;

   ulong check;

   ulong startFrame;
   ulong localPlayerLoggedIn;

   S_String kickedMsg;

   SF_SpriteList *msl;
   S_Window *window;
   S_Thread *engineThread;
   int threadRunning;
   int run;
   int selfTermination;
   int stopFast;
   int won;

   int state;
   char *serverName;
   char *gameStateFilename;

   S_Timer *timer;
   S_StopWatch sw;

   SF_Client *client;
   SF_NetEngine *netEngine;
   SF_GameEngine *gameEngine;
   SF_DisplayEngine *displayEngine;

   int PushKeyboardMsg(int,int);

   SF_Engine(char *,SF_Client *,SF_SpriteList *,char * =NULL);
   ~SF_Engine();
   };

#endif
