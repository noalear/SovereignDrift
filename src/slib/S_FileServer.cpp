/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
#include "S_OS.h"
#include "S_Server.h"
#include "S_Client.h"
#include "S_Packet.h"
#include "S_DirList.h"
#include "S_FileServer.h"


int
S_FileServer::Init()
{
  cache = NULL;
  numCachedFiles = 0;
  maxCachedFiles = 0;
  iniCachedFiles = 1024;
  incCachedFiles = 1024;
  numFilesDownloaded = 0;
  numBytesDownloaded = 0;
  numFilesUploaded = 0;
  numBytesUploaded = 0;
  minCompSize = 1024;
  return 0;
}


S_FileServer::S_FileServer()
{
  Init();
}


S_FileServer::S_FileServer(int iport,int ioptions,char *ilogFilename)
{
  Init();
  Create(iport,ioptions,ilogFilename);
}


int
S_FileServer::Create(int iport,int ioptions,char *ilogFilename)
{
  Destroy();
  S_Server::Create(iport,NULL,NULL,ioptions,ilogFilename);
  
  inclusion.Create();
  exclusion.Create();
  
  exclusion.AppendLine(".zap");
  return 0;
}


int
S_FileServer::Destroy()
{
  if ( cache )
    {
      S_free(cache);
    }
  
  Init();
  return 0;
}


S_FileServer::~S_FileServer()
{
  Destroy();
}


int
S_FileServer::HandleRequestFileListMsg(S_Client *client)
{
  S_Packet p;
  client->ReadPacket(&p);
  if ( p.readComplete )
    {
      char *mask = p.PopString();
      if ( mask )
	{
	  _OsFilename(mask);
	  p.Flush();
	  S_DirList dirList(mask);
	  p.PushULong(dirList.numFiles);
	  for ( int i=0; i<dirList.numFiles; i++ )
	    {
	      
	      char *filename = dirList.file[i];
	      
	      if ( strcmp(filename,S_Logger::filename.str) &&
		   strcmp(filename,".") &&
		   strcmp(filename,"..") )
		{
		  
		  ulong size = 0;
		  ulong exists = FALSE;
		  ulong checkSum = 0;
		  ulong date = _OsGetFileDate(dirList.file[i]);
		  
		  int i;
		  int cacheHit = -1;
		  
		  cacheMutex.Request();
		  
		  /* ******************************************* */
		  /* *** See if file is in the cache already *** */
		  /* ******************************************* */
		  for ( i=0; i<numCachedFiles; i++ ) {
		    if ( !strcmp(filename,cache[i].filename) )
		      cacheHit = i;
	          }
		  
		  /* ******************************************** */
		  /* *** If file is in cache and the date has *** */
		  /* *** changed recompute the checkSum.      *** */
		  /* ******************************************** */
		  if ( cacheHit >= 0 )
		    {
		      if ( date != cache[cacheHit].date )
			{
			  //Log("Calculating new checksum for %s",filename);
			  checkSum = _OsFileCheckSum(filename,0,0,&size);
			  exists = size > 0;
			}
		      else
			{
			  //Log("Using cached checksum and size for %s",filename);
			  checkSum = cache[cacheHit].checkSum;
			  size = cache[cacheHit].size;
			  exists = size > 0;
			}
		    }
		  
		  /* ******************************************* */
		  /* *** If file is not in cache then add it *** */
		  /* ******************************************* */
		  if ( cacheHit < 0 )
		    {
		      //Log("Calculating initial checksum/size for %s",filename);
		      checkSum = _OsFileCheckSum(filename,0,0,&size);
		      exists = size > 0;
		      
		      if ( !cache || maxCachedFiles == 0 )
			{
			  cache = (S_FileServer_FileListCache *)
			    S_malloc(iniCachedFiles*
				     sizeof(S_FileServer_FileListCache));
			  maxCachedFiles = iniCachedFiles;
			}
		      else if ( cache && numCachedFiles >= maxCachedFiles )
			{
			  cache = (S_FileServer_FileListCache *)
			    S_realloc(cache,
				      sizeof(S_FileServer_FileListCache)*
				      (maxCachedFiles+incCachedFiles));
			  maxCachedFiles += incCachedFiles;
			}
		      
		      if ( cache )
			{
			  strncpy(cache[numCachedFiles].filename,
				  filename,
				  S_FILESERVER_MAXFILENAMESIZE);
			  cache[numCachedFiles].
			    filename[S_FILESERVER_MAXFILENAMESIZE] = 0;
			  cache[numCachedFiles].exists = exists;
			  cache[numCachedFiles].size = size;
			  cache[numCachedFiles].checkSum = checkSum;
			  cache[numCachedFiles].date = date;
			  numCachedFiles++;
			}
		      else
			{
			  numCachedFiles = 0;
			  maxCachedFiles = 0;
			}
		    }
		  
		  cacheMutex.Release();
		  
		  p.PushString(filename);
		  p.PushULong(exists);
		  p.PushULong(size);
		  p.PushULong(checkSum);
		  p.PushULong(date);
		}
	      
	    }
	  client->WritePacket(&p);
	  numBytesDownloaded += p.uncompPacketSize;
	  S_free(mask);
	}
    }
  return 0;
}


