/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
#include "sf.h"
#include "SF_PortCpp.h"
#include "SF_Externals.h"
#include "SF_ClientWindow.h"
#include "SF_Globals.h"
#include "SF_Client.h"
#include "SF_Server.h"
#include "SF_ServerWindow.h"
#include "SF_Slot.h"
#include "SF_Player.h"
#include "SF_GameEngine.h"
#include "SF_Nexus.h"
#include "SF_Msg.h"
#include "SF_Sprite.h"
#ifdef SFHOST
#  include "sf_net_api.h"   /* port 2026-10-12: portable sockets */
#endif

int _ServerRunThread(void *);
int _ServerEngineThread( void * );
int _NexusVerifyThread( void * );
char *_Server_TimeStamp();


int
SF_Server::Init() {
   runThread = NULL;
   guiMode = SF_SERVER_NOGUI;
   thread = NULL;
   run = FALSE;
   nexusVerifyThread = FALSE;
   slot = NULL;
   ge = NULL;
   nexusName = NULL;
   altNexusName = NULL;
   registerWithNexus = FALSE;
   everybodyGo = FALSE;
   ss = this;
   timeOutSec = SF_TIMEOUT_SEC;
   lagMax = SF_SERVER_MAXLAGMS;
   maxGameStateMsgs = 0;
   minGameStateMsgPeriodicitySec = SF_MINF9PERIOD_SEC;
   startInTray = FALSE;
   S_FileServer::Init();
   S_Server::dynamicDenyIPList = TRUE;
   return 0;
   }

SF_Server::SF_Server() {
   Init();
   }


int
SF_Server::Create(char *inexusName,int iguiMode,char *iautoStart,
		  int iport,int ilog,char *ialtNexusName,int istartInTray) {
   S_FileServer::Create(iport,ilog,"server.log");

   guiMode = iguiMode;
   startInTray = istartInTray;

   if ( inexusName ) {
      nexusName = S_strdup(inexusName);
      registerWithNexus = TRUE;
      }

   if ( ialtNexusName ) {
      altNexusName = S_strdup(ialtNexusName);
      }

   if ( guiMode == SF_SERVER_GUI ) {
      window.Create(_root,this);
      if ( iautoStart ) {
         window.startButton.SetEnabled(FALSE);
         window.stopButton.SetEnabled(TRUE);
         window.warnButton.SetEnabled(TRUE);
         window.kickButton.SetEnabled(TRUE);
         Start(iautoStart);
         }
      }
   else
      Start(iautoStart);

   Log("");
   Log("Stellar Frontier Server v%s",SF_Client::VersionString());
   Log("Copyright (c) 1997-2003 Stardock Systems");
   Log("All rights reserved");
   Log("");

   if ( window.created )
      window.Refresh();

   return 0;
   }


int
SF_Server::Start(char *iscenarioName) {

   if ( rc != S_SERVER_OK ) {
      Log("Server failed to start network engine.");
      Log("It is possible there is already a server running.");
      Log("Or a previous instance of the server left a hung socket.");
      Log("");
      Log("You must exit the server and restart it.");
      if ( window.created ) {
         window.stopButton.SetEnabled(FALSE);
         window.warnButton.SetEnabled(FALSE);
         window.kickButton.SetEnabled(FALSE);
         window.startButton.SetEnabled(TRUE);
         }
      return 0;
      }

   maxSlots = max_players;
   slot = new SF_Slot[maxSlots];

   S_String scenarioName;

   if ( iscenarioName )
     {
#      ifdef UNIX
       scenarioName.Create("../dat/%s",iscenarioName);
#      else
       scenarioName.Create("..\\dat\\%s",iscenarioName);
#      endif
     }
   else 
     {
       if ( window.created )
	 {
	   int index = window.scenarioList.selectedLine;
	   if ( index >= 0 )
	     {
	       scenarioName.Create(window.dirList->file[index]);
	     }
	 }
     }

   ge = new SF_GameEngine(NULL,scenarioName.str);
   
   /* ************************************************************ */
   /* *** debug code to verify data integrity across platforms *** */
   /* ************************************************************ */
#  ifdef SF_DEVMODE
     ulong gameDataSize = game->Size();
      if ( gameDataSize > 0 ) {
         char *gameData = (char *) S_malloc( gameDataSize );
         if ( gameData ) {
            gameDataSize = EngineGetGameState( gameData );
            FILE *fptr = fopen("game.out","wb");
            if ( fptr ) {
               fwrite(gameData,1,gameDataSize,fptr);
               fclose(fptr);
               }
            }
         }
#  endif
   /* ************************************************************ */

   serverName = ge->scenarioName;

   run = TRUE;
   everybodyGo = FALSE;

   thread = new S_Thread(_ServerEngineThread,(void *)this);

   nexusVerifyThread = NULL;
   if ( nexusName && registerWithNexus ) {
      nexusVerifyThread = new S_Thread(_NexusVerifyThread,(void *)this);
      }

   if ( window.created ) {
      window.console.scroll = 0;
      window.RefreshPlayerList();
      runThread = new S_Thread(_ServerRunThread,(void *)this);
      }

   return 0;
   }


int
_ServerRunThread(void *arg) {
   SF_Server *server = (SF_Server *)arg;
   server->S_Server::dynamicDenyIPList = TRUE;
   server->S_Server::Run();
   return 0;
   }


int
SF_Server::Run() {
   if ( guiMode == SF_SERVER_GUI )
      _root->Run();
   else
      S_Server::Run();
   return 0;
   }


