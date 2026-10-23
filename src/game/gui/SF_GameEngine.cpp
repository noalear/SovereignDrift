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
#include "SF_Server.h"
#include "SF_GameEngine.h"
#include "SF_PortCpp.h"
#include "SF_Button.h"
#include "SF_DisplayEngine.h"
#include "SF_NetEngine.h"
#include "SF_VideoConsole.h"
#include "SF_MapConsole.h"
#include "SF_ChatConsole.h"
#include "SF_UpgradeWindow.h"
#include "SF_Engine.h"
#include "SF_MsgBuffer.h"
#include "SF_Player.h"
#include "SF_Msg.h"
#include "SF_Slot.h"
#include "SF_MsgBox.h"
#include "SF_Profile.h"
#include "SF_WaveList.h"
#include "SF_SpriteList.h"
#include "SF_Sprite.h"
#include "SF_Font.h"

inline slong 
IsServer()
{
  return sf && sf->engine == NULL;
}

#define NUM_HELP_MSGS 6
char *_HelpMsgs[] =
 {
  "Stardock> Press F8 to save your stats.",
  "Stardock> To get a colony, orbit a planet and press J",
  "Stardock> Shift R shows you all the ranks",
  "Stardock> Press D to dock to a starbase and upgrade your ship",
  "Stardock> Press [Del] to detonate your weapons",
  "Stardock> You can get larger colonies by pressing J multiple times."
};


#define NUM_AC_MSGS 8
char *_ACMsgs[] =
 {
  "Stardock> Join "SF_REGISTERED_CLUB" for only $19.95. A Bargain!",
  "Stardock> Join "SF_REGISTERED_CLUB" and get lasers and ram :) Woo Hoo!",
  "Stardock> Wanna fly a Starbase? $19.95 get's you in the driver's seat.",
  "Stardock> "SF_REGISTERED_CLUB":http://www.stardock.com/products/sf/admiral.html.",
  "Stardock> Mmmmmmm. Lasers.  Must have.  "SF_REGISTERED_CLUB".  Must have.",
  "Stardock> Support starving game developers (^_^) Join "SF_REGISTERED_CLUB".",
  "Stardock> Fly a Marauder as a Cadet. Join "SF_REGISTERED_CLUB".",
  "Stardock> Studies have show "SF_REGISTERED_CLUB" members have 30% less acne."
};

int SetupConsole();

SF_GameEngine::SF_GameEngine(SF_Engine *iengine,char *gameStateFilename) {
   e = iengine;

   de = NULL;
   ne = NULL;

   check = 0xAAAAAAAA;
   guiInitDone = FALSE;

   event = 0;
   ch = 0;
   s = 0;
   l = 0;

   numLives = 3;

   int i,j;
   for ( i=0; i<256; i++ )
      for ( j=0; j<256; j++ )
         msgHistograph[i][j] = 0;

   for ( i=0; i<max_players; i++ ) {
      boostOn[i] = FALSE;
      impulseOn[i] = FALSE;
      }

   itime = 0;
   frame = 0;

   controlsDisabled = FALSE;
   disableInput = 0;
   disableF9 = 0;

   objectCPUWarning = 0;
   objectCPUWarningTime = 0;

   DrawLoginStatusBar();

   egg.ZCreate("egg.bmp",
               S_BITMAP_MASK_TRANSLUCENT,
               S_BITMAP_BLIT_USEMASK,
               1,100,0);

   bgSprite = NULL;
   bg2Sprite = NULL;

   for ( i=0; i<SF_GAMEENGINE_NUMSHIELDSPRITES; i++ )
     shldSprites[i] = NULL;

   paused = FALSE;
   pauseEvent.Set();
   retMsgBuffer = NULL;
   msgBuffer = new SF_MsgBuffer();

   localSlot = localSlotNumber = 0;

   /* **************************************************************** */
   /* *** Fix this so that nothing is constructed for network game *** */
   /* **************************************************************** */
   if ( gameStateFilename && _OsFileExists(gameStateFilename) ) {
      EngineConstructGameState(gameStateFilename,bitmapsFilename);
      scenarioName = ReadValueFromScenarioFile(gameStateFilename,"name");
      }
   else if (defaultGameStateFilename&&_OsFileExists(defaultGameStateFilename)){
      EngineConstructGameState(defaultGameStateFilename,bitmapsFilename);
      scenarioName = ReadValueFromScenarioFile(defaultGameStateFilename,
                                               "name");
      }

   rc = SF_GAMEENGINE_OK;
   if ( !game || (game && !game->IsActive()) )
      rc = SF_GAMEENGINE_GAMESTATEFAILED;

   sl = NULL;

   }

SF_GameEngine::~SF_GameEngine() {
  /*
   FILE *fptr = fopen("frontier.msg","w");
   if ( fptr ) {
      int i,j;
      for ( i=0; i<256; i++ )
         for ( j=0; j<256; j++ )
            if ( msgHistograph[i][j] > 0 )
               fprintf(fptr,"slot: %3d msg: %3d --> %6d\n",
                       i,j,msgHistograph[i][j]);
      fclose(fptr);
      }
  */
   if ( scenarioName ) {
      S_free(scenarioName);
      scenarioName = NULL;
      }
   if ( msgBuffer ) {
      delete msgBuffer;
      msgBuffer = NULL;
      }
   if ( sl ) {
      delete sl;
      sl = NULL;
      }
   if ( Dir ) {
      delete[] Dir;
      Dir = NULL;
      }
   if ( game ) {
      delete game;
      game = NULL;
      }
   }


