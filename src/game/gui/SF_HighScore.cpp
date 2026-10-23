#include "sf.h"
#include "SF_HighScore.h"
#include "SF_Externals.h"

SF_HighScore::SF_HighScore(char *iname,ulong iscore,ulong ielapsedFrames) {
   name = S_strdup(iname);
   score = iscore;
   seconds = ielapsedFrames/frameRate;
   minutes = seconds/60;
   seconds %= 60;
   }

SF_HighScore::~SF_HighScore() {
   if ( name ) {
      S_free(name);
      name = NULL;
      }
   }

SF_HighScore::SF_HighScore(FILE *fptr) {
   rc = SF_HIGHSCORE_OK;

   short len;
   fread(&len,sizeof(len),1,fptr);
   if ( len >= 0 ) {
      name = (char *)S_malloc(len+1);
      fread(name,len,1,fptr);
      name[len] = 0;
      if ( len != (int)strlen(name) )
         rc = SF_HIGHSCORE_CORRUPT;
      }
   else
      rc = SF_HIGHSCORE_CORRUPT;

   fread(&score,sizeof(ulong),1,fptr);
   fread(&minutes,sizeof(ulong),1,fptr);
   fread(&seconds,sizeof(ulong),1,fptr);
   }


int
SF_HighScore::Save(FILE *fptr) {

   ushort len = strlen(name);
   fwrite(&len,sizeof(len),1,fptr);
   fwrite(name,len,1,fptr);

   fwrite(&score,sizeof(ulong),1,fptr);
   fwrite(&minutes,sizeof(ulong),1,fptr);
   fwrite(&seconds,sizeof(ulong),1,fptr);

   return 0;
   }
