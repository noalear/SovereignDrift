/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
#include "sf.h"
#include "SF_PortCpp.h"
#include "SF_Externals.h"
#include "SF_Globals.h"
#include "SF_ClientWindow.h"
#include "SF_Client.h"
#include "SF_MsgBuffer.h"
#include "SF_Server.h"
#include "SF_Profile.h"
#include "SF_NetEngine.h"
#include "SF_GameEngine.h"
#include "SF_DisplayEngine.h"
#include "SF_Engine.h"
#include "SF_Slot.h"
#include "SF_Player.h"
#include "SF_Msg.h"
#include "SF_SpriteList.h"
#include "SF_Sprite.h"
#include "SF_ChatConsole.h"
#ifdef SFHOST
#  include "sf_net_api.h"   /* port 2026-10-12: portable sockets */
#endif

SF_NetEngine::SF_NetEngine(SF_Engine *iengine) {
   e = iengine;
   ge = e->gameEngine;
   c = e->client;

   fastDataMode = S_CLIENT_UDP;
   if ( c->profile->tcpipOnly )
      fastDataMode = S_CLIENT_TCP;

   dontTimeout = FALSE;
   lastTCPRcvd = 0;
   lastUDPRcvd = 0;
   lastTCPSent = 0;
   lastUDPSent = 0;

   lagCounter = 0;
   for ( int i=0; i<SF_NETENGINE_NUMLAGHISTORIES; i++ ) {
      lagHistory[i] = SF_NETENGINE_MAXLAG;
      driftHistory[i] = SF_NETENGINE_MAXLAG;
      }

   numGameStateMsgsRcvd = 0;
   numGameStateBytesRcvd = 0;
   serverFrame = 0;
   drift = 0;
   lag = 0;

   slot = new SF_Slot();
   /*slot->SetPlayer(ge->nullPlayer);*/

   ge->localSlot = localSlotNumber = 0;

   if ( e->serverName != NULL && e->run ) {

      SetLoginStatusText("Connecting . . .");

      e->state = SF_CLIENT_CONNECTING;
      c->rcEngine = e->rc = SF_ENGINE_CONNECTFAILED;
      if ( c->Connect(e->serverName,sf->serverPort,SF_SERVER_CONNECT) >= 0 ) {
         c->rcEngine = e->rc = SF_ENGINE_OK;

         S_Packet packet;
         packet.PushChar(SF_MAJOR_VERSION);
         packet.PushChar(SF_MINOR_VERSION);
         packet.PushChar(SF_MINOR_MINOR_VERSION);
         c->WritePacket(&packet);
         packet.Flush();

         SetLoginStatusText("Negotiating version . . .\n");

         c->ReadUShort(&ge->localSlot,S_CLIENT_TCP | S_CLIENT_BLOCK);

         if ( ge->localSlot == SF_SLOT_SERVERFULL ) {
            c->rcEngine = e->rc = SF_ENGINE_SERVERFULL;
            c->Disconnect();
            CloseLoginDialog(c);
            return;
            }

         if ( ge->localSlot == SF_SLOT_WRONGVERSION ) {
            c->rcEngine = e->rc = SF_ENGINE_WRONGVERSION;
            c->Disconnect();
            CloseLoginDialog(c);
            return;
            }

         if ( ge->localSlot == SF_SLOT_OLDVERSION ) {
            c->rcEngine = e->rc = SF_ENGINE_OLDVERSION;
            c->Disconnect();
            CloseLoginDialog(c);
            return;
            }

         localSlotNumber = ge->localSlot; /* ** Eventually take this away ** */

         slot->writeBuffer.mutex.Request();
	 mstate = 50;

         slot->writeBuffer.PushMsg((uchar)SF_MSG_GAMESTATE,
                                   (ushort)ge->localSlot,
                                   (ulong)0,
                                   (char *)NULL,
                                   (int)S_CLIENT_TCP);

         slot->writeBuffer.PushMsg((uchar)SF_MSG_FRAMERATE,
                                   (ushort)ge->localSlot,
                                   (uchar)0,
				   (int)S_CLIENT_TCP);

         slot->writeBuffer.PushMsg((uchar)SF_MSG_UPDATERATE,
                                   (ushort)ge->localSlot,
                                   (uchar)0,
				   (int)S_CLIENT_TCP);

	 mstate = 0;
         slot->writeBuffer.mutex.Release();

         SetLoginStatusText(" Waiting for game data . . .\n");

         e->state = SF_CLIENT_CONNECTED;
         }

      }
   else if ( e->run ) {
      /* *** Enable the login dialog Ok button, set race,ship data *** */
      EnableLoginDialog(c,0,0,TRUE);

      SetLoginStatusText("Select a player,race,ship . . . Press 'Ok'\n");

      e->state = SF_CLIENT_SINGLEPLAYER;
      c->rcEngine = e->rc = SF_ENGINE_OK;
      }

   if ( e->rc != SF_ENGINE_OK )
      CloseLoginDialog(c);

   }

