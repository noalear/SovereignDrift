/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
#include "sf.h"
#include "SF_MsgBuffer.h"
#include "SF_Player.h"
#include "SF_Msg.h"

SF_MsgBuffer::SF_MsgBuffer(void) {
   udpBytesSent = 0;
   tcpBytesSent = 0;
   udpBytesRcvd = 0;
   tcpBytesRcvd = 0;
   }

SF_MsgBuffer::~SF_MsgBuffer(void) {
   }


int
SF_MsgBuffer::SeekEnd() {
   udpPacket.index = udpPacket.packetSize;
   tcpPacket.index = tcpPacket.packetSize;
   return SF_MSG_OK;
   }


int
SF_MsgBuffer::ShrinkBuffers() {
   udpPacket.ShrinkBuffer();
   tcpPacket.ShrinkBuffer();
   return SF_MSG_OK;
   }


int
SF_MsgBuffer::Rewind() {
   udpPacket.Rewind();
   tcpPacket.Rewind();
   return SF_MSG_OK;
   }


int
SF_MsgBuffer::Flush(int flag) {
   udpPacket.Flush(flag);
   tcpPacket.Flush(flag);
   return SF_MSG_OK;
   }


int
SF_MsgBuffer::IsEmpty() {
   return (tcpPacket.packetSize == tcpPacket.headerSize &&
           udpPacket.packetSize == udpPacket.headerSize);
   }


int
SF_MsgBuffer::FilterMsg(ushort islot, uchar imsg,int flag) {
   SF_Msg msg;
   Rewind();
   while ( GetNextMsg(islot,&msg,flag,imsg) );
   return 0;
   }