int
S_FileServer::HandleRequestFileStatusMsg(S_Client * client)
{
  Log("RequestFileStatus from %s",client->numIPAddress);
  
  S_Packet packet;
  client->SetTimeOut(60000000); /* Set timeout to 1 minute */
  client->ReadPacket(&packet);
  char *filename = packet.PopString();
  
  rc = S_FILESERVER_PACKETREADFAILED;
  if ( filename )
    {
      _OsFilename(filename);
      rc = S_FILESERVER_PACKETWRITEFAILED;
      if ( client->WriteULong(_OsFileExists(filename)) == S_CLIENT_OK )
	{
	  numBytesDownloaded += sizeof(ulong);
	  rc = S_FILESERVER_OK;
	}
      S_free(filename);
    }
  
  switch ( rc )
    {
    case S_FILESERVER_OK :
      break;
    case S_FILESERVER_PACKETREADFAILED :
      Log("RequestFileStatus failed for %s.  Failed to read incoming "
	  "packet.",client->numIPAddress);
      break;
    }
  
  return 0;
}


int
S_FileServer::HandleRequestFileCheckSumMsg(S_Client * client)
{
  Log("RequestFileCheckSum from %s",client->numIPAddress);
  
  S_Packet packet;
  client->SetTimeOut(60000000); /* Set timeout to 1 minute */
  client->ReadPacket(&packet);
  char *filename = packet.PopString();
  slong begin = packet.PopLong();
  slong end = packet.PopLong();
  
  rc = S_FILESERVER_PACKETREADFAILED;
  if ( filename )
    {
      _OsFilename(filename);
      rc = S_FILESERVER_PACKETWRITEFAILED;
      if ( !client->WriteULong(_OsFileCheckSum(filename,begin,end)) )
	{
	  numBytesDownloaded += sizeof(ulong);
	  rc = S_FILESERVER_OK;
	}
      S_free(filename);
    }
  
  switch ( rc )
    {
    case S_FILESERVER_OK :
      break;
    case S_FILESERVER_PACKETREADFAILED :
      Log("RequestFileCheckSum failed for %s.  Failed to read incoming "
	  "packet.",client->numIPAddress);
      break;
    }
  
  return 0;
}


int
S_FileServer::HandleRequestFileSizeMsg(S_Client *client)
{
  Log("RequestFileSize from %s",client->numIPAddress);
  
  S_Packet packet;
  client->SetTimeOut(60000000); /* Set timeout to 1 minute */
  client->ReadPacket(&packet);
  char *filename = packet.PopString();
  
  rc = S_FILESERVER_PACKETREADFAILED;
  if ( filename )
    {
      rc = S_FILESERVER_PACKETWRITEFAILED;
      _OsFilename(filename);
      ulong size = _OsFileSize(filename);
      if ( client->WriteULong(size) == S_CLIENT_OK )
	{
	  numBytesDownloaded += sizeof(ulong);
	  rc = S_FILESERVER_OK;
	}
      S_free(filename);
    }
  
  switch ( rc )
    {
    case S_FILESERVER_OK :
      break;
    case S_FILESERVER_PACKETREADFAILED :
      Log("RequestFileSize failed for %s.  Failed to read incoming packet.",
	  client->numIPAddress);
      break;
    }
  
  return 0;
}


