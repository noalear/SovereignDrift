#ifndef S_SECUREFILECLIENT_INCLUDED
#define S_SECUREFILECLIENT_INCLUDED

#include "S_FileClient.h"

#define S_SECUREFILECLIENT_REGCODEFAILED        -11100


class S_SecureFileClient : public S_FileClient 
{
 public:
  
  int Init();
  S_SecureFileClient();
  int Create();
  int Destroy();
  virtual ~S_SecureFileClient();
  
  virtual int SecureRequestFile(int,char *,char *,int,char *,S_Packet *);
  virtual int SecureRequestFileCheckSum(int,char *,char *,int,char *,int=0,int=0);
  virtual int SecureRequestFileList(int,char *,char *,int,char *,S_Packet *);
  virtual int SecureRequestRegCodeDuration(int,char *,char *,int,S_Packet *);
};

#endif
