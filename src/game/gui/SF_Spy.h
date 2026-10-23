#ifndef SF_SPY_INCLUDED
#define SF_SPY_INCLUDED

class SF_Spy : public S_GWindow {
public:
   S_String nexusName;
   S_GFont lucida;

   S_Timer timer;

   int Init();
   SF_Spy();
   int Create(S_Root *,char *);
   int Destroy();
   ~SF_Spy();

   S_Font helv;

   S_GConsole status;
   S_GButton closeButton;
   S_GButton refreshButton;

   int maxHostnames;
   char **hostnames;
   S_Thread *nexusThread;

   int SetItemUnknown(int,char *);
   S_GListBox listbox;

   int HandleCreateMsg();
   int HandleSizeMsg();
   int HandleCloseMsg();
   };

int SF_Spy_RefreshButtonHandler(S_GButton *);
int SF_Spy_CloseButtonHandler(S_GButton *);
int SF_Spy_ListBoxHandler(S_GListBox *);

#endif