int
SF_GameEngine::FilterKeystroke(char msg,char scancode) {

  /* *** Returning TRUE means to filter out the keystroke. *** */
  /* *** Returning FALSE means to send it on to the engine *** */

  RemapPointers();

  //
  // Returning FALSE tells the client to not process the keystroke.  Disable
  // keyboard input for up to 10 seconds or until NetEngine sets disableInput
  // to zero after receiving the GAMESTATE msg.
  //
  if ( controlsDisabled )
    {
      return TRUE;
    }

   switch(scancode) {

      case SF_KEY_F1 : /* *** OS/2 traps key press event for some reason *** */
         if ( msg == SF_MSG_KEYRELEASED_4 ) {
            for ( int i=0; i<12; i++ )
               de->button[i]->mode = SF_BUTTON_UP;
            de->button[11]->mode = SF_BUTTON_DOWN;
            de->redrawButtons = TRUE;
            SndCueWav(game->soundEvent[push_button1_event],
                      game->soundEvent[push_button1_event]);
            console->SetMode(VIDEO_HELP);
	    }
        return TRUE;

	/*
      case SF_KEY_F3 :
         if ( msg == SF_MSG_KEYPRESSED_4 ) {
            if ( de->video )
               if ( de->video->visible )
                  de->video->SetVisible(FALSE);
               else
                  de->video->SetVisible(TRUE);
	    }
         return TRUE;

      case SF_KEY_F5 :
         if ( msg == SF_MSG_KEYPRESSED_4 ) {
            if ( de->chat )
               if ( de->chat->visible )
                  de->chat->SetVisible(FALSE);
               else
                  de->chat->SetVisible(TRUE);
	    }
         return TRUE;

      case SF_KEY_F4 :
         if ( msg == SF_MSG_KEYPRESSED_4 ) {
            if ( de->map )
               if ( de->map->visible )
                  de->map->SetVisible(FALSE);
               else
                  de->map->SetVisible(TRUE);
	    }
         return TRUE;
	 */

      case SF_KEY_F7 :
         if ( msg == SF_MSG_KEYPRESSED_4 ) {
            if ( de->showDebugString )
               de->showDebugString = FALSE;
            else
               de->showDebugString = TRUE;
            SndCueWav(game->soundEvent[push_button1_event],
                      game->soundEvent[push_button1_event]);
	    }
         return TRUE;

      case SF_KEY_F2 :
         if ( msg == SF_MSG_KEYPRESSED_4 ) {
            de->button[12]->mode = SF_BUTTON_DOWN;
	    if ( de->button[13]->mode == SF_BUTTON_DOWN )
	      {
		de->button[13]->mode = SF_BUTTON_UP;
	      }
	    if ( de->button[14]->mode == SF_BUTTON_DOWN )
	      {
		de->button[14]->mode = SF_BUTTON_UP;
	      }
            de->redrawButtons = TRUE;
	    de->chat->channel = &de->chat->channels[SF_CHANNEL_BROADCAST];
	    de->chat->redrawAll = TRUE;
            SndCueWav(game->soundEvent[push_button1_event],
                      game->soundEvent[push_button1_event]);
            return FALSE;
	    }
         return TRUE;

      case SF_KEY_F3 :
         if ( msg == SF_MSG_KEYPRESSED_4 ) {
	    if ( de->button[12]->mode == SF_BUTTON_DOWN )
	      {
		de->button[12]->mode = SF_BUTTON_UP;
	      }
            de->button[13]->mode = SF_BUTTON_DOWN;
	    if ( de->button[14]->mode == SF_BUTTON_DOWN )
	      {
		de->button[14]->mode = SF_BUTTON_UP;
	      }
            de->redrawButtons = TRUE;
	    de->chat->channel = &de->chat->channels[SF_CHANNEL_TEAM];
	    de->chat->redrawAll = TRUE;
            SndCueWav(game->soundEvent[push_button1_event],
                      game->soundEvent[push_button1_event]);
            return FALSE;
	    }
         return TRUE;

      case SF_KEY_F4 :
         if ( msg == SF_MSG_KEYPRESSED_4 ) {
	    if ( de->button[12]->mode == SF_BUTTON_DOWN )
	      {
		de->button[12]->mode = SF_BUTTON_UP;
	      }
	    if ( de->button[13]->mode == SF_BUTTON_DOWN )
	      {
		de->button[13]->mode = SF_BUTTON_UP;
	      }
            de->button[14]->mode = SF_BUTTON_DOWN;
            de->redrawButtons = TRUE;
	    de->chat->channel = &de->chat->channels[SF_CHANNEL_IRC];
	    de->chat->redrawAll = TRUE;
            SndCueWav(game->soundEvent[push_button1_event],
                      game->soundEvent[push_button1_event]);

	    if ( de && de->chat )
	      {
		de->chat->ConnectIRC();
	      }

            return FALSE;
	    }
         return TRUE;

      case SF_KEY_F5 :
         if ( msg == SF_MSG_KEYPRESSED_4 ) {
            de->button[14]->mode = !de->button[14]->mode;
            de->redrawButtons = TRUE;
            SndCueWav(game->soundEvent[push_button1_event],
            game->soundEvent[push_button1_event]);
            return FALSE;
	    }
         return TRUE;

      case SF_KEY_F8 :
      case SF_KEY_F11 :
	 if ( msg == SF_MSG_KEYPRESSED_4 && ne && sf->connectedThroughNexus) {
            for ( int i=0; i<12; i++ )
               de->button[i]->mode = SF_BUTTON_UP;
            de->button[10]->mode = SF_BUTTON_DOWN;
            de->redrawButtons = TRUE;
            e->client->profile->Update(e);
            e->client->profile->Save();

	    //
	    // Check for RAM cheat first.
	    //
	    int pi = game->ship[localSlotNumber].playerIndex;
	    game->player[pi].AddToStat(game->player[pi].totalScore,
				       (ulong)0
				       );

            int rc = sf->SetPlayerDataOnNexus(sf->profile->nexusName,
                                              sf->profile->players[
                                              sf->profile->selectedPlayer]);
            if ( rc == S_CLIENT_OK )
               SetScreenMessage("Your stats were saved on the nexus",0);
            else
               SetScreenMessage("Your stats were not saved on the nexus",0);
            de->button[10]->mode = SF_BUTTON_UP;
            de->redrawButtons = TRUE;
 	    }
         SndCueWav(game->soundEvent[push_button2_event],
                   game->soundEvent[push_button2_event]);
         return TRUE;


      case SF_KEY_X :
      case SF_KEY_Y :
	if ( game && localSlotNumber >= 0 )
	  {
	    return game->ship[localSlotNumber].IsWarping() ||
	      IsInInternetCafe(&game->ship[localSlotNumber]);
	  }
	return FALSE;

      case SF_KEY_F9 :
      case SF_KEY_F12 :
         if ( msg == SF_MSG_KEYRELEASED_4 && ne ) {

	   if ( !game->ship[localSlotNumber].IsWarping() )
	     {
	       disableInput = frameNumber;
	       disableF9 = frameNumber;
	       
	       ne->slot->writeBuffer.mutex.Request();
	       mstate = 30;
	       ne->slot->writeBuffer.PushMsg((uchar)SF_MSG_GAMESTATE,
					     (ushort)localSlot,
					     (ulong)0,
					     (char *)NULL,
					     (int)S_CLIENT_TCP);
	       mstate = 0;
	       ne->slot->writeBuffer.mutex.Release();
	       SndCueWav(game->soundEvent[push_button3_event],
			 game->soundEvent[push_button3_event]);
	     }
	 }
         return TRUE;

      case SF_KEY_ENTER :
      case SF_KEY_NUM_ENTER :
         return FALSE;

      case SF_KEY_PLUS :
         if ( msg == SF_MSG_KEYPRESSED_4 ) {
            de->tacticalZoom /= 2;
            if ( de->tacticalZoom < 1 )
               de->tacticalZoom = 1;
            de->tacticalScale = de->tacticalZoom;
	    }
         SndCueWav(game->soundEvent[push_button2_event],
                   game->soundEvent[push_button2_event]);
         return TRUE;

      case SF_KEY_MINUS :
         if ( msg == SF_MSG_KEYPRESSED_4 ) {
            de->tacticalZoom *= 2;
            if ( de->tacticalZoom > 32 )
               de->tacticalZoom = 32;
            de->tacticalScale = de->tacticalZoom;
	    }
         SndCueWav(game->soundEvent[push_button1_event],
                   game->soundEvent[push_button1_event]);
         return TRUE;

      case SF_KEY_NUM_PLUS :
         if ( msg == SF_MSG_KEYPRESSED_4 ) {
            de->strategicZoom /= 2;
            if ( de->strategicZoom < 4 )
               de->strategicZoom = 4;
            de->strategicScale = de->strategicZoom;
            if ( de->map )
               de->map->zoom = de->strategicZoom/4;
            if ( de->map )
               de->map->redrawAll = TRUE;
            SndCueWav(game->soundEvent[push_button2_event],
                      game->soundEvent[push_button2_event]);
            }
         return TRUE;

      case SF_KEY_NUM_MINUS :
         if ( msg == SF_MSG_KEYPRESSED_4 ) {
            de->strategicZoom *= 2;
            if ( de->strategicZoom > 512 )
               de->strategicZoom = 512;
            de->strategicScale = de->strategicZoom;
            if ( de->map )
               de->map->zoom = de->strategicZoom/4;
            if ( de->map )
               de->map->redrawAll = TRUE;
            SndCueWav(game->soundEvent[push_button1_event],
                      game->soundEvent[push_button1_event]);
	    }
         return TRUE;

      case SF_KEY_R :
         if ( msg == SF_MSG_KEYPRESSED_4 ) {
            for ( int i=0; i<12; i++ )
               de->button[i]->mode = SF_BUTTON_UP;
            if ( sf->shiftDown ) {
               console->SetMode(VIDEO_RANKLIST);
               de->button[7]->mode = SF_BUTTON_DOWN;
	       }
            else {
               console->SetMode(VIDEO_RACELIST);
               de->button[6]->mode = SF_BUTTON_DOWN;
	       }
            SndCueWav(game->soundEvent[push_button1_event],
                      game->soundEvent[push_button1_event]);
            de->redrawButtons = TRUE;
            return FALSE;
	    }
         return TRUE;

      case SF_KEY_D :
         if ( msg == SF_MSG_KEYPRESSED_4 ) {
            for ( int i=0; i<12; i++ )
               de->button[i]->mode = SF_BUTTON_UP;
            de->button[9]->mode = SF_BUTTON_DOWN;
            de->redrawButtons = TRUE;
            return FALSE;
	    }
         return TRUE;

      case SF_KEY_M :
         if ( msg == SF_MSG_KEYPRESSED_4 ) {
            for ( int i=0; i<12; i++ )
               de->button[i]->mode = SF_BUTTON_UP;
            de->button[3]->mode = SF_BUTTON_DOWN;
            de->redrawButtons = TRUE;
            SndCueWav(game->soundEvent[push_button1_event],
                      game->soundEvent[push_button1_event]);
            console->SetMode(VIDEO_MESSAGE);
            return FALSE;
	    }
         return TRUE;

      case SF_KEY_A :
      case SF_KEY_T :
         if ( msg == SF_MSG_KEYPRESSED_4 ) {
            for ( int i=0; i<12; i++ )
               de->button[i]->mode = SF_BUTTON_UP;
            de->button[1]->mode = SF_BUTTON_DOWN;
            de->redrawButtons = TRUE;
            SndCueWav(game->soundEvent[push_button1_event],
                      game->soundEvent[push_button1_event]);
            console->SetMode(VIDEO_TARGET);
            console->updateIcon = TRUE;
            return FALSE;
	    }
         return TRUE;

      case SF_KEY_N :
         if ( msg == SF_MSG_KEYPRESSED_4 ) {
            for ( int i=0; i<12; i++ )
               de->button[i]->mode = SF_BUTTON_UP;
            de->button[0]->mode = SF_BUTTON_DOWN;
            de->redrawButtons = TRUE;
            SndCueWav(game->soundEvent[push_button1_event],
                      game->soundEvent[push_button1_event]);
            console->SetMode(VIDEO_NAVIGATE);
            return FALSE;
	    }
         return TRUE;

      case SF_KEY_P :
         if ( msg == SF_MSG_KEYPRESSED_4 ) {
            for ( int i=0; i<12; i++ )
               de->button[i]->mode = SF_BUTTON_UP;
            if ( sf->shiftDown )
               de->button[8]->mode = SF_BUTTON_DOWN;
            else
               de->button[4]->mode = SF_BUTTON_DOWN;
            de->redrawButtons = TRUE;
            SndCueWav(game->soundEvent[push_button1_event],
                      game->soundEvent[push_button1_event]);
            console->SetMode(VIDEO_PLAYERLIST);
            return FALSE;
	    }
         return TRUE;

      case SF_KEY_S :
         if ( msg == SF_MSG_KEYPRESSED_4 ) {
            for ( int i=0; i<12; i++ )
               de->button[i]->mode = SF_BUTTON_UP;
            de->button[2]->mode = SF_BUTTON_DOWN;
            console->SetMode(VIDEO_STATUS);
            SndCueWav(game->soundEvent[push_button1_event],
                      game->soundEvent[push_button1_event]);
            SetupConsole();
            de->redrawButtons = TRUE;
            return FALSE;
	    }
         return TRUE;

      /* *** Mask these completely out *** */
      case SF_KEY_E :
         return TRUE;

      /* *** Send only the press event for these *** */
      case SF_KEY_1 :
      case SF_KEY_2 :
      case SF_KEY_3 :
      case SF_KEY_4 :
      case SF_KEY_5 :
      case SF_KEY_6 :
      case SF_KEY_7 :
      case SF_KEY_8 :
      case SF_KEY_9 :
      case SF_KEY_0 :
      case SF_KEY_C :
      case SF_KEY_SPACE :
      case SF_KEY_BACKSPACE :
	 if ( msg == SF_MSG_KEYPRESSED_4 )
            return FALSE;
         return TRUE;

      case SF_KEY_HOME :
         if ( msg == SF_MSG_KEYPRESSED_4 && 
             (console->mode == VIDEO_MESSAGE || console->mode == VIDEO_HELP) ){
            console->ScrollHome();
            return TRUE;
            }
         return FALSE;
      case SF_KEY_LT :
	 if ( msg == SF_MSG_KEYPRESSED_4 ) {
            SndCueWav(game->soundEvent[push_button3_event],
                      game->soundEvent[push_button3_event]);
            if ( console->mode==VIDEO_MESSAGE || console->mode==VIDEO_HELP ) {
               console->ScrollDown();
               return TRUE;
	       }
            }
         return FALSE;
      case SF_KEY_GT :
         if ( msg == SF_MSG_KEYPRESSED_4 ) {
            SndCueWav(game->soundEvent[push_button3_event],
                      game->soundEvent[push_button3_event]);
            if ( console->mode==VIDEO_MESSAGE || console->mode==VIDEO_HELP ) {
               console->ScrollUp();
               return TRUE;
	       }
	    }
         return FALSE;

      }
   return FALSE;
   }


