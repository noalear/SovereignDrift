#include "sf.h"
#include "SF_Spy.h"
#include "SF_Nexus.h"
#include "SF_ClientWindow.h"
#include "SF_Globals.h"
#include "SF_Client.h"
#include "SF_Server.h"


int
SF_Spy::Init() {
   nexusThread = NULL;
   maxHostnames = 0;
   hostnames = NULL;
   return 0;
   }

SF_Spy::SF_Spy() {
   Init();
   S_GWindow::Init();
   }


int
SF_Spy::Create( S_Root *iroot,char *inexusName ) {
   nexusName.Create(inexusName);
   S_GWindow::Create( iroot,NULL,"SFSpy","blue_spy.bmp",1,
                      iroot->cx/2-192,iroot->cy/2-68,
                      384,136 );
   return 0;
   }


int
NexusThread(void *arg) {
  int i;
  int success = FALSE;
  ulong numServers = 0;
  ulong numOldServers = 0;
  SF_Spy *spy = (SF_Spy *)arg;

  char tmp[128];

  for (;;) {

    spy->timer.Set(60000000);

    spy->refreshButton.Hide();
    spy->Refresh();

    sprintf(tmp,"Contacting %s...",spy->nexusName.str);
    spy->status.SetLine(0,tmp);
    spy->status.Draw();

    success = FALSE;

    S_Client c;
    c.SetTimeOut(60000000);
    if ( c.Connect(spy->nexusName.str,
		    SF_NEXUS_PORT,
		    SF_NEXUS_SERVERLIST)==S_CLIENT_OK ){

      spy->status.SetLine(0,"Getting server list...");
      spy->status.Draw();

      S_Packet packet;
      c.ReadPacket(&packet);
      c.Disconnect();

      ulong response = packet.PopULong();
      if ( packet.readComplete && response == S_CLIENT_OK ) {

	success = TRUE;

	numOldServers = numServers;
	numServers = packet.PopULong();

	if ( numServers > 0 ) {

	  if ( !spy->hostnames ) {
	    spy->hostnames=(char **)S_malloc(numServers*sizeof(char *));
	    for ( i=0; i<(int)numServers; i++ )
	      spy->hostnames[i] = NULL;
	    spy->maxHostnames = numServers;
	  }
	  else if ( (int)numServers > spy->maxHostnames ) {
	    spy->hostnames = (char **)S_realloc(spy->hostnames,
						numServers*sizeof(char *));
	    for ( i=spy->maxHostnames; i<(int)numServers; i++ )
	      spy->hostnames[i] = NULL;
	    spy->maxHostnames = numServers;
	  }

	  for ( i=0; i<(int)numServers; i++ ) {
	    char *serverString = packet.PopString();
            if ( serverString ) {
	      S_TokenArray tokens(serverString,':',':');
	      S_free(serverString);
	      if ( tokens.numLines >= 1 && tokens.lines[0] ) {
		char *hostname = tokens.lines[0];
		if ( spy->hostnames[i] )
		  S_free(spy->hostnames[i]);

                int port = SF_SERVER_PORT;
                if ( tokens.numLines >= 2 && tokens.lines[1] )
		  {
                    port = atoi(tokens.lines[1]);
		  }

                S_String serverString("%s:%d",hostname,port);
		spy->hostnames[i] = S_strdup(serverString.str);

		sprintf(tmp,"Contacting %s...",serverString.str);
		spy->status.SetLine(0,tmp);
		spy->status.Draw();
		S_Client c;
		c.SetTimeOut(60000000);
		if ( c.Connect(hostname,
			       port,
			       SF_SERVER_TOUCH) == S_CLIENT_OK ) {

		  spy->status.SetLine(0,"Calculating lag...");
		  spy->status.Draw();

		  S_Packet packet;

		  packet.PushUShort(SF_CLIENT_NUMLOOPS);
		  packet.PushUShort(SF_CLIENT_NUMBYTES);
		  c.WritePacket(&packet);
		  packet.Flush();

		  c.ReadPacket(&packet);

		  ushort numPlayers = packet.PopUShort();
		  ushort  maxSlots = packet.PopUShort();
		  /*uchar majorVersion = */packet.PopUChar();
		  /*uchar minorVersion = */packet.PopUChar();
		  char *osString = packet.PopString();
		  char *serverName = packet.PopString();
		  /*uchar minorminorVersion = */packet.PopUChar();
		  char *versionModifier = packet.PopString();
		  char *versionString = packet.PopString();

		  S_StopWatch sw;
		  if ( packet.readComplete && serverName && osString ) {

		    packet.Flush();

		    sw.Start();
		    for ( int j=0; j<SF_CLIENT_NUMLOOPS; j++ ) {
		      for ( int k=0; k<SF_CLIENT_NUMBYTES; k++ )
			packet.PushUChar((uchar)i);
		      c.WritePacket(&packet);
		      packet.Flush();
		      c.ReadPacket(&packet);
		      packet.Flush();
		    }
		    sw.Stop();
		  }

		  c.Disconnect();

		  if ( serverName && osString ) {

		    double lagUsec = (sw.dtSec*1000000+sw.dtUsec) /
		      SF_CLIENT_NUMLOOPS;
		    int len = strlen(serverName) + strlen(hostname) +
		      strlen(osString) + 64;
		    char *touchString = (char *)S_malloc(len);

		    if ( strlen(serverName) > 35 )
		      {
			serverName[35] = 0;
		      }

		    sprintf(touchString,
			    "%-35s|%3d/%-3d %4.0fms",
			    serverName,numPlayers,maxSlots,lagUsec/1000.0);

		    S_free(osString);
		    S_free(serverName);
		    S_free(versionModifier);
		    S_free(versionString);

		    spy->listbox.SetLine(i,touchString);
		    spy->listbox.Draw();

		    S_free(touchString);
		  }
		  else {
		    spy->SetItemUnknown(i,hostname);
		  }
		}
		else {
		  spy->SetItemUnknown(i,hostname);
		}
	      }
	    }
	  }

	  if ( numOldServers > numServers )
	    for ( i=numServers; i<(int)numOldServers; i++ )
	      spy->listbox.SetLine(i," ");

	  sprintf(tmp,"Found %ld servers.",numServers);
	  spy->status.SetLine(0,tmp);
	  spy->status.Draw();

	}
	else {
	  spy->status.SetLine(0,"SFSpy :) . . . No servers :(");
	  spy->status.Draw();
	}
      }

    }
    else {
      char tmp[128];
      sprintf(tmp,"SFSpy :( Nexus is down %ld :(",c.rc);
      spy->status.SetLine(0,tmp);
      spy->status.Draw();
    }

    spy->refreshButton.Show();

    if ( success )
      spy->timer.WaitFor();
    else
      _OsSleep(1000000);
  }

  return 0;
}


