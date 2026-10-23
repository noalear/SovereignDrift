#define S_LIB_GUI
#define SF_MAIN

#include "sf.h"
#include "SF_UpgradeWindow.h"
#include "SF_Nexus.h"

#ifdef SF_EXPERIMENTAL
#  define NUMFILES 8
#else
#  define NUMFILES 10
#endif

#ifdef OS2
   char *files[NUMFILES] = {
#     ifdef SF_EXPERIMENTAL
        "..\\binos2\\frontier-x.exe",
#     else
        "..\\binos2\\frontier.exe",
        "..\\binos2\\server.exe",
        "..\\binos2\\spy.exe",
#     endif
      "..\\dat\\*.dat",
      "..\\dat\\*.sec",
      "..\\dat\\*.scn",
      "..\\wav\\*.wav",
      "..\\mid\\*.mid",
      "..\\bmp\\256\\*.bmp",
      "..\\bmp\\65536\\*.bmp"
 };
#endif
#ifdef WIN32
   char *files[NUMFILES] = {
#     ifdef SF_EXPERIMENTAL
        "..\\binwin\\frontier-x.exe",
#     else
        "..\\binwin\\frontier.exe",
        "..\\binwin\\server.exe",
        "..\\binwin\\spy.exe",
#     endif
      "..\\dat\\*.dat",
      "..\\dat\\*.sec",
      "..\\dat\\*.scn",
      "..\\wav\\*.wav",
      "..\\mid\\*.mid",
      "..\\bmp\\256\\*.bmp",
      "..\\bmp\\65536\\*.bmp"
 };
#endif
#ifdef UNIX
   char *files[NUMFILES] = {
#     ifdef SF_EXPERIMENTAL
        "../binlin/frontier-x.exe",
#     else
        "../binlin/frontier.exe",
        "../binlin/server.exe",
        "../binlin/spy.exe",
#     endif
      "../dat/*.dat",
      "../dat/*.sec",
      "../dat/*.scn",
      "../wav/*.wav",
      "../mid/*.mid",
      "../bmp/256/*.bmp",
      "../bmp/65536/*.bmp"
 };
#endif

SF_UpgradeWindow *window;


int
_CloseFunc(void *iarg) {
   S_Process process;
   process.Start(files[0],"-nexus");
   window->Close();
   return 0;
   }

int
main( int argc, char *argv[] )
 {

   S_DebugMem_SetVerbose(FALSE);
   char *hostAddress = NULL;
   if ( argc > 1 )
     {
       hostAddress = argv[1];
     }
   else
     {
#      ifdef SF_EXPERIMENTAL
         hostAddress = "sf-x.stardock.net";
#      else
         hostAddress = "sf.stardock.net";
#      endif
     }
   S_DebugMem_SetPageException();

   S_Root root(&argc,&argv);
   window = new SF_UpgradeWindow(&root);
   window->Show();
   window->StartThread(hostAddress,SF_NEXUS_PORT,files,NUMFILES,
                      _CloseFunc,NULL);
   root.Run();
   delete window;

   S_DebugMem_SetVerbose(TRUE);

   return(0);
   }


