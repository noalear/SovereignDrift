class SF_PlayerManagerWindow;
class SF_PlayerStatsWindow;
class SF_FleetManagerWindow;
class SF_FleetStatsWindow;

class SF_NexusClient : public S_Window {
public:
   char *nexusName;

   int cxButton;
   int cxWideButton;
   int cyButton;

   int cyPlayerManagerButton;
   int cyPlayerStatsButton;
   int cyFleetManagerButton;
   int cyFleetStatsButton;

   SF_PlayerManagerWindow *playerManagerWindow;
   SF_PlayerStatsWindow *playerStatsWindow;
   /*
   SF_FleetManagerWindow *fleetManagerWindow;
   SF_FleetStatsWindow *fleetStatsWindow;
   */

   S_Button *okButton;

   S_Button *playerManagerButton;
   S_Button *playerStatsButton;
   S_Button *fleetManagerButton;
   S_Button *fleetStatsButton;

   int HandleCreateMsg();
   int HandleSizeMsg();
   int HandlePaintMsg();
   int HandleCloseMsg();

   SF_NexusClient(S_Object *,char *);
   ~SF_NexusClient();
   };

class SF_PlayerManagerWindow : public S_Window {
public:
   char *nexusName;

   int cxButton;
   int cyButton;
   int cxListBox;
   int cyListBox;
   int cxStatsListBox;
   S_ListBox *playerListBox;
   S_TextEdit *playerStatsTextEdit;

   int action;
   int runThread;
   S_Event playerManagerEvent;
   S_Thread playerManagerThread;

   S_Button *refreshButton;
   S_Button *viewButton;

   int HandleCreateMsg();
   int HandleSizeMsg();
   int HandlePaintMsg();
   int HandleCloseMsg();

   SF_PlayerManagerWindow(S_Object *,char *);
   ~SF_PlayerManagerWindow();
   };

class SF_PlayerStatsWindow : public S_Window {
public:
   char *nexusName;

   int cxButton;
   int cyButton;

   int runThread;
   S_Event playerStatsEvent;
   S_Thread playerStatsThread;

   S_ScrollBar *scrollBar;

   S_Button *refreshButton;

   int numHeaderButtons;
   S_Button **headerButtons;

   int HandleCreateMsg();
   int HandleSizeMsg();
   int HandlePaintMsg();
   int HandleCloseMsg();

   SF_PlayerStatsWindow(S_Object *,char *);
   ~SF_PlayerStatsWindow();
   };

int SF_PlayerStatsWindow_Refresh(S_Button *);

int SF_PlayerManagerWindow_PlayerListBox(S_ListBox *);
int SF_PlayerManagerWindow_Refresh(S_Button *);
int SF_PlayerManagerWindow_View(S_Button *);

int SF_NexusClient_OkButtonHandler(S_Button *);
int SF_NexusClient_PlayerManagerHandler(S_Button *);
int SF_NexusClient_PlayerStatsHandler(S_Button *);
int SF_NexusClient_FleetManagerHandler(S_Button *);
int SF_NexusClient_FleetStatsHandler(S_Button *);

