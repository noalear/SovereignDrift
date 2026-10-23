#define S_LIB_CONSOLE
#define SF_MAIN

#include "sf.h"
#include "SF_Globals.h"
#include "SF_ClientWindow.h"
#include "SF_Client.h"
#include "SF_Server.h"
#include "SF_PortCpp.h"
#include "SF_Externals.h"
#include "SF_Player.h"

int
main( int argc, char *argv[] ) {

   if ( argc == 1 ) {
      printf("Usage -- client IPaddress\n");
      return 0;
      }

   logFile.open(logFilename);

   /*
   while ( 1 ) {
      SF_Client client;
      S_Packet packet;

      client.GetLeaderBoardDataFromNexus(argv[1],1000,"Score",&packet);
      int numServers = client.GetInternetServerList(argv[1]);

      client.CreateNewPlayerOnNexus(argv[1],"Kirk","466unu",
                                    "hendrix@solarsystems.com");

      SF_Player *player = new SF_Player("Kirk","466unu","Enterprise");

      client.GetPlayerDataFromNexus(argv[1],player);
      printf("player->playerName: %s\n",player->playerName.str);
      printf("player->score: %d\n",player->score);
      printf("player->kills: %d\n",player->kills);
      printf("player->losses: %d\n",player->losses);

      player->score++;
      player->kills++;
      player->losses++;
      client.SetPlayerDataOnNexus(argv[1],player);
      }
      */

   while ( 1 )
     {
       SF_Client client;
       ServerDataStruct sd;
       sd.port = SF_SERVER_PORT;
       strcpy(sd.hostName,argv[1]);
       printf("%s\n",client.TouchServer(&sd));
       client.Run(argv[1]);
     }

   return(0);
   }