int
SF_MsgBuffer::GetNextMsg(slong islot,SF_Msg *pmsg,int flag,slong imsg,
                         int flush) {

  char *str = NULL;

   S_Packet *packet = &tcpPacket;
   if ( flag & S_CLIENT_UDP )
      packet = &udpPacket;

   int rc = FALSE;
   int numBytesToFlush = 0;
   while ( rc == FALSE && packet->index < packet->packetSize ) {
      pmsg->msg = packet->PeekUChar();
      numBytesToFlush = packet->index;
      switch (pmsg->msg) {
         case SF_MSG_DOWNLOADFILE :
            pmsg->msg = packet->PopChar();
            pmsg->slot = packet->PopUShort();
            str = packet->PopString();
	    if ( str )
	      {
		pmsg->str.Create(str);
		S_free(str);
		str = NULL;
	      }
            if ( (pmsg->msg == imsg || imsg < 0 ) && 
                 (pmsg->slot == islot || islot < 0) ) {
               pmsg->bSize = packet->PeekULong();
               pmsg->buffer = packet->PopBuffer();
               rc = TRUE;
	       }
            else {
               pmsg->bSize = packet->PopULong();
               packet->index += pmsg->bSize;
	       }
            break;

         case SF_MSG_SETTARGET :
         case SF_MSG_SETNAVTARGET :
         case SF_MSG_SETDUTY :
         case SF_MSG_SEEKTARGETLEFT :
         case SF_MSG_SEEKTARGETRIGHT :
         case SF_MSG_SEEKOBJECT :
         case SF_MSG_SEEKSHIP :
         case SF_MSG_SEEKCOLONY :
         case SF_MSG_SEEKPLANET :
         case SF_MSG_HELP :
         case SF_MSG_ATTACK :
            pmsg->msg = packet->PopChar();
            pmsg->slot = packet->PopUShort();
            pmsg->tslot = packet->PopUShort();
            if ( (pmsg->msg == imsg || imsg < 0) && 
                 (pmsg->slot == islot || islot < 0) )
               rc = TRUE;
            break;

         case SF_MSG_SPAWNCOLONY :
            pmsg->msg = packet->PopChar();
            pmsg->slot = packet->PopUShort();
            pmsg->dslot = packet->PopUShort();
            pmsg->bslot = packet->PopUShort();
            pmsg->race = packet->PopUShort();
            pmsg->pop1 = packet->PopLong();
            pmsg->pop2 = packet->PopLong();
            if ( (pmsg->msg == imsg || imsg < 0) && 
                 (pmsg->slot == islot || islot < 0) )
               rc = TRUE;
            break;

         case SF_MSG_UNDOCK :
         case SF_MSG_COMPLETEBUILD :
            pmsg->msg = packet->PopChar();
            pmsg->slot = packet->PopUShort();
            pmsg->dslot = packet->PopUShort();
            if ( (pmsg->msg == imsg || imsg < 0) && 
                 (pmsg->slot == islot || islot < 0) )
               rc = TRUE;
            break;

         case SF_MSG_SEEKPOSITION :
            pmsg->msg = packet->PopChar();
            pmsg->slot = packet->PopUShort();
            pmsg->frame = packet->PopChar();
            pmsg->x = packet->PopLong();
            pmsg->y = packet->PopLong();
            if ( (pmsg->msg == imsg || imsg < 0) && 
                 (pmsg->slot == islot || islot < 0) )
               rc = TRUE;
            break;

         case SF_MSG_FIRETUBES :
            pmsg->msg = packet->PopChar();
            pmsg->slot = packet->PopUShort();
            pmsg->frame = packet->PopChar();
            pmsg->angle = packet->PopChar();
            pmsg->mask = packet->PopUChar();
            pmsg->vx = packet->PopShort();
            pmsg->vy = packet->PopShort();
            pmsg->x = packet->PopLong();
            pmsg->y = packet->PopLong(); 
            if ( (pmsg->msg == imsg || imsg < 0) && 
                 (pmsg->slot == islot || islot < 0) )
               rc = TRUE;
            break;

         case SF_MSG_ADDTOPLAYERSCORE :
            pmsg->msg = packet->PopChar();
            pmsg->slot = packet->PopUShort();
            pmsg->dslot = packet->PopUShort();
            pmsg->bonus = packet->PopULong();
            pmsg->credit = packet->PopULong();
            if ( (pmsg->msg == imsg || imsg < 0) && 
                 (pmsg->slot == islot || islot < 0) )
               rc = TRUE;
            break;

         case SF_MSG_SETSHIPINDEX :
            pmsg->msg = packet->PopChar();
            pmsg->slot = packet->PopUShort();
            pmsg->dslot = packet->PopUShort();
            pmsg->index = packet->PopUShort();
            pmsg->flag = packet->PopUChar();
            pmsg->port = packet->PopUChar();
            if ( (pmsg->msg == imsg || imsg < 0) && 
                 (pmsg->slot == islot || islot < 0) )
               rc = TRUE;
            break;

            case SF_MSG_SETMODULEPOINTER :
         case SF_MSG_SETRACEINDEX :
         case SF_MSG_SETRANKINDEX :
            pmsg->msg = packet->PopChar();
            pmsg->slot = packet->PopUShort();
            pmsg->index = packet->PopChar();
            if ( (pmsg->msg == imsg || imsg < 0) && 
                 (pmsg->slot == islot || islot < 0) )
               rc = TRUE;
            break;

         case SF_MSG_CAPTURE :
         case SF_MSG_HUMANCAPTURE :
            pmsg->msg = packet->PopChar();
            pmsg->slot = packet->PopUShort();
            pmsg->dslot = packet->PopUShort();
            pmsg->index = packet->PopChar();
            if ( (pmsg->msg == imsg || imsg < 0) && 
                 (pmsg->slot == islot || islot < 0) )
               rc = TRUE;
            break;

         case SF_MSG_FILLSLOT :
            pmsg->msg = packet->PopChar();
            pmsg->slot = packet->PopUShort();
            pmsg->dslot = packet->PopUShort();
            pmsg->index = packet->PopUShort();
            pmsg->race = packet->PopUShort();
            pmsg->flag = packet->PopUShort();
            pmsg->port = packet->PopUShort();
            if ( (pmsg->msg == imsg || imsg < 0) && 
                 (pmsg->slot == islot || islot < 0) )
               rc = TRUE;
            break;

         case SF_MSG_SETPOPULATION :
            pmsg->msg = packet->PopChar();
            pmsg->slot = packet->PopUShort();
            pmsg->pop1 = packet->PopLong();
            pmsg->pop2 = packet->PopLong();
            pmsg->ai1 = packet->PopUChar();
            pmsg->ai2 = packet->PopUChar();
            pmsg->ai3 = packet->PopUChar();
            pmsg->ai4 = packet->PopUChar();
            pmsg->ai5 = packet->PopUChar();
            pmsg->ai6 = packet->PopUChar();
            pmsg->x = packet->PopLong();
            pmsg->y = packet->PopLong();
            if ( (pmsg->msg == imsg || imsg < 0) && 
                 (pmsg->slot == islot || islot < 0) )
               rc = TRUE;
            break;

         case SF_MSG_CLOAK :
            pmsg->msg = packet->PopChar();
            pmsg->slot = packet->PopUShort();
            pmsg->mp1 = packet->PopLong();
            pmsg->mp2 = packet->PopLong();
            if ( (pmsg->msg == imsg || imsg < 0) && 
                 (pmsg->slot == islot || islot < 0) )
               rc = TRUE;
            break;

         case SF_MSG_INITIATEWARP :
            pmsg->msg = packet->PopChar();
            pmsg->slot = packet->PopUShort();
            pmsg->dslot = packet->PopUShort();
            pmsg->ai1 = packet->PopUChar();
            if ( (pmsg->msg == imsg || imsg < 0) && 
                 (pmsg->slot == islot || islot < 0) )
               rc = TRUE;
            break;

         case SF_MSG_TERMINATEWARP :
            pmsg->msg = packet->PopChar();
            pmsg->slot = packet->PopUShort();
            pmsg->dslot = packet->PopUShort();
            pmsg->ai1 = packet->PopUChar();
            pmsg->x = packet->PopLong();
            pmsg->y = packet->PopLong();
            if ( (pmsg->msg == imsg || imsg < 0) && 
                 (pmsg->slot == islot || islot < 0) )
               rc = TRUE;
            break;

         case SF_MSG_ISDOCKED :
         case SF_MSG_HUMANISDOCKED :
            pmsg->msg = packet->PopChar();
            pmsg->slot = packet->PopUShort();
            pmsg->dslot = packet->PopUShort();
            pmsg->port = packet->PopUChar();
            if ( (pmsg->msg == imsg || imsg < 0) && 
                 (pmsg->slot == islot || islot < 0) )
               rc = TRUE;
            break;

         case SF_MSG_DOCK :
            pmsg->msg = packet->PopChar();
            pmsg->slot = packet->PopUShort();
            pmsg->dslot = packet->PopUShort();
            pmsg->port = packet->PopUChar();
            pmsg->mp1 = packet->PopULong();
            pmsg->mp2 = packet->PopULong();
            if ( (pmsg->msg == imsg || imsg < 0) && 
                 (pmsg->slot == islot || islot < 0) )
               rc = TRUE;
            break;

         case SF_MSG_DEACTIVATEDOCK :
            pmsg->msg = packet->PopChar();
            pmsg->slot = packet->PopUShort();
            pmsg->port = packet->PopUChar();
            if ( (pmsg->msg == imsg || imsg < 0) && 
                 (pmsg->slot == islot || islot < 0) )
               rc = TRUE;
            break;

         case SF_MSG_PLANETSIEGESUCCEEDS :
         case SF_MSG_RESETGAME :
            pmsg->msg = packet->PopChar();
            pmsg->slot = packet->PopUShort();
            pmsg->race = packet->PopLong();
            if ( (pmsg->msg == imsg || imsg < 0) && 
                 (pmsg->slot == islot || islot < 0) )
               rc = TRUE;
            break;


         case SF_MSG_REQUESTPOPULATION :
         case SF_MSG_REMOVECOLONY :
         case SF_MSG_UNCLOAK :
            pmsg->msg = packet->PopChar();
            pmsg->slot = packet->PopUShort();
            if ( (pmsg->msg == imsg || imsg < 0) && 
                 (pmsg->slot == islot || islot < 0) )
               rc = TRUE;
            break;

         case SF_MSG_DISCONNECT :
            pmsg->msg = packet->PopChar();
            pmsg->slot = packet->PopUShort();
            str = packet->PopString();
	    pmsg->str.Create("%s",str);
	    if ( str )
	      {
		S_free(str);
		str = NULL;
	      }
            if ( (pmsg->msg == imsg || imsg < 0) && 
                 (pmsg->slot == islot || islot < 0) )
               rc = TRUE;
            break;

         case SF_MSG_EXPLODEME :
         case SF_MSG_EXPLODESHIP :
         case SF_MSG_EXPLODECOLONY :
            pmsg->msg = packet->PopChar();
            pmsg->slot = packet->PopUShort();
            pmsg->dslot = packet->PopUShort();
            if ( (pmsg->msg == imsg || imsg < 0) && 
                 (pmsg->slot == islot || islot < 0) )
               rc = TRUE;
            break;
         case SF_MSG_FRAMERATE :
            pmsg->msg = packet->PopChar();
            pmsg->slot = packet->PopUShort();
            pmsg->frameRate = packet->PopChar();
            if ( (pmsg->msg == imsg || imsg < 0) && 
                 (pmsg->slot == islot || islot < 0) )
               rc = TRUE;
            break;
         case SF_MSG_UPDATERATE :
            pmsg->msg = packet->PopChar();
            pmsg->slot = packet->PopUShort();
            pmsg->frameRate = packet->PopChar();
            if ( (pmsg->msg == imsg || imsg < 0) && 
                 (pmsg->slot == islot || islot < 0) )
               rc = TRUE;
            break;
         case SF_MSG_REQUESTDEV :
            pmsg->msg = packet->PopChar();
            pmsg->slot = packet->PopUShort();
            pmsg->frameRate = packet->PopChar();
            if ( (pmsg->msg == imsg || imsg < 0) && 
                 (pmsg->slot == islot || islot < 0) )
               rc = TRUE;
            break;
         case SF_MSG_KEYPRESSED_4 :
         case SF_MSG_KEYRELEASED_4 :
            pmsg->msg = packet->PopChar();
            pmsg->slot = packet->PopUShort();
            pmsg->frame = packet->PopChar();
            pmsg->scancode = packet->PopChar();
            if ( (pmsg->msg == imsg || imsg < 0) && 
                 (pmsg->slot == islot || islot < 0) )
               rc = TRUE;
            break;
         case SF_MSG_PLAYERINFO : {
            pmsg->msg = packet->PopChar();
            pmsg->slot = packet->PopUShort();
            slong shipID = packet->PopULong();
            slong kills = packet->PopULong();
            slong losses = packet->PopULong();
            slong timeOnline = packet->PopULong();
            slong score = packet->PopULong();
            char rank = packet->PopChar();
            char race = packet->PopChar();
            char *playerName = packet->PopString();
            char *password = packet->PopString();
            char *shipName = packet->PopString();
            char demo = packet->PopChar();
            slong credits = packet->PopULong();
	    slong serial = packet->PopULong();
            if ( (pmsg->msg == imsg || imsg < 0 ) && 
                 (pmsg->slot == islot || islot < 0) ) {
	      pmsg->player.Create(playerName,password,shipName,
				  shipID,kills,losses,score,
				  rank,race,timeOnline,demo,credits,
				  serial);
	      rc = TRUE;
	    }
            S_free(playerName);
            S_free(password);
            S_free(shipName);
            break;
	    }
         case SF_MSG_GAMESTATE :
            pmsg->msg = packet->PopChar();
            pmsg->slot = packet->PopUShort();
            if ( (pmsg->msg == imsg || imsg < 0 ) && 
                 (pmsg->slot == islot || islot < 0) ) {
               pmsg->buffer = packet->PopBuffer();
               rc = TRUE;
	       }
            else {
               pmsg->bSize = packet->PopULong();
               packet->index += pmsg->bSize;
	       }
            break;

         case SF_MSG_GETPOWERUP :
            pmsg->msg = packet->PopChar();
            pmsg->slot = packet->PopUShort();
            pmsg->bslot = packet->PopUShort();
            pmsg->id = packet->PopLong();
            pmsg->mp1 = packet->PopLong();
            if ( (pmsg->msg == imsg || imsg < 0) && 
                 (pmsg->slot == islot || islot < 0) )
               rc = TRUE;
            break;
         case SF_MSG_PING_10 :
         case SF_MSG_PINGUDP :
         case SF_MSG_PINGTCP :
            pmsg->msg = packet->PopChar();
            pmsg->slot = packet->PopUShort();
            pmsg->frame = packet->PopLong();
            pmsg->itime = packet->PopLong();
            if ( (pmsg->msg == imsg || imsg < 0) && 
                 (pmsg->slot == islot || islot < 0) )
               rc = TRUE;
            break;
         case SF_MSG_OBJECTTRACK_17 :
            pmsg->msg = packet->PopChar();
            pmsg->slot = packet->PopUShort();
            pmsg->frame = packet->PopUChar();
            pmsg->angle = packet->PopUChar();
            pmsg->bitmap = packet->PopUChar();
            pmsg->vx = packet->PopShort();
            pmsg->vy = packet->PopShort();
            pmsg->x = packet->PopLong();
            pmsg->y = packet->PopLong();
            if ( (pmsg->msg == imsg || imsg < 0) && 
                 (pmsg->slot == islot || islot < 0) )
               rc = TRUE;
            break;
         case SF_MSG_AIOBJECTTRACK_17 :
            pmsg->msg = packet->PopChar();
            pmsg->slot = packet->PopUShort();
            pmsg->frame = packet->PopUChar();
            pmsg->angle = packet->PopUChar();
            pmsg->bitmap = packet->PopUChar();
            pmsg->vx = packet->PopShort();
            pmsg->vy = packet->PopShort();
            pmsg->x = packet->PopLong();
            pmsg->y = packet->PopLong();
            pmsg->ai1 = packet->PopUChar();
            pmsg->ai2 = packet->PopUChar();
            pmsg->ai3 = packet->PopUChar();
            pmsg->ai4 = packet->PopUChar();
            pmsg->dslot = packet->PopUShort();
            if ( (pmsg->msg == imsg || imsg < 0) && 
                 (pmsg->slot == islot || islot < 0) )
               rc = TRUE;
            break;
         case SF_MSG_PLAYERCHAT_N :
            pmsg->msg = packet->PopChar();
            pmsg->slot = pmsg->from = packet->PopUShort();
            pmsg->mode = packet->PopChar();
            pmsg->to = packet->PopChar();
            str = packet->PopString();
            if ( str )
	      {
		pmsg->str.Create(str);
		S_free(str);
		str = NULL;
	      }
            if ( (pmsg->msg == imsg || imsg < 0 ) && 
                 (pmsg->slot == islot || islot < 0) )
               rc = TRUE;
            break;
         case SF_MSG_MOUSEBUTTONDOWN_12 :
         case SF_MSG_MOUSEBUTTONUP_12 :
         case SF_MSG_REL_MOUSEBUTTONDOWN_12 :
         case SF_MSG_REL_MOUSEBUTTONUP_12 :
            pmsg->msg = packet->PopChar();
            pmsg->slot = packet->PopUShort();
            pmsg->frame = packet->PopChar();
            pmsg->scancode = packet->PopChar();
            pmsg->x = packet->PopLong();
            pmsg->y = packet->PopLong();
            if ( (pmsg->msg == imsg || imsg < 0 ) && 
                 (pmsg->slot == islot || islot < 0) )
               rc = TRUE;
            break;
         default :
            printf("**********************************************\n");
            printf("*** ERROR: Unknown network msg %ld\n",pmsg->msg);
            if ( packet == &tcpPacket )
               printf("***  The packet was sent TCP\n");
            else
               printf("*** The packet was sent UDP\n");
            printf("*** packet->packetSize:       %ld\n",packet->packetSize);
            printf("*** packet->uncompPacketSize: %ld\n",
                   packet->uncompPacketSize);
            printf("*** packet->index:            %ld\n",packet->index);
            printf("*** packet->buffer:             \n");
            //if ( packet->buffer )
            //   for ( i=0; i<(int)packet->packetSize; i++ )
            //      printf("%d ",packet->buffer[i]);
            //printf("\n");
            printf("Flushing malformed packet . . .\n");
            packet->Flush();
            printf("**********************************************\n");
            return rc;
	 }
      }

   if ( rc && flush ) {
      numBytesToFlush = packet->index - numBytesToFlush;
      if ( numBytesToFlush > 0 ) {
         packet->Flush(numBytesToFlush);
         }
      }

   return rc;
   }


