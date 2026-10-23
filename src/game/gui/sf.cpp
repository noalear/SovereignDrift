#define S_LIB_GUI
#define SF_MAIN

#include "sf.h"
#include "SF_Globals.h"
#include "SF_ClientWindow.h"
#include "SF_Client.h"
#include "SF_PortCpp.h"
#include "SF_Externals.h"

int
main( int argc, char *argv[] ) {

   S_DebugMem_SetPageException();
   S_DebugMem_SetThrowException(FALSE);

   int log = FALSE;

   char *hostname = NULL;

   int i = 1;
   while ( i < argc )
     {
       if ( strcmp(argv[i],"-log") == 0 )
	 {
	   log = TRUE;
	 }
       else
	 {
	   hostname = argv[i];
	 }
       i++;
     }

   if ( !log )
     {
       logFilename = NULL_DEV;
       clientLogFilename = NULL_DEV;
     }

   logFile.open(logFilename);

   sf = new SF_Client(SF_CLIENT_DATA_ENGINE_DISPLAY,hostname,&argc,&argv);
   sf->Run();
   delete sf;

   S_DebugMem_SetVerbose(TRUE);
   return(0);
   }
