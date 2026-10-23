/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
#include "S_OS.h"
#ifdef SFHOST
#  include "sf_net_api.h"   /* port 2026-10-12: portable sockets */
#endif
#include "S_Packet.h"
#include "S_Thread.h"
#include "S_StopWatch.h"
#include "S_Client.h"


inline int
_TcpipSetSocketBlockingMode( slong sock,int flag )
{
  flag = !flag;
# ifdef WIN32
  ioctlsocket (sock,FIONBIO,(ulong *)&flag);
# endif
# ifdef OS2
  ioctl (sock,FIONBIO,(char*)&flag,sizeof(int));   
# endif
# ifdef UNIX
  ioctl (sock,FIONBIO,(char*)&flag,sizeof(int));   
# endif
  return(0);
}


inline int
_TcpipNumBytesToReadInSocket( slong sock )
{
  int numBytes = 0;
# ifdef WIN32
  ioctlsocket (sock,FIONREAD,(ulong*)&numBytes);
# endif
# ifdef OS2
# ifdef IBMTCPIP
  ioctl (sock,FIONREAD,(char*)&numBytes,sizeof(int));   
# elif GNUCPP
  ioctl (sock,FIONREAD,(char*)&numBytes,sizeof(int));
# endif
# endif
# ifdef UNIX
  ioctl (sock,FIONREAD,(char*)&numBytes,sizeof(int));   
# endif
  return(numBytes);
}


int
S_Client::SetTimeOut(ulong iusecTimeOut ) 
{
  usecTimeOut = iusecTimeOut;
  return 0;
}


int _tcpMaxChunkSize = 32767;

int
S_Client::Recv(char *buffer,int totalBytesToRecv, int flag)
{
  int totalBytesRcvd = 0;
  
  int sock = 0;
  struct sockaddr_in *remoteAddr = NULL;
  if ( flag & S_CLIENT_TCP )
    {
      sock = tcpSocket;
      remoteAddr = &tcpRemoteAddr;
    }
  else if ( flag & S_CLIENT_UDP )
    {
      sock = udpSocket;
      remoteAddr = &udpRemoteAddr;
    }
  else if ( flag & S_CLIENT_RAW )
    {
      sock = rawSocket;
      remoteAddr = &icmpRemoteAddr;
    }
  
  if ( totalBytesToRecv <= 0 || 
       ( flag & S_CLIENT_NONBLOCK && 
	 _TcpipNumBytesToReadInSocket(sock) <= 0) )
    {
      return 0;
    }
  
  int readAgain;
  sw.Start();
  do
    {
      int totalBytesLeftToRecv = totalBytesToRecv - totalBytesRcvd;
      int numBytesToRecv = min(_tcpMaxChunkSize,totalBytesLeftToRecv);
      rc = recvfrom(sock,buffer+totalBytesRcvd,numBytesToRecv,0,
		    (sockaddr *)remoteAddr,&remoteAddrLen);
      if ( rc > 0 )
	{
	  sw.Start();
	  totalBytesRcvd += rc;
	}
      readAgain = /*rc >= 0 &&*/ totalBytesRcvd < totalBytesToRecv && 
	flag & S_CLIENT_BLOCK && sw.Stop() < usecTimeOut;
      if ( readAgain ) _OsSleep(10000);
    } while ( readAgain );
  
  rc = 0; /* *** Don't return error condition *** */
  
  return totalBytesRcvd;
}


int
S_Client::Send(char *buffer,int totalBytesToSend,int flag)
{
  if ( totalBytesToSend <= 0 )
    return 0;
  
  rc = 1;
  int totalBytesSent = 0;
  
  int sock = 0;
  struct sockaddr_in *remoteAddr = NULL;
  if ( flag & S_CLIENT_TCP )
    {
      sock = tcpSocket;
      remoteAddr = &tcpRemoteAddr;
    }
  else if ( flag & S_CLIENT_UDP )
    {
      sock = udpSocket;
      remoteAddr = &udpRemoteAddr;
    }
  else if ( flag & S_CLIENT_RAW )
    {
      sock = rawSocket;
      remoteAddr = &icmpRemoteAddr;
    }
  remoteAddrLen = sizeof(struct sockaddr_in);
  int writeAgain;
  sw.Start();
  do
    {
      int totalBytesLeftToSend = totalBytesToSend - totalBytesSent;
      int numBytesToSend = min(_tcpMaxChunkSize,totalBytesLeftToSend);
      rc = sendto(sock,buffer+totalBytesSent,numBytesToSend,0,
                  (sockaddr *)remoteAddr,remoteAddrLen);
      if ( rc >= 0 )
	{
	  totalBytesSent += rc;
	  sw.Start();
	}
      
      writeAgain = /*rc >= 0 &&*/ totalBytesSent < totalBytesToSend &&
	sw.Stop() < usecTimeOut &&
	totalBytesToSend > _tcpMaxChunkSize;
      
      if ( writeAgain )
	_OsSleep(10000);
    } while ( writeAgain );
  
  rc = 0; /* *** Don't return error condition *** */
  
  return totalBytesSent;
}


