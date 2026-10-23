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
#include "S_Server.h"
#include "S_Client.h"
#include "S_Thread.h"
#include "S_Packet.h"
#include "S_TokenArray.h"

int
S_Server::Init()
{
  tcpSocket = 0;
  udpSocket = 0;
  hits = 0;
  port = 0;
  dynamicDenyIPList = FALSE;
  pConnectMsgHandler = NULL;
  arg = NULL;
  return 0;
}


S_Server::S_Server()
{
  Init();
}


S_Server::S_Server(int iport,int (*ipConnectMsgHandler)(S_Server *,S_Client *),
                   void *iarg,int ioptions,char *ilogFilename)
{
  Init();
  Create(iport,ipConnectMsgHandler,iarg,ioptions,ilogFilename);
}


int
S_Server::Create(int iport,int (*ipConnectMsgHandler)(S_Server *,S_Client *),
                 void *iarg,int ioptions,char *ilogFilename)
{
  
  Destroy();
  
  S_Logger::Create(ilogFilename,ioptions);
  
  LoadDenyIPList("server.deny");
  
  hits = 0;
  port = iport;
  pConnectMsgHandler = ipConnectMsgHandler;
  arg = iarg;
  
  rc = _OsTcpipInit();
  if ( rc != 0 )
    {
      rc = S_SERVER_TCPIPINITFAILED;
      return rc;
    }
  
  /* *** Create TCP socket *** */
  tcpSocket = socket(AF_INET,SOCK_STREAM,0);
  if ( tcpSocket < 0 )
    {
      rc = S_SERVER_TCPSOCKETFAILED;
      return rc;
    }
  
  /* *** Create UDP socket *** */
  udpSocket = socket(AF_INET,SOCK_DGRAM,0);
  if ( udpSocket < 0 )
    {
      rc = S_SERVER_UDPSOCKETFAILED;
      return rc;
    }
  
  struct sockaddr_in serverAddr;
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons((short)port);
  serverAddr.sin_addr.s_addr = INADDR_ANY;
  
  /* *** Bind the TCP socket to the port *** */ 
  int opt = TRUE;
  setsockopt(tcpSocket,SOL_SOCKET,SO_REUSEADDR,(char *)&opt,sizeof(int));
  rc = bind(tcpSocket,(struct sockaddr *)&serverAddr,sizeof(serverAddr));
  if ( rc < 0 )
    {
      rc = S_SERVER_TCPBINDFAILED;
      return rc;
    }
  
  /* *** Bind the UDP socket to the port *** */
  setsockopt(udpSocket,SOL_SOCKET,SO_REUSEADDR,(char *)&opt,sizeof(int));
  rc = bind(udpSocket,(struct sockaddr *)&serverAddr,sizeof(serverAddr));
  if ( rc < 0 )
    {
      rc = S_SERVER_UDPBINDFAILED;
      return rc;
    }
  
  /* *** Run listen only for TCP socket *** */
  rc = listen(tcpSocket,5);
  if ( rc < 0 )
    {
      rc = S_SERVER_TCPLISTENFAILED;
      return rc;
    }
  
  rc = S_SERVER_OK;
  return rc;
}


int
S_Server::Destroy()
{
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
  denyIPList.Destroy();
  Init();
  return 0;
}


S_Server::~S_Server(void)
{
  Destroy();
}


typedef struct
{
  S_Server *server;
  S_Thread *thread;
  struct sockaddr_in *clientAddr;
  int tcpClientSocket;
  int udpServerSocket;
} _ServerThreadArg;


int
_ServerThreadFunc(void *arg)
{
  _ServerThreadArg *sa = (_ServerThreadArg *)arg;
  
  if ( sa && sa->thread )
    {
      S_Client *client = new S_Client(sa->clientAddr,
                                      sa->tcpClientSocket,
                                      sa->udpServerSocket);
      
      if ( client )
	{
	  
	  if ( client->rc == S_CLIENT_OK )
	    {	      

	      if ( sa->server->dynamicDenyIPList )
		{
		  sa->server->denyIPListMutex.Request();
		  sa->server->LoadDenyIPList("server.deny");
		}

	      int denied = FALSE;
	      for ( int i=0; i<sa->server->denyIPList.numLines; i++ )
		{
		  if ( strstr(client->numIPAddress,
			      sa->server->denyIPList.lines[i]) ==
		       client->numIPAddress )
		    {
		      denied = TRUE;
		      sa->server->NewClientDenied(client);
		    }
		}

	      if ( sa->server->dynamicDenyIPList)
		{
		  sa->server->denyIPListMutex.Release();
		}

	      if ( !denied )
		{
		  sa->server->AddChild(client);
		  sa->server->ProcessConnectMsg(client);
		}
	      
            }
	  else
	    {
	      sa->server->NewClientFailed();
            }
	  
	  delete client;
	}
      
      sa->thread->selfDelete = TRUE;
      S_free(sa);
    }
  
  return 0;
}


