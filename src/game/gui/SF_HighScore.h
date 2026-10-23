#ifndef SF_HIGHSCORE_INCLUDED
#define SF_HIGHSCORE_INCLUDED

#define SF_HIGHSCORE_OK       0
#define SF_HIGHSCORE_CORRUPT -1

class SF_HighScore {
public:
   int rc;

   char *name;
   ulong score;
   ulong minutes;
   ulong seconds;

   int Save(FILE *);

   SF_HighScore(char *,ulong,ulong);
   SF_HighScore(FILE *);
   ~SF_HighScore();
   };

#endif
