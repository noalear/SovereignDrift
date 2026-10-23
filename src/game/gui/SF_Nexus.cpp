/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
#include "sf.h"
#include "SF_Nexus.h"
#include "SF_Globals.h"
#include "SF_ClientWindow.h"
#include "SF_Client.h"
#include "SF_Server.h"

#define SF_NEXUS_COMPRESSTHRESHOLD 256

int _NexusThread( void * );
char *_Nexus_TimeStamp( void );

SF_Nexus::SF_Nexus(int ilog,char *ibackupHostname) {
   S_FileServer::Create(SF_NEXUS_PORT,ilog,"nexus.log");
   LoadDenyIPList("nexus.deny");
   run = TRUE;
   numServers = 0;
   maxServers = 0;
   contactTime = SF_NEXUS_CONTACT_TIME;
   dropTime = SF_NEXUS_DROP_TIME;
   bmpBannerFilename = NULL;
   urlBanner = NULL;
   backupHostname.Create(ibackupHostname);
   Load();
   for ( int i=0; i<numServers; i++ ) {
      servers[i].state = SF_NEXUS_SERVER_ALIVE;
      servers[i].timeLastContact = time(NULL);
      }

   thread = new S_Thread(_NexusThread,(void  *)this);

   if ( backupHostname.str )
     {
       backupThread.Start(SF_Nexus_BackupThread,(void *)this);
     }

   }

SF_Nexus::~SF_Nexus()
{
  Save();
  run = FALSE;

  if ( bmpBannerFilename )
    {
      S_free(bmpBannerFilename);
      bmpBannerFilename = NULL;
    }
  if ( urlBanner )
    {
      S_free(urlBanner);
      urlBanner = NULL;
    }

  if ( thread )
    {
      thread->WaitFor();
      delete thread;
      thread = NULL;
    }

  backupThread.WaitFor();
}


int
SF_Nexus::GetBannerAdInfo(char *filename) {
   if ( filename ) {
      if ( _OsFileExists(filename) ) {
         int filesize = _OsFileSize(filename);
         if ( filesize > 0 ) {
            if ( bmpBannerFilename ) {
               S_free(bmpBannerFilename);
               bmpBannerFilename = NULL;
	       }
            if ( urlBanner ) {
               S_free(urlBanner);
               urlBanner = NULL;
               }
            FILE *fptr = fopen(filename,"r");
            if ( fptr ) {
               char line[1024];
               fgets(line,1023,fptr);
               S_Record_RemoveCharacter(line,'\n');
               S_Record_RemoveCharacter(line,10);
               S_Record_RemoveCharacter(line,13);
               if ( line[0] )
                  urlBanner = S_strdup(line);
               fgets(line,1023,fptr);
               S_Record_RemoveCharacter(line,'\n');
               S_Record_RemoveCharacter(line,10);
               S_Record_RemoveCharacter(line,13);
               if ( line[0] )
                  bmpBannerFilename = S_strdup(line);
               //Log("Banner Ad",bmpBannerFilename,urlBanner);
               //Log("%s",bmpBannerFilename);
               //Log("%s",urlBanner);
               //Log("");
               fclose(fptr);
	       }
	    }
         }
      }
   return 0;
   }


int
SF_Nexus::AcceptFailed() {
   Log("Accept Failed");

   //
   // If the server fails on an accept, kill the server.
   //
   exit(0);

   return 0;
   };


int
SF_Nexus::NewClientFailed() {
   Log("New client failed");
   return 0;
   };


int
SF_Nexus::NewClientDenied(S_Client *client) {
   Log("Denied client connect from %s",client->numIPAddress);
   return 0;
   };


int
SF_Nexus::ClientThreadFailed() {
   Log("Client thread failed");

   //
   // If the server fails to spawn a new thread, kill the server.
   //
   exit(0);

   return 0;
   };


int
SF_Nexus::UpdateSortedList(char *keyword,char *name,char *value)
{
  Log("UpdateSortedList(%s,%s,%s)",keyword,name,value);

  int i;
  sortedListMutex.Request();
  char filename[1024];
  sprintf(filename,"%s.rec",keyword);
  S_Record list(filename,4096);
  if ( list.Exists() )
    {
      list.Open();

      if ( !S_Record_HasWhiteSpace(name) )
	{
	  S_Record_RemoveCharacter(name,'"');
	}

      for ( i=0; i<list.numEntries; i++ )
	{
	  if ( strcmp(list.entries[i].keyword,name) == 0 )
	    {
	      list.RemoveEntry(i);
	      i--;
	    }
	}

      i = 0;
      while ( i < list.numEntries &&
	      atol(value) < atol(list.entries[i].value) )
	{
	  i++;
	}

      if ( i >= list.numEntries )
	{
	  list.AddEntry(name,value);
	}
      else
	{
	  list.InsertEntry(i,name,value);
	}

      //
      // Do a quick bubble sort here on the entire list.  No cracks about
      // using a bubble sort.  I'm a doctor not a computer scientist.
      //
      int swapMade;
      do {
	swapMade = FALSE;
	for ( i=0; i<list.numEntries-1; i++ )
	  {
	    if ( list.entries[i].value && list.entries[i+1].value )
	      {
		if (atol(list.entries[i].value)<atol(list.entries[i+1].value))
		  {
		    char tname[S_RECORD_STRSIZE];
		    char tval[S_RECORD_STRSIZE];

		    strcpy(tname,list.entries[i].keyword);
		    strcpy(tval,list.entries[i].value);

		    strcpy(list.entries[i].keyword,list.entries[i+1].keyword);
		    strcpy(list.entries[i].value,list.entries[i+1].value);
		    strcpy(list.entries[i+1].keyword,tname);
		    strcpy(list.entries[i+1].value,tval);

		    swapMade = TRUE;
		  }
	      }
	  }

      } while ( swapMade == TRUE );

      list.Close();

    }
  else
    {
      list.New();
      list.AddEntry(name,value);
      list.Close();
    }
  sortedListMutex.Release();
  return 0;
}


