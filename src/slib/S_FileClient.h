/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
#ifndef S_FILECLIENT_INCLUDED
#define S_FILECLIENT_INCLUDED

class S_Packet;

#define S_FILECLIENT_OK                       0
#define S_FILECLIENT_PACKETPUSHFAILED    -11001
#define S_FILECLIENT_PACKETREADFAILED    -11002
#define S_FILECLIENT_PACKETWRITEFAILED   -11003
#define S_FILECLIENT_FILEDOESNOTEXIST    -10004
#define S_FILECLIENT_FILEISEMPTY         -11005
#define S_FILECLIENT_FILEOPENFAILED      -11006
#define S_FILECLIENT_FILEREADFAILED      -11007
#define S_FILECLIENT_FILEWRITEFAILED     -11008
#define S_FILECLIENT_MALLOCFAILED        -11009

#include "S_Client.h"

class S_FileClient : public S_Client
{
 public:
  char *ErrorString();
  
  S_FileClient();
  ~S_FileClient();
  
  virtual ulong RequestFileModDate(char *,int,char *);
  virtual int   RequestFileStatus(char *,int,char *);
  virtual int   RequestFileCheckSum(char *,int,char *,int=0,int=0);
  virtual slong  RequestFileSize(char *,int,char *);
  virtual int   RequestFile(char *,int,char *,S_Packet *);
  virtual int   RequestFileList(char *,int,char *,S_Packet *);
  virtual int   UploadFile(char *,int,char *,S_Packet *);

  int HTTPGet(char *,int,char *,S_Packet *);

  int WriteFile(char *,S_Packet *);
};

#endif
