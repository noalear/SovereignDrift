/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
#define S_LIB_CONSOLE
#define SF_MAIN

#include "sf.h"
#include "SF_Nexus.h"

#define DB_ACTION_SET 0
#define DB_ACTION_GET 1

int InteractiveMode();

int
main( int argc, char *argv[] ) {

  if ( argc == 1 )
    {
      InteractiveMode();
      return 0;
    }


  int action = DB_ACTION_GET;
  char *actionStr[2] = {"Set","Get"};
  char *key = "Score";
  char *val = "0";
  char *mask = "*.rec";

  S_StringArray maskArray;

  int i=1;
  do {
    if ( argv[i] ) {
      if ( !strcmp(argv[i],"-set") ) {
	action = DB_ACTION_SET;
	key = argv[++i];
      }
      else if ( !strcmp(argv[i],"-val") ) {
	val = argv[++i];
      }
      else {
	mask = argv[i];
	maskArray.AppendLine(mask);
      }
      i++;
    }
  } while ( i < argc );

  printf("%s %s %s %s\n",actionStr[action],key,val,mask);

  int j;
  for ( j=0; j<maskArray.numLines; j++ ) {
    S_DirList list(maskArray.lines[j]);
    for ( i=0; i<list.numFiles; i++ ) {
      S_Record record(list.FileName(i));
      if ( record.Exists() ) {
	record.Open();

	if ( action == DB_ACTION_GET )
	  record.GetEntry(key);
	else if ( action == DB_ACTION_SET )
	  record.SetEntry(key,val);

	record.Close();
      }
    }
  }

  if ( action == DB_ACTION_SET ) {
    S_String file;
    file.Create("%s.rec",key);
    printf("Delete %s\n",file.str);
    _OsDeleteFile(file.str);
  }

  return(0);
}


