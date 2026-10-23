#include "sf.h"
#include "SF_Globals.h"
#include "SF_PortCpp.h"

SF_Globals::SF_Globals() {
   numPlayers = 0;
   maxSlots = max_players;
   serverName = NULL;

   osType = _OsType();
   osVersion = _OsVersion();
   stopWatchType = S_STOPWATCH_HIGHRES;
   if ( osType == S_OS_WINNT || osType == S_OS_WIN95 )
     {
       stopWatchType = S_STOPWATCH_LOWRES;
     }
}


SF_Globals::~SF_Globals() {
   }
