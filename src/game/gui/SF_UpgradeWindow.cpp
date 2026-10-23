#include "sf.h"
#include "SF_UpgradeWindow.h"


int
UpgradeThread(void *iarg) {
   SF_UpgradeWindow *w = (SF_UpgradeWindow *)iarg;

   int rc = 0;
   S_FileClient client;
   client.SetTimeOut(60000000);

   w->totalNumFiles = 0;
   w->currentFile = 0;
   char text[1024];

   int i,j;

   for ( i=0; i<w->numMasks; i++ ) {
      char text[1024];
      sprintf(text,"Getting list of files (%s)",w->masks[i]);
      w->fileGroupBox->SetText(text);
      int n = client.RequestFileList(w->hostName,w->port,w->masks[i],
                                     w->fileList[i]);
      if ( n > 0 ) {
         w->packets[i] = (S_Packet **) S_malloc(n*sizeof(S_Packet *));
         for ( j=0; j<n; j++ ) {
            w->packets[i][j] = new S_Packet;
	    }
         w->numFiles[i] = n;
         w->totalNumFiles += n;
         }

      }

   rc = 0;
   int firstTime = TRUE;
   for ( i=0; i<w->numMasks && !rc; i++ ) {

      w->fileGroupBox->SetText("Downloading");

      w->fileList[i]->Rewind();

      ulong status = 0;
      ulong size = 0;
      ulong checkSum = 0;
      ulong modDate = 0;
      int tryAgain = FALSE;

      ulong localCheckSum = 0;
      ulong localSize = 0;

      j = 0;
      w->numFiles[i] = w->fileList[i]->PopLong();
      while ( j<w->numFiles[i] && !rc ) {

         if ( !tryAgain ) {
            w->currentFileName = w->fileList[i]->PopString();
            w->currentPacket = w->packets[i][j];
            status = w->fileList[i]->PopULong();
            size = w->fileList[i]->PopULong();
            checkSum = w->fileList[i]->PopULong();
            modDate = w->fileList[i]->PopULong();
            localCheckSum = _OsFileCheckSum(w->currentFileName);
            localSize = _OsFileSize(w->currentFileName);
	    }

         tryAgain = FALSE;

         if ( w->currentFileName ) {

            _OsFilename(w->currentFileName);

            if ( firstTime ) {
               sprintf(text,"Name: %s (%d of %d)",w->currentFileName,
                                                  w->currentFile+1,
                                                  w->totalNumFiles);
               w->nameText->SetText(text);
               }

            rc = 0;
            if ( checkSum != localCheckSum || size != localSize ) {
               rc = client.RequestFile(w->hostName,w->port,
                                        w->currentFileName,
                                        w->currentPacket);
	       }
            else {
               w->currentPacket->totalBytesRcvd = 
                  _OsFileSize(w->currentFileName);
               w->currentPacket->packetSize = w->currentPacket->totalBytesRcvd;
	       }

            sprintf(text,"Name: %s (%d of %d)",w->currentFileName,
                                               w->currentFile+1,
                                               w->totalNumFiles);
            w->nameText->SetText(text);

            sprintf(text,"Size: %ld bytes (%ld compressed)",
                    w->currentPacket->uncompPacketSize,
                    w->currentPacket->packetSize);

            w->sizeText->SetText(text);
            sprintf(text,"Rcvd: %ld of %ld compressed bytes",
                    w->currentPacket->totalBytesRcvd,
                    w->currentPacket->packetSize);
            w->rcvdText->SetText(text);

            switch(rc) {
               case S_CLIENT_TCPCONNECTFAILED :
                  sprintf(text,
                          "The server at %s did not respond while trying to "
                          "download %s.  \nIt may be down or just very busy.  "
                          "\n\nDo you want me to retry the connection?",
                           w->hostName,w->currentFileName);
               break;
               case S_FILECLIENT_PACKETREADFAILED :
                  sprintf(text,
                          "I could not download the data for %s on the server "
                          "at %s. \nIt may be that the server was very busy.  "
                          "\n\nDo you want me to try to download %s again?",
                          w->currentFileName,w->hostName,w->currentFileName);
                  break;
               default :
                  sprintf(text,
                          "I could not download the data for %s on the server "
                          "at %s. \nIt may be that the server was very busy. "
                          "rc: %d \n\nDo you want me to try to download %s "
                          "again?",
                           w->currentFileName,w->hostName,rc,
                           w->currentFileName);
                  break;
               }

            if ( rc ) {
               S_MsgBox msg(w,"SF Upgrade",text,S_MSGBOX_YESNO);
               if ( msg.rc == S_MSGBOX_YES) {
                  tryAgain = TRUE;
                  }
               }
            else {

               rc = 0;
               if ( checkSum != localCheckSum || size != localSize ) {
                  rc = client.WriteFile(w->currentFileName,w->currentPacket);
	          }

               if ( rc ) {
                  sprintf(text,
                          "I could not save the data for %s to the local "
                          "hard disk.  rc = %d  \nYou may have left a "
                          "copy running.\n\nShould I try to save it again?",
                          w->currentFileName,rc);
                  S_MsgBox msg(w,"SF Upgrade",text,S_MSGBOX_YESNO);
                  if ( msg.rc == S_MSGBOX_YES) {
                     tryAgain = TRUE;
                     }
                  }

	       }

            rc = 0;

            if ( !tryAgain ) {
               S_free(w->currentFileName);
               w->currentFile++;
               j++;
	       }

	    }
         else {
            j++;
            w->currentFile++;
            }

         }
      }

   if ( !rc )
      w->upgradeSucceeded = TRUE;
   else
      S_MsgBox msg(w,"SF Upgrade","Upgrade Failed.",S_MSGBOX_OK);

   w->drawProgressBar = FALSE;
   w->Hide();
   if ( w->func )
      (*w->func)(w->arg);

   return 0;
   }