int
SF_MsgBuffer::PushMsg(uchar msg,
                      ushort slot,
                      uchar frame,
                      uchar scancode,
                      slong x,
                      slong y,
                      int flag) {

   SeekEnd();
   S_Packet *packet = &tcpPacket;
   if ( flag & S_CLIENT_UDP )
      packet = &udpPacket;

   packet->PushUChar(msg);
   packet->PushUShort(slot); 
   packet->PushUChar(frame); 
   packet->PushUChar(scancode); 
   packet->PushLong(x);
   packet->PushLong(y);
   return SF_MSG_OK;
   }


int
SF_MsgBuffer::PushMsg(uchar msg,
                      ushort from,
                      uchar mode,
                      uchar to,
                      char *str,
                      int flag) {

   SeekEnd();
   S_Packet *packet = &tcpPacket;
   if ( flag & S_CLIENT_UDP )
      packet = &udpPacket;

   packet->PushUChar(msg);
   packet->PushUShort(from); 
   packet->PushUChar(mode); 
   packet->PushUChar(to); 
   packet->PushString(str);
   return SF_MSG_OK;
   }


int
SF_MsgBuffer::PushMsg(uchar msg,
                      ushort slot,
                      char *filename,
                      ulong numBytes,
                      char *buffer,
                      int flag) {

   SeekEnd();
   S_Packet *packet = &tcpPacket;
   if ( flag & S_CLIENT_UDP )
      packet = &udpPacket;

   packet->PushUChar(msg);
   packet->PushUShort(slot); 
   packet->PushString(filename);
   packet->PushBuffer(numBytes,buffer); 
   return SF_MSG_OK;
   }


