#include "sf.h"
#include "SF_Globals.h"
#include "SF_ClientWindow.h"
#include "SF_Client.h"
#include "SF_ServerWindow.h"
#include "SF_Server.h"
#include "SF_PortCpp.h"
#include "SF_GameEngine.h"
#include "SF_Externals.h"
#include "SF_Slot.h"


int
SF_ServerWindow::Init() {
   server = NULL;
   unload = FALSE;
   diagString[0] = 0;
   updateThread = NULL;
   S_GWindow::Init();
   return 0;
   }

SF_ServerWindow::SF_ServerWindow() {
   Init();
   }


int
SF_ServerWindow::Create( S_Root *iroot,SF_Server *iserver ) {
   server = iserver;
   S_GWindow::Create( iroot,NULL,"Stellar Frontier Server",
                      "blue_server.bmp",1,
                      iroot->cx/2-320,iroot->cy/2-240,
                      640,480 );
   diagString[0] = 0;

   if ( server )
     {
       S_String port("%d",server->port);
       portTextBox.SetText(port.str);
     }

   return 0;
   }


int
SF_ServerWindow::SetDiagString(const char *fmt,...) {
   va_list v;
   va_start(v,fmt);
   vsprintf(diagString,fmt,v);
   va_end(v);
   return 0;
   }


int
UpdateServerWindowThread(void *iargs) {
  //SF_ServerWindow *sw = (SF_ServerWindow *)iargs;
   /*
   while ( sw->update ) {
      sw->diagTextBox->SetText(sw->diagString);
      _OsSleep(100000);
      }
      */
   return 0;
   }


int
SF_ServerWindow::HandleCreateMsg() {

   nexusCheckBox.Create(this,SF_Server_NexusCheckBoxHandler,
                        "blue_nexusreg.bmp",184,78);
   nexusCheckBox.toggle = TRUE;

   fps11CheckBox.Create(this,SF_Server_11fpsCheckBoxHandler,
                        "blue_11fps.bmp",184,108);
   fps16CheckBox.Create(this,SF_Server_16fpsCheckBoxHandler,
                        "blue_16fps.bmp",184,125);
   fps16CheckBox.toggle = TRUE;

   portTextBox.Create(this,NULL,"blue_textbox.bmp",296,119,48,18);

   lagCheckBox.Create(this,SF_Server_LagCheckBoxHandler,
                      "blue_lag.bmp",184,155);
   lagCheckBox.toggle = TRUE;

   lagTextBox.Create(this,NULL,"blue_textbox.bmp",296,156,48,18);

   S_String lag("%d",SF_SERVER_MAXLAGMS);
   lagTextBox.SetText(lag.str);

   closeButton.Create(this,SF_Server_CloseButtonHandler,
                      "blue_close.bmp",cxCanvas-16-4,4);

   minimizeButton.Create(this,SF_Server_MinimizeButtonHandler,
                         "blue_minimize.bmp",cxCanvas-40,4);

   startButton.Create(this,SF_Server_StartButtonHandler,
                      "blue_start.bmp",553,78);
   warnButton.Create(this,SF_Server_WarnButtonHandler,
                     "blue_warn.bmp",553,102);
   kickButton.Create(this,SF_Server_KickButtonHandler,
                     "blue_kick.bmp",553,126);
   stopButton.Create(this,SF_Server_StopButtonHandler,
                     "blue_stop.bmp",553,150);

   stopButton.SetEnabled(FALSE);
   warnButton.SetEnabled(FALSE);
   kickButton.SetEnabled(FALSE);
   closeButton.SetEnabled(TRUE);
   minimizeButton.SetEnabled(TRUE);

   scenarioList.Create(this,NULL,
                       "blue_scenariolist.bmp",375,78,150,100);
   scenarioList.CreateScrollBar("blue_scroll.bmp","blue_up.bmp",
                                "blue_down.bmp","blue_slider.bmp");

   playerList.Create(this,SF_Server_PlayerListBoxHandler,
                     "blue_playerlist.bmp",12,70,120,397);
   playerList.CreateScrollBar("blue_scroll.bmp","blue_up.bmp",
                              "blue_down.bmp","blue_slider.bmp");
   playerList.selectedLine = 0;

   console.Create(this,NULL,"blue_console.bmp",174,214,435,233);
   console.CreateScrollBar("blue_scroll.bmp","blue_up.bmp",
                           "blue_down.bmp","blue_slider.bmp");

   textbox.Create(this,SF_Server_TextBoxHandler,
                  "blue_textbox.bmp",174,449,435,18);

   lucida.ZCreate("blue_font.bmp",7,10,8);
   playerList.SetGFont(&lucida);
   scenarioList.SetGFont(&lucida);

   helv.Create(S_FONT_HELV,8);
   console.SetFont(&helv);
   textbox.SetFont(&helv);

   dirList = new S_DirList(SF_ServerFileDirMask);
   for ( int i=0; i<dirList->numFiles; i++ ) {
      char *scenarioName = ReadValueFromScenarioFile(dirList->file[i],"name");
      if ( scenarioName ) {
         scenarioList.SetLine(i,scenarioName);
         S_free(scenarioName);
         }
      else
         scenarioList.SetLine(i,"???");
      }
   scenarioList.selectedLine = 0;

   //update = TRUE;
   //updateThread = new S_Thread(UpdateServerWindowThread,(void *)this);

   CreateSystemTrayIcon();

   trayMenu.Create(this,"SF Server");
   titleMenuItem.Create(&trayMenu,"SF Server",NULL);
   trayMenu.AddSeparator();
   showMenuItem.Create(&trayMenu,"Show",SF_ServerWindow_ShowMenuItemHandler);
   hideMenuItem.Create(&trayMenu,"Hide",SF_ServerWindow_HideMenuItemHandler);
   trayMenu.AddSeparator();
   exitMenuItem.Create(&trayMenu,"Close",
                       SF_ServerWindow_CloseMenuItemHandler);

   prompt.Create(this);
   prompt.selfDelete = FALSE;

   if ( !server->startInTray )
     {
       Show();
     }

   return 0;
   }


