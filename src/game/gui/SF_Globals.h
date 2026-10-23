#ifndef SF_GLOBALS_INCLUDED
#define SF_GLOBALS_INCLUDED

class SF_Globals {
public:
   ushort numPlayers;
   ushort maxSlots;
   char *serverName;

   int osType;
   int osVersion;
   int stopWatchType;

   SF_Globals();
   ~SF_Globals();
   };

#endif
