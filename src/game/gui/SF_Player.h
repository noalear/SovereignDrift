/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
#ifndef SF_PLAYER_INCLUDED
#define SF_PLAYER_INCLUDED

#define SF_PLAYER_OK       0
#define SF_PLAYER_CORRUPT -1

class SF_Player
{
 public:
  slong check;
  
  /* *** Player Info *** */
  slong shipID;
  slong kills;
  slong losses;
  slong timeOnline;
  slong score;
  slong credits;
  signed char rank;
  signed char race;
  slong serial;
  
  S_String playerName;
  S_String password;
  S_String shipName;
  S_String ipAddress;
  
  char demo;
  
  int rc;
  
  int Init();
  int Destroy();
  SF_Player(char * =NULL,char * =NULL,char * =NULL,
	    slong=0001,slong=0,slong=0,slong=0,char=0,char=0,slong=0,char=0,
	    slong=0,slong=0);
  int Create(char * =NULL,char * =NULL,char * =NULL,
	     slong=0001,slong=0,slong=0,slong=0,char=0,char=0,slong=0,char=0,
	     slong=0,slong=0);
  ~SF_Player();
  
  SF_Player(FILE *);
  int Create(FILE *);
  int Save(FILE *);
  
  int SetShipName(char *);
  int SetPlayerName(char *);
};

#endif
