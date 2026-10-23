/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
#ifndef SF_CLIENT_INCLUDED
#define SF_CLIENT_INCLUDED

#define SF_CLIENT_DATA                0
#define SF_CLIENT_DATA_ENGINE         1
#define SF_CLIENT_DATA_ENGINE_DISPLAY 2

#define SF_CLIENT_TIMEOUT 10000000
#define SF_CLIENT_NUMLOOPS 2
#define SF_CLIENT_NUMBYTES 50

#define SF_CLIENT_DISCONNECTED       0
#define SF_CLIENT_CONNECTED          1
#define SF_CLIENT_CONNECTING         2
#define SF_CLIENT_DISCONNECTING      3
#define SF_CLIENT_SINGLEPLAYER       4
#define SF_CLIENT_NOSTATE            5
#define SF_CLIENT_DENIEDBYSERVER     6
#define SF_CLIENT_WRONGSERVERVERSION 7

#define MAXSERVERS 64

#define SF_CLIENT_MAXNICKLEN 30
#define SF_CLIENT_NICKCHARS  "1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz_-[]{}^|"

class SF_Engine;
class SF_SpriteList;
class SF_MidiList;
class SF_WaveList;
class SF_Profile;
class SF_TextBuffer;
class SF_MsgBox;
class SF_Player;
class SF_NagDialog;
class SF_UpgradeWindow;
class SF_Server;

typedef struct {
   char serverString[128];
   char hostName[128];
   char serverName[128];
   char osString[128];
   char majorVersion;
   char minorVersion;
   char minorminorVersion;
   char versionModifier[128];
   char versionString[128];
   short numPlayers;
   short maxPlayers;
   short numHumanPlayers;
   char upTimeStr[128];
   char playerList[8092];
   short port;
   } ServerDataStruct;
  
class SF_Client : public S_Client,public SF_Globals {
public:
   int mode;
   char *hostname;
   int serverPort;

   int rcEngine;
   SF_ClientWindow window;

   S_Mutex mutex;

   S_Root *root;
   S_Font *font;
   S_StdWindow *mainWindow;
   S_CanvasBuffer *backBuffer;
   SF_Engine *engine;

   S_AudioDevice *audio;
   S_WaveData *introWav;

   int cxHalftone;
   int cyHalftone;
   int cxVideoConsole;
   int cyVideoConsole;
   int cxMapConsole;
   int cyMapConsole;
   int cyPanel;
   int cxPanel;
   int margin;
   int cyConsoleButtons;
   int cyChatMin;

   int tabToggle;

   int dy1;
   int dy2;

   int runCredits;
   int y0Credits;
   int DrawCredits();

   int demo;
   int oldAC;
   int connectedThroughNexus;

   int shiftDown;
   int ctrlDown;

   int ResetScrollingText();
   int AddHighScoresTo(SF_TextBuffer *);
   int AddCreditsTo(SF_TextBuffer *);
   SF_TextBuffer *scrollingText;

   int PlayerExistsOnNexus(char *,char *);
   int CreateNewPlayerOnNexus(char *,char *,char *,char *);
   int SetRecordDataOnNexus(char *,char *,char *,S_Packet *);
   int SetPlayerDataOnNexus(char *,SF_Player *);
   int GetPlayerDataFromNexus(char *,SF_Player *);
   int GetLeaderBoardDataFromNexus(char *,slong,char *,S_Packet *);

   int mouseMenuEnabled;
   int mouseMenuItem;
   int InSplash(int,int);
   int DrawSplash();
   int DrawMenuItem(int,S_SystemBitmap *);
   int splashThreadKeepRunning;
   S_Thread *splashThread;
   S_SystemBitmap *bmpSplash;
   S_SystemBitmap *bmpSF1Splash;
   S_SystemBitmap *bmpSF2Splash;
   S_SystemBitmap *bmpSDSSplash;

   S_WaveData *menuPing;

   SF_SpriteList *msl;  /* *** Master sprite list *** */
   SF_MidiList *mml;
   SF_WaveList *mwl;

   int numFiles;
   char **files;
   int numwFiles;
   char **wfiles;
   SF_UpgradeWindow *downloadDialog;
   SF_UpgradeWindow *wdownloadDialog;
   SF_NagDialog *nagDialog;

   SF_MsgBox *msgBox;
   SF_Profile *profile;

   SF_Client(int=SF_CLIENT_DATA,char * =NULL,int * =NULL,char *** =NULL);
   ~SF_Client();

   int serverIndex;
   ServerDataStruct serverList[MAXSERVERS];
   int GetInternetServerList(char *);

   S_SystemBitmap *bmpBanner;
   char *bmpBannerFilename;
   char *urlBanner;

   static char *VersionString(int=SF_MAJOR_VERSION,int=SF_MINOR_VERSION,
                       int=SF_MINOR_MINOR_VERSION,char * =SF_VERSION_MODIFIER,
                       char * =SF_VERSION_STRING);

   uchar nexusMajorVersion;
   uchar nexusMinorVersion;
   uchar nexusMinorMinorVersion;
   char  nexusVersionModifier[128];
   char  nexusVersionString[128];
   int needToUpgrade;

   int StartEngine(char * =NULL,char * =NULL);
   int WaitForEngine();
   int StopEngine(int=FALSE);

   int Run(char * =NULL);

   /* *** Network engine stuff *** */
   S_String touchString;
   char *TouchServer(ServerDataStruct *,ushort=SF_CLIENT_NUMLOOPS,
                     ushort=SF_CLIENT_NUMBYTES);

   /* *** Menubar items *** */
   S_MenuBar *menuBar;
    S_SubMenu *gameMenu;
     S_MenuItem *stardockNetMenuItem;
     S_MenuItem *internetMenuItem;
     S_MenuItem *intranetMenuItem;
     S_MenuItem *disconnectMenuItem;
     S_MenuItem *singlePlayerMenuItem;
     S_MenuItem *pauseMenuItem;
     S_MenuItem *stopMenuItem;
     S_MenuItem *optionsMenuItem;
     S_MenuItem *exitMenuItem;
   };

class SF_LoadScenarioDialog : public S_Window {
public:

   S_DirList *dirList;

   S_TextEdit *descriptionTextEdit;
   S_ListBox *scenarioListBox;
   S_Button *okButton;
   S_Button *cancelButton;

   int HandleCreateMsg();
   int HandlePaintMsg();
   int HandleCloseMsg();

   SF_LoadScenarioDialog(S_Window *);
   ~SF_LoadScenarioDialog();
   };

int FilterString(char *);

int DrawLoginStatusBar(int=0);
int SetLoginStatusText(char *);
int CloseLoginDialog(SF_Client *);
int EnableLoginDialogButtons(int=TRUE);
int EnableLoginDialog(SF_Client *,int,int,int);
int LaunchLoginDialog(S_Window *);
int SndCueWav(int,int,int=-1);

#endif
