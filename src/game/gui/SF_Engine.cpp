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
#include "SF_Engine.h"
#include "SF_ClientWindow.h"
#include "SF_Client.h"
#include "SF_SpriteList.h"
#include "SF_Player.h"
#include "SF_Profile.h"
#include "SF_GameEngine.h"
#include "SF_NetEngine.h"
#include "SF_Slot.h"
#include "SF_Msg.h"
#include "SF_MsgBox.h"
#include "SF_MsgBuffer.h"
#include "SF_DisplayEngine.h"
#include "SF_ChatConsole.h"

int EngineThreadFunc(void *);

SF_Engine::SF_Engine(char *iserverName,SF_Client *iclient,SF_SpriteList *imsl,
                     char *igameStateFilename){

   client = iclient;
   netEngine = NULL;
   gameEngine = NULL;
   displayEngine = NULL;
   stopFast = FALSE;

   check = 0xAAAAAAAA;

   kickedMsg.Create("No reason");

   window = NULL;
   if ( client )
      window = client->mainWindow;
   msl = imsl;

   localPlayerLoggedIn = 0;

   LaunchLoginDialog(window);

   if ( iserverName == NULL )
      serverName = NULL;
   else {
      serverName = (char *)S_malloc(strlen(iserverName)+1);
      strcpy(serverName,iserverName);
      }

   if ( igameStateFilename == NULL )
      gameStateFilename = NULL;
   else {
      gameStateFilename = (char *)S_malloc(strlen(igameStateFilename)+1);
      strcpy(gameStateFilename,igameStateFilename);
      }

   state = SF_CLIENT_DISCONNECTED;

   timer = new S_Timer(1000000/SF_DEFAULT_FRAMERATE,sf->stopWatchType);

   run = TRUE;
   engineThread = new S_Thread(EngineThreadFunc,(void *)this,131072);

   }


int
SF_Engine_OkButtonHandler(void *iarg) {
   return 0;
   }



SF_Engine::~SF_Engine() {

   if ( run && threadRunning ) {
      run = FALSE;
      if ( engineThread && !selfTermination )
         engineThread->WaitFor();
      }

   if ( gameStateFilename ) {
      S_free(gameStateFilename);
      gameStateFilename = NULL;
      }

   if ( serverName ) {
      S_free(serverName);
      serverName = NULL;
      }

   if ( engineThread ) {
      delete engineThread;
      engineThread = NULL;
      }

   if ( timer ) {
      delete timer;
      timer = NULL;
      }

   if ( sf->msgBox ) {
      switch ( rc ) {
         case SF_ENGINE_READFAILED :
            sf->msgBox->Show("Disconnected","Error reading data from server.",
                             SF_Engine_OkButtonHandler);
            break;
         case SF_ENGINE_WRITEFAILED :
            sf->msgBox->Show("Disconnected","Error writing data to server.",
                             SF_Engine_OkButtonHandler);
            break;
         case SF_ENGINE_LOSTCONTACT :
            sf->msgBox->Show("Disconnected","Lost contact with server.",
                             SF_Engine_OkButtonHandler);
            break;
         case SF_ENGINE_SERVERKICKED :
	   {
	     S_String msg("Server kicked you off.\n\nReason:\n%s",
                          kickedMsg.str);
             sf->msgBox->Show("Disconnected",msg.str,
                              SF_Engine_OkButtonHandler);
             kickedMsg.Create("No reason");
             break;
	   }
	 default :
             sf->msgBox->Hide();
      }
   }


   }


int
EngineMsgBoxOkHandler(void *iarg) {
   SF_Engine *e = (SF_Engine *)iarg;
   if ( e ) {
      e->run = FALSE;
      e->selfTermination = TRUE;
      }
   return 0;
   }
   

