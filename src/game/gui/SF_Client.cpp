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
#include "SF_NexusClient.h"
#include "SF_ClientWindow.h"
#include "SF_Client.h"
#include "SF_Profile.h"
#include "SF_SpriteList.h"
#include "SF_WaveList.h"
#include "SF_MidiList.h"
#include "SF_TextBuffer.h"
#include "SF_Engine.h"
#include "SF_GameEngine.h"
#include "SF_NetEngine.h"
#include "SF_Player.h"
#include "SF_Msg.h"
#include "SF_Scenario.h"
#include "SF_MsgBuffer.h"
#include "SF_MsgBox.h"
#include "SF_HighScore.h"
#include "SF_DisplayEngine.h"
#include "SF_VideoConsole.h"
#include "SF_ChatConsole.h"
#include "SF_Server.h"
#include "SF_Nexus.h"
#include "SF_Sprite.h"
#include "SF_MapConsole.h"
#include "SF_NagDialog.h"
#include "SF_UpgradeWindow.h"
#include "SF_Slot.h"

SF_NexusClient *stardockNetDialog = NULL;
S_Dialog *internetDialog = NULL;
S_Dialog *intranetDialog = NULL;

/* ************************ SF_Client class methods ************************ */
int _SplashThreadFunc(void *);
int MainWindowHandler(S_StdWindow *);
SF_Client::SF_Client(int imode,char *ihostname,int *pargc,char ***pargv) {

   sf = this;
   mode = imode;
   hostname = NULL;
   connectedThroughNexus = FALSE;
   if ( ihostname ) {
      hostname = S_strdup(ihostname);
      connectedThroughNexus = TRUE;
      }

   rcEngine = 0;
   tabToggle = FALSE;
   game = NULL;
   engine = NULL;
   root = NULL;
   mainWindow = NULL;
   msl = NULL;
   shiftDown = FALSE;
   ctrlDown = FALSE;
   backBuffer = NULL;
   font = NULL;
   serverIndex = -1;
   scrollingText = NULL;
   msgBox = NULL;
   menuBar = NULL;
   gameMenu = NULL;
   bmpBanner = NULL;
   bmpBannerFilename = NULL;
   urlBanner = NULL;
   stardockNetMenuItem = NULL;
   internetMenuItem = NULL;
   intranetMenuItem = NULL;
   disconnectMenuItem = NULL;
   singlePlayerMenuItem = NULL;
   pauseMenuItem = NULL;
   stopMenuItem = NULL;
   optionsMenuItem = NULL;
   exitMenuItem = NULL;
   nagDialog = NULL;
   numFiles = 0;
   files = NULL;
   numwFiles = 0;
   wfiles = NULL;
   downloadDialog = NULL;
   wdownloadDialog = NULL;
   mml = NULL;
   mwl = NULL;
   menuPing = NULL;
   splashThread = NULL;
   introWav = NULL;
   audio = NULL;
   runCredits = FALSE;
   bmpSplash = NULL;
   bmpSDSSplash = NULL;
   bmpSF1Splash = NULL;
   bmpSF2Splash = NULL;
   mouseMenuItem = 0;
   mouseMenuEnabled = TRUE;
   demo = FALSE;
   oldAC = FALSE;
   nexusMajorVersion = 0;
   nexusMinorVersion = 0;
   nexusMinorMinorVersion = 0;
   nexusVersionModifier[0] = 0;
   nexusVersionString[0] = 0;
   needToUpgrade = FALSE;
   profile = new SF_Profile();

   if ( profile->SetRegCode() == SF_PROFILE_INVALIDREGCODE )
     {
       demo = TRUE;
     }
   
   if ( profile->SetRegCode() == SF_PROFILE_OLDREGCODE )
     {
       demo = TRUE;
       oldAC = TRUE;
     }
   
#  ifdef OS2
     demo = FALSE;
     oldAC = FALSE;
#  endif

   if ( mode == SF_CLIENT_DATA_ENGINE_DISPLAY ) {

      root = new S_Root(pargc,pargv,S_ROOT_USEDIRECTSOUND);

      if ( 0 ) {
         window.Create(root);
         }
      else {
         char *windowTitle = (char *)S_malloc(48);
         sprintf(windowTitle,
                 "Stellar Frontier v%s",
                 VersionString());
         mainWindow = new S_StdWindow(root,windowTitle,MainWindowHandler,1,
                                      -root->cxSizeBorder,-root->cySizeBorder,
                                      root->cx + 2*root->cxSizeBorder,
                                      root->cy + 2*root->cySizeBorder,
                                      S_WINDOW_STANDARD,(void *)this);
         mainWindow->selfDelete = TRUE;
         if ( windowTitle ) {
            S_free(windowTitle);
            windowTitle = NULL;
            }

         cxHalftone = 64;
         cyHalftone = 64;
         cxVideoConsole = 320;
         cyVideoConsole = 200;
         cxMapConsole = 320;
         cyMapConsole = 320;
         cxPanel = 340;
         cyPanel = 115;
         margin = 5;
         cyConsoleButtons = 40;
         cyChatMin = 130;

         /* *** Little patch to make things fit *** */
         if ( _root->cx >= 1152 ) cxPanel += 4;
         if ( _root->cx >= 1280 ) cxPanel += 4;

         scrollingText = new SF_TextBuffer();

         msgBox = new SF_MsgBox(mainWindow);
         msgBox->selfDelete = TRUE;

         downloadDialog = new SF_UpgradeWindow(mainWindow);
         downloadDialog->selfDelete = TRUE;

         wdownloadDialog = new SF_UpgradeWindow(mainWindow);
         wdownloadDialog->selfDelete = TRUE;

         msl = new SF_SpriteList();

         audio = new S_AudioDevice();
         audio->SetVolume(profile->soundVolume);

         menuPing = new S_WaveData("..\\wav\\sonar2.wav",9);
         menuPing->SetVolume(50);

         mwl = new SF_WaveList();

         mwl->Add(new S_WaveData("..\\wav\\torpedo.wav",6) );
         mwl->Add(new S_WaveData("..\\wav\\button1.wav",1) );
         mwl->Add(new S_WaveData("..\\wav\\button2.wav",1) );
         mwl->Add(new S_WaveData("..\\wav\\button3.wav",1) );
         mwl->Add(new S_WaveData("..\\wav\\expm.wav",6) );   
         mwl->Add(new S_WaveData("..\\wav\\engine.wav",1) ); 
         mwl->Add(new S_WaveData("..\\wav\\engine.wav",1) ); 
         mwl->Add(new S_WaveData("..\\wav\\klaxon.wav",9) ); 
         mwl->Add(new S_WaveData("..\\wav\\sonar.wav",9) ); 
         mwl->Add(new S_WaveData("..\\wav\\sonar2.wav",9) ); 
         mwl->Add(new S_WaveData("..\\wav\\laser.wav",1) );  
         mwl->Add(new S_WaveData("..\\wav\\thrust1.wav",1,TRUE) );
         mwl->Add(new S_WaveData("..\\wav\\thrust2.wav",2,TRUE) );
         mwl->Add(new S_WaveData("..\\wav\\radar.wav",1) );  
         mwl->Add(new S_WaveData("..\\wav\\radar.wav",1) );  
         mwl->Add(new S_WaveData("..\\wav\\warpout3.wav",5) );
         mwl->Add(new S_WaveData("..\\wav\\warpin.wav",5) );
         mwl->Add(new S_WaveData("..\\wav\\explode.wav",10) );
         mwl->Add(new S_WaveData("..\\wav\\torpedo2.wav",6) );
         mwl->Add(new S_WaveData("..\\wav\\torpedo3.wav",6) );
         mwl->Add(new S_WaveData("..\\wav\\torpedo4.wav",6) );
         mwl->Add(new S_WaveData("..\\wav\\torpedo5.wav",6) );
         mwl->Add(new S_WaveData("..\\wav\\torpedo6.wav",6) );
         mwl->Add(new S_WaveData("..\\wav\\torpedo7.wav",6) );

         /* *************************************************************** */
         /* *** Check to see if any wave files failed to load correctly *** */
         /* *************************************************************** */
         int i;
         int loadErr = FALSE;
         char errStr[1024];
         sprintf(errStr,"The following wave files failed to load: ");
         for ( i=0; i<mwl->numWaves; i++ ) {
            S_WaveData *w = mwl->Wave(i);
            if ( w->rc != 0 ) {
               loadErr = TRUE;
               sprintf(errStr,"%s%s ",errStr,w->name);
               }
            }
         if ( loadErr )
            S_MsgBox(mainWindow,"WARNING!",errStr,S_MSGBOX_OK);
         /* *************************************************************** */

         mwl->SetVolume(profile->soundVolume);

         mml = new SF_MidiList();

         mml->Add(new S_Midi("..\\mid\\sf-music1.mid"));
         mml->Add(new S_Midi("..\\mid\\sf-music2.mid"));
         mml->Add(new S_Midi("..\\mid\\sf-music3.mid"));
         mml->Add(new S_Midi("..\\mid\\sf-music4.mid"));
         mml->Add(new S_Midi("..\\mid\\sf-music5.mid"));
         mml->Add(new S_Midi("..\\mid\\sf-music6.mid"));
         mml->Add(new S_Midi("..\\mid\\sf-music7.mid"));
         mml->Add(new S_Midi("..\\mid\\sf-music8.mid"));

         mml->StartThread();

         int numColors = mainWindow->root->numColors;

         if ( numColors < 256 || _root->cx < 800 || _root->cy < 600 ) {
            char tmp[1024];
            sprintf(tmp,
                    "Your video resolution of %dx%d with %d colors\nis not "
                    "supported.\n\nThe minimum required video resolution for "
                    "Stellar Frontier\nis 800x600 with 256 colors",
                    _root->cx,_root->cy,numColors);
            S_MsgBox(mainWindow,"ERROR",tmp,S_MSGBOX_OK);
            mainWindow->Close();
            exit(0);
	    }

         if ( numColors == 0 || numColors > 65536 ) numColors = 65536;
         char filename[128];
         if ( mainWindow->root->cx >= 1024 )
            sprintf(filename,"..\\bmp\\%d\\sf-menu1-1024",numColors);
         else
            sprintf(filename,"..\\bmp\\%d\\sf-menu1-800",numColors);
         bmpSF1Splash = new S_SystemBitmap(filename);

         if ( mainWindow->root->cx >= 1024 )
            sprintf(filename,"..\\bmp\\%d\\sf-menu2-1024",numColors);
         else
            sprintf(filename,"..\\bmp\\%d\\sf-menu2-800",numColors);
         bmpSF2Splash = new S_SystemBitmap(filename);

         sprintf(filename,"..\\bmp\\%d\\sf-sds",numColors);
         bmpSDSSplash = new S_SystemBitmap(filename);

         if ( bmpSF1Splash->rc || bmpSF2Splash->rc ) {
            char tmp[1024];
            sprintf(tmp,
                    "A main screen menu bitmap failed to create. rc: %d %d\n",
                    bmpSF1Splash->rc,bmpSF2Splash->rc);
            S_MsgBox(mainWindow,"WARNING",tmp,S_MSGBOX_OK);
            if ( bmpSF1Splash ) {
               delete bmpSF1Splash;
               bmpSF1Splash = NULL;
	       }
            if ( bmpSF2Splash ) {
               delete bmpSF2Splash;
               bmpSF2Splash = NULL;
               }
            }

         mainWindow->Show();

         if ( internetDialog )
            internetDialog->Show();
         if ( intranetDialog )
            intranetDialog->Show();

         if ( bmpSF1Splash && bmpSF2Splash )
            splashThread = new S_Thread(_SplashThreadFunc,(void *)this);
         else
            mainWindow->Close();
         }
      }
   }


int
SndCueWav(int currentIndex,int newIndex,int objectIndex) {
   if ( sf && sf->mwl && sf->profile->sound &&
        sf->mode == SF_CLIENT_DATA_ENGINE_DISPLAY ) {
      S_WaveData *nw = sf->mwl->Wave(newIndex);
      if ( !nw )
         return currentIndex;
#     ifdef WIN32
         if ( !nw->rc ) {
            if ( objectIndex >= 0 && objectIndex < game->MaxObjects() &&
                 sf->engine->displayEngine->onScreen[objectIndex] ) {
               sf->audio->Play(nw);
               nw->rc = 1;
               }
            else if ( objectIndex < 0 ) {
               sf->audio->Play(nw);
               nw->rc = 1;
               }
	    }
#     endif
      S_WaveData *cw = sf->mwl->Wave(currentIndex);
      if ( !cw )
         return newIndex;
      if ( nw->priority < cw->priority )
         return currentIndex;
      return newIndex;
      }
   return 0;
   }

int
_SplashThreadFunc(void *arg) {
   SF_Client *c = (SF_Client *)arg;
   if ( !c->bmpSDSSplash || !c->bmpSF1Splash || !c->bmpSF2Splash )
      return 0;
   c->splashThreadKeepRunning = TRUE;
   S_Timer t(7250000);
   int firstTime = TRUE;
   int oldMenuItem = 0;
   int menuItem = 0;
   c->ResetScrollingText();
   int shortIntro = sf->profile->Exists();
   if ( shortIntro )
      t.Set(0);
   while ( c->splashThreadKeepRunning ) {
      if ( !c->engine ) {
         if ( c->bmpSplash == NULL && firstTime && !shortIntro ) {
            char *filename = (char *)S_malloc( strlen(SF_WaveFileDir) +
                                             strlen("Intro.wav") + 1 );
            sprintf(filename,"%sIntro.wav",SF_WaveFileDir);
            c->introWav = new S_WaveData(filename);
            S_free(filename);
            c->audio->Play(c->introWav);
            if ( c->introWav->rc < 0 )
               t.Set(0);
	    }
         else if ( c->bmpSplash == NULL && !firstTime ) {
            c->bmpSplash = c->bmpSDSSplash;
            t.Set(12750000);
            if ( shortIntro )
               t.Set(2000000);
            if ( c->bmpSplash->rc < 0 )
               t.Set(0);
            c->mainWindow->Refresh();
            }
         else if ( c->bmpSplash == c->bmpSDSSplash ) {
            if ( sf->nagDialog )
               sf->nagDialog->Show();
            if ( c->profile->music )
               c->mml->PlayLoop();
            c->bmpSplash = c->bmpSF1Splash;
            c->runCredits = TRUE;
            c->y0Credits = c->mainWindow->cyCanvas;
            c->mainWindow->Refresh();
            t.Set(100000);
            if ( c->hostname && intranetDialog ) {
               intranetDialog->SetSize(540,220);
               intranetDialog->SetPosition(_root->cx/2-270,_root->cy/2-110);
	       }
            }
         else if ( !c->engine && c->runCredits == TRUE && c->backBuffer ) {
            c->y0Credits--;
            c->DrawCredits();

            if ( sf->mouseMenuEnabled ) {

               if ( menuItem != c->mouseMenuItem &&
                    c->mouseMenuItem >= 0 && c->mouseMenuItem < 5 ) {
                  oldMenuItem = menuItem;
                  menuItem = c->mouseMenuItem;
                  if ( c->profile->sound ) {
                     S_WaveData *w = c->menuPing;
                     if (w) c->audio->Play(w);
	             }
	          }
               if ( menuItem >= 0 && menuItem < 5 ) {
                  c->DrawMenuItem(oldMenuItem,c->bmpSF1Splash);
                  c->DrawMenuItem(menuItem,c->bmpSF2Splash);
                  }
	       }

            if ( c->backBuffer ) {
               c->backBuffer->Blit( c->mainWindow,0,-c->dy2,
                                    S_CANVASBUFFER_BLIT_RECTANGLES );
               c->backBuffer->ResetRects();
	       }

            t.Set(100000);
	    }
         }

      if ( firstTime )
         firstTime = FALSE;

      t.WaitFor();
      }
   return 0;
   }


int
SF_Client::Run(char *serverString) {
   if ( root )
      root->Run();
   else {
      StartEngine(serverString);
      _OsSleep(120000000);
      StopEngine();
      }
   return 0;
   }

SF_Client::~SF_Client() {
   if ( msgBox ) {
      msgBox->selfDelete = TRUE;
      msgBox->Close();
      msgBox = NULL;
      }
   if ( splashThread ) {
      splashThreadKeepRunning = FALSE;
      splashThread->WaitFor();
      delete splashThread;
      splashThread = NULL;
      }
   if ( scrollingText ) {
      delete scrollingText;
      scrollingText = NULL;
      }
   if ( audio ) {
      audio->Stop(audio->wavePlaying);
      delete audio;
      audio = NULL;
      }
   if ( introWav ) {
      delete introWav;
      introWav = NULL;
      }
   if ( bmpSDSSplash ) {
      delete bmpSDSSplash;
      bmpSDSSplash = NULL;
      }
   if ( bmpSF2Splash ) {
      delete bmpSF2Splash;
      bmpSF2Splash = NULL;
      }
   if ( bmpSF1Splash ) {
      delete bmpSF1Splash;
      bmpSF1Splash = NULL;
      }
   if ( bmpBanner ) {
      delete bmpBanner;
      bmpBanner = NULL;
      }
   if ( urlBanner ) {
      S_free(urlBanner);
      urlBanner = NULL;
      }
   if ( bmpBannerFilename ) {
      S_free(bmpBannerFilename);
      bmpBannerFilename = NULL;
      }
   if ( files ) {
      for ( int i=0; i<numFiles; i++ )
         if ( files[i] ) {
            S_free(files[i]);
            files[i] = NULL;
	    }
      numFiles = 0;
      S_free(files);
      files = NULL;
      }
   if ( wfiles ) {
      for ( int i=0; i<numwFiles; i++ )
         if ( wfiles[i] ) {
            S_free(wfiles[i]);
            wfiles[i] = NULL;
	    }
      numwFiles = 0;
      S_free(wfiles);
      wfiles = NULL;
      }
   if ( downloadDialog ) {
      downloadDialog->Delete();
      downloadDialog = NULL;
      }
   if ( wdownloadDialog ) {
      wdownloadDialog->Delete();
      wdownloadDialog = NULL;
      }
   StopEngine();
   if ( engine && engine->engineThread )
      engine->engineThread->WaitFor();
   if ( mml ) {
      delete mml;
      mml = NULL;
      }
   if ( menuPing ) {
      delete menuPing;
      menuPing = NULL;
      }
   if ( mwl ) {
      mwl->DeleteAll();
      delete mwl;
      mwl = NULL;
      }
   if ( msl ) {
      msl->DeleteAll();
      delete msl;
      msl = NULL;
      }
   if ( root ) {
      delete root;
      root = NULL;
      }
   if ( profile ) {
     /*profile->Save();*/
      delete profile;
      profile = NULL;
      }
   if ( hostname ) {
      S_free(hostname);
      hostname = NULL;
      }
   if ( backBuffer ) {
      S_CanvasBuffer *tmp = backBuffer;
      backBuffer = NULL;
      delete tmp;
      }
   if ( font ) {
      delete font;
      font = NULL;
      }
   }



int
SF_Client::CreateNewPlayerOnNexus(char *nexus,char *name,char *password,
                                  char *email) {
   slong rc = SF_NEXUS_CONNECTFAILED;
   S_Client c;
   c.SetTimeOut(SF_CLIENT_TIMEOUT);
   if ( c.Connect(nexus,SF_NEXUS_PORT,SF_NEXUS_NEWPLAYER)==S_CLIENT_OK ) {
      rc = SF_NEXUS_OK;
      S_Packet packet;
      packet.PushString(name);
      packet.PushString(password);
      packet.PushString(email);
      c.WritePacket(&packet);
      c.ReadLong(&rc);
      c.Disconnect();
      }
   return rc;
   }


