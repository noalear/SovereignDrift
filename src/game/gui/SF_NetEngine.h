/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
#ifndef SF_NETENGINE_INCLUDED
#define SF_NETENGINE_INCLUDED

class SF_Client;
class SF_GameEngine;
class SF_Engine;
class SF_Player;
class SF_Slot;
class SF_Msg;

#define SF_NETENGINE_NUMLAGHISTORIES 10
#define SF_NETENGINE_MAXLAG          (1<<30)

class SF_NetEngine {
public:

   slong lagCounter;
   slong lagHistory[SF_NETENGINE_NUMLAGHISTORIES];
   slong driftHistory[SF_NETENGINE_NUMLAGHISTORIES];

   slong lag;
   slong drift;
   slong serverFrame;

   int fastDataMode;

   int ResetTimeout();
   int dontTimeout;
   slong lastTCPSent;
   slong lastUDPSent;
   slong lastTCPRcvd;
   slong lastUDPRcvd;

   ulong numGameStateBytesRcvd;
   ulong numGameStateMsgsRcvd;

   int HandlePingMsg(SF_Msg *);

   SF_Engine *e;
   SF_GameEngine *ge;
   SF_Client *c;

   SF_Player *nullPlayer;
   SF_Slot *slot;

   SF_NetEngine(SF_Engine *);
   ~SF_NetEngine();

   S_StopWatch sw;

   int Update();
   };

#endif
