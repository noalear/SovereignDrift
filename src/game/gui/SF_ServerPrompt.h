#ifndef SF_SERVERPROMPT_INCLUDED
#define SF_SERVERPROMPT_INCLUDED

class SF_ServerPrompt : public S_GWindow {
public:

   int Init();
   SF_ServerPrompt();
   int Create(S_GWindow *);
   int Destroy();
   ~SF_ServerPrompt();

   S_GButton yesButton;
   S_GButton noButton;

   int HandleCreateMsg();
   int HandleSizeMsg();
   int HandleCloseMsg();
   };

int SF_ServerPrompt_YesButtonHandler(S_GButton *);
int SF_ServerPrompt_NoButtonHandler(S_GButton *);

#endif
