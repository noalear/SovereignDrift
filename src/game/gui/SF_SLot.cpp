#include "sf.h"
#include "SF_PortCpp.h"
#include "SF_Externals.h"
#include "SF_Slot.h"
#include "SF_Player.h"
#include "SF_GameEngine.h"


int
SF_Slot::Init() {
   filled = FALSE;
   player = NULL;
   lagCount = 0;
   kicked = FALSE;
   serial = 0;
   continueConnection = FALSE;
   kickReason.Create("No reason");
   ResetLagHistory();
   return 0;
   }


SF_Slot::SF_Slot(void) {
   Init();
   }

SF_Slot::~SF_Slot(void) {
   if ( player ) {
      delete player;
      player = NULL;
      }
   }

int
SF_Slot::ResetLagHistory()
{
  for ( int i=0; i<SF_SLOT_NUMLAGHIST; i++ )
    {
      lagHist[i] = 0;
    }
  return 0;
}


int
SF_Slot::SetPlayer(SF_Player *iplayer) {
   if ( player )
      delete player;
   player = new SF_Player(iplayer->playerName.str,iplayer->password.str,
                          iplayer->shipName.str,
                          iplayer->shipID,iplayer->kills,iplayer->losses,
                          iplayer->score,iplayer->rank,iplayer->race,
                          iplayer->timeOnline,iplayer->demo,
			  iplayer->credits,iplayer->serial);
   return 0;
   }



int
SF_Slot::StartBandWidth() {
   sw.Start();
   tempWriteBuffer.tcpBytesSent = 0;
   tempWriteBuffer.tcpBytesRcvd = 0;
   readBuffer.tcpBytesSent = 0;
   readBuffer.tcpBytesRcvd = 0;
   tempWriteBuffer.udpBytesSent = 0;
   tempWriteBuffer.udpBytesRcvd = 0;
   readBuffer.udpBytesSent = 0;
   readBuffer.udpBytesRcvd = 0;
   return 0;
   }


int
SF_Slot::SetLag(int lag) {
   lagHist[lagCount++] = (char)lag;
   lagCount = lagCount % SF_SLOT_NUMLAGHIST;
   return 0;
   }


int
SF_Slot::GetMinLag() {
   int minLag = 100;
   for ( int i=0; i<SF_SLOT_NUMLAGHIST; i++ )
      if ( lagHist[i] < minLag )
         minLag = lagHist[i];
   return minLag;
   }

ulong 
SF_Slot::GetBandWidth(ulong flag) {
   ulong bytes = 0;

   if ( flag & S_CLIENT_TCP ) {
      if ( flag & SF_SLOT_BYTESSENT )
         bytes += tempWriteBuffer.tcpBytesSent;
      if ( flag & SF_SLOT_BYTESRCVD )
         bytes += readBuffer.tcpBytesRcvd;
      }
   if ( flag & S_CLIENT_UDP ) {
      if ( flag & SF_SLOT_BYTESSENT )
         bytes += tempWriteBuffer.udpBytesSent;
      if ( flag & SF_SLOT_BYTESRCVD )
         bytes += readBuffer.udpBytesRcvd;
      }

   sw.Stop();
   return bytes/(sw.dtSec+1);
   }

