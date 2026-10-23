#ifndef SF_NEXUS_INCLUDED
#define SF_NEXUS_INCLUDED

#define SF_NEXUS_CONTACT_TIME  120  /* 2 minutes in seconds */
#define SF_NEXUS_DROP_TIME     360  /* 6 minutes in seconds */

#define SF_NEXUS_SERVER_DEAD   0
#define SF_NEXUS_SERVER_ALIVE  1

#define SF_NEXUS_PORT      20000

#define SF_NEXUS_OK                   0
#define SF_NEXUS_GETVERSION           1
#define SF_NEXUS_DOWNLOADFILE         3

#define SF_NEXUS_GETFILE              0
#define SF_NEXUS_GETFILEEXISTS        1
#define SF_NEXUS_GETFILESIZE          2

#define SF_NEXUS_GETBANNERAD         10
#define SF_NEXUS_SERVERSTOP          12
#define SF_NEXUS_SERVERLIST          13
#define SF_NEXUS_SERVERSTART         15
#define SF_NEXUS_STOP                16

#define SF_NEXUS_NEWPLAYER           17
#define SF_NEXUS_GETPLAYERDATA       18
#define SF_NEXUS_SETPLAYERDATA       19
#define SF_NEXUS_GETHIGHSCORELIST    20
#define SF_NEXUS_PLAYEREXISTS        21
#define SF_NEXUS_GETPLAYERLIST       22
#define SF_NEXUS_GETSORTEDSTATS      23
#define SF_NEXUS_GETPLAYERDATA2      24

#define SF_NEXUS_FILEDOESNOTEXIST     3
#define SF_NEXUS_NAMEALREADYEXISTS   -1
#define SF_NEXUS_NAMEDOESNOTEXIST    -2
#define SF_NEXUS_WRONGPASSWORD       -3
#define SF_NEXUS_CONNECTFAILED       -4
#define SF_NEXUS_NOHIGHSCORELIST     -5
#define SF_NEXUS_MALFORMEDREQUEST    -6
#define SF_NEXUS_MALLOCFAILED        -7
#define SF_NEXUS_READFAILED          -8

typedef struct {
   char string[256];
   char ipaddr[256];
   char port[256];
   char state;
   time_t timeLastContact;
   } ServerStruct;

class SF_Nexus : public S_FileServer {
public:
   int run;

   int maxServers;
   int numServers;
   ServerStruct *servers;

   S_Thread *thread;

   int contactTime;
   int dropTime;

   S_String backupHostname;
   S_Thread backupThread;

   int GetBannerAdInfo(char *);
   char *bmpBannerFilename;
   char *urlBanner;

   virtual int AcceptFailed();
   virtual int NewClientFailed();
   virtual int NewClientDenied(S_Client *);
   virtual int ClientThreadFailed();

   SF_Nexus(int=0,char * =NULL);
   ~SF_Nexus();

   S_Mutex playerRecordMutex;
   S_Mutex serverListMutex;
   S_Mutex sortedListMutex;
   int UpdateSortedList(char *,char *,char *);

   int DownloadFile(S_Client *);

   virtual int ProcessConnectMsg(S_Client *);

   int GetVersion(S_Client *);
   int GetBannerAd(S_Client *);
   int GetHighScoreList(S_Client *);
   int PlayerData(S_Client *);
   int ServerStart(S_Client *);
   int ServerStop(S_Client *);
   int ServerList(S_Client *);
   int GetPlayerList(S_Client *);
   int GetSortedStats(S_Client *);

   int AddPlayer(char *name);
   int RemovePlayer(char *name);

   int Load();
   int Save();
   int AddServer(char *);
   int RemoveServer(int);
   int ActivateServer(char *,int);
   int Profile();
   
   };

int SF_Nexus_BackupThread(void *);

#endif