int
SF_Spy::SetItemUnknown(int i,char *hostname) {
   char *def = "|???" "/" "??? ????ms";
   int len = strlen(hostname) + strlen(def) + 1 + 35;
   char *touchString = (char *)S_malloc(len);
   if ( touchString ) {
      sprintf(touchString,"%-35s%s",hostname,def);
      listbox.SetLine(i,touchString);
      listbox.Draw();
      S_free(touchString);
      }
   return 0;
   }


int
SF_Spy::HandleCreateMsg() {
   closeButton.Create(this,SF_Spy_CloseButtonHandler,
                      "blue_close.bmp",cxCanvas-16-4,4);

   refreshButton.Create(this,SF_Spy_RefreshButtonHandler,
                        "blue_refresh.bmp",cxCanvas-16-4-16-4,4);

   status.Create(this,NULL,"blue_info.bmp",185,4,150,16);
   status.ymargin = 10;

   helv.Create(S_FONT_HELV,8);
   status.SetFont(&helv);

   listbox.Create(this,SF_Spy_ListBoxHandler,"blue_listbox.bmp",4,30,357,100);
   listbox.CreateScrollBar("blue_scroll.bmp","blue_up.bmp",
                           "blue_down.bmp","blue_slider.bmp");


   lucida.ZCreate("blue_font.bmp",7,10,8);
   listbox.SetGFont(&lucida);

   nexusThread = new S_Thread(NexusThread,this);

   Show();
   return 0;
   }


int
SF_Spy_ListBoxHandler(S_GListBox *lb) {
   SF_Spy *spy = (SF_Spy *)lb->parent;
   if ( spy->button == 1 && !spy->keyup && spy->repeat ) {
      if ( lb->selectedLine >= 0 ) {
         S_Process p;
         p.Start("frontier.exe",spy->hostnames[lb->selectedLine]);
         spy->Close();
         }
      }
   return 0;
   }


int
SF_Spy_CloseButtonHandler(S_GButton *b) {
   b->parent->Close();
   return 0;
   }


int
SF_Spy_RefreshButtonHandler(S_GButton *b) {
   SF_Spy *spy = (SF_Spy *)b->parent;
   spy->timer.Set(0);
   return 0;
   }


int
SF_Spy::HandleSizeMsg() {
   listbox.SetSize(cxClient,cyClient);
   listbox.SetPosition(0,0);
   return 0;
   }


int
SF_Spy::HandleCloseMsg() {
   if ( hostnames ) {
      for ( int i=0; i<maxHostnames; i++ ) {
	 if ( hostnames[i] ) {
            S_free(hostnames[i]);
            hostnames[i] = NULL;
	    }
         }
      S_free(hostnames);
      hostnames = NULL;
      }
   if ( nexusThread ) {
      nexusThread->Stop();
      delete nexusThread;
      nexusThread = NULL;
      }
   return 0;
   }


int
SF_Spy::Destroy() {
   S_GWindow::Destroy();
   return 0;
   }


SF_Spy::~SF_Spy() {
   }