int
EngineThreadFunc(void *arg) {
   SF_Engine *e = (SF_Engine *)arg;
   SF_Client *c = e->client;
   if ( !e ) return 0;

   e->won = FALSE;
   e->selfTermination = FALSE;
   e->threadRunning = TRUE;

   frameRate = SF_DEFAULT_FRAMERATE;

   e->gameEngine = new SF_GameEngine(e,e->gameStateFilename);
   e->netEngine = new SF_NetEngine(e);

   if ( e->state == SF_CLIENT_CONNECTING ) {
      e->state = SF_CLIENT_DISCONNECTED;
      e->selfTermination = TRUE;
      e->run = FALSE;
      }
   if ( e->gameEngine &&
		e->gameEngine->rc != SF_GAMEENGINE_OK && 
        e->gameStateFilename ) {
      char tmp[8192],line[1024],title[1024];
      tmp[0] = 0;
      sprintf(title,"ERROR -- %s",e->gameStateFilename);
      FILE *fptr = fopen("frontier.log","r");
      if ( fptr ) {
         while ( fgets(line,1023,fptr) != NULL )
            sprintf(tmp,"%s%s",tmp,line);
         fclose(fptr);
         }
      S_MsgBox msg(sf->mainWindow,title,tmp,S_MSGBOX_OK);
      e->selfTermination = TRUE;
      e->run = FALSE;
      }
   else if ( e->client && e->window && e->run )
      e->displayEngine = new SF_DisplayEngine(e);

   e->gameEngine->de = e->displayEngine;
   e->gameEngine->ne = e->netEngine;

   if ( e->state == SF_CLIENT_SINGLEPLAYER && 
        e->gameEngine->rc == SF_GAMEENGINE_OK ) {
      if ( e && e->window && e->run )
         e->gameEngine->InitGUIClient();
      }

   if ( _root && _root->numColors > 256 ) {
      SetScreenMessage("*",CLR_YELLOW);
      SetScreenMessage("* NOTE ***",CLR_YELLOW);
      SetScreenMessage("* Press F7 to toggle debug info which includes FPS",
                       CLR_YELLOW);
      SetScreenMessage("* If you are experiencing low FPS, then try setting "
                       "your video color depth to 8-bit or 256 colors",
                       CLR_YELLOW);
      SetScreenMessage("* Stellar Frontier runs best at 256 colors",
                       CLR_YELLOW);
      SetScreenMessage("* NOTE ***",CLR_WHITE);
      SetScreenMessage("*",CLR_YELLOW);
      }

   e->timer->Set(1000000/frameRate);
   while ( e->run ) {

      e->sw.Start();

      if ( e->netEngine )
         e->netEngine->Update();

      if ( e->gameEngine )
	{
	  if (  e->gameEngine->retMsgBuffer && 
		!e->gameEngine->retMsgBuffer->IsEmpty() )
	    {
	      e->gameEngine->msgBuffer->Merge(e->gameEngine->retMsgBuffer);
	      e->gameEngine->retMsgBuffer->Flush();
	    }
	  e->gameEngine->Update();
	}

      if ( e->displayEngine && e->gameEngine->guiInitDone )
	{
	  e->displayEngine->Update();
	}

      e->sw.Stop();

      /* *** Decide if single player game is over *** */
      if ( e->window && e->state == SF_CLIENT_SINGLEPLAYER ) {

         if ( !game->IsActive() ) {
            int playerIndex = game->ship[e->gameEngine->localSlot].playerIndex;
            int raceIndex = game->player[playerIndex].raceId;

            if ( c->msgBox && game->race[raceIndex].status != 
                 RaceClass::victorious ) {
               c->msgBox->Show("You Lost",
                               "Ha Ha! Your race has been assimilated!",
                               EngineMsgBoxOkHandler,(void *)e);
               }

            if ( c->msgBox && game->race[raceIndex].status == 
                 RaceClass::victorious ) {
               e->won = TRUE;
               c->msgBox->Show("You Won","Hmmph!",EngineMsgBoxOkHandler,
                               (void *)e);
               }
	    }

         }

      e->timer->WaitFor();
      if ( e->state == SF_CLIENT_SINGLEPLAYER )
         e->timer->Set(1000000/frameRate);

#     ifdef OS2
         POINTL ptl;
         WinQueryPointerPos(HWND_DESKTOP,&ptl);
         WinSetPointerPos(HWND_DESKTOP,ptl.x,ptl.y);
#     endif

      }

   if ( e && e->displayEngine )
     {
       e->displayEngine->chat->DisconnectIRC();
     }

   //
   // Check for RAM cheat first.
   //
   int pi = game->ship[localSlotNumber].playerIndex;
   game->player[pi].AddToStat(game->player[pi].totalScore,
			      (ulong)0
			      );

   SF_Player *sfplayer = sf->profile->players[sf->profile->selectedPlayer];
   if ( sfplayer->playerName.str[0] == '~' )
     {
       sfplayer->playerName.RemoveCharacter('~');
     }
   
   e->client->profile->Save();

   if ( e->netEngine ) {
      delete e->netEngine;
      e->netEngine = NULL;
      }

   if ( e->displayEngine ) {
      delete e->displayEngine;
      e->displayEngine = NULL;
      }

   if ( !e->stopFast ) {
      c->engine = NULL;
      if ( c->mainWindow )
         c->mainWindow->Refresh();
      }

   if ( e->localPlayerLoggedIn ) {
      e->client->profile->Update(e);
      if ( c->connectedThroughNexus ) {
         if ( !e->stopFast && sf->msgBox && sf->profile->selectedPlayer>=0 ) {
            char tmp[1024];
            sprintf(tmp," Writing data for %s . . .\n",
               sf->profile->players[sf->profile->selectedPlayer]->playerName.str);
            sf->msgBox->Show(sf->profile->nexusName,tmp);
            }

         int rc = 0;
         do {

            rc = c->SetPlayerDataOnNexus(c->profile->nexusName,
                                         c->profile->players[
                                         c->profile->selectedPlayer]);
            if ( rc ) {
               char rcstr[128];
               char tmp[1024];
               sprintf(rcstr,"Unknown error");
               if ( rc == -3 )
                  sprintf(rcstr,"Wrong Password");
               sprintf(tmp,"I could not upload your stats to the nexus.\n"
                           "It may be that the nexus is down or was very "
                           "busy.\n\nrc = %d (%s)\n\nShould I try again?",
                           rc,rcstr);
               S_MsgBox msg(sf->mainWindow,"SF Nexus",tmp,S_MSGBOX_YESNO);
               if ( msg.rc == S_MSGBOX_NO)
                  rc = 0;
               }
            } while ( rc );

         if ( !e->stopFast && sf->msgBox )
            sf->msgBox->Hide();
         }
      c->connectedThroughNexus = FALSE;
      }

   if ( e->gameEngine ) {
      delete e->gameEngine;
      e->gameEngine = NULL;
      }

   if ( !e->stopFast ) {

      if ( c && c->mainWindow && c->bmpSplash )
         c->y0Credits = c->mainWindow->cyClient/2 + 
                        c->bmpSplash->cy/2 - 10;

      e->threadRunning = FALSE;

      if ( e && e->selfTermination )
         delete e;

      if ( c->mainWindow )
         c->mainWindow->Refresh();
      }

   return 0;
   }


