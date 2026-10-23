#include "sf.h"
#include "SF_PortCpp.h"
#include "SF_Externals.h"
#include "SF_Scenario.h"
#include "SF_HighScore.h"
#include "SF_GameEngine.h"

SF_Scenario::SF_Scenario(char *ifilename) {
   filename = S_strdup(ifilename);
   scenarioName = ReadValueFromScenarioFile(filename,"name");
   numHighScores = 0;
   highScores = NULL;
   }

SF_Scenario::~SF_Scenario() {
   if ( scenarioName ) {
      S_free(scenarioName);
      scenarioName = NULL;
      }
   if ( filename ) {
      S_free(filename);
      filename = NULL;
      }
   }

SF_Scenario::SF_Scenario(FILE *fptr) {
   rc = SF_SCENARIO_OK;

   short len;
   fread(&len,sizeof(len),1,fptr);
   if ( len >= 0 ) {
      scenarioName = (char *)S_malloc(len+1);
      fread(scenarioName,len,1,fptr);
      scenarioName[len] = 0;
      if ( len <= 0 || (len > 0 && len != (int)strlen(scenarioName)) )
         rc = SF_SCENARIO_CORRUPT;
      }
   else
      rc = SF_SCENARIO_CORRUPT;

   fread(&len,sizeof(len),1,fptr);
   if ( len >= 0 ) {
      filename = (char *)S_malloc(len+1);
      fread(filename,len,1,fptr);
      filename[len] = 0;
      if ( len != (int)strlen(filename) )
         rc = SF_SCENARIO_CORRUPT;
      }
   else
      rc = SF_SCENARIO_CORRUPT;

   numHighScores = 0;
   highScores = NULL;

   fread(&len,sizeof(len),1,fptr);
   for ( int i=0; i<len; i++ ) {
      SF_HighScore *newHighScore = new SF_HighScore(fptr);
      AddHighScore(newHighScore);
      }

   }


int 
SF_Scenario::AddHighScore(SF_HighScore *highScore) {
   if ( highScores == NULL )
      highScores = (SF_HighScore **) S_malloc( sizeof(highScore) );
   else
      highScores = (SF_HighScore **) S_realloc( highScores,
                                     sizeof(highScore)*(numHighScores+1) );

   highScores[numHighScores] = highScore;
   numHighScores++;
   return 0;
   }


int
SF_Scenario::SortHighScores() {
   if ( !highScores )
      return 0;
   int done = FALSE;
   do {
      done = TRUE;
      for ( int i=0; i<numHighScores-1; i++ ) {

         int iFinished = highScores[i]->minutes != 0 ||
                         highScores[i]->seconds != 0;
         int i1Finished = highScores[i+1]->minutes != 0 ||
                          highScores[i+1]->seconds != 0;
         int swap = (i1Finished && !iFinished) ||
                    (iFinished && i1Finished && 
                     highScores[i]->score < highScores[i+1]->score ) ||
                    (!iFinished && !i1Finished && 
                     highScores[i]->score < highScores[i+1]->score );

         if ( swap ) {
            SF_HighScore *tmp = highScores[i];
            highScores[i] = highScores[i+1];
            highScores[i+1] = tmp;
            done = FALSE;
	    }
         }
      } while ( !done );
   return 0;
   }


int
SF_Scenario::CropHighScores(int num) {
   if ( !highScores || numHighScores <= num )
      return 0;
   for ( int i=num; i<numHighScores; i++ )
      delete highScores[i];
   numHighScores = num;
   return 0;
   }


int
SF_Scenario::Save(FILE *fptr) {

   ushort len = strlen(scenarioName);
   fwrite(&len,sizeof(len),1,fptr);
   fwrite(scenarioName,len,1,fptr);

   len = strlen(filename);
   fwrite(&len,sizeof(len),1,fptr);
   fwrite(filename,len,1,fptr);

   len = numHighScores;
   fwrite(&len,sizeof(len),1,fptr);
   for ( int i=0; i<len; i++ )
      highScores[i]->Save(fptr);

   return 0;
   }