int
GameEngineMsgBoxOkHandler(void *iarg) {
   return 0;
   }


int
SF_GameEngine::RemapPointers()
{
  if ( sf )
    {
      e = sf->engine;
      if ( e )
	{
	  de = e->displayEngine;
	  ne = e->netEngine;
	}
    }
  return 0;
}


int
SF_GameEngine::Update() {

   if ( !game )
      return 0;

   pauseEvent.WaitFor();

   sw.Start();

   SF_Msg msg;

   frameNumber = frame;   /* Patch to be gotten rid of */
   
   controlsDisabled = disableInput > 0 && 
     (disableInput+frameRate*10) > (ulong)frameNumber;

   if ( sf )
     {
       e = sf->engine;
       if ( e )
	 {
	   de = e->displayEngine;
	 }
     }

   EngineStartFrame();

   if ( sf )
     sf->mutex.Release();

   numObjects = EngineQueryNumObjects(numShips,numPlanets,numMissiles);

   int i;

   msgBuffer->mutex.Request();

   /* ************************************* */
   /* *** Extract chat msgs from buffer *** */
   /* ************************************* */
   for ( i=0; i<max_players; i++ )
     {
       msgBuffer->tcpPacket.Rewind();
       while ( msgBuffer->GetNextMsg(i,&msg,S_CLIENT_TCP,SF_MSG_PLAYERCHAT_N) )
	 {
	   RemapPointers();
	   if ( e && e->localPlayerLoggedIn && 
		de && de->chat && de->chat->channels )
	     {
	       if ( IsValidObjectPointer(game->objectList[(short)msg.from]) )
		 {
		   int raceID = game->objectList[(short)msg.from]->raceID;
		   ulong color = raceTextColor[raceID%MAX_COLORS];
		   SF_ChatChannel *channel = NULL;
		   switch ( msg.mode )
		     {
		     case SF_MSG_CHAT_TOALL :
		       channel = &de->chat->channels[SF_CHANNEL_BROADCAST];
		       break;
		     case SF_MSG_CHAT_TORACE :
		       channel = &de->chat->channels[SF_CHANNEL_TEAM];
		       break;
		     }
		   if ( channel )
		     {
		       channel->AppendLine( msg.str.str,color);
		     }
		 }
	     }
       if ( ss )
	 {
	   ss->Log("%s",msg.str.str);
	   if ( ss->window.created )
	     ss->window.Refresh();
	 }
	 }
     }

   //
   // Send out msgs to demo players.
   //
   RemapPointers();
   int ri = game->player[game->ship[localSlotNumber].playerIndex].rank;
   if ( e && e->localPlayerLoggedIn && ri>=13 && 
	sf && sf->demo && de && de->chat && de->chat->channels )
     {
       if ( itime % (16*60*5) == 0 )  // Every 5 minutes
	 {
	   ulong i = ((ulong)rand()) % NUM_AC_MSGS;
	   SF_ChatChannel *channel = 
	     &de->chat->channels[SF_CHANNEL_BROADCAST];
	   channel->AppendLine(_ACMsgs[i],RGB_GREEN);
	 }
     }

   //
   // Send out help msgs.
   //
   RemapPointers();
   if ( e && e->localPlayerLoggedIn && ri<=5 && 
	sf && sf->demo && de && de->chat && de->chat->channels )
     {
       if ( itime % (16*60*5) == 8*60*5 )  // Every 5 minutes
	 {
	   ulong i = ((ulong)rand()) % NUM_HELP_MSGS;
	   SF_ChatChannel *channel = 
	     &de->chat->channels[SF_CHANNEL_BROADCAST];
	   channel->AppendLine(_HelpMsgs[i],RGB_GREEN);
	 }
     }

   //
   // Check for RAM cheat first.
   //
   int pi = game->ship[localSlotNumber].playerIndex;
   game->player[pi].AddToStat(game->player[pi].totalScore,(ulong)0);
   
   /* ***************************************************** */
   /* *** Run engine code for all objects with NULL msg *** */
   /* ***************************************************** */
   msg.msg = SF_MSG_NONE;
   for ( i=0; i<numObjects; i++ ) {     
     msg.slot = i;
     EnginePlusPlus(&msg);
   }
   
   /* **************************** */
   /* *** Get all the TCP msgs *** */
   /* **************************** */
   msgBuffer->Rewind();
   if ( msgBuffer->GetNextMsg(-1,&msg,S_CLIENT_TCP,-1,FALSE) ) {
      msgBuffer->Rewind();
      while ( msgBuffer->GetNextMsg(-1,&msg,S_CLIENT_TCP) ) {

	if (IsServer() ||
	    (ne && ne->numGameStateMsgsRcvd > 0) ||
            (sf && sf->engine && sf->engine->state == SF_CLIENT_SINGLEPLAYER))
	  {

         if ( msg.slot >= 0 && msg.slot < max_players ) {

            /* ******************************************************* */
            /* *** Check to see if a player has the boost key down *** */
            /* ******************************************************* */
            if ( msg.msg==SF_MSG_KEYPRESSED_4 && msg.scancode==SF_KEY_NUM_8 )
               boostOn[msg.slot] = TRUE;

            if ( msg.msg==SF_MSG_KEYRELEASED_4 && msg.scancode==SF_KEY_NUM_8 )
               boostOn[msg.slot] = FALSE;
            /* ******************************************************* */

            /* ******************************************************** */
            /* *** Check to see if a player has an impulse key down *** */
            /* ******************************************************** */
            if ( msg.msg==SF_MSG_KEYPRESSED_4 && 
                (msg.scancode==SF_KEY_NUM_5 || msg.scancode==SF_KEY_NUM_1 ||
                 msg.scancode==SF_KEY_NUM_3 || msg.scancode==SF_KEY_NUM_2) )
               impulseOn[msg.slot] = TRUE;

            if ( msg.msg==SF_MSG_KEYRELEASED_4 && 
                 (msg.scancode==SF_KEY_NUM_5  || msg.scancode==SF_KEY_NUM_1 ||
                 msg.scancode==SF_KEY_NUM_3 ||msg.scancode==SF_KEY_NUM_2 ) )
               impulseOn[msg.slot] = FALSE;
            /* ******************************************************* */
	    }

         if ( msg.msg == SF_MSG_DISCONNECT )
	   {
	     if ( msg.slot == localSlotNumber )
	       {
		 if ( e )
		   {
		     e->run = FALSE;
		     e->selfTermination = TRUE;
		     e->rc = SF_ENGINE_SERVERKICKED;
                     if ( msg.str.str )
		       {
                         e->kickedMsg.Create("%s",msg.str.str);
		       }
		   }
	       }
	     if ( game )
	       {
		 char tmp[128];
		 ObjectUnion tShip;
		 tShip.generic = game->objectList[(short)msg.slot];
		 if ( IsValidObjectPointer(tShip.generic) )
		   {
		     if ( tShip.ship->active )
		       {
			 sprintf(tmp,"Goodbye %s",
				 game->player[tShip.ship->playerIndex].name);
			 if ( msg.str.str )
			   {
			     sprintf(tmp,"%s (%s)",tmp,msg.str.str);
			   }
			 RemapPointers();
			 if ( de && de->chat && de->chat->channels )
			   {
			     int raceID = tShip.generic->raceID;
			     ulong color = raceTextColor[raceID%MAX_COLORS];
			     SF_ChatChannel *channel = 
			       &de->chat->channels[SF_CHANNEL_BROADCAST];
			     channel->AppendLine(tmp,color);
			   }
		       }
		   }
	       }
	   }

         if ( msg.msg == SF_MSG_PLAYERINFO ) {
	   if ( e )
	     e->startFrame = itime;
	   if ( ss )
	     {
	       msg.player.ipAddress.Create(ss->slot[msg.slot].ipAddress.str);
	       msg.player.serial = ss->slot[msg.slot].serial;
	     }
	   FillSlot(msg.slot,&msg.player);
	   
            if ( game ) {
	      char tmp[128];
	      ObjectUnion tShip;
	      tShip.generic = game->objectList[(short)msg.slot];
	      
	      if ( IsValidObjectPointer(tShip.generic) )
		{
		  
		  if ( tShip.ship->playerIndex >= 0 &&
		       tShip.ship->playerIndex < game->maxStoredPlayers &&
		       game->player[tShip.ship->playerIndex].raceId >= 0 &&
		       game->player[tShip.ship->playerIndex].raceId < 
		       game->numRaces )
		    {
		      sprintf(tmp,"%s logged in as a %s",
			      game->player[tShip.ship->playerIndex].name,
			      game->race[game->player[tShip.ship->playerIndex].
					raceId].name);
		    }
		  RemapPointers();
		  if ( de && de->chat && de->chat->channels )
		    {
		      int raceID = tShip.generic->raceID;
		      ulong color = raceTextColor[raceID%MAX_COLORS];
		      SF_ChatChannel *channel = 
			&de->chat->channels[SF_CHANNEL_BROADCAST];
		      channel->AppendLine(tmp,color);
		    }

		}
	    }
	    
	    if ( e && msg.slot == localSlot )
	      {
		e->localPlayerLoggedIn = itime + 1;
	      }

	    RemapPointers();
            if ( de && msg.slot == localSlot ) {
               de->repaint = TRUE;
               de->show = TRUE;

               if ( sf->msgBox )
		 {
                   sf->msgBox->Hide();
		 }

               if ( sf && sf->serverList && sf->serverIndex >=0 )
		 {
		   S_String windowTitle(
					"Stellar Frontier v%s | %s | %s",
					sf->VersionString(),
					sf->serverList[sf->serverIndex].
					serverName,
					sf->serverList[sf->serverIndex].
					hostName);
                   if ( sf->mainWindow && windowTitle.str )
		     {
                       sf->mainWindow->SetText(windowTitle.str);
		     }
		 }

               /* ****************************************** */
               /* *** Send msg to acquire nearest target *** */
               /* ****************************************** */
               if ( sf && sf->engine )
                  sf->engine->PushKeyboardMsg(SF_MSG_KEYPRESSED_4,SF_KEY_A);

	       }

	    //
	    // Request dev mode status
	    //
	    if ( ne && ne->slot && msg.slot == localSlot )
	      {
		ne->slot->writeBuffer.PushMsg((uchar)SF_MSG_REQUESTDEV,
					      (ushort)localSlot,
					      (uchar)0,
					      (int)S_CLIENT_TCP);
	      }
	 }

         if ( sf && sf->engine && msg.msg == SF_MSG_RESETGAME &&
              sf->engine->state == SF_CLIENT_CONNECTED ) {
            char tmp[1024];
            sprintf(tmp,"THE %ss HAVE CONQUERED THE SYSTEM!!!",
                    game->race[msg.race].name);
            SetScreenMessage(tmp,CLR_WHITE);

            /* *************************************** */
            /* *** Send msg to save stats on nexus *** */
            /* *************************************** */
            if ( sf && sf->engine )
               sf->engine->PushKeyboardMsg(SF_MSG_KEYPRESSED_4,SF_KEY_F8);

            }

         if ( msg.slot >= 0 && msg.slot < 256 )
            msgHistograph[msg.slot][msg.msg]++;

         EnginePlusPlus( &msg );
	  }

      }
   }

   if ( sf )
      sf->mutex.Request();

   /* **************************** */
   /* *** Get all the UDP msgs *** */
   /* **************************** */
   msgBuffer->Rewind();
   if ( msgBuffer->GetNextMsg(-1,&msg,S_CLIENT_UDP,-1,FALSE) ) {
      msgBuffer->Rewind();
      while ( msgBuffer->GetNextMsg(-1,&msg,S_CLIENT_UDP) ) {
	
	if (IsServer() ||
	    (ne && ne->numGameStateMsgsRcvd > 0) ||
	    (sf && sf->engine && sf->engine->state == SF_CLIENT_SINGLEPLAYER))
	  {
	    if ( msg.slot >= 0 && msg.slot < 256 )
	      msgHistograph[msg.slot][msg.msg]++;
	    
	    EnginePlusPlus( &msg );
	  }
      }
      
   }
 
   msgBuffer->mutex.Release();

   retMsgBuffer = EngineCloseFrame();

   /* ******************************************************************** */
   /* *** Look for local player explode decrement numLives if required *** */
   /* ******************************************************************** */
   retMsgBuffer->Rewind();
   while ( retMsgBuffer->GetNextMsg(localSlot,&msg,S_CLIENT_TCP,
                                    SF_MSG_EXPLODESHIP,FALSE) )
      numLives--;
   /* ******************************************************************** */

   itime++;
   frame++;

   sw.Stop();

   return 0;
   }


