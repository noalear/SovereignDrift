/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
#ifndef S_SERVER
#define S_SERVER

#include "S_Tree.h"
#include "S_Mutex.h"
#include "S_Logger.h"
#include "S_StringArray.h"

#define S_SERVER_REQUESTTIME      1001

#define S_SERVER_OK 0
#define S_SERVER_TCPIPINITFAILED    -1
#define S_SERVER_TCPSOCKETFAILED    -2
#define S_SERVER_UDPSOCKETFAILED    -3
#define S_SERVER_TCPBINDFAILED      -4
#define S_SERVER_UDPBINDFAILED      -5
#define S_SERVER_TCPLISTENFAILED    -6
#define S_SERVER_TCPACCEPTFAILED    -7
#define S_SERVER_CLIENTFAILED       -8
#define S_SERVER_CLIENTTHREADFAILED -9

class S_Client;

class S_Server : public S_Tree, public S_Logger
{
 public:
  slong rc;
  int tcpSocket;
  int udpSocket;
  
  S_Mutex denyIPListMutex;
  int dynamicDenyIPList;
  int LoadDenyIPList(char * ="server.deny");
  S_StringArray denyIPList;
  S_StringArray denyIPReason;
  
  ulong hits;
  
  int port;
  void *arg;
  int (*pConnectMsgHandler)(S_Server *,S_Client *);
  
  virtual int HandleRequestTimeMsg(S_Client *);
  
  virtual int WaitingForConnection();
  virtual int DoneWaitingForConnection();
  virtual int AcceptFailed();
  virtual int NewClientDenied(S_Client *);
  virtual int NewClientFailed();
  virtual int ClientThreadFailed();
  virtual int NormalExit();
  
  virtual int ProcessConnectMsg(S_Client *);
  
  int Init();
  S_Server();
  S_Server(int,int (*)(S_Server *,S_Client *) =NULL,void * =NULL,
	   int=S_LOGGER_TOFILE,char * ="server.log");
  int Create(int,int (*)(S_Server *,S_Client *) =NULL,void * =NULL,
	 int=S_LOGGER_TOFILE,char * ="server.log");
  int Destroy();
  ~S_Server(void);
  
  int continueRunning;
  virtual int Run();
  virtual int Stop();
  int WaitForChildren(ulong=5000000);
  
};

#endif
