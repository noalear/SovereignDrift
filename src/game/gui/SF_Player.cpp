/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
#include "sf.h"
#include "SF_Player.h"


int
SF_Player::Init()
{
  check = 0xAAAAAAAA;
  shipID = 0;
  kills = 0;
  losses = 0;
  score = 0;
  rank = 0;
  race = 0;
  timeOnline = 0;
  demo = 0;
  credits = 0;
  serial = 0;
  return 0;
}


int
SF_Player::Create(char *iplayerName,
		  char *ipassword,
		  char *ishipName,
		  slong ishipID,
		  slong ikills,
		  slong ilosses,
		  slong iscore,
		  char irank,
		  char irace,
		  slong itimeOnline,
		  char idemo,
		  slong icredits,
		  slong iserial)
{

  if ( check != (int)0xAAAAAAAA ) return 0;
  
  Destroy();
  
  if ( iplayerName )
    {
      playerName.Create(iplayerName);
    }
  if( ipassword )
    {
      password.Create(ipassword);
    }
  if ( ishipName )
    {
      shipName.Create(ishipName);
    }
  
  shipID = ishipID;
  kills = ikills;
  losses = ilosses;
  score = iscore;
  rank = irank;
  race = irace;
  timeOnline = itimeOnline;
  demo = idemo;
  credits = icredits;
  serial = iserial;
  return 0;
}   


SF_Player::SF_Player(FILE *ifptr)
{
  Init();

  Create(ifptr);
}


SF_Player::SF_Player(char *iplayerName,
		     char *ipassword,
		     char *ishipName,
                     slong ishipID,
		     slong ikills,
		     slong ilosses,
                     slong iscore,
		     char irank,
		     char irace,
		     slong itimeOnline,
                     char idemo,
		     slong icredits,
		     slong iserial)
{
  Init();
  Create(iplayerName,ipassword,ishipName,ishipID,ikills,ilosses,iscore,
	 irank,irace,itimeOnline,idemo,icredits,iserial);
}

int
SF_Player::Destroy()
{
  if ( check != (int)0xAAAAAAAA ) return 0;

  playerName.Destroy();
  password.Destroy();
  shipName.Destroy();
  ipAddress.Destroy();

  Init();
  return 0;
}


SF_Player::~SF_Player()
{
  Destroy();
}


int
SF_Player::Create(FILE *fptr)
{

  if ( check != (int)0xAAAAAAAA ) return 0;

  Destroy();

  rc = SF_PLAYER_OK;
  
  short len;
  fread(&len,sizeof(len),1,fptr);
  if ( len >= 0 ) {
    playerName.Create(len);
    fread(playerName.str,len,1,fptr);
    playerName.SetLen(len);
    if ( len != playerName.len )
      rc = SF_PLAYER_CORRUPT;
  }
  else
    rc = SF_PLAYER_CORRUPT;
  
  fread(&len,sizeof(len),1,fptr);
  if ( len >= 0 ) {
    password.Create(len);
    fread(password.str,len,1,fptr);
    password.SetLen(len);
    if ( len != password.len )
      rc = SF_PLAYER_CORRUPT;
  }
  else
    rc = SF_PLAYER_CORRUPT;
  
  fread(&len,sizeof(len),1,fptr);
  if ( len >= 0 ) {
    shipName.Create(len);
    fread(shipName.str,len,1,fptr);
    shipName.SetLen(len);
    if ( len != shipName.len )
      rc = SF_PLAYER_CORRUPT;
  }
  else
    rc = SF_PLAYER_CORRUPT;
  
  fread(&shipID,sizeof(shipID),1,fptr);
  fread(&kills,sizeof(kills),1,fptr);
  fread(&losses,sizeof(losses),1,fptr);
  fread(&timeOnline,sizeof(timeOnline),1,fptr);
  fread(&score,sizeof(score),1,fptr);
  fread(&rank,sizeof(rank),1,fptr);
  fread(&race,sizeof(race),1,fptr);
  fread(&credits,sizeof(credits),1,fptr);

  return rc;  
}


int
SF_Player::Save(FILE *fptr) {

  if ( check != (int)0xAAAAAAAA ) return 0;
  
  ushort len = playerName.len;
  fwrite(&len,sizeof(len),1,fptr);
  fwrite(playerName.str,len,1,fptr);
  
  len = password.len;
  fwrite(&len,sizeof(len),1,fptr);
  fwrite(password.str,len,1,fptr);
  
  len = shipName.len;
  fwrite(&len,sizeof(len),1,fptr);
  fwrite(shipName.str,len,1,fptr);
  
  fwrite(&shipID,sizeof(shipID),1,fptr);
  fwrite(&kills,sizeof(kills),1,fptr);
  fwrite(&losses,sizeof(losses),1,fptr);
  fwrite(&timeOnline,sizeof(timeOnline),1,fptr);
  fwrite(&score,sizeof(score),1,fptr);
  fwrite(&rank,sizeof(rank),1,fptr);
  fwrite(&race,sizeof(race),1,fptr);
  fwrite(&credits,sizeof(credits),1,fptr);
  
  return 0;
}



int
SF_Player::SetPlayerName(char *newPlayerName) {

  if ( check != (int)0xAAAAAAAA ) return 0;
  
  playerName.Create(newPlayerName);

  return 0;
}


int
SF_Player::SetShipName(char *newShipName) {

  if ( check != (int)0xAAAAAAAA ) return 0;

  shipName.Create(newShipName);

  return 0;
}
