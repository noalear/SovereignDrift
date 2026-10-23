/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
#include "sf.h"
#include "SF_Profile.h"
#include "SF_Player.h"
#include "SF_PortCpp.h"
#include "SF_Externals.h"
#include "SF_Scenario.h"
#include "SF_Engine.h"
#include "SF_Slot.h"
#include "SF_GameEngine.h"
#include "SF_NetEngine.h"
#include "SF_Globals.h"
#include "SF_ClientWindow.h"
#include "SF_Client.h"
#include "SF_HighScore.h"

SF_Profile::SF_Profile() {
   int rc = 0;

#  ifdef SF_EXPERIMENTAL
     experimental = TRUE;
#  else
     experimental = FALSE;
#  endif

   char *fileName = SF_PROFILE_FILENAME;

   selectedPlayer = 0;
   numStoredPlayers = 0;
   players = NULL;
   numStoredScenarios = 0;
   scenarios = NULL;
   serverName = NULL;
   nexusName = NULL;
   reg1 = NULL;
   reg2 = NULL;

   if ( fileName != NULL ) {
     rc = Load();
     }

   if ( fileName == NULL || rc != 0 ) {
      sound = TRUE;
      soundVolume = SF_SOUND_DEFAULTVOLUME;
      music = TRUE;
      musicVolume = SF_MUSIC_DEFAULTVOLUME;
      tcpipOnly = FALSE;

#     ifdef OS2
         music = FALSE;
#     endif

      nexusName = (char *)S_malloc(strlen(SF_NEXUS_DEFAULT)+1);
      strcpy(nexusName,SF_NEXUS_DEFAULT);

      serverName = NULL;

      numStoredPlayers = 0;
      selectedPlayer = -1;

      S_DirList *dirList = new S_DirList(SF_ScenarioFileDirMask);
      for ( int i=0; i<dirList->numFiles; i++ )
         AddScenario(new SF_Scenario(dirList->file[i]));
      }
   }


int
SF_Profile::Exists()
{

  char *filename = SF_PROFILE_FILENAME;

  FILE *fptr = fopen(filename,"rb");
  if ( fptr )
    {
      fclose(fptr);
      return TRUE;
    }
  return FALSE;
}


int
SF_Profile::Load() {

   char *fileName = SF_PROFILE_FILENAME;

   int i;
   FILE *fptr;
   fptr = fopen(fileName,"rb");

   if ( fptr != NULL ) {
      fread(&sound,sizeof(sound),1,fptr);
      fread(&soundVolume,sizeof(soundVolume),1,fptr);
      fread(&music,sizeof(music),1,fptr);
      fread(&musicVolume,sizeof(musicVolume),1,fptr);
      fread(&tcpipOnly,sizeof(tcpipOnly),1,fptr);

      if ( (sound != 0 && sound != 1) || soundVolume>100 ||
           (music != 0 && music != 1) || musicVolume>100 ||
           (tcpipOnly != 0 && tcpipOnly != 1) ) {
         fclose(fptr);
         return SF_PROFILE_CORRUPT;
         }

      short len;
      fread(&len,sizeof(len),1,fptr);
      if ( len >= 0 ) {
         nexusName = (char *)S_malloc(len+1);
         fread(nexusName,len,1,fptr);
         nexusName[len] = 0;
         if ( len != (int)strlen(nexusName) ) {
            fclose(fptr);
            return SF_PROFILE_CORRUPT;
	    }
         }
      else {
         fclose(fptr);
         return SF_PROFILE_CORRUPT;
         }

      fread(&len,sizeof(len),1,fptr);
      if ( len >= 0 ) {
         serverName = (char *)S_malloc(len+1);
         fread(serverName,len,1,fptr);
         serverName[len] = 0;
         if ( len != (int)strlen(serverName) ) {
            fclose(fptr);
            return SF_PROFILE_CORRUPT;
	    }
         }
      else {
         fclose(fptr);
         return SF_PROFILE_CORRUPT;
         }

      /* *** Read in players *** */
      char num;
      fread(&num,sizeof(num),1,fptr);
      for ( i=0; i<num; i++ ) {
         SF_Player *newPlayer = new SF_Player(fptr);
         if ( newPlayer->rc != 0 ) {
            fclose(fptr);
            return SF_PROFILE_CORRUPT;
	    }
         newPlayer->demo = sf->demo;
         AddPlayer(newPlayer);
         }

      fread(&selectedPlayer,sizeof(selectedPlayer),1,fptr);
      if ( selectedPlayer < -1 || selectedPlayer >= num ) {
         fclose(fptr);
         return SF_PROFILE_CORRUPT;
         }

      /* *** Read in scenarios *** */
      fread(&num,sizeof(num),1,fptr);
      for ( i=0; i<num; i++ ) {
         SF_Scenario *newScenario = new SF_Scenario(fptr);
         if ( newScenario->rc != 0 ) {
            fclose(fptr);
            return SF_PROFILE_CORRUPT;
	    }
         AddScenario(newScenario);
         }

      fread(&len,sizeof(len),1,fptr);
      if ( len >= 0 ) {
         reg1 = (char *)S_malloc(len+1);
         fread(reg1,len,1,fptr);
         reg1[len] = 0;
         if ( len != (int)strlen(reg1) ) {
            fclose(fptr);
            return SF_PROFILE_CORRUPT;
	    }
         }
      else {
         fclose(fptr);
         return SF_PROFILE_CORRUPT;
         }

      fread(&len,sizeof(len),1,fptr);
      if ( len >= 0 ) {
         reg2 = (char *)S_malloc(len+1);
         fread(reg2,len,1,fptr);
         reg2[len] = 0;
         if ( len != (int)strlen(reg2) ) {
            fclose(fptr);
            return SF_PROFILE_CORRUPT;
	    }
         }
      else {
         fclose(fptr);
         return SF_PROFILE_CORRUPT;
         }

     fclose(fptr);
     return SF_PROFILE_OK;
     }

   return SF_PROFILE_FILEDOESNOTEXIST;
   }