int
SF_Server::Stop() {

   everybodyGo = TRUE;

   //
   // For some reason there are children even when nobody is logged in?
   // So this finally times out.  Comment out until this is figured out.
   //
   //WaitForChildren();

   S_Server::Stop();
   if ( runThread ) {
      runThread->WaitFor();
      delete runThread;
      runThread = NULL;
      }

   if ( slot ) {
      delete[] slot;
      slot = NULL;
      }

   run = FALSE;
   if ( thread ) {
      thread->WaitFor();
      delete thread;
      thread = NULL;
      }

   if ( nexusVerifyThread ) {
      nexusVerifyThread->WaitFor();
      delete nexusVerifyThread;
      nexusVerifyThread = NULL;
      }

   if ( ge ) {
      delete ge;
      ge = NULL;
      }

   if ( window.created )
      window.playerList.Flush();

   everybodyGo = FALSE;

   Log("Server stopped.");
   Log("");

   if ( window.created )
      window.Refresh();

   return 0;
   }


int
SF_Server::Destroy() {
   Stop();

   if ( nexusName ) {
      S_free(nexusName);
      nexusName = NULL;
     }
   if ( altNexusName ) {
      S_free(altNexusName);
      altNexusName = NULL;
     }
   S_FileServer::Destroy();
   return 0;
   }

SF_Server::~SF_Server() {
   Destroy();
   }