int
SF_Nexus::GetVersion(S_Client *client) {
  //Log("Version request from %s",client->numIPAddress);
   S_Packet packet;
   packet.PushUChar(SF_MAJOR_VERSION);
   packet.PushUChar(SF_MINOR_VERSION);
   packet.PushUChar(SF_MINOR_MINOR_VERSION);
   packet.PushString(SF_VERSION_MODIFIER);
   packet.PushString(SF_VERSION_STRING);
   client->WritePacket(&packet);
   return 0;
   }


int
SF_Nexus::GetBannerAd(S_Client *client) {
   Log("Banner Ad request from %s",client->numIPAddress);
   S_Packet packet;
   packet.PushString(bmpBannerFilename);
   packet.PushString(urlBanner);
   client->WritePacket(&packet);
   return 0;
   }


int
SF_Nexus::GetPlayerList(S_Client *client) {
   Log("Player List request from %s",client->numIPAddress);
   S_DirList dirList("*.rec");
   if ( dirList.numFiles > 0 ) {
      S_Packet packet;
      for ( int i=0; i<dirList.numFiles; i++ ) {
         char *playerName = S_strdup(dirList.FileName(i));
         if ( playerName ) {
            char *dot = strstr(playerName,".rec");
            dot[0] = 0;
            packet.PushString(playerName);
            S_free(playerName);
	    }
         }
      if ( packet.packetSize > SF_NEXUS_COMPRESSTHRESHOLD )
         packet.Compress();
      client->WritePacket(&packet);
      }
   return 0;
   }


int
SF_Nexus::GetSortedStats(S_Client *client) {
   S_Packet packet;
   client->SetTimeOut(60000000); /* Set timeout to 1 minute */
   client->ReadPacket(&packet);
   if ( packet.readComplete ) {
      }
   return 0;
   }


int
SF_Nexus::GetHighScoreList(S_Client *client) {
   Log("High Score List request from %s",client->numIPAddress);
   S_Packet packet;
   client->SetTimeOut(60000000); /* Set timeout to 1 minute */
   client->ReadPacket(&packet);
   char *keyword = packet.PopString();
   if ( keyword ) {
      slong num = packet.PopLong();
      packet.Flush();
      if ( strlen(keyword)>0 && num > 0 ) {
         char *recordName = (char *)
                             S_malloc(strlen(keyword)+strlen(".rec")+1);
         if ( recordName ) {
            sprintf(recordName,"%s.rec",keyword);
            S_Record record(recordName);
            S_free(recordName);
            if ( record.Exists() ) {
               client->WriteLong(SF_NEXUS_OK);

               sortedListMutex.Request();
               record.Open();
               for ( int i=0; i<min(num,record.numEntries); i++ ) {
                  packet.PushString(record.entries[i].keyword);
                  packet.PushString(record.entries[i].value);
                  }
               sortedListMutex.Release();

               if ( packet.packetSize > SF_NEXUS_COMPRESSTHRESHOLD )
                  packet.Compress();
               client->WritePacket(&packet);
               }
            else {
               client->WriteLong(SF_NEXUS_NOHIGHSCORELIST);
               }
            }
         else {
            client->WriteLong(SF_NEXUS_NOHIGHSCORELIST);
            }
         }
      else {
         client->WriteLong(SF_NEXUS_NOHIGHSCORELIST);
         }
      S_free(keyword);
      }
   else {
      client->WriteLong(SF_NEXUS_MALFORMEDREQUEST);
      }
   return 0;
   }