int
InteractiveMode()
{
  printf("\n");
  printf("Stellar Frontier Database Client\n");
  printf("Copyright (c) 1997-2002 Stardock Systems\n");
  printf("All rights reserved\n");
  printf("\n");
  printf("Type \"quit\" at any time to exit\n");
  
  int exitProgram = FALSE;
  
  char nexus[256] = {0};
  int foundNexus = FALSE;
  do 
    {
      printf("\n");
      printf("Nexus(sf.stardock.net)> ");
      fflush(NULL);
      fgets(nexus,255,stdin);
      
      //
      // Strip out newline characters
      //
      for ( int i=0; i<(int)strlen(nexus); i++ )
	{
	  if ( nexus[i] == 10 || nexus[i] == 13 )
	    {
	      nexus[i] = 0;
	    }
	}
      
      //
      // If nothing entered, set default
      //
      if ( strlen(nexus) == 0 )
	{
	  strcpy(nexus,"sf.stardock.net");
	}
      
      //
      // Quit if desired.
      //
      if ( !strcmp(nexus,"quit") )
	{
	  printf("Bye");
	  return 0;
	}
      
      printf("Connecting to %s . . . ",nexus);
      fflush(NULL);
      
      //
      // Check to see if nexus is up.
      //
      S_Client c;
      c.SetTimeOut(60000000);
      int rc = c.Connect(nexus,SF_NEXUS_PORT,SF_NEXUS_GETVERSION);
      if ( rc == SF_NEXUS_OK )
	{
	  foundNexus = TRUE;
	  c.Disconnect();
	  printf("OK\n");
	}
      else
	{
	  printf("Failed.  %s\n",c.ErrorString());
	}
      
    } while ( !foundNexus );
  
  do {
    
    int i;
    char player[256] = {0};
    char password[256] ={0};
    int foundPlayer = FALSE;
    do
      {
	printf("\n");
	printf("Name or (Score,Kills,Losses,Rank,...) to see sorted stats\n");
	printf("Player> ");
	fflush(NULL);
	fgets(player,255,stdin);

	//
	// Strip out newline characters
	//
	for ( i=0; i<(int)strlen(player); i++ )
	  {
	    if ( player[i] == 10 || player[i] == 13 )
	      {
		player[i] = 0;
	      }
	  }

	//
	// Quit if desired.
	//
	if ( !strcmp(player,"quit") )
	  {
	    printf("Bye");
	    return 0;
	  }

	printf("Retrieving Stats for %s on %s . . . ",player,nexus);
	fflush(NULL);

	S_Packet filePacket;

	S_FileClient fc;
	fc.SetTimeOut(60000000);

	char filename[256];
	sprintf(filename,"%s.rec",player);

	if ( strlen(player) > 0 )
	  {
	    fc.RequestFile(nexus,SF_NEXUS_PORT,filename,&filePacket);

	    if ( fc.rc != S_FILECLIENT_OK )
	      {
		printf("%s",fc.ErrorString());
	      }
	    else
	      {
		printf("OK");
		foundPlayer = TRUE;
	      }
	  }

	printf("\n");


	if ( foundPlayer )
	  {
	    char *tempFilename = "temp.rec";
      
	    if ( fc.rc == S_FILECLIENT_OK )
	      {
		fc.WriteFile(tempFilename,&filePacket);
	      }
      
	    if ( fc.rc != S_FILECLIENT_OK )
	      {
		printf("%s\n",fc.ErrorString());
	      }

	    S_Record rec("temp.rec");
	    rec.Open();
	    strcpy(password,rec.GetEntry("Password"));
	    printf("%s {\n",player);
	    for ( i=0; i<min(rec.numEntries,20); i++ )
	      {
		printf("   %-15s = %s\n",
		       rec.entries[i].keyword,
		       rec.entries[i].value);
	      }
	    printf("   }\n");
	    rec.Close();
	  }

      } while ( !foundPlayer );



    int quit = FALSE;
    char keyword[256];
    char value[256];

    if ( strlen(password) > 0 )
      {
	do 
	  {

	    int foundKeyword = FALSE;
	    do {
	      printf("Keyword> ");
	      fflush(NULL);
	      fgets(keyword,255,stdin);

	      //
	      // Strip out newline characters
	      //
	      for ( int i=0; i<(int)strlen(keyword); i++ )
		{
		  if ( keyword[i] == 10 || keyword[i] == 13 )
		    {
		      keyword[i] = 0;
		    }
		}
	
	      //
	      // Quit if desired.
	      //
	      if ( !strcmp(keyword,"quit") )
		{
		  printf("Bye");
		  _OsDeleteFile("temp.rec");
		  return 0;
		}
	
	      //
	      // If something entered then parse the keyword
	      //
	      if ( strlen(keyword) > 0 )
		{
		  S_Record rec("temp.rec");
		  rec.Open();
		  if ( rec.GetIndex(keyword) >= 0 )
		    {
		      printf("Current Value for %s is %s\n",
			     keyword,
			     rec.GetEntry(keyword));
		      foundKeyword = TRUE;
		    }
		  else
		    {
		      printf("There is no entry for %s\n",keyword);
		    }
		  rec.Close();
		}
	      else
		{
		  quit = TRUE;
		}

	    } while ( !foundKeyword && !quit );

	    if ( !quit )
	      {

		int foundValue = FALSE;
		do {
		  printf("New Value> ");
		  fflush(NULL);
		  fgets(value,255,stdin);

		  //
		  // Strip out newline characters
		  //
		  for ( int i=0; i<(int)strlen(value); i++ )
		    {
		      if ( value[i] == 10 || value[i] == 13 )
			{
			  value[i] = 0;
			}
		    }
	
		  //
		  // Quit if desired.
		  //
		  if ( !strcmp(value,"quit") )
		    {
		      printf("Bye");
		      _OsDeleteFile("temp.rec");
		      return 0;
		    }
	
		  //
		  // If something entered then parse the value
		  //
		  if ( strlen(value) > 0 )
		    {
		      foundValue = TRUE;

		      printf("Connecting to %s . . . ",nexus);
		      fflush(NULL);

		      S_Packet ipacket;
		      ipacket.PushString(keyword);
		      ipacket.PushString(value);

		      S_Client c;
		      c.SetTimeOut(60000000);
		      if ( c.Connect(nexus,SF_NEXUS_PORT,SF_NEXUS_SETPLAYERDATA) ==
			   S_CLIENT_OK)
			{

			  slong rc;
			  S_Packet packet;
			  packet.PushString(player);
			  packet.PushString(password);
			  c.WritePacket(&packet);
			  c.ReadLong(&rc);
			  if ( rc == SF_NEXUS_OK )
			    {
			      c.WritePacket(&ipacket);
			      printf("OK\n");
			      printf("%s = %s\n",keyword,value);
			      printf("\n");
			    }
			  else
			    {
			      printf("Failed\n");
			    }
			  c.Disconnect();
			}

		    }

		} while ( !foundValue );
	      }
	
	  } while ( !quit );
      }

  } while ( !exitProgram );
  
  _OsDeleteFile("temp.rec");
  return 0;
}