int
SF_Client::PlayerExistsOnNexus(char *nexus,char *name) {
   slong rc = SF_NEXUS_CONNECTFAILED;
   S_Client c;
   c.SetTimeOut(SF_CLIENT_TIMEOUT);
   if ( c.Connect(nexus,SF_NEXUS_PORT,SF_NEXUS_PLAYEREXISTS)==S_CLIENT_OK ) {
      S_Packet packet;
      packet.PushString(name);
      packet.PushString("");
      packet.PushString("");
      c.WritePacket(&packet);
      c.ReadLong(&rc);
      c.Disconnect();
      }
   return rc == SF_NEXUS_NAMEALREADYEXISTS;
   }


int
SF_Client::GetLeaderBoardDataFromNexus(char *nexus,slong numPlayers,
                                       char *keyword,S_Packet *ipacket) {
   slong rc = SF_NEXUS_CONNECTFAILED;
   S_Client c;
   c.SetTimeOut(SF_CLIENT_TIMEOUT);
   if ( c.Connect(nexus,SF_NEXUS_PORT,SF_NEXUS_GETHIGHSCORELIST)==S_CLIENT_OK){
      S_Packet packet;
      packet.PushString(keyword);
      packet.PushLong(numPlayers);
      c.WritePacket(&packet);
      c.ReadLong(&rc);
      if ( rc == SF_NEXUS_OK ) {
         ipacket->Flush();
         c.ReadPacket(ipacket);
         if ( !ipacket->readComplete )
            rc = SF_NEXUS_NOHIGHSCORELIST;
         }
      }
   return rc;
   }


int
SF_Client::SetPlayerDataOnNexus(char *nexus,SF_Player *player) {
   slong rc = SF_NEXUS_CONNECTFAILED;
   S_Client c;
   c.SetTimeOut(SF_CLIENT_TIMEOUT);
   if ( c.Connect(nexus,SF_NEXUS_PORT,SF_NEXUS_SETPLAYERDATA)==S_CLIENT_OK) {
      S_Packet packet;
      packet.PushString(player->playerName.str);
      packet.PushString(player->password.str);
      c.WritePacket(&packet);
      c.ReadLong(&rc);
      if ( rc == SF_NEXUS_OK ) {
         char tmp[128];
         packet.Flush();

         packet.PushString("Score");
         sprintf(tmp,"%ld",player->score);
         packet.PushString(tmp);

         packet.PushString("Kills");
         sprintf(tmp,"%ld",player->kills);
         packet.PushString(tmp);

         packet.PushString("Losses");
         sprintf(tmp,"%ld",player->losses);
         packet.PushString(tmp);

         packet.PushString("TimeOnline");
         sprintf(tmp,"%ld",player->timeOnline);
         packet.PushString(tmp);

         packet.PushString("Rank");
         sprintf(tmp,"%d",player->rank);
         packet.PushString(tmp);

         packet.PushString("Credits");
         sprintf(tmp,"%ld",player->credits);
         packet.PushString(tmp);

         packet.PushString("OSString");
         sprintf(tmp,"%s",_OsString());
         packet.PushString(tmp);

         c.WritePacket(&packet);
         }
      c.Disconnect();
      }
   return rc;
   }


int
SF_Client::SetRecordDataOnNexus(char *nexus,char *name,char *password,
                                S_Packet *ipacket) {
   slong rc = SF_NEXUS_CONNECTFAILED;
   S_Client c;
   c.SetTimeOut(SF_CLIENT_TIMEOUT);
   if ( c.Connect(nexus,SF_NEXUS_PORT,SF_NEXUS_SETPLAYERDATA)==S_CLIENT_OK) {
      S_Packet packet;
      packet.PushString(name);
      packet.PushString(password);
      c.WritePacket(&packet);
      c.ReadLong(&rc);
      if ( rc == SF_NEXUS_OK ) {
         c.WritePacket(ipacket);
         }
      c.Disconnect();
      }
   return rc;
   }


int
SF_Client::GetPlayerDataFromNexus(char *nexus,SF_Player *player)
  {
    slong rc = SF_NEXUS_CONNECTFAILED;
    S_Client c;
    c.SetTimeOut(SF_CLIENT_TIMEOUT);
    if ( c.Connect(nexus,SF_NEXUS_PORT,SF_NEXUS_GETPLAYERDATA) == S_CLIENT_OK)
      {
        S_Packet packet;
        packet.PushString(player->playerName.str);
        packet.PushString(player->password.str);
        c.WritePacket(&packet);
        c.ReadLong(&rc);
        if ( rc == SF_NEXUS_OK )
          {
            packet.Flush();
            c.ReadPacket(&packet);
            rc = SF_NEXUS_READFAILED;
            if ( packet.readComplete )
              {
                rc = SF_NEXUS_OK;
                player->score = 0;
                player->kills = 0;
                player->losses = 0;
                player->timeOnline = 0;
                player->rank = 0;
                player->credits = 0;
                char *keyword,*value;
                do
                  {
                    keyword = packet.PopString();
                    value = packet.PopString();
                    if ( keyword && value )
                      {
                        if ( strcmp(keyword,"Score") == 0 )
                          player->score = atol(value);
                        if ( strcmp(keyword,"Kills") == 0 )
                          player->kills = atol(value);
                        if ( strcmp(keyword,"Losses") == 0 )
                          player->losses = atol(value);
                        if ( strcmp(keyword,"TimeOnline") == 0 )
                          player->timeOnline = atol(value);
                        if ( strcmp(keyword,"Rank") == 0 )
                          player->rank = (char) atol(value);
                        if ( strcmp(keyword,"Credits") == 0 )
                          player->credits = (slong) atol(value);
                        S_free(keyword);
                        S_free(value);
                      }
                  } while ( keyword && value );
	      }
	  }
      c.Disconnect();
      }
    return rc;
  }


S_StaticText *nexusStatusText = NULL;

char *
SF_Client::TouchServer(ServerDataStruct *sd,
		       ushort numLoops,ushort numBytes){
  int i,j;
   S_Packet packet;
   S_StopWatch stopWatch;
   char tmp[1024];

   S_Client c;
   c.SetTimeOut(SF_CLIENT_TIMEOUT);
   if ( !c.Connect(sd->hostName,sd->port,SF_SERVER_TOUCH) ){

      if ( nexusStatusText ) {
         sprintf(tmp,"Success.");
         nexusStatusText->SetText(tmp);
         }

      packet.PushUShort(numLoops);
      packet.PushUShort(numBytes);
      c.WritePacket(&packet);
      packet.Flush();

      if ( nexusStatusText ) {
         sprintf(tmp," Waiting for %s server info . . .",sd->hostName);
         nexusStatusText->SetText(tmp);
         }

      c.ReadPacket(&packet);
      ushort numPlayers = packet.PopUShort();
      ushort  maxSlots = packet.PopUShort();
      uchar majorVersion = packet.PopUChar();
      uchar minorVersion = packet.PopUChar();
      char *osString = packet.PopString();
      char *serverName = packet.PopString();
      uchar minorminorVersion = packet.PopUChar();
      char *versionModifier = packet.PopString();
      char *versionString = packet.PopString();
      ushort numHumanPlayers = packet.PopUShort();
      ulong uptime = packet.PopULong();

      //ulong sec = uptime % 60;
      ulong minute = (uptime / 60) % 60;
      ulong hour = (uptime / 3600) % 24;
      ulong day = uptime / 86400;
      S_String upTimeStr("%dd %02d:%02d",day,hour,minute);

      sd->numPlayers = numPlayers;
      sd->maxPlayers = maxSlots;
      sd->majorVersion = majorVersion;
      sd->minorVersion = minorVersion;
      sd->minorminorVersion = minorminorVersion;
      if ( versionModifier )
         strcpy(sd->versionModifier,versionModifier);
      if ( versionString )
	strcpy(sd->versionString,versionString);
      if ( osString )
         strcpy(sd->osString,osString);
      if ( serverName )
         strcpy(sd->serverName,serverName);

      if ( packet.readComplete && serverName && osString ) {

         packet.Flush();

         if ( nexusStatusText ) {
            sprintf(tmp,"Pinging %s",sd->hostName);
            nexusStatusText->SetText(tmp);
            }

         stopWatch.Start();
         for ( i=0; i<numLoops; i++ ) {
            for ( j=0; j<numBytes; j++ )
               packet.PushUChar((uchar)i);
            c.WritePacket(&packet);
            packet.Flush();

            if ( nexusStatusText ) {
               sprintf(tmp,"%s . . .",tmp);
               nexusStatusText->SetText(tmp);
               }

            c.ReadPacket(&packet);

            if ( nexusStatusText ) {
               sprintf(tmp,"%s :)",tmp);
               nexusStatusText->SetText(tmp);
               }

            packet.Flush();
            }
         stopWatch.Stop();
         }

      if ( nexusStatusText ) {
         sprintf(tmp,"Disconnecting from %s",sd->hostName);
         nexusStatusText->SetText(tmp);
         }

      c.Disconnect();

      if ( serverName && osString ) {
         double lagUsec=(stopWatch.dtSec*1000000 + stopWatch.dtUsec)/numLoops;

         touchString.Create(
	      "%s (%s) [ v%s | %s | %d(%d)/%d players | %.0fms | %s ]",
              serverName,sd->hostName,
              VersionString(majorVersion,minorVersion,minorminorVersion,
                            versionModifier,versionString),
              osString,numPlayers,numHumanPlayers,maxSlots,
              lagUsec/1000.0,upTimeStr.str);

         S_free(osString);
         S_free(serverName);
         S_free(versionModifier);
         S_free(versionString);
         }
      else {
	touchString.Create("\?\? (%s) [ \?\? | \?\?/\?\? players | \?\?ms ]",
			   sd->hostName);
         }

      }
   else {

      if ( nexusStatusText ) {
         sprintf(tmp," Failed to connect to %s.",sd->hostName);
         nexusStatusText->SetText(tmp);
         }
      touchString.Create(
              "\?\? (%s) [ \?\? | \?\?/\?\? players | \?\?ms ]",sd->hostName);
      }

   return touchString.str;
   }
/* ************************************************************************* */


char *oldACText = "Thank you for your registration and support of\n"
                  "Stellar Frontier. To help keep piracy down,\n"
                  "you will need an updated "SF_REGISTERED_CLUB" number\n"
                  "(your serial number) with Stellar Frontier 1.03\n"
                  "or later.\n"
                  "\n" 
                  "Stardock has emailed these updated numbers to\n"
                  "your account of record. If you have not received\n"
                  "yours, please visit http://www.stardock.com/support.\n"
                  "You can enter the email address you ordered under\n"
                  "and will be sent the updated serial and a link\n"
                  "to the latest version of Stellar Frontier.\n"
                  "\n"
                  "Please contact sales@stardock.com with your old\n"
                  "and new email addresses if this information has\n"
                  "changed.\n"
                  "\n"
                  "Thanks for your legitimate support of Stellar Frontier!\n";

/* ************************ Event Handling code below ********************** */
int StardockNetMenuItemHandler(S_MenuItem *);
int InternetMenuItemHandler(S_MenuItem *);
int IntranetMenuItemHandler(S_MenuItem *);
int DisconnectMenuItemHandler(S_MenuItem *);
int SinglePlayerMenuItemHandler(S_MenuItem *);
int PauseMenuItemHandler(S_MenuItem *);
int StopMenuItemHandler(S_MenuItem *);
int OptionsMenuItemHandler(S_MenuItem *);
int ExitMenuItemHandler(S_MenuItem *);

int IntranetDialogHandler(S_Dialog *);
int InternetDialogHandler(S_Dialog *);

int 
MainWindowHandler(S_StdWindow *window) {

   SF_Client *c = (SF_Client *)window->puser;

   /* *** This is a patch *** */
   if ( c->engine == NULL ) {
      if ( c->disconnectMenuItem && c->disconnectMenuItem->IsEnabled() )
         c->disconnectMenuItem->SetEnabled(FALSE);
      if ( c->stopMenuItem && c->stopMenuItem->IsEnabled() )
         c->stopMenuItem->SetEnabled(FALSE);
      if ( c->pauseMenuItem && c->pauseMenuItem->IsEnabled() )
         c->pauseMenuItem->SetEnabled(FALSE);
      }

   switch(window->msg) {
      case S_WINDOW_CREATE : {

         /* *********************** Create Menu Bar ************************ */
         c->menuBar = new S_MenuBar(window);
         c->gameMenu = new S_SubMenu(c->menuBar,"Game");

         c->stardockNetMenuItem = new S_MenuItem(c->gameMenu,
                                              "Stardock.net",
                                              StardockNetMenuItemHandler);
         c->gameMenu->AddSeparator();
         c->internetMenuItem = new S_MenuItem(c->gameMenu,
                                              "Play on the Internet",
                                              InternetMenuItemHandler);
         c->intranetMenuItem = new S_MenuItem(c->gameMenu,
                                              "Play on local intranet",
                                              IntranetMenuItemHandler);
         c->disconnectMenuItem = new S_MenuItem(c->gameMenu,"Disconnect",
                                                DisconnectMenuItemHandler);
         c->disconnectMenuItem->SetEnabled(FALSE);
         c->gameMenu->AddSeparator();

         c->singlePlayerMenuItem = new S_MenuItem(c->gameMenu,
                                                  "Play against the computer",
                                                  SinglePlayerMenuItemHandler);
         c->pauseMenuItem = new S_MenuItem(c->gameMenu,"Pause",
                                           PauseMenuItemHandler);
         c->stopMenuItem = new S_MenuItem(c->gameMenu,"Stop",
                                          StopMenuItemHandler);
         c->stopMenuItem->SetEnabled(FALSE);
         c->pauseMenuItem->SetEnabled(FALSE);

         c->gameMenu->AddSeparator();

         c->optionsMenuItem = new S_MenuItem(c->gameMenu,"Options",
                                                OptionsMenuItemHandler);
         c->gameMenu->AddSeparator();
         c->exitMenuItem = new S_MenuItem(c->gameMenu,
                                          "Exit",ExitMenuItemHandler);
         /* **************************************************************** */

         int cy = window->root->cy;
         if ( window->root->cx == 1152 )
            cy = 864;

#        ifdef WIN32
            c->font = new S_Font("Small Fonts",7);
#        else
            c->font = new S_Font(S_FONT_ARIAL,8);
#        endif
         c->backBuffer = new S_CanvasBuffer(window->root->cx,cy,
                                            window->root->bitsPerPixel);
         c->backBuffer->DrawFilledRect(0,0,
                                       c->backBuffer->cxCanvas,
                                       c->backBuffer->cyCanvas,0);
         c->backBuffer->SetFont(c->font);
         c->backBuffer->Blit( window,0,0,S_CANVASBUFFER_BLIT_ALL);

         if ( c->hostname ) {
	    if ( strcmp(c->hostname,"-nexus") == 0 ) {
               c->mouseMenuEnabled = FALSE;
               c->internetMenuItem->SetEnabled(FALSE);
               internetDialog = new S_Dialog(window,
                                             "Play on the Internet",
                                             InternetDialogHandler,0,
                                             window->cx/2-320,
                                             window->cy/2-275,
                                             640,550);
               internetDialog->selfDelete = TRUE;
	       }
            else {
               c->mouseMenuEnabled = FALSE;
               c->intranetMenuItem->SetEnabled(FALSE);
               c->profile->SetServerName(c->hostname);
               intranetDialog = new S_Dialog(
                                  window,"Play on local intranet",
                                  IntranetDialogHandler,0,
                                  -480,-220,540,220);
               intranetDialog->selfDelete = TRUE;
	       }
            }

         if ( sf->demo || sf->oldAC )
            sf->nagDialog = new SF_NagDialog(window);
	 
	 if ( sf->oldAC )
	   {
	     S_MsgBox(window,
		      "Registration",
		      oldACText,
		      S_MSGBOX_OK);
	   }
	 
         return 0;
         }

      case S_WINDOW_KEYBOARD :

         if ( c ) {

            if ( window->scancode == SF_KEY_LSHIFT ||
                 window->scancode == SF_KEY_RSHIFT )
               c->shiftDown = !window->keyup;
            if ( window->scancode == SF_KEY_LCTRL ||
                 window->scancode == SF_KEY_RCTRL )
               c->ctrlDown = !window->keyup;
            if ( !window->repeat && !window->keyup &&
                 window->scancode == SF_KEY_F10 )
               ExitMenuItemHandler(c->exitMenuItem);

            SF_Engine *e = c->engine;
            if ( e ) {
               SF_GameEngine *ge = e->gameEngine;
               SF_DisplayEngine *de = e->displayEngine;

               if ( window->scancode == SF_KEY_TAB && !window->keyup ) {
                  c->tabToggle = !c->tabToggle;
                  if ( de && de->chat &&
                       de->chat->Contains(window->mx,window->my) )
                     c->tabToggle = FALSE;
	          }

               if ( ge ) {

                  /* ************************************************** */
                  /* *** If mouse is in chat console send send all  *** */
                  /* *** to the chat console keystroke handler.  If *** */
                  /* *** it doesn't handle key the send it onward   *** */
                  /* ************************************************** */
                  if ( window->scancode != SF_KEY_LSHIFT &&
                       window->scancode != SF_KEY_RSHIFT &&
                       window->scancode != SF_KEY_RCTRL &&
                       window->scancode != SF_KEY_LCTRL ) {
                     if ( (c->tabToggle ||
                           de && de->chat &&
                           de->chat->Contains(window->mx,window->my)) ) {
                        int rc = FALSE;
                        rc = de->chat->KeyPressed(window->scancode,
                                                  c->shiftDown,
                                                  window->keyup,
						  c->ctrlDown);
                        if ( rc )
                           return 0;
                        }
		     }

                  if ( !window->repeat ) {
                     if ( !window->keyup ) {

                        if ( window->scancode == SF_KEY_ESC ) {
#                          ifdef OS2
                              WinSetPointerPos(HWND_DESKTOP,
                                               16,
                                               window->cyClient-28);
#                          endif
#                          ifdef WIN32
                              SetCursorPos(16,28);
#                          endif
                           return 0;
		           }

                        }

                     uchar msg = SF_MSG_KEYPRESSED_4;
                     if ( window->keyup )
                        msg = SF_MSG_KEYRELEASED_4;

                     if ( !ge->FilterKeystroke(msg,window->scancode) )
                        e->PushKeyboardMsg(msg,window->scancode);

		     }

                  }
	       }
	    }

         return 0;

      case S_WINDOW_MOUSE :

         if ( window->button == 1 )
            window->Focus();

         if ( c->engine && c->engine->displayEngine &&
              c->engine->displayEngine->video )
            if ( c->engine->displayEngine->video->Contains(window->mx,
                                                           window->my) ) {
               c->engine->displayEngine->video->HandleMouseClick(
                                                       window->button,
                                                       window->mx,
                                                       window->my,
                                                       window->keyup);
               return 0;
	       }

         if ( c->engine && c->engine->displayEngine &&
              c->engine->displayEngine->chat )
            if ( c->engine->displayEngine->chat->Contains(window->mx,
                                                          window->my) ) {
               c->engine->displayEngine->chat->HandleMouseClick(
                                                       window->button,
                                                       window->mx,
                                                       window->my,
                                                       window->keyup);
               return 0;
	       }

         if ( c->engine && c->engine->displayEngine ) {
            if ( c->engine->displayEngine->MouseClick(window->button,
                                                      window->mx,
                                                      window->my,
                                                      window->keyup) )
               return 0;
	    }

         /* ******************************************************* */
         /* *** Code for handling mouse selection of menu items *** */
         /* ******************************************************* */
         if ( c->mouseMenuEnabled &&
              !c->engine || (c->engine && !c->engine->run) ) {
            c->mouseMenuItem = c->InSplash(window->mx,window->my);
            if ( window->button == 1 && !window->keyup && 
                 c->bmpSplash && c->bmpSplash == c->bmpSF1Splash )
               switch(c->mouseMenuItem) {
                  case 0:
                     InternetMenuItemHandler(c->internetMenuItem);
                     break;
                  case 1:
                     IntranetMenuItemHandler(c->intranetMenuItem);
                     break;
                  case 2:
                     SinglePlayerMenuItemHandler(c->singlePlayerMenuItem);
                     break;
                  case 3:
                     OptionsMenuItemHandler(c->optionsMenuItem);
                     break;
                  case 4:
                     ExitMenuItemHandler(c->exitMenuItem);
                     break;
	          }
	    }
         /* ******************************************************* */

         if ( window->button == 0 ) {
            /*if ( c->engine )
               window->Focus();*/

            if ( window->my>2*(window->root->cyTitleBar+window->root->cyMenu)){
               window->ShowTitleBar(FALSE);
               c->dy1 = 0;
               c->dy2 = 0;
	       }
            else if ( window->mx < 100/*window->cxClient - sf->cxPanel*/ ) {
               window->ShowTitleBar(TRUE);
               c->dy1 = window->root->cyTitleBar + window->root->cyMenu;
               c->dy2 = c->mainWindow->root->cyTitleBar;
	       }

            return 0;
	    }

         if ( window->keyup && !(window->button == 2) )
            return 0;

         if ( c )
	   {
	     SF_Engine *e = c->engine;
	     if ( e )
	       {
		 SF_GameEngine *ge = e->gameEngine;
		 SF_DisplayEngine *de = e->displayEngine;
		 SF_NetEngine *ne = e->netEngine;
		 if ( ge )
		   {
		     
		     if ( ge->controlsDisabled )
		       {
			 return 0;
		       }
		     
		     uchar msg;
		     slong x = 0;
		     slong y = 0;
		     if ( de && ge->mapSprite && window->mx <
			  window->cxCanvas-10-(int)ge->mapSprite->cx )
		       {
			 x = (window->mx-de->scrX0)*sub_scale*de->tacticalZoom;
			 y = (window->my-de->scrY0)*sub_scale*de->tacticalZoom;
		       }
		     else if ( de &&
			       de->map &&
			       de->map->Contains(window->mx,window->my) )
		       {
			 x = (window->mx - de->map->x0 - 
			      de->map->buffer->cxCanvas/2) *
			   sub_scale*de->strategicZoom;
			 y = (window->my - de->map->y0 - 
			      de->map->buffer->cyCanvas/2) *
			   sub_scale*de->strategicZoom;
		       }
		     msg = SF_MSG_REL_MOUSEBUTTONDOWN_12;
		     if ( window->keyup )
		       msg = SF_MSG_REL_MOUSEBUTTONUP_12;
		     
		     short button = window->button;
		     
		     SF_Msg gmsg;
		     int sockType = S_CLIENT_TCP;
		     if ( ne && window->button == 1 )
		       sockType = ne->fastDataMode;
		     
		     /* ************************************************** */
		     /* *** Avoid stacking up of mouse one fire events *** */
		     /* ************************************************** */
		     ge->msgBuffer->Rewind();
		     if ( button == 1 && (IsInInternetCafe(&game->ship[localSlotNumber]) ||
					  game->ship[localSlotNumber].IsWarping() ||
					  ge->msgBuffer->GetNextMsg(ge->localSlot,
								    &gmsg,
								    sockType,
								    SF_MSG_REL_MOUSEBUTTONDOWN_12,
								    FALSE)) )
		       {
			 return 0;
		       }
		     /* ************************************************** */
		     
		     ge->msgBuffer->mutex.Request();
		     ge->msgBuffer->PushMsg((uchar)msg,
					    (ushort)ge->localSlot,
					    (uchar)ge->frame,
					    (uchar)window->button,
					    (slong)x,
					    (slong)y,
					    (int)sockType);
		     ge->msgBuffer->mutex.Release();
		     
		     if ( ne )
		       {
			 ne->slot->writeBuffer.mutex.Request();
			 mstate = 10;
			 ne->slot->writeBuffer.PushMsg((uchar)msg,
						       (ushort)ge->localSlot,
						       (uchar)ge->frame,
						       (uchar)window->button,
						       (slong)x,
						       (slong)y,
						       (int)sockType);
			 mstate = 0;
			 ne->slot->writeBuffer.mutex.Release();
		       }
		   }
	       }
	     
	   }

         return 0;

      case S_WINDOW_SIZE :
         if ( c->engine && c->engine->displayEngine )
            c->engine->displayEngine->repaint = TRUE;
         break;

      case S_WINDOW_PAINT : {
         if ( c->engine && c->engine->displayEngine ) {
            c->engine->displayEngine->repaint = TRUE;
            }
         else {
            c->DrawSplash();
            c->DrawCredits();
            c->backBuffer->Blit( window,0,-c->dy2,S_CANVASBUFFER_BLIT_ALL);
            if ( c->backBuffer->cyCanvas < _root->cy )
               window->DrawFilledRect(0,c->backBuffer->cyCanvas,
                                      _root->cx,_root->cy,0);
            }

         break;
         }

      case S_WINDOW_CLOSE :

         c->mainWindow->closing = TRUE;

         c->StopEngine(TRUE);
         c->WaitForEngine();

         if ( c->profile )
            c->profile->Save();

         if (c->exitMenuItem) {
            delete c->exitMenuItem;
            c->exitMenuItem = NULL;
            }
         if (c->optionsMenuItem) {
            delete c->optionsMenuItem;
            c->optionsMenuItem = NULL;
            }
         if (c->singlePlayerMenuItem) {
            delete c->singlePlayerMenuItem;
            c->singlePlayerMenuItem = NULL;
            }
         if (c->disconnectMenuItem) {
            delete c->disconnectMenuItem;
            c->disconnectMenuItem = NULL;
            }
         if (c->intranetMenuItem) {
            delete c->intranetMenuItem;
            c->intranetMenuItem = NULL;
            }
         if (c->internetMenuItem) {
            delete c->internetMenuItem;
            c->internetMenuItem = NULL;
            }
         if (c->stardockNetMenuItem) {
            delete c->stardockNetMenuItem;
            c->stardockNetMenuItem = NULL;
            }
         if (c->pauseMenuItem) {
            delete c->pauseMenuItem;
            c->pauseMenuItem = NULL;
            }
         if (c->stopMenuItem) {
            delete c->stopMenuItem;
            c->stopMenuItem = NULL;
            }
         if (c->gameMenu) {
            delete c->gameMenu;
            c->gameMenu = NULL;
            }
         if (c->menuBar) {
            delete c->menuBar;
            c->menuBar = NULL;
            }

         break;

      }

   return 0;
   }