int
SF_GameEngine::CheckObjectCPUWarning(slong seg,slong slot,slong msg,slong frame,
                                     slong dt) {
   if ( dt > objectCPUWarningTime ) {
      objectCPUWarningSegment = seg;
      objectCPUWarningMsg = msg;
      objectCPUWarningFrame = frame;
      objectCPUWarning = slot;
      objectCPUWarningTime = dt;
      }
   return 0;
   }


int
SF_GameEngine::FillSlot(int islot,SF_Player *player) {
   game->playerList[islot] = game->AddPlayer();
   if ( player->race < 0 )
     {
       player->race = 0;
     }
   if ( player->shipID < 0 )
     {
       player->shipID = 0;
     }
   if ( e && e->state == SF_CLIENT_SINGLEPLAYER )
     {
       *(game->playerList[islot]) = PlayerClass(player->playerName.str,
						player->playerName.str,
						player->shipName.str,
						player->ipAddress.str,
						(ulong)player->serial,
						(slong)player->race,
						(slong)player->shipID,
						(slong)player->score,
						(slong)player->timeOnline,
						(slong)player->kills,
						(slong)player->losses,
						(char)player->rank,
						(char)player->demo,
						(slong)player->credits);
     }
   else if ( (e && e->state == SF_CLIENT_CONNECTED) || 
             (sf && sf->engine == NULL) )
     {
       *(game->playerList[islot]) = PlayerClass(player->playerName.str,
						player->playerName.str,
						player->shipName.str,
						player->ipAddress.str,
						(slong)player->serial,
						(slong)player->race,
						(slong)player->shipID,
						(slong)player->score,
						(slong)player->timeOnline,
						(slong)player->kills,
						(slong)player->losses,
						(char)player->rank,
						(char)player->demo,
						(slong)player->credits);
     }
   game->ship[islot].playerIndex = 
     game->player.indexOf(game->playerList[islot]);
   game->ship[islot].activate();
   game->ship[islot].raceID = game->playerList[islot]->raceId;
   game->MapPlayerShipIdToIndex(game->ship[islot].playerIndex);
   
   return 0;
   }