int
SF_MsgBuffer::PushMsg(uchar msg,
                      ushort slot,
                      ushort dslot,
                      ushort index,
                      uchar bflag,
                      uchar port,
                      int flag) {

   SeekEnd();
   S_Packet *packet = &tcpPacket;
   if ( flag & S_CLIENT_UDP )
      packet = &udpPacket;

   packet->PushUChar(msg);
   packet->PushUShort(slot); 
   packet->PushUShort(dslot); 
   packet->PushUShort(index); 
   packet->PushUChar(bflag);
   packet->PushUChar(port);
   return SF_MSG_OK;
   }


int
SF_MsgBuffer::PushMsg(uchar msg,
                      ushort slot,
                      ushort dslot,
                      int flag) {

   SeekEnd();
   S_Packet *packet = &tcpPacket;
   if ( flag & S_CLIENT_UDP )
      packet = &udpPacket;

   packet->PushUChar(msg);
   packet->PushUShort(slot); 
   packet->PushUShort(dslot); 
   return SF_MSG_OK;
   }


int
SF_MsgBuffer::PushMsg(uchar msg,
                      ushort slot,
                      ushort dslot,
                      ushort bslot,
                      ushort race,
                      ulong pop1,
                      ulong pop2,
                      int flag) {

   SeekEnd();
   S_Packet *packet = &tcpPacket;
   if ( flag & S_CLIENT_UDP )
      packet = &udpPacket;

   packet->PushUChar(msg);
   packet->PushUShort(slot); 
   packet->PushUShort(dslot); 
   packet->PushUShort(bslot); 
   packet->PushUShort(race); 
   packet->PushULong(pop1);
   packet->PushULong(pop2);
   return SF_MSG_OK;
   }