int
SF_Nexus::PlayerData(S_Client *client) {
   Log("Player Data request %d from %s",client->connectMsg,
                                        client->numIPAddress);

   S_Packet packet;
   client->SetTimeOut(60000000); /* Set timeout to 1 minute */
   client->ReadPacket(&packet);
   char *name = packet.PopString();
   char *password = packet.PopString();
   char *email = packet.PopString();
   packet.Flush();

   if ( !name ) {
      client->WriteLong(SF_NEXUS_NAMEDOESNOTEXIST);
      if ( password ) S_free(password);
      if ( email ) S_free(email);
      return 0;
      }

   S_Record_RemoveCharacter(name,'"');

   if ( !password ) {
      client->WriteLong(SF_NEXUS_WRONGPASSWORD);
      if ( name ) S_free(name);
      if ( email ) S_free(email);
      return 0;
      }

   char *recordName = (char *)S_malloc(strlen(name)+strlen(".rec")+1);
   if ( !recordName ) {
      client->WriteLong(SF_NEXUS_MALLOCFAILED);
      S_free(name);
      S_free(password);
      if ( email )
         S_free(email);
      return 0;
      }

   sprintf(recordName,"%s.rec",name);
   S_Record record(recordName);
   S_free(recordName);

   if ( client->connectMsg == SF_NEXUS_PLAYEREXISTS ) {
      if ( record.Exists() )
         client->WriteLong(SF_NEXUS_NAMEALREADYEXISTS);
      else
         client->WriteLong(SF_NEXUS_NAMEDOESNOTEXIST);
      S_free(name);
      S_free(password);
      if ( email )
         S_free(email);
      return 0;
      }

   if ( client->connectMsg == SF_NEXUS_NEWPLAYER && email ) {
      if ( record.Exists() ) {

	 playerRecordMutex.Request();
         record.Open();
         if ( record.GetEntry("Password") &&
              strcmp(password,record.GetEntry("Password")) )
            client->WriteLong(SF_NEXUS_NAMEALREADYEXISTS);
         else {
            client->WriteLong(SF_NEXUS_OK);
            if ( record.GetEntry("Email") )
               record.SetEntry("Email",email);
            else
               record.AddEntry("Email",email);
            }
         record.Close();
	 playerRecordMutex.Release();

         S_free(name);
         S_free(password);
         if ( email )
            S_free(email);
         return 0;
         }
      Log("New Player %s(%s) from %s",name,email,client->numIPAddress);

      playerRecordMutex.Request();
      record.New();
      record.Open();
      record.AddEntry("Password",password);
      record.AddEntry("Email",email);
      record.Close();
      playerRecordMutex.Release();

      client->WriteLong(SF_NEXUS_OK);
      }


   //
   // Set the time of the current access and calculate how much
   // time has elapsed since the record was last accessed.
   //
   slong elapsedTime = 0;
   if ( client->connectMsg == SF_NEXUS_SETPLAYERDATA ||
        client->connectMsg == SF_NEXUS_GETPLAYERDATA )
     {

       if ( !record.Exists() )
	 {
	   Log("Record for %s does not exist\n",name);
	   client->WriteLong(SF_NEXUS_NAMEDOESNOTEXIST);
	   S_free(name);
	   S_free(password);
	   if ( email )
	     S_free(email);
	   return 0;
         }

      playerRecordMutex.Request();
      record.Open();
      char *keyword = "LastAccess";

      slong lastAccessTime = 0;
      char *value = record.GetEntry(keyword);
      if ( value )
	{
          lastAccessTime = atol(value);
	}

      time_t currentTime;
      currentTime = time(&currentTime);

      elapsedTime = currentTime-lastAccessTime;
      if ( elapsedTime <= 0 )
	{
	  elapsedTime = 1;
	}

      S_String stime("%d",currentTime);

      if ( record.GetIndex(keyword) >= 0 )
        {
          record.SetEntry(keyword,stime.str);
        }
      else
        {
          record.AddEntry(keyword,stime.str);
        }
      record.Close();
      playerRecordMutex.Release();
     }

   if ( client->connectMsg == SF_NEXUS_SETPLAYERDATA )
     {

       if ( !record.Exists() )
	 {
	   Log("Record for %s does not exist\n",name);
	   client->WriteLong(SF_NEXUS_NAMEDOESNOTEXIST);
	   S_free(name);
	   S_free(password);
	   if ( email )
	     S_free(email);
	   return 0;
         }

       playerRecordMutex.Request();
       record.Open();
       if ( !record.GetEntry("Password") ||
	    (record.GetEntry("Password") &&
	     strcmp(password,record.GetEntry("Password"))) )
	 {
	   Log("Wrong password %s != %s for %s\n",
	       password,record.GetEntry("Password"),name);
	   client->WriteLong(SF_NEXUS_WRONGPASSWORD);
	   S_free(name);
	   S_free(password);
	   if ( email )
	     S_free(email);
	   record.Close();
           playerRecordMutex.Release();
	   return 0;
         }
       record.Close();
       playerRecordMutex.Release();

       client->WriteLong(SF_NEXUS_OK);

       packet.Flush();
       client->SetTimeOut(60000000); /* Set timeout to 1 minute */
       client->ReadPacket(&packet);

       Log("Set Player Data %s(%s) from %s",name,email,client->numIPAddress);

       playerRecordMutex.Request();
       record.Open();

       char *keyword;
       char *value;

       //
       // Pop the user data from the packet and put it in the record.
       //
       do
	 {
	   keyword = packet.PopString();
	   value = packet.PopString();
	   if ( keyword && value )
	     {

               slong numericValue = atol(value);

	       slong numericEntry = 0;
	       char *existingEntry = record.GetEntry(keyword);
	       if ( existingEntry )
		 {
		   numericEntry = atol(existingEntry);
		 }

               slong increase = numericValue - numericEntry;

	       slong rate = 0;
	       if ( elapsedTime > 0 )
		 {
		   rate = increase/elapsedTime;
		 }

	       int entryExists = record.GetIndex(keyword) >= 0;
	       int entryIsBlank = !(existingEntry && strlen(existingEntry)>0);
	       int entryIsANumber = numericEntry > 0;

               if ( entryIsANumber )
		 {
                   Log("%s for %s has increased %d in %d seconds (%d /s)",
                       keyword,name,increase,elapsedTime,rate);
		 }

	       int ignoreIncrease = FALSE;

	       int checkForCheat = FALSE;
	       if ( keyword )
		 {
		   Log("Check %s for cheat?",keyword);
		   checkForCheat = !strcmp("Score",keyword);
		 }

	       if ( checkForCheat && rate > 100 )
		 {
		   ignoreIncrease = TRUE;
		   Log("Cheat found");
		   Log("Ignoring increase in %s of (%d /s) for %s",
                       keyword,rate,name);
		 }

	       if ( !ignoreIncrease )
		 {
		   
		   if ( !entryExists )
		     {
		       record.AddEntry(keyword,value);
		     }
		   else if ( entryIsBlank )
		     {
		       record.SetEntry(keyword,value);
		     }
		   else if ( entryIsANumber )
		     {
		       if ( atol(value) > 0 )
			 {
			   if ( !strcmp(name,"Imposter") )
			     {
			       sprintf(value,"0");
			     }
			   record.SetEntry(keyword,value);
			 }
		     }
		   else
		     {
		       if ( strlen(value) > 0 )
			 {
			   record.SetEntry(keyword,value);
			 }
		     }
		   
		   if ( entryIsANumber )
		     {
		       UpdateSortedList(keyword,name,value);
		     }
		 }
	     }
	   
	   if ( keyword )
	     {
	       S_free(keyword);
	     }

	   if ( value )
	     {
	       S_free(value);
	     }

	 } while ( keyword && value );

       record.Close();
       playerRecordMutex.Release();
     }

   packet.Flush();
   if ( client->connectMsg == SF_NEXUS_GETPLAYERDATA ||
        client->connectMsg == SF_NEXUS_GETPLAYERDATA2 ) {
      Log("Get Player Data %s from %s",name,client->numIPAddress);

      if ( !record.Exists() ) {
         client->WriteLong(SF_NEXUS_NAMEDOESNOTEXIST);
         S_free(name);
         S_free(password);
         if ( email )
            S_free(email);
         return 0;
         }

      playerRecordMutex.Request();
      record.Open();

      if ( client->connectMsg == SF_NEXUS_GETPLAYERDATA &&
           record.GetEntry("Password") &&
           strcmp(password,record.GetEntry("Password")) ) {
         client->WriteLong(SF_NEXUS_WRONGPASSWORD);
         S_free(name);
         S_free(password);
         if ( email )
            S_free(email);
         record.Close();
         playerRecordMutex.Release();
         return 0;
         }

      client->WriteLong(SF_NEXUS_OK);

      for ( int i=0; i<record.numEntries; i++ ) {
         if ( strcmp(record.entries[i].keyword,"Password") ) {
            packet.PushString(record.entries[i].keyword);
            packet.PushString(record.entries[i].value);
	    }
         }
      record.Close();
      playerRecordMutex.Release();
      if ( packet.packetSize > SF_NEXUS_COMPRESSTHRESHOLD )
         packet.Compress();
      client->WritePacket(&packet);
      }

   S_free(name);
   S_free(password);
   if ( email )
      S_free(email);

   return 0;
   }