int
S_FileServer::HandleRequestFileModDateMsg(S_Client *client)
{
  Log("RequestFileModDate from %s",client->numIPAddress);
  
  S_Packet packet;
  client->SetTimeOut(60000000); /* Set timeout to 1 minute */
  client->ReadPacket(&packet);
  char *filename = packet.PopString();
  
  rc = S_FILESERVER_PACKETREADFAILED;
  if ( filename )
    {
      rc = S_FILESERVER_PACKETWRITEFAILED;
      _OsFilename(filename);
      ulong date = _OsGetFileDate(filename,S_OS_FILEMODIFIEDDATE);
      if ( client->WriteULong(date) == S_CLIENT_OK )
	{
	  numBytesDownloaded += sizeof(ulong);
	  rc = S_FILESERVER_OK;
	}
      S_free(filename);
    }
  
  switch ( rc )
    {
    case S_FILESERVER_OK :
      break;
    case S_FILESERVER_PACKETREADFAILED :
      Log("RequestFileModeDate failed for %s.  Failed to read incoming "
	  "packet.",
	  client->numIPAddress);
      break;
    }
  
  return 0;
}


int
S_FileServer::HandleUploadFileMsg(S_Client *client)
{
  S_Packet packet;
  client->SetTimeOut(60000000); /* Set timeout to 1 minute */
  client->ReadPacket(&packet);
  rc = S_FILESERVER_PACKETREADFAILED;
  char *filename = NULL;
  if ( packet.readComplete )
    {
      filename = packet.PopString();
      if ( filename )
	{
	  _OsFilename(filename);
	  ulong bytesWritten = 0;
	  ulong size = packet.PopULong();
	  rc = S_FILESERVER_FILEISEMPTY;
	  Log("Receiving %s (%d bytes)",filename,size);
	  if ( size > 0 )
	    {
	      rc = S_FILESERVER_MALLOCFAILED;
	      char *buffer = packet.PopBuffer();
	      if ( buffer )
		{
		  rc = S_FILESERVER_FILEOPENFAILED;
		  FILE *fptr = fopen(filename,"wb");
		  if ( fptr )
		    {
		      rc = S_FILESERVER_FILEWRITEFAILED;
		      bytesWritten = fwrite(buffer,1,size,fptr);
		      if ( bytesWritten == size )
			{
			  numBytesUploaded += size;
			  numFilesUploaded++;
			  Log("Rcvd %s (%d bytes)",filename,size);
			  rc = S_FILESERVER_OK;
			}
		      fclose(fptr);
		    }
		  S_free(buffer);
		}
            }
	  S_free(filename);
	}
    }
  client->WriteLong(rc);
  
  switch ( rc )
    {
    case S_FILESERVER_PACKETREADFAILED :
      Log("Upload for %s failed on packet read.",filename);
      break;
    case S_FILESERVER_FILEISEMPTY :
      Log("Upload for %s failed.  File is empty.",filename);
      break;
    case S_FILESERVER_MALLOCFAILED :
      Log("Upload for %s failed on memory allocation.",filename);
      break;
    case S_FILESERVER_FILEOPENFAILED :
      Log("Upload for %s failed on file open.",filename);
      break;
    case S_FILESERVER_FILEWRITEFAILED :
      Log("Upload for %s failed on file write.",filename);
      break;
    }
  
  return rc;
}


