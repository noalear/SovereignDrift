#include "sf.h"
#include "SF_MsgBox.h"
#include "SF_Globals.h"
#include "SF_ClientWindow.h"
#include "SF_Client.h"
#include "SF_Externals.h"

SF_MsgBox::SF_MsgBox(S_Object *iparent) :
   S_Window( NULL,iparent,NULL,NULL,0,
             _root->cx/2-100,_root->cy/2-75,
             200,170,
             S_WINDOW_DIALOG,NULL) {
   handler = NULL;
   arg = NULL;
   HandleCreateMsg();
   created = TRUE;
   }


int
SF_MsgBox::Hide() {
   S_Object::Hide();
   if ( sf )
      sf->mainWindow->Focus();
   return 0;
   }


int
SF_MsgBox::Show(char *header,char *msg,int (*ihandler)(void *),void *iarg) {
   HandleSizeMsg();
   if ( msg )
      msgText->SetText(msg);
   if ( header )
      SetText(header);
   S_Object::Show();
   if ( sf )
      sf->mainWindow->Focus();
   handler = ihandler;
   arg = iarg;
   if ( !handler )
      okButton->Hide();
   else
      okButton->Show();
   return 0;
   }


int
SF_MsgBox_OkButtonHandler(S_Button *b) {
   if ( b->parent ) {
      SF_MsgBox *mb = (SF_MsgBox *)b->parent;
      if ( mb->handler )
         mb->handler(mb->arg);
      mb->Hide();
      }
   return 0;
   }


int
SF_MsgBox::HandleCreateMsg() {
   msgText = new S_StaticText(this);
   okButton = new S_Button(this,SF_MsgBox_OkButtonHandler,"Ok");
   return 0;
   }


int
SF_MsgBox::HandleSizeMsg() {
   int margin = 10;
   int cxButton = 35;
   int cyButton = 25;
   int cyText = cyCanvas - 3*margin - cyButton;
   int cxText = cxCanvas - 2*margin;

   msgText->SetSize(cxText,cyText);
   msgText->SetPosition(margin,margin);

   okButton->SetSize(cxButton,cyButton);
   okButton->SetPosition(cxCanvas/2-cxButton/2,cyCanvas-margin-cyButton);

   return 0;
   }


int
SF_MsgBox::HandlePaintMsg() {
   DrawFilledRect(0,0,cxClient,cyClient,backGroundRGB);
   return 0;
   }


int
SF_MsgBox::HandleCloseMsg() {
   return 0;
   }


SF_MsgBox::~SF_MsgBox() {
   if ( okButton ) {
      delete okButton;
      okButton = NULL;
      }
   if ( msgText ) {
      delete msgText;
      msgText = NULL;
      }
   }