int
ProgressThread(void *iarg) {
   ulong bytesRcvd = 0;
   int i=0,lasti = 0;
   char text[1024];
   SF_UpgradeWindow *w = (SF_UpgradeWindow *)iarg;
   while ( w->drawProgressBar ) {

      if ( w->currentPacket ) {

         if ( w->currentPacket->packetSize > 8 )
            i = w->currentFile;
         else
            i = w->currentFile - 1;
         if ( i < 0 ) i = 0;

         if ( i != lasti || bytesRcvd != w->currentPacket->totalBytesRcvd ) {
            lasti = i;
            bytesRcvd = w->currentPacket->totalBytesRcvd;
            if ( w->currentPacket->packetSize > 8 ) {
               sprintf(text,"Name: %s (%d of %d)",w->currentFileName,
                                                  i+1,w->totalNumFiles);
               w->nameText->SetText(text);

               if ( w->currentPacket->uncompPacketSize !=
                    w->currentPacket->packetSize )
                  sprintf(text,"Size: %ld bytes (%ld compressed)",
                          w->currentPacket->uncompPacketSize,
                          w->currentPacket->packetSize);
               else
                  sprintf(text,"Size: %ld bytes",
                          w->currentPacket->packetSize);
               w->sizeText->SetText(text);

               if ( w->currentPacket->uncompPacketSize !=
                    w->currentPacket->packetSize )
                  sprintf(text,"Rcvd: %ld of %ld compressed bytes",
                          w->currentPacket->totalBytesRcvd,
                          w->currentPacket->packetSize);
               else
                  sprintf(text,"Rcvd: %ld of %ld bytes",
                          w->currentPacket->totalBytesRcvd,
                          w->currentPacket->packetSize);
               w->rcvdText->SetText(text);
               }

            w->DrawProgressBar();
            }
         }
      _OsSleep(200000);
      }
   return 0;
   }


SF_UpgradeWindow::SF_UpgradeWindow( S_Root *iroot ) :
   S_Window( iroot,NULL,"SF Download",NULL,0,
             iroot->cx/2-175,iroot->cy/2-100,
             350,195,S_WINDOW_DIALOG ) {
   HandleCreateMsg();
   HandleSizeMsg();
   created = TRUE;
   }

SF_UpgradeWindow::SF_UpgradeWindow( S_Window *iparent ) :
   S_Window( NULL,iparent,"SF Download",NULL,0,
             _root->cx/2-175,_root->cy/2-100,
             350,195,S_WINDOW_DIALOG ) {
   HandleCreateMsg();
   HandleSizeMsg();
   created = TRUE;
   }


int
SF_UpgradeWindow::StartThread(char *ihostName,int iport,
                              char **imasks,int inumMasks,
                              int (*ifunc)(void *),void *iarg ) {

   if ( hostName )
      S_free(hostName);
   hostName = S_strdup(ihostName);

   port = iport;
   numMasks = inumMasks;
   masks = imasks;
   func = ifunc;
   arg = iarg;

   ratio = 0.0;

   int i;

   packets = (S_Packet ***)S_malloc(numMasks*sizeof(S_Packet **));
   for ( i=0; i<numMasks; i++ )
      packets[i] = 0;

   if ( numFiles ) {
      S_free(numFiles);
      numFiles = 0;
      }
   numFiles = (int *)S_malloc(numMasks*sizeof(int));
   for ( i=0; i<numMasks; i++ )
      numFiles[i] = 0;

   fileList = (S_Packet **) S_malloc(numMasks*sizeof(S_Packet *));
   for ( i=0; i<numMasks; i++ )
      fileList[i] = new S_Packet();

   totalNumFiles = 0;
   drawProgressBar = TRUE;
   upgradeSucceeded = FALSE;
   upgradeThread.Start(UpgradeThread,(void *)this);
   progressThread.Start(ProgressThread,(void *)this);

   return 0;
   }


