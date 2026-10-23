#include "sf.h"
#include "SF_ServerPrompt.h"
#include "SF_ServerWindow.h"


int
SF_ServerPrompt::Init() {
   return 0;
   }

SF_ServerPrompt::SF_ServerPrompt() {
   Init();
   S_GWindow::Init();
   }


int
SF_ServerPrompt::Create( S_GWindow *iparent ) {
   S_GWindow::Create( NULL,iparent,"SFServerPrompt","blue_prompt.bmp",1,
                      iparent->root->cx/2-192,
		      iparent->root->cy/2-68,
                      384,136 );
   return 0;
   }


int
SF_ServerPrompt::HandleCreateMsg() {

  int margin = 10;
  int cxButton = 64;
  int cyButton = 25;

   yesButton.Create(this,SF_ServerPrompt_YesButtonHandler,
		    "blue_yes.bmp",
		    cxCanvas/2-margin/2-cxButton,
		    cyCanvas-margin-cyButton);

   noButton.Create(this,SF_ServerPrompt_NoButtonHandler,
		   "blue_no.bmp",
		   cxCanvas/2+margin/2,
		   cyCanvas-margin-cyButton);

   return 0;
   }


int
SF_ServerPrompt_YesButtonHandler(S_GButton *b) {
   SF_ServerPrompt *w = (SF_ServerPrompt *)b->parent;
   SF_ServerWindow *sw = (SF_ServerWindow *)w->parent;

   //
   // Close the server window.
   //
   sw->unload = TRUE;
   sw->Close();

   w->Hide();
   return 0;
   }


int
SF_ServerPrompt_NoButtonHandler(S_GButton *b) {
   SF_ServerPrompt *w = (SF_ServerPrompt *)b->parent;
   w->Hide();
   return 0;
   }


int
SF_ServerPrompt::HandleSizeMsg() {
   return 0;
   }


int
SF_ServerPrompt::HandleCloseMsg() {
   return 0;
   }


int
SF_ServerPrompt::Destroy() {
   S_GWindow::Destroy();
   return 0;
   }

SF_ServerPrompt::~SF_ServerPrompt() {
   }