int
SF_Engine::PushKeyboardMsg(int msg,int sc) {

   SF_NetEngine *ne = netEngine;
   SF_GameEngine *ge = gameEngine;
   SF_Client *c = client;

   ShipClass *s = &game->ship[ge->localSlot];

   if ( !IsValidObjectPointer(s) )
     {
       return 0;
     }

   //
   // Don't allow multiple weapon fire msgs to be sent in the same packet.
   //
   if ( sc == SF_KEY_SPACE || 
       (sc >= SF_KEY_1 && sc <= SF_KEY_0 && !c->shiftDown && !c->ctrlDown) ) {

     if ( !IsInInternetCafe(s) && !s->IsWarping() )
       {
	 
	 SF_Msg gmsg;
	 
	 ge->msgBuffer->mutex.Request();
	 ge->msgBuffer->Rewind();
	 if ( ge->msgBuffer->GetNextMsg(ge->localSlot,
					&gmsg,
					ne->fastDataMode,
					SF_MSG_FIRETUBES,
					FALSE) ) {
	   ge->msgBuffer->mutex.Release();
	   if ( sf )
	     sf->mutex.Release();
	   return 0;
         }
	 
	 short i;
	 if (sc == SF_KEY_SPACE) {
	   s->Command(BusStruct::no_message, nullVector, nullVector);
	   for (i=0; i<s->bus.numTubes; i++)
	     if(!s->bus.tubeMod[i]->CheckTubeMode(ModuleClass::manual_fire_mode))
	       s->bus.tubeMod[i]->activate();
         }
	 else if (sc - SF_KEY_1 < s->bus.numTubes)
	   s->bus.tubeMod[sc - SF_KEY_1]->activate();
	 
	 if ( sf )
	   sf->mutex.Request();
	 s->bus.DecideWhichWeaponsToFire(s);
	 
	 if ( s->tubeSelector > 0 )
	   ge->msgBuffer->PushMsg((uchar)SF_MSG_FIRETUBES,
				  (ushort)ge->localSlot,
				  (uchar)ge->frame,
				  (uchar)s->heading,
				  (uchar)s->tubeSelector,
				  (short)s->tvel.x,
				  (short)s->tvel.y,
				  (slong)s->now.x,
				  (slong)s->now.y,
				  (int)ne->fastDataMode);
	 
	 if ( state == SF_CLIENT_CONNECTED ) {
	   
	   ne->slot->writeBuffer.mutex.Request();
	   mstate = 20;
	   if ( s->tubeSelector > 0 )
	     ne->slot->writeBuffer.PushMsg((uchar)SF_MSG_FIRETUBES,
					   (ushort)ge->localSlot,
					   (uchar)ge->frame,
					   (uchar)s->heading,
					   (uchar)s->tubeSelector,
					   (short)s->tvel.x,
					   (short)s->tvel.y,
					   (slong)s->now.x,
					   (slong)s->now.y,
					   (int)ne->fastDataMode);
	   mstate = 0;
	   ne->slot->writeBuffer.mutex.Release();
         }
	 
	 if ( sf )
	   sf->mutex.Release();
	 
	 ge->msgBuffer->mutex.Release();
       }
   }
   else if ( sc == SF_KEY_BACKSPACE && !c->ctrlDown ) {
      game->DecideToRefitShip(ge->localSlot);
      }
   else if ( sc == SF_KEY_C ) {

      if ( s->bus.stealthMod ) {

         if ( s->stealth == 1 ) {
            ge->msgBuffer->mutex.Request();
            ge->msgBuffer->PushMsg((uchar)SF_MSG_CLOAK,
                                   (ushort)ge->localSlot,
                                   (slong)s->bus.stealthMod->store,
                                   (slong)s->bus.stealthMod->damage,
                                   (int)S_CLIENT_TCP);
            ge->msgBuffer->mutex.Release();

            if ( state == SF_CLIENT_CONNECTED ) {
               ne->slot->writeBuffer.mutex.Request();
	       mstate = 21;
               ne->slot->writeBuffer.PushMsg((uchar)SF_MSG_CLOAK,
                                             (ushort)ge->localSlot,
                                             (slong)s->bus.stealthMod->store,
                                             (slong)s->bus.stealthMod->damage,
                                             (int)S_CLIENT_TCP);
	       mstate = 0;
               ne->slot->writeBuffer.mutex.Release();
               }
	    }
         else {
            ge->msgBuffer->mutex.Request();
            ge->msgBuffer->PushMsg((uchar)SF_MSG_UNCLOAK,
                                   (ushort)ge->localSlot,
                                   (int)S_CLIENT_TCP);
            ge->msgBuffer->mutex.Release();

            if ( state == SF_CLIENT_CONNECTED ) {
               ne->slot->writeBuffer.mutex.Request();
	       mstate = 22;
               ne->slot->writeBuffer.PushMsg((uchar)SF_MSG_UNCLOAK,
                                             (ushort)ge->localSlot,
                                             (int)S_CLIENT_TCP);
	       mstate = 0;
               ne->slot->writeBuffer.mutex.Release();
               }
	    }

         }

      }
   else {
      ge->msgBuffer->mutex.Request();
      ge->msgBuffer->PushMsg((uchar)msg,
                             (ushort)ge->localSlot,
                             (uchar)ge->frame,
                             (uchar)sc,
                             (int)S_CLIENT_TCP);
      ge->msgBuffer->mutex.Release();

      if ( state == SF_CLIENT_CONNECTED ) {
         ne->slot->writeBuffer.mutex.Request();
	 mstate = 23;
         ne->slot->writeBuffer.PushMsg((uchar)msg,
                                       (ushort)ge->localSlot,
                                       (uchar)ge->frame,
                                       (uchar)sc,
                                       (int)S_CLIENT_TCP);
	 mstate = 0;
         ne->slot->writeBuffer.mutex.Release();
         }
      }

   /* ********************************************************************** */
   /* *** If a '/' key is sent and a colony is attached to the ship then *** */
   /* *** also send a SF_MSG_OBJECTTRACKMSG for the colony. Send it TCP  *** */
   /* *** and I suppose the velocity should either be zero or the vel    *** */
   /* *** of the ship.                                                   *** */
   /* ********************************************************************** */
   if ( sc == SF_KEY_QUESTION && 
	s->bus.numPorts > 0 && 
	s->bus.dockMod && 
	s->bus.dockMod[0] && 
        s->bus.dockMod[0]->workPar[1] && 
	s->bus.dockMod[0]->workPar[0] &&
        state == SF_CLIENT_CONNECTED )
     {
       GenericClass *g = (GenericClass *)s->bus.dockMod[0]->workPar[0];
       if ( IsValidObjectPointer(g) && g->Type() == colony )
	 {
	   ne->slot->writeBuffer.mutex.Request();
	   mstate = 24;
	   ne->slot->writeBuffer.PushMsg((uchar)SF_MSG_OBJECTTRACK_17,
					 (ushort)game->ObjectListIndex(g),
					 (uchar)frameNumber % 256,
					 (uchar)g->Offset(),
					 (uchar)g->Index(),
					 (short)g->tvel.x,
					 (short)g->tvel.y,
					 (slong)g->now.x,
					 (slong)g->now.y,
					 S_CLIENT_TCP);
	   mstate = 0;
	   ne->slot->writeBuffer.mutex.Release();
	 }
     }
   
   return 0;
}