int
_BmpDownloadCloseFunc(void *iarg)
{
  if ( sf && 
       sf->engine && sf->engine->check == 0xAAAAAAAA &&
       sf->engine->msl && sf->engine->msl->check == 0xAAAAAAAA &&
       sf->engine->gameEngine && sf->engine->gameEngine->check == 0xAAAAAAAA )
    {
      for ( int i=0; i<sf->engine->msl->numSprites; i++ )
	{
	  SF_Sprite *spr = sf->engine->msl->Sprite(i);
	  if ( spr && spr->check == 0xAAAAAAAA && spr->rc < 0 )
	    {
	      spr->Reload();
	      if ( spr == sf->engine->gameEngine->bgSprite || 
                   spr == sf->engine->gameEngine->bg2Sprite )
		{
		  sf->engine->gameEngine->OverlayBGSprites();
		}
	    }
	}
      EnableLoginDialogButtons(TRUE);
    }
  return 0;
}


int
_WavDownloadCloseFunc(void *iarg) {
   for ( int i=0; i<sf->mwl->numWaves; i++ ) {
      S_WaveData *w = sf->mwl->Wave(i);
      if ( w->rc < 0 )
         w->Load();
      }
   EnableLoginDialogButtons(TRUE);
   return 0;
   }


int
SF_GameEngine::InitGUIClient() {

  ne->dontTimeout = TRUE;

   DrawLoginStatusBar();

   if ( sl )
      delete sl;
   sl = new SF_SpriteList();

   int i;
   /* *** Add any new sprites to the Master sprite list (msl) *** */
   for ( i=0; i<game->numBitmaps; i++ ) {
     ne->ResetTimeout();
     if ( game->numBitmaps > 0 )
       {
	 DrawLoginStatusBar(i*100/game->numBitmaps);
       }
      if ( e->run ) {
         SF_Sprite *sprite = e->msl->Sprite(game->objectBitmapList[i].file);
         if ( sprite == NULL ) {
            char *path = "..\\bmp\\256\\";

            char *clas = game->objectBitmapList[i].Class;
            char *name = game->objectBitmapList[i].name;
            char *file = game->objectBitmapList[i].file;

            char *filename = NULL;
            ulong maskFlags;
            ulong blitFlags;

            int len = strlen(path) + strlen(file) + 1;
            if ( strcmp(clas,"miscellaneous") == 0 &&
		 strcmp(name,"Background") == 0 ) {
               int cx = _root->cx;
               len += 6;
               filename = (char *)S_malloc(len*sizeof(char));
               if ( filename )
                  sprintf(filename,"%s%s%d",path,file,cx);
               maskFlags = S_BITMAP_MASK_NONE;
               blitFlags = S_BITMAP_BLIT_OPAQUE;
               }
            else 
            if ( ( strcmp(clas,"miscellaneous") == 0 &&
                   strcmp(name,"Halftone") == 0 ) ||
                 ( strcmp(clas,"miscellaneous") == 0 &&
                   strcmp(name,"Video") == 0 ) ||
                 ( strcmp(clas,"miscellaneous") == 0 &&
                   strcmp(name,"Map") == 0 ) ||
                 ( strcmp(clas,"miscellaneous") == 0 &&
                   strcmp(name,"Chat") == 0 ) ||
                 ( strcmp(clas,"miscellaneous") == 0 &&
                   strcmp(name,"LeftPanel") == 0 ) ||
                 ( strcmp(clas,"miscellaneous") == 0 &&
                   strcmp(name,"RightPanel") == 0 ) ||
                 ( strcmp(clas,"miscellaneous") == 0 &&
                   strcmp(name,"Background2") == 0 ) ) {
               filename = (char *)S_malloc(len*sizeof(char));
               if ( filename )
                  sprintf(filename,"%s%s",path,file);
               maskFlags = S_BITMAP_MASK_NONE;
               blitFlags = S_BITMAP_BLIT_OPAQUE;
	       }
            else {
               filename = (char *)S_malloc(len*sizeof(char));
               if ( filename )
                  sprintf(filename,"%s%s",path,file);
               maskFlags = S_BITMAP_MASK_TRANSLUCENT;
               blitFlags = S_BITMAP_BLIT_USEMASK;
	       }

            if ( filename ) {
               sprite = new SF_Sprite(file,filename,maskFlags,blitFlags,
                                      game->objectBitmapList[i].numFrames,
                                      game->objectBitmapList[i].gamma,
                                      game->objectBitmapList[i].alpha);
               if ( sprite && sprite->IsACOnly(name) )
		 {
                   game->objectBitmapList[i].acOnly = TRUE;
		 }
               e->msl->Add(sprite);
               S_free(filename);
	       }
	    }
         }
      }

   if ( game->numBitmaps > 0 )
     {
       DrawLoginStatusBar(i*100/game->numBitmaps);
     }

   /* ************************************************************ */
   /* *** Check to see if any bitmaps failed to load correctly *** */
   /* ************************************************************ */
   int num = 0;
   for ( i=0; i<e->msl->numSprites; i++ ) {

      SF_Sprite *spr = e->msl->Sprite(i);
      if ( spr->rc < 0 )
         num++;
      }

   if ( num > 0 ) {

      if ( e->client->files ) {
         for ( int i=0; i<e->client->numFiles; i++ )
            if ( e->client->files[i] ) {
               S_free(e->client->files[i]);
               e->client->files[i] = NULL;
	       }
         e->client->numFiles = 0;
         S_free(e->client->files);
         e->client->files = NULL;
         }

      e->client->numFiles = 0;
      e->client->files = (char **)S_malloc(sizeof(char *)*num);
      for ( i=0; i<e->msl->numSprites; i++ ) {
         SF_Sprite *spr = e->msl->Sprite(i);
         if ( spr->rc < 0 ) {
            char *filename = (char *)S_malloc( (strlen(spr->filename)+5) *
                                             sizeof(char) );
            if ( filename ) {
               sprintf(filename,"%s.bmp",spr->filename);
               e->client->files[e->client->numFiles++] = S_strdup(filename);
               S_free(filename);
               }
            }
         }

      if ( sf->engine->state == SF_CLIENT_CONNECTED &&
           sf->downloadDialog && num > 0 ) {
         EnableLoginDialogButtons(FALSE);
         e->client->downloadDialog->Show();
         e->client->downloadDialog->StartThread(e->serverName,
                                                sf->serverPort,
                                                e->client->files,
                                                e->client->numFiles,
                                                _BmpDownloadCloseFunc,NULL);
         }
      }
   /* ************************************************************ */

   /* *** Now construct the sprite list from the msl *** */
   for ( i=0; i<game->numBitmaps; i++ ) {
      if ( e->run ) {
        char *clas = game->objectBitmapList[i].Class;
         char *name = game->objectBitmapList[i].name;
         char *file = game->objectBitmapList[i].file;

         SF_Sprite *sprite = e->msl->Sprite(file);
	 
         if (strcmp(clas,"miscellaneous") == 0 && strcmp(name,"Halftone")==0)
            halftoneSprite = sprite;
         if (strcmp(clas,"miscellaneous") == 0 && strcmp(name,"Video") == 0) {
            videoSprite = sprite;
            if ( videoSprite->rc ) {
               videoSprite->cx = 320;
               videoSprite->cy = 200;
               }
            de->video->SetBmpBack(sprite);
	    }
         if (strcmp(clas,"miscellaneous") == 0 && strcmp(name,"Map") == 0) {
            mapSprite = sprite;
            if ( mapSprite->rc ) {
               mapSprite->cx = 320;
               mapSprite->cy = 320;
               }
            de->map->SetBmpBack(sprite);
	    }
         if (strcmp(clas,"miscellaneous") == 0 && strcmp(name,"Chat") == 0) {
            chatSprite = sprite;
            if ( chatSprite->rc ) {
               chatSprite->cx = 320;
               chatSprite->cy = 640;
               }
            de->chat->SetBmpBack(sprite);
	    }
         if (strcmp(clas,"miscellaneous") == 0 && strcmp(name,"Scroll") == 0) {
            scrollSprite = sprite;
            if ( scrollSprite->rc ) {
               chatSprite->cx = 16;
               chatSprite->cy = 16;
               }
            de->chat->SetBmpScroll(sprite);
	    }
         if (strcmp(clas,"miscellaneous")==0 && strcmp(name,"Shield")==0)
            shldSprites[sprite->cx - 8] = sprite;
         if (strcmp(clas,"miscellaneous")==0 && strcmp(name,"LeftPanel")==0)
            lpanelSprite = sprite;
         if (strcmp(clas,"miscellaneous")==0 && strcmp(name,"RightPanel")==0)
            rpanelSprite = sprite;
         if (strcmp(clas,"miscellaneous") == 0 && strcmp(name,"Debris") == 0)
            debrisSprite = sprite;
         if (strcmp(clas,"miscellaneous") == 0 && strcmp(name,"MainFont")==0){
            mainFontSprite = sprite;
            de->mainFont->spr = sprite;
	    }
         if (strcmp(clas,"miscellaneous")==0&&strcmp(name,"Background")==0) {
            bgSprite = sprite;
            OverlayBGSprites();
	    }
         if (strcmp(clas,"miscellaneous")==0&&strcmp(name,"Background2")==0) {
            bg2Sprite = sprite;
            OverlayBGSprites();
	    }
         if (strcmp(clas,"explosion")==0&&strcmp(name,"DebrisTrans")==0)
            powerupTransSprite = sprite;

         sl->Add(sprite);
         }
      }


   /* ******************************************************************* */
   /* *** Now load the sounds from gamestate into the master wav list *** */
   /* ******************************************************************* */
   if ( game->numSounds > 0 ) {

      if ( sf->mwl ) {
         delete sf->mwl;
         sf->mwl = new SF_WaveList();
         }

      if ( sf->mwl ) {

         for ( i=0; i<game->numSounds; i++ ) {
            char filename[1024];
            sprintf(filename,"..\\wav\\%s.wav",game->sound[i].file);
            S_WaveData *w = new S_WaveData(filename,
                                           game->sound[i].priority,
                                           game->sound[i].loop);
            sf->mwl->Add( w );
            w->SetVolume(game->sound[i].volume);
	    }

         sf->mwl->SetVolume(sf->profile->soundVolume);

         /* *************************************************************** */
         /* *** Check to see if any wave files failed to load correctly *** */
         /* *************************************************************** */
         if ( sf->engine->state == SF_CLIENT_SINGLEPLAYER ) {
            int i;
            int loadErr = FALSE;
            char errStr[1024];
            sprintf(errStr,"The following wave files failed to load: ");
            for ( i=0; i<sf->mwl->numWaves; i++ ) {
               S_WaveData *w = sf->mwl->Wave(i);
               if ( w->rc != 0 ) {
                  loadErr = TRUE;
                  sprintf(errStr,"%s%s ",errStr,w->name);
                  }
               }
            if ( loadErr )
               new S_MsgBox(sf->mainWindow,"WARNING!",errStr,S_MSGBOX_OK);
            }
         /* *************************************************************** */


         /* *********************************************************** */
         /* *** Check to see if any sounds failed to load correctly *** */
         /* *********************************************************** */
         int num = 0;
         for ( i=0; i<sf->mwl->numWaves; i++ ) {
            S_WaveData *w = sf->mwl->Wave(i);
            if ( w->rc < 0 )
               num++;
            }

         if ( num > 0 ) {

            if ( e->client->wfiles ) {
               for ( int i=0; i<e->client->numwFiles; i++ )
                  if ( e->client->wfiles[i] ) {
                     S_free(e->client->wfiles[i]);
                     e->client->wfiles[i] = NULL;
                     }
               e->client->numwFiles = 0;
               S_free(e->client->wfiles);
               e->client->wfiles = NULL;
               }

            e->client->numwFiles = 0;
            e->client->wfiles = (char **)S_malloc(sizeof(char *)*num);
            for ( i=0; i<sf->mwl->numWaves; i++ ) {
               S_WaveData *w = sf->mwl->Wave(i);
               if ( w->rc < 0 ) {
                  char *filename = (char *)S_malloc( (strlen(w->name)+1) *
                                                      sizeof(char) );
                  if ( filename ) {
                     sprintf(filename,"%s",w->name);
                     e->client->wfiles[e->client->numwFiles++] = 
                        S_strdup(filename);
                     S_free(filename);
                     }
                  }
               }

            if ( sf->engine->state == SF_CLIENT_CONNECTED &&
                 sf->wdownloadDialog && num > 0 ) {
               EnableLoginDialogButtons(FALSE);
               e->client->wdownloadDialog->Show();
               e->client->wdownloadDialog->StartThread(e->serverName,
                                                       sf->serverPort,
                                                       e->client->wfiles,
                                                       e->client->numwFiles,
                                                       _WavDownloadCloseFunc,
                                                       NULL);
               }
            else
               EnableLoginDialogButtons(TRUE);
            }
         else
            EnableLoginDialogButtons(TRUE);
         }
      }
   /* ********************************************************************** */

   guiInitDone = TRUE;

   ne->ResetTimeout();
   ne->dontTimeout = FALSE;

   return 0;
   }


