#include "S_OS.h"
#include "S_Midi.h"
#include "S_Timer.h"

#if defined(LINUX) || (defined(GNUCPP) && !defined(IBMMMOS2))
int MciSendString(char *c,char *r,uint s,ulong h,ulong i){return 0;}
#endif

S_Midi::S_Midi(char *ifilename,int ivolume) {
   if ( !ifilename )
      return;

   rc = 0;
   volume = ivolume;
   play = FALSE;
   loaded = FALSE;
   filename = NULL;
   if ( ifilename )
      filename = S_strdup(ifilename);
   }

S_Midi::~S_Midi() {
   if ( filename ) {
      S_free(filename);
      filename = NULL;
      }
   }


int
S_Midi::Load() {
   if ( !_OsFileExists(filename) )
      return 0;
   char szCommand[80];
#  ifdef OS2
      sprintf(szCommand,"open %s alias midi1 shareable wait",filename);
#  endif
#  ifdef WIN32
      sprintf(szCommand,"open %s alias midi1 wait",filename);
#  endif
   MciSendString(szCommand,NULL,0,0,0);
   /*MciSendString("set midi1 port mapper wait",NULL,0,0,0);*/
   loaded = TRUE;
   return 0;
   }


int
S_Midi::Play(int ivolume) {
   char szReturn[80];

   if ( ivolume >= 0 )
      volume = ivolume;

   if ( !loaded )
      Load();

   SetVolume( volume );

   play = TRUE;
   MciSendString("play midi1",NULL,0,0,0);

   int position = 0;
   MciSendString("status midi1 length wait",szReturn,80,0,0);
   int length = atoi(szReturn);
   while( position<length && play ) {
      S_Timer timer;
      MciSendString("status midi1 position wait",szReturn,80,0,0);
      position = atoi(szReturn);
      timer.Snooze(100000);
      }

   MciSendString("stop midi1 wait",NULL,0,0,0);
   MciSendString("close midi1 wait",NULL,0,0,0);
   loaded = FALSE;
   return 0;
   }


int
S_Midi::SetVolume(int ivolume) {
   volume = ivolume;
#  ifdef OS2
      char command[80],szReturn[80];
      sprintf(command,"set midi1 volume %d wait",volume);
      MciSendString(command,szReturn,80,NULL,0);
#  endif
   return 0;
   }

int
S_Midi::Stop() {
   play = FALSE;
   return 0;
   }