int
SF_UpgradeWindow::StopThread() {
   drawProgressBar = FALSE;
   progressThread.WaitFor();
   upgradeThread.Stop();
   int i,j;

   if ( packets ) {
      for ( i=0; i<numMasks; i++ ) {
	 if ( packets[i] ) {
	    for ( j=0; j<numFiles[i]; j++ ) {
	       if ( packets[i][j] ) {
                  delete packets[i][j];
                  packets[i][j] = 0;
	          }
	       }
            S_free(packets[i]);
            packets[i] = NULL;
	    }
         }
      S_free(packets);
      packets = NULL;
      }
   if ( numFiles ) {
      S_free(numFiles);
      numFiles = NULL;
      }
   if ( fileList ) {
      for ( i=0; i<numMasks; i++ ) {
	 if ( fileList[i] ) {
            delete fileList[i];
            fileList[i] = NULL;
	    }
         }
      S_free(fileList);
      fileList = NULL;
      }
   return 0;
   }


int
_CancelButtonHandler(S_Button *b) {
   SF_UpgradeWindow *uw = (SF_UpgradeWindow *)b->parent;
   if ( uw->rootWindow )
      uw->Close();
   else {
      uw->StopThread();
      uw->Hide();
      if ( uw->func )
         (*uw->func)(uw->arg);
      }
   return 0;
   }



int
SF_UpgradeWindow::HandleCreateMsg() {
   hostName = NULL;
   port = 0;
   numMasks = 0;
   numFiles = NULL;
   fileList = NULL;
   packets = NULL;
   masks = NULL;
   func = NULL;
   arg = NULL;
   currentPacket = NULL;
   fileGroupBox = new S_GroupBox(this,"File");
   nameText = new S_StaticText(this,"Name: ");
   sizeText = new S_StaticText(this,"Size: ");
   rcvdText = new S_StaticText(this,"Rcvd: ");
   cancelButton = new S_Button(this,_CancelButtonHandler,"Cancel");
   return 0;
   }


int
SF_UpgradeWindow::DrawProgressBar() {
   DrawLine(x1,y1,x2,y1,1,0x808080);
   DrawLine(x1,y1,x1,y2,1,0x808080);
   DrawLine(x2,y1,x2,y2,1,0xFFFFFF);
   DrawLine(x1,y2,x2,y2,1,0xFFFFFF);

   double r = 0.0;
   for ( int i=0; i<numMasks; i++ ) {
      if ( numFiles )
      for ( int j=0; j<numFiles[i]; j++ ) {
         if ( totalNumFiles > 0 &&
              packets[i][j]->totalBytesRcvd>0 && packets[i][j]->packetSize>8 )
         r += (double)packets[i][j]->totalBytesRcvd/packets[i][j]->packetSize/
                      totalNumFiles;

         if ( r > ratio )
            ratio = r;
         if ( ratio > 0.0 ) {
            DrawFilledRect(x1+1,y1+1,x1+(int)((x2-x1-2)*ratio),y2-1,
                           RGB_DARKBLUE);
            DrawFilledRect(x1+(int)((x2-x1-2)*ratio)+1,y1+1,x2-1,y2-1,
                           backGroundRGB);
            }
         else
            DrawFilledRect(x1+1,y1+1,x2-1,y2-1,backGroundRGB);
         }
      }

   return 0;
   }


int
SF_UpgradeWindow::HandleSizeMsg() {
   int margin = 10;

   int cyText = 16;

   int cxGroupBox = cxClient - 2*margin;
   int cyGroupBox = 2*margin + 3*cyText;

   int cxText = cxGroupBox - 2*margin;

   int x = margin;
   int y = margin;

   fileGroupBox->SetSize(cxGroupBox,cyGroupBox);
   fileGroupBox->SetPosition(x,y);

   y += cyText;
   nameText->SetSize(cxText,cyText);
   nameText->SetPosition(x+margin,y);

   y += cyText;
   sizeText->SetSize(cxText,cyText);
   sizeText->SetPosition(x+margin,y);

   y += cyText;
   rcvdText->SetSize(cxText,cyText);
   rcvdText->SetPosition(x+margin,y);

   y += 2*margin + cyText;

   x1 = x;
   x2 = x + cxGroupBox;
   y1 = y;
   y2 = y+20;

   cancelButton->SetSize(50,35);
   cancelButton->SetPosition(cxClient/2-25,cyClient - margin - 35);

   return 0;
   }


int
SF_UpgradeWindow::HandlePaintMsg(void) {
   DrawFilledRect(0,0,cxClient,cyClient,backGroundRGB);
   DrawProgressBar();
   return 0;
   }


int
SF_UpgradeWindow::HandleCloseMsg() {
   return 0;
   }

SF_UpgradeWindow::~SF_UpgradeWindow() {
   StopThread();
   if ( cancelButton ) {
      delete cancelButton;
      cancelButton = NULL;
      }
   if ( nameText ) {
      delete nameText;
      nameText = NULL;
      }
   if ( sizeText ) {
      delete sizeText;
      sizeText = NULL;
      }
   if ( rcvdText ) {
      delete rcvdText;
      rcvdText = NULL;
      }
   if ( fileGroupBox ) {
      delete fileGroupBox;
      fileGroupBox = NULL;
      }
   if ( hostName ) {
      S_free(hostName);
      hostName = NULL;
      }
   }