int
SF_GameEngine::OverlayBGSprites() {
   if ( !bgSprite || !bg2Sprite )
      return 0;

   int cx = bgSprite->cx - sf->cxPanel;
   int cy = bgSprite->cy - sf->cyPanel;

   int x1targ = 0;
   int y1targ = 0;
   int x2targ = cx;
   int y2targ = cy;

   int x1src = 0;
   int y1src = 0;
   int x2src = bg2Sprite->cx;
   int y2src = bg2Sprite->cy;

   if ( (int)bg2Sprite->cx < cx || (int)bg2Sprite->cy < cy )
      bg2Sprite->SystemBlit(bgSprite,
                            x1targ,y1targ,x2targ,y2targ,
                            x1src,y1src,x2src,y2src);
   else
      bg2Sprite->SystemBlit(bgSprite,
                            x1targ,y1targ,x2targ,y2targ,
                            x1src,y1src);
   return 0;
   }


int
IsWhiteSpace(char ch) {
   return ch == ' ' || ch == '\t';
   }

/* *** Strip out Quotes " and \t *** */
char *
CleanString(char *str) {
   if ( str ) {
      int i,j;
      int len = strlen(str);
      for ( i=0; i<len; i++ ) {
         while ( str[i] == '"' || str[i] < 32 && i < len ) {
            for ( j=i; j<len; j++ )
               str[j] = str[j+1];
            len--;
            }
         }
      }
   return str;
   }


