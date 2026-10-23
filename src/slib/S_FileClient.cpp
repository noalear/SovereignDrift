/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
#include "S_OS.h"
#include "S_FileServer.h"
#include "S_FileClient.h"
#include "S_Packet.h"
#include "S_TokenArray.h"


S_FileClient::S_FileClient()
{
}


S_FileClient::~S_FileClient()
{
}


slong 
S_FileClient::RequestFileSize(char *hostname,int port,char *filename)
{
  SetTimeOut(60000000); /* *** Increase timeout to 60 sec. (def. is 5) *** */
  Connect(hostname,port,S_FILESERVER_REQUESTFILESIZE);
  if ( rc == S_CLIENT_OK )
    {
      S_Packet packet;
      packet.PushString(filename);
      WritePacket(&packet);
      slong size;
      ReadLong(&size);
      Disconnect();
      return size;
    }
  return rc;
}


int
S_FileClient::RequestFileList(char *hostname,int port,char *mask,S_Packet *p)
{
  ulong numFiles = 0;
  Connect(hostname,port,S_FILESERVER_REQUESTFILELIST);
  if ( rc == S_CLIENT_OK )
    {
      p->Flush();
      p->PushString(mask);
      WritePacket(p);
      if ( rc == S_CLIENT_OK )
	{
	  p->Flush();
	  ReadPacket(p);
	  if ( p->readComplete )
	    {
	      numFiles = p->PeekULong();
	    }
	}
      
      Disconnect();
    }
  return numFiles;
}



ulong 
S_FileClient::RequestFileModDate(char *hostname,int port,char *filename)
{
  SetTimeOut(60000000); /* *** Increase timeout to 60 sec. (def. is 5) *** */
  Connect(hostname,port,S_FILESERVER_REQUESTFILEMODDATE);
  if ( rc == S_CLIENT_OK )
    {
      S_Packet packet;
      packet.PushString(filename);
      WritePacket(&packet);
      ulong date;
      ReadULong(&date);
      Disconnect();
      return date;
    }
  return rc;
}


int
S_FileClient::RequestFileStatus(char *hostname,int port,char *filename)
{
  SetTimeOut(60000000); /* *** Increase timeout to 60 sec. (def. is 5) *** */
  Connect(hostname,port,S_FILESERVER_REQUESTFILESTATUS);
  if ( rc == S_CLIENT_OK )
    {
      S_Packet packet;
      packet.PushString(filename);
      WritePacket(&packet);
      slong status;
      ReadLong(&status);
      Disconnect();
      return status;
    }
  return 0;
}


int
S_FileClient::RequestFileCheckSum(char *hostname,
				  int port,
				  char *filename,
                                  int begin,int end)
{
  SetTimeOut(60000000); /* *** Increase timeout to 60 sec. (def. is 5) *** */
  Connect(hostname,port,S_FILESERVER_REQUESTFILECHECKSUM);
  if ( rc == S_CLIENT_OK )
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
  return 0;
}


int
S_FileClient::RequestFile(char *hostname,
			  int port,
                          char *filename,
			  S_Packet *opacket) {
  
  SetTimeOut(60000000); /* *** Increase timeout to 60 sec. (def. is 5) *** */
  Connect(hostname,port,S_FILESERVER_REQUESTFILE);
  slong src = S_FILESERVER_OK;
  if ( rc == S_CLIENT_OK )
    {
      S_Packet packet;
      packet.PushString(filename);
      WritePacket(&packet);
      
      ReadLong(&src);
      if ( src == S_FILESERVER_OK )
	{
	  opacket->Flush();
	  ReadPacket(opacket);
	  if ( !opacket->readComplete )
            rc = S_FILECLIENT_PACKETREADFAILED;
	}
      else
	{
          rc = S_FILECLIENT_FILEREADFAILED;
	}
      
      Disconnect();
      return rc;
    }
  
  if ( src != S_FILESERVER_OK )
    {
      rc = S_FILECLIENT_PACKETREADFAILED;
    }
  
  return rc;
}