int
SF_Server::ProcessConnectMsg(S_Client *client) {

   switch(client->connectMsg) {

      case S_FILESERVER_REQUESTFILELIST :
         HandleRequestFileListMsg(client);
         break;
      case S_FILESERVER_REQUESTFILESTATUS :
         HandleRequestFileStatusMsg(client);
         break;
      case S_FILESERVER_REQUESTFILEMODDATE :
         HandleRequestFileModDateMsg(client);
         break;
      case S_FILESERVER_REQUESTFILESIZE :
         HandleRequestFileSizeMsg(client);
         break;
      case S_FILESERVER_REQUESTFILE :
         HandleRequestFileMsg(client);
         break;

      case SF_SERVER_DOWNLOADFILE :
	//DownloadFile(client);
         HandleRequestFileMsg(client);
         break;

      case SF_SERVER_TOUCH : {
         int i,j;
         S_Packet packet;
         ushort numLoops,numBytes;

         client->ReadPacket(&packet);
         numLoops = packet.PopUShort();
         numBytes = packet.PopUShort();
         packet.Flush();

         packet.PushUShort(numPlayers+game->numShips-game->maxHumanShips);
         packet.PushUShort(game->numShips);
         packet.PushUChar(SF_MAJOR_VERSION); 
         packet.PushUChar(SF_MINOR_VERSION); 
         packet.PushString(_OsString());
         packet.PushString(serverName);
         packet.PushUChar(SF_MINOR_MINOR_VERSION); 
         packet.PushString(SF_VERSION_MODIFIER);
         packet.PushString(SF_VERSION_STRING);
	 packet.PushUShort(numPlayers);
	 packet.PushULong(frameNumber/frameRate);

         client->WritePacket(&packet);
         packet.Flush();

         for ( i=0; i<numLoops; i++ ) {
            client->ReadPacket(&packet);
            packet.Flush();

            for ( j=0; j<numBytes; j++ )
               packet.PushUChar((uchar)i);
            client->WritePacket(&packet);
            packet.Flush();
	    }

         break;
         }

      case SF_SERVER_CONNECT : {
         int i;

         slotMutex.Request();
         uchar islot = SF_SLOT_SERVERFULL;
         for (i=0;i<game->maxHumanShips;i++) {
            if ( islot == 255 && !slot[i].filled ) {
               numPlayers++;
               islot = i;
               slot[islot].Init();
               slot[i].filled = TRUE;
	       slot[i].ipAddress.Create(client->numIPAddress);
               }
            }
         slotMutex.Release();

         S_Packet packet;
         client->ReadPacket(&packet);
         char majorVersion = packet.PopChar();
         char minorVersion = packet.PopChar();
         /*char minorMinorVersion = */packet.PopChar();
         packet.Flush();

         ulong cversion = /*minorMinorVersion +*/
                          1000*minorVersion + 
                          1000000*majorVersion;

         ulong sversion = /*SF_MINOR_MINOR_VERSION +*/
                          1000*SF_MINOR_VERSION + 
                          1000000*SF_MAJOR_VERSION;

         if ( cversion < sversion ) {
            numPlayers--;
	    slot[i].ipAddress.Create("");
            slot[islot].filled = FALSE;
	    slot[islot].serial = 0;
            islot = SF_SLOT_OLDVERSION;
	    }
         else if ( cversion > sversion ) {
            numPlayers--;
	    slot[i].ipAddress.Create("");
            slot[islot].filled = FALSE;
	    slot[islot].serial = 0;
            islot = SF_SLOT_WRONGVERSION;
	    }

         client->WriteShort(islot,S_CLIENT_TCP);

         if ( islot == SF_SLOT_WRONGVERSION ||
              islot == SF_SLOT_OLDVERSION   ||
              islot == SF_SLOT_SERVERFULL )
            break;

         S_Timer timer(10000,sf->stopWatchType);
         slot[islot].continueConnection = TRUE;
         slot[islot].kicked = FALSE;

         int numObjectTrackMsgsSkipped[max_players];
         for ( i=0; i<game->numShips; i++ )
            numObjectTrackMsgsSkipped[i] = 0;

         SF_Msg msg;
         SF_MsgBuffer *wb = &slot[islot].writeBuffer;
         SF_MsgBuffer *rb = &slot[islot].readBuffer;
         //SF_MsgBuffer *gb = ge->msgBuffer;
         SF_MsgBuffer *bb = &broadcastBuffer;
         SF_MsgBuffer *tb = &slot[islot].tempWriteBuffer;

	 //
	 // Flush all the buffers that this slot uses.
	 //
         wb->mutex.Request();
	 mstate = 1;
         wb->Flush();
	 mstate = 0;
         wb->mutex.Release();

         tb->mutex.Request();
         tb->Flush();
         tb->mutex.Release();

         rb->mutex.Request();
         rb->Flush();
         rb->mutex.Release();

         bb->mutex.Request();
         while ( bb->GetNextMsg(islot,&msg,S_CLIENT_TCP) );
         while ( bb->GetNextMsg(islot,&msg,S_CLIENT_UDP) );
         bb->mutex.Release();

	 int allow = TRUE;
	 int deny = FALSE;
	 int dev = FALSE;
         int playerInfoRcvd = FALSE;
         int fastProtocol = S_CLIENT_UDP;

	 int numGameStateMsgsRcvd = 0;
	 ulong maxDeadTime = 0;  // Maximum time between data reads

	 //
	 // For initial connect, set the timeout to 2 minute.  After the 
	 // player info is rcvd, set it to the timeOutSec.
	 //
	 int loopTimeOutSec = 120;

         S_StopWatch sw;
	 S_StopWatch gameStateSw;
         while ( slot[islot].continueConnection && everybodyGo != TRUE ) {

            /* ************************************** */
            /* *** Read incoming msgs from client *** */
            /* ************************************** */
            rb->Flush();

            if ( rb->RecvFrom(client,S_CLIENT_NONBLOCK) < 0 ) {
               Log("ERROR -- slot %d RecvFrom returned < 0",islot);
               slot[islot].continueConnection = FALSE;
               slot[islot].kickReason.Create("RecvFrom retured < 0");
	       }

            if ( !rb->IsEmpty() )
               sw.Start();

	    ulong deadTime = sw.Stop();
	    if ( deadTime > maxDeadTime )
	      {
		maxDeadTime = deadTime;
	      }

            if ( slot[islot].continueConnection && 
		 deadTime > (ulong)loopTimeOutSec*1000000 ) {
	       Log("ERROR -- slot %d no contact for > %d seconds",
		   islot,sw.Stop()/1000000);
               slot[islot].continueConnection = FALSE;
               slot[islot].kickReason.Create("No contact for > %d seconds",
                                             sw.Stop()/1000000);
	       }

            /* ********************************************************* */
            /* *** Peek at OBJECTTRACK msgs and strip out ping times *** */
            /* ********************************************************* */
            rb->Rewind();
            while ( rb->GetNextMsg(islot,
                                   &msg,
                                   S_CLIENT_TCP,
                                   SF_MSG_OBJECTTRACK_17,
                                   FALSE) )
               {
               slot[islot].SetLag(msg.bitmap);
               }

            rb->Rewind();
            while ( rb->GetNextMsg(islot,
                                   &msg,
                                   S_CLIENT_UDP,
                                   SF_MSG_OBJECTTRACK_17,
                                   FALSE) )
               {
               slot[islot].SetLag(msg.bitmap);
               }

            if ( playerInfoRcvd &&
                 slot[islot].continueConnection &&
                 lagMax > 0 &&
                 slot[islot].GetMinLag()*1000/frameRate > lagMax ) {
               Log("KICKED -- slot %d lag > %d ms",islot,lagMax);
               slot[islot].continueConnection = FALSE;
               slot[islot].kicked = TRUE;
               slot[islot].kickReason.Create("Lag > %d ms",lagMax);
	       }

            /* ************************************************ */
            /* *** Filter out and handle file download msgs *** */
            /* ************************************************ */
            rb->Rewind();
            while ( rb->GetNextMsg(islot,&msg,S_CLIENT_TCP,
                                   SF_MSG_DOWNLOADFILE) ) {
               if ( msg.str.str ) {
                  _OsFilename(msg.str.str);
                  ulong size = _OsFileSize(msg.str.str);
                  if ( size > 0 ) {
                     char *buffer = (char *)S_malloc(size);
                     if ( buffer ) {
                        FILE *fptr = fopen(msg.str.str,"rb");
                        if ( fptr ) {
                           fread(buffer,size,1,fptr);
                           fclose(fptr);

                           wb->mutex.Request();
			   mstate = 2;
                           wb->PushMsg((uchar)SF_MSG_DOWNLOADFILE,
                                       (ushort)islot,
                                       (char *)msg.str.str,
                                       (ulong)size,
                                       (char *)buffer,
                                       (int)S_CLIENT_TCP);
			   mstate = 0;
                           wb->mutex.Release();

	                   }
                        S_free(buffer);
		        }
		     }
                  }
	       }

            /* ******************************************* */
            /* *** Filter out and handle gamestate msg *** */
            /* ******************************************* */
            rb->Rewind();
            if ( rb->GetNextMsg(islot,&msg,S_CLIENT_TCP,SF_MSG_GAMESTATE) ) {

	      //
	      // Don't allow a player to recv gamestate msgs more frequently
	      // than the defined maximum.
	      //
	      int period = gameStateSw.Stop();
	      int tooSoon = period/1000000 < minGameStateMsgPeriodicitySec &&
		numGameStateMsgsRcvd > 0;

	      //
	      // Don't allow the player to receive more than the defined
	      // maximum number of gamestate msgs.
	      //
	      numGameStateMsgsRcvd++;
	      int tooMany = maxGameStateMsgs > 0 && 
		numGameStateMsgsRcvd > maxGameStateMsgs;
	      
	      if ( !tooSoon && !tooMany )
		{
		  
		  gameStateSw.Start();

		  ge->mutex.Request();
		  ulong gameDataSize = game->Size();
		  char *gameData = (char *) S_malloc( gameDataSize );
		  if ( gameData )
		    {
		      gameDataSize = EngineGetGameState( gameData );
		    }
		  ge->mutex.Release();
		  
		  if ( gameData )
		    {
		      wb->mutex.Request();
		      mstate = 3;
		      wb->PushMsg((uchar)SF_MSG_GAMESTATE,
				  (ushort)islot,
				  (ulong)gameDataSize,
				  (char *)gameData,
				  (int)S_CLIENT_TCP);
		      wb->mutex.Release();
		      mstate = 0;
		      S_free(gameData);
		    }

		}
	    }

            /* ************************************************************* */
            /* *** Filter and set server frame in UDP ping msgs sent UDP *** */
            /* ************************************************************* */
            rb->Rewind();
            if ( rb->GetNextMsg(islot,&msg,S_CLIENT_UDP,SF_MSG_PINGUDP) ) {
               wb->mutex.Request();
	       mstate = 4;
               wb->PushMsg((uchar)SF_MSG_PINGUDP,
                           (ushort)islot,
                           (slong)ge->frame,
                           (slong)msg.itime,
                           (int)S_CLIENT_UDP);
	       mstate = 0;
               wb->mutex.Release();
               fastProtocol = S_CLIENT_UDP;
	       }

            /* ************************************************************* */
            /* *** Filter and set server frame in TCP ping msgs sent TCP *** */
            /* ************************************************************* */
            rb->Rewind();
            if ( rb->GetNextMsg(islot,&msg,S_CLIENT_TCP,SF_MSG_PINGTCP) ) {
               wb->mutex.Request();
	       mstate = 5;
               wb->PushMsg((uchar)SF_MSG_PINGTCP,
                           (ushort)islot,
                           (slong)ge->frame,
                           (slong)msg.itime,
                           (int)S_CLIENT_TCP);
	       mstate = 0;
               wb->mutex.Release();
               fastProtocol = S_CLIENT_TCP;
	       }

            /* ************************************************************* */
            /* *** Filter and set server frame in UDP ping msgs sent TCP *** */
            /* ************************************************************* */
            rb->Rewind();
            if ( rb->GetNextMsg(islot,&msg,S_CLIENT_TCP,SF_MSG_PINGUDP) ) {
               wb->mutex.Request();
	       mstate = 6;
               wb->PushMsg((uchar)SF_MSG_PINGUDP,
                           (ushort)islot,
                           (slong)ge->frame,
                           (slong)msg.itime,
                           (int)S_CLIENT_TCP);
	       mstate = 0;
	       wb->mutex.Release();
               fastProtocol = S_CLIENT_TCP;
	       }

            /* ******************************************* */
            /* *** Filter out and handle frameRate msg *** */
            /* ******************************************* */
            rb->Rewind();
            if ( rb->GetNextMsg(islot,&msg,S_CLIENT_TCP,SF_MSG_FRAMERATE) ) {
               wb->mutex.Request();
	       mstate = 7;
               wb->PushMsg((uchar)SF_MSG_FRAMERATE,
                           (ushort)islot,
                           (uchar)frameRate,
                           (int)S_CLIENT_TCP);
	       mstate = 0;
               wb->mutex.Release();
               }

            /* ******************************************* */
            /* *** Filter out and handle updateRate msg *** */
            /* ******************************************* */
            rb->Rewind();
            if ( rb->GetNextMsg(islot,&msg,S_CLIENT_TCP,SF_MSG_UPDATERATE) ) {
               wb->mutex.Request();
	       mstate = 7;
               wb->PushMsg((uchar)SF_MSG_UPDATERATE,
                           (ushort)islot,
                           (uchar)shipUpdateRate,
                           (int)S_CLIENT_TCP);
	       mstate = 0;
               wb->mutex.Release();
               }

            /* ******************************************* */
            /* *** Filter out and handle RequestDev msg *** */
            /* ******************************************* */
            rb->Rewind();
            if ( rb->GetNextMsg(islot,&msg,S_CLIENT_TCP,SF_MSG_REQUESTDEV) ) {
               wb->mutex.Request();
	       mstate = 7;
               wb->PushMsg((uchar)SF_MSG_REQUESTDEV,
                           (ushort)islot,
                           (uchar)dev,
                           (int)S_CLIENT_TCP);
	       mstate = 0;
               wb->mutex.Release();
               }

            /* ****************************************** */
            /* *** Write msgs into broadcastMsgBuffer *** */
            /* ****************************************** */
            if ( !rb->IsEmpty() ) {
               bb->mutex.Request();
               bb->Merge(rb);
               bb->mutex.Release();
               }

            wb->mutex.Request();
	    mstate = 8;

            /* ************************************* */
            /* *** Write outgoing msgs to client *** */
            /* ************************************* */
            if ( !wb->IsEmpty() ) {
               int j;

               /* *********************************** */
               /* *** Peek at any playerInfo msgs *** */
               /* *********************************** */
               wb->Rewind();
               while ( wb->GetNextMsg(islot,&msg,S_CLIENT_TCP,
                                      SF_MSG_PLAYERINFO,FALSE ) ) {
                  playerInfoRcvd = TRUE;
                  slot[islot].StartBandWidth();
                  slot[islot].ResetLagHistory();

		  loopTimeOutSec = timeOutSec;

		  dev = PlayerIsListed("server.devs",
				       msg.player.playerName.str);
		  allow = PlayerIsListed("server.allow",
				       msg.player.playerName.str);
		  char *reason = "You Are Banned!";
		  deny = PlayerIsBanned(msg.player.playerName.str,&reason);
		  if ( deny && !allow )
		    {
		      Log("Go Away! %s (%s) (%d) [%d/%d pl]",
			   msg.player.playerName.str,
			   client->numIPAddress,msg.player.serial,islot,
			   numPlayers);
		      slot[islot].continueConnection = FALSE;
		      slot[islot].kicked = TRUE;
		      slot[islot].kickReason.Create(reason);
		    }
		  else
		    {
		      Log("Welcome %s (%s) (%d) [%d/%d pl]",
			  msg.player.playerName.str,
			  client->numIPAddress,msg.player.serial,islot,
			  numPlayers);
		    }
		  slot[islot].serial = msg.player.serial;

		  //
		  // Check for players using the same serial #
		  //
		  ge->mutex.Request();
		  for ( int i=0; i<game->maxHumanShips; i++ )
		    {
		      if ( slot[i].filled )
			{
			  int pi = game->ship[i].playerIndex;
			  if ( i != islot && 
			       slot[i].serial != 0 &&
			       slot[i].serial == slot[islot].serial &&
			       strcmp(msg.player.playerName.str,
				      game->player[pi].name) &&
			       slot[i].ipAddress.str &&
			       slot[islot].ipAddress.str &&
			       strcmp(slot[i].ipAddress.str,
				      slot[islot].ipAddress.str) )
			    {
			      
			      S_String str(
					   "DUPSER(%d) %s(%s) and %s",
					   slot[islot].serial,
					   msg.player.playerName.str,
					   client->numIPAddress,
					   game->player[pi].name);
			      
			      BroadcastMsg(str.str);
			    }
			}
		    }
		  ge->mutex.Release();

                  if ( window.created ) {
                     window.RefreshPlayerList(islot,msg.player.playerName.str);
                     window.Refresh();
		     }

	          }

               /* ******************************************** */
               /* *** Filter out and handle disconnect msg *** */
               /* ******************************************** */
               wb->Rewind();
               if ( wb->GetNextMsg(islot,&msg,S_CLIENT_TCP,SF_MSG_DISCONNECT)){
                  Log("Rcvd Disconnect from slot %d",islot);
                  slot[islot].continueConnection = FALSE;
                  slot[islot].kickReason.Create("You sent a disconnect");
	          }

               /* ********************************************************* */
               /* *** Get rid of redundant msgs from self.  Msgs listed *** */
               /* *** in the if () are not filtered.                    *** */
               /* ********************************************************* */
               for ( j=0; j<256; j++ )
                  if ( j != SF_MSG_PING_10 && j != SF_MSG_PINGTCP && 
                       j != SF_MSG_PINGUDP && j != SF_MSG_PLAYERCHAT_N && 
                       j != SF_MSG_GAMESTATE && j != SF_MSG_FRAMERATE &&
                       j != SF_MSG_PLAYERINFO && j != SF_MSG_SPAWNCOLONY &&
                       j != SF_MSG_DOWNLOADFILE && j != SF_MSG_UPDATERATE &&
                       j != SF_MSG_REQUESTDEV && 
		       j != SF_MSG_REQUESTPOPULATION ) {
                     wb->FilterMsg(islot,j,S_CLIENT_TCP);
                     wb->FilterMsg(islot,j,S_CLIENT_UDP);
		     }

               /* ******************************************************** */
               /* *** Get rid of msgs from players that are not nearby *** */
               /* ******************************************************** */
	       ge->mutex.Request();
	       int numShips = game->numShips;
	       int raceID = game->ship[islot].raceID;

               if ( game->ship[islot].IsActive() )
               for ( j=0; j<numShips; j++ )
                  if ( game->ship[j].IsActive() &&
                       (slot[j].filled || j>game->maxHumanShips) && islot!=j ){
                     slong dd;
                     /* *********************************************** */
                     /* *** For some reason running this code below *** */
                     /* *** corrupts the islot variable.            *** */
                     /* *********************************************** */
                     dd = max(
                           labs(game->ship[islot].now.x-game->ship[j].now.x),
                           labs(game->ship[islot].now.y-game->ship[j].now.y)) /
                          1024/sub_scale;

                     if ( dd > 10 || dd < 0 ) dd = 10;

                     /* *************************************************** */
                     /* *** Don't send FIRETUBES msgs for ships greater *** */
                     /* *** than 1024 pixels away                       *** */
                     /* *************************************************** */
                     if ( dd > 0 ) {
                        wb->FilterMsg(j,SF_MSG_FIRETUBES,S_CLIENT_TCP);
                        wb->FilterMsg(j,SF_MSG_FIRETUBES,S_CLIENT_UDP);
		        }
                     /* *************************************************** */

                     /* ************************************************ */
                     /* *** Skip more object track msgs as ships are *** */
                     /* *** farther away.                            *** */
                     /* ************************************************ */
                     if ( numObjectTrackMsgsSkipped[j] < dd )
		       {
			 numObjectTrackMsgsSkipped[j]++;
			 wb->FilterMsg(j,SF_MSG_AIOBJECTTRACK_17,S_CLIENT_TCP);
			 wb->FilterMsg(j,SF_MSG_AIOBJECTTRACK_17,S_CLIENT_UDP);
			 //wb->FilterMsg(j,SF_MSG_OBJECTTRACK_17,S_CLIENT_TCP);
			 //wb->FilterMsg(j,SF_MSG_OBJECTTRACK_17,S_CLIENT_UDP);
		       }
                     else
		       {
			 numObjectTrackMsgsSkipped[j] = 0;
		       }
                     /* ************************************************ */

		  }
	       
               /* ****************************************** */
               /* *** Filter out inappropriate chat msgs *** */
               /* ****************************************** */
               for ( int i=0; i<numShips; i++ ) {
		 wb->Rewind();
		 while ( wb->GetNextMsg(i,&msg,S_CLIENT_TCP,
					SF_MSG_PLAYERCHAT_N) ) {
		   
		   if (  msg.mode == SF_MSG_CHAT_TOALL ||
			 (msg.mode == SF_MSG_CHAT_TORACE &&
			  msg.to == raceID) )
		     {
		       FilterString(msg.str.str);
		       wb->PushMsg((uchar)SF_MSG_PLAYERCHAT_N,
				   (ushort)msg.slot,
				   (uchar)msg.mode,
				   (uchar)msg.to,
				   msg.str.str,
				   (int)S_CLIENT_TCP);
		     }
		 }
	       }
	       ge->mutex.Release();

               /* *** Copy Data to temporary buffer *** */
               if ( slot[islot].continueConnection )
                  tb->Merge(wb);

               wb->Flush();

	       }

	    mstate = 0;
            wb->mutex.Release();

            /* *************************** */
            /* *** Send data to client *** */
            /* *************************** */
            if ( !tb->IsEmpty() && slot[islot].continueConnection && 
                 tb->SendTo(client,fastProtocol)<0 ) {
               Log("ERROR -- slot %d SendTo returned < 0");
               slot[islot].continueConnection = FALSE;
               slot[islot].kickReason.Create("SendTo retured < 0");
               }
            tb->Flush();

            timer.WaitFor();
	 }

         if ( slot[islot].kicked == TRUE ) {
            Log("Kicking player in slot %d",islot);
            tb->mutex.Request();
            tb->Flush();
            tb->PushMsg((uchar)SF_MSG_DISCONNECT,
                        (ushort)islot,
			(char *)slot[islot].kickReason.str,
                        (int)S_CLIENT_TCP);
            tb->mutex.Release();
            tb->SendTo(client,fastProtocol);
            slot[islot].kicked = FALSE;
	    }
         else if ( everybodyGo == TRUE ) {
            Log("Everybody Go!");
            tb->mutex.Request();
            tb->Flush();
            tb->PushMsg((uchar)SF_MSG_DISCONNECT,
                        (ushort)islot,
			"Server Shutdown",
                        (int)S_CLIENT_TCP);
            tb->mutex.Release();
            tb->SendTo(client,fastProtocol);
	    }
         else if ( playerInfoRcvd ) {
            tb->mutex.Request();
            tb->Flush();
            tb->PushMsg((uchar)SF_MSG_DISCONNECT,
                        (ushort)islot,
                        (char *)slot[islot].kickReason.str,
                        (int)S_CLIENT_TCP);
            tb->mutex.Release();
            tb->SendTo(client,fastProtocol);
	    }


	 //
	 // Send to server to make sure players get removed from list in
	 // game engine.
	 //
	 bb->mutex.Request();
	 bb->Flush();
	 bb->PushMsg((uchar)SF_MSG_DISCONNECT,
		     (ushort)islot,
		     (char *)slot[islot].kickReason.str,
		     (int)S_CLIENT_TCP);
	 bb->mutex.Release();

         numPlayers--;
         if ( playerInfoRcvd )
	   {
	     Log("Goodbye %s (%d bytes/s) (%d ms) [%d/%d pl]",
		  game->playerList[islot]->name,
		  slot[islot].GetBandWidth(
					   S_CLIENT_TCP | 
					   S_CLIENT_UDP | 
					   SF_SLOT_BYTESSENT | 
					   SF_SLOT_BYTESRCVD),
		  maxDeadTime/1000,
		  islot,
		  numPlayers);
	   }
	 
	 slot[i].ipAddress.Create("");
         slot[islot].filled = FALSE;
	 slot[islot].serial = 0;

         if ( window.created ) {
            window.RefreshPlayerList(islot,"< Empty >");
            window.Refresh();
            }

         break;
         }

      }
   return 0;
   }


