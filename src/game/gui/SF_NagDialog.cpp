#include "sf.h"
#include "SF_NagDialog.h"
#include "SF_Profile.h"
#include "SF_Globals.h"
#include "SF_ClientWindow.h"
#include "SF_Client.h"
#include "SF_Externals.h"

SF_NagDialog::SF_NagDialog(S_Object *iparent) :
   S_Window(NULL,iparent,SF_REGISTERED_CLUB,NULL,0,
            _root->cx/2-190,_root->cy/2-190,
	    380,380,S_WINDOW_DIALOG ) {
   HandleCreateMsg();
   created = TRUE;
   }

int
NoRegButtonHandler(S_Button *b) {
   b->parent->Close();
   return 0;
   }

int
RegButtonHandler(S_Button *b) {
   SF_NagDialog *nd = (SF_NagDialog *)b->parent;
   int rc = sf->profile->SetRegCode(nd->regKeyTextBox1->GetString(),
                                    nd->regKeyTextBox2->GetString());

   if ( rc == SF_PROFILE_OK ) {
      sf->demo = FALSE;
      sf->oldAC = FALSE;
      b->parent->Close();
      }
   else if ( rc == SF_PROFILE_OLDREGCODE )
     {
      S_MsgBox(sf->mainWindow,
               "Registration",
               "The code you entered is for pre 1.02 versions of \n"
               "Stellar Frontier.  Visit http://www.stardock.com/support\n"
	       "to get your new AC serial number.",
               S_MSGBOX_OK);
       sf->demo = TRUE;
       sf->oldAC = TRUE;
     }
   else {
      S_MsgBox(sf->mainWindow,
               "Registration",
               "The code you entered is not valid.",
               S_MSGBOX_OK);
      sf->demo = TRUE;
      sf->oldAC = FALSE;
      }

   return 0;
   }


int
StardockButtonHandler(S_Button *b) {
   _OsOpenURL("http://www.stardock.com/products/sf/admiral.html");
   b->parent->Close();
   return 0;
   }


int
OrderFormButtonHandler(S_Button *b) {
   _OsOpenURL("http://www.stardock.com/order.html");
   b->parent->Close();
   return 0;
   }


int
SF_NagDialog::HandleCreateMsg() {
   msgText = new S_StaticText(this);
   regGroupBox = new S_GroupBox(this,SF_REGISTERED_CLUB);
   regStaticText = new S_StaticText(this);
   regKeyStaticText1 = new S_StaticText(this,"SF - ");
   regKeyStaticText2 = new S_StaticText(this," - ");
   regKeyTextBox1 = new S_TextBox(this);
   regKeyTextBox2 = new S_TextBox(this);
   regButton = new S_Button(this,RegButtonHandler,"[Enter]");
   stardockButton = new S_Button(this,StardockButtonHandler,
                                 "[Join "SF_REGISTERED_CLUB" via Internet]");
   //orderFormButton = new S_Button(this,OrderFormButtonHandler,
   //                              "");
   orderFormButton = NULL;
   noregButton = new S_Button(this,NoRegButtonHandler,
			      "[Play Game without "SF_REGISTERED_CLUB"]");

   msgText->SetText("If you find you like Stellar Frontier and would like to "
                    "support its continued development, you can join the "
                    ""SF_REGISTERED_CLUB".  This helps support our development "
                    "efforts and ensures that you have early access to new "
                    "Stellar Frontier features as well as additional bonus "
                    "ships and weapons as part of the "SF_REGISTERED_CLUB".\n\n"
                    "Bonus Ships: [Starbase,Marauder,BattleCruiser]\n"
                    "Bonus Weapons: [Lasers,Ram]\n\n"
                    "Click on the Join "SF_REGISTERED_CLUB" button below to find "
                    "out how to join the "SF_REGISTERED_CLUB".");

   regStaticText->SetText("Enter your "SF_REGISTERED_CLUB" identification number "
                          "below.");
   HandleSizeMsg();
   return 0;
   }