int
S_FileClient::UploadFile(char *hostname,
			 int port,
                         char *filename,
			 S_Packet *packet) {
  
  SetTimeOut(60000000); /* Set timeout to 1 minute */
  Connect(hostname,port,S_FILESERVER_UPLOADFILE);
  if ( rc != S_CLIENT_OK )
    {
      return rc;
    }
  
  if ( !filename )
    {
      rc = S_FILECLIENT_FILEDOESNOTEXIST;
      return rc;
    }
  
  ulong size = _OsFileSize(filename);
  if ( size == 0 )
    {
      rc = S_FILECLIENT_FILEDOESNOTEXIST;
      return rc;
    }
  
  ulong chunkSize = 16384;
  if ( size < chunkSize )
      chunkSize = size;
  
  char *buffer = (char *)S_malloc(chunkSize);
  if ( !buffer )
    {
      rc = S_FILECLIENT_MALLOCFAILED;
      return rc;
    }
  
  FILE *fptr = fopen(filename,"rb");
  if ( !fptr )
    {
      S_free(buffer);
      rc = S_FILECLIENT_FILEOPENFAILED;
      return rc;
    }
  
  packet->Flush();
  packet->PushString(filename);
  packet->PushULong(size);
  
  /* ***************************************************************** */
  /* *** A Chunked file send.  Read the file into memory in little *** */
  /* *** chunks and send one chunk at a time.  This can not cur-   *** */
  /* *** rently be done with compressed files, however, since the  *** */
  /* *** zlib compression library allows one to work on a stream,  *** */
  /* *** compressed files could eventually be added.  For now if   *** */
  /* *** are downloading very large files, they should already be  *** */
  /* *** compressed on the disk to take advantage of the chunked   *** */
  /* *** file send method.                                         *** */
  /* ***************************************************************** */
  if ( chunkSize < size )
    {
      
      printf("Sending %s(%ldkb) to %s (%ldkb ch)\n",
             filename,
	     size/1024,
             hostname,
             chunkSize/1024);
      
      int numChunks = 0;
      
      ulong totalBytesRead = 0;
      while ( totalBytesRead < size )
	{
	  
	  ulong bytesRead = fread(buffer,1,chunkSize,fptr);
	  if ( bytesRead == 0 )
	    {
	      fclose(fptr);
	      S_free(buffer);
	      rc = S_FILECLIENT_FILEREADFAILED;
	      printf("UploadFile failed for %s.  Read %ld/%ld bytes of the "
		     "file %s.\n",hostname,totalBytesRead,size,filename);
	      return rc;
            }
	  
	  /* ************************************************************* */
	  /* *** The first write needs to look like a complete packet. *** */
	  /* *** We send the remaining parts of the packet manually.   *** */
	  /* ************************************************************* */
	  if ( totalBytesRead == 0 )
	    {
	      packet->PushBuffer(chunkSize,buffer,size);
	      
	      ulong vSize = packet->packetSize;
	      packet->packetSize += size - chunkSize;
	      
	      if ( WriteVirtualPacket(packet,vSize) != S_CLIENT_OK )
		{
		  fclose(fptr);
		  S_free(buffer);
		  rc = S_FILECLIENT_PACKETWRITEFAILED;
		  printf("UploadFile failed for %s.  Failed to write initial "
			 "data block for %s.\n",hostname,filename);
		  return rc;
		}
	      
	      numChunks++;
	    }
	  /* ************************************************************* */
	  
	  /* ************************************************************ */
	  /* *** Send all other data manually after first one is sent *** */
	  /* ************************************************************ */
	  int numTries = 0;
	  int maxTries = 100;
	  if ( totalBytesRead > 0 )
	    {
	      ulong bytesSent = 0;
	      do {
		bytesSent = Send((char *)buffer,bytesRead);
		if ( bytesSent != bytesRead ) {
                  _OsSleep(1000000);
		}
		numTries++;
	      } while ( bytesSent != bytesRead && numTries < maxTries );
	      
	      numChunks++;
	    }
	  /* ************************************************************ */
	  
	  if ( numTries >= maxTries )
	    {
	      fclose(fptr);
	      S_free(buffer);
	      rc = S_FILECLIENT_PACKETWRITEFAILED;
	      printf("UploadFile failed for %s.  Failed to write chunk data "
		     "for %s.\n",hostname,filename);
	      return rc;
	    }
	  
	  totalBytesRead += bytesRead;
	  
	  _OsSleep(10000);
	}
      
      printf("Sent %s (%ldkb) to %s in %d chunks of %ldkb\n",
             filename,size/1024,hostname,numChunks,chunkSize/1024);
    }
  /* ***************************************************************** */
  
  /* ***************************************************************** */
  /* *** A normal file send.  Read the entire file into memory and *** */
  /* *** send it as one big packet.  This is what we do with files *** */
  /* *** that are either smaller than chunkSize or compressed      *** */
  /* ***************************************************************** */
  if ( chunkSize == size )
    {
      
      printf("Sending %s(%ld bytes) to %s\n",filename,size,hostname);
      
      ulong bytesRead = fread(buffer,1,size,fptr);
      if ( bytesRead != size )
	{
	  fclose(fptr);
	  S_free(buffer);
	  rc = S_FILECLIENT_FILEREADFAILED;
	  printf("UploadFile failed for %s.  Read %ld/%ld bytes of the \n"
		 "file %s.",hostname,bytesRead,size,filename);
	  return rc;
	}
      
      packet->PushBuffer(size,buffer);
      
      if ( size > 1024 )
	packet->Compress();
      
      if ( WritePacket(packet) != S_CLIENT_OK )
	{
	  fclose(fptr);
	  S_free(buffer);
	  rc = S_FILECLIENT_PACKETWRITEFAILED;
	  printf("UploadFile failed for %s.  Failed to write data for %s.\n",
		 hostname,filename);
	  return rc;
	}
      
      rc = S_FILECLIENT_OK;
      
    }
  /* ***************************************************************** */
  
  ReadLong(&rc);
  Disconnect();
  
  fclose(fptr);
  S_free(buffer);
  rc = S_FILECLIENT_OK;
  
  return rc;
}


