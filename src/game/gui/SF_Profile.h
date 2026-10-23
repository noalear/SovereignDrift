#ifndef SF_PROFILE_INCLUDED
#define SF_PROFILE_INCLUDED

#define SF_SOUND_DEFAULTVOLUME 75
#define SF_MUSIC_DEFAULTVOLUME 75

#ifdef SF_EXPERIMENTAL
#  define SF_NEXUS_DEFAULT    "sf-x.stardock.net"
#  define SF_PROFILE_FILENAME "sf-x.prf"
#else
#  define SF_NEXUS_DEFAULT    "sf.stardock.net"
#  define SF_PROFILE_FILENAME "sf.prf"
#endif

#define SF_PROFILE_OK                0
#define SF_PROFILE_CORRUPT          -1
#define SF_PROFILE_FILEDOESNOTEXIST -2
#define SF_PROFILE_INVALIDREGCODE   -3
#define SF_PROFILE_OLDREGCODE       -4

class SF_Player;
class SF_Scenario;
class SF_Engine;

class SF_Profile {
public:

  int experimental;

   /* ******************************* */
   /* *** Sound/Music Dialog data *** */
   /* ******************************* */
   uchar sound;
   uchar soundVolume;
   uchar music;
   uchar musicVolume;
   uchar tcpipOnly;

   /* ******************************* */
   /* *** MultiPlayer Dialog Data *** */
   /* ******************************* */
   char *nexusName;
   char *serverName;

   /* ************************* */
   /* *** Login Dialog data *** */
   /* ************************* */
   signed char selectedPlayer;
   signed char numStoredPlayers;
   SF_Player **players;

   /* ******************************** */
   /* *** Scenario High Score Data *** */
   /* ******************************** */
   signed char numStoredScenarios;
   SF_Scenario **scenarios;

   /* *********************** */
   /* *** Additional Data *** */
   /* *********************** */
   char *reg1;
   char *reg2;

   /* *************** */
   /* *** Methods *** */
   /* *************** */
   SF_Profile();
   ~SF_Profile();

   int SetRegCode(char * = NULL,char * = NULL);
   int SetNexusName(char *);
   int SetServerName(char *);

   int Exists();
   int Save();
   int Load();

   int AddPlayer(SF_Player *);
   int AddScenario(SF_Scenario *);
   SF_Scenario *GetScenario(char *);
   int RemovePlayer(int);

   int Update(SF_Engine *e);
   };

#endif
