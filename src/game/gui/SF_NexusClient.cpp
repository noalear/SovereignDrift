/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
#include "sf.h"
#include "SF_Nexus.h"
#include "SF_NexusClient.h"
#include "SF_ClientWindow.h"
#include "SF_Globals.h"
#include "SF_Client.h"
#include "SF_Externals.h"

/* ************************************************************************* */
/* ***************************** SF_NexusClient **************************** */
/* ************************************************************************* */
SF_NexusClient::SF_NexusClient(S_Object *iparent,char *inexusName) :
   S_Window( NULL,iparent,"Stardock.net",NULL,0,
             _root->cx/2-320,_root->cy/2-240,640,480,
             S_WINDOW_DIALOG ) {
   nexusName = S_strdup(inexusName);
   HandleCreateMsg();
   created = TRUE;
   HandleSizeMsg();
   Show();
   }


int
SF_NexusClient::HandleCreateMsg() {
   okButton = new S_Button(this,SF_NexusClient_OkButtonHandler,"Ok");
   playerManagerButton = new S_Button(this,
                                      SF_NexusClient_PlayerManagerHandler,
                                      "Player Manager");

   playerStatsButton = NULL;
   //playerStatsButton = new S_Button(this,
   //                                 SF_NexusClient_PlayerStatsHandler,
   //                                 "Player Stats");

   fleetManagerButton = NULL;
   fleetStatsButton = NULL;
   //fleetManagerButton = new S_Button(this,
   //                                  SF_NexusClient_FleetManagerHandler,
   //                                  "Fleet Manager");
   //fleetStatsButton = new S_Button(this,
   //                                SF_NexusClient_FleetStatsHandler,
   //                                "Fleet Stats");

   playerManagerWindow = new SF_PlayerManagerWindow(this,nexusName);

   playerStatsWindow = NULL;
   //playerStatsWindow = new SF_PlayerStatsWindow(this,nexusName);

   cyPlayerManagerButton = 35;
   cyPlayerStatsButton = 30;
   cyFleetManagerButton = 30;
   cyFleetStatsButton = 30;

   return 0;
   }


int
SF_NexusClient_OkButtonHandler(S_Button *b) {
   b->parent->Close();
   return 0;
   }


int
SF_NexusClient_PlayerManagerHandler(S_Button *b) {
   SF_NexusClient *nc = (SF_NexusClient *)b->parent;
   if ( nc ) {
      nc->cyPlayerManagerButton = 35;
      nc->cyPlayerStatsButton = 30;
      nc->cyFleetManagerButton = 30;
      nc->cyFleetStatsButton = 30;
      nc->HandleSizeMsg();
      if ( nc->playerManagerWindow )
         nc->playerManagerWindow->Show();
      if ( nc->playerStatsWindow )
         nc->playerStatsWindow->Hide();
      }
   return 0;
   }


int
SF_NexusClient_PlayerStatsHandler(S_Button *b) {
   SF_NexusClient *nc = (SF_NexusClient *)b->parent;
   if ( nc ) {
      nc->cyPlayerManagerButton = 30;
      nc->cyPlayerStatsButton = 35;
      nc->cyFleetManagerButton = 30;
      nc->cyFleetStatsButton = 30;
      nc->HandleSizeMsg();
      if ( nc->playerStatsWindow )
         nc->playerStatsWindow->Show();
      if ( nc->playerManagerWindow )
         nc->playerManagerWindow->Hide();
      }
   return 0;
   }


int
SF_NexusClient_FleetManagerHandler(S_Button *b) {
   SF_NexusClient *nc = (SF_NexusClient *)b->parent;
   if ( nc ) {
      nc->cyPlayerManagerButton = 30;
      nc->cyPlayerStatsButton = 30;
      nc->cyFleetManagerButton = 35;
      nc->cyFleetStatsButton = 30;
      nc->HandleSizeMsg();
      if ( nc->playerManagerWindow )
         nc->playerManagerWindow->Hide();
      if ( nc->playerStatsWindow )
         nc->playerStatsWindow->Hide();
      }
   return 0;
   }


