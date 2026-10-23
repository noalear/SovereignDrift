#define S_LIB_CONSOLE

#include "sf.h"


char *mon[12] = {
  "Jan","Feb","Mar",
  "Apr","May","Jun",
  "Jul","Aug","Sep",
  "Oct","Nov","Dec" };

int
main(int argc, char *argv[])
{
  
  /* ************************ */
  /* *** Parse the a file *** */
  /* ************************ */

  if ( argc < 2 )
    {
      printf("\n");
      printf("Usage: log [logfile]\n");
      printf("Example: log server.server-log\n");
      printf("\n");
      return 0;
    }
  
  char *logFilename = argv[1];
  printf("Parsing log file %s\n",logFilename);

  S_String scenarioFilename(logFilename);
  if ( !scenarioFilename.str )
    {
      printf("Failed to create scenario filename\n");
      return 0;
    }

  scenarioFilename.ReplaceCharacter('.',(char)'\0');
  printf("Scenario name is %s\n",scenarioFilename.str);

  S_String chatLogFilename("%s.chat-log",scenarioFilename.str);
  if ( !chatLogFilename.str )
    {
      printf("Failed to create chat log filename\n");
      return 0;
    }
  printf("Chat log filename name is %s\n",chatLogFilename.str);

  S_String popLogFilename("%s.pop-log",scenarioFilename.str);
  if ( !popLogFilename.str )
    {
      printf("Failed to create pop filename\n");
      return 0;
    }
  printf("Pop log filename name is %s\n",popLogFilename.str);

  FILE *fptr = fopen(logFilename,"r");
  if ( !fptr )
    {
      printf("Failed to open %s\n",logFilename);
      return 0;
    }

  FILE *fptrChat = fopen(chatLogFilename.str,"w");
  if ( !fptrChat )
    {
      printf("Failed to open %s\n",chatLogFilename.str);
      fclose(fptr);
      return 0;
    }

  FILE *fptrPop = fopen(popLogFilename.str,"w");
  if ( !fptrPop )
    {
      printf("Failed to open %s\n",popLogFilename.str);
      fclose(fptrChat);
      fclose(fptr);
      return 0;
    }

  char line[1024];
  while ( fgets(line,1023,fptr) )
    {

      int isChatLine = line[24] == ':' && line[25] == ' ';
      if ( isChatLine )
	{
	  fprintf(fptrChat,"%s",line);
	}

      char *pl = strstr(line," pl]");
      int isPopLine = isChatLine && pl != NULL;
      if ( isPopLine )
	{

	  //
	  // Find the month
	  //
	  int month = 0;
	  for ( int i=0; i<12; i++ )
	    {
	      if ( line[4] == mon[i][0] && 
		   line[5] == mon[i][1] && 
		   line[6] == mon[i][2] )
		{
		  month = i+1;
		}
	    }

          line[24] = '\0';
	  int year = atol(line+22);
	  line[24] = ':';

	  line[10] = 0;
	  int day = atol(line+8);
	  line[10] = ' ';

	  fprintf(fptrPop,"%d/%d/%02d ",month,day,year);

          line[19] = '\0';
	  fprintf(fptrPop,"%s\t",line+11);
	  line[19] = ' ';

	  *pl = 0;
	  while ( *pl != '/' )
	    {
	      pl--;
	    }
	  fprintf(fptrPop,"%s\n",pl+1);
	}

    }
  
  fclose(fptr);
  fclose(fptrChat);
  fclose(fptrPop);

  return 0;
}