int
S_Client::ReadPacket(S_Packet *packet,int flag)
{
  
  if ( !packet )
    return FALSE;
  
  rc = S_CLIENT_BADPACKET;
  packet->readComplete = FALSE;
  
  if ( !packet->buffer )
    {
      packet->GrowBuffer(packet->headerSize);
      packet->packetSize = packet->headerSize;
      packet->uncompPacketSize = packet->headerSize;
      packet->Rewind();
      packet->SetPacketSize();
      packet->readComplete = TRUE;
      return packet->readComplete;
    }
  
  rc = S_CLIENT_OK;
  
  int sock = 0;
  struct sockaddr_in *remoteAddr = NULL;
  if ( flag & S_CLIENT_TCP )
    {
      sock = tcpSocket;
      remoteAddr = &tcpRemoteAddr;
    }
  else if ( flag & S_CLIENT_UDP )
    {
      sock = udpSocket;
      remoteAddr = &udpRemoteAddr;
    }
  
  /* ******************************************************************* */
  /* *** If packet is empty then peek and see if header is available *** */
  /* *** and how much data is to be eventually read (bytesInPacket)  *** */
  /* ******************************************************************* */
  int peekAgain;
  ulong bytesInPacket = packet->packetSize;
  sw.Start();
  if ( packet->packetSize == packet->headerSize )
    {
      do
	{
	  *((ulong *)(packet->buffer)) = 0;
	  if ( _TcpipNumBytesToReadInSocket(sock) >= 
	       (int)packet->headerSize ) {
            rc = recvfrom(sock,(char *)packet->buffer,
                          packet->headerSize,MSG_PEEK,
			  (sockaddr *)remoteAddr,&remoteAddrLen);
            if (  sock == udpSocket ||
		  (sock == tcpSocket && rc == (int)packet->headerSize) )
	      bytesInPacket = ntohl( *((ulong *)(packet->buffer)) );
	  }
	  peekAgain = /*rc >= 0 &&*/ bytesInPacket <= packet->headerSize && 
	    flag & S_CLIENT_BLOCK && sw.Stop() < usecTimeOut;
	  if ( peekAgain ) _OsSleep(10000);
	} while ( peekAgain );
    }
  
  /* *** No need to return rc < 0 which indicates an error *** */
  rc = 0;
  
  /* *** If there's no header yet then exit if non-blocking *** */
  if ( bytesInPacket <= packet->headerSize && flag & S_CLIENT_NONBLOCK )
    {
      return FALSE;
    }
  
  /* ********************************************************************* */
  /* *** If we get past this line then we know the header can be read  *** */
  /* *** and we also know the size of the packet.  So make room.  Exit *** */
  /* *** if the GrowBuffer fails (returns FALSE).                      *** */
  /* ********************************************************************* */
  if ( !packet->GrowBuffer(bytesInPacket) )
    {
      packet->ShrinkBuffer();
      return FALSE;
    }
  
  /* *** Now see how many bytes are actually available to be read *** */
  sw.Start();
  int bytesInSock = _TcpipNumBytesToReadInSocket(sock);
  while ( bytesInSock == 0 && flag & S_CLIENT_BLOCK &&
	  sw.Stop() < usecTimeOut)
    {
      bytesInSock = _TcpipNumBytesToReadInSocket(sock);
      if ( bytesInSock == 0 ) _OsSleep(10000);
    }
  
  if ( bytesInSock > (int)bytesInPacket )
    bytesInSock = bytesInPacket;
  
  int readAgain;
  
  sw.Start();
  /* *** packetSize == headerSize just after a flush() *** */
  if ( packet->packetSize == packet->headerSize )
    {
      do
	{
	  int totalBytesLeftToRecv = bytesInSock - packet->totalBytesRcvd;
	  int numBytesToRecv = min(_tcpMaxChunkSize,totalBytesLeftToRecv);
	  rc = recvfrom(sock,(char *)packet->buffer+packet->totalBytesRcvd,
			numBytesToRecv,0,
			(sockaddr *)remoteAddr,&remoteAddrLen);
	  if ( rc > 0 )
	    {
	      sw.Start();
	      packet->totalBytesRcvd += rc;
	    }
	  
	  if ( packet->totalBytesRcvd >= packet->headerSize )
	    {
	      ulong *ul = (ulong *)packet->buffer;
	      packet->packetSize = ntohl(*ul);
	      ul = (ulong *)(packet->buffer + sizeof(ulong));
	      packet->uncompPacketSize = ntohl(*ul);
	      packet->GrowBuffer(packet->uncompPacketSize);
            }
	  readAgain =/*rc >= 0 &&*/ 
	    packet->totalBytesRcvd<packet->headerSize &&
	    flag & S_CLIENT_BLOCK && sw.Stop() < usecTimeOut;
	  if ( readAgain )
            _OsSleep(10000);
	} while ( readAgain );
    }
  
  sw.Start();
  /* *** We grew it to handle as many bytes as are in the socket *** */
  if ( packet->packetSize > packet->headerSize && 
       packet->totalBytesRcvd < packet->packetSize )
    {
      do
	{
	  int totalBytesLeftToRecv = packet->packetSize-packet->totalBytesRcvd;
	  int numBytesToRecv = min(_tcpMaxChunkSize,totalBytesLeftToRecv);
	  rc = recvfrom(sock,(char *)packet->buffer+packet->totalBytesRcvd,
			numBytesToRecv,0,
			(sockaddr *)remoteAddr,&remoteAddrLen);
	  if ( rc > 0 )
	    {
	      sw.Start();
	      packet->totalBytesRcvd += rc;
	    }
	  
	  readAgain =/*rc>=0 &&*/ packet->totalBytesRcvd<packet->packetSize && 
	    flag & S_CLIENT_BLOCK && sw.Stop() < usecTimeOut;
	  if ( readAgain )
            _OsSleep(10000);
	  
	} while ( readAgain );
    }
  
  rc = 0; /* *** Don't return error condition *** */
  
  packet->readComplete = packet->packetSize > packet->headerSize && 
    packet->totalBytesRcvd == packet->packetSize;
  
  if (packet->readComplete && packet->packetSize!=packet->uncompPacketSize)
    {
      packet->UnCompress();
      packet->totalBytesRcvd = packet->packetSize;
    }
  
  packet->index = packet->headerSize;
  return ( packet->readComplete );
}