int
S_FileServer::HandleRequestFileMsg(S_Client * client)
{
  char *ip = client->numIPAddress;
  
  Log("S_FileServer::RequestFile() from %s",ip);
  
  client->SetTimeOut(60000000); /* Set timeout to 1 minute */
  
  S_Packet packet;
  client->ReadPacket(&packet);
  if ( !packet.readComplete )
    {
      rc = S_FILESERVER_PACKETREADFAILED;
      client->WriteULong(rc);
      Log("S_FileServer::RequestFile() failed for %s.  Failed to read "
	  "incoming packet.",ip);
      return rc;
    }

  Log("S_FileServer::RequestFile() Packet Read Complete");

  char *filename = packet.PopString();
  if ( !filename )
    {
      rc = S_FILESERVER_PACKETREADFAILED;
      client->WriteULong(rc);
      Log("S_FileServer::RequestFile() failed for %s.  Rcvd NULL filename.",
	  ip);
      return rc;
    }

  Log("S_FileServer::RequestFile() Filename is %s",filename);
 
  _OsFilename(filename);
  ulong size = _OsFileSize(filename);
  if ( size == 0 )
    {
      rc = S_FILESERVER_FILEDOESNOTEXIST;
      client->WriteULong(rc);
      Log("S_FileServer::RequestFile() failed for %s.  File %s does not "
	  "exist.",ip,filename);
      S_free(filename);
      return rc;
    }

  Log("S_FileServer::RequestFile() %s is %d bytes",filename,size);
  
  int i;
  int compress = size > minCompSize;
  for ( i=0; i<exclusion.numLines; i++ )
    {
      if ( strstr(filename,exclusion.lines[i]) )
	{
	  compress = FALSE;
	  Log("S_FileServer::RequestFile() %s is not compressed",filename);
	}
    }
  for ( i=0; i<inclusion.numLines; i++ )
    {
      if ( strstr(filename,inclusion.lines[i]) )
	{
	  compress = TRUE;
	  Log("S_FileServer::RequestFile() %s is compressed",filename);
	}
    }

  ulong chunkSize = 16384;
  if ( size < chunkSize || compress )
    chunkSize = size;
  
  char *buffer = (char *)S_malloc(chunkSize);
  if ( !buffer )
    {
      rc = S_FILESERVER_MALLOCFAILED;
      client->WriteULong(rc);
      Log("S_FileServer::RequestFile() failed for %s.  Could not allocate "
	  "%d bytes for the "
          "file %s.",ip,chunkSize,filename);
      S_free(filename);
      return rc;
    }

  Log("S_FileServer::RequestFile() buffer for %s allocated",filename);
  
  FILE *fptr = fopen(filename,"rb");
  if ( !fptr )
    {
      rc = S_FILESERVER_FILEOPENFAILED;
      client->WriteULong(rc);
      Log("S_FileServer::RequestFile() failed for %s.  Could not open "
	  "%s for binary read.",
          ip,filename);
      S_free(buffer);
      S_free(filename);
      return rc;
    }
  
  Log("S_FileServer::RequestFile() file for %s opened",filename);


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
      
      Log("S_FileServer::RequestFile() Sending %s(%dkb) to %s (%dkb ch)",
          filename,size/1024,
          ip,
          chunkSize/1024);
      
      packet.Flush();
      packet.PushULong(size);
      
      int numChunks = 0;
      
      ulong totalBytesRead = 0;
      while ( totalBytesRead < size )
	{
	  
	  ulong bytesRead = fread(buffer,1,chunkSize,fptr);
	  if ( bytesRead == 0 )
	    {
	      rc = S_FILESERVER_FILEREADFAILED;
	      client->WriteULong(rc);
	      Log("S_FileServer::RequestFile() failed for %s.  "
		  "Read %d/%d bytes of the file %s.",
		  ip,totalBytesRead,size,filename);
	      fclose(fptr);
	      S_free(buffer);
	      S_free(filename);
	      return rc;
            }
	  
	  /* ************************************************************* */
	  /* *** The first write needs to look like a complete packet. *** */
	  /* *** We send the remaining parts of the packet manually.   *** */
	  /* ************************************************************* */
	  if ( totalBytesRead == 0 )
	    {
	      client->WriteULong(S_FILESERVER_OK);
	      packet.PushBuffer(chunkSize,buffer,size);
	      
	      ulong vSize = packet.packetSize;
	      packet.packetSize += size - chunkSize;
	      
	      if ( client->WriteVirtualPacket(&packet,vSize) != S_CLIENT_OK )
		{
		  rc = S_FILESERVER_PACKETWRITEFAILED;
		  Log("S_FileServer::RequestFile() failed for %s. "
		      "Failed to write initial data "
		      "block for %s.",ip,filename);
		  fclose(fptr);
		  S_free(buffer);
		  S_free(filename);
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
		bytesSent = client->Send((char *)buffer,bytesRead);
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
	      rc = S_FILESERVER_PACKETWRITEFAILED;
	      Log("S_FileServer::RequestFile() RequestFile failed for %s.  "
		  "Failed to write chunk data "
		  "for %s.",ip,filename);
	      fclose(fptr);
	      S_free(buffer);
	      S_free(filename);
	      return rc;
	    }
	  
	  totalBytesRead += bytesRead;
	  
	  _OsSleep(10000);
	}
      
      Log("S_FileServer::RequestFile() Sent %s (%dkb) to %s in %d chunks of "
	  "%dkb",
          filename,size/1024,ip,numChunks,chunkSize/1024);
      
    }
  /* ***************************************************************** */
  
  /* ***************************************************************** */
  /* *** A normal file send.  Read the entire file into memory and *** */
  /* *** send it as one big packet.  This is what we do with files *** */
  /* *** that are either smaller than chunkSize or compressed      *** */
  /* ***************************************************************** */
  if ( chunkSize == size )
    {
      
      Log("S_FileServer::RequestFile() Sending %s(%d bytes) to %s",
	  filename,size,ip);
      
      ulong bytesRead = fread(buffer,1,size,fptr);
      if ( bytesRead != size )
	{
	  rc = S_FILESERVER_FILEREADFAILED;
	  client->WriteULong(rc);
	  Log("S_FileServer::RequestFile() failed for %s.  Read %d/%d bytes "
	      "of the file %s.",
	      ip,bytesRead,size,filename);
	  fclose(fptr);
	  S_free(buffer);
	  S_free(filename);
	  return rc;
	}
      
      packet.Flush();
      packet.PushULong(size);
      packet.PushBuffer(size,buffer);
      
      if ( compress )
	{
	  packet.Compress();
	  Log("S_FileServer::RequestFile() Compressed %s packet %d->%d bytes",
	      filename,
	      packet.uncompPacketSize,
	      packet.packetSize);
	}
      
      Log("S_FileServer::RequestFile() Responding with S_FILESERVER_OK for %s",
	  filename);
      client->WriteULong(S_FILESERVER_OK);
      
      Log("S_FileServer::RequestFile() Writing packet for %s. . .",filename);
      if ( client->WritePacket(&packet) != S_CLIENT_OK )
	{
	  rc = S_FILESERVER_PACKETWRITEFAILED;
	  Log("S_FileServer::RequestFile() failed for %s.  Failed to write "
	      "data for %s.",
	      ip,filename);
	  fclose(fptr);
	  S_free(buffer);
	  S_free(filename);
	  return rc;
	}
      
      Log("S_FileServer::RequestFile() Packet sent for %s.",filename);
      numBytesDownloaded += packet.uncompPacketSize;
      numFilesDownloaded++;
      rc = S_FILESERVER_OK;
      
    }
  /* ***************************************************************** */
  
  Log("S_FileServer::RequestFile() fclose(fptr)");
  fclose(fptr);
  Log("S_FileServer::RequestFile() S_free(buffer)");
  S_free(buffer);
  Log("S_FileServer::RequestFile() S_free(filename)");
  S_free(filename);

  Log("S_FileServer::RequestFile() Responding with S_FILESERVER_OK");
  rc = S_FILESERVER_OK;
  client->WriteULong(rc);

  Log("S_FileServer::RequestFile() End RequestFile");
  
  return rc;
}


int
S_FileServer::ProcessConnectMsg(S_Client *client)
{
  switch(client->connectMsg)
    {
    case S_FILESERVER_REQUESTFILEMODDATE :
      HandleRequestFileModDateMsg(client);
      break;
    case S_FILESERVER_REQUESTFILESTATUS :
      HandleRequestFileStatusMsg(client);
      break;
    case S_FILESERVER_REQUESTFILESIZE :
      HandleRequestFileSizeMsg(client);
      break;
    case S_FILESERVER_REQUESTFILE :
      HandleRequestFileMsg(client);
      break;
    case S_FILESERVER_UPLOADFILE :
      HandleUploadFileMsg(client);
      break;
    }
  
  return 0;
}