int
SF_MsgBuffer::PushMsg(uchar msg,
                      ushort slot,
                      ushort dslot,
                      uchar port,
                      int flag) {

   SeekEnd();
   S_Packet *packet = &tcpPacket;
   if ( flag & S_CLIENT_UDP )
      packet = &udpPacket;

   packet->PushUChar(msg);
   packet->PushUShort(slot); 
   packet->PushUShort(dslot); 
   packet->PushUChar(port);
   return SF_MSG_OK;
   }


int
SF_MsgBuffer::PushMsg(uchar msg,
                      ushort slot,
                      ushort bslot,
                      ulong id,
                      int flag) {

   SeekEnd();
   S_Packet *packet = &tcpPacket;
   if ( flag & S_CLIENT_UDP )
      packet = &udpPacket;

   packet->PushUChar(msg);
   packet->PushUShort(slot); 
   packet->PushUShort(bslot); 
   packet->PushLong(id);
   return SF_MSG_OK;
   }


int
SF_MsgBuffer::PushMsg(uchar msg,
                      ushort slot,
                      ushort bslot,
                      slong id,
                      slong mp1,
                      int flag) {

   SeekEnd();
   S_Packet *packet = &tcpPacket;
   if ( flag & S_CLIENT_UDP )
      packet = &udpPacket;

   packet->PushUChar(msg);
   packet->PushUShort(slot); 
   packet->PushUShort(bslot); 
   packet->PushLong(id);
   packet->PushLong(mp1);
   return SF_MSG_OK;
   }