int
SF_Nexus::ServerStart(S_Client *client) {
  uchar majorVersion;
  uchar minorVersion;
  client->SetTimeOut(60000000); /* Set timeout to 1 minute */
  client->ReadUChar(&majorVersion);
  client->ReadUChar(&minorVersion);

  if ( majorVersion != SF_MAJOR_VERSION ||
       minorVersion != SF_MINOR_VERSION )
    {
      client->WriteULong(SF_SERVER_WRONGVERSION);
      client->WriteUChar(SF_MAJOR_VERSION);
      client->WriteUChar(SF_MINOR_VERSION);

      //
      // Don't allow servers older than 1.09 to play
      //
      if ( majorVersion < 1 || minorVersion < 9 )
	{
          return 0;
	}
    }
  else
    {
      client->WriteULong(SF_NEXUS_OK);
    }

  S_Packet packet;
  ushort numPlayers = 0;
  ushort numHumanPlayers = 0;
  ushort maxSlots = 0;
  char *osString = NULL;
  char *serverName = NULL;
  uchar minorminorVersion = 0;
  char *versionModifier = NULL;
  char *versionString = NULL;
  ushort serverPort = SF_SERVER_PORT;
  char *playerList = NULL;
  char *numIPAddress = NULL;

  S_String upTimeStr;

  client->SetTimeOut(60000000); /* Set timeout to 1 minute */
  client->ReadPacket(&packet);
  if ( packet.readComplete )
    {
      numPlayers = packet.PopUShort();
      maxSlots = packet.PopUShort();
      majorVersion = packet.PopUChar();
      minorVersion = packet.PopUChar();
      osString = packet.PopString();
      serverName = packet.PopString();
      minorminorVersion = packet.PopUChar();
      versionModifier = packet.PopString();
      versionString = packet.PopString();
      serverPort = packet.PopUShort();
      numHumanPlayers = packet.PopUShort();
      playerList = packet.PopString();
      ulong uptime = packet.PopULong();
      numIPAddress = packet.PopString();

      //ulong sec = uptime % 60;
      ulong minute = (uptime / 60) % 60;
      ulong hour = (uptime / 3600) % 24;
      ulong day = uptime / 86400;
      upTimeStr.Create("%dd %02d|%02d",day,hour,minute);

      if ( serverName )
	{
          for ( int i=0; i<(int)strlen(serverName); i++ )
	    {
              if ( serverName[i] == ':' )
		{
                  serverName[i] = ' ';
		}
	    }
	}
    }

  int addServerToList = TRUE;

  if ( addServerToList )
    {

      S_String serverString("%s:%d:%s:%s:%d:%d:%s:%d:%s:%s:%s",
                            client->numIPAddress,
                            serverPort,
                            serverName,
                            SF_Client::VersionString(majorVersion,
	                    minorVersion,
			    minorminorVersion,
			    versionModifier,
			    versionString),
                            numPlayers,
                            maxSlots,
                            osString,
                            numHumanPlayers,
			    playerList,
			    upTimeStr.str,
			    numIPAddress);

      serverListMutex.Request();
      if ( serverString.str &&
           !ActivateServer(serverString.str,SF_NEXUS_SERVER_ALIVE) )
	{
	  AddServer(serverString.str);
	  Log("server started on %s",serverString.str);
	}
      serverListMutex.Release();
    }

  if ( osString )
    {
      S_free(osString);
      osString = NULL;
    }
  if ( serverName )
    {
      S_free(serverName);
      serverName = NULL;
    }
 if ( versionModifier )
    {
      S_free(versionModifier);
      versionModifier = NULL;
    }
  if ( versionString )
    {
      S_free(versionString);
      versionString = NULL;
    }
  if ( playerList )
    {
      S_free(playerList);
      playerList = NULL;
    }
  if ( numIPAddress )
    {
      S_free(numIPAddress);
      numIPAddress = NULL;
    }
  return 0;
}


