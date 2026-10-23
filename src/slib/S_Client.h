/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
#ifndef S_CLIENT
#define S_CLIENT

#include "S_Mutex.h"
#include "S_Tree.h"
#include "S_StopWatch.h"

#define S_CLIENT_OK                   0
#define S_CLIENT_TCPSOCKETFAILED     -1
#define S_CLIENT_UDPSOCKETFAILED     -2
#define S_CLIENT_BADIPADDRESS        -3
#define S_CLIENT_TCPCONNECTFAILED    -4
#define S_CLIENT_TCPDNSFAILED        -5
#define S_CLIENT_UDPBINDFAILED       -6
#define S_CLIENT_TCPCONNECTMSGFAILED -7
#define S_CLIENT_UDPCONNECTMSGFAILED -8
#define S_CLIENT_NOHOSTNAME          -9
#define S_CLIENT_CONNECTMSGFAILED   -10
#define S_CLIENT_REMOTEADDRFAILED   -11
#define S_CLIENT_BADPACKET          -12
#define S_CLIENT_RAWSOCKETFAILED    -13
#define S_CLIENT_RAWBINDFAILED      -14

#define S_CLIENT_NONBLOCK 1
#define S_CLIENT_BLOCK    2
#define S_CLIENT_TCP      4
#define S_CLIENT_UDP      8
#define S_CLIENT_RAW     16

#define S_CLIENT_TIMEOUT_USEC 60000000

#define S_SERVER_STOP    0xFFFFFFFF
#define S_CLIENT_RAWDATA 0xFFFFFFFF

#if defined(WIN32) || defined(OS2)
typedef int addrlenint;
#endif
#ifdef UNIX
typedef uint addrlenint;
#endif

class S_Packet;

#define S_CLIENT_MAXPACKET 128

#include "S_String.h"

class S_Client : public S_Tree
{
 public:
  slong rc;
  
  S_String error;
  virtual char *ErrorString();

  int stopConnect;
  
  int port;
  int tcpSocket;
  int udpSocket;
  int rawSocket;
  
  S_StopWatch sw;
  
  addrlenint remoteAddrLen;
  struct sockaddr_in udpRemoteAddr;
  struct sockaddr_in tcpRemoteAddr;
  struct sockaddr_in icmpRemoteAddr;
  struct sockaddr_in localAddr;
  char *numIPAddress;
  char *alphaIPAddress;
  
  void *puser;
  
  int Ping(char *,int,int);
  
  int SetNumIPAddress(struct sockaddr_in *);
  int DNSLookup(char *,struct sockaddr_in *);
  
  char sendpack[S_CLIENT_MAXPACKET];
  char recvpack[S_CLIENT_MAXPACKET];
  struct icmp *icp;
  
  int Init();
  S_Client();
  ~S_Client();
  
  S_Client(struct sockaddr_in *,int,int);
  
  int Connect(char *,int,ulong=S_CLIENT_RAWDATA);
  int Disconnect(void);
  
  ulong connectMsg;
  
  int ReadPacket(S_Packet *,int = S_CLIENT_TCP | S_CLIENT_BLOCK);
  int WritePacket(S_Packet *,int = S_CLIENT_TCP | S_CLIENT_BLOCK);
  int WriteVirtualPacket(S_Packet *,ulong,int = S_CLIENT_TCP | S_CLIENT_BLOCK);
  
  int ReadULong (ulong  *,int = S_CLIENT_BLOCK | S_CLIENT_TCP);
  int ReadLong  (slong   *,int = S_CLIENT_BLOCK | S_CLIENT_TCP);
  int ReadUShort(ushort *,int = S_CLIENT_BLOCK | S_CLIENT_TCP);
  int ReadShort (short  *,int = S_CLIENT_BLOCK | S_CLIENT_TCP);
  int ReadUChar (uchar  *,int = S_CLIENT_BLOCK | S_CLIENT_TCP);
  int ReadSChar (schar  *,int = S_CLIENT_BLOCK | S_CLIENT_TCP);
  
  int WriteULong (ulong ,int = S_CLIENT_TCP);
  int WriteLong  (slong  ,int = S_CLIENT_TCP);
  int WriteUShort(ushort,int = S_CLIENT_TCP);
  int WriteShort (short ,int = S_CLIENT_TCP);
  int WriteUChar (uchar ,int = S_CLIENT_TCP);
  int WriteSChar (schar ,int = S_CLIENT_TCP);
  
  int Recv(char *,int,int = S_CLIENT_BLOCK | S_CLIENT_TCP);
  int Send(char *,int,int = S_CLIENT_TCP);
  
  ulong usecTimeOut;
  int SetTimeOut(ulong);
};

/*
  class S_DNSCache {
  public:
  char *alias;
  struct in_addr sin_addr;
  S_Mutex mutex;
  
  S_DNSCache();
  ~S_DNSCache();
  };
*/

#endif