char *
ReadValueFromScenarioFile( char *fileName,char *keyword ) {
   FILE *fptr;
   char line[1024];
   char *serverName = NULL;

   fptr = fopen(fileName,"r");
   if ( fptr == NULL ) {
      printf("ERROR -- Could not open %s.\n",fileName);
      serverName = S_strdup("No Name");
      return serverName;
      }

   int inGameData = FALSE;
   char *rc = NULL;
   do {
      char **tokens;
      int i = 0,j,k;

      rc = fgets(line,1023,fptr);
      for ( i=0; i<(int)strlen(line); i++ )
         if ( line[i] == '\n' )
            line[i] = 0;
      int len = strlen(line);

      int numTokens = 0;
      tokens = NULL;

      i = 0;
      while ( rc && i < len ) {

         if ( numTokens == 0 )
            tokens = (char **) S_malloc( sizeof(char *) );
         else
            tokens = (char **) S_realloc( tokens,sizeof(char *)*(numTokens+1) );

         while ( IsWhiteSpace(line[i]) && i<len ) /* Strip leading spaces */
            i++;
         j = i;
         while ( !IsWhiteSpace(line[j]) && !IsWhiteSpace(line[j+1]) && j<len )
            j++;
         j++;
         tokens[numTokens] = (char *) S_malloc( (j-i+1)*sizeof(char) );
         for ( k=i; k<j; k++ )
            tokens[numTokens][k-i] = line[k];
         tokens[numTokens][k-i] = 0;
         numTokens++;
         i = j;

         }

      if ( numTokens == 3 ) {
         int a = strcmp(tokens[0],"game");
         int b = strcmp(tokens[1],"data");
         int c = strcmp(tokens[2],"{");
         if ( a==0 && b==0 && c==0 )
            inGameData = TRUE;
         }

      if ( numTokens == 1 && strcmp(tokens[0],"}") == 0 ) {
        inGameData = FALSE;
        rc = NULL;
        }

      if ( inGameData && numTokens > 1 && strcmp(tokens[0],keyword) == 0 ) {
         serverName = (char *)S_malloc(strlen(line)+1);
         serverName[0] = 0;
         for ( i=1; i<numTokens; i++ ) {
            if ( serverName[0] == 0 )
               sprintf(serverName,"%s",tokens[i]);
            else {
               char tmp[1024];
               sprintf(tmp,serverName);
               sprintf(serverName,"%s %s",tmp,tokens[i]);
	       }
            }
         }

       if ( tokens ) {
 	  for ( i=0; i<numTokens; i++ ) {
	     if ( tokens[i] ) {
                S_free(tokens[i]);
                tokens[i] = NULL;
	        }
             }
          S_free(tokens);
          tokens = NULL;
          }

      } while ( rc != NULL && serverName == NULL );

   fclose(fptr);

   CleanString(serverName);

   return(serverName);
   }


int
SF_DebugMem_Check(void *ptr,char *str) {
   if ( S_DebugMem_Check(ptr) ) {
      ::logFile << "SF_DebugMem_Check FAILED : " << str << endl;
      return -1;
      }
   return 0;
   }