int
SF_Server::BroadcastMsg(char *msg)
{
   SF_MsgBuffer *bb = &broadcastBuffer;
   bb->mutex.Request();
   bb->PushMsg((uchar)SF_MSG_PLAYERCHAT_N,
               (ushort)ge->localSlot,
               (uchar)SF_MSG_CHAT_TOALL,
               (uchar)0,
                msg,
               (int)S_CLIENT_TCP);
   bb->mutex.Release();
  return 0;
}


int
SF_Server::SendWarningMsg() {
   SF_MsgBuffer *bb = &broadcastBuffer;
   bb->mutex.Request();
   bb->PushMsg((uchar)SF_MSG_PLAYERCHAT_N,
               (ushort)ge->localSlot,
               (uchar)SF_MSG_CHAT_TOALL,
               (uchar)0,
                "WARNING: Shutdown imminent! Please log off now! (^_^)",
               (int)S_CLIENT_TCP);
   bb->mutex.Release();
   return 0;
   }


int
SF_Server::KickEverybodyOff() {
   everybodyGo = TRUE;
   return 0;
   }


int
SF_Server::KickPlayerOff(int islot) {
   if ( islot >= 0 && islot < max_players ) {
      slot[islot].kicked = TRUE;
      slot[islot].kickReason.Create("Server Booted You");
      slot[islot].continueConnection = FALSE;
      if ( window.created )
         window.Refresh();
      } 
   return 0;
   }