int
SF_Profile::Save() {

   char *fileName = SF_PROFILE_FILENAME;

   int i;
   FILE *fptr;
   fptr = fopen(fileName,"wb");

   if ( fptr != NULL ) {
      fwrite(&sound,sizeof(sound),1,fptr);
      fwrite(&soundVolume,sizeof(soundVolume),1,fptr);
      fwrite(&music,sizeof(music),1,fptr);
      fwrite(&musicVolume,sizeof(musicVolume),1,fptr);
      fwrite(&tcpipOnly,sizeof(tcpipOnly),1,fptr);

      ushort len = 0;
      if ( nexusName )
         len = strlen(nexusName);
      fwrite(&len,sizeof(len),1,fptr);
      if ( len > 0 )
         fwrite(nexusName,len,1,fptr);

      len = 0;
      if ( serverName )
         len = strlen(serverName);
      fwrite(&len,sizeof(len),1,fptr);
      if ( len > 0 )
         fwrite(serverName,len,1,fptr);

      fwrite(&numStoredPlayers,sizeof(numStoredPlayers),1,fptr);

      for ( i=0; i<numStoredPlayers; i++ ) {
         players[i]->Save(fptr);
         }

      fwrite(&selectedPlayer,sizeof(selectedPlayer),1,fptr);

      fwrite(&numStoredScenarios,sizeof(numStoredScenarios),1,fptr);

      for ( i=0; i<numStoredScenarios; i++ ) {
         scenarios[i]->Save(fptr);
         }

      len = 0;
      if ( reg1 )
         len = strlen(reg1);
      fwrite(&len,sizeof(len),1,fptr);
      if ( len > 0 )
         fwrite(reg1,len,1,fptr);

      len = 0;
      if ( reg2 )
         len = strlen(reg2);
      fwrite(&len,sizeof(len),1,fptr);
      if ( len > 0 )
         fwrite(reg2,len,1,fptr);

     fclose(fptr);
     return 0;
     }

   return -1;
   }

SF_Profile::~SF_Profile() {
   if ( nexusName ) {
      S_free(nexusName);
      nexusName = NULL;
      }
   if ( serverName ) {
      S_free(serverName);
      serverName = NULL;
      }
   if ( players ) {
      for ( int i=0; i<numStoredPlayers; i++ ) {
         if ( players[i] ) {
            delete players[i];
            players[i] = NULL;
            }
         }
      S_free(players);
      players = NULL;
      }
   if ( scenarios ) {
      for ( int i=0; i<numStoredScenarios; i++ ) {
         if ( scenarios[i] ) {
            delete scenarios[i];
            scenarios[i] = NULL;
            }
         }
      S_free(scenarios);
      scenarios = NULL;
      }
   if ( reg1 ) {
      S_free(reg1);
      reg1 = NULL;
      }
   if ( reg2 ) {
      S_free(reg2);
      reg2 = NULL;
      }
   }