int
S_Client::WritePacket(S_Packet *packet,int flag)
{
  int bytesSent = 0;
  if ( packet->packetSize > packet->headerSize )
    {
      packet->SetPacketSize();
      bytesSent = Send((char *)packet->buffer,packet->packetSize,flag);
    }
  else
    return 0;
  packet->index = bytesSent;
  return -(int)(!(bytesSent == (int)packet->packetSize));
}


int
S_Client::WriteVirtualPacket(S_Packet *packet,ulong vSize,int flag)
{
  int bytesSent = 0;
  if ( packet->packetSize > packet->headerSize )
    {
      packet->SetPacketSize();
      bytesSent = Send((char *)packet->buffer,vSize,flag);
    }
  else
    return 0;
  packet->index = bytesSent;
  return -(int)(!(bytesSent == (int)vSize));
}


int
S_Client::ReadLong(slong *l,int flag)
{
  return ReadULong((ulong *)l,flag);
}


int
S_Client::ReadULong(ulong *ul,int flag)
{
  int bytesRcvd = Recv((char *)ul,sizeof(ulong),flag);
  if ( bytesRcvd == sizeof(ulong) )
    *ul = ntohl(*ul);
  return -(int)(!(bytesRcvd == sizeof(ulong)));
}


int
S_Client::WriteLong(slong l,int flag)
{
  return WriteULong((ulong)l,flag);
}


int
S_Client::WriteULong(ulong ul,int flag)
{
  ul = htonl(ul);
  int bytesSent = Send((char *)&ul,sizeof(ulong),flag);
  return -(int)(!(bytesSent == sizeof(ulong)));
}


int
S_Client::ReadShort(short *s,int flag)
{
  return ReadUShort((ushort *)s,flag);
}