int
SF_Server::EngineThread() {
   int i;
   S_Timer timer(1000000/frameRate,sf->stopWatchType);

   Log("Starting %s . . .",serverName);
   Log("Framerate set to %d fps",frameRate);
   if ( shipUpdateRate == 1 )
     {
       Log("Ship Update every frame");
     }
   else
     {
       Log("Ship Update every %d frames",shipUpdateRate);
     }
   if ( aishipUpdateRate == 1 )
     {
       Log("AI Ship Update every frame");
     }
   else
     {
       Log("AI Ship Update every %d frames",aishipUpdateRate);
     }
   Log("Max players set to %d",game->numShips);
   Log("Num Robots is set to %d",game->numShips-game->maxHumanShips);
   Log("Max Human Players set to %d",game->maxHumanShips);
   if ( lagMax > 0 )
      Log("Kick Laggers with > %dms lag",lagMax);
   Log("Connection timeout after %d seconds",timeOutSec);
   if ( maxGameStateMsgs == 0 )
     {
       Log("Allow infinite full game state downloads");
     }
   else if ( maxGameStateMsgs == 1 )
     {
       Log("Allow 1 full game state download");
     }
   else
     {
       Log("Allow %d full game state downloads",maxGameStateMsgs);
     }
   Log("Allow full gamestate download every %d seconds",
       minGameStateMsgPeriodicitySec);
   Log("Listening on port %d",port);
   if ( denyIPList.numLines > 0 )
     {
       Log("Deny the following IP addresses:");
       for ( i=0; i<denyIPList.numLines; i++ )
	 {
	   Log("        [%s]",denyIPList.lines[i]);
	 }
     }
   Log("Scanning for "SF_REGISTERED_CLUB" ships . . .");

   if ( window.created )
      window.Refresh();
   for ( i=0; i<game->numBitmaps; i++ )
     {
       //char *clas = game->objectBitmapList[i].Class;
       char *name = game->objectBitmapList[i].name;
       char *file = game->objectBitmapList[i].file;
       char *path = "..\\bmp\\256\\";
       S_String filename("%s%s",path,file);
       ulong maskFlags = S_BITMAP_MASK_TRANSLUCENT;
       ulong blitFlags = S_BITMAP_BLIT_USEMASK;
       SF_Sprite *sprite = new SF_Sprite(name,filename.str,maskFlags,blitFlags,
					 game->objectBitmapList[i].numFrames,
					 game->objectBitmapList[i].gamma,
					 game->objectBitmapList[i].alpha);
       if ( sprite )
	 {
	   if ( sprite->IsACOnly(name)
		)
	     {
	       game->objectBitmapList[i].acOnly = TRUE;
	     }
	   delete sprite;
	 }
     }

   Log("Ready To Serve . . .");

   if ( window.created )
      window.Refresh();

   SF_MsgBuffer *rb = ge->retMsgBuffer; /* *** return buffer    *** */
   SF_MsgBuffer *gb = ge->msgBuffer;    /* *** game buffer      *** */
   SF_MsgBuffer *bb = &broadcastBuffer; /* *** broadcast buffer *** */
   while ( run ) {

      /* ******************************************************************* */
      /* *** Update the broadcast buffer with amy msgs received from the *** */
      /* *** game engine and the broadcast its contents to all the slots *** */
      /* ******************************************************************* */

      /* *** Add msgs returned by game engine to broadcast buffer *** */
      if ( rb && !rb->IsEmpty() )
	{
	  bb->mutex.Request();
	  bb->Merge(rb);
	  bb->mutex.Release();
	}

      /* *** Add broadcast msgs into client writeBuffers *** */
      for ( i=0; i<maxSlots; i++ ) {
         if ( slot && slot[i].filled ) {
            if ( !bb->IsEmpty() ) {
               slot[i].writeBuffer.mutex.Request();
	       mstate = 9;
	       bb->mutex.Request();
               slot[i].writeBuffer.Merge(bb);
	       bb->mutex.Release();
	       mstate = 0;
               slot[i].writeBuffer.mutex.Release();
               }
	    }
         }

      /* *** Add broadcast msgs to game buffer *** */
      bb->mutex.Request();
      if ( !bb->IsEmpty() )
	{
         gb->Merge(bb);
	}
      bb->Flush();
      bb->mutex.Release();
      /* ******************************************************************* */
      /* ************ Done accessing broadcast buffer ********************** */
      /* ******************************************************************* */

      logMutex.Request();
      ge->mutex.Request();
      ge->Update();
      ge->mutex.Release();
      logMutex.Release();

      rb = ge->retMsgBuffer;

      timer.WaitFor();

      }
   return 0;
   }


