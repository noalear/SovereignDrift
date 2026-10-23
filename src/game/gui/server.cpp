#define S_LIB_GUI
#define SF_MAIN

#include "sf.h"
#include "SF_ClientWindow.h"
#include "SF_Globals.h"
#include "SF_Client.h"
#include "SF_Server.h"
#include "SF_PortCpp.h"
#include "SF_Externals.h"

S_StringArray list;

int startInTray = FALSE;
int port = SF_SERVER_PORT;
int logout = S_LOGGER_TOFILE | S_LOGGER_TOCONSOLE | S_LOGGER_TIMESTAMP;
int timeOutSec = SF_TIMEOUT_SEC;
int maxGameStateMsgs = 0;
int minGameStateMsgPeriodicitySec = SF_MINF9PERIOD_SEC;
int maxLag = SF_SERVER_MAXLAGMS;
char *autoStart = NULL;
char *nexusName = "sf.stardock.net";
char *altNexusName = "sf-x.stardock.net";

int ParseCommandLine(int,char *[]);
int ParseConfigFile(char *);

int
main( int argc, char *argv[] ) {

   S_DebugMem_SetPageException();
   S_DebugMem_SetThrowException(FALSE);

   ParseConfigFile("server.conf");
   ParseCommandLine(argc,argv);

   if ( !(logout & S_LOGGER_TOFILE) )
     {
       logFilename = NULL_DEV;
       clientLogFilename = NULL_DEV;
     }

   logFile.open(logFilename);

   SF_Server server;
#  if defined(WIN32) || defined(OS2)
      S_Root root(&argc,&argv);
      server.Create(nexusName,SF_SERVER_GUI,autoStart,port,logout,altNexusName,startInTray);
#  endif
#  ifdef UNIX
      server.Create(nexusName,SF_SERVER_NOGUI,autoStart,port,logout,altNexusName,startInTray);
#  endif
      server.lagMax = maxLag;
   server.timeOutSec = timeOutSec;
   server.maxGameStateMsgs = maxGameStateMsgs;
   server.minGameStateMsgPeriodicitySec = minGameStateMsgPeriodicitySec;
   server.Run();

   S_DebugMem_SetVerbose(TRUE);

   return(0);
   }


int
ParseConfigFile(char *filename)
{

  if ( !filename )
    {
      return 0;
    }

  FILE *fptr = fopen(filename,"r");
  if ( !fptr )
    {
      return 0;
    }

  list.Destroy();

  list.AppendLine("server");

  int i;
  char *rc;
  char temp[1024];
  do 
    {
      rc = fgets(temp,1023,fptr);
      if ( rc )
	{

	  //
	  // Cut string off at first occurence of comment or end line chars
	  //
	  int len = strlen(temp);
	  for ( i=0; i<len; i++ )
	    {
	      if ( temp[i] == '#' || temp[i] == 10 || temp[i] == 13 )
		{
		  temp[i] = 0;
		}
	    }

	  //
	  // Create string array containing command line params
	  //
	  if ( temp[0] != 0 )
	    {
	      S_TokenArray tokens(temp);
	      for ( i=0; i<tokens.numLines; i++ )
		{
		  list.AppendLine(tokens.lines[i]);
		}
	    }
	}
    } while ( rc != NULL );
  
  fclose(fptr);

  int argc = list.numLines;
  char **argv = (char **)S_malloc(argc*sizeof(char *));
  if ( argv )
    {
      for ( i=0; i<argc; i++ )
	{
	  argv[i] = list.lines[i];
	}
      ParseCommandLine(argc,argv);
      S_free(argv);
    }

  return 0;
}


int
ParseCommandLine(int argc,char *argv[] )
{
  int i = 1;
  
  while ( i < argc )
    {
      if ( strcmp(argv[i],"-logscreen") == 0 )
	{
	  logout = S_LOGGER_TOCONSOLE | S_LOGGER_TIMESTAMP;
	}
      else if ( strcmp(argv[i],"-logfile") == 0 )
	{
	  logout = S_LOGGER_TOFILE | S_LOGGER_TIMESTAMP;
	}
      else if ( strcmp(argv[i],"-logall") == 0 )
	{
	  logout = S_LOGGER_TOFILE | S_LOGGER_TOCONSOLE | S_LOGGER_TIMESTAMP;
	}
      else if ( strcmp(argv[i],"-lognone") == 0 )
{
	  logout = S_LOGGER_TIMESTAMP;
	}
      else if ( strcmp(argv[i],"-autostart") == 0 )
	{
	  autoStart = argv[++i];
	}
      else if ( strcmp(argv[i],"-startintray") == 0 )
	{
	  startInTray = TRUE;
	}
      else if ( strcmp(argv[i],"-framerate") == 0 && argv[i+1] )
	{
	  frameRate = atoi(argv[++i]);
	}
      else if ( strcmp(argv[i],"-updaterate") == 0 && argv[i+1] )
	{
	  shipUpdateRate = atoi(argv[++i]);
	}
      else if ( strcmp(argv[i],"-aiupdaterate") == 0 && argv[i+1] )
	{
	  aishipUpdateRate = atoi(argv[++i]);
	}
      else if ( strcmp(argv[i],"-timeoutsec") == 0 && argv[i+1] )
	{
	  timeOutSec = atoi(argv[++i]);
	}
      else if ( strcmp(argv[i],"-minF9period") == 0 && argv[i+1] )
	{
	  minGameStateMsgPeriodicitySec = atoi(argv[++i]);
	}
      else if ( strcmp(argv[i],"-maxlag") == 0 && argv[i+1] )
	{
	  maxLag = atoi(argv[++i]);
	}
      else if ( strcmp(argv[i],"-maxF9") == 0 && argv[i+1] )
	{
	  maxGameStateMsgs = atoi(argv[++i]);
	}
      else if ( strcmp(argv[i],"-port") == 0 && argv[i+1] )
	{
	  port = atoi(argv[++i]);
	}
      else if ( strcmp(argv[i],"-altnexus") == 0 && argv[i+1] )
	{
	  altNexusName = argv[++i];
	}
      else if ( strcmp(argv[i],"-nexus") == 0 && argv[i+1] )
	{
	  nexusName = argv[++i];
	}
      else if ( argv[i] && argv[i][0] )
	{
	  nexusName = argv[i];
	}
      i++;
    }
  return 0;
}