int
S_Client::ReadUShort(unsigned short *us,int flag)
{
  int bytesRcvd = Recv((char *)us,sizeof(unsigned short),flag);
  if ( bytesRcvd == sizeof(unsigned short) )
    *us = ntohs(*us);
  return -(int)(!(bytesRcvd == sizeof(unsigned short)));
}


int
S_Client::WriteShort(short s,int flag)
{
  return WriteUShort((ushort)s,flag);
}


int
S_Client::WriteUShort(unsigned short us,int flag)
{
  us = htons(us);
  int bytesSent = Send((char *)&us,sizeof(unsigned short),flag);
  return -(int)(!(bytesSent == sizeof(unsigned short)));
}


int
S_Client::ReadSChar(schar *c,int flag)
{
  return ReadUChar((uchar *)c,flag);
}


int
S_Client::ReadUChar(unsigned char *uc,int flag)
{
  int bytesRcvd = Recv((char *)uc,sizeof(unsigned char),flag);
  return -(int)(!(bytesRcvd == sizeof(unsigned char)));
}


int
S_Client::WriteSChar(schar c,int flag)
{
  return WriteUChar((uchar)c,flag);
}


int
S_Client::WriteUChar(unsigned char uc,int flag)
{
  int bytesSent = Send((char *)&uc,sizeof(unsigned char),flag);
  return -(int)(!(bytesSent == sizeof(unsigned char)));
}


int
S_Client::Init()
{
  _OsTcpipInit();
  tcpSocket = 0;
  udpSocket = 0;
  rawSocket = 0;
  numIPAddress = NULL;
  alphaIPAddress = NULL;
  connectMsg = 0;
  remoteAddrLen = sizeof(sockaddr);
  usecTimeOut = S_CLIENT_TIMEOUT_USEC;
  return 0;
}


S_Client::S_Client(void)
 {
   Init();
 }


S_Client::S_Client(struct sockaddr_in *iremoteAddr,
		   int itcpSocket,
                   int serverUdpSocket)
{
  
  Init();
  
  tcpRemoteAddr = *iremoteAddr;
  udpRemoteAddr = *iremoteAddr;
  tcpSocket = itcpSocket;
  udpSocket = serverUdpSocket;
  
  int optval = 0x100000;
  setsockopt(tcpSocket,SOL_SOCKET,SO_SNDBUF,(char*)&optval, sizeof(optval));
  
  _TcpipSetSocketBlockingMode(tcpSocket,FALSE);
  _TcpipSetSocketBlockingMode(udpSocket,FALSE);
  
  sw.Start();
  do
    {
      rc = Recv((char *)&udpRemoteAddr,sizeof(udpRemoteAddr),
		S_CLIENT_NONBLOCK | S_CLIENT_TCP);
      if ( rc <= 0 )
	_OsSleep(100000);
    } while ( rc <= 0 && sw.Stop() < usecTimeOut );
  
  if ( rc < 0 )
    {
      rc = S_CLIENT_REMOTEADDRFAILED;
      return;
    }
  
  udpRemoteAddr.sin_addr = tcpRemoteAddr.sin_addr;
  
  udpSocket = socket(AF_INET,SOCK_DGRAM,0);
  if ( udpSocket < 0 )
    {
      rc = S_CLIENT_UDPSOCKETFAILED;
      return;
    }
  
  setsockopt(udpSocket,SOL_SOCKET,SO_SNDBUF,(char*)&optval, sizeof(optval));
  
  /* *** Bind the UDP socket to the port *** */
  localAddr.sin_family = AF_INET;
  localAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  localAddr.sin_port = htons(0);
  int opt = TRUE;
  setsockopt(udpSocket,SOL_SOCKET,SO_REUSEADDR,(char *)&opt,sizeof(int));
  rc = bind(udpSocket,(struct sockaddr *)&localAddr,sizeof(localAddr));
  if ( rc < 0 )
    {
      rc = S_CLIENT_UDPBINDFAILED;
      return;
    }
  
  addrlenint len = sizeof(localAddr);
  getsockname(udpSocket,(struct sockaddr *)&localAddr,&len);
  
  rc = Send((char *)&localAddr,sizeof(localAddr));
  
  numIPAddress = NULL;
  alphaIPAddress = NULL;
  connectMsg = 0;
  
  SetNumIPAddress(&tcpRemoteAddr);
  
  /*
    struct hostent *host;
    host = gethostbyaddr((char *)&tcpRemoteAddr.sin_addr,
    sizeof(tcpRemoteAddr.sin_addr),AF_INET);
    if ( host != (struct hostent *)NULL ) {
    alphaIPAddress = (char *) S_malloc( strlen(host->h_name) + 1 );
    if ( alphaIPAddress != NULL )
    strcpy(alphaIPAddress,host->h_name);
    }
  */
  
  sw.Start();
  do
    {
      rc = ReadULong(&connectMsg,S_CLIENT_NONBLOCK | S_CLIENT_TCP);
      if ( rc < 0 )
      _OsSleep(100000);
    } while ( rc < 0 && sw.Stop() < usecTimeOut  );
  
  if ( rc < 0 )
    {
      rc = S_CLIENT_CONNECTMSGFAILED;
      return;
    }
  
  rc = S_CLIENT_OK;
}