int
_ServerEngineThread(void *arg) {
   SF_Server *server = (SF_Server *)arg;
   server->EngineThread();
   return 0;
   }


char *
_Server_TimeStamp() {
   static char temp[25];
   time_t timer;
   timer = time(&timer);
   strncpy(temp,ctime(&timer),24);
   temp[24] = 0;
   return( temp );
   }


int
SF_Server::RegisterWithNexus( char *nexusName,
			      ulong startStop,
			      int logWarnings ) {

/* startStop == SF_NEXUS_SERVERSTART,
                SF_NEXUS_SERVERSTOP */

   ulong response;
   S_Client client;

   if ( client.Connect(nexusName,SF_NEXUS_PORT,startStop) )
      return -1;

   char *numIPAddress = inet_ntoa(client.localAddr.sin_addr);

   if ( startStop==SF_NEXUS_SERVERSTOP ) {

      S_Packet packet;
      packet.PushUShort(port);
      client.WritePacket(&packet);

      client.ReadULong(&response);

      return( SF_SERVER_OK );
      }
   else if ( startStop == SF_NEXUS_SERVERSTART ) {
      client.WriteUChar(SF_MAJOR_VERSION);
      client.WriteUChar(SF_MINOR_VERSION);

      client.ReadULong(&response);

      if ( response == SF_SERVER_WRONGVERSION ) {
         uchar majorVersion,minorVersion;
         client.ReadUChar(&majorVersion);
         client.ReadUChar(&minorVersion);

         if ( logWarnings ) {
            if ( (schar)majorVersion >= SF_MAJOR_VERSION && 
                 minorVersion > SF_MINOR_VERSION ) {
               Log("");
               Log("************* Warning *************");
               Log("You are running an outdated server.");
               Log("Visit www.stardock.com to download");
               Log("an updated version.");
               Log("");
               }
            else {
               Log("");
               Log("*********** Warning **************");
               Log("The NEXUS is running an older copy");
               Log("of Stellar Frontier.  This is very");
               Log("strange and should not be possible.");
               Log("Visit www.stardock.com to look for");
               Log("clues to this mystery.");
               Log("");
               }

	    if ( window.created )
	      window.Refresh();

            }
         }

      CreatePlayerList();

      S_Packet packet;
      packet.PushUShort(numPlayers+game->numShips-game->maxHumanShips);
      packet.PushUShort(game->numShips);
      packet.PushUChar(SF_MAJOR_VERSION); 
      packet.PushUChar(SF_MINOR_VERSION); 
      packet.PushString(_OsString());
      packet.PushString(serverName);
      packet.PushUChar(SF_MINOR_MINOR_VERSION); 
      packet.PushString(SF_VERSION_MODIFIER);
      packet.PushString(SF_VERSION_STRING);
      packet.PushUShort(port);
      packet.PushUShort(numPlayers);
      packet.PushString(playerList.str);
      packet.PushULong(frameNumber/frameRate);
      packet.PushString(numIPAddress);
      client.WritePacket(&packet);

      return( SF_SERVER_OK );
      }

   return( -2 );
}