int
SF_Nexus::ServerStop(S_Client *client) {
   int port = SF_SERVER_PORT;

   S_Packet packet;
   client->SetTimeOut(60000000); /* Set timeout to 1 minute */
   client->ReadPacket(&packet);
   if ( packet.readComplete )
     {
       port = packet.PopUShort();
     }

   Log("Server stopped on %s:%d",client->numIPAddress,port);

   S_String serverString("%s:%d",client->numIPAddress,port);

   serverListMutex.Request();
   ActivateServer(serverString.str,SF_NEXUS_SERVER_DEAD);
   client->WriteULong(SF_NEXUS_OK);
   serverListMutex.Release();
   return 0;
   }


int
SF_Nexus::ServerList(S_Client *client) {
   Log("Server List request from %s",client->numIPAddress);

   serverListMutex.Request();

   int i;
   slong numActiveServers = 0;
   for ( i=0; i<numServers; i++ ) {
      if ( servers[i].state == SF_NEXUS_SERVER_ALIVE )
         numActiveServers++;
      }

   S_Packet packet;
   packet.PushULong(SF_NEXUS_OK);
   packet.PushULong(numActiveServers);
   for ( i=0; i<numServers; i++ )
     {
       if ( servers[i].state == SF_NEXUS_SERVER_ALIVE )
	 {
	   //
	   // For people playing behind NAT firewall that want to play
	   // on their own server.  Detect this and send them the local
	   // subnet IP address of their server.
	   //
	   S_TokenArray tokens(servers[i].string,':',':',
			       S_TOKENARRAY_SINGLEDELIMIT);
	   if ( tokens.numLines >= 11  &&
		tokens.lines[10] &&
		strcmp(tokens.lines[10],"(null)") &&
		!strcmp(client->numIPAddress,tokens.lines[0]) )
	     {
	       //
	       // Patch the return string for clients behind NAT with server.
	       //
	       S_String serverString("%s:%s:%s:%s:%s:%s:%s:%s:%s:%s:%s",
				     tokens.lines[10],
				     tokens.lines[1],
				     tokens.lines[2],
                                     tokens.lines[3],
				     tokens.lines[4],
				     tokens.lines[5],
				     tokens.lines[6],
				     tokens.lines[7],
				     tokens.lines[8],
				     tokens.lines[9],
				     tokens.lines[10]);
	       packet.PushString(serverString.str);
	     }
	   else
	     {
	       packet.PushString(servers[i].string);
	     }
         }
     }

   serverListMutex.Release();

   client->WritePacket(&packet);

   return 0;
   }


int
SF_Nexus::ProcessConnectMsg(S_Client *client)
{
  
  switch(client->connectMsg)
    {
    case SF_NEXUS_STOP :
      Stop();
      break;
    case S_SERVER_REQUESTTIME :
      HandleRequestTimeMsg(client);
      break;
    case S_FILESERVER_REQUESTFILELIST :
      HandleRequestFileListMsg(client);
      break;
    case S_FILESERVER_REQUESTFILESTATUS :
      HandleRequestFileStatusMsg(client);
      break;
    case S_FILESERVER_REQUESTFILEMODDATE :
      HandleRequestFileModDateMsg(client);
      break;
    case S_FILESERVER_REQUESTFILESIZE :
      HandleRequestFileSizeMsg(client);
      break;
    case S_FILESERVER_REQUESTFILE :
      HandleRequestFileMsg(client);
      break;
      
    case SF_NEXUS_GETVERSION :
      GetVersion(client);
      break;
      
    case SF_NEXUS_DOWNLOADFILE :
      DownloadFile(client);
      //HandleRequestFileMsg(client);
      break;
      
    case SF_NEXUS_GETBANNERAD :
      GetBannerAd(client);
      break;
      
    case SF_NEXUS_GETHIGHSCORELIST :
      GetHighScoreList(client);
      break;
      
    case SF_NEXUS_GETSORTEDSTATS :
      GetSortedStats(client);
      break;
      
    case SF_NEXUS_GETPLAYERLIST :
      GetPlayerList(client);
      break;
      
    case SF_NEXUS_PLAYEREXISTS :
    case SF_NEXUS_GETPLAYERDATA :
    case SF_NEXUS_GETPLAYERDATA2 :
    case SF_NEXUS_SETPLAYERDATA :
    case SF_NEXUS_NEWPLAYER :
      PlayerData(client);
      break;
      
    case SF_NEXUS_SERVERSTART :
      ServerStart(client);
      break;
      
    case SF_NEXUS_SERVERSTOP :
      ServerStop(client);
      break;
      
    case SF_NEXUS_SERVERLIST :
      ServerList(client);
      break;
    }
  
  return 0;
}


