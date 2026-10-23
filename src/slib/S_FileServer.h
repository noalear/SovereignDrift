#ifndef S_FILESERVER_INCLUDED
#define S_FILESERVER_INCLUDED

/* ***************** */
/* *** Msg codes *** */
/* ***************** */
#define S_FILESERVER_REQUESTFILE          10000
#define S_FILESERVER_REQUESTFILESTATUS    10001
#define S_FILESERVER_REQUESTFILESIZE      10002
#define S_FILESERVER_REQUESTFILEMODDATE   10003
#define S_FILESERVER_REQUESTFILELIST      10004
#define S_FILESERVER_UPLOADFILE           10005
#define S_FILESERVER_REQUESTFILECHECKSUM  10006

/* ******************** */
/* *** Return codes *** */
/* ******************** */
#define S_FILESERVER_OK                       0
#define S_FILESERVER_PACKETPUSHFAILED    -10001
#define S_FILESERVER_PACKETREADFAILED    -10002
#define S_FILESERVER_PACKETWRITEFAILED   -10003
#define S_FILESERVER_FILEDOESNOTEXIST    -10004
#define S_FILESERVER_FILEISEMPTY         -11005
#define S_FILESERVER_FILEOPENFAILED      -10006
#define S_FILESERVER_FILEREADFAILED      -10007
#define S_FILESERVER_FILEWRITEFAILED     -10008
#define S_FILESERVER_MALLOCFAILED        -10009

class S_Client;

#define S_FILESERVER_MAXFILENAMESIZE 63

typedef struct
{
  char filename[S_FILESERVER_MAXFILENAMESIZE+1];
  ulong exists;
  ulong size;
  ulong checkSum;
  ulong date;
} S_FileServer_FileListCache;

#include "S_StringArray.h"
#include "S_Server.h"
#include "S_Mutex.h"

class S_FileServer : public S_Server
{
 public:
  
  int numCachedFiles;
  int maxCachedFiles;
  int iniCachedFiles;
  int incCachedFiles;
  S_Mutex cacheMutex;
  S_FileServer_FileListCache *cache;
  
  ulong numFilesDownloaded;
  ulong numBytesDownloaded;
  ulong numFilesUploaded;
  ulong numBytesUploaded;
  
  ulong minCompSize;    // Minimum size for compressing a file
  S_StringArray inclusion; // strstr Filter for files to compress
  S_StringArray exclusion; // strstr Filter for file not to compress
  
  int Init();
  S_FileServer();
  S_FileServer(int,int=S_LOGGER_TOFILE,char * ="server.log");
  int Create(int,int=S_LOGGER_TOFILE,char * ="server.log");
  int Destroy();
  ~S_FileServer();
  
  virtual int ProcessConnectMsg(S_Client *);
  
  virtual int HandleRequestFileModDateMsg(S_Client *);
  virtual int HandleRequestFileStatusMsg(S_Client *);
  virtual int HandleRequestFileCheckSumMsg(S_Client *);
  virtual int HandleRequestFileSizeMsg(S_Client *);
  virtual int HandleRequestFileMsg(S_Client *);
  virtual int HandleRequestFileListMsg(S_Client *);
  virtual int HandleUploadFileMsg(S_Client *);
};

#endif