int
SF_NexusClient_FleetStatsHandler(S_Button *b) {
   SF_NexusClient *nc = (SF_NexusClient *)b->parent;
   if ( nc ) {
      nc->cyPlayerManagerButton = 30;
      nc->cyPlayerStatsButton = 30;
      nc->cyFleetManagerButton = 30;
      nc->cyFleetStatsButton = 35;
      nc->HandleSizeMsg();
      if ( nc->playerManagerWindow )
         nc->playerManagerWindow->Hide();
      if ( nc->playerStatsWindow )
         nc->playerStatsWindow->Hide();
      }
   return 0;
   }


int
SF_NexusClient::HandleSizeMsg() {
   int x = 10;
   int y = 10;
   cxWideButton = 120;
   cxButton = 70;
   cyButton = 35;

   y = 10 + 35 - cyPlayerManagerButton;
   if ( playerManagerButton ) {
      playerManagerButton->SetSize(cxWideButton,cyPlayerManagerButton);
      playerManagerButton->SetPosition(x,y);
      }
   y = 10 + 35 - cyPlayerStatsButton;
   x += cxWideButton;
   if ( playerStatsButton ) {
      playerStatsButton->SetSize(cxWideButton,cyPlayerStatsButton);
      playerStatsButton->SetPosition(x,y);
      }
   y = 10 + 35 - cyFleetManagerButton;
   x += cxWideButton;
   if ( fleetManagerButton ) {
      fleetManagerButton->SetSize(cxWideButton,cyFleetManagerButton);
      fleetManagerButton->SetPosition(x,y);
      }
   y = 10 + 35 - cyFleetStatsButton;
   x += cxWideButton;
   if ( fleetStatsButton ) {
      fleetStatsButton->SetSize(cxWideButton,cyFleetStatsButton);
      fleetStatsButton->SetPosition(x,y);
      }

   if ( okButton ) {
      okButton->SetSize(cxButton,cyButton);
      okButton->SetPosition(cxCanvas - 10 - cxButton,cyCanvas - 10 - cyButton);
      }

   if ( playerManagerWindow ) {
      playerManagerWindow->SetSize(cxCanvas-22,cyCanvas-32-2*cyButton);
      playerManagerWindow->SetPosition(11,11+cyButton);
      }
   if ( playerStatsWindow ) {
      playerStatsWindow->SetSize(cxCanvas-22,cyCanvas-32-2*cyButton);
      playerStatsWindow->SetPosition(11,11+cyButton);
      }

   return 0;
   }


int
SF_NexusClient::HandlePaintMsg() {
   DrawFilledRect(0,0,cxCanvas,cyCanvas,backGroundRGB);
   int x1 = 10;
   int y1 = 10 + cyButton;
   int x2 = cxCanvas - 10;
   int y2 = cyCanvas - 20 - cyButton;
   DrawLine(x1,y1,x1,y2,2,RGB_DARKGRAY);
   DrawLine(x1,y1,x2,y1,2,RGB_DARKGRAY);
   DrawLine(x2,y1,x2,y2,2,RGB_WHITE);
   DrawLine(x1,y2,x2,y2,2,RGB_WHITE);
   return 0;
   }


int
SF_NexusClient::HandleCloseMsg() {
   sf->mouseMenuEnabled = TRUE;
   if ( playerStatsWindow ) {
      playerStatsWindow->Delete();
      playerStatsWindow = NULL;
      }
   if ( playerManagerWindow ) {
      playerManagerWindow->Delete();
      playerManagerWindow = NULL;
      }
   if ( playerManagerButton ) {
      delete playerManagerButton;
      playerManagerButton = NULL;
      }
   if ( playerStatsButton ) {
      delete playerStatsButton;
      playerStatsButton = NULL;
      }
   if ( fleetManagerButton ) {
      delete fleetManagerButton;
      fleetManagerButton = NULL;
      }
   if ( fleetStatsButton ) {
      delete fleetStatsButton;
      fleetStatsButton = NULL;
      }
   if ( okButton ) {
      delete okButton;
      okButton = NULL;
      }
   return 0;
   }

