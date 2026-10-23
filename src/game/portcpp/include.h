/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
#include <S_Lib.h>

#if defined(OS2) || defined(WIN32)
#  include "../gui/SF_Msg.h"
#  include "../gui/SF_MsgBuffer.h"
#  include "../gui/SF_VideoConsole.h"
#  include "../gui/SF_Globals.h"
#  include "../gui/SF_Engine.h"
#  include "../gui/SF_Client.h"
#endif
#ifdef UNIX
#  include "../gui/SF_Msg.h"
#  include "../gui/SF_MsgBuffer.h"
#  include "../gui/SF_VideoConsole.h"
#  include "../gui/SF_Globals.h"
#  include "../gui/SF_Engine.h"
#  include "../gui/SF_Client.h"
#endif

extern SF_Client *sf;

#include "enginepp.h"
#include "sound.h"
#include "trek.h"
#include "classes.h"

/* ************************************ */
/* ***** Externals from Doug's code *** */
/* ************************************ */
extern char frameRate;
extern slong frameNumber;
extern short localSlotNumber;
/* ************************************ */

/* dlh */
extern SF_MsgBuffer retMsgBuffer;
/* dlh */

extern short warningMessagePosted;
extern slong warningMessagePostedFrame;
extern short warningBottomMessagePosted;
extern slong warningBottomMessagePostedFrame;

extern short numPlanetRobotsSet;
extern short numPopRobotsSet;

extern int inServer;
extern char instanceName[];

extern char message[MESSAGE_BUFFER_SIZE][MESSAGE_STRING_LENGTH+1];
extern char messagePushed;
void SetScreenMessage( char *, int );

/* external objects */
extern GameStateClass*     game;

/* global functions */

/*
#define CALL_PUSH_GAME_OBJ(gb,gs,obj) { /*long n = */gb.Push(obj,gs); /*WRITE_SF_LOG(logFile << "pushing " #obj << " (" << n << " bytes)" << endl);*/ }
#define CALL_POP_GAME_OBJ(gb,gs,obj) { /*long n = */gb.Pop(obj,gs); /*WRITE_SF_LOG(logFile << "popping " #obj << " (" << n << " bytes)" << endl);*/ }
#define CALL_PUSH_OBJ(gb,obj) { /*long n = */gb.Push(obj); /*WRITE_SF_LOG(logFile << "pushing " #obj <<  " (" << n << " bytes)" << endl);*/ }
#define CALL_POP_OBJ(gb,obj) { /*long n = */gb.Pop(obj); /*WRITE_SF_LOG(logFile << "popping " #obj << " (" << n << " bytes)" << endl);*/ }
*/

/**** local inline functions ****/
inline slong IsServer() {
  return sf && sf->engine == NULL;
}

inline slong IsLocalPlayer(ShipClass* s) {
return short(s->Type() == ship && s - game->ship == localSlotNumber);
}

inline slong IsConnectedClient() {
return sf && sf->engine && sf->engine->state == SF_CLIENT_CONNECTED;
}

inline slong IsDisconnectedClient() {
  return sf && sf->engine && sf->engine->state==SF_CLIENT_SINGLEPLAYER;
}

char* Format(slong);
