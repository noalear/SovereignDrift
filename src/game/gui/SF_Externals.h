/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
#define MAX_COLORS 16

class SF_Client;
class SF_Server;
class SF_VideoConsole;
class GameStateClass;

#define SF_YELLOW  0xFFFF00L
#define SF_CYAN    0x00FFFFL
#define SF_RED     0xFF5555L
#define SF_GREEN   0x00FF00L
#define SF_BLUE    0x5B5BFFL
#define SF_PINK    0xFF3499L
#define SF_ORANGE  0xFF9934L
#define SF_SPRING  0xCCFF34L
#define SF_MAGENTA 0xFF34FFL
#define SF_WHITE   0xFFFFFFL
#define SF_CORN    0xFFFF99L
#define SF_ROSE    0xFFCCCCL
#define SF_VIOLET  0xFF99FFL
#define SF_TEAL    0x99FFCCL
#define SF_AZURE   0x99CCFFL
#define SF_GREY    0x999999L

extern ofstream logFile;

#ifdef SF_MAIN
   SF_Client *sf = NULL;
   SF_Server *ss = NULL;

   int mstate = 0;

   /* ************ Externals required by old code on portCPP *************** */
   slong frameNumber;
   slong raceColor[MAX_COLORS] = {
      SF_YELLOW,SF_CYAN,SF_RED,SF_GREEN,SF_BLUE,SF_PINK,SF_ORANGE,SF_SPRING,
      SF_MAGENTA,SF_WHITE,SF_CORN,SF_ROSE,SF_VIOLET,SF_TEAL,SF_AZURE,SF_GREY
      };
   slong raceBlipColor[MAX_COLORS] = {
      SF_YELLOW,SF_CYAN,SF_RED,SF_GREEN,SF_BLUE,SF_PINK,SF_ORANGE,SF_SPRING,
      SF_MAGENTA,SF_WHITE,SF_CORN,SF_ROSE,SF_VIOLET,SF_TEAL,SF_AZURE,SF_GREY
      };
   slong raceTextColor[MAX_COLORS] = {
      SF_YELLOW,SF_CYAN,SF_RED,SF_GREEN,SF_BLUE,SF_PINK,SF_ORANGE,SF_SPRING,
      SF_MAGENTA,SF_WHITE,SF_CORN,SF_ROSE,SF_VIOLET,SF_TEAL,SF_AZURE,SF_GREY
      };
   SF_VideoConsole *console = NULL;
   short localSlotNumber = 0;
   char frameRate = SF_DEFAULT_FRAMERATE;
   char shipUpdateRate = SF_DEFAULT_FRAMERATE;
   char aishipUpdateRate = SF_DEFAULT_FRAMERATE;

   char *devBootMsg = "DEV Kicked You";

   char *logFilename = "frontier.log";
   char *clientLogFilename = "client.log";

#  if defined(OS2) || defined(WIN32)
      char *defaultGameStateFilename = "..\\dat\\server.dat";
      char *bitmapsFilename = "..\\dat\\bitmaps";
      char *SF_ScenarioFileDirMask = "..\\dat\\*.scn";
      char *SF_ServerFileDirMask = "..\\dat\\*.dat"; 
      char *SF_MidiFileDirMask = "..\\mid\\*.mid";
      char *SF_WaveFileDir = "..\\wav\\";
      char *SF_BmpFileDir = "..\\bmp\\";
#  else
      char *defaultGameStateFilename = "../dat/server.dat";
      char *bitmapsFilename = "../dat/bitmaps";
      char *SF_ScenarioFileDirMask = "../dat/*.scn";
      char *SF_ServerFileDirMask = "../dat/*.dat";
      char *SF_MidiFileDirMask = "../mid/*.mid";
      char *SF_WaveFileDir = "../wav/";
      char *SF_BmpFileDir = "../bmp/";
#  endif

   /* ********************************************************************** */
#else

   extern int mstate;

   extern SF_Client *sf;
   extern SF_Server *ss;

   extern slong frameNumber;
   extern GameStateClass *game;
   extern slong raceColor[MAX_COLORS];
   extern slong raceTextColor[MAX_COLORS];
   extern slong raceBlipColor[MAX_COLORS];
   extern SF_VideoConsole *console;
   extern short localSlotNumber;
   extern char frameRate;
   extern char shipUpdateRate;
   extern char aishipUpdateRate;
   extern char *logFilename;

   extern char *demoString;
   extern char *defaultGameStateFilename;
   extern char *bitmapsFilename;
   extern char *SF_ScenarioFileDirMask;
   extern char *SF_ServerFileDirMask;
   extern char *SF_MidiFileDirMask;
   extern char *SF_WaveFileDir;
   extern char *SF_BmpFileDir;

   extern char *devBootMsg;
#endif