int
ExitMenuItemHandler(S_MenuItem *menuItem) {
   sf->mouseMenuEnabled = FALSE;
   if ( menuItem && menuItem->window ) {
      S_MsgBox msg(menuItem->window,"Exit",
                   "Do you really want to exit?",S_MSGBOX_YESNO);
      if ( msg.rc == S_MSGBOX_YES) {
         menuItem->window->Close();
         }
      }
   sf->mouseMenuEnabled = TRUE;
   return 0;
   }


int
SinglePlayerMenuItemHandler(S_MenuItem *menuItem) {
   sf->mouseMenuEnabled = FALSE;
   sf->singlePlayerMenuItem->SetEnabled(FALSE);
   SF_LoadScenarioDialog *dialog = new SF_LoadScenarioDialog(menuItem->window);
   dialog->selfDelete = TRUE;
   return 0;
   }


int
PauseMenuItemHandler(S_MenuItem *menuItem) {
   SF_Client *c = (SF_Client *)menuItem->window->puser;
   if ( c && c->engine && c->engine->gameEngine &&
        c->engine->state == SF_CLIENT_SINGLEPLAYER ) {
      if ( c->engine->gameEngine->paused ) {
         c->engine->gameEngine->paused = FALSE;
         menuItem->SetCheck(FALSE);
         c->engine->gameEngine->pauseEvent.Set();
         }
      else {
         c->engine->gameEngine->paused = TRUE;
         menuItem->SetCheck(TRUE);
         c->engine->gameEngine->pauseEvent.Reset();
         }
      }
   return 0;
   }



int
StopMenuItemHandler(S_MenuItem *menuItem) {
   SF_Client *c = (SF_Client *)menuItem->window->puser;
   if ( c && c->engine && c->engine->state == SF_CLIENT_SINGLEPLAYER )
      c->StopEngine();
   return 0;
   }


int
DisconnectMenuItemHandler(S_MenuItem *menuItem) {
   SF_Client *c = (SF_Client *)menuItem->window->puser;
   if ( c && c->engine && c->engine->state == SF_CLIENT_CONNECTED )
      c->StopEngine();
   return 0;
   }



/* ************************************************************************* */
/* **************************** Options Dialog ***************************** */
/* ************************************************************************* */
S_Dialog *optionsDialog;
int OptionsDialogHandler(S_Dialog *);
int OptionsMenuItemHandler(S_MenuItem *menuItem) {
   sf->mouseMenuEnabled = FALSE;
   sf->optionsMenuItem->SetEnabled(FALSE);
   optionsDialog = new S_Dialog(menuItem->window,"Options",
                                   OptionsDialogHandler,0,
                                   menuItem->window->root->cx/2-240,
                                   menuItem->window->root->cy/2-180,
                                   480,360);
   optionsDialog->selfDelete = TRUE;
   optionsDialog->Show();
   return 0;
   }

S_GroupBox *soundGroupBox;
S_CheckBox *soundCheckBox;
S_ScrollBar *soundVolumeScrollBar;

S_GroupBox *musicGroupBox;
S_CheckBox *musicCheckBox;
S_ScrollBar *musicVolumeScrollBar;

S_GroupBox *networkGroupBox;
S_CheckBox *tcpipOnlyCheckBox;

S_Button *optionsOkButton;
int OptionsOkButtonHandler(S_Button *);

int 
SoundVolumeScrollBarHandler(S_ScrollBar *sb) {
   if ( sb->msg == S_SCROLLBAR_ENDSCROLL ) {
      sf->profile->soundVolume = sb->GetValue();
      sf->mwl->SetVolume(sf->profile->soundVolume);
      sf->audio->SetVolume(sf->profile->soundVolume);
      if ( sf->profile->sound ) {
         S_WaveData *w = sf->menuPing;
         if (w) sf->audio->Play(w);
         }
      }
   return 0;
   }


int
SoundCheckBoxHandler(S_CheckBox *cb) {
   sf->profile->sound = cb->IsChecked();
   return 0;
   }

int
MusicVolumeScrollBarHandler(S_ScrollBar *sb) {
   if ( sb->msg == S_SCROLLBAR_ENDSCROLL ) {
      sf->profile->musicVolume = sb->GetValue();
      sf->mml->SetVolume(sf->profile->musicVolume);
      }
   return 0;
   }

int
MusicCheckBoxHandler(S_CheckBox *cb) {
   sf->profile->music = cb->IsChecked();
   if ( !sf->profile->music )
      sf->mml->Stop();
   else
      sf->mml->PlayLoop();
   return 0;
   }

int
TcpipOnlyCheckBoxHandler(S_CheckBox *cb) {
   sf->profile->tcpipOnly = cb->IsChecked();
   return 0;
   }

int
OptionsDialogHandler(S_Dialog *d) {

   int margin = 10;
   int cxButton = 70;
   int cyButton = 35;
   int labelHeight = 16;
   int cxGroup = (d->cxClient - 3*margin)/2;
   int cyGroup = (d->cyClient - 4*margin - cyButton)/2;
   int width = cxGroup - 2*margin;
   int x = margin;
   int y = margin;

   switch(d->msg) {
      case S_WINDOW_CREATE :

         soundGroupBox = new S_GroupBox(d,"Sound",x,y,cxGroup,cyGroup);

         x += margin;
         y += 2*margin;
         soundCheckBox = 
            new S_CheckBox(d,SoundCheckBoxHandler,
                           "Enable",x,y,width,labelHeight);

         soundCheckBox->SetCheck(sf->profile->sound);

         y = margin + cyGroup - margin - labelHeight;
         soundVolumeScrollBar = new S_ScrollBar(d,
                                                SoundVolumeScrollBarHandler,
                                                x,y,width,labelHeight,
                                                S_SCROLLBAR_HORIZONTAL);
         soundVolumeScrollBar->AddLabel("Volume",width,labelHeight);
         soundVolumeScrollBar->SetRange(0,100);
         soundVolumeScrollBar->SetValue(sf->profile->soundVolume);

         x += cxGroup;
         y = margin;
         musicGroupBox = new S_GroupBox(d,"Music",x,y,cxGroup,cyGroup);

         x += margin;
         y += 2*margin;
         musicCheckBox = 
            new S_CheckBox(d,MusicCheckBoxHandler,
                           "Enable",x,y,width,labelHeight);

         musicCheckBox->SetCheck(sf->profile->music);

         y = margin + cyGroup - margin - labelHeight;
         musicVolumeScrollBar = new S_ScrollBar(d,NULL,x,y,width,labelHeight,
                                                S_SCROLLBAR_HORIZONTAL);
         musicVolumeScrollBar->AddLabel("Volume",width,labelHeight);
         musicVolumeScrollBar->SetRange(0,100);
         musicVolumeScrollBar->SetValue(sf->profile->musicVolume);

#        ifdef WIN32
            musicVolumeScrollBar->SetEnabled(FALSE);
#        endif

         x = margin;
         y = 2*margin + cyGroup;
         networkGroupBox = new S_GroupBox(d,"Network",x,y,
                                          2*cxGroup+margin,cyGroup);

         x += margin;
         y += 2*margin;
         tcpipOnlyCheckBox = 
            new S_CheckBox(d,TcpipOnlyCheckBoxHandler,
                           "Firewall mode (Uses TCP only, no UDP)",
                           x,y,2*width,labelHeight);

         tcpipOnlyCheckBox->SetCheck(sf->profile->tcpipOnly);

         y += margin + labelHeight;
         optionsOkButton = new S_Button(d,OptionsOkButtonHandler,
                                        "Ok",
                                        d->cxCanvas-margin-cxButton,
                                        d->cyCanvas-margin-cyButton,
                                        cxButton,cyButton);
         break;

      case S_WINDOW_DESTROY :

         sf->profile->soundVolume = soundVolumeScrollBar->GetValue();
         sf->profile->musicVolume = musicVolumeScrollBar->GetValue();

         delete soundGroupBox;
         delete soundCheckBox;
         delete soundVolumeScrollBar;

         delete musicGroupBox;
         delete musicCheckBox;
         delete musicVolumeScrollBar;

         delete networkGroupBox;
         delete tcpipOnlyCheckBox;

         delete optionsOkButton;

         sf->mouseMenuEnabled = TRUE;
         sf->optionsMenuItem->SetEnabled(TRUE);
         break;

      }
   return 0;
   }


int
OptionsOkButtonHandler(S_Button *button)
{
  if ( button && button->parent )
    {
      button->parent->Close();
    }
  return 0;
}



/* ************************************************************************* */
/* ************************** Sound/Music Dialog *************************** */
/* ************************************************************************* */


/* ************************************************************************* */
/* ************************** Load Scenario Dialog ************************* */
/* ************************************************************************* */
SF_LoadScenarioDialog::SF_LoadScenarioDialog(S_Window *iparent) :
   S_Window( NULL,iparent,"Load a scenario",NULL,0,
             iparent->root->cx/2-240,iparent->root->cy/2-180,
             480,360,S_WINDOW_DIALOG,NULL) {

   HandleCreateMsg();
   created = TRUE;
   Show();
   }

SF_LoadScenarioDialog::~SF_LoadScenarioDialog() {
   }


int
LoadScenarioOkButtonHandler(S_Button *button) {
   if ( button &&  button->parent ) {
      SF_LoadScenarioDialog *d = (SF_LoadScenarioDialog *)button->parent;
      int index = d->scenarioListBox->GetIndex();
      if ( index >= 0 ) {
         sf->StartEngine(NULL,d->dirList->file[index]);
         button->parent->Close();
         }
      }
   sf->connectedThroughNexus = FALSE;
   return 0;
   }


int
LoadScenarioCancelButtonHandler(S_Button *button) {
   if ( button &&  button->parent )
      button->parent->Close();
   return 0;
   }


int
ScenarioListBoxHandler(S_ListBox *listBox) {
   int index = listBox->GetIndex();
   SF_LoadScenarioDialog *d = (SF_LoadScenarioDialog *)listBox->parent;
   if ( index >= 0 ) {
      char *serverName = ReadValueFromScenarioFile(
                            d->dirList->file[index],"desc");
      if ( serverName ) {
         d->descriptionTextEdit->SetText(serverName);
         S_free(serverName);
         }
      }
   return 0;
   }


int
SF_LoadScenarioDialog::HandleCreateMsg() {

   int i;
   int margin = 10;
   int cxButton = 70;
   int cyButton = 35;
   int labelHeight = 16;
   int cxListBox  = cxClient - 2*margin;
   int cyListBox = 100;
   int cyTextEdit = cyClient - 4*margin - cyButton - cyListBox - 2*labelHeight;
   int x,y;

   dirList = new S_DirList(SF_ScenarioFileDirMask);

   x = margin;
   y = margin + labelHeight;
   scenarioListBox = new S_ListBox(this,ScenarioListBoxHandler,
                                      x,y,cxListBox,cyListBox);
   scenarioListBox->AddLabel("Scenarios",cxListBox,labelHeight);

   for ( i=0; i<dirList->numFiles; i++ ) {
      char *scenarioName=ReadValueFromScenarioFile(dirList->file[i],"name");
      if ( scenarioName ) {
	if ( scenarioListBox )
	  scenarioListBox->AddItem(scenarioName);
         S_free(scenarioName);
         }
      else
	if ( scenarioListBox )
	  scenarioListBox->AddItem("???");
      }

   y += cyListBox + margin + labelHeight;
   descriptionTextEdit = new S_TextEdit(this,NULL,NULL,x,y,
                                        cxListBox,cyTextEdit);
   descriptionTextEdit->AddLabel("Description",cxListBox,labelHeight);
   descriptionTextEdit->SetReadOnly(TRUE);

   y += cyTextEdit + margin;
   x = cxClient - margin - cxButton;
   okButton = new S_Button(this,LoadScenarioOkButtonHandler,"Ok",
                              x,y,cxButton,cyButton);

   x -= cxButton + margin;
   cancelButton = new S_Button(this,LoadScenarioCancelButtonHandler,
                                  "Cancel",x,y,cxButton,cyButton);
   return 0;
   }


int
SF_LoadScenarioDialog::HandlePaintMsg(void) {
   DrawFilledRect(0,0,cxClient,cyClient,backGroundRGB);
   return 0;
   }