int
SF_NagDialog::HandleSizeMsg() {
   int margin = 10;
   int cxMsg = cxCanvas-2*margin;
   int cyMsg = 160;

   //int cxButton = 150;
   int cyButton = 30;

   int cxRegGroupBox = cxMsg;
   int cyRegGroupBox = 100;

   int x0 = margin;
   int y0 = margin;

   msgText->SetSize(cxMsg,cyMsg);
   msgText->SetPosition(x0,y0);

   y0 += cyMsg + margin;
   regGroupBox->SetSize(cxRegGroupBox,cyRegGroupBox);
   regGroupBox->SetPosition(x0,y0);

   x0 += 10;
   y0 += 20;

   int cxRegText = cxCanvas - 4*margin;
   int cyRegText = 48;
   regStaticText->SetSize(cxRegText,cyRegText);
   regStaticText->SetPosition(x0,y0);

   int cxText1 = 20;
   int cxText2 = 10;
   int cyText = 20;
   int cxTextBox1 = 50;
   int cxTextBox2 = 100;
   int cyTextBox = 20;

   y0 = margin + cyMsg + cyRegGroupBox - cyTextBox;

   regKeyStaticText1->SetSize(cxText1,cyText);
   regKeyStaticText1->SetPosition(x0,y0);

   x0 += cxText1 + 5;
   regKeyTextBox1->SetSize(cxTextBox1,cyTextBox);
   regKeyTextBox1->SetPosition(x0,y0);

   x0 += cxTextBox1;
   regKeyStaticText2->SetSize(cxText2,cyText);
   regKeyStaticText2->SetPosition(x0,y0);

   x0 += cxText2;
   regKeyTextBox2->SetSize(cxTextBox2,cyTextBox);
   regKeyTextBox2->SetPosition(x0,y0);

   x0 += cxTextBox2 + 5;
   regButton->SetSize(cxTextBox1,cyTextBox);
   regButton->SetPosition(x0,y0);

   x0 = margin;
   y0 = cyCanvas - margin - 2*cyButton;
   stardockButton->SetSize(cxCanvas-2*margin,cyButton);
   stardockButton->SetPosition(margin,y0);

   //y0 += cyButton;
   //orderFormButton->SetSize(cxCanvas-2*margin,cyButton);
   //orderFormButton->SetPosition(margin,y0);

   y0 += cyButton;
   noregButton->SetSize(cxCanvas-2*margin,cyButton);
   noregButton->SetPosition(margin,y0);

   return 0;
   }


int
SF_NagDialog::HandlePaintMsg() {
   DrawFilledRect(0,0,cxCanvas,cyCanvas,backGroundRGB);
   return 0;
   }


int
SF_NagDialog::HandleDestroyMsg() {
   if ( msgText ) {
      delete msgText;
      msgText = NULL;
      }
   if ( regGroupBox ) {
      delete regGroupBox;
      regGroupBox = NULL;
      }
   if ( regStaticText ) {
      delete regStaticText;
      regStaticText = NULL;
      }
   if ( regKeyTextBox1 ) {
      delete regKeyTextBox1;
      regKeyTextBox1 = NULL;
      }
   if ( regKeyTextBox2 ) {
      delete regKeyTextBox2;
      regKeyTextBox2 = NULL;
      }
   if ( regKeyStaticText1 ) {
      delete regKeyStaticText1;
      regKeyStaticText1 = NULL;
      }
   if ( regKeyStaticText2 ) {
      delete regKeyStaticText2;
      regKeyStaticText2 = NULL;
      }
   if ( regButton ) {
      delete regButton;
      regButton = NULL;
      }
   if ( noregButton ) {
      delete noregButton;
      noregButton = NULL;
      }
   if ( stardockButton ) {
      delete stardockButton;
      stardockButton = NULL;
      }
   if ( orderFormButton ) {
      delete orderFormButton;
      orderFormButton = NULL;
      }
   return 0;
   }


SF_NagDialog::~SF_NagDialog() {
   }
