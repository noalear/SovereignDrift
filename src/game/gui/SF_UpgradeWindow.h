#ifndef SF_UPGRADEWINDOW_INCLUDED
#define SF_UPGRADEWINDOW_INCLUDED

class SF_UpgradeWindow : public S_Window {
public:
   int upgradeSucceeded;
   char *hostName;
   int port;

   int (*func)(void *);
   void *arg;

   int numMasks;
   char **masks;

   S_Packet *currentPacket;
   char *currentFileName;
   int currentFile;
   int totalNumFiles;

   int *numFiles;
   S_Packet **fileList;
   S_Packet ***packets;

   double ratio;
   int drawProgressBar;
   int DrawProgressBar();

   S_Button *cancelButton;

   int x1,y1,x2,y2;
   S_GroupBox *fileGroupBox;
   S_StaticText *nameText;
   S_StaticText *sizeText;
   S_StaticText *rcvdText;
   S_Thread upgradeThread;
   S_Thread progressThread;

   int StartThread(char *,int,char **,int,int(*)(void *),void * = NULL);
   int StopThread();

   SF_UpgradeWindow(S_Root *);
   SF_UpgradeWindow(S_Window *);
   ~SF_UpgradeWindow();

   int HandleCreateMsg();
   int HandleSizeMsg();
   int HandlePaintMsg();
   int HandleCloseMsg();
   };

#endif
