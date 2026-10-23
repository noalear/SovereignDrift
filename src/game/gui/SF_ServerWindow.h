#ifndef SF_SERVERWINDOW_INCLUDED
#define SF_SERVERWINDOW_INCLUDED

#include "SF_ServerPrompt.h"

class SF_Server;

class SF_ServerWindow : public S_GWindow {
public:
   int margin;
   int x;
   int y;

   S_Mutex mutex;

   int unload;

   SF_ServerPrompt prompt;

   int cxDiagTextBox;
   int cyDiagTextBox;
   int cxControlPanel;
   int cyControlPanel;
   int cxPlayerList;
   int cyPlayerList;
   int cxConsole;
   int cyConsole;

   S_DirList *dirList;

   SF_Server *server;

   int update;
   S_Thread *updateThread;

   int SetDiagString(const char *,...);
   char diagString[256];

   S_SubMenu trayMenu;
   S_MenuItem titleMenuItem;
   S_MenuItem showMenuItem;
   S_MenuItem hideMenuItem;
   S_MenuItem exitMenuItem;

   S_Font helv;

   S_GFont lucida;
   S_GButton closeButton;
   S_GButton minimizeButton;
   S_GListBox scenarioList;
   S_GListBox playerList;
   S_GConsole console;
   S_GTextBox textbox;
   S_GButton startButton;
   S_GButton warnButton;
   S_GButton kickButton;
   S_GButton stopButton;
   S_GButton nexusCheckBox;
   S_GButton fps11CheckBox;
   S_GButton fps16CheckBox;
   S_GButton lagCheckBox;
   S_GTextBox lagTextBox;
   S_GTextBox portTextBox;

   S_TextBox *diagTextBox;

   S_ComboBox *frameRateComboBox;
   S_ComboBox *scenarioComboBox;
   S_TextBox *nexusTextBox;

   int RefreshPlayerList(int=-1,char * =NULL);

   int Init();
   SF_ServerWindow();
   int Create(S_Root *,SF_Server *);
   int Destroy();
   ~SF_ServerWindow();

   int HandleSystemTrayMouseMsg();

   int HandleCreateMsg();
   int HandleCloseMsg();
   int HandleDestroyMsg();
   };

int SF_ServerWindow_CloseMenuItemHandler(S_MenuItem *);
int SF_ServerWindow_ShowMenuItemHandler(S_MenuItem *);
int SF_ServerWindow_HideMenuItemHandler(S_MenuItem *);

int SF_Server_TextBoxHandler(S_GTextBox *);
int SF_Server_CloseButtonHandler(S_GButton *);
int SF_Server_MinimizeButtonHandler(S_GButton *);
int SF_Server_StartButtonHandler(S_GButton *);
int SF_Server_StopButtonHandler(S_GButton *);
int SF_Server_WarnButtonHandler(S_GButton *);
int SF_Server_KickButtonHandler(S_GButton *);
int SF_Server_NexusCheckBoxHandler(S_GButton *);
int SF_Server_11fpsCheckBoxHandler(S_GButton *);
int SF_Server_16fpsCheckBoxHandler(S_GButton *);
int SF_Server_LagCheckBoxHandler(S_GButton *);
int SF_Server_PlayerListBoxHandler(S_GListBox *);

#endif