int
S_Client::Disconnect(void)
{
  if ( alphaIPAddress != NULL )
    {
      S_free(alphaIPAddress);
      alphaIPAddress = NULL;
    }
  if ( numIPAddress != NULL )
    {
      S_free(numIPAddress);
      numIPAddress = NULL;
    }
  if ( tcpSocket > 0 )
    {
      _CloseSocket(tcpSocket);
      tcpSocket = 0;
    }
  if ( udpSocket > 0 )
    {
      _CloseSocket(udpSocket);
      udpSocket = 0;
    }
  if ( rawSocket > 0 )
    {
      _CloseSocket(rawSocket);
      rawSocket = 0;
    }
  return 0;
}


S_Client::~S_Client(void)
{
  Disconnect();
}


typedef struct
{
  int socket;
  struct sockaddr_in *remoteAddr;
  int rc;
} _ConnectThreadArgs;


int
_ConnectThread(void *iarg)
{
  _ConnectThreadArgs *arg = (_ConnectThreadArgs *)iarg;
  arg->rc = connect(arg->socket,(struct sockaddr *)arg->remoteAddr,
		    sizeof(sockaddr_in));
  return 0;
}


/*
  S_DNSCache::S_DNSCache() {
  alias = NULL;
  }
  
S_DNSCache::~S_DNSCache() {
if ( alias ) {
S_free(alias);
alias = NULL;
}
}

S_DNSCache _dnsCache;
*/


#define S_CLIENT_MAXADDRLEN 32
typedef struct
{
  int rc;
  char *serverName;
  int h_addrtype;
  char h_addr_list[1][S_CLIENT_MAXADDRLEN];
  int h_length;
} _DNSThreadArgs;



int
_DNSThread(void *iarg)
{
  _DNSThreadArgs *arg = (_DNSThreadArgs *)iarg;
  
  struct hostent *host = gethostbyname( (char *)arg->serverName );
  
  if ( (int)host>=0 && (int)host<=5 )
    {
      arg->rc = S_CLIENT_BADIPADDRESS;
      return arg->rc;
    }
  
  arg->h_addrtype = host->h_addrtype;
  arg->h_length = host->h_length;
  for ( int i=0; i<arg->h_length; i++ )
    arg->h_addr_list[0][i] = host->h_addr_list[0][i];
  
  arg->rc = 0;
  return (arg->rc);
}


int
S_Client::SetNumIPAddress(struct sockaddr_in *pRemoteAddr)
{
  
  if ( numIPAddress )
    S_free(numIPAddress);
  
  numIPAddress=(char *)S_malloc(strlen(inet_ntoa(pRemoteAddr->sin_addr))+1 );
  if ( numIPAddress != NULL )
    {
      strcpy(numIPAddress,inet_ntoa(pRemoteAddr->sin_addr));
    }
  
  return 0;
}