SF_NexusClient::~SF_NexusClient() {
   if ( nexusName ) {
      S_free(nexusName);
      nexusName = NULL;
      }
   }
/* ************************************************************************* */


/* ************************************************************************* */
/* ************************* SF_PlayerManagerWindow ************************ */
/* ************************************************************************* */
int
SF_PlayerManagerThread(void *iarg) {
  SF_PlayerManagerWindow *pmw = (SF_PlayerManagerWindow *)iarg;

  if ( !pmw )
    {
      return 0;
    }

   do {
      int i;
      char str[1024];
      slong rc;
      S_Client c;
      S_Packet packet;

      rc = c.Connect(pmw->nexusName,SF_NEXUS_PORT,pmw->action);

      packet.Flush();
      if ( rc == SF_NEXUS_OK ) {
         switch(pmw->action) {

            case SF_NEXUS_GETPLAYERDATA2 :
               if ( pmw->playerListBox && pmw->playerListBox->GetIndex() < 0 )
                  break;
	       if ( pmw->playerListBox )
		 packet.PushString(pmw->playerListBox->GetItem());
               packet.PushString(" ");
               c.WritePacket(&packet);
               c.ReadLong(&rc);
               if ( rc == SF_NEXUS_OK ) {
                  packet.Flush();
                  c.ReadPacket(&packet);
                  char *keyword,*value;
		  if ( pmw->playerStatsTextEdit )
		    pmw->playerStatsTextEdit->Flush();
                  do {
                     keyword = packet.PopString();
                     value = packet.PopString();
                     if ( keyword && value ) {
                        char tmp[256];
                        if ( strlen(keyword) > 15 )
                           keyword[15] = 0;
                        sprintf(tmp,"%-15s %s",keyword,value);
			if ( pmw->playerStatsTextEdit )
			  pmw->playerStatsTextEdit->AppendLine(tmp);
                        S_free(keyword);
                        S_free(value);
                        }
                     } while ( keyword && value );
                  sprintf(str,"Stats for %s",pmw->playerListBox->GetItem());
		  if ( pmw->playerStatsTextEdit && 
		       pmw->playerStatsTextEdit->label )
                  pmw->playerStatsTextEdit->label->SetText(str);
	          }
	       if ( pmw->viewButton )
		 pmw->viewButton->SetEnabled(TRUE);
               break;

            case SF_NEXUS_GETPLAYERLIST :
               rc = c.ReadPacket(&packet);
               if ( packet.readComplete ) {
                  char *playerName;
                  i = 0;
                  do {
                     playerName = packet.PopString();
                     if ( playerName ) {
                        i++;
                        if ( pmw->playerListBox )
                           pmw->playerListBox->AddItem(playerName);
                        S_free(playerName);
                        }
	             } while ( playerName );
                  sprintf(str,"Players: %d",i);
                  if ( pmw->playerListBox && pmw->playerListBox->label )
		    pmw->playerListBox->label->SetText(str);
	          }
               break;

	    }

         c.Disconnect();
         }

      if ( pmw->refreshButton )
         pmw->refreshButton->SetEnabled(TRUE);

      if ( pmw->runThread ) {
         pmw->playerManagerEvent.Reset();
         pmw->playerManagerEvent.WaitFor();
         }
      } while (pmw->runThread);
   return 0;
   }


SF_PlayerManagerWindow::SF_PlayerManagerWindow(S_Object *iparent,
                                               char *inexusName) :
   S_Window(NULL,iparent,NULL,NULL,0,0,0,0,0,S_WINDOW_STATICCHILD) {

   nexusName = S_strdup(inexusName);

   HandleCreateMsg();
   created = TRUE;
   Show();
   }