int
SF_Server::CreatePlayerList()
{
  int firstTime = TRUE;
  playerList.Destroy();
  for ( int i=0; i<game->numShips; i++ )
    {
      if ( i<game->maxHumanShips )
	{
	  if ( slot && slot[i].filled )
	    {
	      S_String name(game->player[game->ship[i].playerIndex].name);
	      name.ReplaceCharacter(':',"|");
	      name.ReplaceCharacter(',',"|");
	      for ( int j=0; j<32; j++ )
		{
		  name.ReplaceCharacter(j,"|");
		}
	      if ( firstTime )
		{
		  playerList.Create(name.str);
		  firstTime = FALSE;
		}
	      else
		{
		  S_String old(playerList.str);
		  playerList.Create("%s,%s",old.str,name.str);
		}
	    }
	}
    }
  return 0;
}


int
_NexusVerifyThread( void *arg ) {
   SF_Server *s = (SF_Server *)arg;
   _OsSleep(1000000);  // Wait 1 second to allow server object to init.

   s->Log("Registering with Nexus at %s . . .",s->nexusName);
   if ( s->run && s->RegisterWithNexus( s->nexusName,
					SF_NEXUS_SERVERSTART,TRUE ) )
      s->Log("Nexus at %s is down.",s->nexusName);

   if ( s->altNexusName )
     {
       s->Log("Registering with Nexus at %s . . .",s->altNexusName);
       if ( s->run && s->RegisterWithNexus( s->altNexusName,
					    SF_NEXUS_SERVERSTART,TRUE ) )
	 s->Log("Nexus at %s is down.",s->altNexusName);
     }

   if ( s->window.created )
      s->window.Refresh();
   while( s->run ) {
      for ( int i=0; i<10*SF_NEXUS_CONTACT_TIME && s->run; i++ )
	{
         _OsSleep(100000);
	}

      if ( s->run &&
           s->RegisterWithNexus( s->nexusName,SF_NEXUS_SERVERSTART,FALSE ) )
         s->Log("Nexus at %s is down.",s->nexusName);

      if ( s->altNexusName )
	{
	  if ( s->run &&
	       s->RegisterWithNexus( s->altNexusName,
				     SF_NEXUS_SERVERSTART,FALSE ) )
	    s->Log("Nexus at %s is down.",s->altNexusName);
	}
      }
   if ( s->RegisterWithNexus( s->nexusName,SF_NEXUS_SERVERSTOP,FALSE ) )
      s->Log("Nexus at %s is down.",s->nexusName);
   if ( s->altNexusName )
     {
       if ( s->RegisterWithNexus( s->altNexusName,SF_NEXUS_SERVERSTOP,FALSE ) )
	 s->Log("Nexus at %s is down.",s->altNexusName);
     }
   return 0;
   }