int
SF_LoadScenarioDialog::HandleCloseMsg() {
   if ( scenarioListBox ) {
      delete scenarioListBox;
      scenarioListBox = NULL;
      }
   if ( descriptionTextEdit ) {
      delete descriptionTextEdit;
      descriptionTextEdit = NULL;
      }
   if ( okButton ) {
      delete okButton;
      okButton = NULL;
      }
   if ( cancelButton ) {
      delete cancelButton;
      cancelButton = NULL;
      }
   if ( dirList ) {
      delete dirList;
      dirList = NULL;
      }
   sf->mouseMenuEnabled = TRUE;
   sf->singlePlayerMenuItem->SetEnabled(FALSE);
   return 0;
   }

/* ************************************************************************* */
/* ************************** Load Scenario Dialog ************************* */
/* ************************************************************************* */


int
IsValidNick(char *nick,char *ch)
{
  if ( !nick )
    {
      return FALSE;
    }

  int len = strlen(nick);
  if ( len > 30 )
    {
      return FALSE;
    }

  char *nickChars = SF_CLIENT_NICKCHARS;
  for ( int i=0; i<len; i++ )
    {
      int found = FALSE;
      for ( int j=0; j<(int)strlen(nickChars); j++ )
	{
	  if ( nick[i] == nickChars[j] )
	    {
	      found = TRUE;
	    }
	}
      if ( !found )
	{
	  if ( ch )
	    {
	      *ch = nick[i];
	    }
	  return FALSE;
	}
    }

  return TRUE;
}


/* ************************************************************************* */
/* ************************* Player login Dialog *************************** */
/* ************************************************************************* */
S_TextBox *newPlayerAgeTextBox = NULL;
S_ComboBox *newPlayerSexComboBox = NULL;
S_TextBox *newPlayerFirstNameTextBox = NULL;
S_TextBox *newPlayerLastNameTextBox = NULL;
S_TextBox *newPlayerEmailTextBox = NULL;
S_TextBox *newPlayerPasswordTextBox = NULL;
S_TextBox *newPlayerNameTextBox = NULL;
S_TextBox *newPlayerShipTextBox = NULL;
S_ListBox *playerListBox = NULL;
S_CheckBox *nexusCheckBox = NULL;
S_CheckBox *retrieveCheckBox = NULL;
int PlayerListBoxHandler(S_ListBox *);

S_Thread *newPlayerThread = NULL;

int
NewPlayerThread(void *iarg)
{
  S_Button *button = (S_Button *)iarg;
  
  button->SetEnabled(FALSE);
  
  char *name = newPlayerNameTextBox->GetString();
  char *password = newPlayerPasswordTextBox->GetString();
  char *email = newPlayerEmailTextBox->GetString();
  char *ship = newPlayerShipTextBox->GetString();
  char *age = newPlayerAgeTextBox->GetString();
  char *sex = newPlayerSexComboBox->GetItem();
  char *first = newPlayerFirstNameTextBox->GetString();
  char *last = newPlayerLastNameTextBox->GetString();

  char badCh = 0;
  int validNick = IsValidNick(name,&badCh);

  int len = 0;
  if ( name )
    {
      len = strlen(name);
    }

  if ( !validNick )
    {
      char tmp[1024];
      sprintf(tmp,
	      "Stellar Frontier requires your player name to be a valid "
	      "IRC nickname.  Valid IRC nicknames can only be up to 30\n"
	      "characters long from the following list:\n\n"
              SF_CLIENT_NICKCHARS
              "\n\n"
              "Your playername is %d characters long\n"
              "Your playername has the following bad character \"%c\"",
	      len,badCh);
      S_MsgBox(button->parent,sf->profile->nexusName,tmp,S_MSGBOX_OK);
      button->SetEnabled(TRUE);
      return 0;
    }
  
  int rc = SF_NEXUS_OK;
  if ( nexusCheckBox->IsChecked() )
    {
      
      /* ****************************************************************** */
      /* *** If there is no passord given then prompt the user and exit *** */
      /* ****************************************************************** */
      if ( strlen(password) <= 0 )
	{
	  char tmp[1024];
	  sprintf(tmp,"To create a player on %s you need to specify a "
		  "password.",sf->profile->nexusName);
	  S_MsgBox(button->parent,sf->profile->nexusName,tmp,S_MSGBOX_OK);
	  button->SetEnabled(TRUE);
	  return 0;
	}
      /* ****************************************************************** */
      
      /* **************************************************************** */
      /* *** If there is no email given then prompt the user and exit *** */
      /* **************************************************************** */
      if ( strlen(email) <= 0 )
	{
	  char tmp[1024];
	  sprintf(tmp,"To create a player on %s you need to specify an "
		  "email address.",sf->profile->nexusName);
	  S_MsgBox(button->parent,sf->profile->nexusName,tmp,S_MSGBOX_OK);
	  button->SetEnabled(TRUE);
	  return 0;
	}
      /* **************************************************************** */
      
      /* ******************************************************************* */
      /* *** If retrieve check box is checked then make sure player name *** */
      /* *** already exists on nexus before trying to retrieve data.     *** */
      /* ******************************************************************* */
      if ( retrieveCheckBox->IsChecked() )
	{
	  rc = sf->PlayerExistsOnNexus(sf->profile->nexusName,name);
	  if ( !rc )
	    {
	      char tmp[1024];
	      sprintf(tmp,"You have specified that the player named %s "
		      "already exists on the nexus, however there is "
		      "no record for a player named %s.  Check to make "
		      "sure you spelled it correctly and keep in mind "
		      "that the nexus may be case sensitive.  If it still "
		      "doesn't work, just unselect that option to create "
		      "a new player.",
		      name,name);
	      S_MsgBox(button->parent,sf->profile->nexusName,tmp,S_MSGBOX_OK);
	      button->SetEnabled(TRUE);
	      return 0;
	    }
	}
      /* ******************************************************************* */
      
      rc = sf->CreateNewPlayerOnNexus(sf->profile->nexusName,name,password,
                                      email);
      if ( rc == SF_NEXUS_OK )
	{
	  S_Packet packet;
	  packet.PushString("FirstName");
	  packet.PushString(first);
	  packet.PushString("LastName");
	  packet.PushString(last);
	  packet.PushString("Age");
	  packet.PushString(age);
	  packet.PushString("Sex");
	  packet.PushString(sex);
	  rc = sf->SetRecordDataOnNexus(sf->profile->nexusName,
					name,password,
					&packet);
	  if ( rc )
	    {
	      S_String tmp("There was a problem uploading your character "
			   "data to the nexus.  rc = %d ",rc);
	      S_MsgBox(button->parent,
		       sf->profile->nexusName,
		       tmp.str,
		       S_MSGBOX_OK);
	      button->SetEnabled(TRUE);
	    }
	}
    }

  if ( rc == SF_NEXUS_OK )
    {
      SF_Client *c = (SF_Client *) button->parent->parent->puser;
      SF_Player *player = new SF_Player(name,password,ship,0,0,0,0,0,0,0,
					c->demo);
      if ( nexusCheckBox->IsChecked() )
	{
	  rc = sf->GetPlayerDataFromNexus(sf->profile->nexusName,player);
	}
      if ( playerListBox )
	{
	  for ( int i=0; i<c->profile->numStoredPlayers; i++ )
	    if ( strcmp(c->profile->players[i]->playerName.str,name) == 0 )
	      {
		c->profile->RemovePlayer(i);
		playerListBox->RemoveItem(i);
	      }
	  c->profile->AddPlayer(player);
	  int index = playerListBox->GetNumItems();
	  if ( playerListBox )
	    playerListBox->AddItem(player->playerName.str);
	  if ( playerListBox && index >= 0 )
	    {
	      playerListBox->SetIndex(index);
	      PlayerListBoxHandler(playerListBox);
	    }
	  EnableLoginDialog(c,0,0,TRUE);
	}
      if ( button && button->parent )
	{
	  button->parent->Close();
	}
    }
  else if ( rc == SF_NEXUS_NAMEALREADYEXISTS )
    {
      char tmp[1024];
      sprintf(tmp,"Sorry %s already exists on %s and uses a different "
	      "password than the one you gave.  Try another name or if "
	      "you are sure that this %s is your character, try another "
	      "password.",
	      newPlayerNameTextBox->GetString(),sf->profile->nexusName,
	      newPlayerNameTextBox->GetString());
      S_MsgBox(button->parent,sf->profile->nexusName,tmp,S_MSGBOX_OK);
      button->SetEnabled(TRUE);
    }
  else if ( rc == SF_NEXUS_CONNECTFAILED )
    {
      char tmp[1024];
      sprintf(tmp,"Sorry, I couldn't create a player profile on the nexus "
	      "at %s.\nIt appears to de down.",
	      sf->profile->nexusName);
      S_MsgBox(button->parent,sf->profile->nexusName,tmp,S_MSGBOX_OK);
      button->SetEnabled(TRUE);
    }
  else
    {
      button->SetEnabled(TRUE);
    }
  
  return 0;
}



int
NewPlayerOkButtonHandler(S_Button *button) {
   if ( button && button->parent ) {
      if ( newPlayerThread )
         delete newPlayerThread;
      newPlayerThread = new S_Thread(NewPlayerThread,(void *)button);
      }
   return 0;
   }


int
NewPlayerCancelButtonHandler(S_Button *button)
{
  if ( button &&  button->parent )
    {
      button->parent->Close();
    }
  return 0;
}


S_Button *newPlayerButton;
S_Dialog *loginDialog;
S_Dialog *newPlayerDialog;
int NewPlayerDialogHandler(S_Dialog *);

int
NewPlayerButtonHandler(S_Button *button) {
   newPlayerDialog = new S_Dialog(
                      sf->mainWindow,"New Player",
                      NewPlayerDialogHandler,0,
                      _root->cx/2-125,
                      _root->cy/2-234,
                      250,468);
   newPlayerDialog->selfDelete = TRUE;
   newPlayerDialog->Show();
   return 0;
   }


int
NexusCheckBoxHandler(S_CheckBox *cb) {
   if ( cb->IsChecked() ) {
      newPlayerPasswordTextBox->SetEnabled(TRUE);
      newPlayerEmailTextBox->SetEnabled(TRUE);
      newPlayerAgeTextBox->SetEnabled(TRUE);
      newPlayerFirstNameTextBox->SetEnabled(TRUE);
      newPlayerLastNameTextBox->SetEnabled(TRUE);
      newPlayerSexComboBox->SetEnabled(TRUE);
      retrieveCheckBox->SetEnabled(TRUE);
      }
   else {
      newPlayerPasswordTextBox->SetEnabled(FALSE);
      newPlayerEmailTextBox->SetEnabled(FALSE);
      newPlayerAgeTextBox->SetEnabled(FALSE);
      newPlayerFirstNameTextBox->SetEnabled(FALSE);
      newPlayerLastNameTextBox->SetEnabled(FALSE);
      newPlayerSexComboBox->SetEnabled(FALSE);
      retrieveCheckBox->SetCheck(FALSE);
      retrieveCheckBox->SetEnabled(FALSE);
      }
   return 0;
   }

int
RetrieveCheckBoxHandler(S_CheckBox *cb) {
   if ( cb->IsChecked() )
      nexusCheckBox->SetEnabled(TRUE);
   return 0;
   }

S_GroupBox *nexusGroupBox;
S_Button *newPlayerOkButton;
S_Button *newPlayerCancelButton;

int
NewPlayerDialogHandler(S_Dialog *dialog) {
   int xmargin = 10;
   int cxButton = 70;
   int cyButton = 35;
   int labelHeight = 16;
   int cyTextBox = 25;
   int cxTextBox = dialog->cxClient - 2*xmargin;
   int ymargin = 10;
   int x = xmargin;
   int y = ymargin;
   int cyGroupBox;

   switch(dialog->msg) {

      case S_WINDOW_CREATE :
         y += labelHeight;
         newPlayerNameTextBox = new S_TextBox(dialog,NULL,
                                              "My Name",
                                              x,y,cxTextBox,cyTextBox);
         newPlayerNameTextBox->AddLabel("Choose a name",cxTextBox,
                                        labelHeight);

         y += labelHeight + cyTextBox + ymargin;
         newPlayerShipTextBox = new S_TextBox(dialog,NULL,
                                              "My Ship",
                                              x,y,cxTextBox,cyTextBox);
         newPlayerShipTextBox->AddLabel("Choose a ship name",cxTextBox,
                                        labelHeight);

         y += cyTextBox + ymargin;
         cyGroupBox = dialog->cyClient - y - 2*ymargin - cyButton;
         nexusGroupBox = new S_GroupBox(dialog,"Stardock.net",
                                        x,y,cxTextBox,cyGroupBox);

         x += xmargin;
         y += 2*ymargin;
         cxTextBox -= 2*xmargin;
         nexusCheckBox = new S_CheckBox(dialog,NexusCheckBoxHandler,
                                        "Save player data on Stardock.net",
                                        x,y,cxTextBox-xmargin,20);
         nexusCheckBox->SetCheck(TRUE);

         y += 20;
         retrieveCheckBox = new S_CheckBox(dialog,RetrieveCheckBoxHandler,
                                           "Player already exists on nexus",
                                           x,y,cxTextBox-xmargin,20);

         y += 20 + ymargin + labelHeight;
         newPlayerPasswordTextBox = new S_TextBox(dialog,NULL,
                                              "",
                                              x,y,cxTextBox,cyTextBox,
                                              S_TEXTBOX_UNREADABLE);
         newPlayerPasswordTextBox->AddLabel("Choose a password",cxTextBox,
                                            labelHeight);

         y += cyTextBox + ymargin + labelHeight;
         newPlayerEmailTextBox = new S_TextBox(dialog,NULL,
                                               "",
                                               x,y,cxTextBox,cyTextBox);
         newPlayerEmailTextBox->AddLabel("Your e-mail address",cxTextBox,
                                            labelHeight);

         y += cyTextBox + ymargin + labelHeight;
         newPlayerFirstNameTextBox = new S_TextBox(dialog,NULL,
                                               "",
                                               x,y,cxTextBox/2-xmargin,
                                               cyTextBox);
         newPlayerFirstNameTextBox->AddLabel("First Name",cxTextBox/2-xmargin,
                                       labelHeight);

         newPlayerLastNameTextBox = new S_TextBox(dialog,NULL,
                                               "",
                                               x+cxTextBox/2+xmargin,y,
                                               cxTextBox/2-xmargin,
                                               cyTextBox);
         newPlayerLastNameTextBox->AddLabel("Last Name",cxTextBox/2-xmargin,
                                        labelHeight);


         y += cyTextBox + ymargin + labelHeight;
         newPlayerAgeTextBox = new S_TextBox(dialog,NULL,
                                               "",
                                               x,y,cxTextBox/2-xmargin,
                                               cyTextBox);
         newPlayerAgeTextBox->AddLabel("Your Age",cxTextBox/2-xmargin,
                                            labelHeight);

         newPlayerSexComboBox = new S_ComboBox(dialog,NULL,
                                               x+cxTextBox/2+xmargin,y,
                                               cxTextBox/2-xmargin,100);
	 if ( newPlayerSexComboBox )
	   {
	     newPlayerSexComboBox->AddItem("Neither");
	     newPlayerSexComboBox->AddItem("Male");
	     newPlayerSexComboBox->AddItem("Female");
	     newPlayerSexComboBox->AddItem("Please");
	     newPlayerSexComboBox->SetIndex(0);
	     newPlayerSexComboBox->AddLabel("Your Sex",cxTextBox/2-xmargin,
					    labelHeight);
	   }
         y = dialog->cyCanvas - ymargin - cyButton;
         newPlayerOkButton = new S_Button(dialog,
                                          NewPlayerOkButtonHandler,"Ok",
                                          dialog->cxCanvas - xmargin - 
                                          cxButton,y,
                                          cxButton,cyButton);
         newPlayerCancelButton = new S_Button(dialog,
                                              NewPlayerCancelButtonHandler,
                                              "Cancel",
                                              dialog->cxCanvas - 2*xmargin -
                                              2*cxButton,y,
                                              cxButton,cyButton);
         break;

      case S_WINDOW_CLOSE :
         if ( newPlayerNameTextBox ) {
            delete newPlayerNameTextBox;
            newPlayerNameTextBox = NULL;
	    }
         if ( newPlayerPasswordTextBox ) {
            delete newPlayerPasswordTextBox;
            newPlayerPasswordTextBox = NULL;
	    }
         if ( newPlayerEmailTextBox ) {
            delete newPlayerEmailTextBox;
            newPlayerEmailTextBox = NULL;
	    }
         if ( newPlayerAgeTextBox ) {
            delete newPlayerAgeTextBox;
            newPlayerAgeTextBox = NULL;
	    }
         if ( newPlayerFirstNameTextBox ) {
            delete newPlayerFirstNameTextBox;
            newPlayerFirstNameTextBox = NULL;
	    }
         if ( newPlayerLastNameTextBox ) {
            delete newPlayerLastNameTextBox;
            newPlayerLastNameTextBox = NULL;
	    }
         if ( newPlayerSexComboBox ) {
            delete newPlayerSexComboBox;
            newPlayerSexComboBox = NULL;
	    }
         if ( newPlayerShipTextBox ) {
            delete newPlayerShipTextBox;
            newPlayerShipTextBox = NULL;
	    }
         if ( newPlayerOkButton ) {
            delete newPlayerOkButton;
            newPlayerOkButton = NULL;
	    }
         if ( newPlayerCancelButton ) {
            delete newPlayerCancelButton;
            newPlayerCancelButton = NULL;
	    }
         if ( nexusCheckBox ) {
            delete nexusCheckBox;
            nexusCheckBox = NULL;
	    }
         if ( retrieveCheckBox ) {
            delete retrieveCheckBox;
            retrieveCheckBox = NULL;
	    }
         sf->mouseMenuEnabled = TRUE;
         break;
      }

   return 0;
   }

S_Thread *sendPlayerInfoThread = NULL;