int
S_Client::DNSLookup(char *ihostname,struct sockaddr_in *pRemoteAddr)
{
  
  /* ******************************************************************* */
  /* *********** Address name resolution to be done in a thread ******** */
  /* ******************************************************************* */
  _DNSThreadArgs tcpDNSArgs;
  tcpDNSArgs.serverName = ihostname;
  tcpDNSArgs.rc = -2;
  tcpDNSArgs.h_addrtype = 0;
  tcpDNSArgs.h_length = 0;
  
  S_Thread dnsThread(_DNSThread,(void *)&tcpDNSArgs);
  
  sw.Start();
  while (tcpDNSArgs.rc==-2 && sw.Stop()<usecTimeOut && !stopConnect)
    _OsSleep(100000);
  
  rc = tcpDNSArgs.rc;
  
  if ( rc == -2 )
    {
      dnsThread.Stop();
    }
  
  if ( rc < 0 )
    {
      rc = S_CLIENT_TCPDNSFAILED;
      return rc;
    }
  
  pRemoteAddr->sin_family = (short)tcpDNSArgs.h_addrtype;
  if ( tcpDNSArgs.h_addrtype == AF_INET &&
       tcpDNSArgs.h_addr_list && 
       tcpDNSArgs.h_length > 0)
    {
      bcopy( tcpDNSArgs.h_addr_list[0],
	     (caddr_t) &pRemoteAddr->sin_addr,
	     tcpDNSArgs.h_length);
      
      /*
	_dnsCache.mutex.Request();
	_dnsCache.sin_addr = pRemoteAddr->sin_addr;
	if ( _dnsCache.alias )
	S_free(_dnsCache.alias);
	if ( ihostname )
	_dnsCache.alias = S_strdup(ihostname);
	_dnsCache.mutex.Release();
      */
    }
  else
    {
      rc = S_CLIENT_TCPDNSFAILED;
      return rc;
    }
  
  return rc;
}


int
S_Client::Connect(char *serverName, int port, ulong connectMsg)
{

  stopConnect = FALSE;
  
  if ( serverName == NULL )
    return S_CLIENT_NOHOSTNAME;
  
  tcpSocket = socket(AF_INET,SOCK_STREAM,0);
  if ( tcpSocket < 0 )
    {
      rc = S_CLIENT_TCPSOCKETFAILED;
      return rc;
    }
  
  int optval = 0x100000;
  setsockopt(tcpSocket,SOL_SOCKET,SO_SNDBUF,(char*)&optval, sizeof(optval));
  
  udpSocket = socket(AF_INET,SOCK_DGRAM,0);
  if ( udpSocket < 0 ) 
    {
      rc = S_CLIENT_UDPSOCKETFAILED;
      return rc;
    }
  
  setsockopt(udpSocket,SOL_SOCKET,SO_SNDBUF,(char*)&optval, sizeof(optval));
  
  localAddr.sin_family = AF_INET;
  localAddr.sin_port = htons(0);
  localAddr.sin_addr.s_addr = INADDR_ANY;
  int opt = TRUE;
  setsockopt(udpSocket,SOL_SOCKET,SO_REUSEADDR,(char *)&opt,sizeof(int));
  rc=bind(udpSocket,(struct sockaddr *)&localAddr,sizeof(localAddr));
  if ( rc < 0 )
    {
      rc = S_CLIENT_UDPBINDFAILED;
      return rc;
    }
  
  tcpRemoteAddr.sin_family = AF_INET;
  tcpRemoteAddr.sin_port = htons((short)port);
  
  /*
    _dnsCache.mutex.Request();
    if ( _dnsCache.alias && serverName && !strcmp(_dnsCache.alias,serverName) )
    tcpRemoteAddr.sin_addr = _dnsCache.sin_addr;
    else
    tcpRemoteAddr.sin_addr.s_addr = inet_addr( (char *)serverName );
    _dnsCache.mutex.Release();
  */
  
  tcpRemoteAddr.sin_addr.s_addr = inet_addr( (char *)serverName );
  
  if ( tcpRemoteAddr.sin_addr.s_addr == INADDR_NONE )
    {
      rc = DNSLookup(serverName,&tcpRemoteAddr);
      
      if ( rc < 0 )
	{
	  _CloseSocket(tcpSocket);
	  _CloseSocket(udpSocket);
	  tcpSocket = 0;
	  udpSocket = 0;
	  return rc;
	}
      
    }
  
  udpRemoteAddr = tcpRemoteAddr;
  
  /* ********************************************************************** */
  /* ******************* Use a thread for call to connect() *************** */
  /* ********************************************************************** */
  _ConnectThreadArgs tcpConnectArgs;
  tcpConnectArgs.socket = tcpSocket;
  tcpConnectArgs.remoteAddr = &tcpRemoteAddr;
  tcpConnectArgs.rc = -2;
  
  S_Thread connectThread(_ConnectThread,(void *)&tcpConnectArgs);
  
  sw.Start();
  while (tcpConnectArgs.rc==-2 && sw.Stop()<usecTimeOut && !stopConnect)
    _OsSleep(100000);
  
  rc = tcpConnectArgs.rc;
  
  if ( rc == -2 || stopConnect ) 
    {
      connectThread.Stop();
    }
  
  /* ********************************************************************** */
  
  if ( rc < 0 )
    {
      rc = S_CLIENT_TCPCONNECTFAILED;
      _CloseSocket(tcpSocket);
      _CloseSocket(udpSocket);
      tcpSocket = 0;
      udpSocket = 0;
      return rc;
    }
  
  /* *** Set socket to non-blocking *** */
  _TcpipSetSocketBlockingMode(tcpSocket,FALSE);
  
  /* *** Set socket to non-blocking *** */
  _TcpipSetSocketBlockingMode(udpSocket,FALSE);
  
  if ( connectMsg != S_SERVER_STOP && connectMsg != S_CLIENT_OK )
    {
      
      addrlenint len = sizeof(localAddr);
      getsockname(tcpSocket,(struct sockaddr *)&localAddr,&len);
      
      rc = Send((char *)&localAddr,sizeof(localAddr));
      if ( rc <= 0 )
	{ 
	  rc = S_CLIENT_UDPCONNECTMSGFAILED;
	  _CloseSocket(tcpSocket);
	  _CloseSocket(udpSocket);
	  tcpSocket = 0;
	  udpSocket = 0;
	  return rc;
	}
      
      rc = Recv((char *)&udpRemoteAddr,sizeof(udpRemoteAddr));
      udpRemoteAddr.sin_addr = tcpRemoteAddr.sin_addr;
      if ( rc <= 0 )
	{
	  rc = S_CLIENT_UDPCONNECTMSGFAILED;
	  _CloseSocket(tcpSocket);
	  _CloseSocket(udpSocket);
	  tcpSocket = 0;
	  udpSocket = 0;
	  return rc;
	}
      
      rc = WriteULong(connectMsg);
      if ( rc != 0 )
	{
	  rc = S_CLIENT_TCPCONNECTMSGFAILED;
	  _CloseSocket(tcpSocket);
	  _CloseSocket(udpSocket);
	  tcpSocket = 0;
	  udpSocket = 0;
	  return rc;
	}
      
    }
  
  rc = S_CLIENT_OK;
  return rc;
}


