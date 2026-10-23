#ifndef S_SECUREFILESERVER_INCLUDED
#define S_SECUREFILESERVER_INCLUDED

#include "S_FileServer.h"
#include "S_RegCode.h"

#define S_SECUREFILESERVER_REQUESTFILE            11000
#define S_SECUREFILESERVER_REQUESTFILELIST        11004
#define S_SECUREFILESERVER_REQUESTREGCODEDURATION 11005
#define S_SECUREFILESERVER_REQUESTFILECHECKSUM    11006

class S_SecureFileServer : public S_FileServer , public S_RegCode
{
 public:
  
  int Init();
  S_SecureFileServer();
  S_SecureFileServer(int,int,char *,char *,int,char *);
  int Create(int,int,char *,char *,int,char *);
  int Destroy();
  virtual ~S_SecureFileServer();
  
  virtual int HandleUploadFileMsg(S_Client *);
  virtual int HandleRequestFileMsg(S_Client *);
  virtual int HandleRequestFileListMsg(S_Client *);
  virtual int HandleRequestFileCheckSumMsg(S_Client *);
  virtual int HandleRequestRegCodeDurationMsg(S_Client *,int * =NULL,
					      int * =NULL,char * =NULL);
};

#endif