int
_NexusThread( void *arg ) {

   SF_Nexus *nexus = (SF_Nexus *)arg;
   S_Timer timer;

   timer.Set(100000);
   //timer.Set(nexus->contactTime*1000000);
   while( nexus->run ) {

      for ( int i=0; i<nexus->numServers; i++ ) {

         /* ************************************************************* */
         /* *** This block of code assumes that servers will register *** */
         /* *** themselves with the NEXUS every nexus->contactTime    *** */
         /* *** milliseconds.  It checks to see how long it has been  *** */
         /* *** since they last checked in.                           *** */
         /* ************************************************************* */
         time_t dt = time(NULL) - nexus->servers[i].timeLastContact;

         nexus->serverListMutex.Request();

         if ( nexus->servers[i].state == SF_NEXUS_SERVER_ALIVE &&
              dt > nexus->contactTime*3 ) {
            nexus->servers[i].state = SF_NEXUS_SERVER_DEAD;
            nexus->Log("%s:%s is Dead",nexus->servers[i].ipaddr,
                                       nexus->servers[i].port);
            }
         else if ( nexus->servers[i].state == SF_NEXUS_SERVER_DEAD &&
              dt < nexus->contactTime*2 ) {
            nexus->servers[i].state = SF_NEXUS_SERVER_ALIVE;
            nexus->Log("%s:s is Alive",nexus->servers[i].ipaddr,
                                       nexus->servers[i].port);
            }

         if ( dt > nexus->dropTime ) {
            nexus->Log("%s:%d is being dropped",nexus->servers[i].ipaddr,
                                               nexus->servers[i].port);
            nexus->RemoveServer(i);
            }

         nexus->serverListMutex.Release();

         }
      timer.WaitFor();
      }
   return 0;
   }


int
SF_Nexus::Load() {
   FILE *fptr;
   if ( (fptr=fopen("nexus.dat","r")) != NULL ) {
      slong rc;
      char temp[1024];
      while ( (rc = (slong)fgets(temp,1023,fptr)) != (slong)NULL ) {
         temp[strlen(temp)-1] = 0;
         AddServer(temp);
         }
      fclose(fptr);
      }
   return 0;
   }


int
SF_Nexus::Save() {
   FILE *fptr = fopen("nexus.dat","w");
   if ( fptr ) {
      for ( int i=0; i<numServers; i++ ) {
        fprintf(fptr,"%s\n",servers[i].string);
        }
      fclose(fptr);
      }
   return 0;
   }


int
SF_Nexus::AddServer(char *serverString) {

   if ( maxServers == 0 ) {
      maxServers = 1;
      servers = (ServerStruct *)S_malloc(sizeof(ServerStruct));
      }
   else {
      if ( numServers + 1 > maxServers )
         maxServers = numServers + 1;
      servers=(ServerStruct *)S_realloc(servers,sizeof(ServerStruct)*maxServers);
      }

   if ( !servers ) {
      maxServers = 0;
      return 0;
      }

   strcpy(servers[numServers].string,serverString);

   S_TokenArray tokens(serverString,':',':',S_TOKENARRAY_SINGLEDELIMIT);

   if ( tokens.numLines >= 1 && tokens.lines[0] )
     {
       strcpy(servers[numServers].ipaddr,tokens.lines[0]);
     }

   if ( tokens.numLines >= 2 && tokens.lines[1] )
     {
       strcpy(servers[numServers].port,tokens.lines[1]);
     }

   servers[numServers].state = SF_NEXUS_SERVER_ALIVE;
   servers[numServers].timeLastContact = time(NULL);
   numServers++;

   Save();
   return 0;
   }


int
SF_Nexus::RemoveServer(int index) {

   if ( index<0 || index>=numServers ) {
      return 0;
      }

   for ( int i=index; i<numServers-1; i++ ) {
      strcpy(servers[i].string,servers[i+1].string);
      strcpy(servers[i].ipaddr,servers[i+1].ipaddr);
      strcpy(servers[i].port,servers[i+1].port);
      servers[i].state = servers[i+1].state;
      servers[i].timeLastContact = servers[i+1].timeLastContact;
      }
   numServers--;

   Save();

   return 0;
   }