int
SF_Server::Log( const char *fmt, ...) {


  logMutex.Request();

  va_list v;
  va_start(v,fmt);
  S_Logger::VLog(fmt,v);
  va_end(v);

  if ( line.str )
    {
      FilterString(line.str);
      ::logFile << line.str << endl;
      if ( window.created && !window.closing )
	{
	  window.console.AddLine(line.str);
	}
    }

  logMutex.Release();

  return 0;
}


int
SF_Server::SLog( const char *fmt, ...) {


  logMutex.Request();

  va_list v;
  va_start(v,fmt);
  S_Logger::VLog(fmt,v);
  va_end(v);

  if ( line.str )
    {
      FilterString(line.str);

      if ( ge )
	{
	  ge->mutex.Request();
	  ::logFile << line.str << endl;
	  ge->mutex.Release();
	}
      if ( window.created && !window.closing )
	{
	  window.console.AddLine(line.str);
	}
    }

  logMutex.Release();

  return 0;
}


int
SF_Server::NewClientDenied(S_Client *client)
{
  Log("Denied client connect from %s",client->numIPAddress);
  return 0;
}


int
SF_Server::PlayerIsBanned(char *name,char **reason)
{

  if ( !name )
    {
      return TRUE;
    }

  int denied = FALSE;
  for ( int i=0; i<denyIPList.numLines; i++ )
    {
      if ( denyIPList.lines[i] )
	{
	  if ( !strcmp("ALL",denyIPList.lines[i]) ||
	       !strcmp(name,denyIPList.lines[i]) )
	    {
	      if ( reason )
		{
		  *reason = denyIPReason.lines[i];
		}
	      denied = TRUE;
	    }
	}
    }

  return denied;
}


int
SF_Server::PlayerIsListed(char *filename,char *name)
{

  listMutex.Request();

  LoadList(filename);
  
  if ( !name )
    {
      listMutex.Release();
      return FALSE;
    }

  int listed = FALSE;
  for ( int i=0; i<list.numLines; i++ )
    {
      if ( list.lines[i] )
	{ 
	  if ( !strcmp("ALL",list.lines[i]) ||
	       !strcmp(name,list.lines[i]) )
	    {
	      listed = TRUE;
	    }
	}
    }

  listMutex.Release();

  return listed;
}


int
SF_Server::LoadList(char *filename)
{

  list.Destroy();

  if ( filename )
    {
      FILE *fptr = fopen(filename,"r");
      if ( fptr )
	{

	  char *rc;
	  char temp[1024];
	  do 
	    {
	      rc = fgets(temp,1023,fptr);
	      if ( rc && temp[0] != '#' )
		{
		  S_TokenArray tokens(temp);
		  if ( tokens.numLines > 0 )
		    {
		      tokens.RemoveCharacter(0,(char)10);
		      tokens.RemoveCharacter(0,(char)13);
		      tokens.RemoveCharacter(0,':');
		      tokens.RemoveCharacter(0,';');
		      if ( tokens.lines[0][0] != 0 )
			{
			  list.AppendLine(tokens.lines[0]);
			}
		    }
		}
	    } while ( rc != NULL );

	  fclose(fptr);
	}
    }
  return 0;
}