int
S_Server::WaitForChildren(ulong timeOut)
{
  int count = 0;
  while ( child && count < (int)timeOut )
    {
      _OsSleep(100000);
      count += 100000;
    }
  return 0;
}


int
S_Server::Stop()
{
  continueRunning = FALSE;
  S_Client client;
  client.Connect("127.0.0.1",port,S_SERVER_STOP);
  client.Disconnect();
  return 0;
}


int
S_Server::WaitingForConnection()
{
  return 0;
}


int
S_Server::DoneWaitingForConnection()
{
  return 0;
}


int
S_Server::NewClientFailed()
{
  return 0;
}


int
S_Server::NewClientDenied(S_Client *client)
{
  return 0;
}


int
S_Server::AcceptFailed()
{
  return 0;
}


int
S_Server::ClientThreadFailed()
{
  return 0;
}


int
S_Server::NormalExit()
{
  return 0;
}


int
S_Server::Run()
{
  
  int tcpClientSocket;
  struct sockaddr_in clientAddr;
  addrlenint sizeAddr = sizeof(clientAddr);
  
  continueRunning = TRUE;
  while ( continueRunning )
    {
      
      WaitingForConnection();
      
      tcpClientSocket = 
	accept(tcpSocket,(struct sockaddr *)&clientAddr,&sizeAddr);
      if ( tcpClientSocket < 0 )
	{
	  AcceptFailed();
	  rc = S_SERVER_TCPACCEPTFAILED;
	}
      
      DoneWaitingForConnection();
      
      if ( continueRunning && tcpClientSocket >= 0 )
	{
	  _ServerThreadArg *serverThreadArg;
	  serverThreadArg=(_ServerThreadArg *)
	    S_malloc(sizeof(_ServerThreadArg));
	  serverThreadArg->server = this;
	  serverThreadArg->clientAddr = &clientAddr;
	  serverThreadArg->tcpClientSocket = tcpClientSocket;
	  serverThreadArg->udpServerSocket = udpSocket;
	  
	  S_Thread *thread = new S_Thread();
	  serverThreadArg->thread = thread;
	  
	  thread->Start(_ServerThreadFunc,(void *)serverThreadArg);
	  
	  if ( !thread || (thread && thread->rc != S_THREAD_OK) )
	    {
	      ClientThreadFailed();
	      if ( tcpClientSocket )
		_CloseSocket(tcpClientSocket);
	      rc = S_SERVER_CLIENTTHREADFAILED;
            }
	  else
            hits++;
	  
	}
      
    }
  
  NormalExit();
  
  rc = S_SERVER_OK;
  return rc;
}


int
S_Server::ProcessConnectMsg(S_Client *client)
{
  if ( pConnectMsgHandler )
    {
      (*pConnectMsgHandler)(this,client);
    }
  switch ( client->connectMsg )
    {
    case S_SERVER_REQUESTTIME :
      HandleRequestTimeMsg(client);
      break;
    }
  return 0;
}


int
S_Server::HandleRequestTimeMsg(S_Client *client)
{
  S_Packet p;
  p.PushULong(time(NULL));
  client->WritePacket(&p);
  return 0;
}


int
S_Server::LoadDenyIPList(char *filename)
{

  denyIPList.Destroy();
  denyIPReason.Destroy();

  if ( filename )
    {
      FILE *fptr = fopen(filename,"r");
      if ( fptr )
	{

	  char *rc;
	  char temp[1024];
	  do 
	    {
	      rc = fgets(temp,1023,fptr);
	      if ( rc && temp[0] != '#' )
		{
		  S_TokenArray tokens(temp);
		  if ( tokens.numLines > 0 )
		    {
		      tokens.RemoveCharacter(0,(char)10);
		      tokens.RemoveCharacter(0,(char)13);
		      tokens.RemoveCharacter(0,':');
		      tokens.RemoveCharacter(0,';');
		      if ( tokens.lines[0][0] != 0 )
			{
			  denyIPList.AppendLine(tokens.lines[0]);
			  if ( tokens.numLines > 1 && tokens.lines[1][0] != 0 )
			    {
			      denyIPReason.AppendLine(tokens.lines[1]);
			    }
			  else
			    {
			      denyIPReason.AppendLine("You are banned!");
			    }
			}
		    }
		}
	    } while ( rc != NULL );

	  fclose(fptr);
	}
    }
  return 0;
}

