#include "S_OS.h"
#include "S_SecureFileServer.h"

#include "S_Packet.h"
#include "S_Client.h"


int
S_SecureFileServer::Init()
{
  
  // Init member variables
  
  return 0;
}


S_SecureFileServer::S_SecureFileServer()
{
  Init(); // Call at top of all constructors
}


S_SecureFileServer::S_SecureFileServer(int iport,int ioptions,
                                       char *ilogFilename,char *iregKey,
                                       int iroptions,char *irexclude) 
{
  Init(); // Call at top of all constructors
  Create(iport,ioptions,ilogFilename,iregKey,iroptions,irexclude);
}


int
S_SecureFileServer::Create(int iport,int ioptions,
                           char *ilogFilename,char *iregKey,
                           int iroptions,char *irexclude)
{
  Destroy(); // Call at top of all Create() methods
  
  // Call subclass and member Create() methods
  S_FileServer::Create(iport,ioptions,ilogFilename);
  S_RegCode::Create(iregKey,iroptions,irexclude);
  
  // Allocate member variable data
  
  return 0;
}


int
S_SecureFileServer::Destroy()
{
  
  // Free member variable data
  
  Init(); // Call at bottom of all Destroy() methods
  return 0;
}


S_SecureFileServer::~S_SecureFileServer()
{
  Destroy();  // All destructors consist only of call to Destroy()
}


int
S_SecureFileServer::HandleRequestFileListMsg(S_Client *c)
{
  
  int duration = HandleRequestRegCodeDurationMsg(c);
  if ( duration >= 0 )
    rc = S_FileServer::HandleRequestFileListMsg(c);
  
  return rc;
}


int
S_SecureFileServer::HandleRequestFileMsg(S_Client *c)
{
  
  int duration = HandleRequestRegCodeDurationMsg(c);
  if ( duration >= 0 )
    rc = S_FileServer::HandleRequestFileMsg(c);
  
  return rc;
}


int
S_SecureFileServer::HandleUploadFileMsg(S_Client *c)
{
  
  int duration = HandleRequestRegCodeDurationMsg(c);
  if ( duration >= 0 )
    rc = S_FileServer::HandleUploadFileMsg(c);
  
  return rc;
}


int
S_SecureFileServer::HandleRequestFileCheckSumMsg(S_Client *c)
{
  
  int duration = HandleRequestRegCodeDurationMsg(c);
  if ( duration >= 0 )
    rc = S_FileServer::HandleRequestFileCheckSumMsg(c);
  
  return rc;
}


int
S_SecureFileServer::HandleRequestRegCodeDurationMsg(S_Client *c,int *id,
                                                    int *iserial,
                                                    char *iregcode)
{
  
  int duration = -1;
  S_Packet p;
  c->ReadPacket(&p);
  if ( p.readComplete )
    {
      int serial = p.PopLong();
      char *regcode = p.PopString();
      if ( regcode )
	{
	  duration = GetDuration(serial,regcode);
	  S_free(regcode);
	}
    }
  
  p.Flush();
  p.PushLong(duration);
  c->WritePacket(&p);
  
  return duration;
}