int
S_FileClient::WriteFile(char *filename,S_Packet *packet)
{
  ulong bytesWritten = 0;
  rc = S_FILECLIENT_PACKETREADFAILED;
  if ( packet->readComplete )
    {
      rc = S_FILECLIENT_FILEISEMPTY;
      ulong size = packet->PopULong();
      if ( size > 0 )
	{
	  rc = S_FILECLIENT_MALLOCFAILED;
	  char *buffer = packet->PopBuffer();
	  if ( buffer )
	    {
	      rc = S_FILECLIENT_FILEOPENFAILED;
	      _OsFilename(filename);
	      FILE *fptr = fopen(filename,"wb");
	      if ( fptr )
		{
		  rc = S_FILECLIENT_FILEWRITEFAILED;
		  bytesWritten = fwrite(buffer,1,size,fptr);
		  if ( bytesWritten == size )
		    rc = S_FILECLIENT_OK;
		  fclose(fptr);
		}
	      S_free(buffer);
            }
	}
    }
  return rc;
}


#define S_FILECLIENT_HTTPBUFFERSIZE 0xFFFF
#define S_FILECLIENT_HTTPCHUNKSIZE  0xFF

int
S_FileClient::HTTPGet(char *server,int port,char *filename,S_Packet *packet)
{
  
  char readBuf[S_FILECLIENT_HTTPBUFFERSIZE+1];
  char *contentBuf = NULL;
  
  int totalBytesRead = 0;
  int headerSize = 0;
  int contentSize = 0;
  char *contentLocation = NULL;
  
  Connect(server,port);
  if ( rc == S_CLIENT_OK || rc == S_CLIENT_UDPCONNECTMSGFAILED )
    {
      
      char url[1024];
      
      sprintf(url,"GET %s HTTP/1.0 \r\n\r\n",filename);;
      Send(url,strlen(url));
      
      int readHeader = FALSE;
      do
	{
	  int rc = Recv(readBuf+totalBytesRead,S_FILECLIENT_HTTPCHUNKSIZE,
			S_CLIENT_TCP|S_CLIENT_NONBLOCK);
	  
	  if ( rc > 0 )
	    {
	      totalBytesRead += rc;
	      readBuf[totalBytesRead] = 0;
	      char *beginContentLength = strstr(readBuf,"Content-Length");
	      if ( beginContentLength )
		{
		  //char *endContentLength = strstr(beginContentLength,"\r\n");
		  char *beginContent = strstr(beginContentLength,"\r\n\r\n");
		  if ( beginContent )
		    readHeader = TRUE;
		}
	    }
	} while ( !readHeader );
      
      char *beginContentLength = strstr(readBuf,"Content-Length");
      char *beginContent = strstr(beginContentLength,"\r\n\r\n") + 2;
      headerSize = beginContent - readBuf;
      
      int i;
      for ( i=0; i<headerSize; i++ )
	if ( readBuf[i] == '\r' || readBuf[i] == '\n' )
	  readBuf[i] = ' ';
      readBuf[headerSize-1] = 0;
      
      S_TokenArray tokens(readBuf);
      for ( i=0; i<tokens.numLines; i++ ) {
        if ( strcmp(tokens.lines[i],"Content-Length:") == 0 )
	  contentSize = atol(tokens.lines[i+1]) + 2;
        if ( strcmp(tokens.lines[i],"Content-Location:") == 0 )
	  contentLocation = tokens.lines[i+1];
      }
      
      contentBuf = (char *)S_malloc(contentSize+1);
      if ( contentBuf )
	{
	  
	  for ( int i=headerSize; i<totalBytesRead; i++ )
            contentBuf[i-headerSize] = readBuf[i];
	  
	  int contentBytesRead = totalBytesRead - headerSize;
	  int contentBytesToRead = contentSize - contentBytesRead;
	  
	  if ( contentBytesToRead < 0 )
            contentBytesToRead = 0;
	  
	  int rc = Recv(contentBuf+contentBytesRead,contentBytesToRead);
	  if ( rc > 0 )
	    {
	      totalBytesRead += rc;
	      readBuf[totalBytesRead] = 0;
	    }
	  
	  packet->PushBuffer(contentSize-2,contentBuf+2);
	  packet->Rewind();
	  
	  S_free(contentBuf);
	}
      
      Disconnect();
    }
  
  return rc;
}


char *
S_FileClient::ErrorString()
{
  S_Client::ErrorString();
  switch ( rc )
    {
    case S_FILECLIENT_OK :
      error.Create("No error");
      break;
    case S_FILECLIENT_PACKETPUSHFAILED :
      error.Create("Failed to push data to packet");
      break;
    case S_FILECLIENT_PACKETREADFAILED :
      error.Create("Failed to read packet");
      break;
    case S_FILECLIENT_PACKETWRITEFAILED :
      error.Create("Failed to write packet");
      break;
    case S_FILECLIENT_FILEDOESNOTEXIST :
      error.Create("File does not exist");
      break;
    case S_FILECLIENT_FILEISEMPTY :
      error.Create("File is empty");
      break;
    case S_FILECLIENT_FILEOPENFAILED :
      error.Create("File open failed");
      break;
    case S_FILECLIENT_FILEREADFAILED :
      error.Create("File read failed");
      break;
    case S_FILECLIENT_FILEWRITEFAILED :
      error.Create("File write failed");
      break;
    case S_FILECLIENT_MALLOCFAILED :
      error.Create("Failed to allocate memory");
      break;
    }
  
  return error.str;
}