SF_NetEngine::~SF_NetEngine() {
   if ( e->state == SF_CLIENT_CONNECTED ) {

     S_String msg("User Disconnect");

      slot->writeBuffer.mutex.Request();
      mstate = 51;
      slot->writeBuffer.PushMsg((uchar)SF_MSG_DISCONNECT,
                                (ushort)ge->localSlot,
                                msg.str,
                                (int)S_CLIENT_TCP);
      slot->writeBuffer.SendTo(c,fastDataMode);
      mstate = 0;
      slot->writeBuffer.mutex.Release();

      e->timer->Snooze(100000); /* *** This is a patch *** */
      c->Disconnect();
      }
   if ( slot ) {
      delete slot;
      slot = NULL;
      }
   e->state = SF_CLIENT_DISCONNECTED;
   }


int
SF_NetEngine::HandlePingMsg(SF_Msg *msg) {

   if ( msg->msg == SF_MSG_PINGUDP ) {

      int i;

      slong tmpLag = ge->itime - msg->itime;
      lagHistory[lagCounter%SF_NETENGINE_NUMLAGHISTORIES] = tmpLag;

      slong minLag = SF_NETENGINE_MAXLAG;
      for ( i=0; i<SF_NETENGINE_NUMLAGHISTORIES; i++ ) {
         if ( lagHistory[i] < minLag )
            minLag = lagHistory[i];
         }

      slong tmpDrift = ge->frame - msg->frame;
      driftHistory[lagCounter%SF_NETENGINE_NUMLAGHISTORIES] = tmpDrift;

      slong minDrift = SF_NETENGINE_MAXLAG;
      for ( i=0; i<SF_NETENGINE_NUMLAGHISTORIES; i++ ) {
         if ( labs(driftHistory[i]) < labs(minDrift) )
            minDrift = labs(driftHistory[i]);
         }

      int syncTime = (labs(tmpDrift) <= labs(minDrift)) ||
                     (labs(tmpDrift) <= labs(drift)) ||
                     (labs(tmpLag) <= labs(minLag)) ||
                     (labs(tmpLag) <= labs(lag));

      serverFrame = msg->frame;

      if ( syncTime ) {
         lag = ge->itime - msg->itime;
         serverFrame = msg->frame;
         drift = ge->frame - serverFrame;
         ge->frame = serverFrame + lag/2;
         }

      lagCounter++;

      }
   return 0;
   }


inline int
_NumBytesToReadInSocket( slong sock ) {
   int numBytes = 0;
#  ifdef WIN32
   ioctlsocket (sock,FIONREAD,(ulong*)&numBytes);
#  endif
#  ifdef OS2
   ioctl (sock,FIONREAD,(char*)&numBytes,sizeof(int));   
#  endif
#  ifdef UNIX
   ioctl (sock,FIONREAD,(char*)&numBytes,sizeof(int));   
#  endif
   return(numBytes);
   }