int
SendPlayerInfo(void *iarg=NULL) {

   SF_Engine *e = sf->engine;
   if ( e && sf->profile->selectedPlayer >= 0 ) {

      SF_Player *player = sf->profile->players[sf->profile->selectedPlayer];

      player->demo = sf->demo;
      player->serial = 0;
      if ( sf && sf->profile && sf->profile->reg1 )
	{
	  player->serial = atoi(sf->profile->reg1);
	}

      //
      // Prepend a ~ to non-nexus connected players to prevent spoofing.
      //
      if ( !sf->connectedThroughNexus )
	{
	  if ( player->playerName.str[0] != '~' )
	    {
	      player->playerName.Prepend("~");
	    }
	}

      if ( sf->msgBox && sf->connectedThroughNexus ) {
         char tmp[1024];
         sprintf(tmp,"Reading data for %s . . .\n",player->playerName.str);
         sf->msgBox->Show(sf->profile->nexusName,tmp);
         }

      int rc = 0;
      if ( sf->connectedThroughNexus ) {

         do {
            rc = sf->GetPlayerDataFromNexus(sf->profile->nexusName,player);

	    int len = strlen(player->playerName.str);

            if ( rc == SF_NEXUS_WRONGPASSWORD || 
                 rc == SF_NEXUS_NAMEDOESNOTEXIST ||
                 player->playerName.str[0] == ' ' ||
		 player->playerName.str[len-1] == ' ') {

               S_MsgBox msg(sf->mainWindow,"SF Nexus",
                            "Houston, we have a problem.\n\n"
                            "It could be one of these three things:\n"
                            "  1) You gave a wrong password\n"
                            "  2) Your player does not exist\n"
                            "  3) You have a leading space in your name\n"
                            "  3) You have a trailing space in your name\n"
                            "  4) You are an using an offline character\n\n"
                            "This error box is a result of spoof detection\n"
                            "code.  If you were not spoofing, then contact\n"
                            "Stardock.\n\n"
			    /*
			    "For now, your name will temporarily be changed\n"
                            "to Imposter"
			    */
                            ,
                            S_MSGBOX_OK);
               sf->connectedThroughNexus = FALSE;
               sf->engine->selfTermination = TRUE;
               sf->engine->run = FALSE;
               rc = 0;
	       }

            if ( rc )
	      {
		S_String msg("I could not download your stats from the "
                             "nexus.\nIt may be that the nexus is down or "
                             "was very busy.\nThe return code was %d\n\n"
			     "Should I try again?\n\nIf you answer no "
			     "your score won't be saved when\nyou exit the "
			     "server.",rc);
		S_MsgBox msgBox(sf->mainWindow,"SF Nexus",
				msg.str,S_MSGBOX_YESNO);
		if ( msgBox.rc == S_MSGBOX_NO)
		  {
		    sf->connectedThroughNexus = FALSE;
		    rc = 0;
		  }
	      }

            } while ( rc );
         }

      if ( !rc && sf->msgBox && sf->serverIndex >= 0 )
	{
          S_String tmp("Logging into %s:%d . . .",
		       sf->serverList[sf->serverIndex].hostName,
		       sf->serverList[sf->serverIndex].port);
          if ( strlen(sf->serverList[sf->serverIndex].serverName) > 0 )
	    {
	      sf->msgBox->Show(sf->serverList[sf->serverIndex].serverName,
                               tmp.str);
	    }
	  else
	    {
	      sf->msgBox->Show(sf->profile->nexusName,tmp.str);
	    }
	}

      if ( rc && sf->msgBox )
	{
	  sf->msgBox->Hide();
	}

      SF_GameEngine *ge = e->gameEngine;
      SF_NetEngine *ne = e->netEngine;
      if ( ge && e->run && e->state == SF_CLIENT_SINGLEPLAYER ) {
         ge->msgBuffer->mutex.Request();
         ge->msgBuffer->PushMsg((uchar)SF_MSG_PLAYERINFO,
                                (ushort)ge->localSlot,
                                 player,
                                (int)S_CLIENT_TCP);
         ge->msgBuffer->mutex.Release();
         }
      if ( ne && e->run && e->state == SF_CLIENT_CONNECTED ) {
         ne->slot->writeBuffer.mutex.Request();
	 mstate = 11;
         ne->slot->writeBuffer.PushMsg((uchar)SF_MSG_PLAYERINFO,
                                       (ushort)ge->localSlot,
                                        player,
                                       (int)S_CLIENT_TCP);
	 mstate = 0;
         ne->slot->writeBuffer.mutex.Release();
         }
      }
   if ( sendPlayerInfoThread ) {
      delete sendPlayerInfoThread;
      sendPlayerInfoThread = NULL;
      }
   return 0;
   }

S_ComboBox *raceComboBox = NULL;
S_ComboBox *shipComboBox = NULL;

int
ShipComboBoxHandler(S_ComboBox *comboBox) {
   SF_Client *c = (SF_Client *) comboBox->parent->parent->puser;
   if ( c && shipComboBox && raceComboBox && game && 
        c->profile->selectedPlayer >= 0 ) {
      int sindex = shipComboBox->GetIndex();
      int rindex = raceComboBox->GetIndex();
      char *shipName = shipComboBox->GetItem();
      if ( !shipName ) return 0;
      if ( rindex < 0 )
	{
	  rindex = 0;
	}
      if ( sindex < 0 )
	{
	  sindex = 0;
	}
      SF_Player *player = c->profile->players[c->profile->selectedPlayer];
      player->demo = sf->demo;
      for ( int i=0; i<game->numShipDesigns; i++ )
         if ( game->shipDesign[i].raceID == player->race &&
              strcmp(shipName,game->shipDesign[i].name) == 0 )
         c->profile->players[c->profile->selectedPlayer]->shipID = 
            game->shipDesign[i].id;
      EnableLoginDialog(c,-1,-1,-1);
      }
   return 0;
   }


int
LoginOkButtonHandler(S_Button *button) {
   if ( button && button->parent && button->parent->parent ) {
      SF_Client *c = (SF_Client *) button->parent->parent->puser;
      SF_Engine *e = c->engine;
      if ( !e ) return 0;
      SF_GameEngine *ge = e->gameEngine;
      if ( !ge ) return 0;
      SF_DisplayEngine *de = e->displayEngine;
      if ( !de ) return 0;
      S_Window *w = c->mainWindow;
      if ( !w ) return 0;

      button->parent->Hide();
      w->DrawFilledRect(0,0,w->cxCanvas,w->cyCanvas,0);
      c->backBuffer->DrawFilledRect(0,0,w->cxCanvas,w->cyCanvas,0);

      ShipComboBoxHandler(shipComboBox);

      sendPlayerInfoThread = new S_Thread(SendPlayerInfo);

      if ( game && de && de->chat && de->chat->channels ) {
	char tmp[1024];
	sprintf(tmp,"Welcome to %s\n",game->scenarioName);
	SF_ChatChannel *channel = NULL;
	channel = &de->chat->channels[SF_CHANNEL_BROADCAST];
	if ( channel )
	  {
	    channel->AppendLine(CleanString(tmp));
	    channel->AppendLine(" ");
	    channel->AppendLine(CleanString(game->scenarioDesc));
	    channel->AppendLine(" ");
	  }
	channel = &de->chat->channels[SF_CHANNEL_TEAM];
	if ( channel )
	  {
	    channel->AppendLine("Welcome to Team Chat");
	    channel->AppendLine(" ");
	    channel->AppendLine("Messages posted here will only appear to "
				"members of your race");
	    channel->AppendLine(" ");
	  }
      }

      button->parent->Close();
      }
   return 0;
   }