static ushort
in_cksum(ushort *addr, int len)
{
  int nleft = len;
  ushort *w = addr;
  ulong sum = 0;
  ushort answer = 0;
  
  /*
   * Our algorithm is simple, using a 32 bit accumulator (sum), we add
   * sequential 16 bit words to it, and at the end, fold back all the
   * carry bits from the top 16 bits into the lower 16 bits.
   */
  while (nleft > 1)
    {
      sum += *w++;
      nleft -= 2;
    }
  
  /* mop up an odd byte, if necessary */
  if (nleft == 1)
    {
      answer=0;
      *(uchar *)(&answer) = *(uchar *)w ;
      sum += answer;
    }
  
  /* add back carry outs from top 16 bits to low 16 bits */
  sum = (sum >> 16) + (sum & 0xffff);	/* add hi 16 to low 16 */
  sum += (sum >> 16);			/* add carry */
  answer = (ushort)~sum;		/* truncate to 16 bits */
  return(answer);
}


int
S_Client::Ping( char *ihostname,int seq, int bytes )
{
  
  rawSocket = socket(AF_INET,SOCK_RAW,IPPROTO_ICMP);
  if ( rawSocket < 0 )
    {
      rc = S_CLIENT_RAWSOCKETFAILED;
      return rc;
    }
  
  //int on = 1;
  //setsockopt(rawSocket,SOL_SOCKET,SO_DEBUG,(char*)&on,sizeof(on));
  //setsockopt(rawSocket,SOL_SOCKET,SO_DONTROUTE,(char*)&on,sizeof(on));
  SetTimeOut(5000000);
  
  icmpRemoteAddr.sin_family = AF_INET;
  icmpRemoteAddr.sin_port = 0;
  
  /*
    _dnsCache.mutex.Request();
    if ( _dnsCache.alias && ihostname && !strcmp(_dnsCache.alias,ihostname) )
    icmpRemoteAddr.sin_addr = _dnsCache.sin_addr;
    else
    icmpRemoteAddr.sin_addr.s_addr = inet_addr( (char *)ihostname );
    _dnsCache.mutex.Release();
  */
  
  icmpRemoteAddr.sin_addr.s_addr = inet_addr( (char *)ihostname );
  
  if ( icmpRemoteAddr.sin_addr.s_addr == INADDR_NONE )
    {
      rc = DNSLookup(ihostname,&icmpRemoteAddr);
      if ( rc < 0 )
	{
	  _CloseSocket(rawSocket);
	  rawSocket = 0;
	  return rc;
	}
    }
  
  SetNumIPAddress(&icmpRemoteAddr);
  
  int bytesToSend = bytes;
  
  icp = (struct icmp *)sendpack;
  icp->icmp_type = ICMP_ECHO;
  icp->icmp_code = 0;
  icp->icmp_cksum = 0;
  icp->icmp_id = (ushort)_OsGetThreadID();
  icp->icmp_seq = (ushort)seq;
  icp->icmp_data[0] = 3;
  
  icp->icmp_cksum = in_cksum((ushort *)sendpack, bytesToSend);
  
  sw.Start();
  rc = sendto(rawSocket,(char *)icp,bytesToSend,0,
	      (struct sockaddr *)&icmpRemoteAddr,
	      sizeof(icmpRemoteAddr));
  if ( rc < 0 )
    {
      printf("errno: %d\n",errno);
      return 0;
    }
  
  /* *** Set socket to non-blocking *** */
  _TcpipSetSocketBlockingMode(rawSocket,FALSE);
  
  socklen_t len = sizeof(icmpRemoteAddr);
  char recvpack[S_CLIENT_MAXPACKET];
  do
    {
      
      do
	{
	  rc = recvfrom(rawSocket,(char *)recvpack,sizeof(recvpack),0,
			(struct sockaddr *)&icmpRemoteAddr,&len );
	  if ( rc < 0 )
	    _OsSleep(1000);
	} while ( rc < 0 && sw.Stop() < usecTimeOut );
      
      int offset = rc - bytesToSend;
      
      localAddr.sin_addr.s_addr = *((slong *)&recvpack[16]);
      
      for ( int i=offset; i<(int)sizeof(*icp); i++ )
	((char *)icp)[i-offset] = recvpack[i];
      icp->icmp_data[0] = recvpack[8];
      icp->icmp_data[1] = (uchar) bytesToSend;
      icp->icmp_data[2] = (uchar) rc;
      
    } while ( icp->icmp_id != _OsGetThreadID() || icp->icmp_seq != seq );
  
  _CloseSocket(rawSocket);
  rawSocket = 0;
  
  rc = S_CLIENT_OK;
  
  return sw.Stop(); 
}


