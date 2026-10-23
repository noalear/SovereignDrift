/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
#include "S_OS.h"
#include "S_SecureFileClient.h"
#include "S_SecureFileServer.h"

#include "S_Packet.h"

int
S_SecureFileClient::Init()
{
  
  // Init member variables
  
  return 0;
}


S_SecureFileClient::S_SecureFileClient()
{
  Init(); // Call at top of all constructors
  
  Create(); // Call Create() method matching params
  
}


int
S_SecureFileClient::Create()
{
  Destroy(); // Call at top of all Create() methods
  
  // Call subclass and member Create() methods
  S_FileClient::Create();
  
  // Allocate member variable data
  
  return 0;
}


int
S_SecureFileClient::Destroy()
{
  
  // Free member variable data
  
  Init(); // Call at bottom of all Destroy() methods
  return 0;
}


S_SecureFileClient::~S_SecureFileClient()
{
  Destroy();  // All destructors consist only of call to Destroy()
}


int
S_SecureFileClient::SecureRequestFileCheckSum(int serial,char *regcode,
                                              char *hostname,int port,
                                              char *filename,
                                              int begin,int end)
{
  
  SetTimeOut(60000000); /* *** Increase timeout to 60 sec. (def. is 5) *** */
  
  Connect(hostname,port,S_SECUREFILESERVER_REQUESTFILECHECKSUM);
  if ( rc == S_CLIENT_OK )
    {
      
      int regcodeOK = FALSE;
      
      S_Packet p;
      p.Flush();
      p.PushLong(serial);
      p.PushString(regcode);
      WritePacket(&p);
      if ( rc == S_CLIENT_OK )
	{
	  p.Flush();
	  ReadPacket(&p);
	  if ( p.readComplete && p.PopLong() >= 0 )
            regcodeOK = TRUE;
	}
      
      rc = S_SECUREFILECLIENT_REGCODEFAILED;
      if ( regcodeOK )
	{
	  S_Packet packet;
	  packet.PushString(filename);
	  packet.PushLong(begin);
	  packet.PushLong(end);
	  WritePacket(&packet);
	  slong checkSum;
	  ReadLong(&checkSum);
	  Disconnect();
	  return checkSum;
	}
      
    }
  return rc;
}


int
S_SecureFileClient::SecureRequestRegCodeDuration(int serial,char *regcode,
                                                 char *hostname,int port,
                                                 S_Packet *p)
{
  
  int duration = S_SECUREFILECLIENT_REGCODEFAILED;
  if ( !Connect(hostname,port,S_SECUREFILESERVER_REQUESTREGCODEDURATION) )
    {
      
      p->Flush();
      p->PushLong(serial);
      p->PushString(regcode);
      WritePacket(p);
      if ( rc == S_CLIENT_OK )
	{
	  p->Flush();
	  ReadPacket(p);
	  if ( p->readComplete )
	    duration = p->PopLong();
	  if ( duration < 0 )
            rc = S_SECUREFILECLIENT_REGCODEFAILED;
	}
      
      Disconnect();
    }
  
  return duration;
}


int
S_SecureFileClient::SecureRequestFileList(int serial,char *regcode,
                                          char *hostname,int port,
                                          char *mask,S_Packet *p)
{
  ulong numFiles = 0;
  if ( !Connect(hostname,port,S_SECUREFILESERVER_REQUESTFILELIST) )
    {
      
      int regcodeOK = FALSE;
      
      p->Flush();
      p->PushLong(serial);
      p->PushString(regcode);
      WritePacket(p);
      if ( rc == S_CLIENT_OK )
	{
	  p->Flush();
	  ReadPacket(p);
	  if ( p->readComplete && p->PopLong() >= 0 )
            regcodeOK = TRUE;
	}
      
      if ( regcodeOK )
	{
	  p->Flush();
	  p->PushString(mask);
	  WritePacket(p);
	  if ( rc == S_CLIENT_OK )
	    {
	      p->Flush();
	      ReadPacket(p);
	      if ( p->readComplete )
		numFiles = p->PeekULong();
            }
	}
      
      Disconnect();
    }
  
  return numFiles;
}


int
S_SecureFileClient::SecureRequestFile(int serial,char *regcode,
                                      char *hostname,int port,
                                      char *filename,S_Packet *opacket)
{
  
  SetTimeOut(60000000); /* *** Increase timeout to 60 sec. (def. is 5) *** */
  Connect(hostname,port,S_SECUREFILESERVER_REQUESTFILE);
  if ( rc == S_CLIENT_OK )
    {
      S_Packet packet;
      
      int regcodeOK = FALSE;
      
      packet.Flush();
      packet.PushLong(serial);
      packet.PushString(regcode);
      WritePacket(&packet);
      
      if ( rc == S_CLIENT_OK )
	{
	  packet.Flush();
	  ReadPacket(&packet);
	  if ( packet.readComplete && packet.PopLong() >= 0 )
            regcodeOK = TRUE;
	}
      
      if ( !regcodeOK )
	return S_SECUREFILECLIENT_REGCODEFAILED;
      
      packet.Flush();
      packet.PushString(filename);
      WritePacket(&packet);
      
      ReadLong(&rc);
      
      if ( rc == S_FILESERVER_OK )
	{
	  opacket->Flush();
	  
	  ReadPacket(opacket);
	  if ( !opacket->readComplete )
            rc = S_FILECLIENT_PACKETREADFAILED;
	}
      
      Disconnect();
      return rc;
    }
  
  return rc;
}