int
SF_PlayerManagerWindow::HandleCreateMsg() {
   playerListBox = new S_ListBox(this,SF_PlayerManagerWindow_PlayerListBox);
   playerListBox->AddLabel("Players",150,16);

   playerStatsTextEdit = new S_TextEdit(this);
   playerStatsTextEdit->AddLabel("Stats",150,16);
   playerStatsTextEdit->SetFont(S_FONT_COURIER,8);
   playerStatsTextEdit->SetReadOnly(TRUE);

   refreshButton = new S_Button(this,SF_PlayerManagerWindow_Refresh,"Refresh");
   refreshButton->SetEnabled(FALSE);

   viewButton = new S_Button(this,SF_PlayerManagerWindow_View,"View");
   viewButton->SetEnabled(FALSE);

   action = SF_NEXUS_GETPLAYERLIST;
   runThread = TRUE;
   playerManagerEvent.Reset();
   playerManagerThread.Start(SF_PlayerManagerThread,(void *)this);
   return 0;
   }


int
SF_PlayerManagerWindow_PlayerListBox(S_ListBox *lb) {
   SF_PlayerManagerWindow *pmw = (SF_PlayerManagerWindow *)lb->parent;
   if ( lb->GetIndex() >= 0 )
      pmw->viewButton->SetEnabled(TRUE);
   return 0;
   }


int
SF_PlayerManagerWindow_Refresh(S_Button *b) {
   SF_PlayerManagerWindow *pmw = (SF_PlayerManagerWindow *)b->parent;
   pmw->action = SF_NEXUS_GETPLAYERLIST;
   pmw->refreshButton->SetEnabled(FALSE);
   pmw->viewButton->SetEnabled(FALSE);
   pmw->playerManagerEvent.Set();
   return 0;
   }


int
SF_PlayerManagerWindow_View(S_Button *b) {
   SF_PlayerManagerWindow *pmw = (SF_PlayerManagerWindow *)b->parent;
   pmw->action = SF_NEXUS_GETPLAYERDATA2;
   pmw->refreshButton->SetEnabled(FALSE);
   pmw->viewButton->SetEnabled(FALSE);
   pmw->playerManagerEvent.Set();
   return 0;
   }



int
SF_PlayerManagerWindow::HandleSizeMsg() {
   cxButton = 60;
   cyButton = 25;
   cxListBox = 150;
   cyListBox = cyCanvas-30-16-cyButton;
   cxStatsListBox = cxCanvas - cxListBox - 30;
   if ( playerListBox ) {
      playerListBox->SetSize(cxListBox,cyListBox);
      playerListBox->SetPosition(10,26);
      }
   if ( playerStatsTextEdit ) {
      playerStatsTextEdit->SetSize(cxStatsListBox,cyListBox);
      playerStatsTextEdit->SetPosition(20+cxListBox,26);
      }
   if ( refreshButton ) {
      refreshButton->SetSize(cxButton,cyButton);
      refreshButton->SetPosition(10,cyCanvas-10-cyButton);
      }
   if ( viewButton ) {
      viewButton->SetSize(cxButton,cyButton);
      viewButton->SetPosition(20+cxButton,cyCanvas-10-cyButton);
      }
   return 0;
   }


int
SF_PlayerManagerWindow::HandlePaintMsg() {
   DrawFilledRect(0,0,cxCanvas,cyCanvas,backGroundRGB);
   return 0;
   }


int
SF_PlayerManagerWindow::HandleCloseMsg() {

   runThread = FALSE;
   playerManagerEvent.Set();

   playerManagerThread.Stop();

   if ( refreshButton ) {
      delete refreshButton;
      refreshButton = NULL;
      }
   if ( viewButton ) {
      delete viewButton;
      viewButton = NULL;     
      }

   if ( playerListBox ) {
      delete playerListBox;
      playerListBox = NULL;
      }
   if ( playerStatsTextEdit ) {
      delete playerStatsTextEdit;
      playerStatsTextEdit = NULL;
      }
   return 0;
   }

SF_PlayerManagerWindow::~SF_PlayerManagerWindow() {
   if ( nexusName ) {
      S_free(nexusName);
      nexusName = NULL;
      }
   }