int
CalculateRank(SF_Player *player)
{
  if ( game && player )
    {
      PlayerClass gplayer(player->playerName.str,
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
      player->rank = (signed char) gplayer.CalculateRank(localSlotNumber,1);
      return gplayer.rank;
    }
  return 0;
}


S_TextBox *playerNameTextBox;
S_TextBox *shipNameTextBox;


int
PlayerListBoxHandler(S_ListBox *listBox) {
   SF_Client *c = (SF_Client *) listBox->parent->parent->puser;
   int index = listBox->GetIndex();
   if ( index >= 0 && shipNameTextBox && playerNameTextBox &&
        raceComboBox && shipComboBox ) {
      c->profile->selectedPlayer = index;
      SF_Player *player = c->profile->players[index];
      playerNameTextBox->SetText(player->playerName.str);
      shipNameTextBox->SetText(player->shipName.str);
      CalculateRank(player);

      if ( game && shipComboBox )
	{
	  shipComboBox->Flush();
	  for ( int i=0; i<game->numShipDesigns; i++ )
	    {
              int bi = game->shipDesign[i].bitmapIndex;
              int acOnly = game->objectBitmapList[bi].acOnly;
	      if ( game->shipDesign[i].raceID == player->race &&
		   player->rank >= game->shipDesign[i].reqRank &&
		   !(acOnly && sf->demo))
		{
		  if ( shipComboBox )
		    shipComboBox->AddItem(game->shipDesign[i].name);
		}
	      if ( shipComboBox )
		shipComboBox->SetIndex(0);
	    }
	}

      //playerNameTextBox->SetEnabled(TRUE);
      shipNameTextBox->SetEnabled(TRUE);
      raceComboBox->SetEnabled(TRUE);
      shipComboBox->SetEnabled(TRUE);
      }
   return 0;
   }


int
RemovePlayerButtonHandler(S_Button *button)
{
  SF_Client *c = (SF_Client *) playerListBox->parent->parent->puser;
  int index = playerListBox->GetIndex();
  c->profile->selectedPlayer = 0;
  if ( index >= 0 )
    {
      c->profile->RemovePlayer(index);
      playerListBox->RemoveItem(index);
    }
  playerListBox->SetIndex(0);
  if ( c->profile->numStoredPlayers == 0 )
    {
      c->profile->selectedPlayer = -1;
      EnableLoginDialog(c,-1,-1,FALSE);
    }
  PlayerListBoxHandler(playerListBox);
  return 0;
}


int
PlayerNameTextBoxHandler(S_TextBox *textBox) {
   SF_Client *c = (SF_Client *) textBox->parent->parent->puser;
   char *newName = textBox->GetString();
   if ( strlen(newName) > 0 && c->profile->selectedPlayer >= 0 )
      c->profile->players[c->profile->selectedPlayer]->SetPlayerName(newName);
   return 0;
   }


int
ShipNameTextBoxHandler(S_TextBox *textBox) {
   SF_Client *c = (SF_Client *) textBox->parent->parent->puser;
   char *newName = textBox->GetString();
   if ( strlen(newName) > 0 && c->profile->selectedPlayer >= 0 )
      c->profile->players[c->profile->selectedPlayer]->SetShipName(newName);
   return 0;
   }


int
CloseLoginDialog(SF_Client *c) {

   if ( !c->mainWindow ) {
      if ( c->engine->rc == SF_ENGINE_CONNECTFAILED )
         printf("Failed to Connect -- Connect Failed.  The server was not "
                "accepting connections.  It is probably down.");

      if ( c->engine->rc == SF_ENGINE_SERVERFULL )
         printf("Connect Failed -- The server is full.");

      if ( c->engine->rc == SF_ENGINE_WRONGVERSION )
         printf("Connect Failed -- Wrong version.  The server is running an "
                "older version of Stellar Frontier and won't let you play.");

      if ( c->engine->rc == SF_ENGINE_OLDVERSION )
         printf("Connect Failed -- Old version.  You are running an outdated "
                "version of Stellar Frontier.  Go to www.stardock.com to "
                "upgrade.");

      c->engine->rc = SF_ENGINE_OK;
      return 0;
      }

   if ( loginDialog )
     {
       loginDialog->Close(TRUE);
     }

   return 0;
   }

S_Button *loginOkButton;


int
EnableLoginDialog(SF_Client *c,int rindex, int sindex,int flag) {

  if ( !c->mainWindow ) {
     if ( c->profile->selectedPlayer >= 0 ) {
        c->profile->players[c->profile->selectedPlayer]->race = 0;
        c->profile->players[c->profile->selectedPlayer]->shipID = 0;
        }
     SendPlayerInfo();
     return 0;
     }

   int i,j;

   int pindex = c->profile->selectedPlayer;
   int enable = pindex >= 0;
   if ( playerNameTextBox && pindex >= 0 ) {
     //playerNameTextBox->SetEnabled(enable);
      playerNameTextBox->SetText(c->profile->players[pindex]->playerName.str);
      }
   if ( shipNameTextBox ) {
      shipNameTextBox->SetEnabled(enable);
      if ( enable )
         shipNameTextBox->SetText(c->profile->players[pindex]->shipName.str);
      else
         shipNameTextBox->SetText("");
      }
   if ( shipComboBox ) {
      shipComboBox->SetEnabled(enable);
      }
   if ( raceComboBox ) {
      raceComboBox->SetEnabled(enable);
      }

   if ( flag >= 0 ) {

      if ( game && playerListBox ) {
         playerListBox->Flush();
         for ( i=0; i<c->profile->numStoredPlayers; i++ ) {
           char tmp[1024];
	   if ( c->profile->players[i]->rank > game->numRanks )
	     {
	       c->profile->players[i]->rank = game->numRanks -1 ;
	     }
	   if ( c->profile->players[i]->password.str &&
                strlen(c->profile->players[i]->password.str)>0 )
	     {
	       sprintf(tmp,"%s %s [%ld kills] [%ld Deaths]",
		       game->rank[c->profile->players[i]->rank].name,
		       c->profile->players[i]->playerName.str,
		       c->profile->players[i]->kills,
		       c->profile->players[i]->losses);
	     }
	   else
	     {
	       sprintf(tmp,"%s %s [%ld kills] [%ld Deaths] (Offline)",
		       game->rank[c->profile->players[i]->rank].name,
		       c->profile->players[i]->playerName.str,
		       c->profile->players[i]->kills,
		       c->profile->players[i]->losses);
	     }
	   if ( playerListBox )
	     playerListBox->AddItem(tmp);
            }
         if ( c->profile->selectedPlayer < 0 ||
              c->profile->selectedPlayer >= c->profile->numStoredPlayers ) {
            playerListBox->SetIndex(0);
            PlayerListBoxHandler(playerListBox);
	    }
         else {
            playerListBox->SetIndex(c->profile->selectedPlayer);
            PlayerListBoxHandler(playerListBox);
	    }
         }

      if ( loginOkButton )
	{
	  if ( flag && c->profile->selectedPlayer >= 0 )
	    loginOkButton->SetEnabled(TRUE);
	  else
	    loginOkButton->SetEnabled(FALSE);
	}

      }

   if ( rindex >= 0 && raceComboBox ) {
      raceComboBox->Flush();
      for ( i=0; i<game->numRaces; i++ ) {
         int nPlayers = 0;
         for (j=0; j<game->maxHumanShips; j++)
            if (game->objectList[j]->raceID == i && game->ship[j].IsActive())
               nPlayers++;
         char *tmp = (char *)S_malloc(strlen(game->race[i].name)+
                                    strlen(" (12345 players)")+1);
         sprintf(tmp,"%s (%d players)",game->race[i].name,nPlayers);
	 if ( raceComboBox )
	   raceComboBox->AddItem(tmp);
         S_free(tmp);
         }
      if ( c->profile->selectedPlayer >= 0 )
         c->profile->players[c->profile->selectedPlayer]->race = rindex;
      raceComboBox->SetIndex(rindex);
      }

   if ( sindex >= 0 && c->profile->selectedPlayer >= 0 && shipComboBox && game ) {
      shipComboBox->Flush();
      for ( i=0; i<game->numShipDesigns; i++ )
	{
         int bi = game->shipDesign[i].bitmapIndex;
         int acOnly = game->objectBitmapList[bi].acOnly;
         if ( game->shipDesign[i].raceID == 
              c->profile->players[c->profile->selectedPlayer]->race &&
              c->profile->players[c->profile->selectedPlayer]->rank >= 
              game->shipDesign[i].reqRank &&
              !(acOnly && sf->demo) )
	   {
	     if ( shipComboBox )
	       shipComboBox->AddItem(game->shipDesign[i].name);
	   }
	 if ( shipComboBox )
	   shipComboBox->SetIndex(sindex);
	}
      }

   return 0;
   }

S_Button *loginCancelButton = NULL;


int
EnableLoginDialogButtons(int enable) {
   if ( loginCancelButton )
      loginCancelButton->SetEnabled(enable);
   if ( loginOkButton )
      loginOkButton->SetEnabled(enable);
   return 0;
   }


int
RaceComboBoxHandler(S_ComboBox *comboBox) {
   SF_Client *c = (SF_Client *) comboBox->parent->parent->puser;
   int rindex = raceComboBox->GetIndex();
   int sindex = shipComboBox->GetIndex();
   if ( rindex < 0 )
     {
       rindex = 0;
     }
   if ( sindex < 0 )
     {
       sindex = 0;
     }
   if ( c->profile->selectedPlayer >= 0 ) {
      c->profile->players[c->profile->selectedPlayer]->race = rindex;
      EnableLoginDialog(c,-1,0,-1);
      ShipComboBoxHandler(shipComboBox);
      }
   return 0;
   }


int
LoginCancelButtonHandler(S_Button *button) {
   if ( button &&  button->parent ) {
      SF_Client *client = (SF_Client *) button->parent->parent->puser;

      if ( client->engine )
	{
	  if ( client->engine->state == SF_CLIENT_CONNECTING )
	    {
	      client->engine->stopFast = TRUE;
	      client->engine->client->stopConnect = TRUE;
	      client->engine->engineThread->WaitFor();
	      delete client->engine;
	      client->engine = NULL;
	    }
	  else
	    {
	      client->StopEngine();
	    }
	}

      client->rcEngine = 0;

      button->parent->Close();
      }
   sf->mouseMenuEnabled = TRUE;
   return 0;
   }

char *ListBoxHelpText = 
"Select a player from the list below or create a new player by pressing "
"the 'New Player' button.";

char *_PlayerHelpText = 
"Below, you can change your name, your ship's name, your race and your ship "
"class.";

int LoginDialogHandler(S_Dialog *);


int
LaunchLoginDialog(S_Window *window) {
   if ( window ) {
      loginDialog = new S_Dialog(window,"Login",LoginDialogHandler,0,
                                 window->root->cx/2-240,
                                 window->root->cy/2-180,480,360);
      loginDialog->selfDelete = TRUE;
      loginDialog->Show();

      if ( sf->profile->numStoredPlayers <= 0 )
         NewPlayerButtonHandler(newPlayerButton);

      }
   return 0;
   }

S_StaticText *loginStatusText;


int
SetLoginStatusText(char *text) {
   if ( loginStatusText )
      loginStatusText->SetText(text);
   return 0;
   }

int lastVal;

int
DrawLoginStatusBar(int val) {
   if ( !loginDialog ) return 0;
   if ( val < 0 )
      val = lastVal;
   lastVal = val;
   int margin = 10;
   int cx = 3*loginDialog->cxClient/5;
   int x1 = margin;
   int x2 = x1 + val*cx/100;
   int y1 = loginDialog->cyClient - 2*margin + 1;
   int y2 = y1 + margin - 2;
   loginDialog->DrawFilledRect(x1+1,y1+1,x2-1,y2-1,RGB_DARKBLUE);
   loginDialog->DrawFilledRect(x2+1,y1+1,x1+cx-1,y2-1,
                               loginDialog->backGroundRGB);
   loginDialog->DrawLine(x1,y1,x1+cx,y1,1,RGB_DARKGRAY);
   loginDialog->DrawLine(x1,y1,x1,y2,1,RGB_DARKGRAY);
   loginDialog->DrawLine(x1,y2,x1+cx,y2,1,RGB_WHITE);
   loginDialog->DrawLine(x1+cx,y1,x1+cx,y2,1,RGB_WHITE);
   return 0;
   }


S_StaticText *loginHelpText;
S_Button *removePlayerButton;
S_StaticText *playerHelpText;


int
LoginDialogHandler(S_Dialog *dialog) {

   int i;
   int margin = 10;
   int labelHeight = 16;
   int cxButton = 70;
   int cyButton = 35;
   int staticTextHeight = 2*cyButton+margin-labelHeight;
   int listBoxWidth = 3*dialog->cxClient/5;
   int listBoxHeight = dialog->cyClient - 4*margin - 
                       staticTextHeight - cyButton - labelHeight;
   int staticTextWidth = listBoxWidth - cxButton - margin - 20;
   int cxTextBox = dialog->cxClient - listBoxWidth - 4*margin;
   int cyTextBox = 25;

   int x = margin,y = margin;

   SF_Client *client = (SF_Client *) dialog->parent->puser;
   /*
   SF_Player *player = NULL;
   if ( client->profile->selectedPlayer > = 0 )
      client->profile->players[client->profile->selectedPlayer];
      */

   switch(dialog->msg) {
      case S_WINDOW_CREATE :

         loginHelpText = new S_StaticText(dialog,ListBoxHelpText,x,y,
                                          staticTextWidth,staticTextHeight);

         newPlayerButton = new S_Button(dialog,NewPlayerButtonHandler,
                                        "New Player",
                                        x+staticTextWidth+margin,y,
                                        cxButton+20,cyButton);

         removePlayerButton = new S_Button(dialog,RemovePlayerButtonHandler,
                                           "Kill Player",
                                           x+staticTextWidth+margin,
                                           y+cyButton+margin,
                                           cxButton+20,cyButton);

         y += staticTextHeight + margin + labelHeight;
         playerListBox = new S_ListBox(dialog,PlayerListBoxHandler,
                                       x,y,listBoxWidth,listBoxHeight);
         playerListBox->AddLabel("Player List",listBoxWidth/2,labelHeight);

         loginStatusText = new S_StaticText(dialog,
                                           "Loading graphics and sounds . . .",
                                           x,
                                           dialog->cyClient-2*margin -
                                           labelHeight,
                                           listBoxWidth,labelHeight);

         x = dialog->cxClient - cxButton - margin;
         y += listBoxHeight + margin;
         loginOkButton = new S_Button(dialog,LoginOkButtonHandler,"Ok",
                                      x,y,cxButton,cyButton);

         EnableLoginDialog(client,-1,-1,FALSE);

         x -= cxButton + margin;
         loginCancelButton = new S_Button(dialog,LoginCancelButtonHandler,
                                          "Cancel",
                                          x,y,
                                          cxButton,cyButton);

         x = 3*margin + listBoxWidth;
         y = margin;
         playerHelpText = new S_StaticText(dialog,_PlayerHelpText,x,y,
                                           cxTextBox,staticTextHeight);

         y += margin + staticTextHeight + labelHeight;
         playerNameTextBox = new S_TextBox(dialog,PlayerNameTextBoxHandler,
                                           NULL,
                                           x,y,cxTextBox,cyTextBox);
         playerNameTextBox->AddLabel("Player Name",cxTextBox,labelHeight);
         playerNameTextBox->SetEnabled(FALSE);

         y += margin + cyTextBox + labelHeight;
         shipNameTextBox = new S_TextBox(dialog,ShipNameTextBoxHandler,
                                         NULL,
                                         x,y,cxTextBox,cyTextBox);
         shipNameTextBox->AddLabel("Ship Name",cxTextBox,labelHeight);
         shipNameTextBox->SetEnabled(FALSE);

         y += margin + cyTextBox + labelHeight;
         raceComboBox = new S_ComboBox(dialog,RaceComboBoxHandler,
                                       x,y,cxTextBox,100);
         raceComboBox->AddLabel("Player Race",cxTextBox,labelHeight);
         raceComboBox->SetEnabled(FALSE);

         y += margin + cyTextBox + labelHeight;
         shipComboBox = new S_ComboBox(dialog,ShipComboBoxHandler,
                                       x,y,cxTextBox,200);
         shipComboBox->AddLabel("Ship Class",cxTextBox,labelHeight);
         shipComboBox->SetEnabled(FALSE);

         playerListBox->Flush();
         for ( i=0; i<client->profile->numStoredPlayers; i++ ) {
	   char tmp[1024];
	   if ( client->profile->players[i]->password.str &&
                strlen(client->profile->players[i]->password.str)>0 )
	     {
	       sprintf(tmp,"%s [%ld kills] [%ld Deaths]",
		       client->profile->players[i]->playerName.str,
		       client->profile->players[i]->kills,
		       client->profile->players[i]->losses);
	     }
	   else
             {
	       sprintf(tmp,"%s [%ld kills] [%ld Deaths] (Offline)",
		       client->profile->players[i]->playerName.str,
		       client->profile->players[i]->kills,
		       client->profile->players[i]->losses);
             }
	   if ( playerListBox )
	     playerListBox->AddItem(tmp);
	 }

         if ( client->profile->selectedPlayer < 0 &&
              client->profile->numStoredPlayers > 0 )
            client->profile->selectedPlayer = 0;

         if ( client->profile->selectedPlayer >= 0 &&
              client->profile->selectedPlayer < 
              client->profile->numStoredPlayers ) {
            playerListBox->SetIndex(client->profile->selectedPlayer);
            PlayerListBoxHandler(playerListBox);
	    }

         break;

      case S_WINDOW_PAINT :
         DrawLoginStatusBar(-1);
         break;

   case S_WINDOW_CLOSE :
     if ( client )
       {
	 if ( client->rcEngine == SF_ENGINE_CONNECTFAILED )
	   {
	     S_MsgBox(client->mainWindow,
		      "Failed to Connect",
		      "Connect Failed",
		      S_MSGBOX_OK);
	     client->rcEngine = 0;
	   }
	 if ( client->rcEngine == SF_ENGINE_SERVERFULL )
	   {
	     S_MsgBox(client->mainWindow,
		      "Connect Failed",
		      "The server is full",
		      S_MSGBOX_OK);
	     client->rcEngine = 0;
	   }	 
	 if ( client->rcEngine == SF_ENGINE_WRONGVERSION )
	   {
	     S_MsgBox(client->mainWindow,
		      "Connect Failed",
		      "Wrong version",
		      S_MSGBOX_OK);
	     client->rcEngine = 0;
	   }	 
	 if ( client->rcEngine == SF_ENGINE_OLDVERSION )
	   {
	     char tmp[1024];
	     sprintf(tmp,
		     "You are running an outdated version of Stellar "
		     "Frontier.  You can either go to www.stardock.com "
		     "to download the latest version or automatically "
		     "upgrade from the files from a server on the nexus.");
	     S_MsgBox msg(client->mainWindow,
			  "Stellar Frontier: Old Version",
			  tmp,
			  S_MSGBOX_OK);
	     client->rcEngine = 0;
	   }
	 
	 if ( client->rcEngine != SF_ENGINE_OK && client->engine )
	   {
	     client->engine->selfTermination = TRUE;
	     client->engine->run = FALSE;
	     client->rcEngine = 0;
	   }
       }
     
     sf->mouseMenuEnabled = TRUE;
     
         if ( loginStatusText) {
            delete loginStatusText;
            loginStatusText = NULL;
	    }
         if ( loginHelpText ) {
            delete loginHelpText;
            loginHelpText = NULL;
	    }
         if ( newPlayerButton ) {
            delete newPlayerButton;
            newPlayerButton = NULL;
	    }
         if ( removePlayerButton ) {
            delete removePlayerButton;
	    removePlayerButton = NULL;
	    }
         if ( playerListBox ) {
            delete playerListBox;
            playerListBox = NULL;
	    }
         if ( loginOkButton ) {
            delete loginOkButton;
            loginOkButton = NULL;
	    }
         if ( loginCancelButton ) {
            delete loginCancelButton;
            loginCancelButton = NULL;
	    }
         if ( playerHelpText ) {
            delete playerHelpText;
            playerHelpText = NULL;
	    }
         if ( playerNameTextBox ) {
            delete playerNameTextBox;
            playerNameTextBox = NULL;
	    }
         if ( shipNameTextBox ) {
            delete shipNameTextBox;
            shipNameTextBox = NULL;
	    }
         if ( raceComboBox ) {
            delete raceComboBox;
            raceComboBox = NULL;
	    }
         if ( shipComboBox ) {
            delete shipComboBox;
            shipComboBox = NULL;
	    }

         loginDialog = NULL;

         sf->mouseMenuEnabled = TRUE;
         break;

      }
   return 0;
   }
/* ************************************************************************* */
/* ************************* Player login Dialog *************************** */
/* ************************************************************************* */


/* ************************************************************************* */
/* ***************************** Internet Dialog *************************** */
/* ************************************************************************* */
int 
SF_Client::GetInternetServerList(char *nexusName) {

   slong numServers = -1;
   char tmp[1024];
   S_Client *c = new S_Client();
   c->SetTimeOut(SF_CLIENT_TIMEOUT);

   if ( nexusStatusText ) {
      sprintf(tmp,"Contacting nexus at %s . . .",nexusName);
      nexusStatusText->SetText(tmp);
      }

   if ( c &&
        c->Connect(nexusName,SF_NEXUS_PORT,SF_NEXUS_SERVERLIST)==S_CLIENT_OK ){

      if ( nexusStatusText ) {
         sprintf(tmp,"Reading server data from %s . . .",nexusName);
         nexusStatusText->SetText(tmp);
         }

      S_Packet packet;
      c->ReadPacket(&packet);
      c->Disconnect();

      ulong response = packet.PopULong();
      numServers = packet.PopULong();

      if ( numServers > MAXSERVERS )
	{
	  numServers = MAXSERVERS;
	}

      if ( packet.readComplete && response == S_CLIENT_OK && numServers > 0 ) {
         int i;
         for ( i=0; i<(int)numServers; i++ ) {
            char *serverString = packet.PopString();
            if ( serverString ) {
               strcpy(serverList[i].serverString,serverString);
               S_TokenArray tokens(serverString,':',':',
				   S_TOKENARRAY_SINGLEDELIMIT);
               if ( tokens.numLines >= 1 && tokens.lines[0] )
		 {
                   strcpy(serverList[i].hostName,tokens.lines[0]);
		 }
               if ( tokens.numLines >= 7 )
		 {
                   serverList[i].port = atoi(tokens.lines[1]);
                   strcpy(serverList[i].serverName,tokens.lines[2]);
                   strcpy(serverList[i].versionString,tokens.lines[3]);
                   serverList[i].numPlayers = atoi(tokens.lines[4]);
                   serverList[i].maxPlayers = atoi(tokens.lines[5]);
                   strcpy(serverList[i].osString,tokens.lines[6]);

		   serverList[i].numHumanPlayers = 0;
		   if ( tokens.numLines >= 8 )
		     {
		       serverList[i].numHumanPlayers = atoi(tokens.lines[7]);
		     }

                   strcpy(serverList[i].playerList,"");
		   if ( tokens.numLines >= 9 )
		     {
		       strcpy(serverList[i].playerList,tokens.lines[8]);
		     }

                   strcpy(serverList[i].upTimeStr,"");
		   if ( tokens.numLines >= 10 )
		     {
		       strcpy(serverList[i].upTimeStr,tokens.lines[9]);
		       for ( uint j=0; j<strlen(serverList[i].upTimeStr); j++ )
			 {
			   if ( serverList[i].upTimeStr[j] == '|' )
			     {
			       serverList[i].upTimeStr[j] = ':';
			     }
			 }
		     }
		 }
               else
		 {
                   serverList[i].port = SF_SERVER_PORT;
                   strcpy(serverList[i].serverName,"");
                   strcpy(serverList[i].versionString,"");
                   serverList[i].numPlayers = 0;
                   serverList[i].maxPlayers = 0;
                   strcpy(serverList[i].osString,"");
                   serverList[i].numHumanPlayers = 0;
                   strcpy(serverList[i].playerList,"");
                   strcpy(serverList[i].upTimeStr,"");
		 }
               sf->serverPort = serverList[i].port;
               S_free(serverString);
	       }
            }
         serverList[i].serverString[0] = 0;
         serverList[i].hostName[0] = 0;
         serverList[i].serverName[0] = 0;
         serverList[i].versionModifier[0] = 0;
         serverList[i].versionString[0] = 0;
         serverList[i].osString[0] = 0;
         serverList[i].playerList[0] = 0;
         serverList[i].upTimeStr[0] = 0;
         }

      delete c;
      c = NULL;

      if ( nexusStatusText )
         nexusStatusText->SetText(" ");

      return( numServers );
      }

   return( numServers );
   }


int
StardockNetMenuItemHandler(S_MenuItem *mi) {
   sf->mouseMenuEnabled = FALSE;
   stardockNetDialog = new SF_NexusClient(sf->mainWindow,
                                          sf->profile->nexusName);
   return 0;
   }


int
CancelButtonHandler(S_Button *button) {
   if ( button && button->parent )
      button->parent->Close();
   return 0;
   }


S_Button *internetToggleButton = NULL;
S_TextEdit *newsTextEdit = NULL;
S_ListBox *playerStatsListBox;
S_ListBox *serverListListBox;



int
InternetOkButtonHandler(S_Button *button) {
   if ( button && button->parent ) {
      SF_Client *client = (SF_Client *)button->parent->parent->puser;
      client->serverIndex = serverListListBox->GetIndex();
      if ( client->serverIndex >= 0 ) {
         button->parent->Hide();
	 S_String hostname("%s:%d",
			   client->serverList[client->serverIndex].hostName,
			   client->serverList[client->serverIndex].port);
         client->StartEngine(hostname.str);
         button->parent->Close();
         client->connectedThroughNexus = TRUE;
         }
      sf->mouseMenuEnabled = TRUE;
      }
   return 0;
   }



int
InternetToggleButtonHandler(S_Button *button)
  {
   if ( playerStatsListBox && newsTextEdit ) {
      if ( playerStatsListBox->IsVisible() )
	{
          playerStatsListBox->Hide();
          newsTextEdit->Show();
          button->SetText("Show Aces");
	}
      else
        {
          playerStatsListBox->Show();
          newsTextEdit->Hide();
          button->SetText("Show News");
        }
      }
   return 0;
   }


int
ServerListListBoxHandler(S_ListBox *listBox) {
   return 0;
   }

S_StaticText *nexusUpgradeReminder;
S_Button *nexusUpgradeButton;
S_Button *nexusRefreshButton;
S_TextBox *nexusNameTextBox;

#define SF_CLIENT_NEXUS_REFRESHSERVERS     1
#define SF_CLIENT_NEXUS_REFRESHLEADERBOARD 2
#define SF_CLIENT_NEXUS_UPGRADE            4

int nexusRefresh = TRUE;
int refreshContinue;
S_Event refreshEvent;



int
_NexusRefreshThread(void *arg) {

   SF_Client *client = sf;

   int leaderBoardUpdated = FALSE;
   int newsUpdated = FALSE;
   refreshEvent.Reset();
   refreshContinue = TRUE;
   while ( refreshContinue ) {

      int i = 0;
      char text[1024];
      int numServers = -1;

      char *nexusName = SF_NEXUS_DEFAULT;
      if ( nexusNameTextBox )
         nexusName = nexusNameTextBox->GetString();

      if ( nexusRefreshButton )
         nexusRefreshButton->SetEnabled(FALSE);
      if ( nexusUpgradeButton )
         nexusUpgradeButton->SetEnabled(FALSE);

      /* ******************************************************************* */
      /* ************************* Get nexus version *********************** */
      /* ******************************************************************* */
      if ( sf->nexusMajorVersion == 0 && sf->nexusMinorVersion == 0 &&
           sf->nexusMinorMinorVersion == 0 ) {
         S_Client c;
         c.SetTimeOut(SF_CLIENT_TIMEOUT);
         int rc = c.Connect(nexusName,SF_NEXUS_PORT,SF_NEXUS_GETVERSION);
         if ( rc == SF_NEXUS_OK ) {
            S_Packet packet;
            c.ReadPacket(&packet);
            if ( packet.readComplete ) {
               sf->nexusMajorVersion = packet.PopUChar();
               sf->nexusMinorVersion = packet.PopUChar();
               sf->nexusMinorMinorVersion = packet.PopUChar();
               char *versionModifier = packet.PopString();
               if ( versionModifier ) {
                  strcpy(sf->nexusVersionModifier,versionModifier);
                  S_free(versionModifier);
	          }
               char *versionString = packet.PopString();
               if ( versionString ) {
                  strcpy(sf->nexusVersionString,versionString);
                  S_free(versionString);
	          }
               sf->needToUpgrade = FALSE;
               if ( sf->nexusMajorVersion > SF_MAJOR_VERSION )
                  sf->needToUpgrade = TRUE;
               if ( sf->nexusMajorVersion == SF_MAJOR_VERSION &&
                    sf->nexusMinorVersion > SF_MINOR_VERSION )
                  sf->needToUpgrade = TRUE;
               if ( sf->nexusMajorVersion == SF_MAJOR_VERSION &&
                    sf->nexusMinorVersion == SF_MINOR_VERSION &&
                    sf->nexusMinorMinorVersion > SF_MINOR_MINOR_VERSION )
                  sf->needToUpgrade = TRUE;
	       }
            c.Disconnect();
	    }
         }

      /* *************************** */
      /* *** Show upgrade status *** */
      /* *************************** */
      if ( nexusUpgradeButton )
         nexusUpgradeButton->SetEnabled(TRUE/*sf->needToUpgrade*/);
      if ( nexusRefreshButton )
         nexusRefreshButton->SetEnabled(TRUE);

      if ( nexusUpgradeReminder && sf->needToUpgrade ) {
         char tmp[128];
         sprintf(tmp,"<<< Upgrade Available\n<<< (version %s)",
                 sf->VersionString(sf->nexusMajorVersion,sf->nexusMinorVersion,
                 sf->nexusMinorMinorVersion,sf->nexusVersionModifier,
                 sf->nexusVersionString));
	 if ( nexusUpgradeReminder )
	   nexusUpgradeReminder->SetText(tmp);
         }
      /* *************************** */

      /* ************************************* */
      /* *** Get banner ad info from nexus *** */
      /* ************************************* */
      if ( !sf->bmpBanner || !sf->urlBanner )
	{
	  S_Client c;
	  c.SetTimeOut(SF_CLIENT_TIMEOUT);
	  int rc = c.Connect(nexusName,SF_NEXUS_PORT,SF_NEXUS_GETBANNERAD);
	  if ( rc == SF_NEXUS_OK )
	    {
	      S_Packet packet;
	      if ( sf->bmpBannerFilename )
		{
		  S_free(sf->bmpBannerFilename);
		  sf->bmpBannerFilename = NULL;
		}
	      if ( sf->urlBanner )
		{
		  S_free(sf->urlBanner);
		  sf->urlBanner = NULL;
		}
	      c.ReadPacket(&packet);
	      sf->bmpBannerFilename = packet.PopString();
	      sf->urlBanner = packet.PopString();

	      S_FileClient dclient;
	      if ( sf->bmpBannerFilename )
		{
		  char filename[128];
		  sprintf(filename,"%s.bmp",sf->bmpBannerFilename);

		  if ( !_OsFileExists(filename) )
		    {
		      dclient.RequestFile(nexusName,
					  SF_NEXUS_PORT,
					  filename,
					  &packet);
		      if ( dclient.rc == SF_NEXUS_OK )
			dclient.WriteFile(filename,&packet);
		    }
		  sf->bmpBanner = new S_SystemBitmap(sf->bmpBannerFilename);

		  if ( internetDialog )
		    internetDialog->Refresh();
		}
	      c.Disconnect();
	    }
	}

      //
      // Get the news file from the nexus.
      //

      if ( !newsUpdated )
	{

          newsUpdated = TRUE;
      
	  S_Packet packet;
	  packet.Flush();

	  S_FileClient dclient;
	  dclient.RequestFile(nexusName,SF_NEXUS_PORT,"news.txt",&packet);
	  if ( dclient.rc == SF_NEXUS_OK )
	    {
	      dclient.WriteFile("news.txt",&packet);
	    }

	  if ( newsTextEdit && _OsFileExists("news.txt") )
	    {
	      newsTextEdit->SetReadOnly(FALSE);
	      int bytesRead = 0;
	      char *buffer = NULL;
	      ulong size = _OsFileSize("news.txt");
	      if ( size > 0 )
		{
		  FILE *fptr = fopen("news.txt","rb");
		  if ( fptr )
		    {
		      buffer = (char *)S_malloc(size);
		      if ( buffer )
			{
			  bytesRead = fread(buffer,1,size,fptr);
			  for ( i=0; i<bytesRead; i++ )
			    {
			      if ( buffer[i] == 13 )
				buffer[i] = ' ';
			      if ( buffer[i] == 10 )
				buffer[i] = 0;
			    }
			}
		      fclose(fptr);
		    }
		}

	      if ( newsTextEdit && buffer )
		{
		  i = 0;
		  do
		    {
		      if ( newsTextEdit )
			newsTextEdit->AppendLine(buffer+i);
		      i += strlen(buffer+i);
		      i++;
		    } while ( i < bytesRead );
		  S_free(buffer);
		  if ( newsTextEdit )
		    newsTextEdit->ScrollToTop();
		  if ( newsTextEdit )
		    newsTextEdit->SetReadOnly(TRUE);
		}


	    }
	}
      /* ******************************************************************* */
      /* ******************** Upgrade Stellar Frontier ********************* */
      /* ******************************************************************* */
      if ( nexusRefresh & SF_CLIENT_NEXUS_UPGRADE )
	{
	  if ( nexusStatusText )
	    {
	      sprintf(text,"Upgrading from %s",nexusName);
	      if ( nexusStatusText )
		nexusStatusText->SetText(text);
	    }
	  
	  /* ************************************************************* */
	  /* ********* Get the upgrade executable from the server ******** */
	  /* ************************************************************* */
#         ifdef OS2
	    char *upgrade = "..\\binos2\\upgrade.exe";
#         endif
#         ifdef WIN32
	    char *upgrade = "..\\binwin\\upgrade.exe";
#         endif
#         ifdef UNIX
	    char *upgrade = "../binlin/upgrade.exe";
#         endif
	  if ( sf->profile->experimental )
	    {
#             ifdef OS2
	        char *upgrade = "..\\binos2\\upgrade-x.exe";
#             endif
#             ifdef WIN32
	        char *upgrade = "..\\binwin\\upgrade-x.exe";
#             endif
#             ifdef UNIX
	        //char *upgrade = "../binlin/upgrade-x.exe";
#             endif
	    }

	  int rc;
	  S_Packet packet;
	  S_FileClient dclient;
	  rc = dclient.RequestFile(nexusName,SF_NEXUS_PORT,upgrade,&packet);
	  switch(rc)
	    {
            case S_FILECLIENT_OK :
	      rc = dclient.WriteFile(upgrade,&packet);
	      if ( rc == S_FILECLIENT_FILEWRITEFAILED )
		{
                  sprintf(text,
			  "The file %s was downloaded from the nexus at %s, "
			  "but I couldn't save it to your local hard disk :(",
			  upgrade,nexusName);
                  S_MsgBox(sf->mainWindow,
                           "Stellar Frontier: Upgrade Failed",
                           text,S_MSGBOX_OK);
		}
	      break;
            case S_CLIENT_TCPCONNECTFAILED :
	      sprintf(text,
		      "I could not connect to the nexus at %s :(",nexusName);
	      S_MsgBox(sf->mainWindow,"Stellar Frontier: Upgrade Failed",
		       text,S_MSGBOX_OK);
	      break;
            case S_FILECLIENT_FILEDOESNOTEXIST :
	      sprintf(text,
		      "I could not find the file %s on the nexus at %s :( "
		      "It may be that this upgrade doesn't support your "
		      "Operating system yet.",upgrade,nexusName);
	      S_MsgBox(sf->mainWindow,"Stellar Frontier: Upgrade Failed",
		       text,S_MSGBOX_OK);
	      break;
            case S_FILECLIENT_PACKETREADFAILED :
	      sprintf(text,
		      "Failed to read %s from the nexus at %s.  Read "
		      "%ld/%ld bytes",upgrade,nexusName,
		      packet.totalBytesRcvd,packet.packetSize);
	      S_MsgBox(sf->mainWindow,"Stellar Frontier: Upgrade Failed",
		       text,S_MSGBOX_OK);
	      break;
            default :
	      sprintf(text,
		      "The nexus at %s does not support upgrades for "
		      "your operating system yet. Return code is %d",
		      nexusName,rc);
	      S_MsgBox(sf->mainWindow,"Stellar Frontier: Upgrade Failed",
		       text,S_MSGBOX_OK);
	      break;
	    }
	  /* ************************************************************* */

	  /* ************************************************************* */
	  /* ****** Run the upgrade program and close the SF Client ****** */
	  /* ************************************************************* */
	  if ( rc == S_FILECLIENT_OK )
	    {
	      S_Process process;
	      process.Start(upgrade,nexusName);
	      sf->mainWindow->Close();
	    }
	  /* ************************************************************* */
	}
      /* ******************************************************************* */

      /* ******************************************************************* */
      /* ***************** Get a list of internet servers ****************** */
      /* ******************************************************************* */
      if ( nexusRefresh & SF_CLIENT_NEXUS_REFRESHSERVERS ||
           nexusRefresh & SF_CLIENT_NEXUS_REFRESHLEADERBOARD ) {

         if ( client )
            numServers = client->GetInternetServerList(nexusName);

         /* **************************************************************** */
         /* *************** Get Leader Board data from Nexus *************** */
         /* **************************************************************** */
         if ( numServers >= 0 && !leaderBoardUpdated && 
              nexusRefresh & SF_CLIENT_NEXUS_REFRESHLEADERBOARD ) {
            slong rc;
            S_Packet packet;
            if ( nexusStatusText )
               nexusStatusText->SetText(
                  "Getting Leader Board data from nexus . . .");
            rc = client->GetLeaderBoardDataFromNexus(nexusName,1000,
                                                     "Score",
                                                     &packet);
            if ( playerStatsListBox && rc == SF_NEXUS_OK ) {
               leaderBoardUpdated = TRUE;
	       if ( playerStatsListBox )
		 playerStatsListBox->Flush();
               char *keyword,*value;
               int i=1;
               do {
                  keyword = packet.PopString();
                  value = packet.PopString();
                  if ( keyword && value ) {
                     char tmp[256];
                     if ( strlen(keyword) > 15 )
                        keyword[15] = 0;
                     sprintf(tmp,"%4d: %-25s %s points",i++,keyword,value);
		     if ( playerStatsListBox )
		       playerStatsListBox->AddItem(tmp);
		  }
                  if ( keyword )
                     S_free(keyword);
                  if ( value )
                     S_free(value);
                  } while ( keyword && value );
	       }
	    }
         }
      /* ******************************************************************* */


      /* ******************************************************************* */
      /* ************ Ping the servers in the server list ****************** */
      /* ******************************************************************* */
      if ( refreshContinue &&
           nexusRefresh & SF_CLIENT_NEXUS_REFRESHSERVERS )
	{

	  if ( serverListListBox && serverListListBox->GetNumItems() == 0 )
	    {
	      for ( i=0; i<numServers; i++ )
		{
		  S_String str(
			       "%s (%s) [ %s | %d(%d)/%d players | ??ms | %s ]",
                               client->serverList[i].serverName,
			       client->serverList[i].hostName,
                               client->serverList[i].osString,
                               client->serverList[i].numPlayers,
                               client->serverList[i].numHumanPlayers,
                               client->serverList[i].maxPlayers,
			       client->serverList[i].upTimeStr);
		  if ( serverListListBox )
		    serverListListBox->AddItem(str.str);
		}
	    }

	  for ( i=0; i<numServers; i++ )
	    {

	      if ( refreshContinue &&
                   nexusRefresh & SF_CLIENT_NEXUS_REFRESHSERVERS )
		{
		  sprintf(text,"Contacting %s on port %d. . .\n",
			  client->serverList[i].hostName,
			  client->serverList[i].port);
		  if ( nexusStatusText )
		    {
		      nexusStatusText->SetText(text);
		    }
		  if ( refreshContinue )
		    {
		      char *touchText = 
			client->TouchServer(&client->serverList[i]);
		      if ( refreshContinue && serverListListBox )
			{
			  int index = -1;
			  if ( serverListListBox )
			    serverListListBox->GetIndex();
			  if ( serverListListBox )
			    serverListListBox->RemoveItem(i);
			  if ( serverListListBox )
			    serverListListBox->InsertItem(touchText,i);
			  if ( refreshContinue && index >= 0 )
			    {
			      if ( serverListListBox )
				serverListListBox->SetIndex(index);
			    }
			}
		      if ( nexusStatusText )
			nexusStatusText->SetText(" ");
		    }
		}

	    }

	}
      
      if ( refreshContinue && nexusStatusText )
	{
	  if ( numServers > 1 )
            sprintf(text,"Found %d servers.  Select one and press 'Ok'.",i);
	  else if ( numServers == 1 )
            sprintf(text,"Found %d server.  Select it and press 'Ok'.",i);
	  else if ( numServers == 0 )
	    sprintf(text,"Nexus reports there are no servers out there.");
	  else
            sprintf(text,"Nexus is not responding.");
	  if ( nexusStatusText )
            nexusStatusText->SetText(text);
	}
      /* ******************************************************************* */
      
      if ( refreshContinue )
	{
	  if ( numServers >= 0 )
	    refreshEvent.WaitFor(60000000); /* *** 1 min. *** */
	  else
	    _OsSleep(1000000);
	}

      refreshEvent.Reset();

   }
   
   return 0;
   }



int
NexusRefreshButtonHandler(S_Button *button) {
   nexusRefresh = SF_CLIENT_NEXUS_REFRESHSERVERS |
                  SF_CLIENT_NEXUS_REFRESHLEADERBOARD;
   refreshEvent.Set();
   return 0;
   }


int
NexusUpgradeButtonHandler(S_Button *button) {
   nexusRefresh = SF_CLIENT_NEXUS_UPGRADE;
   refreshEvent.Set();
   return 0;
   }


int
PlayerStatComboBoxHandler(S_ComboBox *cb) {
  /*
   nexusRefreshAll = FALSE;
   if ( refreshEvent )
      refreshEvent->Set();
      */
   return 0;
   }

char *internetHelpText = 
"And now some words from our sponsor "
"(Click on this advertisement to support Stellar Frontier)";


int
InternetMenuItemHandler(S_MenuItem *menuItem) {
   sf->mouseMenuEnabled = FALSE;
   sf->internetMenuItem->SetEnabled(FALSE);
   internetDialog = new S_Dialog(menuItem->window,"Play on the Internet",
                                 InternetDialogHandler,0,
                                 menuItem->window->cx/2-320,
                                 menuItem->window->cy/2-275,
                                 640,550);
   internetDialog->selfDelete = TRUE;
   internetDialog->Show();
   return 0;
   }


int
_DrawBanner(S_Window *dialog,int x1,int y1,int x2,int y2,int up) {
   if ( sf->bmpBanner) {
      if ( up ) {
         sf->bmpBanner->Blit(dialog,0,x1-1,y1-1);
         dialog->DrawLine(x1,y1,x1,y2,2,RGB_PALEGRAY);
         dialog->DrawLine(x1,y1,x2,y1,2,RGB_PALEGRAY);
         dialog->DrawLine(x1,y2,x2,y2,2,RGB_DARKGRAY);
         dialog->DrawLine(x2,y1,x2,y2,2,RGB_DARKGRAY);
         }
      else {
         sf->bmpBanner->Blit(dialog,0,x1,y1);
         dialog->DrawLine(x1,y1,x1,y2,2,RGB_DARKGRAY);
         dialog->DrawLine(x1,y1,x2,y1,2,RGB_DARKGRAY);
         dialog->DrawLine(x1,y2,x2,y2,2,RGB_PALEGRAY);
         dialog->DrawLine(x2,y1,x2,y2,2,RGB_PALEGRAY);
         }
      }
   return 0;
   }

S_Thread *nexusRefreshThread;
S_StaticText *internetStaticText;
S_Button *internetCancelButton;
S_Button *internetOkButton;


int
InternetDialogHandler(S_Dialog *dialog) {

   int margin = 10;
   int x = margin;
   int y = margin;
   int labelHeight = 16;
   int width = dialog->cxClient - 2*margin;
   int cyStatTextBox = 180;
   int serverListBoxWidth = width;
   int helpTextHeight = 80;
   int cyButton = 35;
   int cxButton = 70;
   int cyTextBox = 25;
   int cxTextBox = 150;
   int serverListListBoxHeight = 120;

   int adx1 = 0;
   int adx2 = 0;
   int ady1 = 0;
   int ady2 = 0;

   if ( sf->bmpBanner ) {
      adx1 = dialog->cxClient/2-sf->bmpBanner->cx/2;
      ady1 = 30;
      adx2 = adx1 + sf->bmpBanner->cx;
      ady2 = ady1 + sf->bmpBanner->cy;
      }

   SF_Client *client = (SF_Client *) dialog->parent->puser;

   switch(dialog->msg) {

      case S_WINDOW_CREATE :
	{
         internetStaticText = new S_StaticText(dialog,internetHelpText,
                                               x,y,width,labelHeight);

         y += margin + helpTextHeight + labelHeight;
         nexusNameTextBox = new S_TextBox(dialog,NULL,
                                          client->profile->nexusName,
                                          x,y,
                                          cxTextBox,cyTextBox);
         nexusNameTextBox->AddLabel("NEXUS Name",cxTextBox,labelHeight);

         nexusStatusText = new S_StaticText(dialog,"Contacting nexus . . .",
                                            x+cxTextBox+margin,y,
                                            serverListBoxWidth-cxTextBox-
                                            margin,
                                            16);

         y += cyTextBox;
         serverListListBox = new S_ListBox(dialog,ServerListListBoxHandler,
                                           x,y,serverListBoxWidth,
                                           serverListListBoxHeight);

         y += serverListListBoxHeight + margin + 20;
         playerStatsListBox = new S_ListBox(dialog,ServerListListBoxHandler,
                                            x,y,
                                            serverListBoxWidth,
                                            cyStatTextBox);
         playerStatsListBox->SetFont(S_FONT_COURIER,8);
         playerStatsListBox->AddLabel("Galactic Aces",serverListBoxWidth/2,16);
         playerStatsListBox->Hide();

         newsTextEdit = new S_TextEdit(dialog,NULL,NULL,
                                            x,y,
                                            serverListBoxWidth,
                                            cyStatTextBox);
         newsTextEdit->SetFont(S_FONT_COURIER,8);
         newsTextEdit->AddLabel("Stellar Frontier News",serverListBoxWidth/2,16);
         newsTextEdit->Hide();

         internetToggleButton = new S_Button(dialog,InternetToggleButtonHandler,
                                             "Show Aces",
                                             x+serverListBoxWidth-cxButton-10,
                                             y-cyButton/2-4,
                                             cxButton+10,
                                             cyButton/2+4);

	 int n = rand() % 2;
         if ( n )
	   {
             playerStatsListBox->Hide();
             newsTextEdit->Show();
             internetToggleButton->SetText("Show Aces");
	   }
	 else
	   {
             playerStatsListBox->Show();
             newsTextEdit->Hide();
             internetToggleButton->SetText("Show news");
	   }

         y += cyStatTextBox + margin;
         nexusRefreshButton = new S_Button(dialog,NexusRefreshButtonHandler,
                                           "Refresh",
                                           x,y,cxButton,cyButton);

         nexusUpgradeButton = new S_Button(dialog,NexusUpgradeButtonHandler,
                                           "Upgrade",
                                           x+margin+cxButton,y,
                                           cxButton,cyButton);
         /*nexusUpgradeButton->SetEnabled(FALSE);*/

         nexusUpgradeReminder = new S_StaticText(dialog,"",
                                                 x+2*margin+2*cxButton,
                                                 y,
                                                 dialog->cxClient -
                                                 4*cxButton - 6*margin,
                                                 cyButton);
#        ifdef OS2
            nexusUpgradeReminder->SetFont(S_FONT_HELV,8);
#        else
            nexusUpgradeReminder->SetFont(S_FONT_HELV,10);
#        endif

         internetOkButton = new S_Button(dialog,
                                         InternetOkButtonHandler,
                                         "Ok",margin+width-cxButton,y,
                                         cxButton,cyButton);
         internetCancelButton = new S_Button(dialog,
                                             CancelButtonHandler,
                                             "Cancel",width-2*cxButton,y,
                                             cxButton,cyButton);

         nexusRefresh = SF_CLIENT_NEXUS_REFRESHSERVERS |
                        SF_CLIENT_NEXUS_REFRESHLEADERBOARD;
         nexusRefreshThread = new S_Thread(_NexusRefreshThread,(void *)dialog);
         break;
	}

      case S_WINDOW_MOUSE : {
         int mouseOver = dialog->mx > adx1 && dialog->mx < adx2 &&
                         dialog->my > ady1 && dialog->my < ady2;
         if ( dialog->button == 0 && sf->bmpBanner ) {
            if ( !mouseOver ) {
               _DrawBanner(dialog,adx1,ady1,adx2,ady2,TRUE);
               }
	    }
         if ( dialog->button == 1 && sf->bmpBanner ) {
            if ( mouseOver && sf->urlBanner ) {
               _DrawBanner(dialog,adx1,ady1,adx2,ady2,dialog->keyup);
               if ( dialog->keyup )
                  _OsOpenURL(sf->urlBanner);
	       }
	    }
         break;
         }

      case S_WINDOW_PAINT :
         _DrawBanner(dialog,adx1,ady1,adx2,ady2,TRUE);
         break;

      case S_WINDOW_CLOSE :
         refreshContinue = FALSE;
         refreshEvent.Set();
         if ( nexusRefreshThread ) {
	   //nexusRefreshThread->WaitFor();
	    nexusRefreshThread->Stop();
            delete nexusRefreshThread;
	    }

         if ( internetCancelButton ) {
            delete internetCancelButton;
            internetCancelButton = NULL;
            }
         if ( internetStaticText ) {
            delete internetStaticText;
            internetStaticText = NULL;
            }
         if ( nexusRefreshButton ) {
            delete nexusRefreshButton;
            nexusRefreshButton = NULL;
            }
         if ( nexusUpgradeButton ) {
            delete nexusUpgradeButton;
            nexusUpgradeButton = NULL;
            }
         if ( nexusUpgradeReminder ) {
            delete nexusUpgradeReminder;
            nexusUpgradeReminder = NULL;
            }
         if ( nexusStatusText ) {
            delete nexusStatusText;
            nexusStatusText = NULL;
            }
         if ( serverListListBox ) {
            delete serverListListBox;
            serverListListBox = NULL;
            }
         if ( playerStatsListBox ) {
            delete playerStatsListBox;
            playerStatsListBox = NULL;
            }
         if ( newsTextEdit ) {
            delete newsTextEdit;
            newsTextEdit = NULL;
            }
         if ( internetToggleButton ) {
            delete internetToggleButton;
            internetToggleButton = NULL;
            }
         if ( internetOkButton ) {
            delete internetOkButton;
            internetOkButton = NULL;
            }
         client->profile->SetNexusName(nexusNameTextBox->GetString());
         if ( nexusNameTextBox ) {
            delete nexusNameTextBox;
            nexusNameTextBox = NULL;
            }
         sf->mouseMenuEnabled = TRUE;
         sf->internetMenuItem->SetEnabled(TRUE);
         break;

      }
   return 0;
   }
/* ************************************************************************* */
/* ***************************** Internet Dialog *************************** */
/* ************************************************************************* */


/* ************************************************************************* */
/* ***************************** Intranet Dialog *************************** */
/* ************************************************************************* */
S_StaticText *intranetStaticText;
S_Button *touchButton;
S_Button *intranetOkButton;
S_TextBox *serverNameTextBox;
S_Button *intranetCancelButton;

int touchContinue;
S_Event *touchEvent;


int
_TouchThread(void *arg) {

   S_Dialog *dialog = (S_Dialog *)arg;
   touchEvent = new S_Event();

   touchEvent->Reset();
   touchContinue = TRUE;
   while ( touchContinue ) {

      touchEvent->WaitFor(60000000); /* *** 1 min. *** */
      touchEvent->Reset();

      if ( touchContinue ) {
         if ( touchButton )
            touchButton->SetEnabled(FALSE);
         SF_Client *client = (SF_Client *) dialog->parent->puser;
         ServerDataStruct sd;
         S_TokenArray tokens(serverNameTextBox->GetString(),':',':',
			     S_TOKENARRAY_SINGLEDELIMIT);
         if ( tokens.numLines >= 1 && tokens.lines[0] )
	   {
             strcpy(sd.hostName,tokens.lines[0]);
	   }
         sd.port = SF_SERVER_PORT;
         if ( tokens.numLines >= 2 && tokens.lines[1] )
	   {
             sd.port = atoi(tokens.lines[1]);
	   }
         sf->serverPort = sd.port;
         client->TouchServer(&sd);
         if ( serverNameTextBox && serverNameTextBox->label )
            serverNameTextBox->label->SetText(client->touchString.str);
         if ( touchButton )
            touchButton->SetEnabled(TRUE);
         }

      }

   delete touchEvent;
   touchEvent = NULL;
   
   return 0;
   }


int
TouchButtonHandler(S_Button *button) {
   if ( touchEvent )
      touchEvent->Set();
   return 0;
   }


int
IntranetOkButtonHandler(S_Button *button) {
   if ( button && button->parent ) {
      SF_Client *client = (SF_Client *)button->parent->parent->puser;
      client->StartEngine(serverNameTextBox->GetString());
      button->parent->Close();
      }
   return 0;
   }

char *intranetHelpText =
"If you don't have internet access, but are connected to a local intranet or "
"if you already know the address of the server where you want to play Stellar "
"Frontier, you can simply enter the address in the 'Server Name' text box "
"below.  Press 'Touch' to get the server stats and press 'Ok' to play.";


int
IntranetMenuItemHandler(S_MenuItem *menuItem) {
   sf->mouseMenuEnabled = FALSE;
   sf->intranetMenuItem->SetEnabled(FALSE);
   intranetDialog = new S_Dialog(
                      menuItem->window,"Play on local intranet",
                      IntranetDialogHandler,0,
                      menuItem->window->root->cx/2-270,
                      menuItem->window->root->cy/2-110,
                      540,220);
   intranetDialog->selfDelete = TRUE;
   intranetDialog->Show();
   return 0;
   }

S_Thread *touchThread;


int
IntranetDialogHandler(S_Dialog *dialog) {

   int margin = 10;
   int x = margin;
   int y = margin;
   int labelHeight = 16;
   int width = dialog->cxClient - 2*margin;
   int helpTextHeight = 85;
   int cyButton = 35;
   int cxButton = 70;
   int cyTextBox = 25;
   int cxTextBox = width;
   int cyBuffer = dialog->cyClient-4*margin-helpTextHeight-cyButton-cyTextBox-
                  labelHeight;
   SF_Client *client = (SF_Client *) dialog->parent->puser;

   switch(dialog->msg) {

      case S_WINDOW_CREATE :

         intranetStaticText = new S_StaticText(dialog,intranetHelpText,
                                             x,y,width,helpTextHeight);

         y += helpTextHeight + margin + labelHeight + cyBuffer;
         serverNameTextBox = new S_TextBox(dialog,NULL,
                                           client->profile->serverName,x,y,
                                           cxTextBox,cyTextBox);
         serverNameTextBox->AddLabel("Server Name",cxTextBox,labelHeight);

         y += cyTextBox + margin;
         touchButton = new S_Button(dialog,TouchButtonHandler,"Touch",x,y,
                                     cxButton,cyButton);
         intranetOkButton = new S_Button(dialog,IntranetOkButtonHandler,
                                      "Ok",margin+width-cxButton,y,
                                      cxButton,cyButton);
         intranetCancelButton = new S_Button(dialog,
                                     CancelButtonHandler,
                                     "Cancel",width-2*cxButton,y,
                                     cxButton,cyButton);
         touchThread = new S_Thread(_TouchThread,(void *)dialog);
         break;
      case S_WINDOW_CLOSE :
         client->profile->SetServerName(serverNameTextBox->GetString());
         client->profile->Save();

         touchContinue = FALSE;
         touchEvent->Set();

         if ( touchThread ) {
            delete touchThread;
            touchThread = NULL;
            }

         if ( intranetStaticText ) {
            delete intranetStaticText;
            intranetStaticText = NULL;
            }
         if ( touchButton ) {
            delete touchButton;
            touchButton = NULL;
            }
         if ( intranetOkButton ) {
            delete intranetOkButton;
            intranetOkButton = NULL;
            }
         if ( serverNameTextBox ) {
            delete serverNameTextBox;
            serverNameTextBox = NULL;
            }
         if ( intranetCancelButton ) {
            delete intranetCancelButton;
            intranetCancelButton = NULL;
            }
         sf->mouseMenuEnabled = TRUE;
         sf->intranetMenuItem->SetEnabled(TRUE);
         break;

      }
   return 0;
   }


int
SF_Client::StartEngine(char *serverString,char *gameStateFilename) {
  if ( engine )
    {
      if ( engine->state == SF_CLIENT_CONNECTING )
	{
	  engine->client->stopConnect = TRUE;
	  engine->stopFast = TRUE;
	  engine->engineThread->WaitFor();
	}
      else
	{
	  engine->stopFast = TRUE;
	  engine->run = FALSE;
	}
      delete engine;
    }

   char *serverName = NULL;
   sf->serverPort = SF_SERVER_PORT;

   S_TokenArray tokens(serverString,':',':',S_TOKENARRAY_SINGLEDELIMIT);
   if ( serverString && tokens.lines )
     {
       serverName = tokens.lines[0];
       if ( tokens.numLines >= 2 && tokens.lines[1] )
	 {
	   sf->serverPort = atoi(tokens.lines[1]);
	 }
     }

   rcEngine = 0;
   engine = new SF_Engine(serverName,this,msl,gameStateFilename);

   if ( mode == SF_CLIENT_DATA_ENGINE_DISPLAY ) {
 
      if ( serverName ) {
         disconnectMenuItem->SetEnabled(TRUE);
         }
      else {
         stopMenuItem->SetEnabled(TRUE);
         pauseMenuItem->SetEnabled(TRUE);
         }

      }

   return 0;
   }


int
SF_Client::WaitForEngine() {
   if ( engine && engine->engineThread ) {
      engine->engineThread->WaitFor();
      }
   return 0;
   }


int
SF_Client::StopEngine(int stopFast) {
   if ( engine != NULL ) {
      engine->stopFast = stopFast;
      engine->selfTermination = TRUE;
      engine->run = FALSE;
      }
   return 0;
   }


#define NUM_CREDIT_LINES 50

char *creditsText[NUM_CREDIT_LINES] = {
   " ",
   " ",
   " ",
   " ",
   " ",
   " ",
   "Credits",
   " ",
   "Game Design and Programming",
   "   Douglas Hendrix",
   "   Neil Hillis",
   "   Brad Wardell",
   " ",
   "Artificial Intelligence ",
   "   Neil Hillis",
   " ",
   "Artwork",
   "   Alex Gounaropoulos",
   "   Nathan Allen",
   "   Michl Binderbauer",
   " ",
   "Music",
   "   Walter Ho",
   " ",
   " ",
   "Solar Systems Beta testers",
   "Michael D'Amico, Derek Suzuki, Anthony Sintes",
   "Mychal Manie Gernot Stoeckl, Ronnie Morgan",
   "Robert McDermid, Wayne Dunham, Chad Montplaisir",
   "David Mearns, John Lagerquist, Roger Johnson",
   "Christopher Dick, Jim Lewis, Andrew Welty",
   "James Kurien, Rick Vinger, Anthony Beckett",
   "William Barnes, Wim Heemskerk, Brandy Bogard",
   "Johannes Gramsch, Christian Andersen, Robert Schroeder",
   "David Eckard, Gregory Breslin, Christopher Blackmon",
   "Gary Church, Jonathan Grande, Simon Peacock",
   "John Brack, Walter Haberl, Matthew Cleveland",
   "Jochen Hoff, Glenn Jordan, Aaron Jordan",
   "Gaelen Jorden, Shawn Fogg, David D'Antonio",
   "Fernando Cassia, David Benhart, Edward Lentz",
   "Wojciech Giziewicz, Douglas Munson, Ben Lee",
   "Stan Michalowski, Don Eddleman, Jason Nielsen",
   "Mikel Featherston, Alan Van Drie, Forrest Peterson",
   "Alexander Antoniades, Kris Kwilas, Pat Ford ",
   " ",
   "Developed and Published by Stardock Systems",
   " ",
   "Stellar Frontier is copyright 1997-2003",
   "Stardock Systems, Canton, MI.",
   "All rights reserved"
   };


int
SF_Client::ResetScrollingText() {
   scrollingText->Flush();
   AddHighScoresTo(scrollingText);
   AddCreditsTo(scrollingText);
   return 0;
   }


int
SF_Client::AddCreditsTo(SF_TextBuffer *tb) {
   for ( int i=0; i<NUM_CREDIT_LINES; i++ )
      tb->AddLine(creditsText[i],RGB_WHITE);
   return 0;
   }


int
SF_Client::AddHighScoresTo(SF_TextBuffer *tb) {

   int i;
   int showHighScores = FALSE;
   for ( i=0; i<profile->numStoredScenarios; i++ )
      if ( profile->scenarios[i]->numHighScores > 0 )
         showHighScores = TRUE;
   if ( !showHighScores )
      return 0;


   tb->AddLine("High Scores",RGB_WHITE);
   tb->AddLine(" ");
   tb->AddLine(" ");

   for ( i=0; i<profile->numStoredScenarios; i++ ) {
      SF_Scenario *scn = profile->scenarios[i];
      if ( scn->numHighScores > 0 ) {
         tb->AddLine(scn->scenarioName,RGB_WHITE);
         scn->SortHighScores();
         scn->CropHighScores(3);
         for ( int j=0; j<scn->numHighScores; j++ ) {
            SF_HighScore *hs = scn->highScores[j];
            char tmp[1024];
            sprintf(tmp,"%s ----- %ld",hs->name,hs->score);
            if ( hs->minutes > 0 || hs->seconds > 0 )
               sprintf(tmp,"%s [ %ld:%ld ]",tmp,hs->minutes,hs->seconds);
            tb->AddLine(tmp,RGB_WHITE);
            }
         tb->AddLine(" ");
         }
      }
   return 0;
   }


int
SF_Client::DrawCredits() {
   if ( !bmpSplash )
     return 0;
   if ( runCredits && backBuffer ) {

      int x = 50;
      int y = 10;
      int cy = 100;
      int xLeft = mainWindow->cxClient/2 - bmpSplash->cx/2 + x;
      int xRight = mainWindow->cxClient/2 + bmpSplash->cx/2 - x;
      int yBottom = mainWindow->cyClient/2 + bmpSplash->cy/2 - y;
      int yTop = yBottom - y - cy;

      if (y0Credits < 
          yTop - (int)(scrollingText->numLines*backBuffer->lMaxBaselineExt)){
         y0Credits = yBottom;
         }

      bmpSplash->SystemBlit(backBuffer,
                            xLeft,
                            yTop-backBuffer->lMaxBaselineExt-1/*-dy2*/, 
                            xRight,
                            yBottom+2,
                            x,
                            y-2);

      if ( backBuffer )
         backBuffer->AddRect(xLeft,
                             yTop-backBuffer->lMaxBaselineExt-1, 
                             xRight,
                             yBottom+2);

      for ( int i=0; i<scrollingText->numLines; i++ ) {
         int y = y0Credits + i*backBuffer->lMaxBaselineExt - dy2;
         if ( y > yTop && y < yBottom ) {
            backBuffer->DrawString(scrollingText->text[i],xLeft+10,y,
                                   scrollingText->textColor[i]);
	    }
         }

      }
   return 0;
   }


int
SF_Client::InSplash(int x,int y) {
   if ( !bmpSplash ||
        x < mainWindow->cxClient/2 - (int)bmpSplash->cx/2 ||
        x > mainWindow->cxClient/2 + (int)bmpSplash->cx/2 ||
        y < mainWindow->cyClient/2 - (int)bmpSplash->cy/2 ||
        y > mainWindow->cyClient/2 + (int)bmpSplash->cy/2 )
      return -1;
   if ( mainWindow->root->cx < 1024 )
      return (y - (mainWindow->cyClient/2 - bmpSplash->cy/2 + 180))/41;
   else
      return (y - (mainWindow->cyClient/2 - bmpSplash->cy/2 + 228))/53;
   }


int
SF_Client::DrawSplash() {
   if ( bmpSplash ) {
      backBuffer->DrawFilledRect(0,0,
                                 mainWindow->root->cx,
                                 mainWindow->root->cy,0);
      bmpSplash->SystemBlit(backBuffer,
                            mainWindow->cxClient/2 - 
                            bmpSplash->cx/2,
                            mainWindow->cyClient/2 - 
                            bmpSplash->cy/2,
                            mainWindow->cxClient/2 + 
                            bmpSplash->cx/2,
                            mainWindow->cyClient/2 + 
                            bmpSplash->cy/2,
                            0,0 );
      if ( bmpSplash == bmpSF1Splash && mouseMenuItem >= 0 )
         DrawMenuItem(mouseMenuItem,bmpSF1Splash);
      }
   return 0;
   }


int
SF_Client::DrawMenuItem(int i,S_SystemBitmap *bmp) {

   if ( mainWindow && !mainWindow->closing && bmp && backBuffer &&
        mainWindow->root ) {

      int x1 = mainWindow->cxClient/2 - bmp->cx/2;
      int x2 = mainWindow->cxClient/2 + bmp->cx/2;
      int x3 = 0;

      int y1,y2,y3;

      if ( mainWindow && mainWindow->root && mainWindow->root->cx < 1024 &&
           bmp ) {
         y1 = mainWindow->cyClient/2 - bmp->cy/2 + 180 + 41*i;
         y2 = mainWindow->cyClient/2 - bmp->cy/2 + 180 + 41*(i+1);
         y3 = bmp->cy - 180 - 41*(i+1);
         }
      else if ( mainWindow && bmp ) {
         y1 = mainWindow->cyClient/2 - bmp->cy/2 + 228 + 53*i;
         y2 = mainWindow->cyClient/2 - bmp->cy/2 + 228 + 53*(i+1);
         y3 = bmp->cy - 228 - 53*(i+1);
         }
      if ( bmp && backBuffer ) {
         bmp->SystemBlit(backBuffer,x1,y1,x2,y2,x3,y3);
         backBuffer->AddRect(x1,y1,x2,y2);
         }
      }

   return 0;
   }


char *
SF_Client::VersionString(int major,
			 int minor,
			 int minorminor,
			 char *imod,
			 char *str) {
   static S_String version;
   S_String mod;

   mod.Create("");
   if ( imod )
      mod.Create(imod);

   if ( str && strlen(str) > 0 ) {
      version.Create(str);
      }
   else if ( minorminor == 0 ) {
      if ( minor < 10 && minor != 0 ) {
	 if ( minor % 10 == 0 )
            version.Create("%d.%d%s",major,minor/10,mod.str);
         else
            version.Create("%d.%02d%s",major,minor,mod.str);
         }
      else {
	 if ( minor % 10 == 0 )
            version.Create("%d.%d%s",major,minor/10,mod.str);
         else
            version.Create("%d.%d%s",major,minor,mod.str);
         }
      }
   else {
      if ( minor < 10 )
         version.Create("%d.%02d.%d%s",major,minor,minorminor,mod.str);
      else
         version.Create("%d.%d.%d%s",major,minor,minorminor,mod.str);
      }

   return version.str;
   }



#define SF_CLIENT_NUMBADWORDS 114
char *badWords[SF_CLIENT_NUMBADWORDS] = {
  "blow me",
  "blow job",
  "blowjob",
  "bastard",
  "whore",
  "slut",
  "scumbag",
  "scum",
  "cumbag",
  "cumbucket",
  "cum ",
  " cum",
  "dirtbag",
  "shitbag",
  "snatch",
  "crap",
  "assholes",
  "asshole",
  "ass hole",
  " ass ",
  "dumbass",
  "dumbfuck",
  "dumbshit",
  "dumb",
  "homosexual",
  "homos",
  "homo",
  "dammit",
  "damnit",
  "damn",
  "goddammit",
  "godammit",
  "god damn",
  "god damnit",
  "vaginas",
  "vagina",
  "cock sucker",
  "cocksucker",
  "cocks",
  "cock",
  "shut up",
  "shitass",
  "shitface",
  "shit face",
  "shits",
  "shit",
  "shitty",
  "fuck off",
  "fuck you",
  "fuck face",
  "fuckface",
  "fucking",
  "fuckin",
  "fucker",
  "fucks",
  "fuck",
  "screw off",
  "screw you",
  "screw face",
  "screwface",
  "screwing",
  "screwin",
  "screwer",
  "screws",
  "screw",
  "frickin",
  "frick",
  "faggot",
  "fags",
  "fag",
  "jack off",
  "jackoff",
  "jackass",
  "jack ass",
  "moron",
  "jerk",
  "jerk off",
  "jerkoff",
  "jerkass",
  "jerk ass",
  "pussies",
  "pussys",
  "pussy",
  "puss",
  "bitches",
  "bitchin",
  "bitch",
  "sucks",
  "suck",
  "cunts",
  "cunt",
  "gays",
  "gay",
  "dickheads",
  "dickhead",
  "dicklick",
  "dicks",
  "dick",
  "penis",
  "nigers",
  "niger",
  "niggers",
  "nigger",
  "rapist",
  "rapes",
  "raped",
  "rape",
  "raping",
  "dykes",
  "dykey",
  "dyke",
  "dikes",
  "dikey",
  "dike"
};
char *fillWord = "!@#$%^&*!@#$%^&*!@$%^&*!@#$%^&*!@#$%^&*";

int
FilterString(char *str)
{
  if ( str )
    {
      int len = strlen(str);
      for ( int i=0; i<len; i++ )
	{
	  for ( int j=0; j<SF_CLIENT_NUMBADWORDS; j++ )
	    {
	      int k;
	      int isBadWord = TRUE;
	      int blen = strlen(badWords[j]);
	      for ( k=0; k<blen; k++ )
		{
		  if ( i+k<len )
		    {	
		      char ch = tolower(str[i+k]);
		      if ( ch != badWords[j][k] )
			{
			  isBadWord = FALSE;
			}
		    }
		  else
		    {
		      isBadWord = FALSE;
		    }
		}
	      if ( isBadWord )
		{
		  int flen = strlen(fillWord);
		  for ( k=0; k<blen; k++ )
		    {
		      if ( i+k<len )
			{
			  str[i+k] = fillWord[(k+rand()) % flen];
			}
		    }
		}
	    }
	}
    }
  return 0;
}
