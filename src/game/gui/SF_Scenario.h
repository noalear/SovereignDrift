#ifndef SF_SCENARIO_INCLUDED
#define SF_SCENARIO_INCLUDED

#define SF_SCENARIO_OK       0
#define SF_SCENARIO_CORRUPT -1

class SF_HighScore;

class SF_Scenario {
public:
   int rc;

   char *scenarioName;
   char *filename;

   int numHighScores;
   SF_HighScore **highScores;
   int AddHighScore(SF_HighScore *);

   SF_Scenario(char * =NULL);
   ~SF_Scenario();

   SF_Scenario(FILE *);
   int Save(FILE *);

   int SortHighScores();
   int CropHighScores(int);

   };

#endif