int
SF_Nexus::ActivateServer(char *serverString, int state)
{
  for ( int i=0; i<numServers; i++ )
    {
      S_TokenArray tokens(serverString,':',':',S_TOKENARRAY_SINGLEDELIMIT);

      char *serverIP = "";
      if ( tokens.numLines >= 1 )
	{
	  serverIP = tokens.lines[0];
	}

      char *serverPort = "20001";
      if ( tokens.numLines >= 2 )
	{
	  serverPort = tokens.lines[1];
	}

      if ( strcmp(servers[i].ipaddr,serverIP) == 0 &&
           strcmp(servers[i].port,serverPort) == 0 )
	{

	  if ( servers[i].state == SF_NEXUS_SERVER_DEAD && 
	       state == SF_NEXUS_SERVER_ALIVE )
	    {
	      Log("Server started on %s",servers[i].ipaddr);
	    }
          strcpy(servers[i].string,serverString);
          strcpy(servers[i].ipaddr,serverIP);
          strcpy(servers[i].port,serverPort);
	  servers[i].state = state;
	  servers[i].timeLastContact = time(NULL);

	  if ( state == SF_NEXUS_SERVER_DEAD )
	    {
	      RemoveServer(i);
	    }

          Save();

	  return TRUE;
	}

    }

  return FALSE;
}


int
SF_Nexus::Profile()
{
  for ( int i=0; i<numServers; i++ )
    Log("%s",servers[i].string);
  return 0;
}


int
SF_Nexus::DownloadFile(S_Client * client) {
   Log("Download request from %s",client->numIPAddress);
   S_Packet packet;
   client->SetTimeOut(60000000); /* Set timeout to 1 minute */
   client->ReadPacket(&packet);
   ulong flag = packet.PopULong();
   char *filename = packet.PopString();
   if ( filename ) {

      _OsFilename(filename);

      ulong size = _OsFileSize(filename);

      if ( flag == SF_NEXUS_GETFILEEXISTS ) {
         client->WriteULong(_OsFileExists(filename));
         S_free(filename);
         return 0;
         }
      if ( flag == SF_NEXUS_GETFILESIZE ) {
         client->WriteULong(size);
         S_free(filename);
         return 0;
         }

      if ( size > 0 ) {
         Log("Sending %s(%d bytes) to %s",filename,size,
                                            client->numIPAddress);
         char *buffer = (char *)S_malloc(size);
         if ( buffer ) {
            client->WriteULong(SF_NEXUS_OK);
            FILE *fptr = fopen(filename,"rb");
            if ( fptr ) {
               fread(buffer,size,1,fptr);
               fclose(fptr);
               }
            packet.Flush();
            packet.PushULong(size);
            packet.PushBuffer(size,buffer);
            packet.Compress();
            int rc = client->WritePacket(&packet);
            S_free(buffer);
            if ( rc != S_CLIENT_OK )
               Log("Write failed to %s (%d/%d bytes sent).",
                   client->numIPAddress,packet.index,packet.packetSize);
            }
         }
      else
         client->WriteULong(SF_NEXUS_FILEDOESNOTEXIST);
      S_free(filename);
      }
   return 0;
   }


