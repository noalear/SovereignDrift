#define S_LIB_CONSOLE

#include "sf.h"
#include "SF_Nexus.h"

int
main(int argc, char *argv[]) {

   /* ********************** */
   /* *** Stop the nexus *** */
   /* ********************** */
   S_Client client;
   if ( !client.Connect("127.0.0.1",SF_NEXUS_PORT,SF_NEXUS_STOP) )
      client.Disconnect();
   printf("%ld\n",client.rc);

   return 0;
   }