int
SF_MsgBuffer::PushMsg(uchar msg,
                      ushort slot,
                      ushort dslot,
                      ushort index,
                      ushort race,
                      ushort bflag,
                      ushort port,
                      int flag) {

   SeekEnd();
   S_Packet *packet = &tcpPacket;
   if ( flag & S_CLIENT_UDP )
      packet = &udpPacket;

   packet->PushUChar(msg);
   packet->PushUShort(slot);
   packet->PushUShort(dslot);
   packet->PushUShort(index); 
   packet->PushUShort(race);
   packet->PushUShort(bflag);
   packet->PushUShort(port);
   return SF_MSG_OK;
   }


int
SF_MsgBuffer::PushMsg(uchar msg,
                      ushort slot,
                      ushort dslot,
                      uchar port,
                      ulong mp1,
                      ulong mp2,
                      int flag) {

   SeekEnd();
   S_Packet *packet = &tcpPacket;
   if ( flag & S_CLIENT_UDP )
      packet = &udpPacket;

   packet->PushUChar(msg);
   packet->PushUShort(slot); 
   packet->PushUShort(dslot); 
   packet->PushUChar(port);
   packet->PushULong(mp1);
   packet->PushULong(mp2);
   return SF_MSG_OK;
   }


int
SF_MsgBuffer::PushMsg(uchar msg,
                      ushort slot,
                      ushort dslot,
                      uchar ai1,
                      slong x,
                      slong y,
                      int flag) {

   SeekEnd();
   S_Packet *packet = &tcpPacket;
   if ( flag & S_CLIENT_UDP )
      packet = &udpPacket;

   packet->PushUChar(msg);
   packet->PushUShort(slot); 
   packet->PushUShort(dslot); 
   packet->PushUChar(ai1);
   packet->PushLong(x);
   packet->PushLong(y);
   return SF_MSG_OK;
   }


int
SF_MsgBuffer::PushMsg(uchar msg,
                      ushort objectIndex,
                      uchar frame,
                      uchar angle,
                      uchar bitmapIndex,
                      short vx,
                      short vy,
                      slong x,
                      slong y,
                      int flag) {

   SeekEnd();
   S_Packet *packet = &tcpPacket;
   if ( flag & S_CLIENT_UDP )
      packet = &udpPacket;

   packet->PushUChar(msg);
   packet->PushUShort(objectIndex);
   packet->PushUChar(frame);
   packet->PushUChar(angle);
   packet->PushUChar(bitmapIndex);
   packet->PushShort(vx);
   packet->PushShort(vy);
   packet->PushLong(x);
   packet->PushLong(y);
   return SF_MSG_OK;
   }


int
SF_MsgBuffer::PushMsg(uchar msg,
                      ushort objectIndex,
                      uchar frame,
                      uchar angle,
                      uchar bitmapIndex,
                      short vx,
                      short vy,
                      slong x,
                      slong y,
                      ushort crew,
                      int flag) {

   SeekEnd();
   S_Packet *packet = &tcpPacket;
   if ( flag & S_CLIENT_UDP )
      packet = &udpPacket;

   packet->PushUChar(msg);
   packet->PushUShort(objectIndex);
   packet->PushUChar(frame);
   packet->PushUChar(angle);
   packet->PushUChar(bitmapIndex);
   packet->PushShort(vx);
   packet->PushShort(vy);
   packet->PushLong(x);
   packet->PushLong(y);
   packet->PushUShort(crew);
   return SF_MSG_OK;
   }


int
SF_MsgBuffer::PushMsg(uchar msg,
                      ushort objectIndex,
                      uchar frame,
                      uchar angle,
                      uchar bitmapIndex,
                      short vx,
                      short vy,
                      slong x,
                      slong y,
                      uchar ai1,
                      uchar ai2,
                      uchar ai3,
                      uchar ai4,
                      ushort dslot,
                      int flag) {

   SeekEnd();
   S_Packet *packet = &tcpPacket;
   if ( flag & S_CLIENT_UDP )
      packet = &udpPacket;

   packet->PushUChar(msg);
   packet->PushUShort(objectIndex);
   packet->PushUChar(frame);
   packet->PushUChar(angle);
   packet->PushUChar(bitmapIndex);
   packet->PushShort(vx);
   packet->PushShort(vy);
   packet->PushLong(x);
   packet->PushLong(y);
   packet->PushUChar(ai1);
   packet->PushUChar(ai2);
   packet->PushUChar(ai3);
   packet->PushUChar(ai4);
   packet->PushUShort(dslot);
   return SF_MSG_OK;
   }


