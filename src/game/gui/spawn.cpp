#define S_LIB_CONSOLE

#include "sf.h"
#include "SF_ClientWindow.h"
#include "SF_Globals.h"
#include "SF_Client.h"
#include "SF_Server.h"
#include "SF_Nexus.h"

int
main(int argc, char *argv[])
{
  int i = 1;

  int checkNexus = FALSE;
  int checkServer = FALSE;

# ifdef UNIX
    char *nexusSpawnCommand = "./nexus";
    char *nexusSpawnParams  = "-log &";
# else
    char *nexusSpawnCommand = "nexus.exe";
    char *nexusSpawnParams  = "-log";
# endif

# ifdef UNIX
    char *serverSpawnCommand = "./server";
    char *serverSpawnParams  = "sf.stardock.net &";
# else
    char *serverSpawnCommand = "server.exe";
    char *serverSpawnParams  = "-autostart server.dat sf.stardock.net";
# endif

  while ( i < argc )
    {
      if ( !strcmp(argv[i],"-nexus") )
	{
	  checkNexus = TRUE;
	}
      else if ( !strcmp(argv[i],"-server") )
	{
	  checkServer = TRUE;
	}
      else if ( !strcmp(argv[i],"-nexusparams") )
	{
          nexusSpawnParams = argv[++i];
	}
      else if ( !strcmp(argv[i],"-serverparams") )
	{
          serverSpawnParams = argv[++i];
	}
      else if ( !strcmp(argv[i],"-nexuscommand") )
	{
          nexusSpawnCommand = argv[++i];
	}
      else if ( !strcmp(argv[i],"-servercommand") )
	{
          serverSpawnCommand = argv[++i];
	}
      i++;
    }

  if ( checkNexus )
    {
      //printf("Nexus Start Command: %s %s\n",
      //     nexusSpawnCommand,
      //     nexusSpawnParams);
    }

  if ( checkServer )
    {
      //printf("Server Start Command: %s %s\n",
      //     serverSpawnCommand,
      //     serverSpawnParams);
    }

  while ( 1 )
    {

      if ( checkNexus )
	{
	  //
	  // Check to see if nexus is up.
	  //
	  int nexusUp = FALSE;
	  S_Client c;
	  c.SetTimeOut(SF_CLIENT_TIMEOUT);
	  int rc = c.Connect("127.0.0.1",SF_NEXUS_PORT,SF_NEXUS_GETVERSION);
	  if ( rc == SF_NEXUS_OK )
	    {
              nexusUp = TRUE;
              c.Disconnect();
	    }

	  //
	  // If not, then restart it.
	  //
	  if ( !nexusUp )
	    {
#             ifdef UNIX 
		S_String command("%s %s",nexusSpawnCommand,nexusSpawnParams);
	        system(command.str);
#             else
		S_Process p;
                p.Start(nexusSpawnCommand,nexusSpawnParams);
#             endif
	    }
	}
      
      if ( checkServer )
	{
	  //
	  // Check to see if server is up.
	  //
	  int serverUp = FALSE;
	  S_Client c;
	  c.SetTimeOut(SF_CLIENT_TIMEOUT);
	  int rc = c.Connect("127.0.0.1",SF_SERVER_PORT,SF_SERVER_TOUCH);
	  if ( rc == SF_SERVER_OK )
	    {
              serverUp = TRUE;
              c.Disconnect();
	    }
      
	  //
	  // If not restart it.
	  //
	  if ( !serverUp )
	    {
#             ifdef UNIX 
		S_String command("%s %s",serverSpawnCommand,serverSpawnParams);
	        system(command.str);
#             else
		S_Process p;
                p.Start(serverSpawnCommand,serverSpawnParams);
#             endif
	    }
	}

      _OsSleep(60000000);

    }

  return 0;
}