int
SF_ServerWindow::HandleSystemTrayMouseMsg()
{
  switch(button)
    {
    case 1 :
      if ( repeat && !IsVisible() )
	Show();
      break;
    case 2 :
      if ( !keyup )
	{
	  trayMenu.PopupOnMouse();
	}
      break;
    }
  return 0;
}


int
SF_ServerWindow::RefreshPlayerList(int index,char *name)
 {
   mutex.Request();

   if ( !game )
     {
       return 0;
     }

   S_String tmp;
   for ( int i=0; i<game->numShips; i++ )
     {
       if ( i<game->maxHumanShips )
         {
           if ( server->slot[i].filled )
             tmp.Create("%d: %s",
                        i,
                        game->player[game->ship[i].playerIndex].name);
           else
             tmp.Create("%d: < Empty >",i);
           }
      else
        {
          tmp.Create("%d: [ AI ]",i);
        }

      if ( name && i == index )
	{
          tmp.Create("%d: %s",i,name);
	}

      playerList.SetLine(i,tmp.str);
      }

   mutex.Release();

   return 0;
   }


int
SF_ServerWindow::HandleCloseMsg() {

  Hide();

  closing = unload;

   return 0;
   }


int
SF_ServerWindow::HandleDestroyMsg()
{

  DestroySystemTrayIcon();
  
  update = FALSE;
  if ( updateThread ) {
    updateThread->WaitFor();
    delete updateThread;
    updateThread = NULL;
  }
  if ( dirList ) {
    delete dirList;
    dirList = NULL;
  }
  
  server->Stop();
  
  return 0;
}


int
SF_ServerWindow::Destroy() {
   S_GWindow::Destroy();
   return 0;
   }


SF_ServerWindow::~SF_ServerWindow() {
   Destroy();
   }


int
SF_Server_CloseButtonHandler(S_GButton *b) {
   SF_ServerWindow *sw = (SF_ServerWindow *)b->parent;
   sw->Hide();
   return 0;
   }


int
SF_Server_MinimizeButtonHandler(S_GButton *b) {
   ((S_Window *)b->parent)->Minimize();
   return 0;
   }


int
SF_Server_PlayerListBoxHandler(S_GListBox *b) {
   return 0;
   }