/* ************************************************************************* */

/* ************************************************************************* */
/* ************************** SF_PlayerStatsWindow ************************* */
/* ************************************************************************* */
int
SF_PlayerStatsThread(void *iarg) {
   SF_PlayerStatsWindow *psw = (SF_PlayerStatsWindow *)iarg;
   do {
      if ( psw && psw->runThread ) {
         if ( psw->refreshButton )
            psw->refreshButton->SetEnabled(TRUE);
         psw->playerStatsEvent.Reset();
         psw->playerStatsEvent.WaitFor();
         }
      } while (psw && psw->runThread);
   return 0;
   }


SF_PlayerStatsWindow::SF_PlayerStatsWindow(S_Object *iparent,
                                           char *inexusName) :
   S_Window(NULL,iparent,NULL,NULL,0,0,0,0,0,S_WINDOW_STATICCHILD) {

   nexusName = S_strdup(inexusName);

   HandleCreateMsg();
   created = TRUE;
   }


int
SF_PlayerStatsWindow::HandleCreateMsg() {
   refreshButton = new S_Button(this,SF_PlayerStatsWindow_Refresh,"Refresh");
   refreshButton->SetEnabled(FALSE);

   numHeaderButtons = 5;
   headerButtons = (S_Button **)S_malloc(sizeof(S_Button *)*numHeaderButtons);
   headerButtons[0] = new S_Button(this,NULL,"Name");
   headerButtons[1] = new S_Button(this,NULL,"Kills");
   headerButtons[2] = new S_Button(this,NULL,"Losses");
   headerButtons[3] = new S_Button(this,NULL,"Score");
   headerButtons[4] = new S_Button(this,NULL,"Time (sec)");

   scrollBar = new S_ScrollBar(this);

   runThread = TRUE;
   playerStatsEvent.Reset();
   playerStatsThread.Start(SF_PlayerStatsThread,(void *)this);
   return 0;
   }


int
SF_PlayerStatsWindow_Refresh(S_Button *b) {
   SF_PlayerStatsWindow *psw = (SF_PlayerStatsWindow *)b->parent;
   psw->refreshButton->SetEnabled(FALSE);
   psw->playerStatsEvent.Set();
   return 0;
   }


int
SF_PlayerStatsWindow::HandleSizeMsg() {
   cxButton = 60;
   cyButton = 25;
   for ( int i=0; i<numHeaderButtons; i++ ) {
      headerButtons[i]->SetSize(cxCanvas/numHeaderButtons,20);
      headerButtons[i]->SetPosition(cxCanvas*i/numHeaderButtons,0);
      }
   if ( refreshButton ) {
      refreshButton->SetSize(cxButton,cyButton);
      refreshButton->SetPosition(10,cyCanvas-10-cyButton);
      }
   if ( scrollBar ) {
      scrollBar->SetSize(15,cyCanvas-18);
      scrollBar->SetPosition(cxCanvas-15,20);
      }
   return 0;
   }


int
SF_PlayerStatsWindow::HandlePaintMsg() {
   DrawFilledRect(0,0,cxCanvas,cyCanvas,backGroundRGB);
   return 0;
   }


int
SF_PlayerStatsWindow::HandleCloseMsg() {
   return 0;
   }


SF_PlayerStatsWindow::~SF_PlayerStatsWindow() {

   runThread = FALSE;
   playerStatsEvent.Set();
   playerStatsThread.WaitFor();

   if ( refreshButton ) {
      delete refreshButton;
      refreshButton = NULL;
      }

   if ( headerButtons ) {
      for ( int i=0; i<numHeaderButtons; i++ ) {
         delete headerButtons[i];
         headerButtons[i] = NULL;
         }
      S_free(headerButtons);
      headerButtons = NULL;
      }
   if ( scrollBar ) {
      delete scrollBar;
      scrollBar = NULL;
      }
   if ( nexusName ) {
      S_free(nexusName);
      nexusName = NULL;
      }
   }
/* ************************************************************************* */

