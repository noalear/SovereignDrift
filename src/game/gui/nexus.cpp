#define S_LIB_CONSOLE
#define SF_MAIN

#include "sf.h"
#include "SF_Nexus.h"
#include "SF_PortCpp.h"
#include "SF_Externals.h"
#include "SF_Globals.h"
#include "SF_ClientWindow.h"
#include "SF_Client.h"

int
main( int argc, char *argv[] )
{

  S_DebugMem_SetPageException();
  S_DebugMem_SetThrowException(FALSE);

  int i = 1;
  int log = 0;
  char *backupHostname = NULL;
  while ( i < argc )
    {
      if ( strcmp(argv[i],"-logscreen") == 0 )
	{
	  log |= S_LOGGER_TOCONSOLE | S_LOGGER_TIMESTAMP;
	}
      else if ( strcmp(argv[i],"-logfile") == 0 )
	{
	  log |= S_LOGGER_TOFILE | S_LOGGER_TIMESTAMP;
	}
      else if ( strcmp(argv[i],"-log") == 0 )
	{
	  log = S_LOGGER_TOFILE | S_LOGGER_TOCONSOLE | S_LOGGER_TIMESTAMP;
	}
      else if ( strcmp(argv[i],"-backup") == 0 )
	{
	  backupHostname = argv[++i];
	}
      i++;
    }

  logFilename = NULL_DEV;
  clientLogFilename = NULL_DEV;

  SF_Nexus nexus(log,backupHostname);

  nexus.Log("Stellar Frontier Nexus v%s",SF_Client::VersionString());
  nexus.Log("Copyright (c) 1997-2002 Stardock Systems");
  nexus.Log("All rights reserved");
  nexus.Log("");

  nexus.GetBannerAdInfo("nexus.ad");

  nexus.Run();

  nexus.WaitForChildren();

  S_DebugMem_SetVerbose(TRUE);

  return(0);
}