int
SF_MsgBuffer::PushMsg(uchar msg,
                      ushort objectIndex,
                      uchar frame,
                      uchar angle,
                      uchar bitmapIndex,
                      short vx,
                      short vy,
                      slong x,
                      slong y,
                      ushort crew,
                      uchar ai1,
                      uchar ai2,
                      uchar ai3,
                      uchar ai4,
                      ushort dslot,
                      int flag) {

   SeekEnd();
   S_Packet *packet = &tcpPacket;
   if ( flag & S_CLIENT_UDP )
      packet = &udpPacket;

   packet->PushUChar(msg);
   packet->PushUShort(objectIndex);
   packet->PushUChar(frame);
   packet->PushUChar(angle);
   packet->PushUChar(bitmapIndex);
   packet->PushShort(vx);
   packet->PushShort(vy);
   packet->PushLong(x);
   packet->PushLong(y);
   packet->PushUShort(crew);
   packet->PushUChar(ai1);
   packet->PushUChar(ai2);
   packet->PushUChar(ai3);
   packet->PushUChar(ai4);
   packet->PushUShort(dslot);
   return SF_MSG_OK;
   }


int
SF_MsgBuffer::PushMsg(uchar msg,
                      ushort objectIndex,
                      uchar frame,
                      uchar angle,
                      uchar bitmapIndex,
                      short vx,
                      short vy,
                      slong x,
                      slong y,
                      ulong pop1,
                      ulong pop2,
                      uchar ai1,
                      uchar ai2,
                      uchar ai3,
                      uchar ai4,
                      ushort dslot,
                      int flag) {

   SeekEnd();
   S_Packet *packet = &tcpPacket;
   if ( flag & S_CLIENT_UDP )
      packet = &udpPacket;

   packet->PushUChar(msg);
   packet->PushUShort(objectIndex);
   packet->PushUChar(frame);
   packet->PushUChar(angle);
   packet->PushUChar(bitmapIndex);
   packet->PushShort(vx);
   packet->PushShort(vy);
   packet->PushLong(x);
   packet->PushLong(y);
   packet->PushULong(pop1);
   packet->PushULong(pop2);
   packet->PushUChar(ai1);
   packet->PushUChar(ai2);
   packet->PushUChar(ai3);
   packet->PushUChar(ai4);
   packet->PushUShort(dslot);
   return SF_MSG_OK;
   }



int
SF_MsgBuffer::PushMsg(uchar msg,
                      ushort objectIndex,
                      int flag) {

   SeekEnd();
   S_Packet *packet = &tcpPacket;
   if ( flag & S_CLIENT_UDP )
      packet = &udpPacket;

   packet->PushUChar(msg);
   packet->PushUShort(objectIndex); 
   return SF_MSG_OK;
   }


int
SF_MsgBuffer::PushMsg(uchar msg,
                      ushort slot,
                      char *str,
                      int flag) {

   SeekEnd();
   S_Packet *packet = &tcpPacket;
   if ( flag & S_CLIENT_UDP )
      packet = &udpPacket;

   packet->PushUChar(msg);
   packet->PushUShort(slot); 
   packet->PushString(str);
   return SF_MSG_OK;
   }


int
SF_MsgBuffer::PushMsg(uchar msg,
                      ushort objectIndex,
                      uchar frameRate,
                      int flag) {

   SeekEnd();
   S_Packet *packet = &tcpPacket;
   if ( flag & S_CLIENT_UDP )
      packet = &udpPacket;

   packet->PushUChar(msg);
   packet->PushUShort(objectIndex); 
   packet->PushUChar(frameRate);
   return SF_MSG_OK;
   }


int
SF_MsgBuffer::PushMsg(uchar msg,
                      ushort objectIndex,
                      ulong numBytes,
                      char *buffer,
                      int flag) {

   SeekEnd();
   S_Packet *packet = &tcpPacket;
   if ( flag & S_CLIENT_UDP )
      packet = &udpPacket;

   packet->PushUChar(msg);
   packet->PushUShort(objectIndex); 
   packet->PushBuffer(numBytes,buffer);
   return SF_MSG_OK;
   }


int
SF_MsgBuffer::PushMsg(uchar msg,
                      ushort objectIndex,
                      slong frame,
                      slong itime,
                      int flag) {

   SeekEnd();
   S_Packet *packet = &tcpPacket;
   if ( flag & S_CLIENT_UDP )
      packet = &udpPacket;

   packet->PushUChar(msg);
   packet->PushUShort(objectIndex); 
   packet->PushLong(frame);
   packet->PushLong(itime);
   return SF_MSG_OK;
   }


int
SF_MsgBuffer::PushMsg(uchar msg,
                      ushort objectIndex,
                      slong frame,
                      slong itime,
                      uchar ai1,
                      uchar ai2,
                      uchar ai3,
                      uchar ai4,
                      uchar ai5,
                      uchar ai6,
                      slong x,
                      slong y,
                      int flag) {

   SeekEnd();
   S_Packet *packet = &tcpPacket;
   if ( flag & S_CLIENT_UDP )
      packet = &udpPacket;

   packet->PushUChar(msg);
   packet->PushUShort(objectIndex); 
   packet->PushLong(frame);
   packet->PushLong(itime);
   packet->PushChar(ai1);
   packet->PushChar(ai2);
   packet->PushChar(ai3);
   packet->PushChar(ai4);
   packet->PushChar(ai5);
   packet->PushChar(ai6);
   packet->PushLong(x);
   packet->PushLong(y);
   return SF_MSG_OK;
   }