char *
S_Client::ErrorString()
{
  
  switch ( rc )
    {
    case S_CLIENT_OK :
      error.Create("No error");
      break;
    case S_CLIENT_TCPSOCKETFAILED :
      error.Create("Failed to create TCP socket");
      break;
    case S_CLIENT_UDPSOCKETFAILED :
      error.Create("Failed to create UDP socket");
      break;
    case S_CLIENT_BADIPADDRESS :
      error.Create("Bad IP address");
      break;
    case S_CLIENT_TCPCONNECTFAILED :
      error.Create("Failed on TCP connect");
      break;
    case S_CLIENT_TCPDNSFAILED :
      error.Create("Failed on DNS lookup");
      break;
    case S_CLIENT_UDPBINDFAILED :
      error.Create("Failed to bind UDP socket");
      break;
    case S_CLIENT_TCPCONNECTMSGFAILED :
      error.Create("Failed to receive TCP connect msg");
      break;
    case S_CLIENT_UDPCONNECTMSGFAILED :
      error.Create("Failed to receive UDP connect msg");
      break;
    case S_CLIENT_NOHOSTNAME :
      error.Create("No hostname given");
      break;
    case S_CLIENT_CONNECTMSGFAILED :
      error.Create("Failed to receive connect msg");
      break;
    case S_CLIENT_REMOTEADDRFAILED :
      error.Create("Failed to receive remote address");
      break;
    case S_CLIENT_BADPACKET :
      error.Create("Read a bad packet");
      break;
    case S_CLIENT_RAWSOCKETFAILED :
      error.Create("Failed to create a RAW socket");
      break;
    case S_CLIENT_RAWBINDFAILED :
      error.Create("Failed to bind RAW socket");
    default :
      error.Create("Unknown error");
    }
  return error.str;
}