int
SF_NetEngine::Update() {
   SF_Msg msg;
   SF_MsgBuffer *rb = &slot->readBuffer;
   SF_MsgBuffer *wb = &slot->writeBuffer;
   if ( e->state == SF_CLIENT_CONNECTED ) {

      wb->mutex.Request();
      mstate = 52;

      /* *************************************************************** */
      /* *** Send all msgs returned by the engine across the network *** */
      /* *************************************************************** */
      if ( ge->retMsgBuffer )
	{
          wb->Merge(ge->retMsgBuffer);
	}

      /* ******************************************************** */
      /* *** Ping the server UDP to get frame synchronization *** */
      /* ******************************************************** */
      if ( (ge->itime + ge->localSlot)%(4*frameRate) == 0 ) {
         wb->PushMsg((uchar)SF_MSG_PINGUDP,
                     (ushort)ge->localSlot,
                     (slong)0,
                     (slong)ge->itime,
                      fastDataMode);
         }

      /* ******************************************************** */
      /* *** Ping the server TCP to get frame synchronization *** */
      /* ******************************************************** */
      if ( (ulong)((ge->itime+ge->localSlot)%(4*frameRate)) == 
           (ulong)(2*frameRate)) {
         wb->PushMsg((uchar)SF_MSG_PINGTCP,
                     (ushort)ge->localSlot,
                     (slong)0,
                     (slong)ge->itime,
                      S_CLIENT_TCP);
         }

      /* ****************************************** */
      /* *** Broadcast Object track information *** */
      /* ****************************************** */
      if ( (ge->itime + ge->localSlot) % shipUpdateRate == 0 ) {
	
	slong minLag = SF_NETENGINE_MAXLAG;
	for ( int i=0; i<SF_NETENGINE_NUMLAGHISTORIES; i++ ) {
	  if ( lagHistory[i] < minLag )
	    minLag = lagHistory[i];
	}
	
	if ( IsValidObjectPointer(game->objectList[ge->localSlot]) )
	  {
	    wb->PushMsg((uchar)SF_MSG_OBJECTTRACK_17,
			(ushort)ge->localSlot,
			(uchar)ge->frame % 256,
			(uchar)game->objectList[ge->localSlot]->Offset(),
			(uchar)minLag,
			(short)game->objectList[ge->localSlot]->tvel.x,
			(short)game->objectList[ge->localSlot]->tvel.y,
			(slong)game->objectList[ge->localSlot]->now.x,
			(slong)game->objectList[ge->localSlot]->now.y,
			fastDataMode);
	  }
      }
      
      /* ************************************* */
      /* *** Write outgoing msgs to server *** */
      /* ************************************* */
      if ( wb->SendTo(c,fastDataMode) < 0 ) {
         e->run = FALSE;
         e->selfTermination = TRUE;
         c->rcEngine = e->rc = SF_ENGINE_WRITEFAILED;
         }

      if ( wb->tcpPacket.packetSize > 8 ) {
         lastTCPSent = ge->itime;
         if ( fastDataMode == S_CLIENT_TCP )
            lastUDPSent = ge->itime;
         }

      if ( wb->udpPacket.packetSize > 8 )
         lastUDPSent = ge->itime;

      wb->Flush();

      mstate = 0;
      wb->mutex.Release();

      /* ************************************** */
      /* *** Read incoming msgs from server *** */
      /* ************************************** */
      int numReads = 0;
      int continueReading;
      do {

         continueReading = FALSE;

         ulong udpBytes = rb->udpPacket.totalBytesRcvd;
         ulong tcpBytes = rb->tcpPacket.totalBytesRcvd;

         if ( rb->RecvFrom(c,S_CLIENT_NONBLOCK) < 0 ) {
            e->run = FALSE;
            e->selfTermination = TRUE;
            c->rcEngine = e->rc = SF_ENGINE_READFAILED;
            }

         if ( rb->udpPacket.totalBytesRcvd > udpBytes )
            lastUDPRcvd = ge->itime;
         if ( rb->tcpPacket.totalBytesRcvd > tcpBytes )
            lastTCPRcvd = ge->itime;

         if ( numGameStateBytesRcvd >= 0 && rb->tcpPacket.packetSize > 100 &&
              rb->tcpPacket.totalBytesRcvd > numGameStateBytesRcvd ) {
            numGameStateBytesRcvd = rb->tcpPacket.totalBytesRcvd;
            DrawLoginStatusBar(numGameStateBytesRcvd*100/
                               rb->tcpPacket.packetSize);
	    }

         if ( numGameStateMsgsRcvd > 0 && !dontTimeout && (
              lastUDPSent-lastUDPRcvd > (slong)SF_TIMEOUT_SEC*frameRate ||
              lastTCPSent-lastTCPRcvd > (slong)SF_TIMEOUT_SEC*frameRate ) ){
            e->run = FALSE;
            e->selfTermination = TRUE;
            c->rcEngine = e->rc = SF_ENGINE_LOSTCONTACT;
            }

         if ( numGameStateMsgsRcvd > 0 && (
              lastUDPSent-lastUDPRcvd > (slong)SF_TIMEOUT_SEC/2*frameRate ||
              lastTCPSent-lastTCPRcvd > (slong)SF_TIMEOUT_SEC/2*frameRate ) ){
            fastDataMode = S_CLIENT_TCP;
            }

         if ( rb->udpPacket.readComplete ) {

            lastUDPRcvd = ge->itime;

            continueReading = TRUE;

            /* **************************************** */
            /* *** Extract UDP Ping msg from buffer *** */
            /* **************************************** */
            rb->udpPacket.Rewind();
            while ( rb->GetNextMsg(ge->localSlot,&msg,
                    S_CLIENT_UDP,SF_MSG_PINGUDP) )
               HandlePingMsg(&msg);

            ge->msgBuffer->udpPacket.Merge(&rb->udpPacket);
            rb->udpPacket.Flush();

            numReads++;
            }

         if ( rb->tcpPacket.readComplete ) {

            lastTCPRcvd = ge->itime;
            if ( fastDataMode == S_CLIENT_TCP )
               lastUDPRcvd = ge->itime;

            continueReading = TRUE;

            /* **************************************** */
            /* *** Extract TCP Ping msg from buffer *** */
            /* **************************************** */
            rb->tcpPacket.Rewind();
            while ( rb->GetNextMsg(ge->localSlot,&msg,
                    S_CLIENT_TCP,SF_MSG_PINGTCP) )
               HandlePingMsg(&msg);

            /* **************************************** */
            /* *** Extract UDP Ping msg from buffer *** */
            /* **************************************** */
            rb->tcpPacket.Rewind();
            while ( rb->GetNextMsg(ge->localSlot,&msg,
                    S_CLIENT_TCP,SF_MSG_PINGUDP) )
               HandlePingMsg(&msg);

            /* ********************************** */
            /* *** Extract File download msgs *** */
            /* ********************************** */
            rb->tcpPacket.Rewind();
            while ( rb->GetNextMsg(ge->localSlot,&msg,
                                S_CLIENT_TCP,SF_MSG_DOWNLOADFILE) ) {
               if ( msg.str.str && msg.buffer ) {
                  _OsFilename(msg.str.str);
                  char tmp[128];
                  sprintf(tmp,"Received %s from server . . .",msg.str.str);
                  SetScreenMessage(tmp,RGB_WHITE);
                  FILE *fptr = fopen(msg.str.str,"wb");
                  if ( fptr ) {
                     fwrite(msg.buffer,msg.bSize,1,fptr);
                     fclose(fptr);
		     }
                  S_free(msg.buffer);
	          }
	       }

            /* ****************************************** */
            /* *** Extract gamestate data from buffer *** */
            /* ****************************************** */
            rb->tcpPacket.Rewind();
            if ( rb->GetNextMsg(ge->localSlot,&msg,
                                S_CLIENT_TCP,SF_MSG_GAMESTATE) ) {
	      ge->disableInput = 0;
	      ge->disableF9 = frameNumber;
               if ( msg.buffer ) {
                  EngineSetGameState(msg.buffer);
                  S_free(msg.buffer);
                  msg.buffer = NULL;

                  SF_DisplayEngine *de = e->displayEngine;
                  if ( de ) {

                     for ( int i=0; i<MAXOBJECTS; i++ )
                        de->onScreen[i] = FALSE;
		     }

                  if ( numGameStateMsgsRcvd == 0 ) {
                     if ( e && e->window )
                        ge->InitGUIClient();
		     EnableLoginDialog(sf,0,0,TRUE);

                     SetLoginStatusText("Select a player,race,ship . . . Press"
                                        " 'Ok'\n");
	             }

		  for ( int i=0; i<game->numBitmaps; i++ )
		    {
		      if ( e && e->msl )
			{
			  //char *clas = game->objectBitmapList[i].Class;
			  char *name = game->objectBitmapList[i].name;
			  char *file = game->objectBitmapList[i].file;
			  SF_Sprite *sprite = e->msl->Sprite(file);
			  if ( sprite && sprite->IsACOnly(name) )
			    {
			      game->objectBitmapList[i].acOnly = TRUE;
			    }
			}
		    }

		  ResetTimeout();
                  ge->frame = game->frame;
                  numGameStateMsgsRcvd++;
	          }
               }

            /* ****************************************** */
            /* *** Extract frameRate data from buffer *** */
            /* ****************************************** */
            rb->tcpPacket.Rewind();
            if ( rb->GetNextMsg(ge->localSlot,&msg,
                 S_CLIENT_TCP,SF_MSG_FRAMERATE) ) {
               if ( msg.frameRate > 0 ) {
                  frameRate = (uchar)msg.frameRate;
		  shipUpdateRate = frameRate;  // default to once per second
                  e->timer->Set(1000000/frameRate);
	          }
	       }

            /* ****************************************** */
            /* *** Extract updateRate data from buffer *** */
            /* ****************************************** */
            rb->tcpPacket.Rewind();
            if ( rb->GetNextMsg(ge->localSlot,&msg,
				S_CLIENT_TCP,SF_MSG_UPDATERATE) ) {
	      if ( msg.frameRate > 0 ) {
		shipUpdateRate = (uchar)msg.frameRate;
	      }
	    }
	    
            /* ****************************************** */
            /* *** Extract RequestDev data from buffer *** */
            /* ****************************************** */
            rb->tcpPacket.Rewind();
            if ( rb->GetNextMsg(ge->localSlot,&msg,
				S_CLIENT_TCP,SF_MSG_REQUESTDEV) ) {
	      SF_DisplayEngine *de = e->displayEngine;
	      if ( de && de->chat ) {
		de->chat->devmode = msg.frameRate;
		if ( de->chat->devmode )
		  {
		    SF_ChatChannel *channel = NULL;
		    channel = &de->chat->channels[SF_CHANNEL_BROADCAST];
		    channel->AppendLine(" ");
		    channel->AppendLine("You have dev status.  Commands are:");
		    channel->AppendLine("  /kick [slot]           "
					"Example: /kick 0");
		    channel->AppendLine(" ");
		  }
	      }
	    }
	    
            ge->msgBuffer->tcpPacket.Merge(&rb->tcpPacket);
            rb->tcpPacket.Flush();

            numReads++;

            if ( !numGameStateMsgsRcvd && e->state == SF_CLIENT_CONNECTED )
               rb->Flush();
	    }

         } while ( continueReading );

      }

   return 0;
   }

int
SF_NetEngine::ResetTimeout()
{
  lastTCPRcvd = ge->itime;
  lastUDPRcvd = ge->itime;
  lastTCPSent = ge->itime;
  lastUDPSent = ge->itime;
  return 0;
}