int
SF_Profile::SetRegCode(char *ireg1,char *ireg2)
{
  if ( ireg1 && ireg2 )
    {
      if ( reg1 )
	S_free(reg1);
      reg1 = S_strdup(ireg1);
      
      if ( reg2 )
	S_free(reg2);
      reg2 = S_strdup(ireg2);
    }
  
  if ( reg1 && reg2 )
    {
      S_RegCode rc;
      rc.Create(SF_REGCODE_KEY);

      rc.AddPiratedSerial(1551);
      rc.AddPiratedSerial(999999);

      //
      // David123, ARUN, ChrisUK, Maharlika, AnihilatoR1337, StealthEdge,
      // Honda_Fan, Mojoman, O-Mega-Xeon, 0110101101, DARKBLADE, Delta-Xeon
      //
      //rc.AddPiratedSerial(1376);

      //
      // Gator's, FlAMeDX, the claw, RABBI, SISTER ANN
      //
      //rc.AddPiratedSerial(1068);

      //
      // Wren Zeland, bilbo85, tzp166, Zaentz
      //
      //rc.AddPiratedSerial(1562);

      //
      // Dragoon313, jaye II
      //
      //rc.AddPiratedSerial(1372);

      //
      // CROM, galastar
      //
      //rc.AddPiratedSerial(1492);

      rc.GenerateCode(atoi(reg1));
      if ( strcmp(reg2,rc.code.str) == 0 )
	{
	  return SF_PROFILE_OK;
	}
    }

  if ( reg1 && reg2 )
    {
      S_RegCode rc;
      rc.Create(SF_REGCODE_OLDKEY);
      rc.GenerateCode(atoi(reg1));
      rc.AddPiratedSerial(1551);
      if ( strcmp(reg2,rc.code.str) == 0 )
	{
	  return SF_PROFILE_OLDREGCODE;
	}
    }

  return SF_PROFILE_INVALIDREGCODE;
}


int
SF_Profile::SetServerName(char *newServerName) {
   if ( !newServerName )
      return -1;

   if ( serverName == NULL )
      serverName = (char *) S_malloc(strlen(newServerName)+1);
   else
      serverName = (char *) S_realloc(serverName,strlen(newServerName)+1);
   strcpy(serverName,newServerName);
   return 0;
   }


int
SF_Profile::SetNexusName(char *newNexusName) {
   if ( !newNexusName )
      return -1;

   if ( nexusName )
     {
       S_free(nexusName);
       nexusName = NULL;
     }

   nexusName = S_strdup(newNexusName);
   return 0;
   }


int
SF_Profile::AddScenario(SF_Scenario *scenario) {
   if ( scenarios == NULL )
      scenarios = (SF_Scenario **) S_malloc( sizeof(scenario) );
   else
      scenarios = (SF_Scenario **) S_realloc( scenarios,
                                    sizeof(scenario)*(numStoredScenarios+1) );

   scenarios[numStoredScenarios] = scenario;
   numStoredScenarios++;
   return 0;
   }


int
SF_Profile::AddPlayer(SF_Player *player) {
   if ( players == NULL )
      players = (SF_Player **) S_malloc( sizeof(player) );
   else
      players = (SF_Player **) S_realloc( players,
                                        sizeof(player)*(numStoredPlayers+1) );

   players[numStoredPlayers] = player;
   numStoredPlayers++;
   return 0;
   }


int
SF_Profile::RemovePlayer(int index) {
   for ( int i=index+1; i<numStoredPlayers; i++ )
      players[i-1] = players[i];
   numStoredPlayers--;
   return 0;
   }

SF_Scenario *
SF_Profile::GetScenario(char *name) {
   for ( int i=0; i<numStoredScenarios; i++ )
      if ( strcmp(name,scenarios[i]->filename) == 0 )
         return scenarios[i];
   return NULL;
   }


int
SF_Profile::Update(SF_Engine *e) {
   if ( game && selectedPlayer<numStoredPlayers &&
        selectedPlayer >= 0 && players[selectedPlayer] ) {

      SF_Player *p = players[selectedPlayer];

      slong playerIndex = game->ship[e->gameEngine->localSlot].playerIndex;

      if ( game->ship[e->gameEngine->localSlot].active ) {
         p->kills = game->player[(short)playerIndex].numKillsTotal;
         p->losses = game->player[(short)playerIndex].numKilled;
         p->rank = (schar)game->player[(short)playerIndex].rank;
         if ( sf->connectedThroughNexus )
            p->timeOnline += (e->gameEngine->itime - e->startFrame)/frameRate;
         p->score = game->player[(short)playerIndex].totalScore;
         p->credits = game->player[(short)playerIndex].refValue;
         }

      /* *** If played in single player mode, calculate score and record *** */
      if ( e->state == SF_CLIENT_SINGLEPLAYER ) {
         p->score = game->player[(short)playerIndex].totalScore;
         SF_Scenario *scn = GetScenario(e->gameStateFilename);
         if ( !scn ) {
            scn = new SF_Scenario(e->gameStateFilename);
            AddScenario(scn);
	    }
         if ( !e->won )
            game->player[(short)playerIndex].activeFrames = 0;
         if ( p->score > 0 )
            scn->AddHighScore(new SF_HighScore(
                                 p->playerName.str,
                                 p->score,
                                 game->player[(short)playerIndex].activeFrames));
         e->client->ResetScrollingText();
         }

      Save();

      }
   return 0;
   }
