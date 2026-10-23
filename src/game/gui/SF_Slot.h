/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
#ifndef SF_SLOT_INCLUDED
#define SF_SLOT_INCLUDED

#define SF_SLOT_SERVERFULL   255
#define SF_SLOT_WRONGVERSION 254
#define SF_SLOT_OLDVERSION   253

#define SF_SLOT_BYTESSENT 16
#define SF_SLOT_BYTESRCVD 32

#define SF_SLOT_NUMLAGHIST 10

#include "SF_MsgBuffer.h"

class SF_Player;
class SF_GameEngine;

class SF_Slot {
public:
   char filled;
   char reserved;

   int continueConnection;
   int kicked;

   slong serial;

   S_String kickReason;
   S_String ipAddress;

   S_StopWatch sw;

   int SetLag(int);
   int GetMinLag();

   int lagCount;
   char lagHist[SF_SLOT_NUMLAGHIST];

   int ResetLagHistory();

   SF_MsgBuffer readBuffer;
   SF_MsgBuffer writeBuffer;
   SF_MsgBuffer tempWriteBuffer;

   /* *** Server Info *** */
   ulong frameDtUsec;
   char serverMajorVersion;
   char serverMinorVersion;
   char serverName[80];
   char serverIPAddress[80];

   /* *** Client Info *** */
   char clientMajorVersion;
   char clientMinorVersion;

   /* *** Player Info *** */
   SF_Player *player;
   int SetPlayer(SF_Player *);

   int Init();
   SF_Slot(void);
   ~SF_Slot(void);

   ulong GetBandWidth(ulong);
   int StartBandWidth();
   };

#endif