int
SF_Nexus_BackupThread(void *iarg)
{
  SF_Nexus *nexus = (SF_Nexus *)iarg;

  int counter = 0;

  _OsSleep(1000000);  // Wait 1 second to allow object to get created

  S_Timer timer(3600000000UL); // 1 hour intervals

  //
  // Get the time difference between the two servers
  //
  ulong timeDiff = 0xFFFFFFFF;
  S_Client c;
  if (!c.Connect(nexus->backupHostname.str,SF_NEXUS_PORT,S_SERVER_REQUESTTIME))
    {
      S_Packet p;
      c.ReadPacket(&p);
      if ( p.readComplete )
	{
	  timeDiff = time(NULL) - p.PopULong();
          nexus->Log("Time difference is %d seconds to nexus %s",
	      timeDiff,
	      nexus->backupHostname.str);
	}
      c.Disconnect();
    }

  while ( nexus->run )
    {
      S_Packet p;
      S_FileClient fc;
      fc.SetTimeOut(600000000);

      if ( counter % 24 == 12) // Only do it every 24 hours starting 12 hours
	                       // after the last restart.
	{
	  for ( int i=1; i<256; i++ )
	    {
              char ch = (char)i;
	      if ( ch != '*' && ch != '?' )
	      {
		S_String mask("%c*.rec",ch);
		//S_String mask("*.rec");

		nexus->Log("Requesting file list (%s) from %s",
			   mask.str,
			   nexus->backupHostname.str);

		fc.RequestFileList(nexus->backupHostname.str,
				   SF_NEXUS_PORT,
				   mask.str,
				   &p);

		int numFiles = p.PopULong();

		if ( fc.rc != S_FILECLIENT_OK )
		  {
		    nexus->Log("%s",fc.ErrorString());
		  }

		nexus->Log("List of %d files received.",numFiles);

		for ( int i=0; i<numFiles; i++ )
		  {

		    //
		    // Properties of remote file
		    //
		    char *filename = p.PopString();
		    /*  ulong status   = */p.PopULong();
		    ulong size     = p.PopULong();
		    ulong checksum = p.PopULong();
		    /*ulong modDate  = */p.PopULong();

		    if ( filename )
		      {

			//
			// Properties of local file
			//
			ulong localSize = 0;
			ulong localChecksum = _OsFileCheckSum(filename,
							      0,
							      0,
							      &localSize);
			//ulong localFileExists = localSize > 0;

			if ( checksum != localChecksum || size != localSize )
			  {
			    S_Packet filePacket;

			    nexus->Log("Requesting file %d/%d %s",
				       i,
				       numFiles,
				       filename);

			    fc.SetTimeOut(60000000);

			    fc.RequestFile(nexus->backupHostname.str,
					   SF_NEXUS_PORT,
					   filename,
					   &filePacket);

			    if ( fc.rc != S_FILECLIENT_OK )
			      {
				nexus->Log("%s",fc.ErrorString());
			      }

			    char *tempFilename = "temp.rec";

			    if ( fc.rc == S_FILECLIENT_OK )
			      {
				nexus->playerRecordMutex.Request();
				fc.WriteFile(tempFilename,&filePacket);
				nexus->playerRecordMutex.Release();
			      }

			    if ( fc.rc != S_FILECLIENT_OK )
			      {
				nexus->Log("%s",fc.ErrorString());
			      }

			    //
			    // Figure out which record is newer.  The one on
			    // the main nexus or the one on the backup nexus.
			    //
			    ulong backupLastAccessed = 0;
			    ulong mainLastAccessed = 0;
                            int mainIsNewer = TRUE;

			    if ( timeDiff != 0xFFFFFFFF )
			      {

				//
				// Get time of last access on the main nexus.
				//				
				S_Record mainRec(tempFilename);
				nexus->playerRecordMutex.Request();
				mainRec.Open();
				mainLastAccessed = 
				  atol(mainRec.GetEntry("LastAccess"));
				mainRec.Close();
				nexus->playerRecordMutex.Release();

				//
				// Get time of last access on local backup.
				//				
				S_Record backupRec(filename);
				nexus->playerRecordMutex.Request();
				backupRec.Open();
				backupLastAccessed = 
				  atol(backupRec.GetEntry("LastAccess"));
				backupRec.Close();
				nexus->playerRecordMutex.Release();

				//
				// Add the time difference to the main nexus
				//
				if ( mainLastAccessed != 0 )
				  {
				    mainLastAccessed += timeDiff;
                                    if ( backupLastAccessed>mainLastAccessed )
				      {
                                        mainIsNewer = FALSE;
				      }
				  }

			      }

			    //
			    // If record on main nexus is newer then copy
			    // temporary record to record filename.
			    //
			    if ( mainIsNewer )
			      {
				if ( _OsCopyFile(tempFilename,filename) !=
				     S_OS_OK )
				  {
				    nexus->Log("Error copying %s to %s",
					       tempFilename,filename);
				  }
			      }

			    //
			    // If record on main nexus is older then send
			    // backup record data to main nexus.
			    //
			    if ( !mainIsNewer )
			      {

				nexus->Log("Sending player record %s to %s",
					   filename,
					   nexus->backupHostname.str);
				S_Record rec(filename);
				nexus->playerRecordMutex.Request();
				rec.Open();

				S_Client c;
				c.SetTimeOut(SF_CLIENT_TIMEOUT);
				if ( c.Connect(nexus->backupHostname.str,
					       SF_NEXUS_PORT,
					       SF_NEXUS_SETPLAYERDATA) ==
				     S_CLIENT_OK) {
				  slong rc;
				  S_Packet p;

				  char *dot = strstr(filename,".rec");
				  if ( dot )
				    {
				      dot[0] = 0;
				      p.PushString(filename);
                                      dot[0] = '.';
				    }
				  else
				    {
				      p.PushString(filename);
				    }

				  p.PushString(rec.GetEntry("Password"));
				  c.WritePacket(&p);
				  c.ReadLong(&rc);
				  if ( rc == SF_NEXUS_OK ) {
				    p.Flush();

				    if ( rec.GetIndex("Score") >= 0 )
				      {
					p.PushString("Score");
					p.PushString(rec.GetEntry("Score"));
				      }

				    if ( rec.GetIndex("Losses") >= 0 )
				      {
					p.PushString("Losses");
					p.PushString(rec.GetEntry("Losses"));
				      }

				    if ( rec.GetIndex("TimeOnline") >= 0 )
				      {
					p.PushString("TimeOnline");
					p.PushString(rec.GetEntry("TimeOnline"));
				      }

				    if ( rec.GetIndex("Rank") >= 0 )
				      {
					p.PushString("Rank");
					p.PushString(rec.GetEntry("Rank"));
				      }

				    if ( rec.GetIndex("Credits") >= 0 )
				      {
					p.PushString("Credits");
					p.PushString(rec.GetEntry("Credits"));
				      }

				    c.WritePacket(&p);
				  } // if rc == SF_NEXUS_OK

				  if ( rc != SF_NEXUS_OK )
				    {
				      nexus->Log("Error sending record. rc %d",
						 rc);
				    }

				  c.Disconnect();
				} // if Connect succeeded

				if ( c.rc != S_CLIENT_OK )
				  {
				    nexus->Log("%s",c.ErrorString());
				  }

				rec.Close();
				nexus->playerRecordMutex.Release();
				
			      } // !mainIsNewer


			  }

			S_free(filename);
			filename = NULL;
		      }
		  }
	      }
	    }
	}
      //timer.WaitFor();
      _OsSleep(3600000000UL);
      counter++;
  

    }

  return 0;
}