int
SF_MsgBuffer::PushMsg(uchar msg,
                      ushort objectIndex,
                      SF_Player *player,
                      int flag) {

   SeekEnd();
   S_Packet *packet = &tcpPacket;
   if ( flag & S_CLIENT_UDP )
      packet = &udpPacket;

   packet->PushUChar(msg);
   packet->PushUShort(objectIndex); 

   packet->PushULong (player->shipID);
   packet->PushULong (player->kills);
   packet->PushULong (player->losses);
   packet->PushULong (player->timeOnline);
   packet->PushULong (player->score);
   packet->PushChar  (player->rank);
   packet->PushChar  (player->race);
   packet->PushString(player->playerName.str);
   packet->PushString("");
   packet->PushString(player->shipName.str);
   packet->PushChar  (player->demo);
   packet->PushULong (player->credits);
   packet->PushULong (player->serial);

   return SF_MSG_OK;
   }


int
SF_MsgBuffer::PushMsg(uchar msg,
                      ushort objectIndex,
                      uchar timeStamp,
                      uchar scancode,
                      int flag) {

   SeekEnd();
   S_Packet *packet = &tcpPacket;
   if ( flag & S_CLIENT_UDP )
      packet = &udpPacket;

   packet->PushUChar(msg);
   packet->PushUShort(objectIndex);
   packet->PushUChar(timeStamp);
   packet->PushUChar(scancode);
   return SF_MSG_OK;
   }

int
SF_MsgBuffer::PushMsg(uchar msg,
                      ushort objectIndex,
                      uchar timeStamp,
                      ushort tobjectIndex,
                      int flag) {

   SeekEnd();
   S_Packet *packet = &tcpPacket;
   if ( flag & S_CLIENT_UDP )
      packet = &udpPacket;

   packet->PushUChar(msg);
   packet->PushUShort(objectIndex);
   packet->PushUChar(timeStamp);
   packet->PushUShort(tobjectIndex);
   return SF_MSG_OK;
   }


int
SF_MsgBuffer::PushMsg(uchar msg,
                      ushort objectIndex,
                      slong frame,
                      int flag) {

   SeekEnd();
   S_Packet *packet = &tcpPacket;
   if ( flag & S_CLIENT_UDP )
      packet = &udpPacket;

   packet->PushUChar(msg);
   packet->PushUShort(objectIndex);
   packet->PushLong(frame);
   return SF_MSG_OK;
   }


int
SF_MsgBuffer::PushMsg(uchar msg,
                      ushort objectIndex,
                      uchar frame,
                      slong l1,
                      int flag) {

   SeekEnd();
   S_Packet *packet = &tcpPacket;
   if ( flag & S_CLIENT_UDP )
      packet = &udpPacket;

   packet->PushUChar(msg);
   packet->PushUShort(objectIndex);
   packet->PushUChar(frame);
   packet->PushLong(l1);
   return SF_MSG_OK;
   }


int
SF_MsgBuffer::PushMsg(uchar msg,
                      ushort objectIndex,
                      uchar frame,
                      slong l1,
                      slong l2,
                      int flag) {

   SeekEnd();
   S_Packet *packet = &tcpPacket;
   if ( flag & S_CLIENT_UDP )
      packet = &udpPacket;

   packet->PushUChar(msg);
   packet->PushUShort(objectIndex);
   packet->PushUChar(frame);
   packet->PushLong(l1);
   packet->PushLong(l2);
   return SF_MSG_OK;
   }


int
SF_MsgBuffer::Merge(SF_MsgBuffer *msgBuffer) {
   udpPacket.Merge(&msgBuffer->udpPacket);
   tcpPacket.Merge(&msgBuffer->tcpPacket);
   return SF_MSG_OK;
   }


int
SF_MsgBuffer::RecvFrom(S_Client *client,int flag) {

   client->ReadPacket(&udpPacket,S_CLIENT_UDP|flag);
   if ( client->rc < 0 )
      return 0; /* *** Don't care if UDP recv fails *** */
   if ( udpPacket.readComplete )
      udpBytesRcvd += udpPacket.packetSize;

   client->ReadPacket(&tcpPacket,S_CLIENT_TCP|flag);
   if ( client->rc < 0 )
      return -1;
   if ( tcpPacket.readComplete )
      tcpBytesRcvd += tcpPacket.packetSize;

   return 0;
   }


int
SF_MsgBuffer::SendTo(S_Client *client,int fastProtocol) {
   client->WritePacket(&udpPacket,fastProtocol);
   if ( client->rc < 0 )
      return 0; /* *** Dont't care if UDP send fails *** */
   if ( udpPacket.packetSize > udpPacket.headerSize )
      udpBytesSent += udpPacket.packetSize;
   if ( tcpPacket.packetSize > 8192 )
      tcpPacket.Compress();
   client->WritePacket(&tcpPacket,S_CLIENT_TCP);
   if ( client->rc < 0 )
      return -1;
   if ( tcpPacket.packetSize > tcpPacket.headerSize )
      tcpBytesSent += tcpPacket.packetSize;

   return 0;
   }