int
SF_Server_StartButtonHandler(S_GButton *b) {

   SF_ServerWindow *sw = (SF_ServerWindow *)b->parent;

   if ( sw->nexusCheckBox.toggle ) {
      sw->server->registerWithNexus = TRUE;
      }
   else {
      sw->server->registerWithNexus = FALSE;
      }

   frameRate = 11;
   if ( sw->fps16CheckBox.toggle )
      frameRate = 16;

   sw->server->lagMax = atol(sw->lagTextBox.buffer.str) *
                        sw->lagCheckBox.toggle;

   sw->server->port = atol(sw->portTextBox.buffer.str);

   ((S_FileServer *)sw->server)->Create(sw->server->port,
                                        S_LOGGER_TIMESTAMP|S_LOGGER_TOCONSOLE,
                                        "server.log");

   b->SetEnabled(FALSE);
   sw->stopButton.SetEnabled(TRUE);
   sw->kickButton.SetEnabled(TRUE);
   sw->warnButton.SetEnabled(TRUE);

   sw->server->Start();

   return 0;
   }


int
SF_Server_StopButtonHandler(S_GButton *b) {
   b->SetEnabled(FALSE);
   ((SF_ServerWindow *)b->parent)->server->Stop();
   ((SF_ServerWindow *)b->parent)->startButton.SetEnabled(TRUE);
   ((SF_ServerWindow *)b->parent)->kickButton.SetEnabled(FALSE);
   ((SF_ServerWindow *)b->parent)->warnButton.SetEnabled(FALSE);
   return 0;
   }


int
SF_Server_WarnButtonHandler(S_GButton *b) {
   ((SF_ServerWindow *)b->parent)->server->SendWarningMsg();
   return 0;
   }


int
SF_Server_KickButtonHandler(S_GButton *b) {
   //((SF_ServerWindow *)b->parent)->server->KickEverybodyOff();
   int slot = ((SF_ServerWindow *)b->parent)->playerList.selectedLine;
   if ( slot >= 0 && slot < max_players ) {
      ((SF_ServerWindow *)b->parent)->server->KickPlayerOff(slot);
      }
   return 0;
   }


int
SF_Server_NexusCheckBoxHandler(S_GButton *b) {
   b->toggle = !b->toggle;
   return 0;
   }


int
SF_Server_11fpsCheckBoxHandler(S_GButton *b) {
   SF_ServerWindow *sw = (SF_ServerWindow *)b->parent;
   sw->fps11CheckBox.toggle = TRUE;
   sw->fps16CheckBox.toggle = FALSE;
   sw->Refresh();
   return 0;
   }


int
SF_Server_16fpsCheckBoxHandler(S_GButton *b) {
   SF_ServerWindow *sw = (SF_ServerWindow *)b->parent;
   sw->fps11CheckBox.toggle = FALSE;
   sw->fps16CheckBox.toggle = TRUE;
   sw->Refresh();
   return 0;
   }


int
SF_Server_LagCheckBoxHandler(S_GButton *b) {
   SF_ServerWindow *sw = (SF_ServerWindow *)b->parent;
   sw->lagCheckBox.toggle = !sw->lagCheckBox.toggle;
   sw->server->lagMax = atol(sw->lagTextBox.buffer.str) *
                        sw->lagCheckBox.toggle;
   sw->Refresh();
   return 0;
   }


int
SF_Server_TextBoxHandler(S_GTextBox *tb) {
   SF_ServerWindow *sw = (SF_ServerWindow *)tb->parent;
   if ( sw->scancode == S_KEY_ENTER && !sw->keyup ) {

      char tmp[1024];
      sprintf(tmp,"Server> %s",tb->buffer.str);
      tb->Flush();

      SF_MsgBuffer *bb = &sw->server->broadcastBuffer;
      bb->mutex.Request();
      bb->PushMsg((uchar)SF_MSG_PLAYERCHAT_N,
                  (ushort)0,
                  (uchar)SF_MSG_CHAT_TOALL,
                  (uchar)0,
                   tmp,
                  (int)S_CLIENT_TCP);
      bb->mutex.Release();
      sw->Refresh();
      }
   return 0;
   }


int
SF_ServerWindow_CloseMenuItemHandler(S_MenuItem *mi) {
  SF_ServerWindow *w = (SF_ServerWindow *)mi->window;
  if ( w->server->numPlayers > 0 )
    {
      w->prompt.Show();
    }
  else
    {
      w->unload = TRUE;
      w->Close();
    }
  return 0;
}


int
SF_ServerWindow_ShowMenuItemHandler(S_MenuItem *mi) {
   SF_ServerWindow *w = (SF_ServerWindow *)mi->window;
   w->Show();
   return 0;
   }


int
SF_ServerWindow_HideMenuItemHandler(S_MenuItem *mi) {
   SF_ServerWindow *w = (SF_ServerWindow *)mi->window;
   w->Hide();
   return 0;
   }
