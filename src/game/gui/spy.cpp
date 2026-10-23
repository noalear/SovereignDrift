#define S_LIB_GUI
#define SF_MAIN

#include "sf.h"
#include "SF_Spy.h"

int
main( int argc, char *argv[] ) {

   S_DebugMem_SetPageException();

   char *nexusAddress = NULL;
   if ( argc > 1 )
      nexusAddress = argv[1];
   else
      nexusAddress = "sf.stardock.net";

   S_Root root(&argc,&argv);
   SF_Spy spy;
   spy.Create(&root,nexusAddress);
   root.Run();

   S_DebugMem_SetVerbose(TRUE);

   return(0);
   }
