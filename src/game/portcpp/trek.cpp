/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
#include <S_Lib.h>

#ifdef OS2
#  include <iomanip.h>
#else
#  include <iomanip>
#endif

#if defined(OS2) || defined(WIN32)
#  include "../gui/sf.h"
#  include "../gui/SF_Player.h"
#  include "../gui/SF_Msg.h"
#  include "../gui/SF_MsgBuffer.h"
#  include "../gui/SF_VideoConsole.h"
#  include "../gui/SF_ChatConsole.h"
#  include "../gui/SF_Globals.h"
#  include "../gui/SF_Engine.h"
#  include "../gui/SF_ClientWindow.h"
#  include "../gui/SF_Client.h"
#  include "../gui/SF_DisplayEngine.h"
#  include "../gui/SF_Engine.h"
#  include "../gui/SF_Button.h"
#  include "../gui/SF_WaveList.h"
#endif
#ifdef UNIX
#  include "../gui/sf.h"
#  include "../gui/SF_Player.h"
#  include "../gui/SF_Msg.h"
#  include "../gui/SF_MsgBuffer.h"
#  include "../gui/SF_VideoConsole.h"
#  include "../gui/SF_ChatConsole.h"
#  include "../gui/SF_Globals.h"
#  include "../gui/SF_Engine.h"
#  include "../gui/SF_ClientWindow.h"
#  include "../gui/SF_Client.h"
#  include "../gui/SF_DisplayEngine.h"
#  include "../gui/SF_Engine.h"
#  include "../gui/SF_Button.h"
#  include "../gui/SF_WaveList.h"
#endif

extern SF_Client *sf;

#include "classes.h"
#include "inc_portcpp.h"
#include "enginepp.h"

/* ************************************ */
/* ***** Externals from Doug's code *** */
/* ************************************ */
extern char frameRate;
extern char aishipUpdateRate;
extern slong frameNumber;
extern short localSlotNumber;
/* ************************************ */

/* dlh */
extern SF_MsgBuffer retMsgBuffer;
/* dlh */

extern short warningMessagePosted;
extern slong warningMessagePostedFrame;
extern short warningBottomMessagePosted;
extern slong warningBottomMessagePostedFrame;

extern short numPlanetRobotsSet;
extern short numPopRobotsSet;
extern slong numSpawnMessagesSent;

extern int inServer;
extern char instanceName[];

extern char message[MESSAGE_BUFFER_SIZE][MESSAGE_STRING_LENGTH+1];
extern char messagePushed;
void SetScreenMessage( char *, int );

ulong frameAtLastVictory = 0;

/* external objects */
extern GameStateClass*     game;

/* global functions */

/*
#define CALL_PUSH_GAME_OBJ(gb,gs,obj) {long n = gb.Push(obj,gs);WRITE_SF_LOG(logFile << "pushing " #obj << " (" << n << " bytes)" << endl);}
#define CALL_POP_GAME_OBJ(gb,gs,obj) {long n = gb.Pop(obj,gs);WRITE_SF_LOG(logFile << "popping " #obj << " (" << n << " bytes)" << endl);}
#define CALL_PUSH_OBJ(gb,obj) {long n = gb.Push(obj);WRITE_SF_LOG(logFile << "pushing " #obj <<  " (" << n << " bytes)" << endl);}
#define CALL_POP_OBJ(gb,obj) {long n = gb.Pop(obj);WRITE_SF_LOG(logFile << "popping " #obj << " (" << n << " bytes)" << endl);}
*/

/**** local inline functions ****/

char* Format(slong);
/********************************/

extern void Parse(char*, SFString*, SFString*, short, short =0);
extern void Parse(SFString&, SFString*, SFString*, short, short =0);

ObjectBitmapClass NoObjectBitmap(ObjectBitmapClass::stationary,-1,1,1);


int
IsValidObjectPointer(GenericClass *g,char *file, int line, short logNull)
{

  int isValid = FALSE;

  //
  // NULL pointer may be bad, but no further object access should occur, so return
  //
  if ( !g )
    {

      if (logNull)
	logFile << "ERROR> null pointer found at line " << line << " in " << file << " on frame " <<
	  game->frame << endl;
      
      return isValid;
    }


  //
  // Check to see if pointer value is between known bounds of already
  // allocated objects.
  //
  isValid = ( g >= game->ship.rootOf() && 
              g <  game->ship.rootOf() + game->numShips ) ||
	    ( g >= game->planet.rootOf() && 
              g < game->planet.rootOf() + game->maxPlanets ) ||
	    ( g >= game->colony.rootOf() && 
              g < game->colony.rootOf() + game->maxColonies ) ||
	    ( g >= game->missile.rootOf() && 
              g < game->missile.rootOf() + game->maxMissiles );

  //
  // Sometimes a pointer can be so bad that even checking a member var
  // can crash the program, so don't access g->check unless pointer is 
  // obviously an OK value.  OK value being a value that is between bounds 
  // of known object array.
  //
  if ( isValid )
    {
      isValid = g->check == (int)0xAAAAAAAA;
    }

  if ( !isValid && file && line && logNull )
    {
      logFile << "ERROR> Invalid GenericClass pointer " << file << 
                 " line " << line << endl;
    }

  return isValid;
}


int
IsValidDesignStructPointer(DesignStruct *d,char *file,int line,short logNull)
{
  int isValid = FALSE;
  if ( !d )
    {
      isValid = FALSE;
      if ( logNull )
	{
          logFile << "ERROR> NULL DesignStruct " << file << 
                     " line " << line << endl;
	}
      return isValid;
    }
  isValid = ( d >= game->shipDesign.rootOf() && 
              d <  game->shipDesign.rootOf() + game->numShipDesigns ) ||
	    ( d >= game->colonyDesign && 
              d < game->colonyDesign + game->numColonyDesigns ) ||
	    ( d >= game->missileDesign.rootOf() && 
              d < game->missileDesign.rootOf() + game->numMissileDesigns );
  if ( isValid )
    {
      isValid = d->check == (int)0xAAAAAAAA;
    }
  if ( !isValid )
    {
      logFile << "ERROR> Invalid DesignStruct " << file << 
                 " line " << line << endl;
    }
  return isValid;
}


void ModuleClass::Fire() {
  if ( check != 0xAAAAAAAA ) return;
#if defined(SF_DIAGNOSTIC_MODE)
  if (HasValidFireFunction())
    (this->*fire)();
  else if ( IsValidObjectPointer(host.generic,__FILE__,__LINE__) ) {
    logFile << "a ";
    host.generic->ShortProfile();
    logFile << " had a bad fire function (" << *((slong*)(&fire)) <<
      ") in module " << this - host.designed->bus.moduleList <<
      " (" << name << ") on frame " << game->frame;
  }
#else
  (this->*fire)();
#endif
}

slong ModuleClass::Storage() {
  if ( check != 0xAAAAAAAA ) return 0;
#if defined(SF_DIAGNOSTIC_MODE)
  if (HasValidStorageFunction())
    return (this->*storage)();
  else {
    logFile << "a ";
    host.designed->ShortProfile();
    logFile << " had a bad storage function (" << *((slong*)(&storage)) <<
      ") in module " << this - host.designed->bus.moduleList <<
      " (" << name << ") on frame " << game->frame;
    return 0;
  }
#else
  return (this->*storage)();
#endif
}

slong ModuleClass::Requirement() {
  if ( check != 0xAAAAAAAA ) return 0;
#if defined(SF_DIAGNOSTIC_MODE)
  if (HasValidRequirementFunction())
    return (this->*requirement)();
  else {
    logFile << "a ";
    host.designed->ShortProfile();
    logFile << " had a bad requirement function (" << *((slong*)(&requirement)) <<
      ") in module " << this - host.designed->bus.moduleList <<
      " (" << name << ") on frame " << game->frame;
    return 0;
  }
#else
  return (this->*requirement)();
#endif
}

slong ModuleClass::Withdraw() {
  if ( check != 0xAAAAAAAA ) return 0;
#if defined(SF_DIAGNOSTIC_MODE)
  if (HasValidWithdrawFunction())
    return (this->*withdraw)();
  else {
    logFile << "a ";
    host.designed->ShortProfile();
    logFile << " had a bad withdraw function (" << *((slong*)(&withdraw)) <<
      ") in module " << this - host.designed->bus.moduleList <<
      " (" << name << ") on frame " << game->frame;
    return 0;
  }
#else
  return (this->*withdraw)();
#endif
}

slong ModuleClass::Deposit(slong& xs) {
  if ( check != 0xAAAAAAAA ) return 0;
#if defined(SF_DIAGNOSTIC_MODE)
  if (HasValidDepositFunction())
    return (this->*deposit)(xs);
  else {
    logFile << "a ";
    host.designed->ShortProfile();
    logFile << " had a bad deposit function (" << *((slong*)(&deposit)) <<
      ") in module " << this - host.designed->bus.moduleList <<
      " (" << name << ") on frame " << game->frame;
    return 0;
  }
#else
  return (this->*deposit)(xs);
#endif
}

slong ModuleClass::XStorage() {
  if ( check != 0xAAAAAAAA ) return 0;
  return Storage();
}

short DesignedClass::ModuleStore(short j) {
  /* if (bus.moduleList[j].moduleFunc != ModuleClass::tube)*/
  return (short)(bus.moduleList[j].Storage());
  /* else
     return bus.moduleList[j].workPar[1] * module_function_scale /
     max(bus.moduleList[j].par[2],1);*/
}

ObjectBitmapClass::ObjectBitmapClass(SequenceType st, short i,
                                     short n, short s) {
  /* bitmap class: primary constructor */

  seq       = st;
  index     = i;
  numFrames = n;
  size      = s;
  acOnly    = FALSE;

  init();

}


ObjectBitmapClass::~ObjectBitmapClass()
{
}


void
ObjectBitmapClass::init()
{
  const double pi = 3.14159265;
  for ( int i=0; i <numFrames; i++ )
    {
      dir[i].initializeWith(nlong(-sin(i*2*pi/numFrames) * sub_scale),
			    nlong( cos(i*2*pi/numFrames) * sub_scale));
    }
}


slong ObjectBitmapClass::PushTo(class GameStateBuffer& gb, GameStateClass* g) {

  char* bufferPos = gb.BufferPosition();

  CALL_PUSH_OBJ(gb,acOnly);
  CALL_PUSH_OBJ(gb,index);
  CALL_PUSH_OBJ(gb,numFrames);
  CALL_PUSH_OBJ(gb,size);
  CALL_PUSH_OBJ(gb,seq);
  CALL_PUSH_OBJ(gb,alpha);
  CALL_PUSH_OBJ(gb,gamma);

  return gb.BufferPosition() - bufferPos;

}

slong GameBitmapClass::PushTo(class GameStateBuffer& gb, GameStateClass* g) {

  //long subClassSize = ObjectBitmapClass::PushTo(gb,g);
  ObjectBitmapClass::PushTo(gb,g);

  char* bufferPos = gb.BufferPosition();

  CALL_PUSH_OBJ(gb,name);
  CALL_PUSH_OBJ(gb,Class);
  CALL_PUSH_OBJ(gb,file);
  CALL_PUSH_OBJ(gb,raceName);

  return gb.BufferPosition() - bufferPos;

}

slong ObjectBitmapClass::PopFrom(class GameStateBuffer& gb, GameStateClass* g) {

  char* bufferPos = gb.BufferPosition();

  CALL_POP_OBJ(gb,acOnly);
  CALL_POP_OBJ(gb,index);
  CALL_POP_OBJ(gb,numFrames);
  CALL_POP_OBJ(gb,size);
  CALL_POP_OBJ(gb,seq);
  CALL_POP_OBJ(gb,alpha);
  CALL_POP_OBJ(gb,gamma);

  const double pi = 3.14159265;

  short i;
  for (i = 0; i < numFrames; i++)
    {
      dir[i].initializeWith(nlong(-sin(i*2*pi/numFrames) * sub_scale),
			    nlong( cos(i*2*pi/numFrames) * sub_scale));
    }

  return gb.BufferPosition() - bufferPos;
}

slong GameBitmapClass::PopFrom(class GameStateBuffer& gb, GameStateClass* g) {

  //long subClassSize = ObjectBitmapClass::PopFrom(gb,g);
  ObjectBitmapClass::PopFrom(gb,g);

  char* bufferPos = gb.BufferPosition();

  CALL_POP_OBJ(gb,name);
  CALL_POP_OBJ(gb,Class);
  CALL_POP_OBJ(gb,file);
  CALL_POP_OBJ(gb,raceName);

  return gb.BufferPosition() - bufferPos;

}

GameStateClass::GameStateClass() {
  /* Game State Class: default constructor */

  tech = NULL;
  colonyDesign = NULL;

  ClearCurrentState();

}

GameStateClass::GameStateClass(short ns, short np, short nm, short nb,
                               short nsd, short nmd, short nmo, short nr,
                               short nsp, short nrk, short nco, short ncd,
                               short mp, short mm, short mb, short msd,
                               short mmd, short mmo, short mr, short msp,
			       short nec, short nso) {

  tech = NULL;
  colonyDesign = NULL;

  ClearCurrentState();

  InitializeWith(ns, np, nm, nb,
		 nsd, nmd, nmo, nr,
		 nsp, nrk, nco, ncd,
		 mp, mm, mb, msd,
		 mmd, mmo, mr, msp,
		 nec, nso);
}

void GameStateClass::InitializeWith(short ns, short np, short nm, short nb,
				    short nsd, short nmd, short nmo, short nr,
				    short nsp, short nrk, short nco, short ncd,
				    short mp, short mm, short mb, short msd,
				    short mmd, short mmo, short mr, short msp,
				    short nec, short nso) {
  /* Game State Class: dynamic constructor */

  /* clear all settings from the current game state */
  /* before building new structure                  */
  DestroyCurrentState();

  numShips = ns;
  numPlanets = np;
  numMissiles = nm;
  numBitmaps = nb;
  numActiveShips = 0;
  numMissilesDestroyed = 0;
  numShipDesigns = nsd;
  numMissileDesigns = nmd;
  numModules = nmo;
  numRaces = nr;
  numStoredPlayers = nsp;
  numRanks = nrk;
  numColonies = nco;
  numColoniesDestroyed = 0;
  numColoniesCreated = 0;
  numColonyDesigns = ncd;
  numSounds = nso;
  maxPlanets = mp;
  maxMissiles = mm;
  maxBitmaps = mb;
  maxShipDesigns = msd;
  maxMissileDesigns = mmd;
  maxModules = mmo;
  maxRaces = mr;
  maxStoredPlayers = msp;
  extraColonies = nec;
  maxColonies = mp + nec;
  
  short i;

  /* allocate space for game elements */
  if (ns)
    ship.initializeWith(ns);
  else
    ship.initialize();

  if (mp)
    planet.initializeWith(mp);
  else
    planet.initialize();

  if (mm)
    missile.initializeWith(mm);
  else
    missile.initialize();

  if (mm)
    missileList.initializeWith(mm);
  else
    missileList.initialize();
  for (i=0; i<maxMissiles; i++)
    missileList[i] = 0;

  if (msd)
    shipDesign.initializeWith(msd);
  else
    shipDesign.initialize();

  if (mmd)
    missileDesign.initializeWith(mmd);
  else
    missileDesign.initialize();

  if (mb)
    objectBitmapList.initializeWith(mb);
  else
    objectBitmapList.initialize();

  if (mmo)
    moduleList.initializeWith(mmo);
  else
    moduleList.initialize();

  if (mr)
    //race = new RaceClass[mr];
    race.initializeWith(mr);
  else
    race.initialize();

  if (msp)
    player.initializeWith(msp);
  else
    player.initialize();

  if (ns)
    playerList.initializeWith(ns); /* one for each ship */
  else
    playerList.initialize();

  if (nrk)
    rank.initializeWith(nrk);
  else
    rank.initialize();

  if (maxColonies)
    colony.initializeWith(maxColonies);
  else
    colony.initialize();

  if (ncd)
    colonyDesign = new ColonyDesignStruct[ncd];
  else
    colonyDesign = 0;

  if (maxColonies)
    colonyList.initializeWith(maxColonies);
  else
    colonyList.initialize();

  for (i=0; i<maxColonies; i++)
    colonyList[i] = 0;

  if (numSounds)
    sound.initializeWith(numSounds);
  else
    sound.initialize();

  if (mr)
    tech = new ListStruct[mr];
  else
    tech = 0;
  for (i=0; i<mr; i++)
    tech[i].InitializeWith(0);

  loadedModuleList.initialize();

  maxAcc = max_acc;
  maxAngles = max_angles;
  lightSpeed = light_speed / frameRate;
  planetShipGFactor = planet_ship_g_factor;
  planetPlanetGFactor = planet_planet_g_factor;
  damageToMass = dam_to_mass;
  defectionRate = defection_rate * frameRate;
  loyaltyCycle = loyalty_cycle * frameRate;
  offensiveFactor = offensive_factor;
  salvageRightPct = salvage_right_pct;
  bountyRightPct = bounty_right_pct;
  surrenderAllowed = 1;
  landRightFactor = land_right_factor;
  AILevel = medium_AI_level;
  numPopRobots = 0;
  numPlanetRobots = 0;
  popPercentPerRobot = 0;
  planetsPerRobot = 0;
  randomSeed = 0;
  debrisCount = 4; /* four debris bitmaps per race */
  colonyPending = 0;
  pluginAction = swap; /* default action */
  pluginPar = 0;
  balanceMaxGap = 2;
  bonusFactor = 1;
  maxColonyMillions = max_colony_millions;

  totPop = 0;

  frame = 0;

  mode = laserDefense + betweenFrames;  

  objectList.initialize();

  MapObjectList();
}


short GameStateClass::N(CountType c) {
  /* Game State Class: primary counting facility -- move to inline */
  switch (c) {
  case objects:
    return(numShips+numPlanets+numMissiles+maxColonies); /* renum */
  case ships:
    return(numShips);
  case first_ship:
    return shipObjectIndexStart;
  case last_ship:
    return shipObjectIndexEnd;
  case planets:
    return(numPlanets);
  case first_planet:
    return planetObjectIndexStart;
  case last_planet:
    return planetObjectIndexEnd;
  case first_colony:
    return colonyObjectIndexStart;
  case last_colony:
    return colonyObjectIndexEnd;
  case statics:
    return(maxPlanets+numShips);
  case missiles:
    return(numMissiles);
  case bitmaps:
    return(numBitmaps);
  case ship_designs:
    return(numShipDesigns);
  case missile_designs:
    return(numMissileDesigns);
  case races:
    return numRaces;
  case stored_players:
    return numStoredPlayers;
  case robot_players:
    return numRobotPlayers;
  case human_ships:
    return maxHumanShips;
  case colonies:
    return numColonies;
  case max_colonies:
    return maxColonies;
  case modules:
    return numModules;
  default:
    return(0);
  }
}

void GameStateClass::ClearCurrentState() {

  /* use static limits in lieu of dynamic limits */
  numShips = 0;
  numPlanets = 0;
  numMissiles = 0;
  numActiveShips = 0;
  numMissilesDestroyed = 0;
  numBitmaps = 0;
  numShipDesigns = 0;
  numMissileDesigns = 0;
  numRanks = 0;
  numColonies = 0;
  numColonyDesigns = 0;
  numSounds = 0;
  maxColonies = 0;
  extraColonies = 0;
  maxMissiles = 0;

  maxAcc = max_acc;
  lightSpeed = light_speed;
  planetShipGFactor = planet_ship_g_factor;
  planetPlanetGFactor = planet_planet_g_factor;
  damageToMass = dam_to_mass;
  numPopRobots = 0;
  numPlanetRobots = 0;
  popPercentPerRobot = 0;
  planetsPerRobot = 0;
  randomSeed = 0;
  debrisCount = 4; /* four debris bitmaps per race */
  colonyPending = 0;
  pluginAction = swap; /* default action */
  pluginPar = 0;
  surrenderAllowed = 1;
  numLoadedModules = 0;

  mode = laserDefense + betweenFrames;

  frame = 0;

  totPop = 0;

  ship.initialize();
  planet.initialize();
  missile.initialize();
  colony.initialize();
  shipDesign.initialize();
  missileDesign.initialize();

  if (colonyDesign) {
    delete[] colonyDesign;
    colonyDesign = 0;
  }

  objectList.initialize();
  missileList.initialize();
  colonyList.initialize();
  objectBitmapList.initialize();
  player.initialize();
  playerList.initialize();
  rank.initialize();
  race.initialize();
  sound.initialize();
  loadedModuleList.initialize();
  moduleList.initialize();
  if (tech)
    delete[] tech;
  tech = 0;

  short i;
  for (i=0; i < last_event; i++)
    soundEvent[i] = -1;

}

void GameStateClass::DestroyCurrentState() {
  short i;
  if (ship.IsNotNull())
    for (i=0; i<numShips; i++)
      if (ship[i].bus.moduleList.IsNotNull())
	ship[i].bus.FreeModuleList();

  if (missile.IsNotNull())
    for (i=0; i<maxMissiles; i++)
      if (missile[i].bus.moduleList.IsNotNull())
	missile[i].bus.FreeModuleList();

  if (colony.IsNotNull())
    for (i=0; i<maxColonies; i++)
      if (colony[i].bus.moduleList.IsNotNull())
	colony[i].bus.FreeModuleList();
  
  planet.destroy();
  ship.destroy();
  shipDesign.destroy();

  if (colony.IsNotNull()) {
    colony.destroy();
  }
  if (colonyDesign) {
    delete[] colonyDesign;
    colonyDesign = 0;
  }

  colonyList.destroy();

  missile.destroy();

  missileDesign.destroy();

  missileList.destroy();

  player.destroy();

  playerList.destroy();

  rank.destroy();

  objectList.destroy();
  
  objectBitmapList.destroy();

  loadedModuleList.destroy();

  moduleList.destroy();

  sound.destroy();

  /* for (i=0; i<numRaces; i++)
     if (tech[i].list) {
     delete (tech+i);
     } */

  if (tech) {
    delete[] tech;
    tech = 0;
  }

  ClearCurrentState();

}

void GameStateClass::DestroyObject(MissileClass* m) {
  /* remove object from count -- object will be removed from list at
     start of frame -- see Update() . */
  m->bus.FreeModuleList();
  m->deactivate();
  numMissilesDestroyed++;
}

void GameStateClass::SendDestroyObjectMsg(ColonyClass* c) {
  /* remove object from count -- object will be removed from list at  */
  /* start of frame -- see Update().  This message should only be     */
  /* sent by the server or a disconnected client.                     */
  
  if(!c->IsPending(ShipClass::disappearing) &&
     (IsServer() || IsDisconnectedClient())) {

    logFile << "sent> a request to remove a ";
    c->ShortProfile();
    logFile << " has been sent on frame " << frame << endl;

    c->SetPendingFlag(ShipClass::disappearing);
    retMsgBuffer.PushMsg((uchar)SF_MSG_REMOVECOLONY,
			 (ushort)ObjectListIndex(c),
			 (int)S_CLIENT_TCP);

  }
    
}

void GameStateClass::ReceiveDestroyObjectMsg(ColonyClass* c, short log) {
  /* remove object from count -- object will be removed from list at  */
  /* start of frame -- see Update().  This message should only be     */
  /* sent by the server or a disconnected client.                     */

  if (log) {
    logFile << "received> a request to remove a ";
    c->ShortProfile();
    logFile << " has been received on frame " << frame << endl;
  }

  c->ClearPendingFlag(ShipClass::disappearing);

  /* if docked, then detach */
  if ( IsValidObjectPointer(c->dockObj,__FILE__,__LINE__) )
    {
      c->dockObj->ProcessFreeDockMsg(c->portNum);
    }

  /* if this object was building something destroy it */
  if (c->bus.factoryMod && c->bus.factoryMod->workPar[0]) {
    ObjectUnion buildObj;
    buildObj.objLong = c->bus.factoryMod->workPar[0];
    buildObj.generic->Explode("the building object was destroyed");
  }

  c->bus.FreeModuleList();
  c->deactivate();

  short i;
  for (i = 0; i < numPlanets; i++)
    if (planet[i].colony == c) {
      planet[i].colony = 0;
      planet[i].raceID = -1; /* neutral */
    }
  
  numColoniesDestroyed++;
  
  logFile << "note> after destroying requested slot " << colony.indexOf(c) <<
    " on frame " << frame <<
    ", numColonies = " << numColonies << " numColoniesCreated = " << numColoniesCreated <<
    " numColoniesDestroyed = " << numColoniesDestroyed << endl;
  
  CleanOrphanObjects(c); /* replaces code below */
}

short GameStateClass::IndexOfBitmap(const char* nameStr, const short objType, const slong raceID) const {

  /* set the starting point in the bitmap list for the search */
  short iStart = 0, iStop = numBitmaps;
  switch (objType) {
  case ::explosion:
    iStart = explosionBitmapIndexStart;
    iStop = explosionBitmapIndexEnd;
    break;
  case ::planet:
    iStart = planetBitmapIndexStart;
    iStop = planetBitmapIndexEnd;
    break;
  case ::missile:
    iStart = missileBitmapIndexStart;
    iStop = missileBitmapIndexEnd;
    break;
  case ::ship:
    iStart = shipBitmapIndexStart;
    iStop = shipBitmapIndexEnd;
    break;
  case ::colony:
    iStart = colonyBitmapIndexStart;
    iStop = colonyBitmapIndexEnd;
    break;
  }

  /* first, try to match an integer */
  short i;
  i = short(atol(nameStr));
  if (i || !strcmp(nameStr, "0"))
    return short(iStart + i - 1);

  /* second, try to find a bitmap with the same name */

  i = iStart;
  while (i <= iStop && (strcmp(objectBitmapList[i].name, nameStr) ||
	 (raceID != -1 &&
	  strcmp(race[raceID].name, objectBitmapList[i].raceName) &&
	  strcmp(objectBitmapList[i].raceName, "all"))))
    i++;
  
  if (i <= iStop)
    return i;

  i = iStart;
  while (i <= iStop && strcmp(objectBitmapList[i].name, nameStr))
    i++;
  
  if (i <= iStop) {
    logFile << "WARNING> match for bitmap identifier " << nameStr << 
      " found outside of specified race." << endl;
    return i;
  }
  
  i=0;
  while (i < numBitmaps && (strcmp(objectBitmapList[i].name, nameStr) ||
	 (raceID != -1 &&
	  strcmp(race[raceID].name, objectBitmapList[i].Class) &&
	  strcmp(objectBitmapList[i].raceName, "all"))))
    i++;
  
  if (i < numBitmaps) {
    
    logFile << "WARNING> match for bitmap identifier " << nameStr << 
      " found outside of type." << endl;
    
    return i;
  }

  logFile << "ERROR> unable to map bitmap identifier: " << nameStr << endl;
  
  /* set it to default to the first bitmap of the same type */
  return short(iStart);
  
}

short GameStateClass::IndexOfSound(const char* nameStr, const short objectType, const slong raceID) const {

  /* first, if the name is "none" then return -1 */
  if (!strcmp(nameStr, "none"))
    return -1;

  /* second, try to find a sound with the same name, class and race */

  short i = 0;
  while (i < numSounds &&
	 (strcmp(sound[i].name, nameStr) ||
	  (objectType != all_type &&
	   objectType != sound[i].objectType &&
	   sound[i].objectType != all_type) ||
	  (raceID != -1 &&
	   raceID != sound[i].raceID &&
	   sound[i].raceID != -1)))
    i++;
  
  if (i < numSounds)
    return i;

  i = 0;
  while (i < numSounds &&
	 (strcmp(sound[i].name, nameStr) ||
	  (objectType != all_type &&
	   objectType != sound[i].objectType &&
	  sound[i].objectType != -1)))
    i++;
  
  if (i < numSounds) {
    logFile << "WARNING> match for sound identifier " << objectTypeName[objectType] <<
      ":" << nameStr << "[";
    if (raceID != -1)
      logFile << race[raceID].name;
    else 
      logFile << "all";
    
    logFile << "] found outside of specified race." << endl;
    
    return i;
  }

  i = 0;
  while (i < numSounds && strcmp(sound[i].name, nameStr))
    i++;
  
  if (i < numSounds) {
    logFile << "WARNING> match for sound identifier " << objectTypeName[objectType] <<
      ":" << nameStr << " found in " << objectTypeName[sound[i].objectType] << ", outside of specified type." << endl;
    return i;
  }

  /* if no match was found then try to convert to an integer */
  return short(atol(nameStr));

}

slong GameStateClass::IDOfModule(const char* nameStr, const slong raceID) const {

  /* first try to convert to a long integer id value */
  slong id = atol(nameStr);
  
  if (id > 0 || *nameStr == '0')
    return id;

  /* second try to find a module with the same name and race */
  short i = 0;
  while (i < numModules &&
	 (strcmp(moduleList[i].name, nameStr) ||
	  (raceID != -1 &&
	   raceID != moduleList[i].raceID &&
	   moduleList[i].raceID != -1)))
    i++;
  
  if (i < numModules)
    return moduleList[i].id;

  i = 0;
  while (i < numModules &&
	 (strcmp(moduleList[i].name, nameStr)))
    i++;
  
  if (i < numModules) {
    logFile << "WARNING> match for module identifier " << ":" << nameStr << "[";
    if (raceID != -1)
      logFile << race[raceID].name;
    else 
      logFile << "all";
    
    logFile << "] found outside of specified race." << endl;
    
    return game->moduleList[i].id;
  }

  logFile << "ERROR> unable to map module identifier " << nameStr << endl;

  /* return error condition */
  return -1;
  
}

short GameStateClass::IndexOfPlanet(const char* nameStr) const {

  /* first, try to find a planet with the same name */
  short i = 0;
  while (i < numPlanets && strcmp(planet[i].name, nameStr))
    i++;
  
  if (i < numPlanets)
    return i;
  else
    /* if no match was found then try to convert to an integer */
    return short(atol(nameStr));

}

short GameStateClass::IndexOfRace(const char* nameStr) const {

  /* first, try to find a race with the same name */
  short i = 0;
  while (i < numRaces && strcmp(race[i].name, nameStr))
    i++;
  
  if (i < numRaces)
    return i;
  else
    /* if no match was found then try to convert to an integer */
    return short(atol(nameStr));
  
}

short GameStateClass::IndexOfPlayer(const char* nameStr) const {

  /* first, try to find a player with the same name */
  short i = 0;
  while (i < numStoredPlayers && strcmp(player[i].name, nameStr))
    i++;
  
  if (i < numStoredPlayers)
    return i;
  else {
    /* if no match was found then try to convert to an integer */
    short index = short(atol(nameStr));
    if (index == 0 && nameStr[0] != '0')
      return -1;
    else
      return index;
  }
}

short GameStateClass::IndexOfDesign(const char* nameStr, const ObjectType ot, const slong raceID) const {

  /* first, try to find a design with the same name */
  short i = 0;

  switch (ot) {
  case ::ship:
    while (i < numShipDesigns && (strcmp(shipDesign[i].name, nameStr) ||
				  raceID != shipDesign[i].raceID))
      i++;
    
    if (i < numShipDesigns)
      return i;
    
    break;

  case ::colony:
    while (i < numColonyDesigns && (strcmp(colonyDesign[i].name, nameStr) ||
				    raceID != colonyDesign[i].raceID))
      i++;
    
    if (i < numColonyDesigns)
      return i;
    
    break;

  default :
    break;
  }

  /* if no match was found then try to convert to an integer */
  return short(atol(nameStr));
  
}


int numFramesColonyPending = 0;


void 
GameStateClass::Recycle()
{
  short numObjects = NumObjects();
  short i;
  ObjectUnion obj;

  //
  // Patch to clean up colonyPending Flag
  //
  if ( colonyPending )
    {
      numFramesColonyPending++;
    }
  else
    {
      numFramesColonyPending = 0;
    }

  if ( numFramesColonyPending > 10*frameRate )
    {
      logFile << "NOTE> Colony Pending Flag was stuck.  Unsticking it now.";
      colonyPending = 0;
    }


  //
  // Clean up object list.  If there are two items in list with same
  // value, then zero out the second one.  Also, if there is a bad
  // pointer, clean it out too.
  //

  numMissiles = 0;
  for ( i=numShips + numPlanets + numColonies; i<numObjects; i++ )
    {
      if ( IsValidObjectPointer(objectList[i],__FILE__,__LINE__) )
	{
	  if ( objectList[i]->IsActive() )
	    {
	      numMissiles++;
	    }
	}
      if ( !IsValidObjectPointer(objectList[i],__FILE__,__LINE__) )
	{
	  objectList[i] = NULL;
	}
      for ( int j=i+1; j<numObjects; j++ )
	{
	  if ( objectList[j] == objectList[i] )
	    {
	      objectList[j] = NULL;
	    }
	}
    }

  UpdateMissileList();

  for ( i=0; i<numShips; i++ )
    {
      if ( !IsValidObjectPointer(objectList[i]) )
	{
	  objectList[i] = &ship[i];
	}
      ship[i].design->AddToStat(ship[i].design->volumeCap,0);
    }

  for (i = 0; i < numObjects; i++)
    {
      if ( IsValidObjectPointer(objectList[i]) )
	{      
	  obj.generic = objectList[i];
	  
	  if ( obj.generic->IsActive() ||
	       (obj.generic->Type() == ::ship &&
		obj.ship->stat == DesignedClass::awaiting_construction) )
	    {
	      obj.generic->Update();
	      obj.generic->Recycle();
	    }
	}
    }
}


void GameStateClass::UpdateDockedObject(ShipClass* dockee) {

  if ( !IsValidObjectPointer(dockee) )
    {
      return;
    }

  //
  // If there is a bad docker ptr then sever the link
  //
  if ( !IsValidObjectPointer(dockee->dockObj) )
    {
      dockee->dockObj = 0;
      return;
    }

  //
  // if the docker is not active then sever the link
  //
  ShipClass* docker = dockee->dockObj;
  if ( !docker->IsActive() )
    {
      dockee->dockObj = 0;
      return;
    }

  //
  // If we made it this far s, docker, and dockee->dockObj are
  // all valid objects.
  //
  ModuleClass port = docker->bus.moduleList[dockee->portNum];
  if (dockee->IsExploding())
    {
      dockee->FreeFromDock();
      return;
    }

  //
  // Pointer to the ship that the docker thinks is docked
  // to port.
  //
  ShipClass *dockerDockee = (ShipClass*) port.workPar[0];
  if ( !IsValidObjectPointer(dockerDockee) )
    {
      dockerDockee = NULL;
    }

  int shipIsDockedToPort = port.workPar[1];

  if ( dockee != dockerDockee || !shipIsDockedToPort )
    {

      logFile << 
	"ERROR> illegal docking has occurred for " << dockee->TypeName() <<
	" " << TypeListIndex(dockee) << " docked with " << 
	dockee->dockObj->TypeName() <<
	" " << TypeListIndex(dockee->dockObj) << " at port " << 
	dockee->portNum << " on frame " << frame <<
	". workPar[1] = " << port.workPar[1] << " workPar[0] = ";
      
      if ( dockerDockee )
	{
	  //
	  // dockerDockee != 0 implies that a ship is docked, 
	  // but it is not the same object
	  //
	  logFile << port.workPar[0] << endl;
	  
	}
      else
	{
	  logFile << "null" << endl;
	}
      //dockee->FreeFromDock(1);
      dockee->SendUnDockMsg();
    }
  
  ObjectBitmapClass* bm = objectBitmapList + docker->originalBitmapIndex;
  short nF = bm->numFrames;

  if ( nF )
    {
      
      /**************************************************************/
      /* specPar[1] determines how far from the center the object   */
      /* is positioned.  If a null value is given it should default */
      /* to the bitmap edge.                                        */
      /**************************************************************/
      slong h = port.specPar[1];
      if (!h)
	{
	  h = docker->bitmap.size / 2;
	}
      
      int index = (port.specPar[0] + docker->heading + 3*nF) % nF;

      dockee->now = docker->now +
	bm->dir[index] * (dockee->bitmap.size/2 + h);
      dockee->gnow = dockee->now;
    }
  
  dockee->vis = docker->vis;
  if ( docker->IsTransitioning() )
    {
      dockee->bitmap.index = docker->bitmap.index;
      dockee->offset = docker->offset;
    }
  else if (docker->bitmap.IsTime())
    {
      if (nF && docker->rotRate && docker->frame % docker->rotRate == 0)
	{
	  dockee->heading = (dockee->heading + 1) % nF;
	  dockee->offset = dockee->heading;
	}
    } else
      {
	if ( nF )
	  {
	    dockee->heading = (RotationType)((port.specPar[2] + docker->heading + nF) % nF);
	  }
	dockee->bitmap.index = dockee->originalBitmapIndex;
	dockee->offset = dockee->heading;
      }
  dockee->vel = docker->vel;
  dockee->tvel = dockee->vel;
}


void GameStateClass::UpdateDockedObjects() {
  short i;
  for (i = 0; i < numShips; i++)
    UpdateDockedObject(ship + i);

  for (i = 0; i < numColonies; i++)
    if ( IsValidObjectPointer(colonyList[i]) ) {
       UpdateDockedObject(colonyList[i]);
       }
    else {
      logFile << "ERROR> GameStateClass::UpdateDockedObjects colonyList ["
              << i << "] = NULL" << endl;
      }

}

void GameStateClass::Update() {
  /* Game State Class: update -- recalculate active missile list */

  /* increment frame counter */
  /*frame++;*/
  frame = frameNumber;

  static char msg[1024];
  short i,j;
  for (i = 0; i < numShips+maxPlanets+numMissiles+maxColonies; i++)
  {
    if ( IsValidObjectPointer(objectList[i],__FILE__,__LINE__) )
      {
        objectList[i]->currentSound = -1;
      }
  }

  //UpdateMissileList();
  MapObjectList();

  /* reset each race's total population and robot count */
  for (i=0; i < numRaces; i++) {
    race[i].totPop = 0;
    race[i].numRobots = 0;
    race[i].numPlanets = 0;
    race[i].strategicPoints = 0;
    race[i].activeFrames = 0;
  }

  short numRobots = 0;
  /* count the robots for each race */
  for (i=maxHumanShips; i < numShips; i++) {

    if (ship[i].active) {
      /* increment the robot counter for the race */
      race[ship[i].raceID].numRobots++;
      /* increment the game total of robots */
      numRobots++;
    }
  }

  numActiveShips = 0;

  /* check for rank promotion among active players */
  for (i = 0; i < numShips; i++) {

    /* count the number of active ships */
	  if (ship[i].active && playerList[i]) {
      numActiveShips++;
      playerList[i]->activeFrames++;
      //ship[i].tubeSelector = 0;
      //ship[i].beamSelector = 0;

      /* add up each players strategic points and active frames */
      race[ship[i].raceID].activeFrames += playerList[i]->activeFrames;
      race[ship[i].raceID].strategicPoints += playerList[i]->strategicPoints;

      /* reset the firing angles */
      for (j=0; j<ship[i].bus.numTubes; j++) {
	ship[i].tubeOffset[j] = 0;
	ship[i].beamOffset[j] = 0;
      }

      /* check for player promotions/demotions */
      playerList[i]->CalculateRank(i);

      /* reset the throttle flag */
      ship[i].throttle = 0;
      
      /* reset tube firing flag */
      ship[i].firingTubes = 0;
      
      ship[i].repairRate = (short)max(min(playerList[i]->CrewExperienceLevel() + 1 +
				   ship[i].pop.InUnitsOf(200),
				 ship[i].pop.InUnitsOf(20)), 1) ;

      //
      // Check for a suspicious volume
      //
      if ( IsConnectedClient() &&
	   i == localSlotNumber &&
	   ship[i].Volume() > ship[i].design->volumeCap )
	{
	  HandleCheckSumFail();
	}
      

    }
  }


  slong raceIndex = 0;
  slong optPop;
  totPop = 0;
  for (i = 0; i < numColonies; i++) {

    if ( !IsValidObjectPointer(colonyList[i]) ) {

      logFile << "ERROR> GameStateClass::Update colonyList[" << i
	      << "] = NULL" << endl;
    } else {

      /* reset the tube selector bits */
      //colonyList[i]->tubeSelector = 0;
      //colonyList[i]->beamSelector = 0;

      /* reset the firing angles */
      for (j=0; j<colonyList[i]->bus.numTubes; j++)
	{
	  if ( IsValidObjectPointer(colonyList[i]) )
	    {
	      colonyList[i]->tubeOffset[j] = 0;
	      colonyList[i]->beamOffset[j] = 0;
	    }
	}

    /* reset the throttle flag */
      colonyList[i]->throttle = 0;
    
      /* reset tube firing flag */
      colonyList[i]->firingTubes = 0;

      colonyList[i]->repairRate = (short)max(colonyList[i]->pop.InUnitsOf(200000000) +
				      colonyList[i]->pop.log10(), 1);

      /* add up each races total number of planets */
      if ( IsValidObjectPointer(colonyList[i]->lockObj.generic,__FILE__,__LINE__) &&
           colonyList[i]->lockObj.generic) {
	race[colonyList[i]->raceID].numPlanets++;

	optPop = planet[colonyList[i]->planetIndex].
	  OptimalPopulation(race[colonyList[i]->raceID]).
	  InUnitsOf(1000000);

	/* add up each race's total population */
	/* do not let the colonies population total exceed the planet's */
	/* optimal population */
	race[colonyList[i]->raceID].totPop +=
	  min(colonyList[i]->pop.InUnitsOf(1000000), optPop);
      
	/* add up total system-wide population */
	totPop += optPop;

      }
    }
  }

  /* calculate the number of population-pool robots each race is entitled to */
  for (i=0; i < numRaces; i++) {

    if ( planetsPerRobot )
       race[i].maxPlanetRobots = race[i].numPlanets / planetsPerRobot;

    if (totPop && popPercentPerRobot)
      race[i].maxPopRobots = (short)(race[i].totPop * 100 /
	totPop / popPercentPerRobot);
    else
      race[i].maxPopRobots = 1;

  }

  /* first, make sure there is a slot available */
  if (numRobots < numShips - maxHumanShips && !IsPending()) {
    
    /* select a race at random at see if it can build a ship */
    short iRace = Random(numRaces);
    /* check if this race has a buildable factory */
    ShipClass* factoryObj = FindFreeFactory(iRace);
    
    if (factoryObj &&
	(race[iRace].maxPopRobots + race[iRace].maxPlanetRobots >
	 race[iRace].numRobots)) {
      /**** Ship Construction ****/
      /* a robot slot is available so start building a ship */
      /* new procedure for creating a new ship                      */
      /* (1) calculate the number of robots each player is entitled */
      /*     to                                                     */
      /* (2) select a race at random and if it has a buildable      */
      /*     factory and a deficit of robots then go to step 3      */
      /* (3) find an available player of the same race              */
      /* (4) if none are available find one that is inactive        */
      /* (5) convert the player's race, refit, and activate         */
      
      /* find first available slot */
      i=maxHumanShips;
      while (ship[i].active && i < numShips)
	i++;
      
      if (i == numShips) {
	logFile <<
	  "GameStateClass::Update: couldn't find an open slot on frame"
		<< frame << endl;
	return; 
      }
      
      /* select random starting index */
      short start = 0;
      if ( numRobotPlayers )
         start = (short)(frame % numRobotPlayers);
      
      /**********************************************************/
      /* first try to find one with a factory (space station)   */
      /* rebuilding space stations should be the first priority */
      /**********************************************************/
      short playerIndex = FindFreePlayerWithFactoryIndex(start, iRace);
      
      /* if that fails try to find anybody of the same race */
      if (playerIndex == -1) 
	playerIndex = FindNextFreePlayerIndex(start, iRace);
      
      /* if that fails try to find anybody */
      if (playerIndex == -1) {
	playerIndex = FindNextFreePlayerIndex(start);
      }

      /****************************************************************************/
      /* if that fails then log the problem and do not send the fillslot message, */
      /* otherwise send the message if the proper conditions are met.             */
      /****************************************************************************/
      if (playerIndex != -1) {
      
	/*** message ***/
	/* the following message needs to be sent */
	/* SF_MSG_FILLSLOT */
	/* o    slot of player being built        */
	/* o    index of object that owns factory */
	/* o    index of player being built       */
	/* o    index of race that is building    */
      
      /*** action ***/
      
	if (!player[playerIndex].IsPending() &&
	    /* !IsPending() && */
	    /******************************************************************/
	    /* there is a potential conflict with the ISDOCKED message where  */
	    /* a FILLSLOT message will steal the port from it.  To prevent it */
	    /* we check the docking flag on the factory object.               */
	    /******************************************************************/
	    !factoryObj->IsPending(ShipClass::docking)  &&
	    /* (IsDisconnectedClient() ||
	       (IsConnectedClient() && IsLocalPlayer(ship+i)) ||
	       (IsServer() && game->IsRobotPlayer(ship+i))) */
	    (IsDisconnectedClient() ||
	     IsServer())) {
	  
	  SetPendingFlag();
	  
	  short buildPort = 0;
	  
	  if (factoryObj->Type() == ::ship) {
	    buildPort = factoryObj->FindNextFreePort();
	  }

	  WRITE_SF_LOG(logFile << "sent> Update: sending fill_slot msg on frame "
		       << game->frame << ": " <<
		       i << " " <<
		       ObjectListIndex(factoryObj) << " " <<
		       playerIndex << " " <<
		       iRace << " " <<
		       player[playerIndex].IsVeteran() << " " << 
		       buildPort <<
		       endl);

	  retMsgBuffer.PushMsg((uchar)SF_MSG_FILLSLOT,
			       (ushort)i,
			       (ushort)ObjectListIndex(factoryObj),
			       (ushort)playerIndex,
			       (ushort)iRace,
			       (ushort)player[playerIndex].IsVeteran(),
			       (ushort)buildPort,
			       S_CLIENT_TCP);

	  /*************************************************************************/
	  /* run FillSlot now to lock up the port on the factory ship if necessary */
	  /* this should prevent conflicts over ports from ISDOCKED messages that  */
	  /* come through at the same time.                                        */
	  /*************************************************************************/
	  game->FillSlot(i, playerIndex, iRace, player[playerIndex].IsVeteran(),
			 factoryObj, buildPort);
	  
	  /* set the player's pending flag to prevent this message from  */
	  /* being resent                                                */
	  player[playerIndex].SetPendingFlag();
	}
      } else if (frame / max(frameRate,1) % 120 == 0)
	/* log the error condition every two minutes */
	logFile << "ERROR> unable to find an available player on frame " << frame << endl;
    } /**** Ship Construction ****/
  }
  
  short newPlanetIndexToHarvest = 0;

  /* the following events are checked once each second */
  if (frameRate && frame % frameRate == 0) {

    /* these events are run once every ten seconds */
    if (frame % (10 * frameRate) == 0) {
      for (i = 0; i < numRaces; i++) {
	if (newPlanetIndexToHarvest == GetMaxPopRatioPlanetIndex(i))
	  race[i].harvestIndex = newPlanetIndexToHarvest;
      }
    }

    /* increment time online counters for each active player */
    for (i = 0; i < numShips; i++)
      if (ship[i].active) {
	playerList[i]->secondsOnline++;
	if (ship[i].stat == DesignedClass::orbiting &&
	    ship[i].parent.planet &&
	    !ship[i].parent.planet->IsFriendlyTo(ship+i) &&
	    ship[i].parent.planet->enemyOrbiting == 1) {

	  if (ship[i].parent.planet->colony && !ship[i].parent.planet->colony->IsFriendlyTo(ship + i) &&
	      ship[i].parent.planet->Habitability(race[ship[i].parent.planet->colony->raceID], 0) < self_sustain_limit) {
	  
	    /* call for help every 20 seconds */
	  
	    if (frame - ship[i].parent.planet->colony->distressFrame > 20 * frameRate) {
	      ship[i].parent.planet->colony->SendDistressSignal(short(ship[i].parent.planet->colony->pop.InUnitsOf(1000000)));
	      sprintf(msg, "%s is under siege by the %ss.  Press 'L' to respond.", 
		      ship[i].parent.planet->name, 
		      race[ship[i].raceID].name);
	      PostGameMessage(msg, (short)ship[i].parent.planet->colony->raceID,
			      (short)ship[i].parent.planet->colony->raceID, broadcast);
	      race[ship[i].parent.planet->colony->raceID].distressObj.generic = ship[i].parent.planet->colony;
	      ship[i].parent.planet->colony->AttackObj(ship + i);
	      /* make sure that the colony updates its distress frame */
	      ship[i].parent.planet->colony->distressFrame = frame;
	    }
	  }
	}
      }
    
    /* check victory conditions */
    short j;
    short victoryFlag = 0;
    for (i=0; i<numRaces; i++) {

      race[i].condition = 0;
      switch (race[i].conditionType) {
	
      case hold_num:
      case conquer_num:
	race[i].condition = race[i].numPlanets >= race[i].conditionNumPar;
	break;
      case hold_list:
      case conquer_list:
	if (race[i].conditionNumPar) {
	  race[i].condition = 1;
	  slong op;
	  for (j=0; race[i].conditionNumPar >> j; j++) {

	    /* check to see if the planet is uninhabitable */
	    op = planet[j].OptimalPopulation(race[i]).InUnitsOf(100);

	    if (race[i].conditionNumPar >> j & 1 && op > 0)
	      race[i].condition = race[i].condition &&
		planet[j].raceID == race[i].id;
	  }
	}
	break;
      default:
	break;
      }
      
      switch (race[i].conditionTiming) {
      case within_time:
	if (race[i].condition && race[i].status == RaceClass::normal) {
	  if (race[i].conditionTimePar > 0 &&
	      race[i].conditionSeconds <= race[i].conditionTimePar ||
	      race[i].conditionTimePar < 0 &&
	      race[i].numPlayerDeaths <= -race[i].conditionTimePar) {
	    logFile << "note> The " << race[i].name <<
	      "s have satisfied their victory conditions on frame " << frame << endl;
	    race[i].status = RaceClass::victorious;
	    victoryFlag = 1;
	  }
	} else {
	  race[i].conditionSeconds++;
	  if (!(race[i].conditionTimePar > 0 &&
		race[i].conditionSeconds <= race[i].conditionTimePar ||
		race[i].conditionTimePar < 0 &&
		race[i].numPlayerDeaths <= -race[i].conditionTimePar))
	    /* victory conditions are impossible to achieve */
	    race[i].status = RaceClass::hopeless;
	}
	break;
      case for_time:
	if (race[i].condition) {
	  race[i].conditionSeconds++;
	  if (race[i].conditionSeconds > race[i].conditionTimePar) {
	    logFile << "note> The " << race[i].name <<
	      "s have satisfied their victory conditions on frame " << frame << endl;
	    race[i].conditionSeconds = 0;
	    race[i].status = RaceClass::victorious;
	    victoryFlag = 1;
	  }
	} else
	  race[i].conditionSeconds = 0;
	break;
      case unlimited_time:
	if (race[i].condition) {
	  logFile << "note> The " << race[i].name <<
	    "s have satisfied their victory conditions on frame " << frame << endl;
	  race[i].status = RaceClass::victorious;
	  victoryFlag = 1;
	} else
	  race[i].conditionSeconds++;
	break;
      }
    }

    /* check racial diversity -- if all same race or neutral then reset */
    
    /* set the first non-zero race encountered */
    i = 0;
    do {
      if (planet[i].colony)
	raceIndex = planet[i].colony->raceID;
      i++;
    } while (i < numPlanets && raceIndex == 0);
    
    short same = 1;
    for (i=0; i<numPlanets; i++) {
      if (planet[i].colony)
	
	same = same && (planet[i].colony->raceID == raceIndex);
      
    }
    
    if (same || raceIndex == 0) {
      race[raceIndex].status = RaceClass::victorious;

      if (same) {
	logFile << "note> the " << race[raceIndex].name <<
	  "s have satisfied the conditions for a military victory on frame " << frame << endl;
      }
    }

    slong victoryRaceID = 0;

    /* if one or more races have satisfied victory conditions then set */
    /* other races status flags to "defeated" */
    if (same || victoryFlag) {

      logFile << "note> At scenario completion: " << endl;

      for (i=0; i<numRaces; i++) {

	if (race[i].status != RaceClass::victorious) {
	  race[i].status = RaceClass::defeated;
	  race[i].conditionSeconds = 0;
	}

	logFile << "   " << race[i].name << "s: ";

	switch (race[i].status) {
	  
	case RaceClass::victorious:
	  logFile << "victorious" << endl;
	  /* log why this race was victorious */
	  logFile << "      " << race[i].ProfileCondition() << endl;
	  victoryRaceID = i;
	  break;
	case RaceClass::defeated:
	  logFile << "defeated" << endl;
	  break;

	}
      }

      if (IsServer() || IsDisconnectedClient()) {

	slong bonus;
	unsigned short bonusPct;

	logFile << "note> bonuses were given to the victorious players as follows:" << endl;

	slong conflictDuration = (frameNumber - frameAtLastVictory)/frameRate/2;
        frameAtLastVictory = frameNumber;

	/* hand out bonuses to all winning players */
	for (i=0; i<numShips; i++) {
	  if (ship[i].IsActive() && 
	      race[ship[i].raceID].status == RaceClass::victorious) {
	    bonusPct = BonusPctOf(playerList[i]);
	    bonus = bonusPct * race[ship[i].raceID].bonus / 100;

            if ( bonus > max_bonus )
	      {
                bonus = max_bonus;
	      }

	    //
	    // This code patch prevents the cheating servers from giving 
	    // out max_bonus points every second.
	    //
	    if ( bonus > conflictDuration )
	      {
                bonus = conflictDuration;
	      }

	    logFile << "   * ";
	    logFile.width(24);
	    sprintf(msg, "%s %s", rank[playerList[i]->rank].name, playerList[i]->name);
	    logFile << msg;
	    logFile.width(24);
	    logFile << Format(bonus);
	    logFile.width(8);
	    logFile << "(" << bonusPct << "%)" << endl;

	    /* dispatch message */

	    retMsgBuffer.PushMsg((uchar)SF_MSG_ADDTOPLAYERSCORE,
				 (ushort)planetObjectIndexStart,
				 (ushort)i,
				 (ulong)bonus,
				 0UL,
				 S_CLIENT_TCP);
	  }
	}

	deactivate();

	logFile << "sent> game over on frame " << frame << endl;

	retMsgBuffer.PushMsg((uchar)SF_MSG_RESETGAME,
			     (ushort)planetObjectIndexStart,
			     victoryRaceID,
			     S_CLIENT_TCP);
      
      }
      
      
    }
    
    for (i=0; i<numShipDesigns; i++) {
      /* check for availability of new ship designs */
      if (frame == shipDesign[i].inFrame) {
	sprintf(msg, "%s design now available.", shipDesign[i].name);
	PostGameMessage(msg, (short)shipDesign[i].raceID, (short)shipDesign[i].raceID,
			      broadcast, 0);
	
	logFile << "The " << race[shipDesign[i].raceID].name << " " <<
	  shipDesign[i].name <<
	  " design is available on frame " << frame << endl;
      }
    }


    ColonyClass* c;
    for (i=0; i<numPlanets; i++) {

      /**********************************************************/
      /* a colony will use up one unit of supply per second     */
      /* it will be resupplied once per second if the colony is */
      /* (a) self sufficient                                    */
      /* (b) not orbited by unfriendly ships                    */
      /**********************************************************/
      if ( planet[i].enemyOrbiting && 
	   planet[i].colony &&
	   planet[i].Habitability(race[planet[i].colony->raceID], 0) <
	   self_sustain_limit) {
	planet[i].colony->supply = max(planet[i].colony->supply - 1, 0);
      }
      
      c = planet[i].colony;

      if ( c ) {
	
	if ( planet[i].Habitability(race[planet[i].colony->raceID], 0) >= 
	     self_sustain_limit ||
	     !planet[i].enemyOrbiting)
	  {
	    c->supply = min(c->supply + 2, max_supply);
	  }
	else if (c->supply == 0)
	  {
	    
	    if (IsServer() || IsDisconnectedClient()) {
	      
	      /***************************************************************/
	      /* the colony is without supplies-if an enemy ship is orbiting */
	      /* then it will surrender to that race.                        */
	      /* To determine the victor race follow this procedure:         */
	      /* (1) add up the blockade points for all ships orbiting this  */
	      /*     planet for all races.                                   */
	      /* (2) the race with the highest total wins the planet         */
	      /* (3) the bonus score and credit are handed out according to  */
	      /*     percentage of the total blockade frames for all players */
	      /*     of the winning race.                                    */
	      /***************************************************************/
	      
	      slong raceBlockadeFrames[max_players];
	      slong maxBlockadeRaceIndex = -1;
	      slong maxBlockadeFrames = 0;
	      slong playerCreditBonus;
	      slong playerScoreBonus;

	      /* initialize the accumulators */
	      for (j=0; j<numRaces; j++)
		raceBlockadeFrames[j] = 0;
	      
	      /* add up the frames for each race */
	      for (j=0; j<numShips; j++) {
		if (ship[j].IsActive())
		  raceBlockadeFrames[ship[j].raceID] += 
		    ship[j].blockadeFrames;
	      }
	      
	      /* determine the race with the highest total */
	      for (j=0; j<numRaces; j++) {
		if (maxBlockadeFrames < raceBlockadeFrames[j]) {
		  maxBlockadeRaceIndex = j;
		  maxBlockadeFrames = raceBlockadeFrames[j];
		}
	      }
	      
	      /* if a winning race could be determined... */
	      if (maxBlockadeRaceIndex != -1) {
		
		slong bonus = planet[i].colony->pop.log10() / 2 +
		  planet[i].ExploitationPotential()/100*game->bonusFactor;
		
		slong cBonus = planet[i].ExploitationPotential() * 
		  game->landRightFactor;
		
		/* send a PLANETSIEGESUCCEEDS message */
		if (!colonyPending && !c->IsPending(ShipClass::captured) &&
		    (IsDisconnectedClient() || IsServer())) {
		  
		  /* determine the percentages for each winning player and */
		  /* send messages for player score and credit             */
		  for (j=0; j<numShips; j++) {
		    
		    if ( ship[j].IsActive() && 
			 ship[j].raceID == maxBlockadeRaceIndex ) {
		      playerCreditBonus = cBonus * ship[j].blockadeFrames / 
			max(raceBlockadeFrames[maxBlockadeRaceIndex], 1);
		      playerScoreBonus = bonus * ship[j].blockadeFrames /
			max(raceBlockadeFrames[maxBlockadeRaceIndex], 1);
		      
		      if ( bonus > max_bonus )
			{
			  bonus = bonus;
			}
		      
		      retMsgBuffer.PushMsg((uchar)SF_MSG_ADDTOPLAYERSCORE,
					   (ushort)planetObjectIndexStart,
					   (ushort)j,
					   (ulong)bonus,
					   (ulong)cBonus,
					   S_CLIENT_TCP);
		      
		    }
		  }
		  
		  colonyPending = 1;
		  
		  retMsgBuffer.PushMsg((uchar)SF_MSG_PLANETSIEGESUCCEEDS,
				       (ushort)(planetObjectIndexStart + i),
				       (slong)maxBlockadeRaceIndex,
				       S_CLIENT_TCP);
		}
	      }
	    }
	  }
      }
      
      planet[i].enemyOrbiting = 0;

    }

  }
  
  slong rNew = 0;
  slong rOld = 0;
  short cIndex;
  if ( numColonies > 0 && numColonies < maxColonies - extraColonies) {
    cIndex = (short)((frame + 1) % numColonies);
    if ( IsValidObjectPointer(colonyList[cIndex]) &&
	 IsValidObjectPointer(colonyList[cIndex]->lockObj.generic,__FILE__,__LINE__) ) {
      
      slong loyalty = colonyList[cIndex]->Loyalty();

      if (loyalty < 0) {
	
	if (loyalty < -20) {
  
	  colonyList[cIndex]->pop.GrowBy(70000 + Random(30000));
	  
	} else if ((IsServer() || IsDisconnectedClient()) &&
		   Random((short)(defectionRate / -loyalty),
			   frame+colonyList[cIndex]->frame) == 0 &&
		   !game->colonyPending && 
		   !colonyList[cIndex]->IsPending(ShipClass::spawning)) {
	  rOld = colonyList[cIndex]->raceID;
	  rNew = Random(numRaces);
	  if (rNew == rOld && numRaces )
	    rNew = (rNew + (rNew == rOld)) % numRaces;
	  short pct = Random(100);

	  /******************/
	  /* colony defects */
	  /******************/

	  colonyList[cIndex]->
	    SendSpawnMsg(colonyList[cIndex]->pop / 100 * pct, (short)rNew);
	  
	}
      }
    }
  }
  
}

void
GameStateClass::FillSlot( short i, 
			  short playerIndex,
			  short iRace, 
			  short veteran,
			  ShipClass* factoryObj,
			  short buildPort )
{
  
  static char msg[1024];
  
  /* reset the player's pending flag */
  player[playerIndex].ClearPendingFlag();
  
  /* reset the gamestate's pending flag */
  ClearPendingFlag();
  
  /* make sure alignments match */
  player[playerIndex].raceId = iRace;
  ship[i].raceID = iRace;
  
  /* remove the player from probation */
  player[playerIndex].TakeOffProbation();
  
  /* assign player index to open slot */
  ship[i].playerIndex = playerIndex;
  
  /* record event in log */
  logFile << "received> ship slot " << TypeListIndex(ship+i) <<
    " is being filled on frame " << frame << endl;
  
  WRITE_SF_LOG(logFile << " total system-wide population: " <<
	       totPop << " million." << endl);
  
  WRITE_SF_LOG(logFile << " total " << race[iRace].name << " population: " <<
	       race[iRace].totPop << " million (" << 
	       race[iRace].totPop * 100 / max(totPop,1) << "%)." << endl);
  
  WRITE_SF_LOG(logFile << " number of planets under " << 
	       race[iRace].name << " control: " <<
	       race[iRace].numPlanets << endl);
  
  WRITE_SF_LOG(logFile << " max number of population robots: " <<
	       race[iRace].maxPopRobots << endl);
  
  WRITE_SF_LOG(logFile << " max number of planet robots: " <<
	       race[iRace].maxPlanetRobots << endl);
  
  WRITE_SF_LOG(logFile << " number of robots for " << 
	       race[iRace].name << "s: " <<
	       race[iRace].numRobots << endl);
  
  logFile << " new player selected: " << player[playerIndex].name <<
    " (" << playerIndex << ")" << endl;
  
  WRITE_SF_LOG(if (factoryObj)
	       logFile << " factory object: " << factoryObj->TypeName() << " "
	       << game->TypeListIndex(factoryObj) << " (" <<
	       game->ObjectListIndex(factoryObj) << ")" << endl);
  
  if (veteran) {
    
    /* tell the designated factory to start building */
    factoryObj->Build(ship+i, buildPort);
    
    logFile << "received> a ";
    factoryObj->ShortProfile();
    logFile << " initiated construction of a ";
    ship[i].ShortProfile();
    if (factoryObj->Type() == ::ship)
      logFile << " at port " << ship[i].portNum;
    logFile << " on frame " << frame << endl;
    
    if (factoryObj->Type() == ::ship &&
	(factoryObj->bus.moduleList[ship[i].portNum].workPar[0] != 
	 slong(ship + i) ||
	 factoryObj->bus.moduleList[ship[i].portNum].workPar[1] != 1))
      logFile << "error> docking failed! (workPar[0] == ship + i) = " <<
	(factoryObj->bus.moduleList[ship[i].portNum].workPar[0] == slong(ship + i)) <<
	" workPar[1] = " << factoryObj->bus.moduleList[ship[i].portNum].workPar[1] << endl;
    
    /* the following message is suppressed because it pollutes */
    /* the message window                                      */
    /* sprintf(msg, "%s has joined the %ss",
       player[playerIndex].name, race[iRace].name);
       PostGameMessage(msg, 0, 0, broadcast, 1); */
    
  } else {
    
    /* build instantly the first time a player enters a game */
    ship[i].bus.FreeModuleList();
    ship[i].initializeWith(i, playerIndex);
    
    /* reset the credit value */
    /* this is no longer necessary as the refValue is stored on the nexus */
    //playerList[i]->refValue = 0;
    
    logFile << "received> a ";
    ship[i].ShortProfile();
    logFile << " has entered the game on frame " << frame << endl;
    
    player[playerIndex].numMissions++;
    
    player[playerIndex].EnterGame();
    
    sprintf(msg, "%s logged in as a %s",
	    player[playerIndex].name, race[iRace].name);
    PostGameMessage(msg, 0, 0, broadcast, 1);
    
  }
  
  ship[i].playerIndex = playerIndex;
  
  /* refit ship if player's ship does not match player's race or */
  /* player's rank is greater than required for current ship     */
  CheckRank(playerIndex, ship+i, veteran);
  
  playerList[i] = player+playerIndex;
  
  player[playerIndex].activate();
  
  ship[i].activate();
}


void GameStateClass::UpdateMissileList() {
  short i;
  short j = 0;

  missileObjectIndexStart = colonyObjectIndexEnd + 1;
  missileObjectIndexEnd = missileObjectIndexStart + maxMissiles - 1;

  /* scan list for actives */
  slong n = numMissiles - numMissilesDestroyed;
  for (i = 0; j < n && i < maxMissiles; i++)
    if (missile[i].active) {
      objectList[j+missileObjectIndexStart] = missileList[j] = &missile[i];
      j++;
    }

  /* update counters */
  numMissiles -= numMissilesDestroyed;
  numMissilesDestroyed = 0;
  lastMissile = i;

  j = 0;

  if (numColonies - numColoniesDestroyed + numColoniesCreated > maxColonies) {
    logFile << "ERROR> colony list is going out of bounds on frame " << frame << endl;
    logFile << "       numColonies          = " << numColonies << endl;
    logFile << "       numColoniesDestroyed = " << numColoniesDestroyed << endl;
    logFile << "       numColoniesCreated   = " << numColoniesCreated << endl;
    logFile << "       maxColonies          = " << maxColonies << endl;
  }
  
  for (i = 0; j < min(numColonies - numColoniesDestroyed +
		      numColoniesCreated, maxColonies) && i < maxColonies;
       i++)
    if (colony[i].active) {
      colonyList[j] = &colony[i];
      j++;
    }

  /* update counters */
  numColonies += numColoniesCreated - numColoniesDestroyed;
  numColonies = min(numColonies, maxColonies);
  numColoniesDestroyed = 0;
  numColoniesCreated = 0;
}

short GameStateClass::GetMaxPopRatioPlanetIndex(slong raceID) {
  short i;
  short iMax = 0;
  static PopStruct cPop;
  static PopStruct oPop;
  slong ratio;
  slong maxRatio = 0;

  for (i=1; i<numPlanets; i++) {
    if (planet[i].raceID == raceID &&
	PlanetIsSpawnable(planet + i, this) &&
	(oPop = planet[i].OptimalPopulation(race[raceID])).InUnitsOf(10000) > 0) {
      
      cPop = planet[i].colony->pop;
      
      //rangeFactor = max((game->planet[i] - *this).Mag(0)
      //	    / sub_scale / 1000, 10) + 1;
      
      ratio = cPop.InUnitsOf(100) / oPop.InUnitsOf(10000);
      
      if (ratio /* * rangeFactor */ > maxRatio) {
	maxRatio = ratio /* * rangeFactor */;
	//maxPopRatio = ratio;
	iMax = i;
      }
    }
  }
  
  return iMax;
}

void GameStateClass::BalanceNewPlayerOnServer(short index) {

  /* after a player logon, the server and each client should     */
  /* check the racial balance in the game state and if necessary */
  /* change the raceId of the newly added player to maintain a   */
  /* balance                                                     */

  if (!ServerIsBalanced())
    return;


  short i;
  short iMin = -1;
  short nMin = maxHumanShips + 1;
  short iMax = -1;
  short nMax = -1;
  short* nRace = new short[numRaces];

  for (i=0; i < numRaces; i++)
    nRace[i] = 0;

  for (i = 0; i < maxHumanShips; i++)
    if (ship[i].IsActive())
      nRace[player[ship[i].playerIndex].raceId]++;
  
  for (i=1; i < numRaces; i++) {
    if (nMin > nRace[i]) {
      nMin = nRace[i];
      iMin = i;
    }
    if (nMax < nRace[i]) {
      nMax = nRace[i];
      iMax = i;
    }
  }

  if (nMax - nMin > balanceMaxGap) {
    /* send the new player to the race with the fewest ships */
    playerList[index]->raceId = iMin;
    ship[index].raceID = iMin;
    OldRefitShip(index, 1);

    PostGameMessage
      ("Server> Your race has been reassigned to maintain balance.",
       index, iMin, personal);

    /* assign default planet for new race */
    ship[index].AssignToPlanet(index);

  }

  delete[] nRace;

}

void GameStateClass::SetDefaults() {
  
  if (numPopRobotsSet)
    numPlanetRobots = (numShips - maxHumanShips) - numPopRobots;
  else if (numPlanetRobotsSet)
    numPopRobots = (numShips - maxHumanShips) - numPlanetRobots;
  else {
    numPlanetRobots = (numShips - maxHumanShips) / 2;
    numPopRobots = (numShips - maxHumanShips) - numPlanetRobots;
  }

  
  slong totPop = 0;
  short i;
  for (i = 0; i < numPlanets; i++) {
    /* add up total system-wide population */
    totPop += planet[i].OptimalPopulation(race[0]).InUnitsOf(1000000);
  }

  if (!popPercentPerRobot && numPopRobots)
    popPercentPerRobot = max(100 / numPopRobots, 1);

  if (!planetsPerRobot && numPlanetRobots)
  planetsPerRobot = max(numPlanets / numPlanetRobots, 1);

}

short GameStateClass::ReadData(ifstream& file, SFString* nameList,
			       SFString* valueList, short nList) {
  char name[max_name_char];
  slong val;
  SFString nameString;
  char *out = NULL;

  file >> name;
  if (strcmp(name, "name") == 0) {
    nameString = SFString();
    nameString.ReadFrom(file);
    out = nameString.out();
    if ( out ) {
      strcpy(scenarioName,out);
      delete out;
      out = NULL;
    }
  } else if (strcmp(name, "desc") == 0) {
    nameString = SFString();
    nameString.ReadFrom(file);
    out = nameString.out();
    if ( out ) {
      strcpy(scenarioDesc,out);
      delete out;
      out = NULL;
    }
  } else if (strcmp(name, "rank_demotions") == 0) {
    nameString = SFString();
    nameString.ReadFrom(file);
    if (nameString == "on") {  /* default */
      SetRankDemotions();
    } else
      ClearRankDemotions();
  } else if (strcmp(name, "powerup_action") == 0) {
    nameString = SFString();
    nameString.ReadFrom(file);
    if (nameString == "swap") {  /* default */
      pluginAction = swap;
      pluginPar = 0;
    } else if (nameString.match("level+*%")) {
      /* with this setting, the players will receive credit equal */
      /* to the value of the new system and if there is enough    */
      /* credit to upgrade the existing system to the next level  */
      /* then the upgrade will occur automatically                */
      
      pluginAction = increment_level_upgrade;
      pluginPar = (short) nameString.sub(nameString.Index("+")+1,
				         nameString.Index("%")-1).Long();

    } else if (nameString.match("value+*%")) {
      /* with this setting, the players will receive incremental */
      /* upgrades in system performance each time they pickup a  */
      /* powerup  where the percentage defines approximately what*/
      /* percentage of the value of the powerup is added in      */
      /* attribute enhancements to their current system          */
      pluginAction = increment_pct_value_upgrade;
      pluginPar = (short)nameString.sub(nameString.Index("+")+1,
				        nameString.Index("%")-1).Long();
    } else if (nameString.match("credit+*%")) {
      /* with this setting, the players will receive incremental */
      /* credit each time they pickup a powerup where the        */
      /* percentage defines approximately what percentage of the */
      /* credit they will receive                                */
      pluginAction = increment_pct_credit_upgrade;
      pluginPar = (short)nameString.sub(nameString.Index("+")+1,
				        nameString.Index("%")-1).Long();
    }
  } else if (strcmp(name, "allow_surrender") == 0) {
    /* val determines whether colony surrenders are allowed in the game */
    /* note that without them, dropping a colony on the planet is the   */
    /* only way to take a planet over                                   */
    nameString.ReadFrom(file);
    if (nameString == "y" || nameString == "Y")
      surrenderAllowed = 1;
    else
      surrenderAllowed = 0;
  } else if (strcmp(name, "AI_level") == 0) {
    /* val determines the level of intelligence of the AI ships with    */
    /* respect to using defensive fire against proximity, bombs and     */
    /* smart weapons.  The settings are:                                */
    /* low: no ships or colonies will fire defensively                  */
    /* medium: only Space Stations and planet-bound colonies will use   */
    /*         defensive fire                                           */
    /* high: all ships.                                                 */
    nameString = SFString();
    nameString.ReadFrom(file);
    if (nameString == "low" || nameString == "LOW")
      AILevel = low_AI_level;
    else if (nameString == "medium" || nameString == "MEDIUM")
      AILevel = medium_AI_level;
    else if (nameString == "high" || nameString == "HIGH")
      AILevel = high_AI_level;

  } else if (strcmp(name, "laser_defense") == 0) {
    /* val determines whether human players are allowed to use defensive */
    /* fire against proximity, bombs and smart weapons.                  */
    nameString = SFString();
    nameString.ReadFrom(file);
    if (nameString == "yes" || nameString == "YES" || nameString == "on" || nameString == "ON")
      SetMode(laserDefense);
    else
      ClearMode(laserDefense);

  } else if (strcmp(name, "auto_spawn") == 0) {
    /* val determines whether the server will allow colonies to spawn   */
    /* automatically when they are overpopulated and slots are avail-   */
    /* able                                                             */
    nameString = SFString();
    nameString.ReadFrom(file);
    if (nameString == "on" || nameString == "ON")
      SetMode(autoSpawn);
  } else if (strcmp(name, "balance") == 0) {
    /* val determines whether the server will attempt to balance the    */
    /* number of players on each side                                   */
    nameString = SFString();
    nameString.ReadFrom(file);
    if (nameString == "on" || nameString == "ON")
      SetServerBalance();
  } else
    file >> val;

  if (strcmp(name, "year") == 0) 
    year = val;
  else  if (strcmp(name, "players") == 0) {
    /* maximum number of active players */
    numShips = (short)val;
    maxHumanShips = (short)(numShips - val);
    /* delete old ship list */
    ship.destroy();
    /* create new ship list with new dimensions */
    if (numShips)
      ship.initializeWith(numShips);
    else
      ship.initialize();

    /* remap object list */
    //MapObjectListFromMissileList();
    MapObjectList();
  } else if (strcmp(name, "robots")  == 0)
    maxHumanShips = (short)(numShips - val);
  else if (strcmp(name, "default_angles")  == 0) {
    maxAngles = (short)val; /* default value for max_angles */
    InitDir();
  } else if (strcmp(name, "max_missiles")  == 0) {

    maxMissiles = (short)val; /* default value for max_missiles */

    /* delete old missile and missileList lists */
    missile.destroy();
    missileList.destroy();
    /* create new missile and missileList lists with new dimensions */
    if (maxColonies) {
      missile.initializeWith(maxMissiles);
      missileList.initializeWith(maxMissiles);
    } else {
      missile.initialize();
      missileList.initialize();
    }

    /* remap object list */
    MapObjectList();    
  } else if (strcmp(name, "max_acc")  == 0)
    maxAcc = (short)val; /* maximum acceleration allowed (pixels/s/s) */
  else if (strcmp(name, "ship_g")  == 0)
    planetShipGFactor = (short)val; /* planet-to-ship gravity factor */
  else if (strcmp(name, "planet_g")  == 0)
    planetPlanetGFactor = (short)val; /* planet-to-planet gravity factor */
  else if (strcmp(name, "damage_mass")  == 0)
    damageToMass = (short)val; /* damage to mass ratio for close-proximity */
  else if (strcmp(name, "robot_population_total")  == 0) {
    /* number of robots available as population-pool robots */

    if (!numPlanetRobotsSet) {
      numPopRobotsSet = 1;
      numPopRobots = (short)val; 
    } else
      logFile << "GameStateClass::ReadData: the planet-based number of robots has already been set.  This number will be discarded." << endl;
  } else if (strcmp(name, "robot_population_rate")  == 0)
    /* percent of total system population to support a robot */
    popPercentPerRobot = (short)val; 
  else if (strcmp(name, "robot_planet_total")  == 0) {
    /* number of robots available as planet-pool robots */
    if (!numPopRobotsSet) {
      numPlanetRobotsSet = 1;
      numPlanetRobots = (short)val; 
    } else
      logFile << "GameStateClass::ReadData: the population-based number of robots has already been set.  This number will be discarded." << endl;
  } else if (strcmp(name, "robot_planet_rate")  == 0)
    /* number of planets to support a robot */
    planetsPerRobot = (short)val; 
  else if (strcmp(name, "debris_count")  == 0)
    /* number of debris bitmaps per race */
    debrisCount = (short)val;
  else if (strcmp(name, "extra_colonies")  == 0) {
    /* number of colonies beyond one per planet */
    extraColonies = (short)val; 
    maxColonies = maxPlanets + extraColonies;
    /* delete old colony and colonyList lists */
    colony.destroy();
    colonyList.destroy();
    /* create new colony and colonyList lists with new dimensions */
    if (maxColonies) {
      colony.initializeWith(maxColonies);
      colonyList.initializeWith(maxColonies);
    } else {
      colony.initialize();
      colonyList.initialize();
    }

    /* remap object list */
    MapObjectList();
  } else if (strcmp(name, "light_speed") == 0) {
    /* val is speed of light in pixels per second */
    lightSpeed = val / frameRate * sub_scale;
  } else if (strcmp(name, "defection_rate") == 0) {
    /* val determines the rate at which planets defect */
    defectionRate = val;
  } else if (strcmp(name, "loyalty_cycle") == 0) {
    /* val determines the amount of time in seconds that a planet with  */
    /* zero loyalty will wait without help arriving before surrendering */
    loyaltyCycle = val;
  } else if (strcmp(name, "offensive_factor") == 0) {
    /* val determines the percentage advantage that a colony landing on */
    /* an occupied planet enjoys in combat.  A value of 200 would imply */
    /* that the attacking colony would be evenly matched against a      */
    /* enemy colony with twice the population                           */
    offensiveFactor = val;
  } else if (strcmp(name, "salvage_right_pct") == 0) {
    /* val determines the percentage of the total value the capturing   */
    /* receives in credit for the captured player's ship.  The default  */
    /* value is [salvage_right_pct]                                     */
    salvageRightPct = short(val);
  } else if (strcmp(name, "bounty_right_pct") == 0) {
    /* val determines the percentage of the total value the lastAttacker*/
    /* receives in credit for the destroyed player's ship.  The default */
    /* value is [bounty_right_pct]                                      */
    bountyRightPct = short(val);
  } else if (strcmp(name, "land_right_factor") == 0) {
    /* val determines the percentage of the total value the capturing   */
    /* receives in credit for colonizing a planet.  The default value   */
    /* is [land_right_factor]                                           */
    landRightFactor = short(val);
  } else if (strcmp(name, "max_balance_gap") == 0) {

    /* val the largest gap that can be present between the race with the */
    /* greatest number of ships and the one with the fewest              */
    balanceMaxGap = (short)val;

  } else if (strcmp(name, "bonus_factor") == 0) {

    bonusFactor = (short)val;
    bonusFactor = 1;

  } else if (strcmp(name, "max_colony_millions") == 0) {

    maxColonyMillions = max(val,1);

  }
  short got = file.good();

  if (file.eof()) {
    //logFile << "note> closing input file..." << endl;
    file.close();
    return 0;
  }
  
  file.clear();
  return got;
}

short GameStateClass::NumModulesByType(short rID, short t){
  short i, n = 0, inList;
  for (i=0; i<numModules; i++) {

    if (t != -1)
      inList = game->moduleList[i].IsEquivalentTo(t);
    else
      inList = game->moduleList[i].moduleFunc != ModuleClass::power_plant &&
	game->moduleList[i].moduleFunc != ModuleClass::tube &&
	game->moduleList[i].moduleFunc != ModuleClass::beam &&
	game->moduleList[i].moduleFunc != ModuleClass::ram;

    if (tech[rID].IsSet(i) && inList)
      n++;

  }

  return n;
}

short GameStateClass::ExtendRace(RaceClass& r) {

  short i;

  /* check for array bounds */
  if (numRaces == maxRaces) {
    /* extend the race array boundary */
    if (!race.extendBy(1)) {
      logFile <<
	"ERROR> unable to extend race array" << 
	endl;
      
      return 0;
    }

    maxRaces++;
  }
  
  race[numRaces] = r;

  /* reallocate tech array */
  ListStruct* oldTech = tech;
  tech = new ListStruct[numRaces + 1];

  if (!tech) {
    logFile <<
      "ERROR> unable to extend tech array" << 
      endl;
    
    return 0;
  }
  
  for (i=0; i<numRaces; i++)
    tech[i] = oldTech[i];

  tech[numRaces].InitializeWith(0);
  
  ::delete[] oldTech;

  numRaces++;

  return 1;
}

short GameStateClass::AddRace(ifstream& file,
			      SFString* nameList, SFString* valueList,
			      short nList) {
  RaceClass r;
  short got = r.get(file, nameList, valueList, nList);
  if (got) {

    ExtendRace(r);

  } else if (file.eof()) {
    //logFile << "note> closing input file..." << endl;
    file.close();
    return 0;
  } else if (!file) {
    logFile << "ERROR> failed to add race " <<
      numRaces+1 << endl;
  }
  return got;
}

short GameStateClass::AddRank(ifstream& file,
			      SFString* nameList, SFString* valueList,
			      short nList) {
  RankStruct r;
  short got = r.get(file, nameList, valueList, nList);
  if (got) {

    if (!rank.add(r)) {

      logFile << "ERROR> unable to extend rank array" << endl;
      return 0;
    }

    numRanks++;

  } else if (file.eof()) {
    //logFile << "note> closing input file..." << endl;
    file.close();
    return 0;
  } else if (!file) {
    logFile << "ERROR> failed to add rank " <<
      numRanks+1 << endl;
  }
  return got;
}

short GameStateClass::ExtendColonyDesign(ColonyDesignStruct& c) {

  short i;

  ColonyDesignStruct* oldDesign = colonyDesign;
  colonyDesign = new ColonyDesignStruct[numColonyDesigns+1];
  /* copy designs from old list into new list */
  for (i = 0; i < numColonyDesigns; i++)
    colonyDesign[i] = oldDesign[i];
  /* remap design pointers for all colonies */
  for (i = 0; i < numColonies; i++)
    colony[i].design = colonyDesign + ((ColonyDesignStruct *)colony[i].design - oldDesign);
  colonyDesign[numColonyDesigns] = c;
  
  numColonyDesigns++;

  if ( oldDesign ) {
    delete[] oldDesign;
    oldDesign = NULL;
  }
  
  return 1;
}

short GameStateClass::ExpandColony() {

  /* check for array bounds */
  if (numColonies == maxColonies || maxPlanets + extraColonies != maxColonies) {
    /* extend the colony array boundary */
    if (!colony.extendBy(1) || !colonyList.extendBy(1)) {
      logFile <<
	"ERROR> unable to extend colony array"
	      << endl;
      
      return 0;
    } else {
      /* we can now safely transfer the old array contents */
      maxColonies++;

    }
  }

  return 1;
}

short GameStateClass::ExtendColony(ColonyClass& c) {

  if (ExpandColony()) {

    colony[numColonies] = c;

    if (colony[numColonies].lockObj.planet)
      colony[numColonies].InitializeFromColonyDesign(colony + numColonies);
    else
      colony[numColonies].InitializeFromDesign(colony + numColonies);

    /* activate the new object */
    colony[numColonies].activate();
    /* map the colonies into the object list before MapObjects runs */
    colonyList[numColonies] = colony + numColonies;

    numColonies++;

    /* we may also need to extend the planet array */
    if (maxPlanets + extraColonies != maxColonies)
      ExpandPlanet();

    return 1;

  } else

    return 0;
}

short GameStateClass::AddColony(ifstream& file,
			      SFString* nameList, SFString* valueList,
			      short nList) {
  ColonyClass c;
  short got = c.get(file, nameList, valueList, nList);
  if (got) {
    
    ExtendColony(c);

  } else if (file.eof()) {
    //logFile << "note> closing input file..." << endl;
    file.close();
    return 0;
  } else if (!file) {
    logFile << "ERROR> failed to add colony " <<
      numColonies+1 << endl;
  }
  return got;
}

short GameStateClass::AddShip(ifstream& file,
			      SFString* nameList, SFString* valueList,
			      short nList) {
  ShipClass s;
  short got = s.get(file, nameList, valueList, nList);

  if (got) {
    short i = maxHumanShips;
    while (ship[i].active && i < numShips)
      i++;
    
    if (i == numShips) {
      logFile <<
	"ERROR> AddShip couldn't find an open slot." << endl;
      return 0; 
    }
    
    ship[i] = s;
    ship[i].InitializeFromDesign(ship+i);
    ship[i].activate();
    ship[i].SetMode(DesignedClass::mode_auto, op_on);
    ship[i].viewObj.ship = ship + i;


    playerList[i] = player + ship[i].playerIndex;
    playerList[i]->numMissions++;
    playerList[i]->EnterGame();
    playerList[i]->activate();

  } else if (file.eof()) {
    //logFile << "note> closing input file..." << endl;
    file.close();
    return 0;
  }

  return got;
}

short GameStateClass::ExpandPlayer() {

  /* check for array bounds */
  if (numStoredPlayers == maxStoredPlayers) {
    /* extend the player array boundary */
    if (!player.extendBy(1)) {
      logFile <<
	"ERROR> unable to extend player array"
	      << endl;
      
      return 0;
    } else {
      /* we can now safely transfer the old array contents */
      maxStoredPlayers++;

    }
  }

  return 1;
}

short GameStateClass::ExtendPlayer(PlayerClass& p) {

  if (ExpandPlayer()) {

    player[numStoredPlayers] = p;
    player[numStoredPlayers].LockSlot();

    numStoredPlayers++;

    return 1;

  } else

    return 0;
}

short GameStateClass::AddPlayer(ifstream& file,
			      SFString* nameList, SFString* valueList,
			      short nList) {
  PlayerClass p;
  short got = p.get(file, nameList, valueList, nList) &&
    numStoredPlayers < maxStoredPlayers;
  if (got) {
    ExtendPlayer(p);
  } else if (file.eof()) {
    //logFile << "note> closing input file..." << endl;
    file.close();
    return 0;
  } else if (!file) {
    logFile << "ERROR> failed to add player " <<
      numStoredPlayers+1 << endl;
  }
  numRobotPlayers = numStoredPlayers;
  return got;
}

PlayerClass* GameStateClass::AddPlayer() {

  //if (maxStoredPlayers < maxHumanPlayers)
  //  ExpandPlayer();
  
  short i = 0;
  while (!player[i].SlotIsAvailable() && i < maxStoredPlayers)
    i++;
  if (i == maxStoredPlayers)
    return 0;
  else {
    numStoredPlayers++;
    player[i].LockSlot();
    return player+i;
  }
}

void GameStateClass::RemovePlayer(short pI) {
  player[pI].FreeSlot();
  numStoredPlayers--;
}

PlayerClass* GameStateClass::FindPlayer(char* name, char* login) {
  short i=numRobotPlayers;
  while (i < numStoredPlayers && (strcmp(player[i].name, name) != 0 ||
                                  strcmp(player[i].login, login) != 0))
    i++;
  if (i < numStoredPlayers)
    return player + i;
  else 
    return AddPlayer();
}

short GameStateClass::AddSoundEvent(ifstream& file,
			      SFString* nameList, SFString* valueList,
			      short nList) {

  char name[max_name_char], soundStr[max_name_char], c;
  short soundIndex;

  file >> c;
  if (c == '}')
    return 0;

  file.putback(c);

  file >> name >> soundStr;

  soundIndex = IndexOfSound(soundStr);

  short i = 0;
  while (i < last_event && strcmp(name, eventName[i]))
    i++;

  if (i == last_event) {
    logFile << "ERROR> unknown event identifier (" << name << ") found." << endl;
    return 0;
  }

  soundEvent[i] = soundIndex;
  
  short got = file.good();

  if (file.eof()) {
    //logFile << "note> closing input file..." << endl;
    file.close();
    return 0;
  }

  file.clear();
  return got;
}

short GameStateClass::AddSound(ifstream& file,
			      SFString* nameList, SFString* valueList,
			      short nList) {

  SoundClass s;
  short got = s.get(file);
  if (got) {
    
    /* extend the sound array boundary */
    if (!sound.add(s)) {
      logFile <<
	"ERROR> unable to extend sound array" << endl;
      
      return 0;
    }
    
    numSounds++;

  } else if (file.eof()) {
    //logFile << "note> closing input file..." << endl;
    file.close();
    return 0;
  }

  file.clear();

  return got;
}

short GameStateClass::ExtendBitmap(GameBitmapClass& b) {

  /* check for array bounds */
  if (numBitmaps == maxBitmaps) {
    /* extend the bitmap array boundary */
    if (!objectBitmapList.add(b)) {
      logFile <<
	"ERROR> unable to extend bitmap array"
	      << endl;
      
      return 0;
    } else {
      /* we can now safely transfer the old array contents */
      maxBitmaps++;

    }
  }

  objectBitmapList[numBitmaps].index = numBitmaps;

  numBitmaps++;

  return 1;
}

short GameStateClass::AddBitmap(ifstream& file, char* className,
			      SFString* nameList, SFString* valueList,
			      short nList) {
  GameBitmapClass tBmp;
  short got = tBmp.get(file, nameList, valueList, nList);
  if (got) {

    strcpy(tBmp.Class, className);

    ExtendBitmap(tBmp);

  } else if (file.eof()) {
    //logFile << "note> closing input file..." << endl;
    file.close();
    return 0;
  } else if (!file) {

    logFile << "ERROR> failed to add bitmap " <<
      numBitmaps+1 << endl;

  }
  return got;
}

short GameStateClass::AddColonyDesign(ifstream& file,
			      SFString* nameList, SFString* valueList,
			      short nList) {
  ColonyDesignStruct c;
  short got = c.get(file, ::colony, nameList, valueList, nList);
  if (got) {
    
    ExtendColonyDesign(c);

  } else if (file.eof()) {
    //logFile << "note> closing input file..." << endl;
    file.close();
    return 0;
  } else if (!file) {
    logFile << "ERROR> failed to add colony design " <<
      numColonyDesigns+1 << endl;
  }
  return got;
}

short GameStateClass::AddShipDesign(ifstream& file,
			      SFString* nameList, SFString* valueList,
			      short nList) {
  ShipDesignStruct sd;
  short got = sd.get(file, ::ship, nameList, valueList, nList) && 
    numShipDesigns < max_ship_designs;
  if (got) {
    shipDesign[numShipDesigns] = sd;
    numShipDesigns++;
  } else if (file.eof()) {
    //logFile << "note> closing input file..." << endl;
    file.close();
    return 0;
  } else if (!file) {
    logFile << "ERROR> failed to add ship design " <<
      numShipDesigns+1 << endl;
  }
  file.clear();
  return got;
}

short GameStateClass::AddMissileDesign(ifstream& file,
			      SFString* nameList, SFString* valueList,
			      short nList) {
  MissileDesignStruct md;
  short got = md.get(file, nameList, valueList, nList) &&
    numMissileDesigns < max_missile_designs;
  if (got) {
    missileDesign[numMissileDesigns] = md;
    numMissileDesigns++;
  } else if (file.eof()) {
    //logFile << "note> closing input file..." << endl;
    file.close();
    return 0;
  } else if (!file) {
    logFile << "ERROR> failed to add missile design " <<
      numMissileDesigns+1 << endl;
  }
  file.clear();
  return got;
}

short GameStateClass::ExpandPlanet(PlanetClass* p) {

  short i;

  /* check for array bounds */
  if (numPlanets == maxPlanets || maxPlanets + extraColonies != maxColonies) {
    /* extend the planet array boundary */
    PlanetClass* oldPlanet = planet.rootOf();
    if (!planet.extendBy(1)) {
      logFile <<
	"ERROR> unable to extend planet array"
	      << endl;
      
      return 0;
    } else {
      /* we can now safely transfer the old array contents */
      maxPlanets++;

      for (i=0; i < numPlanets; i++) {
	
	/* reassign planet parent relationships */
	if (planet[i].parent.planet)
	  planet[i].parent.planet = planet + (planet[i].parent.planet - oldPlanet);
      }

      /* if a new object is passed in then remap its parent assignment as well */
      if (p && p->parent.planet)
	p->parent.planet = planet + (p->parent.planet - oldPlanet);
      
    }
  }

  return 1;
}

short GameStateClass::ExtendPlanet(PlanetClass& p) {

  if (ExpandPlanet(&p)) {

    planet[numPlanets] = p;
    planet[numPlanets].activate();
    numPlanets++;

    return 1;
  } else

    return 0;
}

short GameStateClass::AddPlanet(ifstream& file,
				SFString* nameList, SFString* valueList,
				short nList) {
  PlanetClass g;
  short got = g.get(file, nameList, valueList, nList);
  if (got) {

    ExtendPlanet(g);

    /* we may also need to extend the colony array */
    if (maxPlanets + extraColonies != maxColonies)
      ExpandColony();

  } else if (file.eof()) {
    //logFile << "note> closing input file..." << endl;
    file.close();
    return 0;
  } else if (!file) {
    logFile << "ERROR> failed to add planet " <<
      numPlanets+1 << endl;
  }
  file.clear();
  return got;
}

short GameStateClass::AddModule(ifstream& file,
				SFString* nameList, SFString* valueList, 
				short nList) {
  ModuleClass m;
  short got = m.get(file, nameList, valueList, nList) && numModules < maxModules;

  if (got) {
    moduleList[numModules] = m;
    numModules++;
  } else if (file.eof()) {
    //logFile << "note> closing input file..." << endl;
    file.close();
    return 0;
  } else if (!file) {
    logFile << "ERROR> failed to add module " <<
      numModules+1 << endl;
  }
  file.clear();
  return got;
}

ShipClass* GameStateClass::FindFreeFactory() {
  short i;
  slong maxProd = 0;
  ShipClass* s = 0;

  /********************************************************************/
  /* note: change the starting point in the loop from maxHumanPlayers */
  /* to 0 to allow human ships to start building.  See the note in    */
  /* SendDockActivationMsg.                                           */
  /********************************************************************/

  for(i=maxHumanShips; i < numShips; i++) {
    if (ship[i].bus.factoryMod && !ship[i].IsExploding() &&
	ship[i].IsAvailable() && !ship[i].IsDone() &&
	/* to prevent selecting a ship that is already docking */
	!ship[i].IsPending(ShipClass::docking) &&
	!ship[i].bus.factoryMod->active && ship[i].FindNextFreePort())
      if (maxProd < ship[i].bus.factoryMod->specPar[0] &&
	  ship[i].bus.factoryMod->specPar[2] > lower_production_limit) {
	maxProd = ship[i].bus.factoryMod->specPar[0];
	s = ship + i;
      }
  }
  for(i=0; i < numColonies; i++) {
    if ( IsValidObjectPointer(colonyList[i]) ) {
      if (colonyList[i]->bus.factoryMod && !colonyList[i]->IsExploding() &&
          colonyList[i]->IsActive() && !colonyList[i]->IsDone() &&
 	  !colonyList[i]->bus.factoryMod->active)
        if (maxProd < colonyList[i]->bus.factoryMod->specPar[0] && 
	    colonyList[i]->bus.factoryMod->specPar[2]>lower_production_limit) {
          maxProd = colonyList[i]->bus.factoryMod->specPar[0];
	  s = colonyList[i];
        }
    } else {
      logFile << "ERROR> GameStateClass::FindFreeFactory colonyList["
              << i << "] = NULL" << endl;
    }
  }
  return s;
}

ShipClass* GameStateClass::FindFreeFactory(slong iR) {
  short i;
  slong maxProd = 0;
  ShipClass* s = 0;

  /********************************************************************/
  /* note: change the starting point in the loop from maxHumanPlayers */
  /* to 0 to allow human ships to start building.  See the note in    */
  /* SendDockActivationMsg.                                           */
  /********************************************************************/

  for(i=maxHumanShips; i < numShips; i++) {
    if (ship[i].raceID == iR && ship[i].bus.factoryMod &&
	ship[i].IsActive() && !ship[i].IsExploding() &&
	!ship[i].IsDone() && !ship[i].bus.factoryMod->active &&
	/* to prevent selecting a ship that is already docking */
	!ship[i].IsPending(ShipClass::docking) &&
	ship[i].FindNextFreePort())
      if (maxProd < ship[i].bus.factoryMod->specPar[0]) {
	maxProd = ship[i].bus.factoryMod->specPar[0];
	s = ship + i;
      }
  }

  for(i=0; i < numColonies; i++) {
    if ( IsValidObjectPointer(colonyList[i]) ) {
      if (colonyList[i]->raceID == iR && colonyList[i]->bus.factoryMod &&
	  colonyList[i]->IsActive() && !colonyList[i]->IsExploding() &&
	  !colonyList[i]->IsDone() && !colonyList[i]->bus.factoryMod->active &&
	  !colonyList[i]->IsPending(ShipClass::disappearing))
        if (maxProd < colonyList[i]->bus.factoryMod->specPar[0]) {
	  maxProd = colonyList[i]->bus.factoryMod->specPar[0];
	  s = colonyList[i];
        }
      } else {
	logFile << "ERROR> GameStateClass::FindFreeFactory colonyList["
		<< i << "] = NULL" << endl;
      }
  }

  return s;
}

short GameStateClass::FindNextFreePlayerIndex(short start) {
  short i = start;
  while (i < start + numRobotPlayers &&
	 (player[i % numRobotPlayers].IsActive() ||
	  player[i % numRobotPlayers].IsPending())) 
    i++;
  if (i % numRobotPlayers == start && i != start) 
    return -1;
  else
    return i % numRobotPlayers;
}

short GameStateClass::FindNextFreePlayerIndex(short start, slong race) {
  short i = start;
  /* if (FindFreeFactory(0, race))
     return 0; default player 
     else */
  while (i < start + numRobotPlayers &&
	 (player[i % numRobotPlayers].IsActive() || 
	  player[i % numRobotPlayers].IsPending() ||
	  player[i % numRobotPlayers].raceId != race)) 
    i++;
  if (i % numRobotPlayers == start && i != start) 
    return -1;
  else
    return i % numRobotPlayers;
}

short GameStateClass::FindFreePlayerWithFactoryIndex(short start, slong race) {
  short i = start;

  while (i < start + numRobotPlayers &&
	 (player[i % numRobotPlayers].IsActive() || 
	  player[i % numRobotPlayers].IsPending() ||
	  player[i % numRobotPlayers].raceId != race ||
	  !shipDesign[player[i % numRobotPlayers].shipIndex].factory)
	 ) 
    i++;
  if (i % numRobotPlayers == start && i != start) 
    return -1;
  else
    return i % numRobotPlayers;

}

short GameStateClass::FindNextShipTypeIndex(slong raceID,
					    short startIndex,
					    short rank,
					    short acPlayer) {
  short i;
  for (i = startIndex + 1; i < startIndex + numShipDesigns; i++) {
    if (shipDesign[i%numShipDesigns].raceID == raceID &&
	shipDesign[i%numShipDesigns].reqRank <= rank &&
	shipDesign[i%numShipDesigns].inFrame <= frame &&
	(objectBitmapList[shipDesign[i%numShipDesigns].bitmapIndex].acOnly && acPlayer ||
	 !objectBitmapList[shipDesign[i%numShipDesigns].bitmapIndex].acOnly))
      return i % numShipDesigns;
  }
  return startIndex;
}

short GameStateClass::FindNextShipTypeIndexAtLevel(slong raceID, 
						   short startIndex,
						   short rank,
						   short acPlayer) {
  short currentLevel = shipDesign[startIndex].reqRank;
  //rank;
  short designSel = startIndex;
  short i;
  for (i = startIndex + 1; i < startIndex + numShipDesigns; i++) {
    if (shipDesign[i%numShipDesigns].raceID == raceID &&
	shipDesign[i%numShipDesigns].reqRank <= rank &&
	shipDesign[i%numShipDesigns].reqRank >= currentLevel &&
	shipDesign[i%numShipDesigns].inFrame <= frame &&
	(objectBitmapList[shipDesign[i%numShipDesigns].bitmapIndex].acOnly && acPlayer ||
	 !objectBitmapList[shipDesign[i%numShipDesigns].bitmapIndex].acOnly)) {
      designSel = i % numShipDesigns;
      currentLevel = shipDesign[i%numShipDesigns].reqRank;
    }
  }
  return designSel;
}

void GameStateClass::RandomizeGameState(slong raceID) {
  logFile << "note> game state randomized on frame " << 
    frame << endl;

  short i;

  /* destroy all missiles */
  for (i=0; i<numMissiles; i++)
    {
      if ( IsValidObjectPointer(missileList[i]) )
	{
	  missileList[i]->Explode("the game ended.");
	}
    }

  /* destroy all free-floating colonies and colonies on planets with no revert race */
  for (i=0; i < numColonies; i++)
    {
      if ( IsValidObjectPointer(colonyList[i]) )
	{
	  if ( !colonyList[i]->lockObj.generic)
	    {
	      ReceiveDestroyObjectMsg(colonyList[i], 0);
	    }
	  else if (colonyList[i]->lockObj.planet->revertRaceIndex == -1)
	    {
	      ReceiveDestroyObjectMsg(colonyList[i], 0);
	    }
	}
    }
  
  /* remap the object lists */
  MapObjectList();

  /* revert planets to revertRaceIndex alignment */
  for (i=0; i < numPlanets; i++) {

    if (planet[i].revertRaceIndex != -1 && 
	planet[i].OptimalPopulation(race[planet[i].revertRaceIndex]) > PopStruct()) {
      
      CreateColonyOnPlanet(i, planet[i].revertRaceIndex,
			   planet[i].
			   OptimalPopulation(race[planet[i].revertRaceIndex]),
			   planet[i].revertColonyDesignIndex);

      /* set population to 90% of optimal */
      planet[i].colony->pop.GrowBy(90000);

    }
    
  }

  /* reset all player scores and active frames */
  for (i=0; i<numStoredPlayers; i++) {
    player[i].SetStat(player[i].strategicPoints,0);
    //player[i].strategicPoints = 0;
    player[i].activeFrames = 0;
  }

  /* reset all player alignments */
  for (i=0; i<numRobotPlayers; i++) {
    player[i].raceId = player[i].revertRaceId;
    player[i].SetNewPlayerMode();
  }


  /* recreate all computer-controlled ships */
  for (i=maxHumanShips; i < numShips; i++)
    ship[i].Explode("the game ended.", 0); /* do not spawn debris */

  /*************************************/
  /* RACE:                             */
  /* reset all victory timing counters */
  /* reset colonization target         */
  /*************************************/
  for (i=0; i<numRaces; i++) {
    race[i].status = 0;
    race[i].conditionSeconds = 0;
    race[i].condition = 0;
    race[i].targetIndex = 0;
    race[i].targetIndexRank = 0;
    race[i].harvestIndex = 0;
  }

  /* reinitialize the tech lists */
  InitializeTechList();

  PostGameMessage("A system-wide revolution has occurred!",
		  (short)raceID, 0, broadcast, 1);  

  activate();
}

void GameStateClass::ExchangeShips(short i) {
  /* ship[i] tries to switch ships with the ship docked at its primary port */

  static char msg[1024];

  static ShipClass sh;

  if ( ship[i].bus.dockMod && 
       ship[i].bus.dockMod[0] && 
       ship[i].bus.numPorts > 0 &&
       ship[i].bus.dockMod[0]->workPar[1] ) {
    /* this player must outrank the other */
    ObjectUnion s;
    s.objLong = ship[i].bus.dockMod[0]->workPar[0];
    
    if ( !IsValidObjectPointer(s.generic,__FILE__,__LINE__))
      return;
    
    if (s.generic->Type() != ::ship) {
      
      sprintf(msg,
	      "1st Officer> You can't assume command of a %s!",
	      s.generic->TypeName());
      PostGameMessage(msg, i, (short)ship[i].raceID, personal);
      
      return;
    }
    
    if (player[s.ship->playerIndex].rank >= playerList[i]->rank &&
	!player[s.ship->playerIndex].IsOnProbation()) {
      
      sprintf(msg, "1st Officer> I don't think %s %s is willing to trade!",
	      rank[player[s.ship->playerIndex].rank].name,
	      player[s.ship->playerIndex].name);
      PostGameMessage(msg, i, (short)ship[i].raceID, personal);
      
      return;
    }
    
    if (s.ship->bitmap.acOnly || ship[i].bitmap.acOnly && 
	(player[s.ship->playerIndex].IsDemoPlayer() ||
	 player[ship[i].playerIndex].IsDemoPlayer())) {
      
      if (player[s.ship->playerIndex].IsDemoPlayer()) {
	
	sprintf(msg, "1st Officer>  %s %s is not a member of the "SF_REGISTERED_CLUB".",
		rank[player[s.ship->playerIndex].rank].name,
		player[s.ship->playerIndex].name);
	PostGameMessage(msg, i, (short)ship[i].raceID, personal);
	
	return;
      } else {
	sprintf(msg, "1st Officer> %s %s's ship is "SF_REGISTERED_CLUB" only.",
		rank[player[s.ship->playerIndex].rank].name,
		player[s.ship->playerIndex].name);

	PostGameMessage(msg, i, (short)ship[i].raceID, personal);
	return;
      }
    }

    sh = *s.ship;
    short playerIndex2 = ship[i].playerIndex;
    *s.ship = ship[i];
    s.ship->playerIndex = sh.playerIndex;
    ship[i] = sh;
    ship[i].playerIndex = playerIndex2;
      
    /***********************************************************/
    /* sh will be destroyed in the global scope, so to prevent */
    /* the modulelist from causing a crash, we sever the link  */
    /* to the ship[i] object by setting its moduleList pointer */
    /* to NULL.                                                */
    /***********************************************************/
    sh.bus.moduleList._initialize();
    sh.bus.InitializeWith(0);
    sh.initialize();

      /* remap bus host pointers */
    ship[i].bus.MapFunctions(ship+i, ship+i);
    s.ship->bus.MapFunctions(s.ship, s.ship);
      
    short j;
    ShipClass* d;
    /* redirect the dock owner's dock pointers */
    for (j=0; j<s.ship->bus.numPorts; j++) {
      if (s.ship->bus.dockMod[j] &&
	  s.ship->bus.dockMod[j]->workPar[0]) {
	d = (ShipClass*)s.ship->bus.dockMod[j]->workPar[0];
	if (d == s.ship) {
	  s.ship->bus.dockMod[j]->workPar[0] = slong(ship + i);
	  /* redirect all docked ship's dockObj pointers */
	  ship[i].dockObj = s.ship;
	  ship[i].portNum = s.ship->bus.moduleList.indexOf(s.ship->bus.dockMod[j]);
	} else {
	  d->dockObj = s.ship;
	  /* this shouldn't change, but just in case */
	  if (d->portNum != s.ship->bus.moduleList.indexOf(s.ship->bus.dockMod[j])) {

	    logFile << "WARNING> port " << moduleList.indexOf(s.ship->bus.dockMod[i]) << ", holding " << 
	      d->TypeName() << " " <<
	      game->TypeListIndex(d) <<
	      ", changed its index during a ship exchange for a ";
	    s.ship->ShortProfile();
	    logFile << " on frame " << game->frame << endl;
	      
	    d->portNum = s.ship->bus.moduleList.indexOf(s.ship->bus.dockMod[j]);
	  }
	}
      }
    }

    /* remap bus host pointers */
    ship[i].bus.RemapFunctions(ship+i, ship+i, "a host ship exchange");
    s.ship->bus.RemapFunctions(s.ship, s.ship, "a docked ship exchange");

      /* reset robot-behavior flags */
    if (IsRobotPlayer(ship+i))
      ship[i].SetMode(DesignedClass::mode_auto, op_on);
    else {
      ship[i].SetMode(DesignedClass::mode_auto, op_off);
      ship[i].duty = none;
      ship[i].assign = DesignedClass::manual;
      ship[i].stat = DesignedClass::idle;
    }

    if (IsRobotPlayer(s.ship))
      s.ship->SetMode(DesignedClass::mode_auto, op_on);
    else {
      s.ship->SetMode(DesignedClass::mode_auto, op_off);
      s.ship->duty = none;
      s.ship->assign = DesignedClass::manual;
      s.ship->stat = DesignedClass::idle;
    }

    /* set view objects */
    s.ship->viewObj.ship = s.ship;
    ship[i].viewObj.ship = ship + i;
      
  }
}

void 
GameStateClass::BlastObjects(DesignedClass* fromObj,
			     slong range,
			     slong damage,
			     short push,
			     short vortex)
{
  
  short i;
  slong dam;
  slong dist, rangeF;
  slong exempt;
  ObjectUnion obj;
  
  static CoordinateClass disp;
  for (i=0; i<NumObjects(); i++)
    {
      
      obj.generic = objectList[i];
      
      if ( IsValidObjectPointer(obj.generic,__FILE__,__LINE__) &&
	   !IsInInternetCafe(obj.generic) &&
	   obj.generic->IsActive() &&
	   obj.generic->Type() != ::planet &&
	   obj.generic != fromObj && 
	   !obj.generic->IsExploding() )
	{
	  
	  exempt = 0;
	  
	  if (obj.generic->Type() == ::missile &&
	      obj.missile->design->tracking == ::bomb)
	    {
	      exempt = 1;
	    }
	  else if ( obj.generic->Type() >= ::designed &&
		    (obj.designed->stat == DesignedClass::under_construction ||
		     obj.designed->IsWarping()) )
	    {
	      exempt = 1;
	    }
	  
	  disp = *obj.generic - *fromObj;
	  dist = disp.Mag();
	  rangeF = max(range - dist, 0) / max(range / sub_scale, 1);
	  
	  dam = damage * rangeF / (fromObj->frame - fromObj->bitmapStartFrame + 1) / sub_scale;
	  /* normalize the displacement vector */
	  disp = disp * sub_scale / (dist+1);
	  if (dam && !exempt)
	    obj.generic->Hit(dam, fromObj->parent.designed, disp, push, vortex);
	}
    }
}


void GameStateClass::PostGameMessage(char* msg, short fromPlayerSlot,
				     short toRace,
				     short msgType, short global) {

  short fromRace;

  if (msgType & broadcast) {

    fromRace = fromPlayerSlot;

    warningBottomMessagePosted = 1;
    warningBottomMessagePostedFrame = frame;

    if (!global)
      PushMessageNew(FROM_RACE0-min(fromRace,3), FROM_RACE0-min(toRace,3),
		     msg);
    else
      PushMessageNew(FROM_GOD, TO_BROADCAST, msg);
  }
  
  if (msgType & personal && localSlotNumber == fromPlayerSlot) {

    fromRace = (short)ship[fromPlayerSlot].raceID;

    warningMessagePosted = 1;
    warningMessagePostedFrame = frame;
    SetScreenMessage(msg, fromRace);
  }
}

void GameStateClass::DecideToRefitShip(short i, short force, short build,
				       ShipClass* s) {

  static char msg[1024];

  /* allow RefitShip to run only when appropriate: */
  /* o   all players single-player game            */
  /* o   local player only in connected client     */
  /* o   all robot players in server               */
  if ( !ship[i].IsPending(ShipClass::refitting) &&
       !ship[i].IsPending(ShipClass::captured) &&
       !ship[i].IsPending(ShipClass::undocking) &&
       ( IsDisconnectedClient() ||
       ( IsConnectedClient() && (IsLocalPlayer(ship+i) || s && IsLocalPlayer(s)) ) || 
       ( IsServer() && game->IsRobotPlayer(ship+i) ) ) ) {

    /* do not allow forced upgrade of human players */
    if (force && !IsRobotPlayer(ship+i) && playerList[i]->rank >=
	ship[i].design->reqRank &&
	ship[i].design->raceID == playerList[i]->raceId &&
	ship[i].raceID == playerList[i]->raceId )
      return;
    
    /* send dock object index and port number */
    short portNum = 0;
    ShipClass* dockObj = 0;
    if (ship[i].dockObj) {
      dockObj = ship[i].dockObj;
      portNum = ship[i].portNum;
    }
    
    /* only allow refit when docked at multi-port facility and */
    /* fully repaired                                          */
    if (ship[i].dockObj && /* ship[i].dockObj->bus.numPorts > 1 && */
	ship[i].dockObj->bus.factoryMod /* multi-port changed to factory */ &&
	ship[i].bus.aveFunc == module_function_scale ||
	force) {
      
      if (IsRobotPlayer(ship + i) || force) {
	/* robot-controlled ships refit to the highest allowed level */
	short shipIndex = 
	  FindNextShipTypeIndexAtLevel(player[ship[i].playerIndex].raceId, 
				       player[ship[i].playerIndex].shipIndex,
				       player[ship[i].playerIndex].rank,
				       !player[ship[i].playerIndex].IsDemoPlayer());

	/* give credit for current upgrades */
	slong refValue = min(playerList[i]->refValue + 
			    ship[i].TotalValue() - ship[i].design->TotalValue(),
			    max_credit);

	if (refValue < 0) {
	  return;
	}
	

	/* send msg here -- include build flag */
        if ( shipIndex != player[ship[i].playerIndex].shipIndex ) {

	  /* set the event pending flag */
	  ship[i].SetPendingFlag(ShipClass::refitting);
	  
	  retMsgBuffer.PushMsg((uchar)SF_MSG_SETSHIPINDEX,
			       (ushort)i,
			       (ushort)game->ObjectListIndex(dockObj),
			       (ushort)shipIndex,
			       (uchar)build,
			       (uchar)portNum,
			       S_CLIENT_TCP);
	  
	}
      } else { 
	/* player-controlled ships refit interactively */
	short shipIndex = 
	  FindNextShipTypeIndex(ship[i].raceID, 
				player[ship[i].playerIndex].shipIndex,
				player[ship[i].playerIndex].rank,
				!player[ship[i].playerIndex].IsDemoPlayer());

	/* give credit for current upgrades */
	slong refValue = min(playerList[i]->refValue + 
			    ship[i].TotalValue() - ship[i].design->TotalValue(),
			    max_credit);
	
	if (refValue < 0) {
	  
	  sprintf(msg, "1st Officer> We need %s more credit to upgrade.", Format(-refValue));
	  
	  game->PostGameMessage(msg, i, (short)ship[i].raceID, personal);
	  
	  return;
	}
	

	/* set the event pending flag */
	ship[i].SetPendingFlag(ShipClass::refitting);
	
	/* send msg here -- include build flag */
	retMsgBuffer.PushMsg((uchar)SF_MSG_SETSHIPINDEX,
			     (ushort)i,
			     (ushort)game->ObjectListIndex(dockObj),
			     (ushort)shipIndex,
			     (uchar)build,
			     (uchar)portNum,
			     S_CLIENT_TCP);
      }
    }
  }
}

void GameStateClass::RefitShip(short i, short shipIndex, short build,
			       short portNum, ShipClass* dockObj) {

  /* clear the event pending flag */
  ship[i].ClearPendingFlag(ShipClass::refitting);

  if (ship[i].IsExploding() || !ship[i].IsActive() ||
      dockObj != ship[i].dockObj) {
    return;
  }

  WRITE_SF_LOG(logFile << "received> a ");

  WRITE_SF_LOG(ship[i].ShortProfile());
  
  if (dockObj) {

    //ship[i].dockObj = dockObj;
    //ship[i].portNum = portNum;

    WRITE_SF_LOG(logFile << ", docked at port " << portNum << " on a ");
    WRITE_SF_LOG(dockObj->ShortProfile());
    WRITE_SF_LOG(logFile << ",");

  }

  WRITE_SF_LOG(logFile << " has refitted to a ");

  /* give credit for current upgrades */
  slong refValue = min(playerList[i]->refValue + 
		      ship[i].TotalValue() - ship[i].design->TotalValue(),
		      max_credit);

  ship[i].bus.FreeModuleList();
  player[ship[i].playerIndex].shipIndex = shipIndex;
  
  if (build) {
    ship[i].initializeWith(i, ship[i].playerIndex, 1);
    ship[i].stat = DesignedClass::under_construction;
    ship[i].activate();

    ship[i].ShortProfile();

    WRITE_SF_LOG(logFile << " and is currently under construction on frame " <<
      game->frame << endl);

  } else if (ship[i].InitializeFromShipDesign(ship+i)) {
    /* we were unable to allocate memory for the module list   */
    /* so record an error message.  There is no recovery from  */
    /* this condition, but attempt to deactivate this ship and */
    /* hope for the best */
    logFile <<
      "ERROR> unable to allocate memory for new ship on frame " <<
      frame << endl;
    ship[i].deactivate();
  } else {
    ship[i].ShortProfile();
    
    logFile << " on frame " << game->frame << endl;
  }
  
  player[ship[i].playerIndex].shipId = ship[i].design->id;
  MapPlayerShipIdToIndex(ship[i].playerIndex);
  
  if (dockObj && dockObj->IsActive() &&
      dockObj->bus.moduleList[portNum].moduleFunc == ModuleClass::dock) {
    ship[i].dockObj = dockObj;
    ship[i].dockObj->bus.moduleList[portNum].AssignPortToShip(ship+i);
  }

  playerList[i]->SetStat(playerList[i]->refValue,refValue);
  //playerList[i]->refValue = refValue;
}

void GameStateClass::OldRefitShip(short i, short force, short build) {

  /* do not allow forced upgrade of human players */
  if (force && !IsRobotPlayer(ship+i) && playerList[i]->rank >=
      ship[i].design->reqRank &&
      ship[i].design->raceID == playerList[i]->raceId &&
      ship[i].raceID == playerList[i]->raceId )
    return;

  /* only allow refit when docked at multi-port facility and fully repaired */
  if (ship[i].dockObj && /* ship[i].dockObj->bus.numPorts > 1 && */
      ship[i].dockObj->bus.factoryMod /* multi-port changed to factory */ &&
      ship[i].bus.aveFunc == module_function_scale ||
      force) {

    short portNum = 0;
    ShipClass* dockObj = 0;
    if (ship[i].dockObj) {
      dockObj = ship[i].dockObj;
      portNum = ship[i].portNum;
      /* subtract old mass from dock object before refit */
      ship[i].dockObj->mass -= ship[i].mass;
    }

    if (IsRobotPlayer(ship + i) || force) {
      /* robot-controlled ships refit to the highest allowed level */
      ship[i].bus.FreeModuleList();
      player[ship[i].playerIndex].shipIndex = 
	FindNextShipTypeIndexAtLevel(player[ship[i].playerIndex].raceId, 
				     player[ship[i].playerIndex].shipIndex,
				     player[ship[i].playerIndex].rank);

      if (build) {
	ship[i].initializeWith(i, ship[i].playerIndex, 1);
	ship[i].stat = DesignedClass::under_construction;
	ship[i].activate();
      } else if (ship[i].InitializeFromShipDesign(ship+i)) {
	/* we were unable to allocate memory for the module list   */
	/* so record an error message.  There is no recovery from  */
	/* this condition, but attempt to deactivate this ship and */
	/* hope for the best */
	logFile <<
	  "ERROR> unable to allocate memory for new ship on frame " <<
	  frame << endl;
	ship[i].deactivate();
      }

      player[ship[i].playerIndex].shipId = ship[i].design->id;
      MapPlayerShipIdToIndex(ship[i].playerIndex);
  
    } else { 
      /* player-controlled ships refit interactively */

      /* give credit for current upgrades */
      playerList[i]->SetStat(playerList[i]->refValue,
				 min(playerList[i]->refValue +
				     ship[i].TotalValue() - ship[i].design->TotalValue(),
				     max_credit));
      //playerList[i]->refValue = min(playerList[i]->refValue +
      //ship[i].TotalValue() - ship[i].design->TotalValue(),
      //max_credit);

      ship[i].bus.FreeModuleList();
      player[ship[i].playerIndex].shipIndex = 
	FindNextShipTypeIndex(ship[i].raceID, 
			      player[ship[i].playerIndex].shipIndex,
			      player[ship[i].playerIndex].rank);
      if (build) {
	ship[i].initializeWith(i, ship[i].playerIndex, 1);
	ship[i].stat = DesignedClass::under_construction;
	ship[i].activate();
      } else if (ship[i].InitializeFromShipDesign(ship+i)) {
	/* we were unable to allocate memory for the module list   */
	/* so record an error message.  There is no recovery from  */
	/* this condition, but attempt to deactivate this ship and */
	/* hope for the best */
	logFile <<
	  "ERROR> unable to allocate memory for new ship on frame " <<
	  frame << endl;
	ship[i].deactivate();
      }

      player[ship[i].playerIndex].shipId = ship[i].design->id;
      MapPlayerShipIdToIndex(ship[i].playerIndex);

    }

    /* add new mass from dock object after refit */
    if (dockObj  && dockObj->IsActive() &&
      dockObj->bus.moduleList[portNum].moduleFunc == ModuleClass::dock) {
      ship[i].dockObj = dockObj;
      ship[i].dockObj->mass += ship[i].mass;
      ship[i].dockObj->gmass = ship[i].dockObj->mass;
      ship[i].dockObj->bus.moduleList[portNum].AssignPortToShip(ship+i);
    }

    ship[i].ClearPendingFlag(ShipClass::refitting);

  }
}


void GameStateClass::CheckProximityToStars(DesignedClass* d) {
  /* check proximity to sun */

  short i;
  slong dStar, rStar;
  static CoordinateClass r;
  for (i=0; i<numPlanets; i++) {
    if (planet[i].minor == -1) {
      r = planet[i] - *d;
      dStar = r.Mag();
      rStar = 2 * planet[i].bitmap.size * sub_scale;
      if (dStar < rStar) {
	d->Hit(star_prox_check_frames * 100 * (rStar-dStar) / rStar, 0, r, 0,
	       0, DesignedClass::bypass_armor_hit + DesignedClass::bypass_push_hit);
      }
    }
  }
}

void GameStateClass::Defect(short i) {
  /* player defects to original race */
  /* the ship raceID contains the altered raceID */
  /* store the losing race ID */
  slong raceID = ship[i].raceID;

  /* change the ship raceID to the player raceID */
  ship[i].raceID = player[ship[i].playerIndex].raceId;
  
  /* this action requires punishment -- all idle ships attack */
  short j;
  for (j=0; j<numShips; j++) {
    if (ship[j].raceID == raceID && ship[j].AutoResponse() &&
	(ship[j].assign == DesignedClass::manual ||
	 ship[j].assign == DesignedClass::follow_object))
      ship[j].AttackObj(ship+i);
  }
}

void GameStateClass::PlugInModules() {
  /* copy modules from gamestate modulelist into appropriate slots by
     id number */
  short i;
  for ( i = 0; i < numShips; i++) {
    ship[i].PlugInModules();
  }
  for (i = 0; i < numMissiles; i++) {
    missile[i].PlugInModules();
  }
}  

void ShipClass::PlugInModules(short numMod) {
  short j;
  for (j = 0; j < numMod; j++)
    PlugInModuleByDesign(j);
}

slong DesignStruct::TotalValue() {
  short i;
  ModuleClass* m;
  short index;
  slong totval = 0;
  for (i=0; i<bus.numMod; i++) {
    index = game->ValidateModuleID(bus.moduleIdList[i]);
    m = game->moduleList + index;
    if ( m )
      {
	totval += m->value();
      }
  }
  return totval;
}

short DesignStruct::HasFactory() {
  short j,k;
  for (j = 0; j < bus.numMod; j++) {
    k = 0;
    while (k < game->N(GameStateClass::modules) && 
	   bus.moduleIdList[j] != game->moduleList[k].id)
      k++;
    if (k == game->N(GameStateClass::modules)) {
      logFile << "ERROR> invalid module ID " << bus.moduleIdList[j]  << " found in " <<
	game->race[raceID].name << " design " << name << endl;
      return 0;
    } else if (game->moduleList[k].moduleFunc == ModuleClass::factory) 
      return 1;
  }
  return 0;
}

void 
ShipClass::PlugInModuleByID(short j, slong modID)
{
  short k,l;
  for (k = 0; k < game->numModules; k++)
    {
      if (modID == game->moduleList[k].id)
	{
	  bus.moduleList[j] = game->moduleList[k];
	  
	  for (l=0; l < max_module_spar; l++)
	    {
	      bus.moduleList[j].specPar[l] =
		design->bus.moduleParList[j*max_module_spar+l];
	    }
	  
	  if (bus.moduleList[j].moduleFunc == ModuleClass::tube ||
	      bus.moduleList[j].moduleFunc == ModuleClass::beam ||
	      bus.moduleList[j].moduleFunc == ModuleClass::ram ||
	      bus.moduleList[j].moduleFunc == ModuleClass::dock)
	    {
	      /* default radius should be half bitmap size */
	      short nF = bitmap.numFrames;
	      if ( nF > 0 )
		{
		  bus.moduleList[j].specPar[3] = 
		    (bus.moduleList[j].specPar[3] + nF) % nF; 
		  bus.moduleList[j].specPar[4] = 
		    (bus.moduleList[j].specPar[4] + nF) % nF;
		  if ( bus.moduleList[j].specPar[4] < 
		       bus.moduleList[j].specPar[3] )
		    {
		      bus.moduleList[j].specPar[4] += nF;
		    }
		}		  
	      if (!game->DefaultsAreApplied() &&
		  (bus.moduleList[j].moduleFunc == ModuleClass::tube ||
		   bus.moduleList[j].moduleFunc == ModuleClass::ram))
		bus.moduleList[j].SetStat(bus.moduleList[j].par[3],
					  (slong)game->LookUpMissileDesignId(bus.moduleList[j].par[3]));
	      //bus.moduleList[j].par[3] = 
	      //  game->LookUpMissileDesignId(bus.moduleList[j].par[3]);
	      
	    }
	  return;
	}
    }
  
  logFile << "ERROR> PlugInModuleByID: unknown module id: " <<
    modID << " in " << game->race[raceID].name << " " <<
    design->name << " design" << endl;  
}


/*void MissileClass::PlugInModules() {
  short j,k,l;
  for (j = 0; j < design->bus.numMod; j++)
    for (k = 0; k < game->numModules; k++) 
      if (design->bus.moduleIdList[j] == game->moduleList[k].id) {
        bus.moduleList[j] = game->moduleList[k];
        for (l=0; l < max_module_spar; l++)
          bus.moduleList[j].specPar[l] =
            design->bus.moduleParList[j*max_module_spar+l];
      }
}*/

void MissileClass::PlugInModules(short numMod) {
  short j;
  for (j = 0; j < numMod; j++)
    PlugInModuleByDesign(j);
}

void MissileClass::PlugInModuleByID(short j, slong modID) {
  short k,l;
  for (k = 0; k < game->numModules; k++) 
    if (modID == game->moduleList[k].id) {
      bus.moduleList[j] = game->moduleList[k];
      for (l=0; l < max_module_spar; l++)
	bus.moduleList[j].specPar[l] =
	  design->bus.moduleParList[j*max_module_spar+l];
      if (bus.moduleList[j].moduleFunc == ModuleClass::tube ||
	  bus.moduleList[j].moduleFunc == ModuleClass::beam ||
	  bus.moduleList[j].moduleFunc == ModuleClass::ram ||
	  bus.moduleList[j].moduleFunc == ModuleClass::dock) {
	/* default radius should be half bitmap size */
	short nF = bitmap.numFrames;
	bus.moduleList[j].specPar[3] = (bus.moduleList[j].specPar[3] +
					nF) %
	  nF; 
	bus.moduleList[j].specPar[4] = (bus.moduleList[j].specPar[4] +
					nF) %
	  nF;
	if (bus.moduleList[j].specPar[4] < bus.moduleList[j].specPar[3])
	  bus.moduleList[j].specPar[4] += nF;
      }
      return;
    }
  
  logFile << "ERROR> MissileClass::PlugInModuleByID: unknown module id: " <<
    modID << " in " << game->race[raceID].name << " " <<
    design->name << " design" << endl;
}

void
GameStateClass::CreateLoadedModuleList()
{
  short i;
  numLoadedModules = 0;

  for (i = 0; i < numShips; i++)
    {
      numLoadedModules += ship[i].bus.numMod;
    }

  for (i = 0; i < numColonies - numColoniesDestroyed + numColoniesCreated; i++)
    {
      if ( IsValidObjectPointer(colonyList[i]) )
	{
	  numLoadedModules += colonyList[i]->bus.numMod;
	}
    }

  for (i = 0; i < numMissiles - numMissilesDestroyed; i++)
    {
      if ( IsValidObjectPointer(missileList[i]) )
	{
	  numLoadedModules += missileList[i]->bus.numMod;
	}
    }

  loadedModuleList.initializeWith(numLoadedModules);
  
  short j,k = 0;
  for (i = 0; i < numShips; i++)
    {
      for (j = 0; j < ship[i].bus.numMod; j++)
	{
	  loadedModuleList[k] = ship[i].bus.moduleList[j];
	  k++;
	}
    }

  for (i = 0; i < numColonies; i++)
    {
      if ( IsValidObjectPointer(colonyList[i]) )
	{
	  for (j = 0; j < colonyList[i]->bus.numMod; j++)
	    {
	      loadedModuleList[k] = colonyList[i]->bus.moduleList[j];
	      k++;
	    }
	}
    }

  for (i = 0; i < numMissiles; i++)
    {
      if ( IsValidObjectPointer(missileList[i]) )
	{
	  for (j = 0; j < missileList[i]->bus.numMod; j++)
	    {
	      loadedModuleList[k] = missileList[i]->bus.moduleList[j];
	      k++;
	    }
	}
    }
}


void GameStateClass::DisperseLoadedModuleList() {
  short i,j;
  short k = 0;
  for (i = 0; i < numShips; i++) {
    for (j = 0; j < ship[i].bus.numMod; j++) {
      ship[i].bus.moduleList[j] = loadedModuleList[k];
      k++;
      /* remap dock object pointers now that loaded module is installed */
      if ((ship[i].bus.moduleList[j].moduleFunc == ModuleClass::dock ||
	   ship[i].bus.moduleList[j].moduleFunc == ModuleClass::factory) &&
	  ship[i].bus.moduleList[j].workPar[0]) 
	ship[i].bus.moduleList[j].workPar[0] =
	  ObjectListObj((short)ship[i].bus.moduleList[j].workPar[0]-1).objLong;
      }
    ship[i].bus.MapFunctions(ship+i, ship+i);
  }

  for (i = 0; i < numColonies; i++) {
    if ( IsValidObjectPointer(colonyList[i]) )
      {
	for (j = 0; j < colonyList[i]->bus.numMod; j++)
	  {
	    colonyList[i]->bus.moduleList[j] = loadedModuleList[k];
	    k++;
	    /* remap dock object pointers now that loaded module installed */
	    if ( (colonyList[i]->bus.moduleList[j].moduleFunc == 
		  ModuleClass::dock ||
		  colonyList[i]->bus.moduleList[j].moduleFunc ==
		  ModuleClass::factory) &&
		 colonyList[i]->bus.moduleList[j].workPar[0] ) 
	      {
		colonyList[i]->bus.moduleList[j].workPar[0] =
		  ObjectListObj((short)colonyList[i]->bus.moduleList[j].
				workPar[0]-1).objLong;
	      }
	  }
	colonyList[i]->bus.MapFunctions(colonyList[i], colonyList[i]);
      }
  }
  
  for (i = 0; i < numMissiles; i++)
    {
      if ( IsValidObjectPointer(missileList[i]) )
	{
	  for (j = 0; j < missileList[i]->bus.numMod; j++) {
	    missileList[i]->bus.moduleList[j] = loadedModuleList[k];
	    k++;
	  }
	  missileList[i]->bus.MapFunctions(missileList[i], missileList[i]);
	}
    }
}

short GameStateClass::LookUpID(ColonyDesignStruct* d) {
  short i;
  for (i=0; i < numColonyDesigns; i++)
    if (colonyDesign[i] == *d)
      return i;

  return -1; /* no match found */
}

short GameStateClass::LookUpMissileDesignId(slong id) {
  /* find missile id's in GameState missile design list */
  short l = 0;
  while (missileDesign[l].id != id && l < numMissileDesigns)
    l++;
  if (missileDesign[l].id == id) return l;
  else return -1;
}

short GameStateClass::NewObject(MissileClass** m)
{
  /* add new object to the game -- returns pointer to first available
     spot. */
  
  short i = 0;
  while (i < maxMissiles && missile[i].active)
    {
      i++;
    }
  if (i == maxMissiles)
    {
      m = 0;
      return -1;
    }
  /*
  else
    {
      objectList[numMissiles+missileObjectIndexStart] =
	missileList[numMissiles] = *m = &missile[i];
      numMissiles++;
      return i;
    }
  */
  else if ( numMissiles < maxMissiles )
    {
      objectList[numMissiles+missileObjectIndexStart] = 
	missileList[numMissiles] = *m = &missile[i];
      numMissiles++;
      return i;
    }
  return -1;
}

short GameStateClass::NewObject(ColonyClass** c, short newSlot) {
  /* add new object to the game -- returns pointer to first available
     spot. */

  if (newSlot != -1) {

    /******************************************************************/
    /* if newSlot is given then this is a client and the server has   */
    /* already determined the slot number.  Because of timing it is   */
    /* possible that the spawn event for a slot precedes the call to  */
    /* DestroyObject for that slot because it did not have time to    */
    /* complete its explosion sequennce.  We can fix this by checking */
    /* for this condition and calling DestroyObject, if necessary     */
    /******************************************************************/

    if (colony[newSlot].active) {

      logFile << "note> a ";
      colony[newSlot].ShortProfile();
      logFile << " was still ";

      if (colony[newSlot].IsExploding())
	logFile << "exploding";
      else
	logFile << "active";

      logFile << " when a request for its slot was received on frame " << frame <<
	endl;

      ReceiveDestroyObjectMsg(colony + newSlot, 1);
    
    }
    /* do not insert into object list until start of next frame */
    /* because it will displace the missiles                    */
    /* objectList[i+colonyObjectIndexStart] = */
    /* colonyList[numColonies] = */
    *c = colony + newSlot;
    numColoniesCreated++;

    logFile << "note> after spawning requested slot " << newSlot << " on frame " << frame <<
      ", numColonies = " << numColonies << " numColoniesCreated = " << numColoniesCreated <<
      " numColoniesDestroyed = " << numColoniesDestroyed << endl;

    return newSlot;

  } else {
    short i = 0;
    while (i < maxColonies && colony[i].active)
      i++;
    if (i == maxColonies) {
      c = 0;
    
      logFile << "note> unable to spawn new colony (numColonies = " << numColonies << ") on frame " <<
	frame << endl;

      return -1;
    } else {
      /* do not insert into object list until start of next frame */
      /* because it will displace the missiles                    */
      /* objectList[i+colonyObjectIndexStart] = */
      /* colonyList[numColonies] = */
      *c = &colony[i];
      numColoniesCreated++;

      logFile << "note> after spawning first available slot " << i << " on frame " << frame <<
	", numColonies = " << numColonies << " numColoniesCreated = " << numColoniesCreated <<
	" numColoniesDestroyed = " << numColoniesDestroyed << endl;

      return i;
    }
  }
}
  
short GameStateClass::MapPlayerShipIdToIndex(short pI) {
  /* convert the shipId value in game->player[pI].shipId to index value
     in game->shipDesign list */
  short is = 0;
  for (is=0; is<numShipDesigns; is++) {
    if (shipDesign[is].id == player[pI].shipId) {
      player[pI].shipIndex = is;
      return 1;
    }
  }

  player[pI].shipIndex = 0;
  return 0;
}
  
GameBitmapClass::GameBitmapClass(SequenceType st, short i, short n, 
                                 short s, char* cl, char* nm, char* fi) :
                                 ObjectBitmapClass(st, i, n, s)
{
  strcpy(Class,cl);
  strcpy(name,nm);
  strcpy(file,fi);
}

//  short GameBitmapClass::get(FILE* f) { /* obsolete */
//    char seqT[max_name_char];
//    int done = 
//      fscanf(f,"%s%s%d%d%s%d",Class, name, size, numFrames, seqT, index);
//    if ( strcmp(seqT,"spin")==0 )
//      seq = ObjectBitmapClass::spin;
//    else if ( strcmp(seqT,"time")==0 )
//      seq = ObjectBitmapClass::time;
//    else if ( strcmp(seqT,"stationary")==0 )
//      seq = ObjectBitmapClass::stationary;
//    else if ( strcmp(seqT,"terminal")==0 )
//      seq = ObjectBitmapClass::terminal;
//    else if ( strcmp(seqT,"transitional")==0 )
//      seq = ObjectBitmapClass::transitional;
//    return (done > 1);
//  }

short GameBitmapClass::get(ifstream& f,
			   SFString* nameList, SFString* valueList,
			   short nList) {
  static char seqT[max_name_char];
  char c;

  /* pattern: */
  /* Torian   Scout       v-sc    36   32   spin   10 100 */

  /* the class name would correspond to the type of object: */
  /*    ship                                                */
  /*    planet                                              */
  /*    missile                                             */
  /*    colony                                              */
  /* the name would correspond to the name of the object:   */
  /*    scout                                               */
  /*    destroyer                                           */
  /*    mars                                                */

  f >> c;
  if (c != '}') {
    f.putback(c);
    f >> Class >> raceName >> name >> file >> numFrames >> size >> seqT >> alpha >>
      gamma;

    if (nameList && valueList && nList) {
     
      Parse(Class, nameList, valueList, nList);
      Parse(raceName, nameList, valueList, nList);
      Parse(name, nameList, valueList, nList);
      Parse(file, nameList, valueList, nList);

    }

    if ( strcmp(seqT,"spin")==0 )
      seq = ObjectBitmapClass::spin;
    else if ( strcmp(seqT,"time")==0 )
      seq = ObjectBitmapClass::time;
    else if ( strcmp(seqT,"stationary")==0 )
      seq = ObjectBitmapClass::stationary;
    else if ( strcmp(seqT,"terminal")==0 )
      seq = ObjectBitmapClass::terminal;
    else if ( strcmp(seqT,"transitional")==0 )
      seq = ObjectBitmapClass::transitional;

    init();

    return (f.good());
  }

  return (c != '}' && f.good());
}

//long CoordinateClass::Mag(long range) const {
  /* return the magnitude of a Coordinate Class variable */
  /*   if the distance is less than the critical distance dist_crit,
       the magnitude is approximated by the max function, otherwise
       the sqrt is returned. */

//long maxXY = max(labs(x),labs(y));
// if (maxXY > range) {
//   return maxXY;
// } else {
//   double sx = double(x)/sub_scale;
//   double sy = double(y)/sub_scale;
//   return nlong(sqrt(sx*sx + sy*sy) * sub_scale);
// }
//}

short CoordinateClass::AddSubjectToLimit(const CoordinateClass& c, 
                                                   slong lim) {
  static CoordinateClass cp;
  cp = *this + c;
  slong cpMag = cp.Mag();
  if (cpMag <= lim) {
    *this = cp;
    return 1;
  } else {
    *this = cp;
    *this *= lim / (cpMag+1.0); 
    return 0;
  }
}

slong CoordinateClass::PushTo(class GameStateBuffer& gb) {
  char* bufferPos = gb.BufferPosition();
  CALL_PUSH_OBJ(gb,x);
  CALL_PUSH_OBJ(gb,y);
  return gb.BufferPosition() - bufferPos;
}

slong CoordinateClass::PopFrom(class GameStateBuffer& gb) {
  char* bufferPos = gb.BufferPosition();
  CALL_POP_OBJ(gb,x);
  CALL_POP_OBJ(gb,y);
  return gb.BufferPosition() - bufferPos;
}

void CoordinateClass::print(ostream& out) {
  out << "( " << x << " , " << y << " )" << endl;
}

void PingPacket::_initialize() {  /* default constructor */

    /* set the distance to the largest possible long */
    dist = pc_long_max;
    //disp = nullVector;
    //vel = nullVector;
    obj.generic = 0;
    if (game)
      frame = game->frame;
    else
      frame = 0;
}

PingPacket::PingPacket(slong dst, CoordinateClass dsp, CoordinateClass v, 
			      GenericClass* g) : disp(dsp), vel(v) {
  //dist = dst;
  //disp = dsp;
  vel = v;
  obj.generic = g;
  frame = game->frame;
}

PingPacket::PingPacket(GenericClass* self,
		       GenericClass* target,
		       slong range, 
		       CoordinateClass& lead)
{
  obj.generic = NULL;
  if ( IsValidObjectPointer(target) && self->check == (int)0xAAAAAAAA )
    {
      disp = *target - *self + lead;
      vel = target->tvel - self->tvel;
      obj.generic = target;
    }
  dist = disp.Mag(range);
  frame = game->frame;
}


slong PingPacket::PushTo(class GameStateBuffer& gb, GameStateClass* g) {
  
  char* bufferPos = gb.BufferPosition();

  CALL_PUSH_OBJ(gb,frame);
  CALL_PUSH_OBJ(gb,dist);
  CALL_PUSH_OBJ(gb,disp);
  CALL_PUSH_OBJ(gb,vel);

  if ( !IsValidObjectPointer(obj.generic,__FILE__,__LINE__) )
    {
      obj.generic = 0;
    }
  CALL_PUSH_GAME_OBJ(gb, g,obj.generic);
  
  return gb.BufferPosition() - bufferPos;
}

slong PingPacket::PopFrom(class GameStateBuffer& gb, GameStateClass* g) {
  char* bufferPos = gb.BufferPosition();

  CALL_POP_OBJ(gb,frame);
  CALL_POP_OBJ(gb,dist);
  CALL_POP_OBJ(gb,disp);
  CALL_POP_OBJ(gb,vel);
  CALL_POP_GAME_OBJ(gb, g,obj.generic);

  return gb.BufferPosition() - bufferPos;
}

PingPacket PingPacket::Sniff(GenericClass* self,
			     GenericClass* target, slong range) {
  /* if the distance between target and self is less than current distance,
     replace current contents with smaller ping packet.  Return target-self
     packet. */
  PingPacket pp(self, target, range);
  if ( IsValidObjectPointer(obj.generic) && IsValidObjectPointer(target) )
    return Sniff(pp, target->bitmap.size*sub_scale/2 -
		 obj.generic->bitmap.size*sub_scale/2);

  *this = pp;

  return pp;
}

PingPacket& PingPacket::Sniff(PingPacket& pp, slong lead) {

  /* lead can be negative, so cast dist to a long */
  if (lead >= 0) {
    if (pp.dist < dist + (ulong) lead) {
      *this = pp;
    }
  } else if (slong(pp.dist) < slong(dist) + lead) {
    *this = pp;
  }
  return pp;
}

void GenericClass::initialize() {
  /* generic constructor */

  check = 0xAAAAAAAA;

  gframe = -1;
  gfresh = FALSE;
  gnow = nullVector;
  gvel = nullVector;

  /* reset frame counter for new object */
  frame = 0;
  nextGravityFrame = 0;
  bitmapStartFrame = 0;

  /* initialize mass */
  mass = 0;
  gmass = 0;

  offset = 0;

  /* initialize all others to zero */
  vel = nullVector;
  bitmap = NoObjectBitmap;
  lastBitmap = NoObjectBitmap;
  originalBitmapIndex = NoObjectBitmapIndex;
  parent.generic = 0;
  numPings = 0;
  
  /* assume cycle rate equals frame rate */
  cycle = 1;

  /* object is visible */
  vis = 1;

  /* no sound */
  currentSound = -1;

  /* neutral race */
  raceID = -1;
  active = 0;
};

PlanetClass::PlanetClass() : GenericClass() {

  typeOfObject = planet;

  colony = 0;
  revertRaceIndex = -1;
  revertColonyDesignIndex = 0;
  accBonus = 0;
  grav = 0;
  atmP = 0;
  terraForm = 0;

  enemyOrbiting = 0;

  /* a raceID of -1 signifies a neutral planet */
  raceID = -1;
};

void DesignedClass::initialize() {

  GenericClass::initialize();

  typeOfObject = designed;

  sensorThreshold = 1;
  stealth = 1;
  thrustOffset = 0;
  heading = 0;
  seekHeading = 0;
  turning = 0;
  seekTurning = 0;
  throttle = 0;
  viewObj.generic = 0;
  lockObj.generic = 0;
  mode = 0;
  targetPP.obj.generic = 0;
  transitSlide = 0;
  lastAttacker.generic = 0;
  ClearAssign();
  assignOverRide = 0;
  stat = 0;
  assignPar1 = 0;
  assignPar2 = 0;
  duty = none;
  dutyPar = 0;
  repairRate = 0;
  rotRate = default_rotation_rate;
  firingTubes = 0;
  numLagFrames = 0;
}

void GenericClass::initializeWith(const GenericClass& g, 
				  ObjectBitmapClass& objectBitmap, slong v) {
  /* copy constructor with bitmap replacement and spatial offset --
     for use with missiles: spawn copy most generic information to the
     generic portion of missile class and resets the bitmap and offset
     the initial position so missile does no come out of middle */

  typeOfObject = generic;

  check = 0xAAAAAAAA;

  gframe = -1;
  gfresh = FALSE;
  gnow = nullVector;
  gvel = nullVector;

  /* initialize mass */
  mass = 0;
  gmass = 0;

  vel = g.tvel;
  now = vel*dt + g.now;
  frame = 0;
  nextGravityFrame = 0;
  bitmapStartFrame = 0;
  bitmap = objectBitmap;
  lastBitmap = NoObjectBitmap;
  originalBitmapIndex = objectBitmap.index;
  numPings = 0;
  offset = 0;

  /* assume cycle rate equals frame rate */
  cycle = 1;
  /* object is visible */
  vis = 1;

  /* no sound */
  currentSound = -1;

  /* neutral race */
  raceID = -1;

  active = 0;
}

void DesignedClass::initializeWith(const DesignedClass& g, 
				   ObjectBitmapClass& objectBitmap, slong v,
				   slong* par) {

  /* copy constructor with bitmap replacement and spatial offset --
     for use with missiles: spawn copy most generic information to the
     generic portion of missile class and resets the bitmap and offset
     the initial position so missile does no come out of middle */

  GenericClass::initializeWith(g, objectBitmap, v);

  typeOfObject = designed;

  sensorThreshold = 1;
  stealth = 1;
  heading = char(g.heading * bitmap.numFrames / g.bitmap.numFrames);
  if (bitmap.IsSpin())
    offset = heading;
  else
    offset = 0;
  turning = g.turning;
  seekTurning = 0;
  if (par) /* debris will have no par pointer */
    Displace(g, v, par);
  thrustOffset = 0;
  throttle = 0;
  viewObj.generic = 0;
  lockObj.generic = 0;
  mode = 0;
  targetPP.obj.generic = 0;
  transitSlide = 0;
  lastAttacker.generic = 0;
  ClearAssign();
  assignOverRide = 0;
  stat = 0;
  assignPar1 = 0;
  assignPar2 = 0;
  duty = none;
  dutyPar = 0;
  repairRate = 0;
  rotRate = default_rotation_rate;
  firingTubes = 0;
  numLagFrames = 0;
};

void GenericClass::initializeWith(short i, ObjectBitmapClass& objectBitmap) {
  /* pre-network-transfer constructor */

  typeOfObject = generic;

  check = 0xAAAAAAAA;

  gframe = -1;
  gfresh = FALSE;
  gnow = nullVector;
  gvel = nullVector;

  frame = 0;
  now = nullVector;
  vel = nullVector;
  bitmap = objectBitmap;
  lastBitmap = NoObjectBitmap;
  originalBitmapIndex = objectBitmap.index;
  offset = 0;
  parent.generic = 0;
  numPings = 0;
  /* assume cycle rate equals frame rate */
  cycle = 1;
  /* object is visible */
  vis = 1;

  /* initialize mass */
  mass = 0;
  gmass = 0;

  /* no sound */
  currentSound = -1;

  /* neutral race */
  raceID = -1;

  active = 0;
}

void DesignedClass::initializeWith(short i, ObjectBitmapClass& objectBitmap) {

  GenericClass::initializeWith(i, objectBitmap);
  typeOfObject = designed;

  sensorThreshold = 1;
  stealth = 1;
  thrustOffset = 0;
  heading = 0;
  seekHeading = 0;
  turning = 0;
  seekTurning = 0;
  throttle = 0;
  viewObj.generic = 0;
  lockObj.generic = 0;
  stat = idle;
  duty = none;
  offset = 0;
  mode = mode_auto;
  targetPP.obj.generic = 0;
  transitSlide = 0;
  lastAttacker.generic = 0;
  assignPar1 = 0;
  assignPar2 = 0;
  dutyPar = 0;
  assign = manual;
  repairRate = 0;
  rotRate = default_rotation_rate;
  firingTubes = 0;
  numLagFrames = 0;
  ClearAssign();
}

void DesignedClass::SetMode( short m, short op) {

  if (op == op_on)
    mode = mode | m;
  else if (op == op_off)
    mode = mode ^ (mode & m);
  else if (op == op_toggle)
    mode = mode ^ m;

}


slong DesignedClass::IsValidDesignedObjectPointer() {
  return (this >= game->ship.rootOf() && this < game->ship.rootOf() + game->numShips ||
	  this >= game->colony.rootOf() && this < game->colony.rootOf() + game->maxColonies ||
	  this >= game->missile.rootOf() && this < game->missile.rootOf() + game->maxMissiles);
}
 
slong DesignedClass::IsValidDesignedObject(short i, char* location) {
  short j;
  slong error, vftError = 0;
  char objType[max_name_char];

  /******************************************/
  /* (0) check that this is a valid pointer */
  /******************************************/
  if (!IsValidDesignedObjectPointer()) {
    logFile << "ERROR> invalid object pointer (" << this << ") found for object " <<
      i << " at " << location << endl;
    return 0;
  }

  /******************************************************************/
  /* (1) check the virtual function table for signs of corruption   */
  /******************************************************************/

  slong* vft = AddressOfVirtualFunctionTable();

  if (this >= (DesignedClass*)game->ship.rootOf() &&
      game->ship.indexOf((ShipClass*)this) < game->numShips) {
    ShipClass refShip;
    if (vft && *vft != *refShip.AddressOfVirtualFunctionTable()) {
      strcpy(objType, "ship");
      vftError = 1;
      j = game->ship.indexOf((ShipClass*)this);

      *AddressOfVirtualFunctionTable() = *refShip.AddressOfVirtualFunctionTable();
    }
  } else if (this >= (DesignedClass*)game->colony.rootOf() &&
	     game->colony.indexOf((ColonyClass*)this) < game->maxColonies) {
    ColonyClass refColony;
    if (vft && *vft != *refColony.AddressOfVirtualFunctionTable()) {
      strcpy(objType, "colony");
      vftError = 1;
      j = game->colony.indexOf((ColonyClass*)this);

      *AddressOfVirtualFunctionTable() = *refColony.AddressOfVirtualFunctionTable();
    }
  } else if (this >= (DesignedClass*)game->missile.rootOf() &&
	     game->missile.indexOf((MissileClass*)this) < game->maxMissiles) {
    MissileClass refMissile;
    if (vft && *vft != *refMissile.AddressOfVirtualFunctionTable()) {
      strcpy(objType, "missile");
      vftError = 1;
      j = game->missile.indexOf((MissileClass*)this);

      *AddressOfVirtualFunctionTable() = *refMissile.AddressOfVirtualFunctionTable();
    }
  }

  if (vft && vftError) {
    logFile << "ERROR> " << objType << " " << j << " (" << i << ") has a bad virtual function table (" << *vft <<
      ") at " << location << " on frame " << game->frame << endl;
    return 0;
  }

  /******************************************************************/
  /* (2) check the module function pointers for signs of corruption */
  /******************************************************************/

  if ((error = HasValidModulePointers(j)) != 0) {
    logFile << "ERROR> a ";
    ShortProfile();
    
    /* log the problem... */
    switch (error) {
    case no_error:
      return 1;
    case module_fire_corrupt:
      logFile << " has a bad fire pointer (" << *((slong*)(&bus.moduleList[j].fire)) <<
	") in module " << j << " (" <<
	bus.moduleList[j].name << ") on frame " << game->frame << endl;
      break;
    case module_deposit_corrupt:
      logFile << " has a bad deposit pointer (" << *((slong*)(&bus.moduleList[j].deposit)) <<
	") in module " << j << " (" <<
	bus.moduleList[j].name << ") on frame " << game->frame << endl;
      break;
    case module_withdraw_corrupt:
      logFile << " has a bad withdraw pointer (" << *((slong*)(&bus.moduleList[j].withdraw)) <<
	") in module " << j << " (" <<
	bus.moduleList[j].name << ") on frame " << game->frame << endl;
      break;
    case module_storage_corrupt:
      logFile << " has a bad storage pointer (" << *((slong*)(&bus.moduleList[j].storage)) <<
	") in module " << j << " (" <<
	bus.moduleList[j].name << ") on frame " << game->frame << endl;
      break;
    case module_requirement_corrupt:
      logFile << " has a bad requirement pointer (" << *((slong*)(&bus.moduleList[j].requirement)) <<
	") in module " << j << " (" <<
	bus.moduleList[j].name << ") on frame " << game->frame << endl;
      break;
    }
    
    /* correct the problem... */
    bus.RemapFunctions(this,this, "detection of corruption");
    
    return 0;
  }
  return 1;
}

short DesignedClass::SeekHeading(const CoordinateClass& c) {
  /* return +/- direction to heading for target object */
  slong Atan2(slong,slong);
  RotationType newHeading = 0;

  seekTurning = 0;
  if (c.x || c.y) {

    /* newHeading = RotationType( (Atan2(c.y,c.x) + 270) * 
                    bitmap.numFrames/360 ) % bitmap.numFrames; */
    newHeading = c.Heading(bitmap.numFrames);

/*    newHeading = RotationType(nlong( (atan2(c.y,c.x)/(2*pi)+1.75) * 
                    bitmap.numFrames)) % bitmap.numFrames;*/

    if (newHeading == heading )
      seekTurning = 0;
    else if ((newHeading < heading && heading-newHeading < 
              bitmap.numFrames/2) || 
             (newHeading>heading && heading-newHeading+
              bitmap.numFrames < bitmap.numFrames/2))
      seekTurning = -1;
    else
      seekTurning = 1;    
  }
  return ((heading + seekTurning) % bitmap.numFrames) == newHeading;
}

short DesignedClass::SeekHeading(const slong l) {
  /* return +/- direction to heading for target object */
  RotationType newHeading;
  
  seekTurning = 0;
  newHeading = (RotationType)((l + bitmap.numFrames) % bitmap.numFrames);
  
  if (newHeading == heading )
    seekTurning = 0;
  else if ((newHeading < heading && heading-newHeading < 
	    bitmap.numFrames/2) || 
	   (newHeading>heading && heading-newHeading+
	    bitmap.numFrames < bitmap.numFrames/2))
    seekTurning = -1;
  else
    seekTurning = 1;    
  return ((heading + seekTurning) % bitmap.numFrames) == newHeading;
}

void DesignedClass::ReceiveTerminateWarpMsg(CoordinateClass& v) {
  
  /* this initiates the end transition sequence and ends warp movements */
  /* the object is left with the velocity v, so it can match velocity   */
  /* with its target                                                    */

  stat = exiting_warp;
  bitmapStartFrame = frame;
  vel = v;
  tvel = vel;
  seekTurning = 0;
  offset = heading;
  bitmap = game->objectBitmapList[game->transitionBitmapIndexStart+1];

  if (sf && sf->mwl && game->soundEvent[warping_event] != -1)
    sf->audio->Stop(sf->mwl->Wave(game->soundEvent[warping_event]));
  
  if (bus.warpMod)
    PlaySoundOnScreen(bus.warpMod->deactivationSoundIndex); /* 15 */
  /* object is visible again */
  vis = 1;
  //bus.warpMod->deactivate();

}

void DesignedClass::SendTerminateWarpMsg(CoordinateClass& v) {
  
  /* this initiates the end transition sequence and ends warp movements */
  /* the object is left with the velocity v, so it can match velocity   */
  /* with its target                                                    */

  if (!IsWarping())
    return;

  /* send a message when appropriate */
  if (Type() == ship) {

    ShipClass* s = (ShipClass*) this;

    if (IsDisconnectedClient() ||
	 (IsConnectedClient() && IsLocalPlayer(s)) ||
	(IsServer() && game->IsRobotPlayer(s))) {

      ushort aiTargetIndex;
      
      if ((assign == follow_object || assign == attack) &&
	  IsValidObjectPointer(targetPP.obj.generic, __FILE__, __LINE__))
	aiTargetIndex = ushort(game->ObjectListIndex(targetPP.obj.generic));
      else
	aiTargetIndex = ushort(assignPar2);
      
      retMsgBuffer.PushMsg((uchar)SF_MSG_TERMINATEWARP,
			   (ushort)game->ObjectListIndex(this),
			   (ushort)aiTargetIndex,
			   (uchar) assign,
			   now.x,
			   now.y,
			   (int)S_CLIENT_TCP);
    } else
      return;
  } else if (Type() == colony) {
    
    if (IsDisconnectedClient() || IsServer()) {
     
      ushort aiTargetIndex;
      
      if ((assign == follow_object || assign == attack) &&
	  IsValidObjectPointer(targetPP.obj.generic, __FILE__, __LINE__))
	aiTargetIndex = ushort(game->ObjectListIndex(targetPP.obj.generic));
      else
	aiTargetIndex = ushort(assignPar2);
 
      retMsgBuffer.PushMsg((uchar)SF_MSG_TERMINATEWARP,
			   (ushort)game->ObjectListIndex(this),
			   (ushort)aiTargetIndex,
			   (uchar) assign,
			   now.x,
			   now.y,
			   (int)S_CLIENT_TCP);

    } else
      return;
  }

  WRITE_SF_LOG(logFile << "sent> a ");
  WRITE_SF_LOG(ShortProfile()); 
  WRITE_SF_LOG(logFile << " is terminating warp on frame " << game->frame << endl);

  ReceiveTerminateWarpMsg();
	
}

void DesignedClass::CheckTransition(CoordinateClass& v,
					   short includeTransition) {
  
  /* this initiates the end transition sequence and ends warp movements */
  /* the object is left with the velocity v, so it can match velocity   */
  /* with its target                                                    */
  
  if (IsWarping(includeTransition) || vis == 0) {
    SendTerminateWarpMsg(nullVector);
  }
}

void DesignedClass::SendInitiateWarpMsg() {

  /* start the warp startup transition sequence             */
  /* use CheckTransition to start the matching end sequence */

  if (IsWarping())
    return;

  unsigned short aiTargetIndex;

  /* send a message when appropriate */
  if (Type() == ship) {

    ShipClass* s = (ShipClass*) this;

    if (IsDisconnectedClient() ||
	 (IsConnectedClient() && IsLocalPlayer(s)) ||
	(IsServer() && game->IsRobotPlayer(s))) {

      if ((assign == follow_object || assign == attack) &&
	  IsValidObjectPointer(targetPP.obj.generic, __FILE__, __LINE__)) {
	if (targetPP.obj.generic->IsActive())
	  aiTargetIndex = ushort(game->ObjectListIndex(targetPP.obj.generic));
	else {
	  aiTargetIndex = 0;
	  logFile << "ERROR> INITIATEWARP sending inactive target index (" <<
	    game->ObjectListIndex(targetPP.obj.generic) << ") on frame " <<
	    game->frame << endl;
	}
      } else
	aiTargetIndex = ushort(assignPar2);
      
      retMsgBuffer.PushMsg((uchar)SF_MSG_INITIATEWARP,
			   (ushort)game->ObjectListIndex(this),
			   (ushort)aiTargetIndex,
			   (uchar) assign,
			   (int)S_CLIENT_TCP);
    } else
      return;

  } else if (Type() == colony) {
    
    if (IsDisconnectedClient() || IsServer()) {
     
      if ((assign == follow_object || assign == attack) &&
	  IsValidObjectPointer(targetPP.obj.generic, __FILE__, __LINE__))
	aiTargetIndex = ushort(game->ObjectListIndex(targetPP.obj.generic));
      else
	aiTargetIndex = ushort(assignPar2);
 
      retMsgBuffer.PushMsg((uchar)SF_MSG_INITIATEWARP,
			   (ushort)game->ObjectListIndex(this),
			   (ushort)aiTargetIndex,
			   (uchar) assign,
			   (int)S_CLIENT_TCP);

    } else
      return;
  }

  WRITE_SF_LOG(logFile << "sent> a ");
  WRITE_SF_LOG(ShortProfile()); 
  WRITE_SF_LOG(logFile << " is initiating a warp jump with the target a ");
  WRITE_SF_LOG(game->objectList[(short)aiTargetIndex]->ShortProfile());
  WRITE_SF_LOG(logFile << " on frame " << game->frame << endl);
  
  ReceiveInitiateWarpMsg();
    
}

void DesignedClass::ReceiveInitiateWarpMsg() {

  /* start the warp startup transition sequence             */
  /* use CheckTransition to start the matching end sequence */

  bitmapStartFrame = frame;
  offset = 0;
  stat = entering_warp;
  vel = nullVector;
  tvel = vel;
  seekTurning = 0;
  bitmap = game->objectBitmapList[game->transitionBitmapIndexStart];

  if (bus.warpMod)
    PlaySoundOnScreen(bus.warpMod->activationSoundIndex);
  //bus.warpMod->activate();

}

void DesignedClass::WarpTransition() {

  /* start the warp startup transition sequence             */
  /* use CheckTransition to start the matching end sequence */

  SendInitiateWarpMsg();

}

void ShipClass::SwapModuleByType(short searchDir) {

  /*******************************************************************/
  /* if the ship is docked at a ship with a factory then allow it to */
  /* swap out the module at [modptr] for another of the same type of */
  /* equal or lesser value (refValue)                                */
  /*******************************************************************/

  if (dockObj && dockObj->bus.factoryMod) {

    short i,ii,j;

    ModuleClass* m = bus.moduleList + bus.modPtr;
    slong ov = 0, v;
    if ( m )
      {
	ov = m->value();
      }
    short mindex = game->ValidateModuleID(m->id);

    game->player[playerIndex].SetStat(game->player[playerIndex].refValue,
					  min(game->player[playerIndex].refValue + ov, max_credit));

    //game->player[playerIndex].refValue = min(game->player[playerIndex].refValue + ov, max_credit);
      
    ii = mindex;
    j=0;
    while(j < game->numModules) {
      
      i = (ii + game->numModules) % game->numModules;
      
      if (game->moduleList[i].IsEquivalentTo(m) &&
	  !game->player[playerIndex].IsRestrictedFromUsing(game->moduleList + i) &&
	  i != mindex &&
	  Volume() - m->volume + game->moduleList[i].volume <=
	  design->volumeCap &&
	  game->tech[raceID].IsSet(i) &&
	  (v = game->moduleList[i].value()) <= (slong)game->player[playerIndex].refValue) {
	
	/* swap the modules */
	ModuleClass mo = game->moduleList[i];

	/***************************************/
	/* copy the existing module parameters */
	/* this preserves the firing angles    */
	/***************************************/

	for (j=0; j < m->nSPar; j++)
	  mo.specPar[j] = m->specPar[j];

	*m = mo;
	m->host.ship = this;
	bus.RemapFunctions(this, this, "a module swap/upgrade");

        game->player[playerIndex].SetStat(game->player[playerIndex].refValue,
					      max(game->player[playerIndex].refValue - v, 0));
	//game->player[playerIndex].refValue = max(game->player[playerIndex].refValue - v, 0);

	SendGetPowerUpMsg(bus.modPtr, bus.moduleList[bus.modPtr].id,
			  game->player[playerIndex].refValue);

	return;
      }    
      
      ii += searchDir;
      j++;
    }

    game->player[playerIndex].SetStat(game->player[playerIndex].refValue,
					  max(game->player[playerIndex].refValue - ov, 0));
    //game->player[playerIndex].refValue = max(game->player[playerIndex].refValue - ov, 0);

  }
}

void ShipClass::DropModule(char n) {

  static char msg[128];

  if (bus.moduleList[n].moduleFunc == ModuleClass::tube ||
      bus.moduleList[n].moduleFunc == ModuleClass::beam ||
      bus.moduleList[n].moduleFunc == ModuleClass::ram ||
      bus.moduleList[n].moduleFunc == ModuleClass::power_plant ||
      bus.moduleList[n].moduleFunc == ModuleClass::engine)
    /* do not allow certain module types to be dropped */
    return;

  /*********************************************************/
  /* if the module to be dropped is a dock and there is an */
  /* object docked at it then prevent the drop operation   */
  /*********************************************************/
  if (bus.moduleList[n].moduleFunc == ModuleClass::dock &&
      bus.moduleList[n].workPar[1]) {
    if (game->ObjectListIndex(this) == localSlotNumber) {
      strcpy(msg, "1st Officer> we can't drop the dock while it's in use.");
      game->PostGameMessage(msg, game->ship.indexOf(this), (short)raceID, personal);
    }
    return;
  }

  /* prevent active modules from being dropped */
  if (bus.moduleList[n].IsActive()) {
    if (game->ObjectListIndex(this) == localSlotNumber) {
      strcpy(msg, "1st Officer> we can't drop a system while it's in use.");
      game->PostGameMessage(msg, game->ship.indexOf(this), (short)raceID, personal);
    }
    return;
  }

  /* clear the single module pointers    */
  /* the MapFunctions call will reassign */
  //bus.ClearSingleModuleMaps();

  static slong par[3] = {0,0,0};

  /* log the drop event */
  logFile << "note> a ";
  ShortProfile();
  logFile << " has dropped a " << funcName[bus.moduleList[n].moduleFunc] << " from slot " <<
    short(n) << " on frame " << game->frame << endl;

  /* if the ship is docked then give credit and don't create power up */
  if (dockObj && dockObj->bus.factoryMod)
    {
      game->player[playerIndex].SetStat(game->player[playerIndex].refValue,
					    min(game->player[playerIndex].refValue +
						bus.moduleList[n].value(), max_credit));
      //game->player[playerIndex].refValue = min(game->player[playerIndex].refValue +
      //				     bus.moduleList[n].value(), max_credit);
    }
  else
    /* create a plugin near the ship */
    Spawn(game->missileDesign[0], par,
	  bus.moduleList[n].id);

  /* if a stealth system was dropped then reset the cloak attributes */
  if (bus.moduleList[n].moduleFunc == ModuleClass::cloak) {
    stealth = 1;
    stealthLevel = 0;
  }

  short i;
  for (i=short(n)+1; i < bus.numMod; i++) {
    bus.moduleList[i-1] = bus.moduleList[i];
  }

  bus.numMod--;

  if (bus.numMod)
    bus.modPtr = bus.modPtr % bus.numMod;

  bus.RemapFunctions(this, this, "a dropped module");

}

void ShipClass::ToggleModuleMode() {
  if (bus.moduleList.IsNotNull()) {
    
    bus.moduleList[bus.modPtr].Lock();

    if (bus.moduleList[bus.modPtr].mode == ModuleClass::locked)
      game->PostGameMessage("system is locked",
			    game->ship.indexOf(this), (short)raceID, personal);
    else if (bus.moduleList[bus.modPtr].mode == ModuleClass::free)
      game->PostGameMessage("system is unlocked",
			    game->ship.indexOf(this), (short)raceID, personal);
    else
      game->PostGameMessage("system is on standby",
			    game->ship.indexOf(this), (short)raceID, personal);

  }
}

short ShipClass::FindModuleToReplace(ModuleClass& m,
				     short& typeMatched,
				     short& volumeExceeded,
				     short checkVolume,
				     short checkValue,
				     short checkStatus,
				     short checkTubeStatus) {

  /************************************************************************/
  /* returns the index of the module to be replaced with the given module */
  /* or -1 if no replacement is found                                     */
  /* typeMatched is set to 1 if no module of the same type is found       */
  /* volumeExceeded is set to 1 if the volume check fails and is enabled  */
  /************************************************************************/

  short i, iMin = -1;
  slong minValue = pc_long_max;
  slong value;
  slong mv = m.value();

  typeMatched = 0;
  volumeExceeded = 0;

  short tubeStat = 0;

  for (i=0; i < bus.numMod; i++) {
    if (bus.moduleList[i].IsEquivalentTo(&m)) {
      
      typeMatched = 1;
      
      value = bus.moduleList[i].value();

      if ((bus.moduleList[i].moduleFunc == ModuleClass::tube ||
	   bus.moduleList[i].moduleFunc == ModuleClass::beam ||
	   bus.moduleList[i].moduleFunc == ModuleClass::ram) &&
	  bus.moduleList[i].CheckTubeMode(ModuleClass::not_upgradeable_mode) &&
	  checkTubeStatus)
	tubeStat = 1;
      else
	tubeStat = 0;
      
      if ((checkValue && (mv > value || 
	   checkStatus && bus.moduleList[i].mode == ModuleClass::free) ||
	   !checkValue) &&
	  (checkStatus && !tubeStat &&
	   bus.moduleList[i].mode != ModuleClass::locked ||
	   !checkStatus) &&
	  value < minValue) {
	
	if (checkVolume && Volume() - bus.moduleList[i].volume +
	    m.volume > design->volumeCap) {
	  
	  volumeExceeded = 1;
	  
	} else {
	  
	  iMin = i;
	  minValue = value;
	  
	}
      }
    }
  }

  return iMin;
}

short ShipClass::NewRotationRate(slong deltaMass) {

  /* calculate the new mass and make sure the engine can handle */
  /* it                                                         */
  /*                                                            */

  slong tmass;

  if (bus.impulseMod) {

    /* calculate the new mass (excluding docked objects) */
    
    tmass = Mass(deltaMass, 0);
    
    short newRotRate = (short)max(min(tmass /(thrust_to_rotation*
					      bus.impulseMod->par[0] + 1),
				      max_rotation_interval),1);
    
    return newRotRate;

  }
  
  return rotRate;
}

void ShipClass::SendGetPowerUpMsg(short listIndex, slong mId, slong value) {

  if (listIndex < -bus.numSlots || listIndex > min(bus.numSlots, max_module_list-1)) {
    logFile << "ERROR> SendGetPowerUpMsg: bad slot value (" << listIndex << ") for a ";
    ShortProfile();
    logFile << " on frame " << game->frame << endl;
    return;
  }

  logFile << "sent> a ";
  ShortProfile();
  logFile << " is sending a GETPOWERUP msg for slot " << listIndex << " and id " << mId << 
    " on frame " << game->frame << endl;

  retMsgBuffer.PushMsg((uchar)SF_MSG_GETPOWERUP,
		       (ushort)game->ObjectListIndex(this),
		       (ushort)listIndex,
		       mId,
		       value,
		       (int)S_CLIENT_TCP);

  ReceiveGetPowerUpMsg(listIndex, mId, value);

}

void ShipClass::PlaceModule(ModuleClass& m,
			    short listIndex,
			    short typeMatched,
			    short& volumeExceeded,
			    short adjustCredit) {

  static char msg[1024];

  /* prevent more than one beam from being installed */
  if (m.moduleFunc == ModuleClass::beam && bus.numBeams)
    return;

  if (listIndex != -1) { /* a system to replace was found */

    slong dMass = m.mass - bus.moduleList[listIndex].mass;
    if (m.moduleFunc != ModuleClass::engine && NewRotationRate(dMass) > 1) {
      sprintf(msg, "1st Officer> The engines can't handle upgrading to %s.",
	      m.name);
      
      game->PostGameMessage(msg,
			    game->ship.indexOf(this), (short)raceID, personal);

      return;
    }

    sprintf(msg, "1st Officer> our %s system has been upgraded to %s.",
	    bus.moduleList[listIndex].name, m.name);
    
    game->PostGameMessage(msg,
			  game->ship.indexOf(this), (short)raceID, personal);

    /************************************************************/
    /* send a message under the following circumstances         */
    /* o  the player is a robot and this is the server          */
    /* o  the player is the local player and this is the client */
    /* o  this is a stand-alone client                          */
    /************************************************************/

    if (adjustCredit) {
      if ((slong)game->player[playerIndex].refValue >= m.value() - bus.moduleList[listIndex].value())
	{
	  game->player[playerIndex].AddToStat(game->player[playerIndex].refValue,
						  bus.moduleList[listIndex].value() - m.value());
	  //game->player[playerIndex].refValue += bus.moduleList[listIndex].value() - m.value();
	}
      else {
	
	sprintf(msg, "1st Officer> we don't have enough credit for a %s.",
		m.name);
	
	game->PostGameMessage(msg,
			      game->ship.indexOf(this), (short)raceID, personal); 

	return;
      }
    }
    /* check index */
    /* retMsgBuffer.PushMsg((uchar)SF_MSG_GETPOWERUP,
       (ushort)game->ObjectListIndex(this),
       (ushort)listIndex,
       m.id,
       game->player[playerIndex].refValue,
       (int)S_CLIENT_TCP); */

    SendGetPowerUpMsg(listIndex, m.id, game->player[playerIndex].refValue);

    //ReceiveGetPowerUpMsg(listIndex, m.id, game->player[playerIndex].refValue);
    
    return;
    
  } else if (volumeExceeded) {
    
    sprintf(msg,
	    "1st Officer> the %s system won't fit in the ship.",
	    m.name);
    
    game->
      PostGameMessage(msg,		  
		      game->ship.indexOf(this), (short)raceID, personal);
    return;
  }
  
  if (typeMatched) {
    sprintf(msg,
	    "1st Officer> we are unable to use the %s.",
	    m.name);
    
    game->
      PostGameMessage(msg,		  
		      game->ship.indexOf(this), (short)raceID, personal);
    return; /* type matched but not swapped */
  }

  /* reset the volume flag */
  volumeExceeded = 0;
  
  /* no matching module was found -- check for room in the moduleList */
  /* and sufficient room in the hull                                  */
  if (bus.numMod >= bus.numSlots ||
      Volume() + m.volume > design->volumeCap) {

    /* reset the volume flag in case the calling function checks it */
    volumeExceeded = 0;
    
    sprintf(msg,
	    "1st Officer> the %s system won't fit in the ship.",
	    m.name);
    
    game->PostGameMessage(msg,
			  game->ship.indexOf(this), (short)raceID, personal);
    return; /* no room to add any more */
  }

  if (NewRotationRate(m.mass) > 1) {
    sprintf(msg, "1st Officer> The engines can't handle adding a %s.",
	    m.name);
    
    game->PostGameMessage(msg,
			  game->ship.indexOf(this), (short)raceID, personal);
    
    return;
  }
  
  sprintf(msg, "1st Officer> a new %s system has been incorporated.",
	  m.name);
  
  game->PostGameMessage(msg,
			game->ship.indexOf(this), (short)raceID, personal);

  if (adjustCredit) {
    if ((slong)game->player[playerIndex].refValue >= m.value())
      {
        game->player[playerIndex].AddToStat(game->player[playerIndex].refValue,-m.value());
	//game->player[playerIndex].refValue -= m.value();
      }
    else {

      sprintf(msg, "1st Officer> we don't have enough credit for a %s.",
	      m.name);
      
      game->PostGameMessage(msg,
			    game->ship.indexOf(this), (short)raceID, personal); 

      return;
    }
  }

  sprintf(msg, "1st Officer> a new %s system has been incorporated.",
	  m.name);
  
  game->PostGameMessage(msg,
			game->ship.indexOf(this), (short)raceID, personal); 

  /* retMsgBuffer.PushMsg((uchar)SF_MSG_GETPOWERUP,
     (ushort)game->ObjectListIndex(this),
     (ushort)bus.numMod,
     m.id,
     game->player[playerIndex].refValue,
     (int)S_CLIENT_TCP); */

  SendGetPowerUpMsg(bus.numMod, m.id,  game->player[playerIndex].refValue);
  //ReceiveGetPowerUpMsg(bus.numMod, m.id, game->player[playerIndex].refValue);
  
  return;
}

void ShipClass::PlugInModuleByCopy(ModuleClass& m) {
  /* *1* find a match for the module function */

  static char msg[1024];
  short iMin = -1;
  short typeMatched = 0;
  short volumeExceeded = 0;

  /***********************************************************************/
  /* check to see if module [m] is a tube and has purchase_only_mode set */
  /* and exit...                                                         */
  /***********************************************************************/
  if (m.IsEquivalentTo(ModuleClass::tube) &&
      m.CheckTubeMode(ModuleClass::purchase_only_mode) || 
      /* prevent more than one beam from being installed */
      m.moduleFunc == ModuleClass::beam && bus.numBeams)
    return;

  switch (game->pluginAction) {
    
  case GameStateClass::swap:
    {
      iMin = FindModuleToReplace(m, typeMatched, volumeExceeded, 1, 1, 1);
      PlaceModule(m, iMin, typeMatched, volumeExceeded);
    }
    break;
  case GameStateClass::increment_level_upgrade:
    /************************************************************/
    /* with this setting, the players will receive credit equal */
    /* to the value of the new system and if there is enough    */
    /* credit to upgrade the existing system to the next level  */
    /* then the upgrade will occur automatically                */
    /************************************************************/
    {

      /* find a module of the same type */
      iMin = FindModuleToReplace(m, typeMatched, volumeExceeded, 1, 1, 1);

      /**************************************************************/
      /* if the type is matched and the value is exceeded, which is */
      /* the case if iMin is not -1, then just swap the old part    */
      /* for the new one.                                           */
      /* or if the type is not matched then install the new system  */
      /**************************************************************/
      if (iMin != -1 || !typeMatched)
	PlaceModule(m, iMin, typeMatched, volumeExceeded);
      else {

	/* find the weakest unit of the same type */
	iMin = FindModuleToReplace(m, typeMatched, volumeExceeded, 0, 0, 1);

	/***********************************************************/
	/* if the type is matched but iMin is still -1 then it in- */
	/* dicates that all of the matching types are locked or    */
	/* special and cannot be replaced.  If this happens then   */
	/* exit...                                                 */
	/***********************************************************/
	if (typeMatched && iMin == -1)
	  return;

	/**********************************************************/
	/* send a message to indicate that credit should be given */
	/* use a -iMin for the moduleList index to indicate that  */
	/* upon receipt a credit value should be given instead of */
	/* module placement                                       */
	/**********************************************************/

	/***********************************************************/
	/* if the message receiver finds that enough credit exists */
	/* then it should send an upgrade message                  */
	/***********************************************************/
	slong mv = m.value() * game->pluginPar / 100;
	short i = GetNextModuleIndexInUpgradePath(iMin, 1, mv);

	if (i >= 0) {

	  ModuleClass* mNext = game->moduleList + i;

          game->player[playerIndex].SetStat(game->player[playerIndex].refValue,
						min(game->player[playerIndex].refValue +
						    mv, max_credit));
	  //game->player[playerIndex].refValue = min(game->player[playerIndex].refValue +
	  //				   mv, max_credit);

	  PlaceModule(*mNext, iMin, typeMatched, volumeExceeded, 1);

	} else if (i == -1) {

          game->player[playerIndex].SetStat(game->player[playerIndex].refValue,
						min(game->player[playerIndex].refValue +
						    mv, max_credit));
	  //game->player[playerIndex].refValue = min(game->player[playerIndex].refValue +
	  //				     mv, max_credit);
	    
	    sprintf(msg,
		    "1st Officer> we have received a credit of %s",
		    Format(mv));
	    
	    game->PostGameMessage(msg,
				  game->ship.indexOf(this), (short)raceID, personal);
	}
	
      }

    }
    break;
  case GameStateClass::increment_pct_value_upgrade:
    /***********************************************************/
    /* with this setting, the players will receive incremental */
    /* upgrades in system performance each time they pickup a  */
    /* powerup  where the percentage defines approximately what*/
    /* percentage of the value of the powerup is added in      */
    /* attribute enhancements to their current system          */
    /***********************************************************/
    {
      /* find a module of the same type */
      iMin = FindModuleToReplace(m, typeMatched, volumeExceeded, 1, 1, 1);

      /**************************************************************/
      /* if the type is matched and the value is exceeded, which is */
      /* the case if iMin is not -1, then just swap the old part    */
      /* for the new one.                                           */
      /* or if the type is not matched then install the new system  */
      /**************************************************************/
      if (iMin != -1 || !typeMatched)
	PlaceModule(m, iMin, typeMatched, volumeExceeded);
      else {

	/* find the weakest unit of the same type */
	iMin = FindModuleToReplace(m, typeMatched, volumeExceeded, 0, 0, 1, 0);

	if (iMin == -1)
	  return;

	/**********************************************************/
	/* send a message to indicate that credit should be given */
	/* use a -iMin for the moduleList index to indicate that  */
	/* upon receipt a credit value should be given instead of */
	/* module placement                                       */
	/**********************************************************/
	slong v = bus.moduleList[iMin].value();

	slong val = m.value() * game->pluginPar / 100;
	slong valPct = min(m.value() * game->pluginPar / max(v,1),
			  50 /* max_pct_powerup_increase */);
	
	/* recalculate the incresed value in case the percentage was capped */
	val = valPct * max(bus.moduleList[iMin].value(),1) / 100;

	/* create a copy of the module */
	ModuleClass m2 = bus.moduleList[iMin];
	
	/* increase the module strength */
	m2.IncreaseValue(val, valPct);

	slong vi = (m2.value() - v) * 100 / max(v, 1);
		
	if (vi > 0) {
	  sprintf(msg,
		  "1st Officer> the strength of our %s has increased by %ld%%",
		  bus.moduleList[iMin].name, vi);
	  
	  game->PostGameMessage(msg,
				game->ship.indexOf(this), (short)raceID, personal);
	  
	  /* retMsgBuffer.PushMsg((uchar)SF_MSG_GETPOWERUP,
	     (ushort)game->ObjectListIndex(this),
	     (ushort)(-iMin-1),
	     m.id,
	     game->player[playerIndex].refValue,
	     (int)S_CLIENT_TCP); */
	  
	  SendGetPowerUpMsg((-iMin-1), m.id,  game->player[playerIndex].refValue);
	  //ReceiveGetPowerUpMsg((-iMin-1), m.id, game->player[playerIndex].refValue);

	}
      }
    }
    break;
  case GameStateClass::increment_pct_credit_upgrade:
    /***********************************************************/
    /* with this setting, the players will receive incremental */
    /* credit each time they pickup a powerup where the        */
    /* percentage defines approximately what percentage of the */
    /* credit they will receive                                */
    /***********************************************************/
    {
      
      /* find a module of the same type */
      iMin = FindModuleToReplace(m, typeMatched, volumeExceeded, 1, 1, 1);

      /**************************************************************/
      /* if the type is matched and the value is exceeded, which is */
      /* the case if iMin is not -1, then just swap the old part    */
      /* for the new one.                                           */
      /* or if the type is not matched then install the new system  */
      /**************************************************************/
      if (iMin != -1 || !typeMatched) {
	PlaceModule(m, iMin, typeMatched, volumeExceeded);
	if (!volumeExceeded)
	  return;
      }

      /**********************************************************/
      /* otherwise:                                             */
      /* send a message to indicate that credit should be given */
      /* use a -1 for the moduleList index to indicate that     */
      /* upon receipt a credit value should be given instead of */
      /* module placement                                       */
      /**********************************************************/
      
      slong mv = m.value() * game->pluginPar / 100;
      
      sprintf(msg,
	      "1st Officer> we have received a credit of %s",
	      Format(mv));
      
      game->PostGameMessage(msg,
			    game->ship.indexOf(this), (short)raceID, personal);
      
      /* retMsgBuffer.PushMsg((uchar)SF_MSG_GETPOWERUP,
	 (ushort)game->ObjectListIndex(this),
	 (ushort)(-1),
	 m.id,
	 game->player[playerIndex].refValue + mv,
	 (int)S_CLIENT_TCP); */

      SendGetPowerUpMsg(-1, m.id, game->player[playerIndex].refValue + mv);
      //ReceiveGetPowerUpMsg(-1, m.id, game->player[playerIndex].refValue + mv);
      
    }
    break;
  }
}

short ShipClass::GetNextModuleIndexInUpgradePath(short iSlot,
						 short checkValue,
						 slong bonus) {

  /***********************************************************************/
  /* return the game state index for the next module in the upgrade path */
  /* for the part in slot [iSlot].  This function will check for volume  */
  /* and value (if [checkValue] is non-zero).  If there is no part in    */
  /* the sequence because either the volume is exceeded or the part is   */
  /* already the best available, then return a -2.  Otherwise, return a  */
  /* -1 for the case where refValue is not high enough, but a better     */
  /* part exists.                                                        */
  /***********************************************************************/


  short index = game->ValidateModuleID(bus.moduleList[iSlot].id);

  ModuleClass* m0 = bus.moduleList + iSlot;
  slong m0v = 0;
  if ( m0 )
    {
      m0v = m0->value();
    }
  slong vol = Volume();  
  slong v;

  short i, ii = index + 1, j = 1;
  while(j < game->numModules) {
    
    i = (ii + game->numModules) % game->numModules;

    if (game->moduleList[i].IsEquivalentTo(m0) &&
	game->tech[raceID].IsSet(i)) {

      v = game->moduleList[i].value();

      if (vol - m0->volume + game->moduleList[i].volume <=
	  design->volumeCap &&
	  v > m0v) {
      
	if (checkValue && v <= (slong)game->player[playerIndex].refValue + bonus + m0v || !checkValue)
	  return i;
	else
	  return -1;

      }
    }

    ii++;
    j++;
    
  }

  return -2;
}

void ShipClass::SendSetModulePointerMsg() {

  if ( bus.modPtr < bus.numMod)
    
    retMsgBuffer.PushMsg((uchar)SF_MSG_SETMODULEPOINTER,
			 (ushort)game->ObjectListIndex(this),
			 (uchar)bus.modPtr,
			 (int)S_CLIENT_TCP);
  
  else {
    
    logFile << "ERROR> SETMODULEPOINTER tried to send bad pointer value (" <<
      bus.modPtr << ") for a ";
    ShortProfile();
    logFile << " with " << bus.numMod << " slots on frame " <<
      game->frame << endl;
    
  }

}

void ShipClass::SendSetTargetMsg(ObjectUnion t) {

  if ( IsValidObjectPointer(t.generic,__FILE__,__LINE__) ) {
    
    retMsgBuffer.PushMsg((uchar)SF_MSG_SETTARGET,
			 (ushort)game->ObjectListIndex(this),
			 (ushort)game->ObjectListIndex(t.generic),
			 (int)S_CLIENT_TCP);

    if (t.generic->Type() >= ship && t.generic != this)
      t.ship->SendRequestForPopulation();
    
  }
}

void ShipClass::SendRequestForPopulation() {

  retMsgBuffer.PushMsg((uchar)SF_MSG_REQUESTPOPULATION,
		       (ushort)game->ObjectListIndex(this),
		       (int)S_CLIENT_TCP);

}

void ShipClass::ReceiveConfirmationOfPopulation(short pop1, slong pop2, 
						uchar ai1, uchar ai2, uchar ai3,
						uchar ai4, uchar ai5, uchar ai6,
						slong x, slong y) {

  if (Type() == colony) {
    gnow.x = x;
    gnow.y = y;
    gvel = tvel;
    gfresh = TRUE;
    gframe = (short)(game->frame % 256);
  }
  
  if (bus.powerMod && bus.powerMod[0] && bus.numPowerPlants > 0 ) {
    bus.powerMod[0]->SetStat(bus.powerMod[0]->func,ai1 * (module_function_scale / 100));
    bus.powerMod[0]->SetStat(bus.powerMod[0]->damage,(module_function_scale - bus.powerMod[0]->func) * bus.powerMod[0]->damageCap / 100);
    bus.powerMod[0]->SetStat(bus.powerMod[0]->store,bus.powerMod[0]->power * bus.powerMod[0]->par[0] * ai2 / 100);
    //bus.powerMod[0]->func = ai1 * (module_function_scale / 100);
    //bus.powerMod[0]->damage = (module_function_scale - bus.powerMod[0]->func) * bus.powerMod[0]->damageCap / 100;
    //bus.powerMod[0]->store = bus.powerMod[0]->power * bus.powerMod[0]->par[0] * ai2 / 100;
  }

  if (bus.shieldMod) {
    bus.shieldMod->SetStat(bus.shieldMod->func,ai3 * (module_function_scale / 100));
    bus.shieldMod->SetStat(bus.shieldMod->damage,(module_function_scale - bus.shieldMod->func) * bus.shieldMod->damageCap / 100);
    bus.shieldMod->SetStat(bus.shieldMod->store,-bus.shieldMod->power * bus.shieldMod->par[0] * ai4 / 100);
    //bus.shieldMod->func = ai3 * (module_function_scale / 100);
    //bus.shieldMod->damage = (module_function_scale - bus.shieldMod->func) * bus.shieldMod->damageCap / 100;
    //bus.shieldMod->store = -bus.shieldMod->power * bus.shieldMod->par[0] * ai4 / 100;
  }

  if (bus.impulseMod) {
    bus.impulseMod->SetStat(bus.impulseMod->func,ai5 * (module_function_scale / 100));
    bus.impulseMod->SetStat(bus.impulseMod->damage,(module_function_scale - bus.impulseMod->func) * bus.impulseMod->damageCap / 100);
    bus.impulseMod->SetStat(bus.impulseMod->store,-bus.impulseMod->power * ai6 / 100);
    //bus.impulseMod->func = ai5 * (module_function_scale / 100);
    //bus.impulseMod->damage = (module_function_scale - bus.impulseMod->func) * bus.impulseMod->damageCap / 100;
    //bus.impulseMod->store = -bus.impulseMod->power * ai6 / 100;
  }  

  pop.SetStat(pop.billions,pop1);
  pop.SetStat(pop.lBillions,pop2);
  //pop.billions = pop1;
  //pop.lBillions = pop2;

  if (!IsServer() &&
      localSlotNumber >= 0 && localSlotNumber < game->numShips && 
      IsValidObjectPointer(game->ship[localSlotNumber].targetPP.obj.generic,__FILE__,__LINE__) &&
      game->ship[localSlotNumber].targetPP.obj.ship == this) {
    game->ship[localSlotNumber].PlaySoundOnScreen(game->soundEvent[acquire_target_event]);
  }

}

void ShipClass::SendConfirmationOfPopulation() {

  uchar ai1 = 0, ai2 = 0, ai3 = 0, ai4 = 0, ai5 = 0, ai6 = 0;

  if (bus.powerMod && bus.powerMod[0] && bus.numPowerPlants > 0 ) {
    ai1 = uchar(bus.powerMod[0]->func / (module_function_scale / 100));
    ai2 = uchar(bus.powerMod[0]->Storage());
  }

  if (bus.shieldMod) {
    ai3 = uchar(bus.shieldMod->func / (module_function_scale / 100));
    ai4 = uchar(bus.shieldMod->Storage());
  }

  if (bus.impulseMod) {
    ai5 = uchar(bus.impulseMod->func / (module_function_scale / 100));
    ai6 = uchar(bus.impulseMod->Storage());
  }

  retMsgBuffer.PushMsg((uchar)SF_MSG_SETPOPULATION,
		       (ushort)game->ObjectListIndex(this),
		       (slong) pop.billions,
		       (slong) pop.lBillions,
		       ai1,
		       ai2,
		       ai3,
		       ai4,
		       ai5,
		       ai6,
		       now.x,
		       now.y,
		       (int)S_CLIENT_TCP);
}

void ShipClass::SendCaptureMsg(ShipClass* s) {

  /* this ship captures s */

  /********************************************************/
  /* WARNING:                                             */
  /* only the server can send messages for another player */
  /* if a human player sends a message with another slot  */
  /* then the message will be received twice.             */
  /********************************************************/
  /* NOTE: the captured ship is the slot number for the   */
  /* messages.  If neither s nor this is the local slot   */
  /* number then use SF_MSG_CAPTURE.  If, however, the    */
  /* local slot number is the capturer then use the       */
  /* SF_MSG_HUMANCAPTURE message.                         */
  /********************************************************/

  s->SetPendingFlag(captured);

  logFile << "sent> a ";
  ShortProfile();
  logFile << " has captured a ";
  s->ShortProfile();
  logFile << " on frame " << game->frame << endl;


  if (game->IsRobotPlayer(s) && game->IsRobotPlayer(this) ||
      !game->IsRobotPlayer(s))

    /* this should only be sent by the server */

    /************************************************************************/
    /* situation: a robot player has been captured by another robot player. */
    /*            Send a SF_MSG_CAPTURE.                                    */
    /************************************************************************/

    retMsgBuffer.PushMsg((uchar)SF_MSG_CAPTURE,
			 (ushort)game->ObjectListIndex(s),
			 (ushort)game->ObjectListIndex(this),
			 (uchar)raceID,
			 S_CLIENT_TCP);
  
  else

    /***********************************************************************/
    /* situation: a human player has been captured by either another human */
    /*            player or a robot player.  Send a SF_MSG_HUMANCAPTURE.   */
    /***********************************************************************/

    retMsgBuffer.PushMsg((uchar)SF_MSG_HUMANCAPTURE,
			 (ushort)game->ObjectListIndex(this),
			 (ushort)game->ObjectListIndex(s),
			 (uchar)raceID,
			 S_CLIENT_TCP);
}

void ShipClass::ReceiveCaptureMsg(ShipClass* s, short rID) {

  ObjectUnion cObj;

  static char msg[1024];
  slong bonus = 0, bounty = 0, addScore = 0, cBonus = 0;

  /* this captures s */

  s->ClearPendingFlag(captured);

  /* failsafe */
  raceID = rID;

  cObj.ship = s;
  
  s->targetPP.obj.ship = this;
  s->SendDistressSignal();

  PlaySoundOnScreen(game->soundEvent[capture_enemy_event]);

  s->PlaySoundOnScreen(game->soundEvent[captured_by_enemy_event]);
  
  switch (s->Type()) {
  case ship:

    pop -= s->pop;
    
    /* make sure that the population is not negative */
    if (pop < PopStruct())
      pop = PopStruct(0,1);

    logFile << "received> a ";
    ShortProfile();
    logFile << " has captured a ";
    s->ShortProfile();
    logFile << " on frame " << game->frame << endl;

    /* convert the captured player */
    game->player[s->playerIndex].raceId = raceID;
    game->player[s->playerIndex].PutOnProbation();

    /************************************************/
    /* reset the following captured player's stats: */
    /* o  credit                                    */
    /* o  strategicPoints                           */
    /* o  activeFrames                              */
    /************************************************/
    game->player[s->playerIndex].SetStat(game->player[s->playerIndex].refValue,0);
    game->player[s->playerIndex].SetStat(game->player[s->playerIndex].strategicPoints,0);
    //game->player[s->playerIndex].refValue = 0;
    //game->player[s->playerIndex].strategicPoints = 0;
    game->player[s->playerIndex].activeFrames = 0;
    
    /* broadcast message to capturing player */
    if (game->TypeListIndex(s) == localSlotNumber) {
      sprintf(msg, "Server> You have been captured by the %ss!",
	      game->race[rID].name);
      
      game->PostGameMessage(msg, game->ship.indexOf(s), rID, personal);
      
    }

    /* change alliance */
    s->raceID = raceID;
    
    break;

  case colony:
    /* colony-changes-race message */

    s->PlaySoundOnScreen(game->soundEvent[planet_surrenders_event]);
    
    logFile << "received> a ";
    ShortProfile();
    logFile << " has forced a ";
    s->ShortProfile();
    logFile << " to surrender on frame " << game->frame << endl;

    bonus = s->pop.log10() / 2;

    if ( IsValidObjectPointer(s->lockObj.generic,__FILE__,__LINE__) ) {

      bonus += s->lockObj.planet->ExploitationPotential() / 100;
      
      sprintf(msg, "%s colony on %s surrendered to %ss.", 
	      game->race[s->raceID].name,
	      s->lockObj.planet->name,
	      game->race[rID].name);
      
      /* claim planet */
      s->lockObj.generic->raceID = rID;
      
    } else {
      
      sprintf(msg, "A %s colony has surrendered to the %ss.", 
	      game->race[s->raceID].name,
	      game->race[rID].name);
      
    }
    
    /* notify old race */
    game->PostGameMessage(msg, (short)s->raceID, (short)s->raceID,
			  broadcast);

    /* change alliance */
    s->raceID = raceID;
    
    /* if this object was building something convert it */
    cObj.colony->ConvertConstructionProject();
    
    /* notify new race */
    game->PostGameMessage(msg, rID, rID, broadcast);

    break;

  default :
    break;
  }

  if (Type() == ship) {
    
    /* broadcast message to capturing player */
    if (IsLocalPlayer(this)) {
      sprintf(msg, "1st Officer> We have captured the enemy %s!", s->TypeName());
      
      game->PostGameMessage(msg,
			    game->ship.indexOf(this),
			    (short)raceID, personal);
      
    }

    switch (s->Type()) {
    case ship:
      bounty = game->player[s->playerIndex].activeScore * bounty_pct / 100;
      addScore = (2*(s->design->reqRank + 1) + bounty) * game->bonusFactor;

      /* give credit for salvage */
      cBonus = s->TotalValue() / 100 * game->salvageRightPct;

      game->player[playerIndex].AddToStat(game->player[playerIndex].refValue,cBonus);
      //game->player[playerIndex].refValue += cBonus;

      if (IsLocalPlayer(this)) {

	sprintf(msg, "1st Officer> We have received a bonus credit of %s", Format(cBonus));
	
	game->PostGameMessage(msg,
			      game->ship.indexOf(this),
			      (short)raceID, personal);
	
      }
      
      /* credit double points to the capturing ship */
      game->player[playerIndex].AddToStat(game->player[playerIndex].totalScore,
					       (ulong)addScore
					       );
      game->player[playerIndex].AddToStat(game->player[playerIndex].activeScore,
					       (ulong)addScore
					       );
      game->player[playerIndex].AddToStat(game->player[playerIndex].strategicPoints,
					       (ulong)addScore
					       );
      game->player[playerIndex].AddToStat(game->player[playerIndex].numKills,
					       (ulong)1
					       );
      game->player[playerIndex].AddToStat(game->player[playerIndex].numKillsTotal,
					       (ulong)1
					       );

      //game->player[playerIndex].activeScore += addScore;
      //game->player[playerIndex].totalScore += addScore;
      //game->player[playerIndex].strategicPoints += addScore;
      //game->player[playerIndex].numKills++;
      //game->player[playerIndex].numKillsTotal++;
      
      /* reset active kills for captured player */
      game->player[s->playerIndex].SetStat(game->player[s->playerIndex].activeScore,
					       0
					       );
      //game->player[s->playerIndex].activeScore = 0;
      
      break;

    case colony:

      addScore = bonus * game->bonusFactor;

      game->player[playerIndex].AddToStat(game->player[playerIndex].totalScore,
					       (ulong)addScore
					       );
      game->player[playerIndex].AddToStat(game->player[playerIndex].activeScore,
					       (ulong)addScore
					       );
      game->player[playerIndex].AddToStat(game->player[playerIndex].strategicPoints,
					       (ulong)addScore
					       );
      game->player[playerIndex].AddToStat(game->player[playerIndex].numKills,
					       (ulong)game->bonusFactor
					       );
      game->player[playerIndex].AddToStat(game->player[playerIndex].numKillsTotal,
					       (ulong)game->bonusFactor
					       );

      //game->player[playerIndex].activeScore += addScore;
      //game->player[playerIndex].totalScore += addScore;
      //game->player[playerIndex].strategicPoints += addScore;
      //game->player[playerIndex].numKills += game->bonusFactor;
      //game->player[playerIndex].numKillsTotal += game->bonusFactor;

      break;

    default :
      break;
    }
  }  
}

void ShipClass::ReceiveGetPowerUpMsg(short iSlot, slong mId, slong value) {

  static char msg[1024];
  short i;

  short index = game->ValidateModuleID(mId);

  if (index == -1) {
    logFile << "ERROR> ReceiveGetPowerUpMsg: unknown module id." << endl;
    return;
  }

  if (iSlot < -bus.numSlots || iSlot > min(bus.numSlots, max_module_list-1)) {
    logFile << "ERROR> ReceiveGetPowerUpMsg: bad slot value (" << iSlot << ") for a ";
    ShortProfile();
    logFile << " on frame " << game->frame << endl;
    return;
  }

  slong newCredit = value - game->player[playerIndex].refValue;

  game->player[playerIndex].SetStat(game->player[playerIndex].refValue,value);
  //game->player[playerIndex].refValue = value;

  /* if the module is unknown to this race then give credit */
  if (game->tech[raceID].set(index)) {

    /* award credit in accordance with the salvage rights percentage */
    game->player[playerIndex].AddToStat(game->player[playerIndex].refValue,
					    (slong)game->moduleList[index].value() * game->salvageRightPct / 100
					    );
    game->player[playerIndex].AddToStat(game->player[playerIndex].activeScore,
                                             (ulong)game->bonusFactor
					     );
    game->player[playerIndex].AddToStat(game->player[playerIndex].totalScore,
                                             (ulong)game->bonusFactor
					     );
    game->player[playerIndex].AddToStat(game->player[playerIndex].strategicPoints,
                                             (ulong)game->bonusFactor
					     );

    //game->player[playerIndex].refValue += game->moduleList[index].value() * game->salvageRightPct / 100;
    //game->player[playerIndex].activeScore += game->bonusFactor;
    //game->player[playerIndex].totalScore += game->bonusFactor;
    //game->player[playerIndex].strategicPoints += game->bonusFactor;
    
    if (game->TypeListIndex(this) == localSlotNumber) {
      
      sprintf(msg, "1st Officer> We have captured alien technology");
      game->PostGameMessage(msg, localSlotNumber, (short)raceID, personal);

      WRITE_SF_LOG(logFile << "received> a ");
      WRITE_SF_LOG(ShortProfile());
      WRITE_SF_LOG(logFile << " has captured an alien " <<  game->moduleList[index].name <<
	" on frame " << frame << endl);
      
    }
  }

  ModuleClass m = game->moduleList[index];

  /*******************************************************************/
  /* if a slot number of -1 is passed then the player should receive */
  /* the value of the module mId as credit                           */
  /*******************************************************************/
  if (iSlot < 0) {

    i = -iSlot - 1;

    if (game->pluginAction == GameStateClass::increment_pct_value_upgrade) {

      slong val = m.value() * game->pluginPar / 100;
      slong valPct = min(m.value() * game->pluginPar /
			max(bus.moduleList[i].value(),1),
			50 /* max_pct_powerup_increase */);

      /* recalculate the incresed value in case the percentage was capped */
      val = valPct * max(bus.moduleList[i].value(),1) / 100;

      /* increase in module strength occurs here */
      bus.moduleList[i].IncreaseValue(val, valPct);

      logFile << "received> a ";
      ShortProfile();
      logFile << " has enhanced a " <<  bus.moduleList[i].name <<
	" in slot " << i << " by " << valPct << "% on frame " << game->frame << endl;

      /* mark the module to show that is enhanced */
      short j = strlen(bus.moduleList[i].name);
      if (j < max_name_char-1 && bus.moduleList[i].name[j-1] != '+') {
	bus.moduleList[i].name[j] = '+';
	bus.moduleList[i].name[j+1] = '\0';
      }

    } else if (game->pluginAction == GameStateClass::increment_level_upgrade) {

      logFile << "received> a ";
      ShortProfile();
      logFile << " has received a credit of " << newCredit << " for picking up a " << m.name <<
	" on frame " << game->frame << endl;
      
      /****/
      /* this code should be moved to the sending side of the message exchange */
      
      
      //  short i = 0;
	
      //        if ((i = GetNextModuleIndexInUpgradePath(-iSlot-1)) != -1) {
	  
      //  	ModuleClass* m0 = bus.moduleList - iSlot - 1;
	  
      //  	/* make a copy of the upgrade system */
      //  	m = game->moduleList[i];
	  
      //  	sprintf(msg, "1st Officer> our %s system has been upgraded to %s.",
      //  		m0->name, m.name);
	  
      //  	game->PostGameMessage(msg,
      //  			      this-game->ship, (short)raceID, personal);

      //  	WRITE_SF_LOG(logFile << "received> a ");
      //  	WRITE_SF_LOG(ShortProfile());
      //  	WRITE_SF_LOG(logFile << " has upgraded to a " << m.name << " from a " << m0->name <<
      //  		     " in slot " << -iSlot - 1 << " on frame " << game->frame << endl);

      //  	/***************************************/
      //  	/* copy the existing module parameters */
      //  	/* this preserves the firing angles    */
      //  	/***************************************/
      //  	short jj;
      //  	for (jj=0; jj < m0->nSPar; jj++)
      //  	  m.specPar[jj] = m0->specPar[jj];

      //  	/* this preserves the active state of the module */
      //  	short activeMod = 0;
      //  	if (m0->IsActive())
      //  	  activeMod = 1;
	  
      //  	/* swap the modules */
      //  	*m0 = m;
      //  	m0->host.designed = this;
	  
      //  	bus.MapFunctions(this, this);

      //  	if (activeMod)
      //m.activate();
	  
      /* set the credit value */
      //refValue = max(refValue - m0->value(), 0);
	  
      //}
    }
    

    if (bus.numMod)
      bus.modPtr = bus.modPtr % bus.numMod;
    else
      bus.modPtr = 0;

    return;
  }

  /***********************************************************************/
  /* if this powerup was a new one (it did not replace an existing one), */
  /* then increment the number of modules                                */
  /***********************************************************************/
  if (iSlot == bus.numMod) {
    bus.moduleList[bus.numMod].initialize();

    logFile << "received> a ";
    ShortProfile();
    logFile << " has added a " << m.name << " in slot " << bus.numMod << " on frame " <<
      game->frame << endl;

    bus.numMod++;

  } else {
    
    logFile << "received> a ";
    ShortProfile();
    logFile << " has upgraded to a " << m.name << " from a " << bus.moduleList[iSlot].name <<
      " in slot " << iSlot << " on frame " << game->frame << endl;
  }
  
  /* copy the existing module parameters */
  /* this preserves the firing angles */
  short j;
  for (j=0; j < bus.moduleList[iSlot].nSPar; j++)
    m.specPar[j] = bus.moduleList[iSlot].specPar[j];

  /* check the active state of the old module */
  short isActive = bus.moduleList[iSlot].active;
  
  /* swap the modules */
  bus.moduleList[iSlot] = m;
  bus.moduleList[iSlot].host.designed = this;

  if (isActive)
    bus.moduleList[iSlot].activate();
  
  //logFile << TypeName() << " " << game->TypeListIndex(this) << 
  //  " has replaced module " << i << " on frame " << game->frame << endl;
  
  bus.RemapFunctions(this, this, "a module swap/upgrade");

}

void DesignedClass::SeekObj(PingPacket& pp, slong sens, slong vSens, slong lead) {
  /* set velocity to seek object pointed to by pp                        */
  /* this function assumes that the flux is limited by impulseMod par[0] */

  static CoordinateClass wvel;

  if ((pp.dist > min_warp_dist/2 && IsWarping()) || 
      (pp.dist > (int)min_warp_dist &&  !IsWarping()) && bus.warpMod &&
      bus.warpMod->IsReady()) {

    /* seek object is far enough away to engage q drive */

    wvel = pp.disp;
    wvel *= (double(bus.warpMod->aPar(0)) / pp.dist);

    if (SeekHeading(wvel) || IsWarping()) {

      /* object is ready to warp or already warping */

      bus.warpMod->Withdraw();
      if (!IsWarping())
	/* start transition if ready */
	WarpTransition();
      else {
	vel = wvel;
      }

    } else if (bus.impulseMod) {

      /* object has not engaged warp yet so continue thrusting impulse */

      if ( IsValidObjectPointer(pp.obj.generic,__FILE__,__LINE__) )
	CheckTransition(pp.obj.generic->tvel);
      else
	CheckTransition(nullVector);

      slong thr;
      slong af = mass*100/frameRate/frameRate;
      if (bus.boostMod && bus.boostMod->active)
	thr = (10000 + bus.boostMod->cPar(2, bus.CalculateSurplusPower()))
	  * bus.impulseMod->afMaxPar(af,0) /
	  10000;
      else
	thr = bus.impulseMod->aPar(0);

      /* do not allow the acceleration to exceed the game state maximum */
      if (Type() != missile)
	thr = max(min(thr, game->maxAcc * af/100) * sub_scale, 0);
      else
	thr = thr * sub_scale;

      short pct = 
	SeekThrust(pp, sens, vSens, lead, thr);
      /* bus.impulseMod->Withdraw(); */
      bus.impulseMod->SetStat(bus.impulseMod->store,max(bus.impulseMod->store +
							bus.impulseMod->power * pct / 100, 0L));
      //bus.impulseMod->store = max(bus.impulseMod->store +
      //			bus.impulseMod->power * pct / 100, 0L);
      stat = idle;

      throttle = 1;
    }
  } else if (bus.impulseMod) {

    /* object is close enough to not use q drive or q drive offline */
    /* or nonexistent                                               */

    if ( IsValidObjectPointer(pp.obj.generic,__FILE__,__LINE__) )
      CheckTransition(pp.obj.generic->tvel);
    else
      CheckTransition(nullVector);

    slong thr;
    slong af = mass*100/frameRate/frameRate;
    if (bus.boostMod && bus.boostMod->active)
      thr = (10000 + bus.boostMod->cPar(2, bus.CalculateSurplusPower())) *
	bus.impulseMod->afMaxPar(af,0) /
	10000;
    else
      thr = bus.impulseMod->aPar(0);

    /* do not allow the acceleration to exceed the game state maximum */
    if (Type() != missile)
      thr = max(min(thr, game->maxAcc * af/100) * sub_scale, 0);
    else
      thr = thr * sub_scale;

    short pct = 
      SeekThrust(pp, sens, vSens, lead, thr);
    /* bus.impulseMod->Withdraw(); */
    bus.impulseMod->SetStat(bus.impulseMod->store,max(bus.impulseMod->store +
						      bus.impulseMod->power * pct / 100, 0L));
    //bus.impulseMod->store = max(bus.impulseMod->store +
    //			bus.impulseMod->power * pct / 100, 0L);
    SeekHeading(vel);

    throttle = 1;
  }
}

void DesignedClass::FleeObj(PingPacket& pp, slong seek, slong flee) {
  /* set velocity to flee object pointed to by pp */
  if (bus.impulseMod) {

    if ( IsValidObjectPointer(pp.obj.generic,__FILE__,__LINE__) )
      CheckTransition(pp.obj.generic->tvel);
    else
      CheckTransition(nullVector);
    
    slong thr;
    slong af = mass*100/frameRate/frameRate;
    if (bus.boostMod && bus.boostMod->active)
      thr = (10000 + bus.boostMod->cPar(2, bus.CalculateSurplusPower()))
	* bus.impulseMod->afMaxPar(af,0) /
	10000;
    else
      thr = bus.impulseMod->aPar(0);
    
    /* do not allow the acceleration to exceed the game state maximum */
    thr = max(min(thr, game->maxAcc * af/100) * sub_scale, 0);
    
    short pct = FleeThrust(pp, -seek, flee, thr);
    bus.impulseMod->SetStat(bus.impulseMod->store,max(bus.impulseMod->store +
						      bus.impulseMod->power * pct / 100, 0L));
    //bus.impulseMod->store = max(bus.impulseMod->store +
    //			bus.impulseMod->power * pct / 100, 0L);

    SeekHeading(vel);
    
    throttle = 1;
  }
}

unsigned short DesignedClass::FleeThrust(PingPacket& pp, slong par, slong per,
				slong limit) {
  /* return limited thrust to flee the target object */
  /*   thrust is limited by the limit parameter      */
  /*   velocity is limited by the maxSpeed member    */
  static CoordinateClass th;
  /* percentage of thrust used */
  unsigned short pct = (unsigned short) FleeCore(th, pp, par, per, limit);
  ApplyThrust(th, maxSpeed);
  return pct;
}

unsigned short DesignedClass::SeekThrust(PingPacket& pp, slong sens, slong vSens,
				slong lead, slong limit,
				short applyLimit) {
  /* return limited thrust to seek the target object */
  /*   thrust is limited by the limit parameter      */
  /*   velocity is limited by the maxSpeed member    */
  static CoordinateClass th;
  /* percentage of thrust used */
  th.initialize();
  unsigned short pct = (unsigned short) SeekCore(th, pp, sens, vSens, lead, limit);
  if (applyLimit)
    ApplyThrust(th, maxSpeed);
  else
    ApplyThrust(th, game->lightSpeed);
  return pct;
}

ulong DesignedClass::SeekCore(CoordinateClass& dv,
			     PingPacket& pp, slong sens, slong vSens,
			     slong lead, slong limit) {
  /* return limited velocity differential to seek the target object */
  static CoordinateClass disp;
  disp = pp.disp;
  if (slong(pp.dist) > lead*sub_scale) {
    disp *= (pp.dist-lead*sub_scale) * sens / (pp.dist+1);
  } else {

    slong dist = pp.dist - lead * sub_scale;
    if (dist < pc_long_sqrt_max)
      disp = pp.disp * dist / (pp.dist+1);
    else
      disp = pp.disp * (dist / 1000) / pp.dist * 1000;

    disp *= sens;
  }
  if ( IsValidObjectPointer(pp.obj.generic,__FILE__,__LINE__) )
    {
      dv.AddSubjectToLimit(disp+(pp.obj.generic->tvel-tvel) * (mass * vSens / 100),
                           limit);
    }

  return dv.Mag() * 100 / (labs(pp.dist - lead)+1);
}

ulong DesignedClass::FleeCore(CoordinateClass& dv,
			      PingPacket& pp, slong par, slong per,
			      slong limit) {
  /* return change in velocity to seek the target object */
  static CoordinateClass dispPar;
  static CoordinateClass dispPer;
  dispPar = pp.disp;
  dispPer = pp.disp.crossZ();

  /* dispPer /= 100; */
  /* dispPar /= 100; */
  dispPer *= per; /* ready for long */
  dispPar *= par; /* ready for long */

  dv.AddSubjectToLimit(dispPar+dispPer+
                       pp.obj.generic->tvel-vel, limit);

  return dv.Mag() * 100 / (pp.dist+1);
}

void GenericClass::PlaySound(short soundIndex) {
  if (soundIndex != -1)
    currentSound = SndCueWav(currentSound, soundIndex);
}

void GenericClass::PlaySoundOnScreen(short soundIndex, short localOnly) {
  short slot = game->ObjectListIndex(this);
  if (soundIndex != -1 && (slot == localSlotNumber && !IsServer() || !localOnly))
    currentSound = SndCueWav(currentSound, soundIndex, slot);
}

CoordinateClass& GenericClass::Attract(PingPacket& pp, short avoid, short free) {
  /* apply gravity to this object due to the target object */

  static CoordinateClass rhat;
  static CoordinateClass dv;

  if ( free || frame >= nextGravityFrame ) {

    double dx = double(pp.disp.x) / sub_scale;
    double dy = double(pp.disp.y) / sub_scale;
    rhat = pp.disp;
    if (pp.dist)
      rhat *= grav_constant_1 / (pp.dist);
    else
      rhat = nullVector;

    /* calculate change in velocity */
    dv = rhat;
    double r2 = dx*dx + dy*dy;
    double pr2 = 0.0;
    if ( IsValidObjectPointer(pp.obj.generic,__FILE__,__LINE__) )
    { 
      pr2 = slong(pp.obj.generic->bitmap.size) * slong(pp.obj.generic->bitmap.size) / 2;
    }
    /* set minimum approach radius to prevent singular response */
    /* situation: ship initialization                           */
    /*            ships warping into center of planet           */
    /*            ships transitioning away from parent planet   */
    r2 = (r2>pr2)?r2:pr2;

    double dvMag = 0.0;

    if ( IsValidObjectPointer(pp.obj.generic,__FILE__,__LINE__) &&
         (pp.dist > (ulong) pp.obj.generic->bitmap.size * usub_scale ||
	  avoid == 0) ) {
      if ( r2 ) {
         dv *=  double(cycle) * pp.obj.generic->gmass / grav_constant_2 /
                r2 * dt;
         if ( pp.dist )
            dvMag = grav_constant_1 * sub_scale / pp.dist * double(cycle) * pp.obj.generic->gmass / grav_constant_2 /
                    r2 * dt;
         }
    } else {
      //dv *= -double(cycle) * pp.obj.generic->gmass / sub_scale /
      //  r2 * dt;
      dv = nullVector;
      /* dv = pp.obj.generic->tvel - tvel;
	 CoordinateClass n = now-pp.obj.generic->now;
         if ( n.Mag() )
            n = n * sub_scale / n.Mag(); 
	 CoordinateClass p = n.crossZ();
	 dv = p * -2 * (n.crossMag(dv) / sub_scale) / sub_scale / 4; */
      dvMag = 0.0;
    }
    if (Type() > designed)
      vel += dv * game->planetShipGFactor / planet_ship_g_factor *
	game->planetShipGFactor / planet_ship_g_factor; 
    else
      vel += dv * game->planetPlanetGFactor / planet_planet_g_factor *
	game->planetPlanetGFactor / planet_planet_g_factor;

    /* criterion for setting cycle */
    if (!free) {
      if (dvMag < 20)
	cycle++;
      else if (dvMag > 200 && cycle > 0)
	cycle--;
    }

    /* decide when to reapply gravity again */
    nextGravityFrame = frame + cycle;

    return dv;
  }

  return nullVector;
}

void DesignedClass::Repel(PingPacket& pp) {
  /* repel this object from the target object */

  /* WARNING: always check for the existense of the object pointer pp.obj */
  /* before calling this function                                         */

  short i;
  static CoordinateClass dv;
  static CoordinateClass rhat;

  /* if this ship is warping then we don't want to enforce the speed limit */
  if (IsWarping())
    return;

  for (i=0; i<bus.numPorts; i++)
    if (bus.dockMod[i] && bus.dockMod[i]->workPar[0] == pp.obj.objLong)
      return;
  if ( IsValidObjectPointer(pp.obj.generic,__FILE__,__LINE__) && 
       pp.dist < (ulong) (bitmap.size + pp.obj.generic->bitmap.size) * usub_scale) {
    double dx = pp.disp.x / sub_scale;
    double dy = pp.disp.y / sub_scale;
    rhat = pp.disp;
    rhat *= sub_scale / (pp.dist+1.0);
  
    dv = rhat * nlong((-50*bitmap.size) /
		      (dx*dx + dy*dy + bitmap.size*bitmap.size) * dt);
    
    vel.AddSubjectToLimit(dv, maxSpeed);

  }
}

void DesignedClass::AvoidCore(PingPacket& pp) {
  /* repel this object from the target object */

  static CoordinateClass dv;
  static CoordinateClass rhat;

  /* if this ship is warping then we don't want to enforce the speed limit */
  if (IsWarping())
    return;

  if (bus.impulseMod && bus.impulseMod->func > module_break_limit) {
    double dx = pp.disp.x / sub_scale;
    double dy = pp.disp.y / sub_scale;
    rhat = pp.disp;
    rhat *= sub_scale / (pp.dist+1.0);
  
    dv.initialize();

    slong thr;
    if (bus.boostMod && bus.boostMod->active)
      thr = max((100 + bus.boostMod->cPar(2, bus.CalculateSurplusPower())) *
		bus.impulseMod->fPar(0) / 100,
		0);
    else
      thr = bus.impulseMod->fPar(0);

    /* do not allow the acceleration to exceed the game state maximum */
    /*                             and                                */
    /* do not allow acceleration to fall below the prescribed minimum */
    slong af = mass*100/frameRate/frameRate;
    thr = max(min(thr, game->maxAcc * af/100), 0);

    if ( mass )
       dv.AddSubjectToLimit(rhat * nlong((-50*mass) /
	   			      (dx*dx + dy*dy + 
		   		       bitmap.size*bitmap.size) * dt),
			     thr * sub_scale* dt/mass);

    vel.AddSubjectToLimit(dv, maxSpeed);

    throttle = 1;
  }
}

CoordinateClass& GenericClass::VelKep(PingPacket& pp) {
  double dx = double(pp.disp.x) / sub_scale;
  double dy = double(pp.disp.y) / sub_scale;
  static CoordinateClass rhat;
  static CoordinateClass kepVel;

  rhat = pp.disp;
  if (pp.dist)
    rhat *= grav_constant_1 / (pp.dist);
  else
    rhat = nullVector;

  double r = max(sqrt(dx*dx + dy*dy), 1.0);
  kepVel = rhat.crossZ();
  if ( IsValidObjectPointer(pp.obj.generic,__FILE__,__LINE__) )
    {
      if (Type() == ship)
        kepVel *=  game->planetShipGFactor * 
          sqrt(pp.obj.generic->gmass / r) /
          grav_constant_2 / planet_ship_g_factor;
      else
        kepVel *= game->planetPlanetGFactor * 
          sqrt(pp.obj.generic->gmass / r) /
          grav_constant_2 / planet_planet_g_factor;
    }
  return kepVel;
}

void DesignedClass::TurnRight() {
  turning = -1;
}

void DesignedClass::TurnLeft() {
  turning = 1;
}
  
void DesignedClass::Straight() {
  turning = 0;
}

PingPacket& GenericClass::Ping(TargetType tt,
			       TargetSelector ts,
			       short n,
			       ulong range,
			       CoordinateClass lead) {
  short i;
  /* ping function for generic objects */
  static PingPacket minPP;
  static PingPacket thisPP;

  minPP.initialize();

  if (ts == nearest || ts == all) {
    if (tt & planet_target) {
      for (i = 0; i < game->N(GameStateClass::planets); i++) {
        thisPP = minPP.Sniff(this, &game->planet[i], /* range */ pc_long_max);
        if (ts == all && thisPP.dist < (int)max_grav_dist && 
            !parent.generic) {
          /* if within max_grav_dist feel gravity */
          Attract(thisPP, 0, 1);
        }
      }
    }
    if (tt & ship_target) {
      for (i = 0; i < game->N(GameStateClass::ships); i++) {
	if (game->ship[i].IsAvailable())
	  minPP.Sniff(this, &game->ship[i], range);
      }
    }
    if ((tt & parent_target) && IsValidObjectPointer(parent.generic,__FILE__,__LINE__) ) {
      minPP = PingPacket(this, parent.generic, pc_long_max);
    }
    if (tt & star_target) {
      for (i=0; i<game->numPlanets; i++) {
	if (game->planet[i].minor == -1) {
	  thisPP = minPP.Sniff(this, &game->planet[i], range);
	}
      }
    }
  } else {
    switch (tt) {
    case planet_target:
      minPP = PingPacket(this,&game->planet[n], 0, lead);
      break;
    case ship_target:
      if (game->ship[n].IsAvailable())
	minPP = PingPacket(this,&game->ship[n], 0, lead);
      break;
    case no_target:
    default:
      break;
    }
  }
  return minPP;
}

short ShipClass::IsAvailableToHelp() {
  return IsAvailable() &&
    !(duty == colonize_detail && game->rank[game->player[playerIndex].rank].dutyInterval <= forget_interval);
}

short DesignedClass::SpawnBeam(ModuleClass* m, PingPacket* overridePP) {
  /**********************************************************/
  /* fire a beam weapon.  Check for the following:          */
  /* o   a valid target                                     */
  /* o   the target is in range                             */
  /* o   the target is not friendly                         */
  /* o   the target is visible                              */
  /* o   the target is not exploding                        */
  /* o   the owning player is NOT a demo player             */
  /*                                                        */
  /* The enery requirements and firing arc restrictions     */
  /* have already been satisfied.                           */
  /**********************************************************/
  /* the work parameters settings are the same as the dock. */
  /* workPar[0] is a long pointer to the beam's target.     */
  /* workPar[1] is a 0/1 switch that determines the beam's  */
  /*            active state.                               */
  /**********************************************************/

  //if ( SF_DebugMem_Check(this,"DesignedClass::SpawnBeam") )
  //  return 0;

  static CoordinateClass tmp;
  static char msg[1024];
  PingPacket* pp;

  m->workPar[1] = 0;

  if ( !overridePP ) 
    pp = &targetPP;
  else
    pp = overridePP;
  
  if ( !IsValidObjectPointer(pp->obj.generic) )
    {
      return 0;
    }

  if ( m->host.ship->playerIndex < 0 || 
       m->host.ship->playerIndex >= game->maxStoredPlayers )
    {
      return 0;
    }

  if ( Type()==ship && 
       game->player[m->host.ship->playerIndex].IsDemoPlayer() )
    {
      return 0;
    }

  /****************************************************************/
  /* if the host ship has beam weapons, make sure the distance to */
  /* target is updated for the range checking in SpawnBeam        */
  /****************************************************************/
  if ( !pp->frame != game->frame && 
       IsValidObjectPointer(pp->obj.generic,__FILE__,__LINE__) ) {
    pp->disp = pp->obj.generic->now - now;
    pp->dist = pp->disp.Mag(0);
    pp->frame = game->frame;
  }

  //
  // Don't let laser happen between ships if one is in the
  // Internet Cafe.
  //
  if ( IsInInternetCafe(m->host.ship) ||
       IsInInternetCafe(pp->obj.generic) )
    {
      return 0;
    }

  if ( IsWarping() ) {
    if ( IsLocalPlayer(m->host.ship) && 
	 !m->CheckTubeMode(ModuleClass::auto_fire_mode) )
      {
	strcpy(msg, 
	       "1st Officer> laser deactivated: Target has engaged Q drive.");
	game->PostGameMessage(msg, 
			      game->ObjectListIndex(this), 
			      (short)raceID, personal, 0);
      }
    return 0;
  }

  if ( IsValidObjectPointer(pp->obj.generic,__FILE__,__LINE__) &&
       pp->obj.generic->IsFriendlyTo(this) )
    {
      if (IsLocalPlayer(m->host.ship) && 
	  !m->CheckTubeMode(ModuleClass::auto_fire_mode)) {
	strcpy(msg, "1st Officer> laser deactivated: the target is friendly.");
	game->PostGameMessage(msg, game->ObjectListIndex(this), 
			      (short)raceID, personal, 0);
      }
      return 0;
    }
  
  if (pp->dist > (ulong) m->par[2] * usub_scale)
    {
      if ( IsLocalPlayer(m->host.ship) && 
	   !m->CheckTubeMode(ModuleClass::auto_fire_mode) )
	{
	  strcpy(msg, 
		 "1st Officer> laser deactivated: Target is out of range.");
	  game->PostGameMessage(msg, 
				game->ObjectListIndex(this), 
				(short)raceID, personal, 0);
	}
      return 0;
    }

  if ( IsValidObjectPointer(pp->obj.generic,__FILE__,__LINE__) &&
       pp->obj.generic->IsExploding() ) {
    if ( IsLocalPlayer(m->host.ship) && 
	 !m->CheckTubeMode(ModuleClass::auto_fire_mode) )
      {
	strcpy(msg, "1st Officer> laser deactivated: the target is exploding");
	game->PostGameMessage(msg, 
			      game->ObjectListIndex(this), 
			      (short)raceID, personal, 0);
      }
    /* reset the target */
    pp->obj.generic = 0;
    return 0;
  }

  if ( IsValidObjectPointer(pp->obj.generic,__FILE__,__LINE__) &&
       !pp->obj.generic->CanBeSeenBy(this) )
    {
      if ( IsLocalPlayer(m->host.ship) && 
	   !m->CheckTubeMode(ModuleClass::auto_fire_mode) )
	{
	  strcpy(msg, "1st Officer> laser deactivated: Target is not visible");
	  game->PostGameMessage(msg, 
				game->ObjectListIndex(this), 
				(short)raceID, personal, 0);
	}
      return 0;
    }
  
  m->workPar[0] = pp->obj.objLong;
  m->workPar[1] = 1;
  
  slong maxDist = max(m->par[2] * sub_scale, 1);
  
  /* apply damage */
  slong r = max(1, pp->dist/sub_scale);
  tmp = pp->disp / r;
  if ( IsValidObjectPointer(pp->obj.generic,__FILE__,__LINE__) )
    {
      slong factor = bus.numBeams;
      if ( factor < 1 )
	{
	  factor = 1;
	}
      pp->obj.generic->Hit(maxDist / ((maxDist + pp->dist) / 100) *
	                   m->par[1] / energy_to_damage / factor, 
			   this, tmp, 0, 0, 
		           beam_hit + bypass_armor_hit);
    } 
 
  return 1;
}

short
DesignedClass::Spawn( MissileDesignStruct& md,
		      slong* par, 
		      slong altModID, 
		      slong initialOffset,
		      GenericClass* targetOverride )
{
  
  /* create new missile from designed class object */
  
  short newSlot = -1;
  MissileClass* m;
  
  if (altModID == 0 || game->numMissiles < game->maxMissiles / 2)
    {
      newSlot = game->NewObject(&m);
    }

  if (newSlot != -1)
    {
      
      m->initializeWith(game->TypeListIndex(m), *this, md, par, altModID);
      if (m->bus.moduleList.rootOf() == 0)
	{
	  /************************************************************/
	  /* we were unable to allocate memory for the module list    */
	  /* so record an error message and exit with a 0 return code */
	  /************************************************************/
	  logFile <<
	    "ERROR> unable to allocate memory for new missile with " << 
	    m->bus.numMod <<
	    " slots on frame " << 
	    game->frame << endl;
	  game->DestroyObject(m);
	  return 0;
	}
      
      if (initialOffset != -1)
	{
	  m->offset = char(initialOffset % m->bitmap.numFrames);
	}
      
      m->raceID = raceID; /* in case of hostile takeover */
      
      m->bus.BudgetPower();
      if ( IsValidObjectPointer(parent.generic,__FILE__,__LINE__) && 
	   parent.generic->Type() == planet &&
	   m->design->actRange && 
	   m->design->initialVelocityDisplacement == 0)
	{
	  m->vel = nullVector;
	  m->lockObj = parent;
	}
      if (/*!m->parentLock*/ !m->lockObj.generic)
	{
	  if (IsValidObjectPointer(targetPP.obj.ship, __FILE__, __LINE__) &&
	      !targetPP.obj.generic->IsFriendlyTo(this))
	    {
	      
	      if (!targetOverride)
		{
		  m->targetPP.obj.ship = targetPP.obj.ship;
		}
	      else
		{
		  m->targetPP.obj.generic = targetOverride;
		}
	      
	      m->targetLock = 1;
	      
	      /**************************************************************/
	      /* if the the parent ship has targeted a missile then enable  */
	      /* missile-to-missile triggering                              */
	      /**************************************************************/
	      if ( m->targetPP.obj.generic->Type() == missile || 
		   targetOverride && 
		   targetOverride->Type() == missile )
		{
		  m->triggerObjType = m->triggerObjType | missile_target;
		}
	    } else
	      {
		m->Acquire();
	      }
	}
      
      m->vel.AddSubjectToLimit(nullVector, m->maxSpeed);
      
      m->activate();
      
    }
  return newSlot;
}


void DesignedClass::SpawnRandomModules(short n, CoordinateClass* d) {

  slong par[3];

  if (!bus.factoryMod)
    return;
  
  short i;
  if (game->tech[raceID].nSet)
  for (i=0; i<n; i++) {

    short index =
      game->tech[raceID].
      get(game->Random(game->tech[raceID].nSet, frame+i) + 1);

    /* if a direction is supplied then spawn in that direction */
    if (d)
      par[0] = d->Heading(game->maxAngles);
    else /* otherwise pick a random direction */
      par[0] = (index + i) % game->maxAngles;

    par[2] = par[0];
    par[1] = bitmap.size / 2;
    
    if (game->moduleList[index].moduleFunc != ModuleClass::factory &&
	game->moduleList[index].moduleFunc != ModuleClass::power_plant &&
	game->moduleList[index].moduleFunc != ModuleClass::fuel_cell) {
      if (Spawn(game->missileDesign[0], par,
		game->moduleList[index].id) != -1) {
      
	/* reset production total to zero */
	bus.factoryMod->specPar[0] = 0;
	bus.factoryMod->specPar[1] = 0;
      }
    }
  }
}
  
void ColonyClass::SpawnToDock(ShipClass* s) {

  if ( !IsDisconnectedClient() && !IsServer() ) {
    return;
  }

  logFile << "note> ColonyClass::SpawnToDock BEGIN" << endl;

  if ( !s ) {
    logFile << "s == NULL" << endl;
    return;
  }

  logFile << "TRY> a ";
  s->ShortProfile();
  logFile << endl;
  logFile << " trying to pick up a colony from " << endl;

  if ( !lockObj.planet ) {
    logFile << "FAILED> lockObj.planet = FALSE" << endl;
    return;
  }

  lockObj.planet->ShortProfile();
  logFile << " on frame " << game->frame << endl;

  if ( IsPending(spawning) ) {
    logFile << "FAILED> IsPending(spawning) == TRUE" << endl;
    return;
  }

  if ( game->colonyPending ) {
    logFile << "FAILED> game->colonyPending == TRUE" << endl;
    return;
  }

  if ( !IsFriendlyTo(s) ) {
    logFile << "FAILED> IsFriendlyTo(s) == FALSE" << endl;
    return;
  }

  /* 2001-04-14 - disabled this condition to allow multiple colonies to be joined */
  /*              while in dock.                                                  */
  //if ( !s->bus.IsDockFree(0) ) {
  //  logFile << "FAILED> IsDockFree(0) == FALSE" << endl;
  //  return;
  //}

  if ( !game->PlanetIsSpawnable(lockObj.planet,game) ) {
    logFile << "FAILED> game->PlanetIsSpawnable() = FALSE" << endl;
    return;
  }

  logFile << "SUCCEEDED> SendSpawnMsg executed" << endl;
  logFile << "note> ColonyClass::SpawnToDock END" << endl;

  PopStruct newPop;
  
  if ( s->bus.dockMod[0] && 
       s->bus.numPorts > 0 &&
       s->bus.dockMod[0]->workPar[1] && 
       s->bus.dockMod[0]->workPar[0] ) {
    ShipClass* dockedObj = (ShipClass*) s->bus.dockMod[0]->workPar[0];
    
    if (dockedObj->Type() == colony) {
      /* a colony is already docked with this ship.  The slot and population of */
      /* the new colony should match the old one plus whatever is added.        */
      
      newPop = Min(lockObj.planet->OptimalPopulation(game->race[raceID]) / 10,
		   PopStruct((game->maxColonyMillions >= 1000)*
			     (short)(game->maxColonyMillions/1000),
			     (game->maxColonyMillions % 1000) * 1000000));
      
      SendSpawnMsg(newPop, (short)raceID, game->ObjectListIndex(s), game->TypeListIndex(dockedObj));

    }
    
  } else {
  
    newPop = Min(lockObj.planet->OptimalPopulation(game->race[raceID]) / 10,
		 PopStruct((game->maxColonyMillions >= 1000)*
			   (short)(game->maxColonyMillions/1000),
			   (game->maxColonyMillions % 1000) * 1000000));

    SendSpawnMsg(newPop, (short)raceID, game->ObjectListIndex(s));
    
  }
  
  //} /* else write a message explaining why it failed */
  //}
}

void ColonyClass::SendSpawnMsg(PopStruct p, short iRace, short iSlot, short newSlot) {

  /* if newSlot is passed with a non-zero value, then it should be used in place of */
  /* one returned by the Spawn method.  It is passed with a non-zero value when the */
  /* requesting ship already has colony[newSlot] docked in port 0.                  */

  /* just in case */
  if (IsPending(spawning) || game->colonyPending) {
    logFile << "ERROR> colony " << game->colony.indexOf(this) << " sent an " <<
      "illegal spawn msg on frame " << game->frame << endl;
    ClearPendingFlag(spawning);
    return;
  }

  /*
    logFile << "SendSpawnMsg: " << (ushort)iSlot << " " <<
    (ushort)game->ObjectListIndex(this) << " " <<
    (ushort)iRace << " " <<
    (ulong)p.billions << " " <<
    (ulong)p.lBillions <<  " " << game->frame << endl;
  */

  short thisSlot = (ushort)game->ObjectListIndex(this);
  //newSlot = 0;
  if (iSlot == -1 || 
      (IsValidObjectPointer(game->objectList[iSlot],__FILE__,__LINE__) &&
       game->objectList[iSlot]->Type() == ship) ) {

    if (p.billions || p.lBillions) {
      /* this indicates that a colony is spawning a new colony */
    
      if (!newSlot)
	newSlot = Spawn(p, iRace, iSlot); /* note: assumes maxColonies <= 255 */
      else
	Spawn(p, iRace, iSlot, newSlot);
      
      if (newSlot == -1) {
	logFile << "ERROR> unable to find open colony slot on frame " << game->frame << endl;
	logFile << "note> at time of error, there were " << game->numColonies <<
	  " colonies out of a possible " << game->maxColonies - game->extraColonies << endl;
	return;
      }

    }

    if (iSlot == -1)
      iSlot = thisSlot;

  }
  
  SetPendingFlag(spawning);
  game->colonyPending = 1;

  numSpawnMessagesSent++;

  ObjectUnion s;
  s.generic = game->objectList[iSlot];

  if ( IsValidObjectPointer(game->objectList[iSlot],__FILE__,__LINE__) )
    {
      
      switch (game->objectList[iSlot]->Type()) {
	
      case ship:
	
	logFile << "sent> (" << numSpawnMessagesSent << " sent) a ";
	s.ship->ShortProfile();
	logFile << " has picked up a ";
	game->colony[newSlot].ShortProfile();
	logFile << " from ";
	ShortProfile();
	logFile << " on frame " << game->frame << endl;
	
	break;
	
      case colony:
	
	if (iSlot == thisSlot) {
	  
	  logFile << "sent> (" << numSpawnMessagesSent << " sent) a ";
	  game->colony[newSlot].ShortProfile();
	  logFile << " was spawned by a ";
	  ShortProfile();
	  logFile << " on frame " << game->frame << endl;
	  
	} else {
	  
	  if ( IsValidObjectPointer(s.generic,__FILE__,__LINE__) &&
	       !s.generic->IsFriendlyTo(this)) {
	    
	    if ( IsValidObjectPointer(s.colony->lockObj.generic,__FILE__,__LINE__) ) {
	      
	      logFile << "sent> (" << numSpawnMessagesSent << " sent) a ";
	      ShortProfile();
	      logFile << " has defeated a ";
	      game->objectList[iSlot]->ShortProfile();
	      logFile << " on frame " << game->frame << endl;
	      
	    } else {
	      
	      logFile << "sent> (" << numSpawnMessagesSent << " sent) a ";
	      ShortProfile();
	      logFile << " has conquered a ";
	      game->objectList[iSlot]->ShortProfile();
	      logFile << " on frame " << game->frame << endl;
	      
	    }
	    
	  } else {
	    
	    logFile << "sent> (" << numSpawnMessagesSent << " sent) a ";
	    ShortProfile();
	    logFile << " has absorbed a ";
	    game->objectList[iSlot]->ShortProfile();
	    logFile << " on frame " << game->frame << endl;
	    
	  }
	  
	}
	
	break;
	
      case planet:
	
	logFile << "sent> (" << numSpawnMessagesSent << " sent) a ";
	ShortProfile();
	logFile << " has landed on ";
	s.planet->ShortProfile();
	logFile << " on frame " << game->frame << endl;
	
	break;

      default :
	break;	
      }
    }
  
  /* send a message to create a new colony */
  retMsgBuffer.PushMsg((uchar)SF_MSG_SPAWNCOLONY,
		       (ushort)iSlot, /* index of ship with dock */
		       (ushort)thisSlot,
		       (uchar)newSlot,
		       (ushort)iRace,
		       (ulong)p.billions,
		       (ulong)p.lBillions,
		       (int)S_CLIENT_TCP);
}

short ColonyClass::Spawn(PopStruct p, short iRace, short iSlot, short newSlot) {
  /* create new colony from colony class object */

  short isNew;
  ColonyClass* c;
  ShipClass* dockedObj;
  static char msg[1024];

  /* reset spawn flag */
  ClearPendingFlag(spawning);


  if (iSlot != -1 && 
      game->ship[iSlot].bus.dockMod &&
      game->ship[iSlot].bus.dockMod[0] &&
      game->ship[iSlot].bus.numPorts > 0 ) {
    
    /* if the iSlot object is a ship and that ship already has the newSlot 
       colony docked at port 0 then add the new population to it.
    */
    
    dockedObj = (ShipClass*) game->ship[iSlot].bus.dockMod[0]->workPar[0];
    
  } else
    dockedObj = 0;
  
  if (dockedObj && dockedObj == game->colony + newSlot) {

    c = game->colony + newSlot;

    if (localSlotNumber == iSlot) {
      
      strcpy(msg, "1st Officer> We have picked up additional colonists.");
      
      game->PostGameMessage(msg, iSlot, (short)raceID, personal, 0);
      
    }
    
    /* combine the colonies and destroy the new one */
    
    PopStruct newPop = dockedObj->pop + p;
    
    PopStruct maxPop((game->maxColonyMillions >= 1000)*
		     (short)(game->maxColonyMillions/1000),
		     (game->maxColonyMillions % 1000) * 1000000);

    if (pop >= p) {
      c->pop = p;
      pop -= p;
      
      /* make sure that the population is not negative */
      if (pop < PopStruct())
	pop = PopStruct(0,1);
      
    } else {
      c->pop = pop/2;
      pop -= c->pop;
      
      /* make sure that the population is not negative */
      if (pop < PopStruct())
	pop = PopStruct(0,1);
      
    }
    
    /* return the excess population to the spawning colony */
    if (newPop > maxPop) 
      pop += newPop - maxPop;
    
    dockedObj->pop = Min(newPop, maxPop);

    isNew = 1;

  } else {
  
    isNew = game->NewObject(&c, newSlot) != -1;
    
    if (isNew) {
      
      /* first call the constructor */
      c->initialize();
      
      /* set the design */
      c->design = design;
      
      /* then initialize based on the design */
      if (c->InitializeFromDesign(c)) {
	/* decrement the colony counter */
	game->numColoniesCreated--;
	return -1;
      }
      
      c->stat = idle;
      c->viewObj.colony = c;
      c->raceID = iRace;
      c->playerIndex = 0;
      //strcpy(c->design->name, "Colony");

      if (pop >= p) {
	c->pop = p;
	pop -= p;
      
	/* make sure that the population is not negative */
	if (pop < PopStruct())
	  pop = PopStruct(0,1);

      } else {
	c->pop = pop/2;
	pop -= c->pop;

	/* make sure that the population is not negative */
	if (pop < PopStruct())
	  pop = PopStruct(0,1);
      
      }

      c->now = now;
      c->gnow = c->now;
      c->vel = vel;
      c->tvel = tvel;

      c->lockObj.generic = 0;
      c->parent.generic = lockObj.generic;

      sprintf(c->name, c->design->name);
    
      //c->now.y += lockObj.planet->bitmap.size * 3 * sub_scale / 2;

      c->vel.AddSubjectToLimit(nullVector, c->maxSpeed);

      c->activate();

      if (iSlot != -1) {

	if (localSlotNumber == iSlot) {

	  strcpy(msg, "1st Officer> We have picked up a colony.  Transport it to a planet and press \"/\" (slash) to drop it.");
	
	  game->PostGameMessage(msg, iSlot, (short)raceID, personal, 0);

	}

	if (game->ship[iSlot].bus.numPorts) {
	  
	  game->ship[iSlot].
	    bus.dockMod[0]->workPar[0] = slong(c);
	  game->ship[iSlot].
	    bus.dockMod[0]->AssignPortToShip(c);
	  /* the parent object should be reset so that the spawned colony */
	  /* can be dropped back on the original colony                   */
	  c->parent.generic = 0;
	  /* the lastDockObjIndex should be set so that the player will get */
	  /* credit for dropping it on a planet                             */
	  c->lastDockObjIndex = iSlot;
	  /* reset the frame counter so that we can tell when */
	  /* lastDockObjIndex should expire                   */
	  c->frame = 1;
	  
	  game->ship[iSlot].PlaySoundOnScreen(game->soundEvent[pickup_colony_event]);
	  
	} else {
	  logFile << "ERROR> a ";
	  game->ship[iSlot].ShortProfile();
	  logFile << " received a bad SpawnToDock msg from a ";
	  ShortProfile();
	  logFile << " on frame " << game->frame << endl;
	  
	}
      }

      if (iRace == raceID) {

	if (iSlot == -1)
	  PlaySoundOnScreen(game->soundEvent[free_colony_spawned_event]);

	sprintf(msg, "Colony departing %s->%s",
		game->planet[planetIndex].name,
		game->planet[c->PlanetAdvisor()].name);
      
	//logFile << msg << " on frame " << game->frame << endl;
      
	game->PostGameMessage(msg, (short)raceID, (short)raceID, broadcast, 0);
      } else {
	/* code to handle defections */

	PlaySoundOnScreen(game->soundEvent[defection_event]);
      
	if ( IsValidObjectPointer(lockObj.generic,__FILE__,__LINE__) ) {
	
	  sprintf(msg, "%s colony on %s defected to %ss.", 
		  game->race[raceID].name, 
		  lockObj.planet->name,
		  game->race[iRace].name);

	  logFile << "note> a " << game->race[raceID].name << " colony, located on " <<
	    lockObj.planet->name << ", defected to the " << game->race[iRace].name <<
	    "s on frame " << game->frame << endl;

	} else {
	
	  sprintf(msg, "%s colony defected to %ss.", 
		  game->race[raceID].name,
		  game->race[iRace].name);

	  logFile << "a " << game->race[raceID].name << " colony defected to the " << game->race[iRace].name <<
	    "s on frame " << game->frame << endl;
	
	}
      
	AttackObj(c);
      
	game->PostGameMessage(msg, 0, 0, broadcast, 1);
      
      }
    } else { /* there was no room in the colony list */

      if (iRace != raceID) {
	/* there is no room in the colony list so just switch sides */
	raceID = iRace;

	PlaySoundOnScreen(game->soundEvent[revolution_event]);

	if ( IsValidObjectPointer(lockObj.generic,__FILE__,__LINE__) )
	  lockObj.planet->raceID = iRace;
	ConvertConstructionProject();
      }
    }
  }

  if (isNew) {

    logFile << "note> a ";
    c->ShortProfile();
    logFile << " was spawned by a ";
    ShortProfile();
    logFile << " on frame " << game->frame << endl;
    
    return(game->colony.indexOf(c));
  } else {
    
    logFile << "WARNING> unable to spawn new colony from a ";
    ShortProfile();
    logFile << " on frame " << game->frame << endl;

    return -1;
  }
}

void ColonyClass::ConvertConstructionProject() {
  
  /* if this object was building something convert it */
  if (bus.factoryMod && bus.factoryMod->workPar[0]) {
    ObjectUnion buildObj;
    buildObj.objLong = bus.factoryMod->workPar[0];
    buildObj.generic->raceID = raceID;
    game->player[buildObj.ship->playerIndex].raceId = raceID;
  }
  
}

void ColonyClass::TransferConstructionProjectTo(ColonyClass* c) {
  
  /*******************************************************************/
  /* if this object was building something and the colony [c] is not */
  /* then transfer the construction project to [c].  Note: this nor- */
  /* mally precedes a consolidation.                                 */
  /*******************************************************************/

  if (c &&
      bus.factoryMod && bus.factoryMod->workPar[0] &&
      c->bus.factoryMod && !c->bus.factoryMod->workPar[0]) {
    
    ObjectUnion buildObj;
    buildObj.objLong = bus.factoryMod->workPar[0];

    short i;
    for (i=0; i < bus.factoryMod->nWPar; i++) {
      c->bus.factoryMod->workPar[i] = bus.factoryMod->workPar[i];
      bus.factoryMod->workPar[i] = 0;
    }

    /* just in case, change the raceID */
    buildObj.generic->raceID = c->raceID;
    game->player[buildObj.ship->playerIndex].raceId = c->raceID;

  }
  
}

slong ColonyClass::Crowding() {

  /******************************************************************************/
  /* this factor reflects a planet's inhabitant's unhappiness from overcrowding */
  /* conditions.  This factor should not contribute until the conditions reach  */
  /* greater than 100%.  It should then rise until it reaches 100% at 200% of   */
  /* optimal, which should stop the growth rate of the population               */
  /******************************************************************************/

  slong r = pop.InUnitsOf(10000) /
    max(lockObj.planet->OptimalPopulation(game->race[raceID]).InUnitsOf(1000000), 1) - 100;
  
  return (r > 0)?(min(r*r/100, 200)):0;
}

slong ColonyClass::Loyalty() {
  if ( IsValidObjectPointer(lockObj.generic,__FILE__,__LINE__) ) {
    return 2*lockObj.planet->Habitability(game->race[raceID]) - Crowding();
  }
  return 100;
}

short ShipClass::PlanetAdvisor(advice a) {
  short i, iMax = 0;
  slong ratio, maxRatio = 0, maxPopRatio = 0;
  PopStruct cPop, oPop;
  slong rangeFactor = 0, range;

  switch (a) {

  case planet_to_colonize: {

    /* to pick a planet to colonize:                                       */
    /* o  pick the planet with the smallest population to optimal ratio    */
    /*    divided by distance                                              */
    /* o  start with friendly planets; if none are found with a population */
    /*    ratio of less than one then search for enemy planets             */

    if (game->race[raceID].targetIndex >= 0 &&
	game->race[raceID].targetIndex < game->numPlanets &&
	game->planet[game->race[raceID].targetIndex].OptimalPopulation(game->race[raceID]).InUnitsOf(1000))
      return game->race[raceID].targetIndex;
    
    for (i=1; i<game->numPlanets; i++) {
      if ((game->planet[i].IsFriendlyTo(this) || !game->planet[i].colony) &&
	  game->planet[i].OptimalPopulation(game->race[raceID]).
	  InUnitsOf(1000) &&
	  (parent.planet != game->planet+i && Type() == colony ||
	   Type() == ship)) {
	if (game->planet[i].colony) {
	  cPop = game->planet[i].colony->pop;
	  if (cPop.billions == 0)
	    {
	      cPop.SetStat(cPop.lBillions,max(cPop.lBillions, 1000000));
	      //cPop.lBillions = max(cPop.lBillions, 1000000);
	    }
	} else
	  cPop = PopStruct(0,1000000);
	
	oPop = game->planet[i].OptimalPopulation(game->race[raceID]);
	
	if (oPop.billions || oPop.lBillions) {
	  
	  range = (game->planet[i] - *this).Mag(0);

	  rangeFactor = max(range / sub_scale / 100, 10) + 1;
	  
	  ratio = oPop.InUnitsOf(100) / (cPop.InUnitsOf(10000)+1);
	  
	} else 
	  ratio = 0;
	if (ratio * 10 / rangeFactor > maxRatio) {
	  maxRatio = ratio * 10 / rangeFactor;
	  maxPopRatio = ratio;
	  iMax = i;
	}
      }
    }
    
    /* skip if no friendly colony is less than 80% full */
    if (iMax && maxPopRatio > 10000 / max(game->race[raceID].fullColonyPct, 1))
      return iMax;
    
    iMax = 0;
    maxRatio = -1;
    
    for (i=1; i<game->numPlanets; i++) {
      if (!game->planet[i].IsFriendlyTo(this) &&
	  game->planet[i].OptimalPopulation(game->race[raceID]).
	  InUnitsOf(1000)) {
	if (game->planet[i].colony) {
	  cPop = game->planet[i].colony->pop;
	  if (cPop.billions == 0)
	    {
	      cPop.SetStat(cPop.lBillions,max(cPop.lBillions, 1000000));
	      //cPop.lBillions = max(cPop.lBillions, 1000000);
	    }
	} else
	  cPop = PopStruct(0,1000000);
	
	oPop = game->planet[i].OptimalPopulation(game->race[raceID]);
	
	if (oPop.billions || oPop.lBillions) {

	  range = (game->planet[i] - *this).Mag(0);
	  
	  rangeFactor = max(range / sub_scale / 100, 10) + 1;
	  
	  ratio =  oPop.InUnitsOf(100) / (cPop.InUnitsOf(10000)+1);
	  
	} else 
	  ratio = 0;
	if (ratio * 10 / rangeFactor > maxRatio) {
	  maxRatio = ratio * 10 / rangeFactor;
	  iMax = i;
	}
      }
    }
    return iMax;

  }
  case planet_to_harvest: {

    /* to pick a planet to harvest:                                        */
    /* o  pick the planet with the largest population to optimal ratio     */
    /*    divided by distance                                              */

    if (game->race[raceID].harvestIndex >= 0 &&
	game->race[raceID].harvestIndex < game->numPlanets &&
	game->planet[game->race[raceID].harvestIndex].OptimalPopulation(game->race[raceID]).InUnitsOf(1000))
      return game->race[raceID].harvestIndex;
    else {
      return game->GetMaxPopRatioPlanetIndex(raceID);
    }
  }
  }

  return 0;
}

void
DesignedClass::Displace(const DesignedClass& g, slong v, slong* par)
 {
   
   /********************************************/
   /* displace object position and velocity -- */
   /* needed to move missiles away from center */
   /* of ship with a higher velocity           */
   /********************************************/
   
   static CoordinateClass nhat;
   
   /*******************************************************************/
   /* par[1] determines how far from the center the object is placed. */
   /* a null value should default to the bitmap edge                  */
   /*******************************************************************/
   slong h = par[1];
   if ( !h )
     {
       h = g.bitmap.size / 2;
     }
   
   slong numAngles = g.bitmap.numFrames;
   
   if (g.bitmap.IsSpin())
     {
       nhat = g.bitmap.Dir((short)g.heading);
       vel += g.bitmap.Dir((short)(g.heading + par[2])) * v;
       now += g.bitmap.Dir((short)(g.heading + par[0])) * h;
       now += 
	 ( nhat * min(g.numLagFrames * (vel / sub_scale * nhat), 
		      max_missile_correction))
	 / sub_scale;

     }
   else
     {
       nhat = Dir[g.heading];
       vel += Dir[(g.heading + par[2] + 2 * numAngles) % numAngles] * v;
       now += Dir[(g.heading + par[0] + 2 * numAngles) % numAngles] * h;
       now += (nhat * min(g.numLagFrames * (vel / sub_scale * nhat), 
			  max_missile_correction))
	 / sub_scale;
     }
 }

short PlanetClass::get(ifstream& f,
		       SFString* nameList, SFString* valueList,
		       short nList) {
  char Class[max_name_char];
  char iPhaseStr[max_name_char];
  char sRad[max_name_char];
  double rad = 0.0;
  char bitmapIndexStr[max_name_char];
  short bitmapIndex = 0;
  char c;
  const double pi = 3.14159265;
  double dmass, datmP, dgrav;
  slong ltemp, hp, ab, tf;
  double period;
  static CoordinateClass cOffset;
  static PingPacket pp;


  f >> c;
  if (c != '}') {
    f.putback(c);
    f >> Class >> name >> sRad >> iPhaseStr >> dmass >> datmP >> bitmapIndexStr >>
      ltemp >> hp >> dgrav >> ab >> tf;

    if (nameList && valueList && nList) {

      Parse(sRad, nameList, valueList, nList);
      Parse(iPhaseStr, nameList, valueList, nList);

    }

    rad = atof(sRad);

    if (!strcmp(bitmapIndexStr, "*"))
      strcpy(bitmapIndexStr, name);

    bitmapIndex = game->IndexOfBitmap(bitmapIndexStr, planet, raceID);

    bitmap = game->objectBitmapList[bitmapIndex];
    bitmap.init();

    /* transfer shorts to longs */
    temp = (short)ltemp;
    hydpc = (char)hp;
    accBonus = (char)ab;
    terraForm = tf * terraform_base;

    atmP = short(datmP * 100.00001);
    grav = short(dgrav * 100.00001);

    lastBitmap = bitmap;
    if (strcmp(Class,"Star") == 0) {
      minor = -1;
      mass = nlong(dmass * 3e7);
      gmass = nlong(dmass * 1e4);
      vel = nullVector;
      parent.generic = 0;

      orbitalRadius = nlong(rad * au_to_pixel);
      period = 0; //1.0/sqrt(rad*rad*rad);

      /* convert the initial Angle to a float */
      if (strcmp(iPhaseStr,"*"))
	{
	  iPhase = (slong)(atof(iPhaseStr) / 180.0 * pi * 10000);
	}
      else
	{
	  iPhase = 0;
	  if ( period != 0.0 )
	    {
	      iPhase = 
		nlong( ((game->year / period) - 
			nlong(game->year / period)) * 2 * pi * 10000 );
	    }
	}
      
      now.initializeWith(nlong(orbitalRadius * cos(double(iPhase)/10000.0)),
			 nlong(orbitalRadius * sin(double(iPhase)/10000.0)));

      game->lastStar = game->numPlanets;

    } else if (strcmp(Class,"Planet") == 0 || strcmp(Class,"Minor") == 0) {
       minor = 2*(strcmp(Class,"Minor") == 0);
       mass = nlong(dmass * 1000);
       gmass = bitmap.size * 10;
       orbitalRadius = nlong(rad * au_to_pixel);
       period = 1.0/sqrt(rad*rad*rad);

       /* convert the initial Angle to a float */
       if (strcmp(iPhaseStr,"*"))
	 {
	   iPhase = (slong)(atof(iPhaseStr) / 180.0 * pi * 10000);
	 }
       else
	 {
	   iPhase = 0;
	   if ( period != 0.0 )
	     {
	       iPhase = nlong(((game->year / period) - 
			       nlong(game->year / period)) * 2 * pi * 10000);
	     }
	 }

       parent.planet = game->planet + game->lastStar;

       cOffset.initializeWith(nlong(orbitalRadius*cos(double(iPhase)/10000.0)),
			      nlong(orbitalRadius*sin(double(iPhase)/10000.0)));

       now = parent.generic->now + cOffset;


       gnow = now;
       pp = Ping(parent_target);

       if ( IsValidObjectPointer(pp.obj.generic,__FILE__,__LINE__) ) {
	 while (Attract(pp, 0, 1).Mag(LONG_MAX) < 20) {
	   cycle++;
	   vel = VelKep(pp);
	 }
       } else {

	 vel = nullVector;

       }

       game->lastPlanet = game->N(GameStateClass::planets);

    } else if (strcmp(Class,"Moon") == 0) {

      minor = 1;
      mass = nlong(dmass * 7.3e22 / 6e24 * 1000);
      gmass = bitmap.size * 10;

      parent.planet = game->planet + game->lastPlanet;

      orbitalRadius = nlong(rad * mo_to_pixel);

      period = 1.0;
      if ( rad > 0.0 )
	{
	  period = 1.0/sqrt(rad*rad*rad);
	}

      if (strcmp(iPhaseStr,"*"))
	{
	  iPhase = (slong)(atof(iPhaseStr) / 180.0 * pi * 10000);
	}
      else
	{
	  iPhase = 0;
	  if ( period != 0.0 )
	    {
	      iPhase = 
		nlong( ((game->year / period) - 
			nlong(game->year / period)) * 2 * pi * 10000 );
	    }
	}

      cOffset.initializeWith(nlong(orbitalRadius * cos(double(iPhase)/10000.0)),
			     nlong(orbitalRadius * sin(double(iPhase)/10000.0)));
      
      now = parent.generic->now + cOffset;

      gnow = now;
      pp = Ping(parent_target);

      if ( IsValidObjectPointer(pp.obj.generic,__FILE__,__LINE__) ) {

	while (Attract(pp, 0, 1).Mag(LONG_MAX) < 10) {
	  cycle++;
	}

	vel = VelKep(pp);

      } else {

	vel = nullVector;

      }
    }
    /* if (parent.generic)
      cout << game->N(GameStateClass::planets) << " " << 
        parent.planet-game->planet <<
          " " << Class << " " << name << " " << rad << " " << bitmapIndex <<
            " " << cycle << endl;
    else
      cout << game->N(GameStateClass::planets) << " " << "none" <<
        " " << Class << " " << name << " " << rad << " " << bitmapIndex <<
          endl; */
  }

  if (!f) {
    logFile << "ERROR> PlanetClass::get: input error -- partial line reads:" << endl;
    logFile <<  Class << " " << name << " " << rad << " " << dmass << " " << 
      bitmapIndex << " " << ltemp << " " << hp <<
      " " << " " << ab << endl;
  }

  return (c != '}' && f.good());
}

slong GenericClass::PushTo(GameStateBuffer& gb, GameStateClass* g) {

  WRITE_SF_LOG(logFile << "Pushing generic section of " << TypeName() << " " <<
	       g->TypeListIndex(this) << "..." << endl);

  char* bufferPos = gb.BufferPosition();

  CALL_PUSH_OBJ(gb,now);
  CALL_PUSH_OBJ(gb,vel);
  CALL_PUSH_OBJ(gb,tvel);
  CALL_PUSH_OBJ(gb,gnow);
  CALL_PUSH_OBJ(gb,gvel);
  CALL_PUSH_OBJ(gb,active);
  CALL_PUSH_OBJ(gb,offset);
  CALL_PUSH_OBJ(gb,frame);
  CALL_PUSH_OBJ(gb,nextGravityFrame);
  CALL_PUSH_OBJ(gb,bitmapStartFrame);
  CALL_PUSH_OBJ(gb,mass);
  CALL_PUSH_OBJ(gb,gmass);
  CALL_PUSH_OBJ(gb,raceID);
  CALL_PUSH_GAME_OBJ(gb, g,bitmap);
  CALL_PUSH_GAME_OBJ(gb, g,lastBitmap);
  CALL_PUSH_OBJ(gb,originalBitmapIndex);
  CALL_PUSH_OBJ(gb,cycle);
  CALL_PUSH_OBJ(gb,vis);
  CALL_PUSH_OBJ(gb,numPings);
  CALL_PUSH_GAME_OBJ(gb, g,parent.generic);

  return gb.BufferPosition() - bufferPos;
}

slong GenericClass::PopFrom(GameStateBuffer& gb, GameStateClass* g) {

  WRITE_SF_LOG(logFile << "Popping generic section of " << TypeName() << " " <<
	       g->TypeListIndex(this) << "..." << endl);

  char* bufferPos = gb.BufferPosition();

  CALL_POP_OBJ(gb,now);
  CALL_POP_OBJ(gb,vel);
  CALL_POP_OBJ(gb,tvel);
  CALL_POP_OBJ(gb,gnow);
  CALL_POP_OBJ(gb,gvel);
  CALL_POP_OBJ(gb,active);
  CALL_POP_OBJ(gb,offset);
  CALL_POP_OBJ(gb,frame);
  CALL_POP_OBJ(gb,nextGravityFrame);
  CALL_POP_OBJ(gb,bitmapStartFrame);
  CALL_POP_OBJ(gb,mass);
  CALL_POP_OBJ(gb,gmass);
  CALL_POP_OBJ(gb,raceID);
  CALL_POP_GAME_OBJ(gb, g,bitmap);
  CALL_POP_GAME_OBJ(gb, g,lastBitmap);
  CALL_POP_OBJ(gb,originalBitmapIndex);
  CALL_POP_OBJ(gb,cycle);
  CALL_POP_OBJ(gb,vis);
  CALL_POP_OBJ(gb,numPings);
  CALL_POP_GAME_OBJ(gb, g,parent.generic);

  return gb.BufferPosition() - bufferPos;
}

void PlanetClass::ProfileCheckSum(GameStateClass* g) {

  GenericClass::ProfileCheckSum(g);

  logFile << "planet check sum: " << GenericClass::CheckSum(g) << " " <<
    g->TypeListIndex(colony) << " " <<
    orbitalRadius << " " <<
    short(minor) << " " <<
    short(hydpc) << " " <<
    temp << " " << 
    accBonus << " " <<
    PlanetClass::CheckSum(g) << endl;
}

slong PlanetClass::CheckSum(GameStateClass* g) {

  slong thisCheckSum = GenericClass::CheckSum(g) +
    g->TypeListIndex(colony) +
    orbitalRadius +
    minor +
    hydpc +
    accBonus +
    temp;

  return thisCheckSum;
}



void PlanetClass::profile(GameStateClass* g) {
  logFile << "name   -> " << name << endl;
  logFile << "minor  -> " << slong(minor) << endl;
  logFile << "hydpc  -> " << slong(hydpc) << endl;
  logFile << "accBon -> " << accBonus << endl;
  if (colony) {
    logFile << "colony -> " << colony->TypeName() <<
      " " << g->TypeListIndex(colony) <<
      "(" << g->ObjectListIndex(colony) << 
      ") ( " << colony->now.x << " , " 
	    << colony->now.y <<
      " ) ( " << colony->tvel.x << " , " 
	    << colony->tvel.y << " )" <<
      " ) ( " << colony->vel.x << " , " 
	    << colony->vel.y << " )" <<
      endl;
  }
  GenericClass::profile(g);
}  

slong PlanetClass::PushTo(class GameStateBuffer& gb, GameStateClass* g) {

  WRITE_SF_LOG(logFile << "Pushing planet section of " << TypeName() << " " <<
	       g->TypeListIndex(this) << "..." << endl);
  
  slong subClassSize = GenericClass::PushTo(gb, g);

  char* bufferPos = gb.BufferPosition();

  CALL_PUSH_OBJ(gb,orbitalRadius);
  CALL_PUSH_OBJ(gb,iPhase);
  CALL_PUSH_OBJ(gb,terraForm);
  CALL_PUSH_OBJ(gb,minor);
  CALL_PUSH_OBJ(gb,hydpc);
  CALL_PUSH_OBJ(gb,temp);
  CALL_PUSH_OBJ(gb,accBonus);
  CALL_PUSH_OBJ(gb,atmP);
  CALL_PUSH_OBJ(gb,grav);
  CALL_PUSH_OBJ(gb,revertRaceIndex);
  CALL_PUSH_OBJ(gb,revertColonyDesignIndex);
  CALL_PUSH_GAME_OBJ(gb, g, colony);
  CALL_PUSH_OBJ(gb,name);

  return subClassSize + (gb.BufferPosition() - bufferPos);

}

slong PlanetClass::PopFrom(class GameStateBuffer& gb, GameStateClass* g) {

  WRITE_SF_LOG(logFile << "Popping planet section of " << TypeName() << " " <<
	       g->TypeListIndex(this) << "..." << endl);

  slong subClassSize = GenericClass::PopFrom(gb, g);

  char* bufferPos = gb.BufferPosition();

  CALL_POP_OBJ(gb,orbitalRadius);
  CALL_POP_OBJ(gb,iPhase);
  CALL_POP_OBJ(gb,terraForm);
  CALL_POP_OBJ(gb,minor);
  CALL_POP_OBJ(gb,hydpc);
  CALL_POP_OBJ(gb,temp);
  CALL_POP_OBJ(gb,accBonus);
  CALL_POP_OBJ(gb,atmP);
  CALL_POP_OBJ(gb,grav);
  CALL_POP_OBJ(gb,revertRaceIndex);
  CALL_POP_OBJ(gb,revertColonyDesignIndex);
  CALL_POP_GAME_OBJ(gb, g, colony);
  CALL_POP_OBJ(gb,name);

  return subClassSize + (gb.BufferPosition() - bufferPos);
}

slong DesignedClass::PushTo(GameStateBuffer& gb, GameStateClass* g) {

  WRITE_SF_LOG(logFile << "Pushing designed section of " << TypeName() << " " <<
	       g->TypeListIndex(this) << "..." << endl);

  slong subClassSize = GenericClass::PushTo(gb, g);

  char* bufferPos = gb.BufferPosition();
  
  CALL_PUSH_OBJ(gb,repairRate);
  CALL_PUSH_OBJ(gb,rotRate);
  CALL_PUSH_OBJ(gb,heading);
  CALL_PUSH_OBJ(gb,seekHeading);
  CALL_PUSH_OBJ(gb,thrustOffset);
  CALL_PUSH_OBJ(gb,turning);
  CALL_PUSH_OBJ(gb,throttle);
  CALL_PUSH_OBJ(gb,seekTurning);
  CALL_PUSH_OBJ(gb,maxSpeed);
  CALL_PUSH_OBJ(gb,comVec);
  CALL_PUSH_OBJ(gb,comVel);
  CALL_PUSH_OBJ(gb,mode);

  if ( !IsValidObjectPointer(viewObj.generic,__FILE__,__LINE__) )
    {
      viewObj.generic = 0;
    }
  CALL_PUSH_GAME_OBJ(gb, g,viewObj.generic);

  if ( !IsValidObjectPointer(lockObj.generic,__FILE__,__LINE__) )
    {
      lockObj.generic = 0;
    }
  CALL_PUSH_GAME_OBJ(gb, g,lockObj.generic);

  if ( !IsValidObjectPointer(targetPP.obj.generic,__FILE__,__LINE__) )
    {
      targetPP.obj.generic = 0;
    }
  CALL_PUSH_GAME_OBJ(gb, g,targetPP);

  if ( !IsValidObjectPointer(lastAttacker.generic,__FILE__,__LINE__) )
    {
      lastAttacker.generic = 0;
    }
  CALL_PUSH_GAME_OBJ(gb, g,lastAttacker.generic);

  CALL_PUSH_OBJ(gb,assign);
  CALL_PUSH_OBJ(gb,assignOverRide);
  CALL_PUSH_OBJ(gb,stat);
  CALL_PUSH_OBJ(gb,dutyPar);
  CALL_PUSH_OBJ(gb,duty);
  CALL_PUSH_OBJ(gb,assignPar1);
  CALL_PUSH_OBJ(gb,assignPar2);
  CALL_PUSH_OBJ(gb,assignPriority);
  CALL_PUSH_OBJ(gb,sensorThreshold);
  CALL_PUSH_OBJ(gb,firingTubes);
  CALL_PUSH_OBJ(gb,stealth);
  CALL_PUSH_GAME_OBJ(gb, g,bus);

  return subClassSize + (gb.BufferPosition() - bufferPos);
}

slong DesignedClass::PopFrom(GameStateBuffer& gb, GameStateClass* g) {

  WRITE_SF_LOG(logFile << "Popping designed section of " << TypeName() << " " <<
	       g->TypeListIndex(this) << "..." << endl);

  slong subClassSize = GenericClass::PopFrom(gb, g);

  char* bufferPos = gb.BufferPosition();
  
  CALL_POP_OBJ(gb,repairRate);
  
  CALL_POP_OBJ(gb,rotRate);
  CALL_POP_OBJ(gb,heading);
  CALL_POP_OBJ(gb,seekHeading);
  CALL_POP_OBJ(gb,thrustOffset);
  CALL_POP_OBJ(gb,turning);
  CALL_POP_OBJ(gb,throttle);
  CALL_POP_OBJ(gb,seekTurning);
  CALL_POP_OBJ(gb,maxSpeed);
  CALL_POP_OBJ(gb,comVec);
  CALL_POP_OBJ(gb,comVel);
  CALL_POP_OBJ(gb,mode);
  CALL_POP_GAME_OBJ(gb, g,viewObj.generic);
  CALL_POP_GAME_OBJ(gb, g,lockObj.generic);
  CALL_POP_GAME_OBJ(gb, g,targetPP);
  CALL_POP_GAME_OBJ(gb, g,lastAttacker.generic);
  CALL_POP_OBJ(gb,assign);
  CALL_POP_OBJ(gb,assignOverRide);
  CALL_POP_OBJ(gb,stat);
  CALL_POP_OBJ(gb,dutyPar);
  CALL_POP_OBJ(gb,duty);
  CALL_POP_OBJ(gb,assignPar1);
  CALL_POP_OBJ(gb,assignPar2);
  CALL_POP_OBJ(gb,assignPriority);
  CALL_POP_OBJ(gb,sensorThreshold);
  CALL_POP_OBJ(gb,firingTubes);
  CALL_POP_OBJ(gb,stealth);
  CALL_POP_GAME_OBJ(gb, g, bus);
  bus.MapFunctions(this,this);

  return subClassSize + (gb.BufferPosition() - bufferPos);
}



void PlanetClass::Update() {
  /* update the generic object -- updates position, velocity, bitmap, frame,
     and checks collsions. */

  static PingPacket parentPP;

  numPings = 0;
  //enemyOrbiting = 0;
  lastBitmap = bitmap;
  
  /* check for dead objects and reset or remove them depending on type */
  if (!IsActive()) {
    /* reset generic object */
  } else {

    offset = RotationType((frame - bitmapStartFrame) % bitmap.N());

    frame++;
    
    // if frame counter reaches object lifetime and object is terminal
    // set bitmapIndex to -1.
    if (frame == bitmapStartFrame + bitmap.N() && bitmap.IsTerminal()) { 
      bitmap = NoObjectBitmap;
    }
    
    if (parent.planet) {
      parentPP = Ping(parent_target);
      if (parentPP.dist > 3 * orbitalRadius / 2)
	/* if the planet is too far out of orbit correct it */
	Explode("the planet lost its orbit");
      /*long mag = */
      Attract(parentPP).Mag(0);
      //if (frame % cycle == 0 && mag < 15)
	/* lower the time resolution of the attractive force so that */
	/* the differential velocity has time to accumulate a larger */
	/* magnitude                                                 */
      //cycle++;
      //Attract(parentPP);
    }
  }
  
  /* check collisions by object type */
  if (!IsExploding() && !IsDone()) {
  }
  /* set global velocity variable */
  if ( IsValidObjectPointer(parent.generic,__FILE__,__LINE__) ) {
    if ( IsValidObjectPointer(parent.generic->parent.generic,__FILE__,__LINE__) ) 
      tvel = vel + parent.generic->vel + parent.generic->parent.generic->vel;
    else tvel = vel + parent.generic->vel;
  } else
    tvel = vel;

  if (colony) {
    /* match colony position and velocity to planet */
    colony->now = now;
    colony->gnow = colony->now;
    colony->tvel = tvel;

    /* accumulate terraform points */
    if (frame % frameRate == 0 && colony->pop.InUnitsOf(1000) > 0 )
      {
        terraForm = min(terraForm +
	                nlong(sqrt((double)colony->pop.InUnitsOf(1000))),
		        terraform_scale);
      }

  }

}


void PlanetClass::Explode(char* reason, short spawn, short direct) {
  /* this should only happen as a result of a network error        */
  /* in the event of an error the planet should reset its position */

  static CoordinateClass cOffset;
  static PingPacket pp;
  
  if (parent.planet) {
    cOffset.initializeWith(0, orbitalRadius);
    now = parent.generic->now + cOffset;
    gnow = now;
    pp = Ping(parent_target);
    vel = VelKep(pp);
  } else {
    now = nullVector;
    vel = nullVector;
  }
}

slong PlanetClass::Hit(slong dam, DesignedClass* d, CoordinateClass& dir,
		      short push, short vortex, short flags) {
  if (terraForm && d && d->IsEnemyOf(this))
    terraForm = max(terraForm - 15*dam, 0);

  if (colony)
    colony->Hit(dam, d, dir, push, vortex, flags);
  return dam;
}


int IsHumanControlled(ShipClass *s)
{
  return (s - &game->ship[0]) < game->maxHumanShips;
}

PopStruct ShipClass::OptimalCrew() {
  int isAIStarbase = !IsHumanControlled(this) && IsStarbase();
  if ( isAIStarbase )
    {
      return PopStruct(0, Mass(0,0));
    }
  else
    {
      return PopStruct(0, Mass(0,0) / 17);
    }
}

slong PlanetClass::ExploitationPotential() {
  slong size = bitmap.size * bitmap.size * bitmap.size * (100-hydpc) / 100;
  return size * (1+10*minor) / 10000;
}

slong PlanetClass::ExploitationLevel() {
  if (colony)
    return ExploitationPotential() * (colony->pop.InUnitsOf(10) /
      (OptimalPopulation(game->race[colony->raceID]).InUnitsOf(1000)+1)) / 100;
  else
    return 0;
}

PopStruct PlanetClass::OptimalPopulation(RaceClass& r) {
  slong size = bitmap.size * bitmap.size / 350 * bitmap.size *
    (100 - hydpc) * (3*(hydpc>0) + 1);

  slong hab = Habitability(r);

  short b;

  if (size < 2000000)
    b = (short)(5 * hab * size / 100000000);
  else
    b = (short)(size / 10000 * 5 * hab / 10000);

  slong lb;
  if (b)
    lb = ((5 * hab * (size / 100)) % 1000000) * 1000;
  else
    lb = 5 * hab * size * 10;

  static PopStruct p;

  p.SetStat(p.billions,b);
  p.SetStat(p.lBillions,lb);
  //p.billions = b;
  //p.lBillions = lb;

  p.validate(p.billions, p.lBillions);

  return p;
}

short PlanetClass::Habitability(RaceClass& r, short includeTF) {
  // return the habitability factor of a planet based on the correlation of
  // planetary and racial attributes

  slong tempFactor = temp * 100 / max(1,r.prefTemp);
  tempFactor = tempFactor * tempFactor / 100 * tempFactor / 100;

  slong atmPFactor = atmP * 100 / max(1,r.prefAtmP);
  if (atmPFactor < 4000)
    atmPFactor = atmPFactor * atmPFactor / 100 * atmPFactor / 100;
  else
    atmPFactor = atmPFactor * atmPFactor / 10000 * atmPFactor;

  slong gravFactor = grav * 100 / r.prefGrav;
  if (gravFactor < 4000)
    gravFactor = gravFactor * gravFactor / 100 * gravFactor / 100;
  else
    gravFactor = gravFactor * gravFactor / 10000 * gravFactor;

  if (tempFactor > 100)
    tempFactor = 10000 / tempFactor;

  if (atmPFactor > 100)
    atmPFactor = 10000 / atmPFactor;

  if (gravFactor > 100)
    gravFactor = 10000 / gravFactor;

  return (short)min((tempFactor + atmPFactor + gravFactor) / 3 +
	     includeTF * terraForm / max(1,terraform_base), 100);
}

void GenericClass::ProfileCheckSum(GameStateClass* g) {

  logFile << "generic check sum: " <<
    frame << " " <<
    mass << " " <<
    gmass << " " <<
    g->ObjectListIndex(parent.generic) << " " <<
    raceID << " " <<
    now.x << " " << now.y << " " <<
    vel.x << " " << vel.y << " " <<
    tvel.x << " " << tvel.y << " " <<
    bitmap.index << " " <<
    lastBitmap.index << " " <<
    cycle << " " <<
    originalBitmapIndex << " " <<
    numPings << " " <<
    short(offset) << " " <<
    short(active) << " " <<
    short(vis) << " " <<
    GenericClass::CheckSum(g) << endl;
}

slong GenericClass::CheckSum(GameStateClass* g) {

  slong thisCheckSum =
    frame +
    mass +
    gmass +
    g->ObjectListIndex(parent.generic) +
    raceID +
    now.x + now.y +
    vel.x + vel.y +
    tvel.x + tvel.y +
    bitmap.index +
    lastBitmap.index +
    cycle +
    originalBitmapIndex +
    numPings +
    offset +
    active +
    vis;

  return thisCheckSum;
}

void GenericClass::profile(GameStateClass* g) {
  logFile << "Generic Profile for " << TypeName() << " " <<
    game->TypeListIndex(this)<< " (" << game->ObjectListIndex(this) << ")" <<
    endl;
  logFile << "position   -> ( " << now.x << " , " << now.y << " )" << endl;
  logFile << "vel        -> ( " << vel.x << " , " << vel.y << " )" << endl;
  logFile << "tvel       -> ( " << tvel.x << " , " << tvel.y << " )" << endl;
  logFile << "bitmap     -> index " <<  bitmap.index << endl;
  logFile << "frame      -> " << frame << endl;
  logFile << "vis        -> " << short(vis) << endl;
}

void DesignedClass::ProfileCheckSum(GameStateClass* g) {
  
  GenericClass::ProfileCheckSum(g);

  logFile << "designed check sum: " << GenericClass::CheckSum(g) << " " <<
    bus.numMod << " " << /**/
    comVec.x << " " <<
    comVec.y << " " <<
    comVel.x << " " <<
    comVel.y << " " <<
    assignPar1 << " " <<
    assignPar2 << " " <<
    dutyPar << " " <<
    maxSpeed << " " <<
    g->ObjectListIndex(viewObj.generic) << " " <<
    g->ObjectListIndex(lockObj.generic) << " " <<
    g->ObjectListIndex(lastAttacker.generic) << " " <<
    assign << " " <<
    assignPriority << " " <<
    assignOverRide << " " <<
    stat << " " <<
    duty << " " <<
    thrustOffset << " " <<
    short(heading) << " " <<
    short(seekHeading) << " " <<
    short(turning) << " " <<
    short(seekTurning) << " " <<
    short(throttle) << " " <<
    short(transitSlide) << " " <<
    short(mode) << " ";
  if (IsValidObjectPointer(targetPP.obj.generic, __FILE__, __LINE__)) {
    logFile <<
      g->ObjectListIndex(targetPP.obj.generic);
    if (!targetPP.obj.generic->active)
      logFile << "(" << targetPP.obj.generic->TypeName() <<
	g->TypeListIndex(targetPP.obj.generic) << ") ";
  } else
    logFile << -1;
  logFile <<
    sensorThreshold << " " <<
    stealth << " " <<
    repairRate << " " <<
    DesignedClass::CheckSum(g) << endl;

}

slong DesignedClass::CheckSum(GameStateClass* g) {
  
  slong thisCheckSum = GenericClass::CheckSum(g) +
    bus.numMod + /**/
    comVec.x + comVec.y +
    comVel.x + comVel.y +
    assignPar1 +
    assignPar2 +
    dutyPar +
    maxSpeed +
    g->ObjectListIndex(viewObj.generic) +
    g->ObjectListIndex(lockObj.generic) +
    g->ObjectListIndex(lastAttacker.generic) +
    assign +
    assignPriority +
    assignOverRide +
    stat +
    duty +
    thrustOffset +
    heading +
    seekHeading +
    turning +
    seekTurning +
    throttle +
    transitSlide +
    mode +
    g->ObjectListIndex(targetPP.obj.generic) +
    sensorThreshold +
    stealth +
    repairRate;

  return thisCheckSum;
}

void GenericClass::ShortProfile() {
  
  if (raceID != -1)
    logFile << game->race[raceID].name << " ";
  else
    logFile << "neutral ";

  logFile <<
      TypeName() << " (slot " << game->TypeListIndex(this) << "/" << 
      game->ObjectListIndex(this) << ")";

}

void MissileClass::ShortProfile() {
  
  logFile << game->race[raceID].name << " " <<
    TypeName() << " (missile " << game->TypeListIndex(this) << "/" << 
    game->ObjectListIndex(this) << ")";

  if (duty == plugin)
    logFile << ", which is a powerup containing a " << bus.moduleList[1].name << " and,";

  if ( IsValidObjectPointer(parent.generic,__FILE__,__LINE__) ) {
    logFile << " which was fired by a ";
    parent.generic->ShortProfile();
  } else
    logFile << " which is an orphan" << endl;

  logFile << ",";
    
}

void ShipClass::ShortProfile() {
  
  logFile << game->race[raceID].name << " " <<
    design->name << " " << game->player[playerIndex].shipName << " (" <<
    TypeName() << " " << game->TypeListIndex(this) << "/" << 
    game->ObjectListIndex(this) <<
    "), commanded by " << game->rank[game->player[playerIndex].rank].name << 
    " " << game->player[playerIndex].name << ",";

}

void ColonyClass::ShortProfile()
{
  if ( raceID >= 0 && raceID < game->numRaces )
    {  
      logFile << game->race[raceID].name << " " <<
	design->name << " (" <<
	TypeName() << " " << game->TypeListIndex(this) << "/" << 
	game->ObjectListIndex(this) << ") with " << pop.print() << " " <<
	game->race[raceID].name << "s";
      if ( IsValidObjectPointer(lockObj.generic,__FILE__,__LINE__) )
	{
	  logFile << ", located on " << lockObj.planet->name << ",";
	}
    }
  else
    {
      logFile << "ERROR> Short profile of a colony with raceID = " << raceID;
    }
}

void DesignedClass::profile(GameStateClass* g) {
  logFile << "Designed Profile:" << endl;
  logFile << "assignment    -> " << assign << endl;
  logFile << "assignPar1    -> " << assignPar1 << endl;
  logFile << "assignPar2    -> " << assignPar2 << endl;
  logFile << "assignPriority-> " << assignPriority << endl;
  logFile << "stat          -> " << stat << endl;
  logFile << "duty          -> " << duty << endl;
  logFile << "dutyPar       -> " << dutyPar << endl;
  logFile << "stealth       -> " << stealth << endl;
  if ( IsValidObjectPointer(lockObj.generic,__FILE__,__LINE__) ) {
    logFile << "lock object    -> " << lockObj.generic->TypeName() <<
      " " << g->TypeListIndex(lockObj.generic) << endl;
  }
  if ( IsValidObjectPointer(lastAttacker.generic,__FILE__,__LINE__) ) {
    logFile << "last attacker -> " << lastAttacker.generic->TypeName() <<
      " " << g->TypeListIndex(lastAttacker.generic) <<
      "(" << g->ObjectListIndex(lastAttacker.generic) << 
      ") ( " << lastAttacker.generic->now.x << " , " 
	    << lastAttacker.generic->now.y <<
      " ) ( " << lastAttacker.generic->tvel.x << " , " 
	    << lastAttacker.generic->tvel.y << " )" <<
      " ) ( " << lastAttacker.generic->vel.x << " , " 
	    << lastAttacker.generic->vel.y << " )" <<
      endl;
  }
  if (IsValidObjectPointer(targetPP.obj.generic, __FILE__, __LINE__)) {
    logFile << "target        -> " << targetPP.obj.generic->TypeName() <<
      " " <<
      g->TypeListIndex(targetPP.obj.generic) <<
      "(" << g->ObjectListIndex(targetPP.obj.generic) << 
      ") ( " << targetPP.obj.generic->now.x << " , " 
	    << targetPP.obj.generic->now.y <<
      " ) ( " << targetPP.obj.generic->tvel.x << " , " 
	    << targetPP.obj.generic->tvel.y << " )" <<
      " ) ( " << targetPP.obj.generic->vel.x << " , " 
	    << targetPP.obj.generic->vel.y << " )" <<
      endl;
  }
  short i;
  for (i = 0; i < bus.numMod; i++)
    bus.moduleList[i].profile(g);
  GenericClass::profile(g);
}

void ShipClass::ProfileCheckSum(GameStateClass* g) {

  DesignedClass::ProfileCheckSum(g);

  logFile << "ship check sum: " << DesignedClass::CheckSum(g) << " " <<
    g->ObjectListIndex(dockObj) << " " <<
    distressFrame << " " <<
    short(playerIndex) << " " <<
    short(videoMode) << " " <<
    short(lastVideoMode) << " " <<
    portNum << " " <<
    accuracy << " " <<
    stealthLevel;

  if (Type() == ship)
    logFile << g->shipDesign.indexOf(design);
  else
    logFile << (ColonyDesignStruct *)design - g->colonyDesign;

  logFile << " " << pop.billions << " " << pop.lBillions
	  << " " << ShipClass::CheckSum(g) << endl;
}

slong ShipClass::CheckSum(GameStateClass* g) {
  slong thisCheckSum = DesignedClass::CheckSum(g) +
    g->ObjectListIndex(dockObj) +
    distressFrame +
    playerIndex +
    videoMode +
    lastVideoMode +
    portNum +
    accuracy +
    stealthLevel;

  if (Type() == ship)
    thisCheckSum += g->shipDesign.indexOf(design);
  else
    thisCheckSum += (ColonyDesignStruct *)design - g->colonyDesign;

  thisCheckSum += pop.billions + pop.lBillions;

  return thisCheckSum;
}

void ShipClass::profile(GameStateClass* g) {
  logFile << "Ship Profile:" << endl;
  logFile << "   design name: " << design->name << endl;
  if (Type() == ship) {
    logFile << "   playerIndex: " << playerIndex << endl;
    logFile << "   crew:        " << pop.print() << endl;
  }
  logFile << "   stealthLevel:" << stealthLevel << endl;
  logFile << "       accuracy:" << accuracy << "%" << endl;
  if (dockObj) {
    logFile << "   dock object: " << dockObj->TypeName() <<
      g->TypeListIndex(dockObj) << endl;
    logFile << "   port number: " << portNum << endl;
  }

  if (Type() == ship)
    g->player[playerIndex].profile(g);

  DesignedClass::profile(g);
}

void ColonyClass::ProfileCheckSum(GameStateClass* g) {

  ShipClass::ProfileCheckSum(g);

  logFile << "colony check sum: " <<
    ShipClass::CheckSum(g) << " " <<
    planetIndex << " " <<
    kills << " " <<
    pop.billions << " " <<
    pop.lBillions << " " <<
    ColonyClass::CheckSum(g) << endl;
}

slong ColonyClass::CheckSum(GameStateClass* g) {

  slong thisCheckSum = ShipClass::CheckSum(g) + 
    planetIndex +
    kills +
    pop.billions +
    pop.lBillions;

  return thisCheckSum;
}

void ColonyClass::profile(GameStateClass* g) {
  logFile << "Colony Profile:" << endl;

  logFile << "Planet Index: " << planetIndex << endl;
  if ( IsValidObjectPointer(lockObj.generic,__FILE__,__LINE__) )
    logFile << "home planet: " << lockObj.generic->TypeName() << " " <<
      g->TypeListIndex(lockObj.generic) << endl;
  else
    logFile << "bound for:   " << g->planet[PlanetAdvisor()].name << endl;

  logFile << "     kills:  " << kills << endl;

  logFile << "population:  " << pop.print() << endl;

  ShipClass::profile(g);
}

void MissileClass::ProfileCheckSum(GameStateClass * g) {
  
  DesignedClass::ProfileCheckSum(g);
  
  logFile << "missile check sum: " << DesignedClass::CheckSum(g) << " " <<
    short(targetLock) << " " <<
    triggerObjQualifier << " " <<
    triggerObjType << " " <<
    onImpact << " " <<
    g->missileDesign.indexOf(design) << " " << MissileClass::CheckSum(g) << endl;

}

slong MissileClass::CheckSum(GameStateClass * g) {
  
  slong thisCheckSum = DesignedClass::CheckSum(g) +
    targetLock +
    triggerObjQualifier +
    triggerObjType +
    onImpact +
    g->missileDesign.indexOf(design);

  return thisCheckSum;
}


void MissileClass::profile(GameStateClass* g) {
  logFile << "Missile Profile:" << endl;

  DesignedClass::profile(g);
}

slong DesignedClass::Hit(slong dam, DesignedClass* from, CoordinateClass& dir,
			short push, short vortex, short flags) {

  static CoordinateClass dPush;
  static CoordinateClass dVortex;

  if (dam)
    lastAttacker.designed = from;

  /* nudge ship */
  if ( mass != 0 && !(flags & bypass_push_hit) ) {
    dPush = dir * dam / mass * 30 * push / 100;
    dVortex = dir.crossZ() * dam / mass * 30 * vortex / 100;
    vel.AddSubjectToLimit(dPush + dVortex, maxSpeed);
  }

  //
  // If something is attached to dock for a human controlled ship that has only
  // one dock, then let damage go through shields and armor.
  //
  int isHumanControlledShip = 
    ((ShipClass *)this - &game->ship[0]) < game->maxHumanShips;
  int hasObjectDocked = 
    bus.numPorts == 1 &&
    bus.dockMod[0] && 
    bus.numPorts > 0 &&
    bus.dockMod[0]->workPar[0] &&
    bus.dockMod[0]->workPar[1];
  int letDamageHit = !(isHumanControlledShip && hasObjectDocked);
  int isStarbase = bus.numPorts > 1;
  char invulnerableSB = 1;

  //
  // Reduce damage to starbases.
  //
  if ( isStarbase && !isHumanControlledShip && invulnerableSB )
    {
      dam = 1;
    }

  //
  // Don't allow damage to *human* ships that are docked to a SB
  //
  if ( isHumanControlledShip && ((ShipClass *)this)->IsAttachedToSB() )
    {
      dam = 0;
    }

  //
  // Don't allow damage to *human* ships in internet cafe
  //
  if ( isHumanControlledShip && IsInInternetCafe(this) )
    {
      dam = 0;
    }

  //
  // Set shields to shield 3 level when carrying colony
  //
  if ( !letDamageHit && bus.shieldMod && bus.shieldMod->store > 400 )
    {
      bus.shieldMod->SetStat(bus.shieldMod->store,400);
      //bus.shieldMod->store = 400;
    }
  
  /* first shields */
  short mn = 0;
  if ( bus.shieldMod && stealth == 1) 
    {
      bus.shieldMod->Deplete(dam);
    }

  /* second armor */
  if ( letDamageHit && bus.armorMod && bus.armorMod->func && 
       !(flags & bypass_armor_hit) ) {

    /* apply ~1% damage to armor */
    slong armorDam = dam/100;
    bus.armorMod->Hit(armorDam);
    
    /* adjust remainder */
    dam += (armorDam - dam/100);
    
    if (dam >= bus.armorMod->par[0]) {

      /* reduce the amount of damage done */
      dam -= bus.armorMod->fPar(0);
      
    } else /* the damage does not penetrate the armor */
      return 0;
  }

  slong popDam = dam;

  //
  // Don't allow AI SBs to lose crew
  //
  if ( isStarbase && !isHumanControlledShip )
    {
      popDam = 0;
    }

  /* apply damage to engines from heat seeker */
  if (dam && (flags & heat_hit) && bus.impulseMod)
    bus.impulseMod->Hit(dam);

  short modNum = 0;
  short numMod;

  if (bus.armorMod)
    numMod = bus.numMod - (bus.armorMod != 0);   /* port: was "> 0" (pointer vs 0) */
  else
    numMod = bus.numMod;

  while (dam && mn < numMod) {

    if (bus.armorMod && bus.armorMod <= bus.moduleList.rootOf() + modNum)
      modNum = (short)((max(frameNumber + dam, frameNumber) + modNum + 2) % bus.numMod);
    else
      modNum = (short)((max(frameNumber + dam, frameNumber) + modNum + 1) % bus.numMod);

    bus.moduleList[modNum].Hit(dam);
    mn++;
  }
  
  return popDam;
}


slong ShipClass::Hit(slong dam, DesignedClass* d, CoordinateClass& dir,
		    short push, short vortex, short flags) {

  static char msg[1024];

  /* ships under construction should not sustain any damage */
  if (/*IsTransitioning() || */
      stat == under_construction ||
      frame < 3 * frameRate /* three seconds of invulnerability */)
    return 0;

  if (bus.armorMod && Type() == ship &&
      game->player[playerIndex].IsRestrictedFromUsing(bus.armorMod))
    flags |= bypass_armor_hit;
  
  slong popDam = DesignedClass::Hit(dam, d, dir, push, vortex, flags);

  /* beam weapons are ineffective at reducing population */
  if (flags & beam_hit)
    popDam = popDam / 10;

  /* counterattack */
  
  if (d && d->Type() != planet) {
    if (AutoResponse()) {
      if (d->IsFriendlyTo(this) && bus.impulseMod)  {
	/* friendly fire -- check me */
	//from->GoHome(attack);
	targetPP.obj.generic = 0;
      } else {
	assignPriority = 0; /* always counterattack */
	AttackObj(d);
      }
    }
  } else
    lastAttacker.generic = 0;
  
  if (popDam) {
    /* for ships and colonies deduct crew losses */

    switch (Type())
      {
      case ship:
	pop.GrowBy(max(100000-10*popDam,0));
	
	if (pop.InUnitsOf(1) < OptimalCrew().InUnitsOf(2) &&
	    AutoResponse())
	  {
	    GoHome(attack);
	    strcpy(msg, "1st Officer> We are returning to base to take on more crew.");
	    game->PostGameMessage(msg, game->TypeListIndex(this),
				  (short)raceID, personal);
	  }
	
	/* send a low priority distress call */
	if (game->frame - distressFrame > 100 && AutoResponse()) {
	  SendDistressSignal(/*low priority */);
	  //distressFrame = game->frame;
	}

	break;

      case colony:
	
	if ( IsValidObjectPointer(lockObj.generic,__FILE__,__LINE__) )
	  pop.GrowBy(100000-min(popDam,100000), -1);
	else
	  pop.GrowBy(max(100000-10*popDam,0));
	
	/* make sure the population never becomes negative */
	if (pop < PopStruct() || !pop.InUnitsOf(1))
	  {
	    pop = PopStruct(0,1);
	    Explode();
	  }
	break;

      default :
	break;
      }
  }

  if (bus.factoryMod)
    /* lower production as a result of attack */
    bus.factoryMod->specPar[0] = 
      bus.factoryMod->specPar[0] / 100 * (100 + dam / 100);

  return popDam;
}

slong ColonyClass::Hit(slong dam, DesignedClass* d, CoordinateClass& dir,
		      short push, short vortex, short flags) {

  static char msg[128];

  if (d && d->IsEnemyOf(this) || flags & bypass_push_hit) {
    ShipClass::Hit(dam, d, dir, push, vortex, flags);

    /* if the planet is hit by a bomb, it loses 3 supply points */
    if (flags & bomb_hit)
      supply = max(supply - 3, 0);

    /* target the attacking ship */
    targetPP.obj.designed = d;

    /* send a high-priority distress signal */
    if (game->frame - distressFrame > 2 * game->loyaltyCycle && d) {

      /* send the signal and check for a response */
      if (!SendDistressSignal((short)pop.InUnitsOf(1000000), ship | colony)) {
	/* update the distress frame */
	//distressFrame = game->frame;
	/* else { */
	
	if ( IsValidObjectPointer(lockObj.generic,__FILE__,__LINE__) )
	  sprintf(msg,"A %s colony at %s is under attack by the %ss.  Press 'L' to respond.", 
		  game->race[raceID].name, 
		  lockObj.planet->name,
		  game->race[d->raceID].name);
	else
	  sprintf(msg, "A %s colony is under attack by the %ss.  Press 'L' to respond.", 
		  game->race[raceID].name, 
		  game->race[d->raceID].name);
	game->PostGameMessage(msg, (short)raceID, (short)raceID, broadcast);
	//distressFrame = game->frame;
	game->race[raceID].distressObj.generic = this;
	
      }
    } else if (frame % low_priority_distress_rate == 0)
      /* send a low priority distress call every 2-3 seconds */
      SendDistressSignal(/*low priority */);

    /******************************************************/
    /* the time is takes to force a surrender is given by */
    /* T = loyaltyCycle * (200 + loyalty) / 300,          */
    /* where loyalty ranges from -100 to +100             */
    /******************************************************/

    if (game->surrenderAllowed && bus.aveFunc < ship_distress_func_level &&
	game->frame - distressFrame > game->loyaltyCycle * (200 + Loyalty()) / 300 * frameRate) {

      /* colony surrenders */

      /* send a colony-changes-race message */
      if (!game->colonyPending && !IsPending(captured) &&
	  (IsDisconnectedClient() || IsServer()) && d)
	((ShipClass*)d)->SendCaptureMsg(this);
      //SendSpawnMsg(PopStruct(), (short)d->raceID, game->ObjectListIndex(d));
    }
  }
  
  return dam;
}

void DesignedClass::Update() {
  /* update the designed class object -- fire all active modules */

  static PingPacket nearPP;

  /* factories (if present) should accumulate production */
  if (bus.factoryMod && Type() != missile) {

    /* check for detached build object */
    if (bus.factoryMod->workPar[0] && !bus.factoryMod->IsActive()) {
	/* the build process was interrupted -- reconnect */
      bus.factoryMod->activate();
    }

    /* raw materials resources */
    slong rawProd = bus.factoryMod->par[0];
    /* population-based resources */
    slong popProd = 0;

    /* give credit for ship's crew and colony population */
    switch (Type()) {
    case ship:
      popProd += ((ShipClass*)this)->pop.InUnitsOf(1) * 10;
      break;
    case colony:
      popProd += ((ShipClass*)this)->pop.Sqrt();
      break;
    default :
      break;
    }

    /* give raw material credit for planet attributes */
    /* this resource should only accumulate if the colony is ship-producing */
    if (Type() == colony && IsValidObjectPointer(lockObj.generic,__FILE__,__LINE__) ) {
      rawProd += lockObj.planet->ExploitationPotential();
      if (rawProd < 0)
	rawProd = 0;
    }

    /* store population resources */
    bus.factoryMod->specPar[0] += popProd;
    if (bus.factoryMod->specPar[0] < 0)
      bus.factoryMod->specPar[0] = 0;

    /* store raw material resources */
    bus.factoryMod->specPar[1] += rawProd;
    if (bus.factoryMod->specPar[1] < 0)
      bus.factoryMod->specPar[1] = 0;

    /* store the current population-based production rate */
    bus.factoryMod->specPar[2] = popProd;

    /* if prod is below a certain level we should consider the colony too */
    /* small to produce anything more than a plugin                       */
    /* large ship-building planets will accumulate resources until they   */
    /* hit the forced_plugin_production_req limit and then they will      */
    /* produce a powerup                                                  */
    if (Type() == colony && 
	!bus.factoryMod->IsActive() &&
	(popProd < lower_production_limit &&
	 bus.factoryMod->specPar[0] >= plugin_production_req ||
	 bus.factoryMod->specPar[0] >= forced_plugin_production_limit)) {
      
      nearPP = Ping(ship_target, nearest_friendly);
      
      if ( IsValidObjectPointer(nearPP.obj.generic,__FILE__,__LINE__) && 
           nearPP.dist < (int)plugin_spawn_dist)
	SpawnRandomModules(1, &nearPP.disp);
      
    }
  }
  
  if (!IsExploding()) {

    //bus.ProcessMessage(this);

    bus.BudgetPower();

    bus.FireModules(this);

    //Type();
  }
}

void ShipClass::AssignDuty(short force) {

  static char msg[1024];

  static PingPacket pp;

  /* in each case check the conditions under which the duty */
  /* may be reassigned.  In some cases the ship should not  */
  /* return to its assigned duty unless it is under manual  */
  /* control and idle                                       */

  switch (game->rank[game->player[playerIndex].rank].duty) {
  case none:
    break;
  case escort_detail:
    if (assign == manual || force) {
      duty = escort_detail;
    }
    break;
  case patrol_detail:
    if (assign == manual || force) {
      duty = patrol_detail;
    }
    break;
  case defend_detail:
    if (assign == manual && duty != defend_detail || force) {
      pp = Ping(planet_target, nearest_friendly);
      if ( IsValidObjectPointer(pp.obj.generic,__FILE__,__LINE__) ) {
	game->BindShipTo(this, pp.obj.generic);
	sprintf(msg, "1st Officer> We have been assigned defense duty.");
	game->PostGameMessage(msg, game->ship.indexOf(this), (short)raceID, personal);
      } 
    }
    break;
  case protect_detail:
    if ((assign == manual || force) && duty != protect_detail) {
      pp = Ping(planet_target, nearest_friendly);
      if ( IsValidObjectPointer(pp.obj.generic,__FILE__,__LINE__) ) {
	dutyPar = game->ObjectListIndex(pp.obj.generic);
	game->BindShipTo(this, pp.obj.generic);
	duty = protect_detail;
	sprintf(msg, "1st Officer> We have been assigned protection duty.");
	game->PostGameMessage(msg, game->ship.indexOf(this), (short)raceID, personal);
      }
    }
    break;
  case attack_detail:
    /* set the duty to attack */
    if (assign == manual || force) {
      duty = attack_detail;
    }
    break;
  case harass_detail:
    /* set the duty to harass small ships */
    if (assign == manual || force) {
      duty = harass_detail;
    }
    break;
  case conquer_detail:
    /* set the duty to conquer enemy colonies */
    if (assign == manual || force) {
      duty = conquer_detail;
    }
    break;
  case colonize_detail: {
    /* the player should pick up colonies from planets that are ready */
    /* to spawn and transport them to another planet                  */
    if (assign == manual || force) {
      if (!bus.numPorts) {
	duty = defend_detail;
	pp = Ping(planet_target, nearest_friendly);
	if ( IsValidObjectPointer(pp.obj.generic,__FILE__,__LINE__) ) {
	  game->BindShipTo(this, pp.obj.generic);
	  sprintf(msg, "1st Officer> We have been assigned defense duty.");
	  game->PostGameMessage(msg, game->ship.indexOf(this), (short)raceID, personal);
	}
      } else
	duty = colonize_detail;
    }
  }
  break;
  }
}

void ShipClass::ProcessAssignment(short& lockedTubes) {
  short attackOverride = 0;
  static char msg[256];
  static CoordinateClass velKep;
  static CoordinateClass velDiff;
  static CoordinateClass dv;
  static PingPacket nearPP;

  if (assign == patrol) {
    assignPar1 = (assignPar1 + 1) % 1000;
    if (!assignPar1) {
      stat = idle;
      parent.generic = 0;
      AssignNextPlanet(1, sequential_friendly);
    }
    nearPP = Ping(planet_target, specific, (short)assignPar2);
    if ( IsValidObjectPointer(nearPP.obj.generic,__FILE__,__LINE__) && 
	 stat == idle) {
      if (nearPP.dist < 
	  usub_scale * (ulong) nearPP.obj.generic->bitmap.size) {
	EstablishOrbit(patrol);
      } else {
	SeekObj(nearPP, 10, 0, 0);
      }
    } else {
      if (stat == idle) ClearAssign();
    }
  } else if (assign == seek_heading) {
    SeekHeading(seekHeading);
  } else if (assign == dry_dock) {
    /* do nothing */
  } else if (assign == go_home) {
    nearPP = Ping(colony_target, nearest_friendly);
    if ( IsValidObjectPointer(nearPP.obj.generic,__FILE__,__LINE__) ) {
      assignPar2 = nearPP.obj.colony->planetIndex;
      GoPlanet();
    } else
      ClearAssign();
  } else if (assign == go_planet) {

    /* is the intended planet valid? */
    if (assignPar2 < 1 || assignPar2 > game->numPlanets) {
      /* check if duty sent them here */
      if ((duty == defend_detail || duty == protect_detail) &&
	  dutyPar == assignPar2) {
	duty = none;
	dutyPar = 0;
      }
      GoHome();
    }
    nearPP = Ping(planet_target, specific, (short)assignPar2);
    if ( IsValidObjectPointer(nearPP.obj.generic,__FILE__,__LINE__) ) {
      SeekHeading(nearPP.disp);

      ulong outerDist, innerDist;

      if (nearPP.obj.planet->IsFriendlyTo(this)) {
	innerDist = 
	  usub_scale * (ulong) nearPP.obj.generic->bitmap.size;
	outerDist =  2 * innerDist;
      } else {
	innerDist = 
	  min(usub_scale * (ulong) nearPP.obj.generic->bitmap.size,
	      (max_blockade_dist + usub_scale * 
	       (ulong) nearPP.obj.generic->bitmap.size) / 2);
	outerDist = min(2 * innerDist, max_blockade_dist);
      }

      if (nearPP.dist < outerDist &&
	  nearPP.dist > innerDist) {
	CheckTransition(nearPP.obj.generic->tvel);
	EstablishOrbit(assignOverRide);
      } else if (nearPP.dist <= innerDist) {
	SeekObj(nearPP, -500, 5, 0);
      } else if (!HasToAvoidStar()) {
	SeekObj(nearPP,  40, 5);
      }
    } else {
      ClearAssign();
    }
  } else if (assign == establish_orbit) {
    if (assignPar2)
      nearPP = Ping(planet_target, specific, (short)assignPar2, 0);
    else {
      nearPP = Ping(planet_target, nearest, 0);
      assignPar2 = game->TypeListIndex(nearPP.obj.planet);
    }

    ulong dist = max_orbit_dist;

    if ( IsValidObjectPointer(nearPP.obj.generic,__FILE__,__LINE__) &&
         !nearPP.obj.generic->IsFriendlyTo(this))
      dist = max_blockade_dist + usub_scale * (ulong) nearPP.obj.generic->bitmap.size / 2;

    if ( IsValidObjectPointer(nearPP.obj.generic,__FILE__,__LINE__) && nearPP.dist < dist
	/*min(max_orbit_dist,
	  2*sub_scale*nearPP.obj.generic->bitmap.size)*/) {
      if (nearPP.dist < usub_scale * (ulong) nearPP.obj.generic->bitmap.size) 
	GoPlanet(assignPar2, assignOverRide);
      else {
	CheckTransition(nearPP.obj.generic->tvel);
	velKep = VelKep(nearPP);
	velDiff = nearPP.obj.generic->tvel + velKep - tvel;
	dv.initialize();
	if (bus.impulseMod && 
	    dv.AddSubjectToLimit(velDiff,
				 min(bus.impulseMod->aPar(0), game->maxAcc *
				     mass*100/frameRate/frameRate/100) * 
				 sub_scale * dt/mass)) {

	  PlaySoundOnScreen(game->soundEvent[orbit_event], 1);

	  vel = velKep;
	  parent.generic = nearPP.obj.generic;
	  assignPar1 = 1;
	  SwapAssign();
	  stat = orbiting;
	  turning = 0;
	  SeekHeading(vel);
	  SetMode(mode_orbit, op_on);
	  if ( IsValidObjectPointer(parent.planet->colony) ) {

	    if (parent.planet->colony->IsFriendlyTo(this)) {

	      if (game->PlanetIsSpawnable(parent.planet, game) &&
		  bus.numPorts) {

		if (game->ship.indexOf(this) == localSlotNumber)
		  strcpy(msg,
			 "1st Officer> Orbit established -- replenishing tubes and crew.  A colony is available; press \"J\" to pick up.");

		targetPP.obj.colony = parent.planet->colony;
	      } else if (game->ship.indexOf(this) == localSlotNumber)
		strcpy(msg,
		       "1st Officer> Orbit established -- replenishing tubes and crew.");

	      game->PostGameMessage(msg, game->ship.indexOf(this), (short)raceID, personal);

	      bus.ReplenishSystems();
	      PopStruct nCrew = OptimalCrew();
	      if (parent.planet->colony->pop < nCrew - pop) {
		pop += parent.planet->colony->pop / 2;
		parent.planet->colony->pop -= parent.planet->colony->pop / 2;
	      
		/* make sure that the population is not negative */
		if (parent.planet->colony->pop < PopStruct())
		  parent.planet->colony->pop = PopStruct(0,1);

	      } else {
		parent.planet->colony->pop -= nCrew - pop;
		pop = nCrew;

		/* make sure that the population is not negative */
		if (parent.planet->colony->pop < PopStruct())
		  parent.planet->colony->pop = PopStruct(0,1);

	      }
	    } else if (game->ship.indexOf(this) == localSlotNumber) {
	      strcpy(msg, "1st Officer> Blockade established.");
	      game->PostGameMessage(msg, game->ship.indexOf(this), (short)raceID, personal);
	    }
	  } else if (game->ship.indexOf(this) == localSlotNumber) {
	    strcpy(msg, "1st Officer> Orbit established.");
	    game->PostGameMessage(msg, game->ship.indexOf(this), (short)raceID, personal);
	  }
	} else {
	  stat = idle;
	  vel.AddSubjectToLimit(dv, maxSpeed);
	  SeekHeading(vel);
	}
      }
    } else {
      GoPlanet(assignPar2, assignOverRide);
    }
  } else if (assign == follow_object) {
    if (IsValidObjectPointer(targetPP.obj.generic, __FILE__, __LINE__) &&
        targetPP.obj.generic->IsActive()) {

      /* do some checks on the assignment of the object being followed */
      if (frame % frameRate == 0 && AutoResponse() &&
	  (targetPP.obj.generic->Type() == ship ||
	   targetPP.obj.generic->Type() == colony) &&
	  IsValidObjectPointer(targetPP.obj.ship->targetPP.obj.generic) &&
	  targetPP.obj.ship->targetPP.obj.generic->IsActive() &&
	  targetPP.obj.ship->targetPP.obj.generic->CanBeSeenBy(this) &&
	  targetPP.obj.designed->IsFriendlyTo(this)) {
	
	/* if the ship is friendly and it is attacking another object */
	/* then we should attack it also                              */
	if (targetPP.obj.designed->assign == attack) {
	  targetPP = targetPP.obj.ship->targetPP;
	  AttackTarget();
	} else if (targetPP.obj.designed->assign == follow_object)
	  /* if the ship is following another object then follow it instead */
	  targetPP = targetPP.obj.ship->targetPP;
      }

      targetPP = Ping(target_target, all, 0, pc_long_max,
		      bitmap.dir[heading]*min_approach_dist / sub_scale);
      
      if (bus.impulseMod && IsValidObjectPointer(targetPP.obj.generic)) {

	if (duty == colonize_detail && targetPP.obj.generic->Type()==colony &&
	    targetPP.obj.generic->IsFriendlyTo(this)) {
	  SeekObj(targetPP, 40, 50, -3*targetPP.obj.generic->bitmap.size/2);
	} else {

	  if (!IsWarping()) {
	    if (!HasToAvoidStar()) {
	      
	      if (duty == colonize_detail && targetPP.obj.generic->Type() == planet) {
		SeekObj(targetPP, 100, 20, -3*targetPP.obj.generic->bitmap.size);
	      } else {
		SeekObj(targetPP, 40, 5, -targetPP.obj.generic->bitmap.size/2);
	      }

	    }
	  } else {
	    SeekObj(targetPP, 40, 5, -targetPP.obj.generic->bitmap.size/2);
	  }
	}

	SeekHeading(vel);

      } else
	/* return to manual control */
	assign = manual;
      
      //if (avoidPP.obj.generic)
      //Avoid(avoidPP);
    } else {
      EnforceDuty();
      return;
    }
  } else if (assign == go_dock || assign == go_multi_dock) {

    /* there is nothing to prevent a ship from trying to dock indefinitely.  */
    /* This ship should be checked to make sure that it does not try to dock */
    /* if it does not need repairs                                           */

    if (AutoResponse() && bus.aveFunc == module_function_scale) {
      Assign(manual);
      return;
    }

    if (assign == go_dock)
      nearPP = Ping(ship_target, nearest_port);
    else
      nearPP = Ping(ship_target, nearest_multi_port);

    if ( IsValidObjectPointer(nearPP.obj.generic) )
      {
	short freePort = nearPP.obj.ship->FindNextFreePort();
	if ( nearPP.dist < (int)min_approach_dist && 
	     freePort && 
	     !nearPP.obj.ship->IsWarping() &&
	     nearPP.obj.ship->bus.dockMod[freePort-1]->Storage() > 50 &&
	     !nearPP.obj.ship->bus.dockMod[freePort-1]->IsActive() )
	  {

	    /* signal dock object for docking */
	    nearPP.obj.ship->bus.dockMod[freePort-1]->workPar[0] = slong(this);
	    
	    /* dock activation */
	    nearPP.obj.ship->bus.dockMod[freePort-1]->SendDockActivationMsg();
	    
	    /* release docked ships */
	    ReleaseDocks();
	  }
	else if ( !HasToAvoidStar() )
	  {
	    SeekObj(nearPP, 50);
	  }
      } else
	{
	  /* can't find a dock target -- go home */
	  GoHome();
	  if (game->ship.indexOf(this) == localSlotNumber)
	    {
	      sprintf(msg,
		      "1st Officer> No docks available.  "
		      "Proceeding to nearest colony."); 
	      game->PostGameMessage(msg,
				    game->ship.indexOf(this),
				    (short)raceID, personal);
	    }      
	}
  } else if (assign == self_destruct) {

    /* if the cloak is active then deactivate it so that shields will be active */
    if (bus.stealthMod)
      bus.stealthMod->deactivate();
    stealth = 1;

    if (!dockObj && ( (bus.numPorts>0 && bus.dockMod[0] && !bus.dockMod[0]->workPar[1]) || !bus.dockMod))
       Hit(200, 0, nullVector);

  } else if (assign == attack) {

    /* acquire a new target under the following circumstances   */
    /* (1) no target                                            */
    /* (2) target cannot be seen, but is not cloaked or warping */
    /* (3) target is friendly                                   */

    if (IsValidObjectPointer(targetPP.obj.generic, __FILE__, __LINE__)) {
      
      /*short invalidTarget =
	targetPP.obj.generic->
	IsOutOfRangeOf(viewObj.designed, game->lightSpeed) ||
	targetPP.obj.generic->IsFriendlyTo(this);*/
      
      /* dlh */
      short invalidTarget,isOutOfRange,isFriendly;
      /* this line has problems when logging into server */
      isOutOfRange = targetPP.obj.generic->IsOutOfRangeOf(this,
                                                          game->lightSpeed);
      isFriendly = targetPP.obj.generic->IsFriendlyTo(this);
      invalidTarget = isOutOfRange || isFriendly;
      /* dlh */

      if (!invalidTarget) {
	
	short attackAdvice = AttackAdvisor();

	switch(attackAdvice) {
	case stand:
	  if (targetPP.obj.generic->IsExploding()) {
	    /* the target has exploded -- go back to assigned duty */
	    EnforceDuty();
	    return;
	  }
	  
	  nearPP = Ping(target_target);

	  GadgetAdvisor(nearPP, attackOverride, attackAdvice);
	  if (nearPP.obj.generic && viewObj.ship &&
	      nearPP.obj.designed->CanBeSeenBy(viewObj.ship)) {

	    targetPP = nearPP;

	    if (targetPP.dist < (int)min_attack_dist) {
	      CheckTransition(targetPP.obj.generic->tvel);
	      if (targetPP.dist < (int)min_approach_dist) {
		if (!attackOverride) {
		  slong h;
		  if (targetPP.obj.designed->IsLeftOf(this))
		    h = 100;
		  else
		    h = -100;
		  FleeObj(targetPP, 50, h);
		}
	      } else if (this->IsInFrontOf(targetPP.obj.designed)  &&
			 !targetPP.obj.designed->IsInFrontOf(this)) {
		/* the object is behind us */
		slong h;
		if (targetPP.obj.designed->IsLeftOf(this))
		  h = 100;
		else
		  h = -100;
		FleeObj(targetPP, 500, h);
		SeekHeading(targetPP.disp.crossZ()*h);
	      } else {
		/* the object is in front of us */
		SeekObj(targetPP, 20);
		SeekHeading(targetPP.disp);
	      }
	      if (!attackOverride && frame % fire_rate_AI == 0 &&
		  stealthLevel == 0) {

		Command(BusStruct::fire_tubes, targetPP.disp, targetPP.vel);

		firingTubes = 1;
		lockedTubes = 1;
		bus.ProcessMessage(this);
	      }
	    } else if (targetPP.dist < (int)attack_dist) {
	      SeekObj(targetPP, 20);
	      SeekHeading(targetPP.disp);
	      if (!IsWarping() && frame % fire_rate_AI == 0 &&
		  stealthLevel == 0) {

		Command(BusStruct::fire_tubes, targetPP.disp, targetPP.vel);

		firingTubes = 1;
		lockedTubes = 1;
		bus.ProcessMessage(this);
	      }
	    } else {
	      
	      if (!IsWarping()) {
		if (!HasToAvoidStar())
		  SeekObj(targetPP, 10);
	      } else {
		
		SeekObj(targetPP, 10);
		SeekHeading(targetPP.disp);

	      }
	    }
	  } else {
	    /* target is cloaked or warping -- be patient */

	    /* seek out last known position and fire at it */
	    /* targetPP is not updated as long as target   */
	    /* cannot be seen                              */
	    if (targetPP.dist < (int)attack_dist) {
	      SeekObj(targetPP, 20);
	      SeekHeading(targetPP.disp);
	      if (!IsWarping() && frame % fire_rate_AI == 0 &&
		  stealthLevel == 0) {

		Command(BusStruct::fire_tubes, targetPP.disp, targetPP.vel);
		
		firingTubes = 1;
		lockedTubes = 1;
		bus.ProcessMessage(this);
	      }
	    } else
	      /* target is not visible and last known position is */
	      /* out of range                                     */
	      seekTurning = 0;

	    /* if target does not reappear before time limit is up then */
	    /* go back to duty                                          */

	    if (frame % forget_interval == 0) {
	      EnforceDuty();
	      return;
	    }
	  }
	  break;
	case flee:
	  /* return to base for repairs */
	  GoDock(attack);
	  break;
	case standby:
	default:
	  EnforceDuty();
	  return;
	}
	
	/* avoid other ships while attacking */
	//if (bus.impulseMod && 
	//    bus.impulseMod->func > module_break_limit && 
	//    !attackOverride) {
	  //PingPacket nearPP = Ping(ship_target, nearest);
	  //if (nearPP.obj.generic)
	  //  Avoid(nearPP);
	//}
	
      } else {
	
	/* target is out of range                                */
	/* or                                                    */
	/* target is friendly                                    */

	seekTurning = 0;

	/* go back to duty */
	EnforceDuty();
	return;
      }
    } else {

      /* no target is assigned -- go back to duty */

      /* go back to duty */
      EnforceDuty();
      return;
    }
  }
}


void ShipClass::Update() {
  /* update the ship object -- updates position, velocity, bitmap, frame,
     and checks collsions. */
  short lockedTubes = 0;
  static char msg[1024];
  static CoordinateClass dv;
  static CoordinateClass velKep;
  static CoordinateClass velDiff;
  static PingPacket nearPP;
  static PingPacket parentPP;
  static PingPacket pp;

  lastBitmap = bitmap;

  //
  // Force a check for crew RAM Cheat
  //
  pop.AddToStat(pop.billions,0);

  /* check for dead objects and reset or remove them depending on type */
  if ( !IsActive() )
    {
      /* turn off the active flag -- object will be inactive until the slot */
      /* is filled again by ship construction                               */
      deactivate();
      
      /* record event in log */
      WRITE_SF_LOG(logFile << TypeName() << " " << game->TypeListIndex(this) <<
		   " (slot " << game->ObjectListIndex(this) << 
		   ") deactivated on frame " << game->frame << endl);
      
      /* put the ship in standby mode: ship construction will not occur until */
      /* a slot is earned by a race                                           */
      /* slot will be made available by GameStateClass::Update                */
      stat = standby;
      
      /* give half credit for previous upgrades */
      slong oldRefValue =
	max(min(game->player[playerIndex].refValue + (TotalValue() - design->TotalValue()) / 2,
		max_credit), 0);
      
      /* reset ship -- leave in place */
      bus.FreeModuleList();
      
      /* check dock status and free if necessary */
      if ( dockObj) {
	FreeFromDock();
      }
      
      /* if this is a robot player then deactivate it */
      if (playerIndex < game->numRobotPlayers)
	{
	  
	  game->player[playerIndex].deactivate();
	  return;
	  
	}
      else
	{ /* do not pick new player */
	  
	  initializeWith(game->ship.indexOf(this), playerIndex);
	  
	  game->player[playerIndex].SetStat(game->player[playerIndex].refValue,oldRefValue);
	  //game->player[playerIndex].refValue = oldRefValue;
	  
	  game->player[playerIndex].numMissions++;
	  
	  /* refit ship if player's ship does not match player's race or */
	  /* player's rank is greater than required for current ship     */
	  game->CheckRank(playerIndex, this);
	  
	  /* } */
	  game->playerList[game->ship.indexOf(this)] = game->player+playerIndex;
	  
	  activate();
	  
	}
      
      /*if ( this-game->ship == localSlotNumber )
	videoWindowNeedsPainting = 1;*/
    }
  else // Active()
    {
      
      //
      // Drop anything if ship is warping
      //
      if ( IsWarping() &&
	   bus.dockMod && 
	   bus.dockMod[0] && 
	   bus.numPorts > 0 &&
	   bus.dockMod[0]->workPar[0] &&
	   bus.dockMod[0]->workPar[1] )
	{
	  ObjectUnion s;
	  s.objLong = bus.dockMod[0]->workPar[0];
	  
	  if ( IsValidObjectPointer(s.generic) )
	    {
	      game->GenericRelease(this);
	      if ( s.generic->Type() == ::ship )
		{
		  s.ship->Explode();
		}
	      if ( s.generic->Type() == ::colony )
		{
		  s.colony->deactivate();
		}
	    }
	}

      /* prevent negative credit */
      if (game->player[playerIndex].refValue < 0)
	{
	  game->player[playerIndex].SetStat(game->player[playerIndex].refValue,0);
	  //game->player[playerIndex].refValue = 0;
	}
      
      CalculateMass();
      
      accuracy = min(game->rank[game->player[playerIndex].rank].accBonus + 
		     5 * game->player[playerIndex].CrewExperienceLevel(), 100);
      
      //if (mass == 0)
      //logFile << "ShipClass::Update: zero mass detected" << endl;
      
      if (stat == under_construction)
	return; /* test -- remove me */
      
      /*********************************************************************/
      /* if this ship is targeting an exploding or inactive missile then 	 */
      /* reset the target.						 */
      /*********************************************************************/
      if (IsValidObjectPointer(targetPP.obj.generic, __FILE__, __LINE__) &&
	  targetPP.obj.generic->Type() == missile &&
	  (!targetPP.obj.missile->IsActive() || targetPP.obj.missile->IsExploding()))
	targetPP.obj.generic = 0;
      
      /* check for boost in maxSpeed */
      if (maxSpeed > design->maxSpeed && bus.boostMod && 
	  !bus.boostMod->IsActive()) {
	/* lower limit gradually over 3 seconds */
	maxSpeed -= (maxSpeed-design->maxSpeed) / (3*frameRate) + 1;
	vel.AddSubjectToLimit(nullVector, maxSpeed);
      }
      
      /* check cloaking status */
      if (bus.stealthMod && !bus.stealthMod->IsActive() && stealthLevel)
	stealthLevel--;
      
      /* repair ship systems */
      /* ships with factories can repair their own armor */
      bus.RepairSystems(repairRate, short(bus.factoryMod != 0));
      if ( IsValidObjectPointer(dockObj)) {
	
	/* only ships docked with factories can repair armor */
	if (dockObj->bus.factoryMod == 0)
	  bus.RepairSystems(dockObj->repairRate);
	else
	  bus.RepairSystems(dockObj->repairRate, 1);
	
	viewObj.ship = dockObj;
      }
      
      /* update the DesignedClass section */
      DesignedClass::Update();
      
      if (stealthLevel == 0 && IsValidObjectPointer(avoidPP.obj.generic,__FILE__,__LINE__) )
	bus.FireDefensiveWeaponModules(this, avoidPP.disp, avoidPP.obj.generic->tvel, lockedTubes);
      
      /* avoid nearby objects */
      if (ShouldRefreshPing(avoidPP, frameRate)) {
	nearPP = Ping(ship_target, nearest);
	avoidPP.Sniff(nearPP, 0);
      }
      
      /* check ship status -- send distress signal */
      if (AutoResponse() && 
	  bus.aveFunc < ship_distress_func_level &&
	  IsValidObjectPointer(targetPP.obj.generic, __FILE__, __LINE__) &&
	  game->frame - distressFrame > 2 * game->loyaltyCycle) {
	/* move this to SendDistress */
	//distressFrame = game->frame;
	SendDistressSignal(game->player[playerIndex].rank);
      }
      
      /* check operational status of docked ships -- release them */
      short i;
      ShipClass* s;
      for (i = 0; i < bus.numPorts; i++)
	{
	  s = (ShipClass*) bus.dockMod[i]->workPar[0];
	  if ( bus.dockMod[i] && 
	       bus.dockMod[i]->workPar[1] )
	    {
	      if ( IsValidObjectPointer(s) &&
		   s->bus.aveFunc == module_function_scale )
		{
		  switch ( s->Type() )
		    {
		    case ship:
		      {
			if ( s->AutoResponse() && 
			     ((bus.factoryMod && 
			       bus.factoryMod->workPar[0] != bus.dockMod[i]->workPar[0]) ||
			      !bus.factoryMod))
			  {
			    if ( game->IsRobotPlayer(s) && 
				 bus.factoryMod &&
				 (game->PlayerIsUpgradeable(s->playerIndex) ||
				  s->design->raceID != game->player[s->playerIndex].raceId) )
			      {
				game->DecideToRefitShip(game->TypeListIndex(s));
			      }
			    if (s->dockObj == this)
			      {
				ReleaseDock(i); /*** fix me ***/
			      }
			    else
			      {
				logFile << "ERROR> illegal dock object found.  A ";
				s->ShortProfile();
				logFile << " is docked with a ";
				ShortProfile();
				if ( IsValidObjectPointer(s->dockObj) )
				  {
				    logFile << " but it thinks it is docked with a";
				    s->ShortProfile();
				    logFile << " on frame " << game->frame << endl;
				  }
				else
				  {
				    logFile << "but it does not think it is docked with anything on frame " <<
				      game->frame << endl;
				  }
				//s->dockObj = 0;
				s->SendUnDockMsg();
				ProcessFreeDockMsg(bus.moduleList.indexOf(bus.dockMod[portNum]));
			      }
			    
			    if (!s->bus.impulseMod && IsValidObjectPointer(parent.generic) )
			      {
				/* put into Keplerian orbit */
				s->parent = parent;
				parentPP = Ping(planet_target, specific,
						game->TypeListIndex(parent.planet));
				vel = VelKep(parentPP);
			      }
			  }
		      }
		      break;
		    case colony:
		      {
			if (AutoResponse() &&
			    duty != colonize_detail)
			  ReleaseDock(i);
		      }
		      break;
		    default :
		      break;
		    }
		}
	    }
	}
      
      /* check status of ship's power */
      /* if out of power -- explode */
      if (bus.powerOut == 0 && !IsExploding()) {
	/* credit a kill to the parent of the last missile */
	
	//game->player[playerIndex].numKilled++;
	
	Explode("it was damaged beyond repair");
      }
      if (bitmap.IsLoop()) {
	/* if bitmap has angles: */
	if (assign == manual && stat == idle && AutoResponse()) {
	  if (bus.impulseMod)
	    GoHome();
	  else {
	    ReceiveUnDockMsg();
	    /* direct client explosion */
	    Explode("it was lost in space", 0, 1);
	  }
	} else {
	  
	  /* decay away the assignment priority so that the object does */
	  /* not drift out to infinity.                                 */
	  if (frame % 1000 == 0)
	    assignPriority = max(assignPriority - 1, 0);
	  
	  if (AutoResponse()) 
	    AssignDuty();
	  
	  if (duty != none)
	    CheckDuty();
	  
	  if (assign != manual)
	    ProcessAssignment(lockedTubes);
	  
	}
	if (stat == orbiting) {
	  /* avoid other ships */
	  //if (bus.impulseMod && game->IsRobotPlayer(this)) {
	  //  PingPacket nearPP = Ping(ship_target, nearest);
	  //  if (nearPP.obj.generic)
	  //    Avoid(nearPP);
	  //}
	  
	  if (transitSlide < max_slide)
	    transitSlide++;
	  
	  parentPP = Ping(parent_target);
	  dv = Attract(parentPP);
	  
	  /* auto adjust orbit once per second */
	  if (frame % frameRate == 0) {
	    
	    velKep = VelKep(parentPP);
	    velDiff = parentPP.obj.generic->tvel + velKep - tvel;
	    dv.initialize();
	    if (bus.impulseMod && 
		dv.AddSubjectToLimit(velDiff,
				     min(bus.impulseMod->aPar(0), game->maxAcc *
					 mass*100/frameRate/frameRate/100) * 
				     sub_scale * dt/mass)) {
	      
	      vel = velKep;
	      
	    } else {
	      vel.AddSubjectToLimit(dv, maxSpeed);
	      SeekHeading(vel);
	    }
	  }
	  
	  if (parent.planet && parent.planet->colony &&
	      !parent.planet->colony->IsFriendlyTo(this)) {
	    /* this ship is orbiting an enemy colony */
	    /* it should accumulate blockadeFrames   */
	    if (parentPP.dist < max_blockade_dist + usub_scale * (ulong) parent.planet->bitmap.size / 2) {
	      blockadeFrames++;
	      /* set enemyOrbiting flags for those ships that are or-  */
	      /* biting an enemy planet                                */
	      parent.planet->enemyOrbiting = 1;
	    } else {
	      strcpy(msg, "1st Officer> Blockade lifted.  We are too far away.");
	      game->PostGameMessage(msg, game->ship.indexOf(this), (short)raceID, personal);
	      LeaveOrbit();
	    }
	  }
	  
	  if (assignPar1)
	    SeekHeading(vel);
	  
	  /* if too far out of orbit, auto-correct */
	  if (AutoOrbit() && bus.impulseMod) {
	    if (assign == manual &&
		(parentPP.dist >  10 * usub_scale *
		 (ulong) parentPP.obj.generic->bitmap.size ||
		 parentPP.dist < usub_scale *
		 (ulong)  parentPP.obj.generic->bitmap.size / 2)) 
	      EstablishOrbit();
	  } else if (dv == nullVector || 
		     X() - parent.generic->X() > 500 ||
		     Y() - parent.generic->Y() > 500) {
	    if (bus.impulseMod)
	      LeaveOrbit();
	    else
	      Explode("it was too far out of orbit");
	  }
	} else if (stat == leaving_orbit) {
	  cycle = 1;
	  blockadeFrames = 0;
	  if (transitSlide) transitSlide--;
	  else {
	    if ( IsValidObjectPointer(parent.generic,__FILE__,__LINE__) ) {
	      if ( IsValidObjectPointer(parent.generic->parent.generic,__FILE__,__LINE__) ) 
		vel += parent.generic->vel + parent.generic->parent.generic->vel;
	      else vel += parent.generic->vel;
	    }
	    parent.generic = 0;
	  }
	} else if (IsWarping(0) && IsValidObjectPointer(parent.generic,__FILE__,__LINE__) ) {
	  blockadeFrames = 0;
	  parent.generic = 0;
	} else {
	  blockadeFrames = 0;
	  cycle = 1;
	}
	
	
	/**********************************************/
	/* check proximity to sun                     */
	/* check every [star_prox_check_frames] frame */
	/**********************************************/
	if ((frame % star_prox_check_frames) == 0 && !IsWarping())
	  game->CheckProximityToStars(this);
	
	/* Experimental: turn off stealth device when firing */
	//if (firingTubes) {
	//if (bus.stealthMod && bus.stealthMod->IsActive()) {
	//bus.stealthMod->deactivate();
	//stealth = 1;
	//}
	//}
	
	if (stealthLevel == 0 && (firingTubes || tubeSelector)) {
	  
	  /* fire weapons */
	  bus.FireWeaponModules(this, comVec, comVel, lockedTubes);
	  
	  if (!tubeSelector)
	    firingTubes = 0;
	  
	}
	
	if (bitmap.IsSpin() || !game->IsRobotPlayer(this) && bitmap.IsTime()) {
	  /* calculate rotation rate */
	  if (bus.impulseMod) {
	    rotRate = (char)max(min(mass/(thrust_to_rotation*
					  bus.impulseMod->fPar(0)+1),
				    max_rotation_interval),1);
	  }
	  if (frame % rotRate == 0 &&
	      bus.impulseMod) {
	    signed char tTurning = (turning)? turning : seekTurning;
	    if (heading == 0 && tTurning == -1) 
	      heading += bitmap.numFrames;
	    heading += tTurning;
	    if (heading >= bitmap.numFrames) {
	      heading -= bitmap.numFrames;
	    }
	  }
	  offset = heading;
	  /* } else
	     offset = 0; */ //redundant
	  frame++;
	} else if (bitmap.IsTime()) {
	  
	  rotRate = default_rotation_rate;
	  
	  frame++;
	  
	  offset = RotationType((frame/rotRate) %
				bitmap.N());
	  heading = offset;
	  
	}
      } else {
	
	offset = RotationType((frame - bitmapStartFrame) % bitmap.N());
	
	frame++;
	
	/* if the status is inconsistent with the bitmap correct it */
	if (bitmap.IsTransitional() && !IsTransitioning()) {
	  if (bitmap.index == game->transitionBitmapIndexStart)
	    stat = entering_warp;
	  else
	    stat = exiting_warp;
	}
	
	// if frame counter reaches object lifetime and object is terminal
	// set bitmapIndex to -1.
	
	if (frame >= bitmapStartFrame + bitmap.N()) {
	  
	  if (bitmap.IsTerminal()) { 
	    bitmap = NoObjectBitmap;
	  } else if (IsTransitioning()) {
	    
	    bitmap = game->objectBitmapList[originalBitmapIndex];
	    bitmap.init();
	    offset = heading;
	    /* toggle visibility */
	    vis = !IsWarping(0);
	    transitSlide = 0;
	    if (stat == exiting_warp)
	      stat = idle;
	    else if (stat == entering_warp) {
	      stat = warping;
	      
	      if (game->soundEvent[warping_event] != -1)
		PlaySoundOnScreen(game->soundEvent[warping_event], 1);
	      
	    }
	  }
	}
      }
      
      /* check collisions by object type */
      if (!parent.generic && !IsExploding() && !IsDone()) {
	pp = Ping();
      }
    }
  
  if ( IsValidObjectPointer(avoidPP.obj.generic,__FILE__,__LINE__) && 
       (assign != manual || game->IsRobotPlayer(this)) ) {
    Avoid(avoidPP);
  }
  
  /* reset the missile avoidance PingPacket -- this must be at the end */
  /* because the missile updates come after */
  avoidPP.initialize();
  
  /* set global velocity variable */
  if ( IsValidObjectPointer(parent.generic,__FILE__,__LINE__) ) {
    if ( IsValidObjectPointer(parent.generic->parent.generic,__FILE__,__LINE__) )
      tvel = vel + parent.generic->vel + 
	parent.generic->parent.generic->vel;
    else tvel = vel + parent.generic->vel;
  } else
    tvel = vel;
  
  /* reset ping counter -- this must be at the end because the missile
     updates come after */
  numPings = 0;
  
  /***************************************************************/
  /* Don't send a msg for the following ship situations:         */
  /*   o   Warping                                               */
  /*   o   human players ships                                   */
  /*                                                             */
  /* Send msgs for each ship each second.                        */
  /***************************************************************/
  if (IsServer() &&
      (frameNumber + (game->ship.indexOf(this))) % aishipUpdateRate == 0 &&
      !IsExploding() && !IsWarping() && game->IsRobotPlayer(this))
    SendObjectTrackingMsg();
  
}

void ShipClass::CheckDuty(short force) {

  static char msg[128];
  static PingPacket pp;

  /* for most details, duty should only be checked according to the */
  /* duty interval                                                  */
  short checkNow = frame %
    max(1, (frameRate*
	    game->rank[game->player[playerIndex].rank].dutyInterval)) == 0 ||
    force;

  GenericClass* g;
  slong d;

  switch (duty) {

  case escort_detail:
    /* the ship looks for a higher ranking officer and follows it */

    if (checkNow) {
      targetPP = Ping(ship_target, nearest_leader);
      if (IsValidObjectPointer(targetPP.obj.generic, __FILE__, __LINE__)) {
	FollowTarget();
	strcpy(msg, "1st Officer> We have been assigned escort duty.");
	game->PostGameMessage(msg, game->ship.indexOf(this), (short)raceID, personal);
      }
    }
    
    break;

  case patrol_detail:

    if (checkNow) {
      Assign(patrol);
      strcpy(msg, "1st Officer> We have been assigned patrol duty.");
      game->PostGameMessage(msg, game->ship.indexOf(this), (short)raceID, personal);
    }

    break;

  case defend_detail:

    g = game->objectList[short(dutyPar)];

    if ( IsValidObjectPointer(g,__FILE__,__LINE__) )
      {
	if ( checkNow ) {
	  d = (g->now - now).Mag();
	  if (g && d > max_patrol_dist) {
	    sprintf(msg,
		    "1st Officer> We are returning to defense duty.  %s",
		    "Press / to be released from duty.");
	    game->PostGameMessage(msg, game->ship.indexOf(this), (short)raceID, personal);
	    
	    /* if the object is a planet go to it, otherwise follow it */
	    if (g->Type() == planet) {
	      GoPlanet(game->TypeListIndex(g));
	    } else if (g->Type() == ship) {
	      FollowObj(g);
	    }
	  }
	} else if (!g->IsFriendlyTo(this) || g->IsExploding()) {
	  
	  duty = none;
	  dutyPar = 0;
	  GoHome(attack);
	  
	}
      }
    
    break;
    
  case protect_detail:
    /* the ship should never go beyond [max_patrol_dist] pixels from */
    /* the assigned object                                           */
    /* this one is checked every frame                               */
    g = game->objectList[short(dutyPar)];
    if ( IsValidObjectPointer(g,__FILE__,__LINE__) )
      {
	d = (g->now - now).Mag();
	
	if (g && d > max_patrol_dist) {
	  
	  sprintf(msg,
		  "1st Officer> We are returning to protection duty.  %s",
		  "Press / to be released from duty.");
	  game->PostGameMessage(msg, game->ship.indexOf(this), (short)raceID, personal);
	  
	  /* if the object is a planet go to it, otherwise follow it */
	  if (g->Type() == planet) {
	    GoPlanet(game->TypeListIndex(g));
	  } else if (g->Type() == ship) {
	    FollowObj(g);
	  }
	}
      }
    
    if (assign == manual) {
      
      /* check for enemies approaching */
      pp = Ping(TargetType(ship_target | colony_target), nearest_enemy);
      
      if ( IsValidObjectPointer(pp.obj.generic,__FILE__,__LINE__) && 
	   pp.dist < (int)attack_dist) {
	
	if (pp.obj.generic->Type() == colony &&
	    dutyPar && pp.obj.colony->planetIndex == dutyPar - game->planetObjectIndexStart) {
	  AttackObj(pp.obj.generic);
	  sprintf(msg, "1st Officer> We are attacking a hostile colony.");
	} else {
	  FollowObj(pp.obj.generic);
	  sprintf(msg, "1st Officer> We have been assigned protection duty.");
	}
	
	game->PostGameMessage(msg, game->ship.indexOf(this), (short)raceID, personal);
      }
    }
    
    break;

  case harass_detail:
    /* the ship should try to acquire a weaker ship target and attack it */

    if (checkNow) {
      targetPP = Ping(ship_target, nearest_weaker_enemy);

      if (IsValidObjectPointer(targetPP.obj.generic, __FILE__, __LINE__)) {
	AttackTarget();
	sprintf(msg,
		"1st Officer> We are returning to harassment duty.  %s",
		"Press / to be released from duty.");
	game->PostGameMessage(msg, game->ship.indexOf(this), (short)raceID, personal);
      }
    }

    break;

  case conquer_detail:
    /* the ship should try to acquire a colony target and attack it */

    if (checkNow) {
      targetPP = Ping(colony_target, nearest_enemy);

      if (IsValidObjectPointer(targetPP.obj.generic, __FILE__, __LINE__)) {
	AttackTarget();
	sprintf(msg,
		"1st Officer> We are returning to conquest duty.  %s",
		"Press / to be released from duty.");
	game->PostGameMessage(msg, game->ship.indexOf(this), (short)raceID, personal);
      }
    }
    break;

  case attack_detail:
    /* the ship should try to acquire any target and attack it */

    if (checkNow) {
      targetPP = Ping(TargetType(ship_target | colony_target), nearest_enemy);

      /* notify any following ships that they should attack as well */
      game->SendAssignToFleet(this, attack, targetPP.obj.objLong);

      if (IsValidObjectPointer(targetPP.obj.generic, __FILE__, __LINE__)) {
	AttackTarget();
	sprintf(msg,
		"1st Officer> We are returning to attack duty.  %s",
		"Press / to be released from duty.");
	game->PostGameMessage(msg, game->ship.indexOf(this), (short)raceID, personal);
      }
    }
    break;
  case colonize_detail:
    /* the ship should try to pickup friendly colonies and transport */
    /* them to other planets                                         */
    
    if (checkNow) {

      short planetTargetIndex;

      if (bus.numPorts) {
	if( bus.dockMod[0] && !bus.dockMod[0]->workPar[1]) {
	  /* the ship has a dock and it is available       */
	  /* find a friendly planet that is ready to spawn */
	  
	  /* if there are free colonies nearby then harvest them first,          */
	  /* otherwise find a planet to harvest.                                 */
	  /*                                                                     */
	  pp = Ping(free_colony_target, nearest_friendly);
	  if ( IsValidObjectPointer(pp.obj.generic,__FILE__,__LINE__) && 
               pp.dist < (int)free_colony_pickup_range) {
	    if (pp.dist < (int)min_approach_dist) {
	      bus.dockMod[0]->workPar[0] = pp.obj.objLong;
	      //bus.dockMod[0]->activate();
	      /* dock activation */
	      bus.dockMod[0]->SendDockActivationMsg();
	    } else {
	      /* dock deactivation */
	      bus.dockMod[0]->ReceiveDockDeactivationMsg();
	      FollowObj(pp.obj.colony);
	    }
	  } else {
	    planetTargetIndex = PlanetAdvisor(planet_to_harvest);
	    if (planetTargetIndex > 0) {
	      if (stat == orbiting &&
		  parent.planet == game->planet + planetTargetIndex &&
		  bus.IsDockFree(0)) {
		/* ship is in orbit -- pick up a colony */
		parent.planet->colony->SpawnToDock(this);
	      } else if (assign != go_planet && assign != establish_orbit ||
			 assignPar2 != planetTargetIndex) {
		if (IsDisconnectedClient() ||
		    IsConnectedClient() && IsLocalPlayer(this)) {
		  sprintf(msg,
			  "1st Officer> We are enroute to %s to pick up a colony.",
			  game->planet[planetTargetIndex].name);
		  game->PostGameMessage(msg, game->ship.indexOf(this), (short)raceID, personal);
		}
		GoPlanet(planetTargetIndex);
	      }
	    } else
	      GoHome(attack); /* fix me -- find a different behavior */
	  }
	} else {
	  /* the ship has an object docked already.          */
	  /* if it is a ship then wait until it leaves       */
	  /* if it is a colony then transport it to a planet */
	  
	  ShipClass* s = (ShipClass*) bus.dockMod[0]->workPar[0];
	  
	  if ( IsValidObjectPointer(s) && s->Type() == colony &&
	      (planetTargetIndex = PlanetAdvisor(planet_to_colonize)) != 0) {

	    pp = s->Ping(planet_target, specific, (short)planetTargetIndex);

	    if (pp.dist < (ulong) pp.obj.generic->bitmap.size * usub_scale / 2) {
	      /******************************************/
	      /* ship is over planet -- drop off colony */
	      /******************************************/
	      ReleaseDock(0);

	      if (IsDisconnectedClient() || IsConnectedClient() && IsLocalPlayer(this)) {
		sprintf(msg, "1st Officer> We have released the colony over %s.",
			game->planet[planetTargetIndex].name);
		game->PostGameMessage(msg, game->ship.indexOf(this), (short)raceID, personal);
	      }

	    } else if (assign != follow_object ||
		       targetPP.obj.generic != game->planet + planetTargetIndex) {

	      if (IsDisconnectedClient() || IsConnectedClient() && IsLocalPlayer(this)) {
		sprintf(msg,
			"1st Officer> We are enroute to %s to drop off a colony.",
			game->planet[planetTargetIndex].name);
		game->PostGameMessage(msg, game->ship.indexOf(this), (short)raceID, personal);
	      }

	      FollowObj(game->planet + planetTargetIndex, (short)s->pop.InUnitsOf(100000));
	    }
	  }
	}
      }
    }
    break;
  }

}

char* ShipClass::Query(ShipClass* g, short post, short force) {
  static char msg[1024];
  static char nam[max_name_char];

  msg[0]='\0';
  static PingPacket pp;

  if ((IsFriendlyTo(g) || game->player[g->playerIndex].IsDeveloper()) &&
      game->ship.indexOf(g) == localSlotNumber ||
      force) {
    ObjectUnion t;
    t.generic = game->objectList[short(dutyPar)];

    if (Type() == ship && !IsLocalPlayer(this))
      strcpy(nam, game->player[playerIndex].name);
    else
      strcpy(nam, "1st Officer");

    if (stat == under_construction) {
      if (post)
	sprintf(msg, "%s> We are under construction.", nam);
      else
	strcpy(msg, "under construction.");
    } else if (stat == awaiting_construction) {
      if (post)
	sprintf(msg, "%s> We are awaiting construction.", nam);
      else
	strcpy(msg, "awaiting construction.");
    } else if (duty == defend_detail) {
      if ( IsValidObjectPointer(t.generic,__FILE__,__LINE__) &&
           t.generic->Type() == planet) {
	if (post)
	  sprintf(msg, "%s> We are defending planet %s.", nam, t.planet->name);
	else
	  sprintf(msg, "defending planet %s.", t.planet->name);
      } else if ( IsValidObjectPointer(t.generic,__FILE__,__LINE__) &&
                  t.generic->Type() == ship) {
	if (post)
	  sprintf(msg, "%s> We are defending %s %s.", 
		  nam,
		  game->rank[game->player[t.ship->playerIndex].rank].name,
		  game->player[t.ship->playerIndex].name);
	else 
	  sprintf(msg, "defending %s %s.", 
		  game->rank[game->player[t.ship->playerIndex].rank].name,
		  game->player[t.ship->playerIndex].name);
      }
    } else if (duty == protect_detail) {
      if ( IsValidObjectPointer(t.generic,__FILE__,__LINE__) &&
           t.generic->Type() == planet) {
	if (post)
	  sprintf(msg, "%s> We are protecting planet %s.", nam,
		  t.planet->name);
	else
	  sprintf(msg, "protecting planet %s.", t.planet->name);
      } else if ( IsValidObjectPointer(t.generic,__FILE__,__LINE__) &&
                  t.generic->Type() == ship) {
	if (post)
	  sprintf(msg, "%s> We are protecting %s %s.", 
		  nam,
		  game->rank[game->player[t.ship->playerIndex].rank].name,
		  game->player[t.ship->playerIndex].name);
	else
	  sprintf(msg, "protecting %s %s.", 
		  game->rank[game->player[t.ship->playerIndex].rank].name,
		  game->player[t.ship->playerIndex].name);
      }
    } else { /* *** comment this case *** */
      switch (assign) {
      case manual:
	if (post)
	  sprintf(msg, "%s> We are awaiting orders.", nam);
	else
	  strcpy(msg, "awaiting orders");
	break;
      case attack:
	if (IsValidObjectPointer(targetPP.obj.generic, __FILE__, __LINE__)){
	  if (targetPP.obj.generic->Type() == ship) {
	    if (post)
	      sprintf(msg, "%s> We are attacking the %s", 
		      nam,
		      game->player[targetPP.obj.ship->playerIndex].shipName);
	    else
	      sprintf(msg, "attacking the %s", 
		      game->player[targetPP.obj.ship->playerIndex].shipName);
	  } else if (targetPP.obj.generic->Type() == colony) {
	    if (post)
	      sprintf(msg, "%s> We are attacking a %s colony.", 
		      nam,
		      game->race[targetPP.obj.generic->raceID].name);
	    else
	      sprintf(msg, "attacking a %s colony", 
		      game->race[targetPP.obj.generic->raceID].name);
	  }
	} else { /* *** comment this case *** */
	  if (post)
	    sprintf(msg, "%s> We are seeking a target to attack.", nam);
	  else
	    strcpy(msg, "seeking a target to attack.");
	}
	break;
      case follow_object:
	if (IsValidObjectPointer(targetPP.obj.generic, __FILE__, __LINE__)){
	  switch (targetPP.obj.generic->Type()) {
	  case ship:
	    if (post)
	      sprintf(msg, "%s> We are escorting the %s", 
		      nam,
		      game->player[targetPP.obj.ship->playerIndex].shipName);
	    else
	      sprintf(msg, "escorting the %s", 
		      game->player[targetPP.obj.ship->playerIndex].shipName);
	    break;
	  case colony:
	    if (post)
	      sprintf(msg, "%s> We are escorting a %s colony.", 
		      nam,
		      game->race[targetPP.obj.generic->raceID].name);
	    else
	      sprintf(msg, "escorting a %s colony", 
		      game->race[targetPP.obj.generic->raceID].name);
	    break;
	  case planet:
	    if (post)
	      sprintf(msg, "%s> We are enroute to %s",
		      nam,
		      targetPP.obj.planet->name);
	    else
	      sprintf(msg, "enroute to %s",
		      targetPP.obj.planet->name);
	    break;	    
	  default :
	    break;
	  }
	}
	break;
      case go_dock:
      case go_multi_dock:
	if (assign == go_dock)
	  pp = Ping(ship_target, nearest_port);
	else
	  pp = Ping(ship_target, nearest_multi_port);

	if ( IsValidObjectPointer(pp.obj.generic,__FILE__,__LINE__) ) {
	  if (post)
	    sprintf(msg, "%s> We are returning to the %s for repairs.", 
		    nam,
		    game->player[pp.obj.ship->playerIndex].shipName);
	  else
	    strcpy(msg, "returning for repairs.");
	}
	break;
      case establish_orbit:
	if (post)
	  sprintf(msg, "%s> We are establishing orbit around %s",
		  nam,
		  game->planet[short(assignPar2)].name);
	else
	  strcpy(msg, "establishing orbit");
	break;
      case go_planet:
	if (post)
	  sprintf(msg, "%s> We are enroute to %s",
		  nam,
		  game->planet[short(assignPar2)].name);
	else
	  sprintf(msg, "enroute to %s",
		  game->planet[short(assignPar2)].name);
	break;
      default:
	if (post)
	  sprintf(msg, "%s> I don't know.", nam);
	else
	  strcpy(msg, "unknown");
      }
    }

    if (post)
      game->PostGameMessage(msg, game->ship.indexOf(g), (short)g->raceID, personal);

  }

  return msg;
}

void ColonyClass::Update() {
  /* update the colony object -- updates position, velocity, bitmap, frame,
     and checks collsions. */
  static PingPacket pp;
  
  short lockedTubes = 0;
  lastBitmap = bitmap;
  
  if (active == 0)
    return; /* do not run inactive objects -- verify that this is necessary */
  
  if (!IsActive()) {
    /* remove dead colonies from the game */
    game->ReceiveDestroyObjectMsg(this, 0);
    bitmap = NoObjectBitmap;
    lastBitmap = NoObjectBitmap;

    /* record event in log */
    logFile << TypeName() << " " << game->TypeListIndex(this) <<
		 " (slot " << game->ObjectListIndex(this) << 
		 ") deactivated on frame " << game->frame << endl;

    return;
  } else { /* *** Is Active *** */
    
    CalculateMass();
    
    /**************************************/
    /* events that happen once per second */
    /**************************************/

    if (frame % frameRate == 0) {
      
      if (lockObj.planet) {
	/* grow population by racial reproductive rate (~.01%) every second */
	pop.GrowBy((game->race[raceID].growthRate-100000) * Loyalty() / 100 +
		   100000);

      } else if (frame % (10 * frameRate) == 0 &&
		 !IsValidObjectPointer(dockObj)) {

	/* once every ten seconds... */

	/* reset the lastDockObjIndex after 10 seconds with no contact */
	lastDockObjIndex = -1;

	/* free floating colonies slowly lose supplies */
	supply = max(supply - 1, 0);

	if (supply == 0) {
	  game->SendDestroyObjectMsg(this);
	  return;
	}

      }
      
      /* calculate targeting accuracy */
      if ( IsValidObjectPointer(lockObj.generic,__FILE__,__LINE__) )
	accuracy = (short)min(lockObj.planet->accBonus +
			      kills * pop.log10() / 
			      score_to_crew_experience_level, 100);
      else
	accuracy = (short)min(kills * pop.log10() / 
			      score_to_crew_experience_level, 100);
    }
    
    /* reload tubes every 3 minutes */
    if (frame % frameRate * 180)
      bus.ReloadTubes();
    
    /* remove colonies with no population */
    if (pop.billions <= 0 && pop.lBillions <= 0) {
      game->SendDestroyObjectMsg(this);
      return;
    }
    
    /**********************************************/
    /* check proximity to sun                     */
    /* check every [star_prox_check_frames] frame */
    /**********************************************/
    if ((frame % star_prox_check_frames) == 0 && !lockObj.generic && !IsWarping())
      game->CheckProximityToStars(this);

    if (game->ModeIsSet(GameStateClass::autoSpawn) &&
	(IsDisconnectedClient() || IsServer()) &&
	!IsPending(spawning) &&
	!game->colonyPending && 
	IsValidObjectPointer(lockObj.generic,__FILE__,__LINE__) &&
	pop.InUnitsOf(1000) > 
	game->planet[planetIndex].OptimalPopulation(game->race[raceID]).
	InUnitsOf(1000) &&
	PlanetAdvisor()) {
      
      if (game->numColonies < game->maxColonies - game->extraColonies) {
	PopStruct newPop = 
	  game->planet[planetIndex].OptimalPopulation(game->race[raceID]);
	newPop.GrowBy(90000);    /* reduce by 10% */
	PopStruct newColPop = min(pop - newPop,
				  PopStruct((game->maxColonyMillions >= 1000)*
					    (short)(game->maxColonyMillions/1000),
					    (short)(game->maxColonyMillions % 1000) * 1000000));
	//pop = newPop;
	
	planetIndex = game->TypeListIndex(lockObj.planet);
	
	SendSpawnMsg(newColPop, (short)raceID);
      }
    }
    
    /* check for boost in maxSpeed */
    if (maxSpeed > design->maxSpeed) {
      /* lower limit gradually */
      maxSpeed -= (maxSpeed-design->maxSpeed) / frameRate + 1; /* dlh */
      vel.AddSubjectToLimit(nullVector, maxSpeed);
    }
    
    /* repair ship systems */
    /* ships with factories can repair their own armor */
    bus.RepairSystems(repairRate, short(bus.factoryMod != 0 && lockObj.generic));
    if ( IsValidObjectPointer(dockObj)) {

      /* only ships docked with factories can repair armor */
      if (dockObj->bus.factoryMod != 0 && lockObj.planet)
	bus.RepairSystems(dockObj->repairRate, 1);
      else
	bus.RepairSystems(dockObj->repairRate);

      viewObj.ship = dockObj;
    }
    
    if (!lockObj.generic) {
      if (lastDockObjIndex == -1) {
	/* this colony was not recently dropped, so it can decide for itself */
	/* where it wants to go                                              */
	if (!planetIndex || frame % frameRate == 0)
	  planetIndex = PlanetAdvisor();
	pp = Ping(planet_target, specific, planetIndex);
	SeekObj(pp, 100);
      }
      
    } else if (lockObj.planet->colony != this && !IsExploding()) {
      logFile << "ERROR> found an orphan ";
      ShortProfile();
      logFile << " on frame " <<
	game->frame << endl;
      Explode("it was orphaned by its planet"); /* this shouldn't happen */
    }  
    
    /* update the DesignedClass section */
    DesignedClass::Update();

    if (stealthLevel == 0 && IsValidObjectPointer(avoidPP.obj.generic,__FILE__,__LINE__) )
      bus.FireDefensiveWeaponModules(this, avoidPP.disp, avoidPP.obj.generic->tvel, lockedTubes);
    
    /* check status of ship's power */
    /* if out of power -- explode */
    if (bus.powerOut == 0 && !IsExploding()) {
      
      if (!lockObj.generic)
	Explode("it was damaged beyond repair");
      else if (pop.billions <= 0 && pop.lBillions <=0)
	Explode("all inhabitants were lost");
    }
    
    if (bitmap.IsLoop()) {
      
      /* reset priority so that colony may respond to help */
      assignPriority = 0;

      /***************************************************************/
      /* colonies on planets should always defend themselves against */
      /* approaching enemy colonies                                  */
      /***************************************************************/
      if ( IsValidObjectPointer(lockObj.generic,__FILE__,__LINE__) ) {
	pp = Ping(colony_target, nearest_enemy);
	if ( IsValidObjectPointer(pp.obj.generic,__FILE__,__LINE__) && 
	     pp.dist < (int)alert_dist)
	  AttackObj(pp.obj.generic);
      }

      /* if bitmap has angles: */
      if (assign == attack || assign == follow_object) {
	
	if ((!lockObj.generic || assign == attack) && frame % forget_interval) {
	  
	  ProcessAssignment(lockedTubes);
	  
	} else
	  assign = manual;
      } else
	assign = manual;
      
      /* fire weapons */
      //if (lockObj.generic == 0)
      if (assign == attack && (firingTubes || tubeSelector)) {
	bus.FireWeaponModules(this, comVec, comVel, lockedTubes);

	if (!tubeSelector)
	  firingTubes = 0;

      }

      if (bitmap.IsSpin()) {
	/* calculate rotation rate */
	if (bus.impulseMod)
	  rotRate = (char)max(min(mass/(thrust_to_rotation*
				  bus.impulseMod->fPar(0)+1),
			    max_rotation_interval),1);
	if (frame % rotRate == 0 &&
	    bus.impulseMod) {
	  signed char tTurning = (turning)? turning : seekTurning;
	  if (heading == 0 && tTurning == -1) 
	    heading += bitmap.numFrames;
	  heading += tTurning;
	  if (heading >= bitmap.numFrames) {
	    heading -= bitmap.numFrames;
	  }
	}
	offset = heading;

	frame++;

      } else if (bitmap.IsTime()) {

	rotRate = default_rotation_rate;

	frame++;
      
	offset = RotationType((frame/rotRate) %
			      bitmap.N());
	heading = offset;
      
      }
    } else {
    
      offset = RotationType((frame - bitmapStartFrame) % bitmap.N());

      frame++;
    
    // if frame counter reaches object lifetime and object is terminal
    // set bitmapIndex to -1.
      if (frame == bitmapStartFrame + bitmap.N()) {
	if (bitmap.IsTerminal()) { 
	  bitmap = NoObjectBitmap;
	} else if (IsTransitioning()) {
	  bitmap = game->objectBitmapList[originalBitmapIndex];
	  bitmap.init();
	  offset = heading;
	  /* toggle visibility */
	  vis = !IsWarping();
	  transitSlide = 0;
	}
      }
    }

    /* check collisions by object type */
    /* if (!parent.generic && !IsExploding() && !IsDone()) {
       PingPacket pp = Ping();
       } */
  }

  /* reset the missile avoidance PingPacket -- this must be at the end */
  /* because the missile updates come after */
  avoidPP.initialize();

  /* set global velocity variable */
  if ( IsValidObjectPointer(lockObj.generic,__FILE__,__LINE__) ) {
    tvel = lockObj.generic->tvel;
  } else {

    pp = Ping(planet_target, all);
    
    if ((IsServer() || IsDisconnectedClient()) &&
	!IsPending(spawning) &&
	!game->colonyPending && 
	pp.obj.planet != parent.planet &&
	!dockObj &&
	pp.dist < (ulong) pp.obj.generic->bitmap.size * 3 * usub_scale / 4 &&
	pp.obj.planet->OptimalPopulation(game->race[raceID]).InUnitsOf(1000) &&
	!IsExploding() && bitmap.index != -1) {

      /**************************************************************************/
      /* NOTE: the check on bitmap.index != -1 is an early indication that this */
      /*       object is about to become inactive.  Without this check there is */
      /*       a possibility that an object can blow up, complete the explosion */
      /*       sequence, and then for one frame land on a planet.               */
      /*       This condition manifests itself with an orphan colony error mes- */
      /*       sage in the client log.                                          */
      /**************************************************************************/

      /* colony lands on planet */

      /*                                                          */
      /* a spawn colony message consists of the following:        */
      /* o  object index of colony that spawned                   */
      /* o  object index of colony that spawned                   */
      /* o  race of spawned colony                                */
      /* o  population of colony (as two longs)                   */
      /*                                                          */

      /*                                                          */
      /* a spawn-to-dock message consists of the following:       */
      /* o  object index of ship with dock                        */
      /* o  object index of colony that spawned                   */
      /* o  race of spawned colony                                */
      /* o  population of colony (as two longs)                   */
      /*                                                          */

      /*                                                          */
      /* a colony-changes-race message consists of the following: */
      /* o  object index of lastAttacker (for credit)             */
      /* o  object index of colony                                */
      /* o  new race of colony                                    */
      /* o  zero population                                       */
      /*                                                          */

      /* a colony-lands-on-planet message consists of the following: */
      /* o  object index of planet                                   */
      /* o  object index of colony that landed                       */
      /* o  race of colony                                           */
      /* o  population of colony (as two longs)                      */
      /*                                                             */
      /* comment: this message is used for an uncontested landing.   */
      /*          if the planet is already occupied then the server  */
      /*          should send a colony-consolidation msg instead     */
      /*                                                             */

      /*                                                           */
      /* a colony-consolidation message consists of the following: */
      /* o  object index of colony that is removed                 */
      /* o  object index of colony that remains                    */
      /* o  race of colony                                         */
      /* o  population of colony (as two longs)                    */
      /*                                                           */
      /* comment: if one of the colonies is attached to a planet   */
      /*          then the resulting colony should be attached to  */
      /*          the same planet                                  */
      /*                                                           */

      strcpy(name, pp.obj.planet->name);

      if (pp.obj.planet->colony) {
	if (pp.obj.planet->colony->IsFriendlyTo(this)) {

	  /* existing colony is friendly -- destroy this colony and add */
	  /* to existing population */
	  if (!pp.obj.planet->colony->IsPending(spawning))
	    pp.obj.planet->colony->
	      SendSpawnMsg(pp.obj.planet->colony->pop + pop,
			   (short)pp.obj.planet->colony->raceID,
			   game->ObjectListIndex(this));

	  //pp.obj.planet->colony->pop += pop;
	  //game->DestroyObject(this);

	} else if (pop.InUnitsOf(10000) * game->offensiveFactor >
		   pp.obj.planet->colony->pop.InUnitsOf(100)) {

	  /* existing colony is unfriendly but has a smaller population */
	  /* so it is conquered                                         */
	  PopStruct newPop = pop;
	  if (game->offensiveFactor)
	    newPop = pop - (pp.obj.planet->colony->pop /
			    game->offensiveFactor) * 100;

	  SendSpawnMsg(newPop,
		       (short)raceID,
		       game->ObjectListIndex(pp.obj.planet->colony));

	  //pop -= pp.obj.planet->colony->pop;
	  //pp.obj.planet->colony->targetPP.obj.colony = this;
	  //pp.obj.planet->colony->
	  // SendDistressSignal(pp.obj.planet->colony->pop.InUnitsOf(1000000));
	  //game->DestroyObject(pp.obj.planet->colony);
	  //pp.obj.planet->colony = this;

	  /* spawn a number of powerups as a reward */
	  //SpawnRandomModules(num_reward_plugins);

	} else {
	  /* existing colony has successfully defended its planet */
	  if (!pp.obj.planet->colony->IsPending(spawning)) {

	    PopStruct newPop = pp.obj.planet->colony->pop;
	    if (game->offensiveFactor)
	      newPop = pp.obj.planet->colony->pop - ((pop / 100) * game->offensiveFactor);
	    
	    pp.obj.planet->colony->
	      SendSpawnMsg(newPop,
			   (short)pp.obj.planet->colony->raceID,
			   game->ObjectListIndex(this));

	  }
	  //pp.obj.planet->colony->pop -= pop;
	  //pp.obj.planet->colony->targetPP.obj.colony = pp.obj.planet->colony;
	  //pp.obj.planet->colony->
	  //SendDistressSignal(pp.obj.planet->colony->pop.InUnitsOf(1000000));
	  //game->DestroyObject(this);
	}
      } else /* colony lands on planet */
	//pp.obj.planet->colony = this;
	SendSpawnMsg(pop,
		     (short)raceID,
		     game->ObjectListIndex(pp.obj.planet));	

      //pp.obj.generic->raceID = pp.obj.planet->colony->raceID;
      //lockObj = pp.obj;
      //planetIndex = game->TypeListIndex(pp.obj.planet);
      //if (planetIndex == 0)
      //game->DestroyObject(this);
      //else {
      //sprintf(msg, "A colony has arrived at %s",
      //	game->planet[planetIndex].name);

      //if (planetIndex == game->race[raceID].targetIndex &&
      //	    !conquered) {
      /* reset the colonization marker if the planet is conquered */
      /* or colonized for the first time                          */
      //  game->race[raceID].targetIndex = 0;
      //	  game->race[raceID].targetIndexRank = 0;
      //	}
	
      //	game->PostGameMessage(msg, raceID, raceID, broadcast);

      //}

      //now = pp.obj.generic->now;
    }
    tvel = vel;
  }

  /* reset ping counter -- this must be at the end because the missile
     updates come after */
  numPings = 0;

  /***************************************************************/
  /* Don't send a msg for the following colony situations:       */
  /*   o   Warping                                               */
  /*   o   planet-based                                          */
  /*   o   carried by ship                                       */
  /*                                                             */
  /* Send msgs for one colony each second.  Each colony is up-   */
  /* dated once every max_colonies seconds.                      */
  /***************************************************************/

  if ( IsServer() && 
       (frameNumber % aishipUpdateRate == 0) && 
       ((frameNumber/aishipUpdateRate) % game->maxColonies == 
	game->colony.indexOf(this)) &&
       !IsExploding() && 
       !IsWarping() && 
       !dockObj && 
       !lockObj.generic )
    SendObjectTrackingMsg();
  
}

char* ColonyClass::Query(ShipClass* g, short post, short force) {
  /* if post is set then call PostGameMessage at end to display message */

  static char msg[1024]; 

  msg[0] = '\0';

  slong l1, l2;

  if ((IsFriendlyTo(g)|| game->player[g->playerIndex].IsDeveloper()) &&
      game->ship.indexOf(g) == localSlotNumber ||
      force) {

    if (assign != manual && !lockObj.generic) {
      return ShipClass::Query(g, post);
    }

    slong rawProd = 0;
    slong popProd = 0;
    if (bus.factoryMod) {
      /* raw-material-based resources */
      rawProd = bus.factoryMod->par[0];
      /* population-based resources */
      popProd = ((ShipClass*)this)->pop.Sqrt();
    }

    popProd = max(popProd, 1);

    /* give raw material credit for planet attributes */
    /* this resource should only accumulate if the colony is ship-producing */
    if ( IsValidObjectPointer(lockObj.generic,__FILE__,__LINE__) ) {
      rawProd += lockObj.planet->ExploitationPotential();
    }

    rawProd = max(rawProd, 1);

    if (!lockObj.generic) {
      if (post)
	sprintf(msg, "colony> We are bound for %s.",
		game->planet[PlanetAdvisor()].name);
      else
	sprintf(msg, "bound for %s.", game->planet[PlanetAdvisor()].name);
    } else if (Type() == colony && bus.factoryMod && !bus.factoryMod->IsActive()) {
      /* && popProd  < lower_production_limit) { */

      if (popProd  < lower_production_limit) {
	l1 = plugin_production_req / popProd / frameRate;
	l2 = max(plugin_production_req-bus.factoryMod->specPar[0],0) /
	  popProd / frameRate;
      } else {
	l1 = forced_plugin_production_limit / popProd / frameRate;
	l2 = max(forced_plugin_production_limit-bus.factoryMod->specPar[0],0) /
	  popProd / frameRate;
      }
      if (post)
	sprintf(msg, "colony> Our module production rate is once every %lds.  Time to completion: %lds", l1, l2);
      else if (l2) 
	sprintf(msg, "new power-up in %ld s", l2);
      else
	strcpy(msg, "new power-up available");

    } else if (bus.factoryMod && bus.factoryMod->workPar[0])/* ship producer */ {
      ShipClass* s = (ShipClass*)bus.factoryMod->workPar[0];
      slong popTot;
      slong rawTot;
      s->CalculateRemainingValueAndMass(popTot, rawTot);
      slong popRate = max(popTot - bus.factoryMod->specPar[0],0) / popProd;
      slong rawRate = max(rawTot - bus.factoryMod->specPar[1],0) / rawProd;
      if (post)
	sprintf(msg, "colony> We are building a %s.  Projected completion in %ld s",
		s->design->name,
		max(popRate, rawRate) / frameRate);
      else
	sprintf(msg, "%s done in %ld s",
		s->design->name,
		max(popRate, rawRate) / frameRate);
    } else {
      if (post)
	sprintf(msg, "colony> We are awaiting construction orders.");
      else
	sprintf(msg, "awaiting build orders.");
    }

    if (post)
      game->PostGameMessage(msg, game->ship.indexOf(g), (short)raceID, personal);

  }

  return msg;
}

void MissileClass::Update() {
  /* update the generic object -- updates position, velocity, bitmap, frame,
     and checks collisions.  Packs network list (this can be removed when
     server implements Game Class). */

  static PingPacket nearPP;
  static PingPacket reversePP;

  numPings = 0;
  lastBitmap = bitmap;
  
  /* check for dead objects and reset or remove them depending on type */
  if (!IsActive()) {
    /* remove dead missiles from the game */
    game->DestroyObject(this);
    bitmap = NoObjectBitmap;
    lastBitmap = NoObjectBitmap;
  } else {

    CalculateMass();

    //logFile << "*** Updating missile " << this - game->missile << endl;
    DesignedClass::Update();
    nearPP.initialize();

    /***************************************/
    if (!IsExploding()) {

      switch (design->tracking) {
      case bomb:
	//nearPP = Ping(planet_target, all); /* necessary? fix me */
	if ( IsValidObjectPointer(lockObj.generic,__FILE__,__LINE__) /* && nearPP.dist < design->actRange*/) {
	  vel = lockObj.generic->tvel;
	  lockObj.generic = 0;
	}
	SeekHeading(vel);
	/* fall through */
      case dumb:
	if (triggerObjType)
	  nearPP = Ping(TargetType(triggerObjType),
			TargetSelector(triggerObjQualifier));
	break;
      case smart:
	
	if (IsValidObjectPointer(targetPP.obj.generic, __FILE__, __LINE__) && 
            targetPP.obj.generic->CanBeSeenBy(this)) {

	  /* heat seeking missiles can only sense their target if its */
	  /* engines are active */
	  if (!(design->trigger & on_heat)) {
	    targetPP = Ping(target_target);
	    nearPP = targetPP;
	  } else if (targetPP.obj.generic->Type() >= designed && 
		     targetPP.obj.designed->throttle) {
	    targetPP = Ping(target_target);
	    nearPP = targetPP;
	  }
	  
	  if (!targetLock) {
	    nearPP = Ping(TargetType(triggerObjType),
			  TargetSelector(triggerObjQualifier));
	    if ( IsValidObjectPointer(nearPP.obj.generic,__FILE__,__LINE__) )
	      targetPP = targetPP.Sniff(this, nearPP.obj.generic);
	  }
	  
	  SeekHeading(targetPP.disp);
	  
	  if (IsValidObjectPointer(targetPP.obj.generic, __FILE__, __LINE__)) /* object is active */
	    SeekObj(targetPP, design->seekSens); 
	  else
	    Acquire();
	  
	} else if ( IsValidObjectPointer(lockObj.generic,__FILE__,__LINE__) ) {
	  /* this object is locked in position relative to another object */

	  /* scan for objects that come within the activation range */
	  nearPP = Ping(TargetType(triggerObjType),
			TargetSelector(triggerObjQualifier));

	  if ( IsValidObjectPointer(nearPP.obj.generic,__FILE__,__LINE__) && 
	       nearPP.dist < design->actRange) {
	    lockObj.generic = 0;
	    /* target fix */
	    targetPP = nearPP;
	  }

	} else {
	  /* this object does not have a target, so acquire a new one */
	  Acquire();
	}
	break;
      }
    }

    if (design->tracking < smart) /* dumb or bomb */
      targetPP = nearPP;

    /* alert enemy planet to close proximity to enemy fire */
    if ( IsValidObjectPointer(nearPP.obj.generic,__FILE__,__LINE__) && 
         IsValidObjectPointer(parent.generic,__FILE__,__LINE__) &&
	nearPP.obj.generic->IsEnemyOf(parent.generic) &&
	nearPP.dist < (uint)max((int)alert_dist, 2 * design->actRange *
			  (design->trigger == on_proximity || 
			   design->trigger == on_time))) {

      if (nearPP.obj.generic->Type() == planet &&
	  nearPP.obj.planet->colony) {

	/* automatic self-defense */
	nearPP.obj.planet->colony->assignPriority = 0;

	/* check for bombs and mines and smart and timed weapons */
	if (!(design->tracking == bomb ||
	    design->trigger == on_proximity || design->trigger == on_time || design->tracking == smart)) /* attack the parent */
	  nearPP.obj.planet->colony->AttackObj(parent.generic);
	else /* attack the bomb or mine or smart weapon (high priority) */ {

	  /* invert the PingPacket so that it points to the missile */
	  reversePP = nearPP.Invert(this);
	  
	  /* test the PingPacket to find the closest missile each frame */
	  nearPP.obj.planet->colony->avoidPP.Sniff(reversePP, 0);
	  
	  //nearPP.obj.planet->colony->assignPriority = 0;
	  //nearPP.obj.planet->colony->AttackObj(this);

	}

      } else if (nearPP.obj.generic->Type() >= ship) {

	if (nearPP.obj.ship->AutoResponse() &&
	    (!nearPP.obj.ship->IsWarping() ||
	     nearPP.obj.ship->assign == attack && !nearPP.obj.ship->IsWarping())) {
	  
	  /* if the ship or colony is already attacking something then */
	  /* only override if their target is far away                 */
	  if (nearPP.obj.ship->assign != attack || 
	      (nearPP.obj.ship->assign == attack &&
	       nearPP.obj.ship->targetPP.dist > (int)min_attack_dist)) {
	    
	    /* automatic self-defense */
	    nearPP.obj.ship->assignPriority = 0;
	    
	    /* check for bombs and mines */
	    if (!(design->tracking == bomb ||
		  design->trigger == on_proximity ||
		  design->trigger == on_time)) /* attack the parent */
	      nearPP.obj.ship->AttackObj(parent.generic);
	    else {/* attack the bomb or mine */
	      //nearPP.obj.ship->AttackObj(this);
	    }
	  }
	}
	
	//if (nearPP.dist < avoid_dist) {
	
	/* invert the PingPacket so that it points to the missile */
	reversePP = nearPP.Invert(this);
	
	/* test the PingPacket to find the closest missile each frame */
	nearPP.obj.ship->avoidPP.Sniff(reversePP, 0);
	
	//}
      }
    }
    
    if ( bitmap.IsSpin() )
      {
	if (heading == 0 && seekTurning == -1)
	  {
	    heading += bitmap.N();
	  }
	heading += seekTurning;
	if (heading >= bitmap.N())
	  {
	    heading -= bitmap.N();
	  }
	offset = heading;
	frame++;
      }
    else
      {
	offset = RotationType((frame - bitmapStartFrame) % bitmap.N());
	
	frame++;
	
	// if frame counter reaches object lifetime and object is terminal
	// set bitmapIndex to -1.
	if (frame == bitmapStartFrame + bitmap.N() && bitmap.IsTerminal())
	  { 
	    bitmap = NoObjectBitmap;
	  }
      }
    
    /* check collisions by object type */
    if ( !IsExploding() && !IsDone()) {
      if (onImpact &&
	  IsValidObjectPointer(targetPP.obj.generic, __FILE__, __LINE__) &&
	  !IsInInternetCafe(targetPP.obj.generic) &&
	  targetPP.dist < (ulong) (bitmap.size +
			   targetPP.obj.generic->bitmap.size) * usub_scale / 2UL &&
	  (targetPP.obj.generic->Type() >= designed && !targetPP.obj.designed->IsWarping(0) ||
	   targetPP.obj.generic->Type() < designed)) {

	/* missile is plug-in (start) */
	if (duty == plugin && targetPP.obj.generic->Type() == ship &&
	    targetPP.obj.ship->IsActive() &&
	    !targetPP.obj.ship->IsExploding()) {

	  if (IsDisconnectedClient() ||
	      (IsConnectedClient() && IsLocalPlayer(targetPP.obj.ship)) ||
	      (IsServer() && game->IsRobotPlayer(targetPP.obj.ship))) {

	    /* simulate an explosion by playing the explosion sound and */
	    /* displaying the explosion bitmap                          */
	    targetPP.obj.generic->PlaySoundOnScreen(design->expSoundIndex);

	    if (bitmap.index != design->expBitmapIndex)
	      {
		bitmap = game->objectBitmapList[design->expBitmapIndex];
		bitmap.init();
	      }

	    targetPP.obj.ship->
	      PlugInModuleByCopy(bus.moduleList[1]);

				     

	  }

	  if (!targetPP.obj.generic->IsFriendlyTo(this)) {
	    /* friendly ships should protect plugins                  */
	    /* this code is pulled from ShipClass::SendDistressCall   */
	    /* it should be moved to a DesignedClass function so that */
            /* the MissileClass can use it                            */
	    /*-----------------*/
            short j;
	    for (j=0; j < game->numShips; j++) {
	      if (game->ship[j].IsFriendlyTo(this) && 
		  game->ship[j].AutoResponse() &&
		  (game->ship[j].assign == manual || 
		   game->ship[j].assign == follow_object)) {
		if (targetPP.obj.ship->CanBeSeenBy(game->ship+j))
		  game->ship[j].AttackObj(targetPP.obj.generic);
	      } 
	    }
	    /*-----------------*/
	  }
	} else  if (!targetPP.obj.generic->IsExploding()) {

	  /* if the target object is cloaked then notify the */
	  /* owner of the missile of the new target location */
	  if (targetPP.obj.generic->Type() >= ship &&
	      targetPP.obj.ship->stealth > 1 &&
	      IsValidObjectPointer(parent.generic,__FILE__,__LINE__) &&
	      parent.generic->Type() >= ship &&
	      parent.ship->targetPP.obj.generic == targetPP.obj.generic) {
	    
	    parent.ship->targetPP.disp = 
	      *targetPP.obj.generic - *parent.ship;
	    parent.ship->targetPP.dist =
	      parent.ship->targetPP.disp.Mag();
	  }

	  //
	  // Make RAM warhead scale with velocity.  Set to zero if ship is warping.
	  //
	  slong warhead = design->warhead; 
	  if (duty == ram_weapon)
	    {
	      if ( !lockObj.ship->IsWarping() )
		{
		  warhead += (targetPP.obj.generic->tvel - tvel).Mag() / vel_to_damage;
		}
	      else
		{
		  warhead = 0;
		}
	    }

	  if (!(design->trigger & on_heat)) {/* not a heat seeker */
	    if (design->tracking != bomb)
	      targetPP.obj.generic->Hit(warhead, parent.designed,
					::Dir[vel.Heading(game->maxAngles)]);
	    else
	      targetPP.obj.generic->Hit(warhead, parent.designed,
					::Dir[vel.Heading(game->maxAngles)],
					design->push, 0, bomb_hit);

	  } else
	    targetPP.obj.generic->Hit(design->warhead, parent.designed,
				      ::Dir[vel.Heading(game->maxAngles)],
				      design->push, 0, heat_hit /* heat seeker */);

	}
	
	vel = targetPP.obj.generic->tvel;

	Explode("it detonated on a target");
        /*logFile << 2 <<" "<<this-game->missile<<" "<<
                targetPP.obj.generic->now.x<<" "<<
                targetPP.obj.generic->bitmap.size<<" "<<
                frameNumber << endl;*/
      } else if (design->trigger &
		 on_time && (ulong) frame >=
		 (ulong) frameRate * design->actRange / 1000) {
	/* if elapsed time given by range parameter (in milliseconds) is matched */
	/* then detonate */
	Explode("it was triggered by time");
      } else if (design->trigger & on_proximity && targetPP.obj.generic &&
		 targetPP.dist < design->actRange) {
	Explode("it detonated when a target came within range.");
      }
      
      /* check fuel exhaustion */
      if (!bus.powerOut) {
	if (bus.aveFunc < module_function_scale)
	  Explode("its fuel was exhausted");
	else {
	  bitmap = NoObjectBitmap;
	  bitmapStartFrame = frame;
	  vel = nullVector;
	  game->DestroyObject(this);
	  return;
	}
        /*logFile << 3 <<" "<<this-game->missile<<" "<<
                frameNumber << endl;*/
      }
      
      /* check for very long life span */
      if (frame > 10000)
	Explode("it exceeded the maximum life span.");
    } else if (design->trigger == on_proximity || design->tracking == bomb) {
      /* continue blasting until bitmap terminates */
      game->BlastObjects(this, 2*design->actRange, design->warhead, design->push, design->vortex);
    } else if (design->trigger == on_time) {
      /* continue blasting until bitmap terminates */
      game->BlastObjects(this, design->actRange / 10 * sub_scale, design->warhead, design->push, design->vortex);
    } 
  }

  if (!lockObj.generic)
    tvel = vel;
  else {

    if (duty == ram_weapon) {

      if (lockObj.ship->IsWarping() && !IsExploding() )
	{
	  active = 0;
	  Explode();
	}

      slong h = lockObj.ship->bus.moduleList[short(assignPar1)].specPar[1];
      if (!h)
	h = lockObj.ship->bitmap.size / 2;

      /***************************************************************/
      /* missiles that have been fired by a ram will have the firing */
      /* {parent) ship for lockObj.  assignpar1 will contain the     */
      /* slot number for the tube that fired it.  Use that link to   */
      /* determine the angle that the missile should be locked in    */
      /* relative to the parent ship.                                */
      /***************************************************************/
      short offset = (short)((lockObj.ship->bus.moduleList[short(assignPar1)].specPar[2] + lockObj.ship->heading +
		      lockObj.ship->bitmap.numFrames) %	lockObj.ship->bitmap.numFrames);
      now = lockObj.ship->now + lockObj.ship->bitmap.dir[offset] * h;

      heading = lockObj.ship->heading;

      offset = heading;

      vel = nullVector;
    }

    tvel = vel + lockObj.generic->tvel;
  }
}

void GenericClass::Recycle() {
  /* finally update position for all types */
  now += (tvel) * dt;
}

void ShipClass::initialize() {
  /* default constructor -- no special characteristics */

  DesignedClass::initialize();

  typeOfObject = ship;

  sensorThreshold = sensor_threshold;
  dockObj = 0;
  distressFrame = 0;
  portNum = 0;
  videoMode = 0;
  lastVideoMode = 0;
  stealthLevel = 0;
  accuracy = 0;
  tubeSelector = 0;
  defensiveTubeSelector = 0;
  pending = 0;
  blockadeFrames = 0;
  short i;
  for (i=0; i<max_tubes_per_ship; i++) {
    tubeOffset[i] = 0;
    beamOffset[i] = 0;
  }
}

void ShipClass::initializeWith(short i, short pI, short numMod) {

  DesignedClass::initializeWith(i,
				game->objectBitmapList[game->shipDesign[game->player[pI].shipIndex].
						      bitmapIndex]);
  typeOfObject = ship;

  /* network object constructor */
  sensorThreshold = sensor_threshold;
  playerIndex = pI;
  portNum = 0;

  accuracy = 0;

  /* reset kill counter */
  game->player[playerIndex].SetStat(game->player[playerIndex].numKills,
                                         0
					 );
  //game->player[playerIndex].numKills = 0;

  /* reset active score counter */
  game->player[playerIndex].SetStat(game->player[playerIndex].activeScore,
                                         0
					 );
  //game->player[playerIndex].activeScore = 0;

  stealthLevel = 0;
  
  mode = (mode_auto + mode_orbit) * (playerIndex < game->numRobotPlayers);

  if (InitializeFromShipDesign(game->ship+i, numMod)) {
    /* we were unable to allocate memory for the module list   */
    /* so record an error message.  There is no recovery from  */
    /* this condition, but attempt to deactivate this ship and */
    /* hope for the best */
    logFile <<
      "ERROR> ShipClass: unable to allocate memory for new ship on frame " <<
      game->frame << endl;
  }
  viewObj.ship = game->ship + i;

  distressFrame = 0;

  lastBitmap = NoObjectBitmap;
  
  /* escaped pilots recover racial identity */
  // game->player[playerIndex].raceId = raceID;

  //if (game->IsRobotPlayer(this))
  //  duty = sentinel;
  //else

  //duty = none;
  //dutyPar = game->ObjectListIndex(game->planet + game->race[raceID].home);

  /* if the design has a factory then bind the new ship to the nearest */
  /* friendly colony                                                   */
  /* if (design->factory) {
     PingPacket nearPP = Ping(colony_target, nearest_friendly);
     if ( IsValidObjectPointer(nearPP.obj.generic,__FILE__,__LINE__) ) 
     game->BindShipTo(this, nearPP.obj.colony);
    }*/

  AssignToPlanet(i);

  dockObj = 0;
  videoMode = 0;
  lastVideoMode = 0;
  tubeSelector = 0;
  defensiveTubeSelector = 0;
  pending = 0;
  blockadeFrames = 0;

  short j;
  for (j=0; j<max_tubes_per_ship; j++) {
    tubeOffset[j] = 0;
    beamOffset[j] = 0;
  }

  /* set crew level */
  pop = OptimalCrew();
}

short ShipClass::InitializeFromShipDesign(ShipClass* s, short numMod) {
  /* initialize design-dependent parameters */
  design = game->shipDesign + game->player[playerIndex].shipIndex;
  raceID = game->player[playerIndex].raceId;

  return InitializeFromDesign(s, numMod);

}

short ColonyClass::InitializeFromColonyDesign(ColonyClass* s, short numMod) {
  /* initialize design-dependent parameters */

  if (InitializeFromDesign(s, numMod))
    return -1;

  parent.planet = game->planet + planetIndex;
  raceID = design->raceID;

  /* set colony pointer on planet */
  game->planet[planetIndex].colony = s;
  game->planet[planetIndex].raceID = s->raceID;

  /* set viewObj to self -- move to more generic location */

  viewObj.colony = s;

  lockObj = parent;
  now = parent.generic->now;
  gnow = now;

  return 0;

}

short ShipClass::InitializeFromDesign(ShipClass* s, short numMod) {

  /* if numMod was not specified then build full array */
  if (!numMod)
    numMod = (short)design->bus.numMod;

  // mass = design->mass;
  maxSpeed = design->maxSpeed;
  
  //raceID = design->raceID;
  
  bus.InitializeWith((short)design->bus.numSlots);
  if (bus.moduleList.rootOf() == 0) {
    logFile << "ERROR> InitializeFromDesign: unable to initialize module list with " <<
      bus.numSlots << " slots on frame " <<
      game->frame << endl;
    return -1;
  }
    
  bus.numMod = numMod;
  PlugInModules(numMod);

  bus.MapFunctions(s, this);

  bitmap = game->objectBitmapList[design->bitmapIndex];
  bitmap.init();
  originalBitmapIndex = bitmap.index;

  /* reset the frame counter */
  frame = 0;

  /* reset stealth */
  stealth = 1;
  stealthLevel = 0;

  CalculateMass();

  return 0;
}

void ColonyClass::initialize() {

  ShipClass::initialize();

  typeOfObject = colony;

  name[0] = 0;
  assign = manual;
  stat = idle;
  active = 0;
  repairRate = 0;
  mode = mode_auto;
  planetIndex = 0;
  kills = 0;
  lastDockObjIndex = -1;
  playerIndex = 0;
  supply = max_supply;
};

void ShipClass::AssignToPlanet(short i, short index) {

  const double pi = 3.14159265;

  static CoordinateClass cOffset;
  static PingPacket newPP;

  double phase = (2 * i * pi / game->numShips);
  parent.planet = game->planet+game->
    race[game->RaceIdToIndex(game->player[playerIndex].raceId)].home; 
  now = parent.generic->now;
  gnow = now;

  if (index)
    newPP = Ping(planet_target, specific, index);
  else
    newPP = Ping(planet_target, nearest_friendly);

  if ( IsValidObjectPointer(newPP.obj.planet) ) {
    parent = newPP.obj;
    now = parent.generic->now;
    gnow = now;
  } else {
    newPP = Ping(planet_target, nearest);
    if ( IsValidObjectPointer(newPP.obj.planet) ) {
      parent = newPP.obj;
      now = parent.generic->now;
      gnow = now;
    } /* else {
	 game->player[playerIndex].raceId = parent.planet->raceID;
	 raceID = parent.planet->raceID;
	 } */
  }

  slong rad;
  if (bus.impulseMod) {
    rad = 10 * sub_scale;

    cOffset.initializeWith(nlong(rad * cos(phase)), nlong(rad * sin(phase)));

    now += cOffset;
    
    assign = go_planet;
    stat = idle;
    vel = parent.generic->tvel;
    parent.generic = 0;
  } else {
    rad = 2 * parent.generic->bitmap.size * sub_scale;

    cOffset.initializeWith(nlong(rad * cos(phase)), nlong(rad * sin(phase)));

    now += cOffset;
    assign = manual;
    stat = orbiting;
    vel = VelKep(newPP);
  }
  
  assignOverRide = manual;
  assignPar1 = 0;
  assignPar2 = game->TypeListIndex(newPP.obj.planet);
}

PingPacket& ShipClass::Ping(TargetType tt,
			    TargetSelector ts,
			    short n,
			    ulong range,
			    CoordinateClass lead) {

  /* ping function for ships */
  static PingPacket minPP;
  static PingPacket thisPP;

  minPP.obj.generic = 0;
  short middle, i;

  //
  // Pathetic attempt to head off bug I haven't been
  // able to to track down.
  //
  if ( !this || (this && this->check != (int)0xaaaaaaaa) )
    {
      return minPP;
    }

  if (Type() == ship)
    middle = game->ship.indexOf(this);
  else /* Type() == colony */
    middle = game->numShips;
  
  if (ts == nearest || ts == all) {
    if (tt == target_target ) {
      if ( IsValidObjectPointer(targetPP.obj.generic) )
	{
	  minPP = PingPacket(this, targetPP.obj.generic, 0, lead);
	}
      return minPP;
    } else if (tt == all_target) {
      for (i = 0; i < game->NumObjects(); i++) {
	if ( IsValidObjectPointer(game->objectList[i],__FILE__,__LINE__) &&
	     game->objectList[i]->IsActive() ) {
	  minPP.Sniff(this, game->objectList[i], range);
	  if (minPP.obj.generic == game->objectList[i] &&
	      game->objectList[i]->Type() == planet &&
	      ((PlanetClass*)game->objectList[i])->colony) {
	    ColonyClass* c = ((PlanetClass*)game->objectList[i])->colony;
	    if ( IsValidObjectPointer(c) )
	      {
		if (minPP.dist < (ulong) c->bitmap.size * usub_scale/2)
		  minPP.obj.colony = c;
	      }
	  }
	}
      }
      return minPP;
    }
    if (tt & star_target) {
      for (i=0; i<game->numPlanets; i++) {
	if (game->planet[i].minor == -1) {
	  thisPP = minPP.Sniff(this, &game->planet[i], range);
	}
      }
    }
    if (tt & planet_target) {
      for (i = 1 /* skip sun */;
	   i < game->N(GameStateClass::planets); i++) {
	thisPP = minPP.Sniff(this, &game->planet[i], /* range */ max_grav_dist);
	if (ts == all && thisPP.dist < (int)max_grav_dist && 
	    !parent.generic) {
	  /* if within max_grav_dist feel gravity */
	  Attract(thisPP, 0, 1);
	}
      }
    }
    if (tt & ship_target) {
      if (ts == all)
	middle = game->numShips;
      for (i = 0; i < middle; i++) {
	/* allow mouse to select ships under construction */
	/*if (game->ship[i].IsAvailable())*/
	if (game->ship[i].IsAvailable())
	  minPP.Sniff(this, &game->ship[i], range);
      }
      for (i = middle+1; i < game->numShips; i++)
	if (game->ship[i].IsAvailable())
	  minPP.Sniff(this, &game->ship[i], range);
    }
    if (tt & colony_target) {
      for (i = 0; i < game->N(GameStateClass::colonies); i++) {
        if ( IsValidObjectPointer(game->colonyList[i],__FILE__,__LINE__) ) {
          if (game->colonyList[i]->IsActive() &&
	      !game->colonyList[i]->IsExploding())
	    thisPP = minPP.Sniff(this, game->colonyList[i], range);
	} else {
          logFile << "ShipClass::Ping colonyList[" << i << "] = NULL" << endl;
	}
      }
    } else if (tt & free_colony_target) {
      for (i = 0; i < game->N(GameStateClass::colonies); i++) {
        if ( IsValidObjectPointer(game->colonyList[i],__FILE__,__LINE__) ) {
	  if (game->colony[i].IsActive() &&
	      !IsValidObjectPointer(game->colonyList[i]->lockObj.generic) &&
	      !game->colonyList[i]->IsExploding())
	    thisPP = minPP.Sniff(this, game->colonyList[i], range);
	} else {
          logFile << "ShipClass::Ping colonyList[" << i << "] = NULL" << endl;
	}
      }
    }
    if ((tt & parent_target) && IsValidObjectPointer(parent.generic,__FILE__,__LINE__) ) {
      minPP = PingPacket(this, parent.generic, pc_long_max);
    }
  } else if (ts == nearest_friendly) {
    if (tt & ship_target) {
      for ( i = 0; i < middle; i++) {
	if (game->ship[i].IsAvailable() &&
	    game->ship[i].IsFriendlyTo(this))
	  minPP.Sniff(this, &game->ship[i], range);
      }
      for (i = middle+1; i < game->numShips; i++)
	if (game->ship[i].IsAvailable() &&
	    game->ship[i].IsFriendlyTo(this))
	  minPP.Sniff(this, &game->ship[i], range);
    }
    if (tt & colony_target) {
      for (i = 0; i < game->N(GameStateClass::colonies); i++) {
        if ( IsValidObjectPointer(game->colonyList[i],__FILE__,__LINE__) ) {
	  if (game->colonyList[i]->IsActive() &&
	      game->colonyList[i]->IsFriendlyTo(this) &&
	      IsValidObjectPointer(game->colonyList[i]->lockObj.generic,__FILE__,__LINE__) &&
	      !game->colonyList[i]->IsExploding())
	    thisPP = minPP.Sniff(this, game->colonyList[i], range);
	} else {
          logFile << "ShipClass::Ping colonyList[" << i << "] = NULL" << endl;
	}
      }
    } else if (tt & free_colony_target) {
      for (i = 0; i < game->N(GameStateClass::colonies); i++) {
        if ( IsValidObjectPointer(game->colonyList[i],__FILE__,__LINE__) ) {
          if (game->colony[i].IsActive() &&
	      game->colonyList[i]->IsFriendlyTo(this) &&
	      !IsValidObjectPointer(game->colonyList[i]->lockObj.generic) &&
	      !IsValidObjectPointer(game->colonyList[i]->dockObj) &&
	      !game->colonyList[i]->IsExploding())
	    thisPP = minPP.Sniff(this, game->colonyList[i], range);
	} else {
          logFile << "ShipClass::Ping colonyList[" << i << "] = NULL" << endl;
	}
      }
    }
    if (tt & planet_target) {
      for (i = 1 /* skip sun */;
	   i < game->N(GameStateClass::planets); i++) {
	if (game->planet[i].IsFriendlyTo(this) && game->planet[i].colony)
	  thisPP = minPP.Sniff(this, &game->planet[i], range);
      }
    }
  } else if (ts == nearest_enemy) {
    if (tt & ship_target) {
      if (Type() == ship)
	middle = game->ship.indexOf(this);
      else /* Type() == colony */
	middle = game->numShips;
      for (i = 0; i < middle; i++)
	if (game->ship[i].IsAvailable() &&
	    game->ship[i].IsEnemyOf(this) && 
	    !game->ship[i].IsExploding() &&
	    game->ship[i].CanBeSeenBy(viewObj.ship))
	  minPP.Sniff(viewObj.designed, &game->ship[i], range);
      for (i = middle+1; i < game->numShips; i++)
	if (game->ship[i].IsAvailable() &&
	    game->ship[i].IsEnemyOf(this) &&
	    !game->ship[i].IsExploding() &&
	    game->ship[i].CanBeSeenBy(viewObj.ship))
	  minPP.Sniff(viewObj.designed, &game->ship[i], range);
    }
    if (tt & colony_target) {
      for (i = 0; i < game->numColonies; i++) {
        if ( IsValidObjectPointer(game->colonyList[i],__FILE__,__LINE__) ) {
	  if (game->colonyList[i]->IsActive() &&
	      !game->colonyList[i]->IsFriendlyTo(this) &&
	      !game->colonyList[i]->IsExploding() &&
	      game->colonyList[i]->CanBeSeenBy(viewObj.ship))
	    thisPP = minPP.Sniff(this, game->colonyList[i], range);
	} else {
          logFile << "ShipClass::Ping colonyList[" << i << "] = NULL" << endl;
	}
      }
    } else if (tt & free_colony_target) {
      for (i = 0; i < game->numColonies; i++) {
        if ( IsValidObjectPointer(game->colonyList[i],__FILE__,__LINE__) ) {
	  if (game->colonyList[i]->IsActive() &&
	      !IsValidObjectPointer(game->colonyList[i]->lockObj.generic) && 
	      !game->colonyList[i]->IsFriendlyTo(this) &&
	      !game->colonyList[i]->IsExploding() &&
	      game->colonyList[i]->CanBeSeenBy(viewObj.ship))
	    thisPP = minPP.Sniff(this, game->colonyList[i], range);
	} else {
          logFile << "ShipClass::Ping colonyList[" << i << "] = NULL" << endl;
	}
      }
    }
    if (tt & missile_target) {
      for (i = 0; i < game->numMissiles; i++) {
        if ( IsValidObjectPointer(game->missileList[i],__FILE__,__LINE__) ) {
	  if (game->missileList[i]->IsActive() &&
	      !game->missileList[i]->IsFriendlyTo(this) &&
	      !game->missileList[i]->IsExploding())
	    thisPP = minPP.Sniff(this, game->missileList[i], range);
	} else {
          logFile << "ShipClass::Ping missileList[" << i << "] = NULL" << endl;
	}
      }
    }
  } else if (ts == nearest_weaker_enemy) {
    if (tt & ship_target) {
      if (Type() == ship)
	middle = game->ship.indexOf(this);
      else /* Type() == colony */
	middle = game->numShips;
      for (i = 0; i < middle; i++)
	if (game->ship[i].IsAvailable() &&
	    game->ship[i].IsEnemyOf(this) && 
	    !game->ship[i].IsExploding() &&
	    game->ship[i].EffectiveValue() < EffectiveValue() &&
	    game->ship[i].CanBeSeenBy(viewObj.ship))
	  minPP.Sniff(viewObj.designed, &game->ship[i], range);
      for (i = middle+1; i < game->numShips; i++)
	if (game->ship[i].IsAvailable() &&
	    game->ship[i].IsEnemyOf(this) &&
	    !game->ship[i].IsExploding() &&
	    game->ship[i].EffectiveValue() < EffectiveValue() &&
	    game->ship[i].CanBeSeenBy(viewObj.ship))
	  minPP.Sniff(viewObj.designed, &game->ship[i], range);
    }
  } else if (ts == nearest_leader) {
    if (tt & ship_target) {
      for (i = 0; i < middle; i++)
	if (game->ship[i].IsAvailable() && raceID == game->ship[i].raceID &&
	    game->player[playerIndex].rank < 
	    game->player[game->ship[i].playerIndex].rank &&
	    !game->ship[i].IsExploding())
	  minPP.Sniff(this, &game->ship[i], range);
      for (i = middle+1; i < game->N(GameStateClass::ships); i++)
	if (game->ship[i].IsAvailable() && raceID == game->ship[i].raceID &&
	    game->player[playerIndex].rank < 
	    game->player[game->ship[i].playerIndex].rank &&
	    !game->ship[i].IsExploding())
	  minPP.Sniff(this, &game->ship[i], range);
    }
  } else if (ts == nearest_port) {
    if (tt & ship_target) {
      for (i = 0; i < middle; i++)
	if (game->ship[i].IsAvailable() && raceID == game->ship[i].raceID && 
	    !game->ship[i].IsExploding() &&
	    game->ship[i].bus.numPorts && !IsValidObjectPointer(game->ship[i].dockObj))
	  minPP.Sniff(this, &game->ship[i], range);
      for (i = middle+1; i < game->N(GameStateClass::ships); i++)
	if (game->ship[i].IsAvailable() && raceID == game->ship[i].raceID &&
	    !game->ship[i].IsExploding() &&
	    game->ship[i].bus.numPorts && !IsValidObjectPointer(game->ship[i].dockObj))
	  minPP.Sniff(this, &game->ship[i], range);
    }
  } else if (ts == nearest_multi_port) {
    if (tt & ship_target) {
      for (i = 0; i < middle; i++)
	if (game->ship[i].IsAvailable() && raceID == game->ship[i].raceID && 
	    !game->ship[i].IsExploding() &&
	    game->ship[i].bus.numPorts > 1 && !IsValidObjectPointer(game->ship[i].dockObj))
	  minPP.Sniff(this, &game->ship[i], range);
      for (i = middle+1; i < game->N(GameStateClass::ships); i++)
	if (game->ship[i].IsAvailable() && raceID == game->ship[i].raceID && 
	    !game->ship[i].IsExploding() &&
	    game->ship[i].bus.numPorts > 1 && !IsValidObjectPointer(game->ship[i].dockObj))
	  minPP.Sniff(this, &game->ship[i], range);
    }
  } else /* specific */ {
    switch (tt) {
    case planet_target:
      minPP = PingPacket(this,&game->planet[n], 0, lead);
      break;
    case ship_target:
      if (game->ship[n].IsAvailable())
	minPP = PingPacket(this,&game->ship[n], 0, lead);
      break;
    case colony_target:
      if (game->colony[n].IsActive())
	minPP = PingPacket(this,&game->colony[n], 0, lead);
      break;
    case no_target:
    default:
      break;
    }
  }
  return minPP;
}

//
// the single point of release for the game engine
// there should be no other release mechanism
//
void ShipClass::ProcessFreeDockMsg(short p)
{
  if ( p >= 0 && p < bus.numMod && 
       bus.moduleList[p].moduleFunc == ModuleClass::dock )
    {
      ShipClass* s = (ShipClass*) bus.moduleList[p].workPar[0];
      if ( bus.moduleList[p].workPar[1] ) {
	if ( IsValidObjectPointer(s) )
	  {
	    mass -= s->mass;
	  }
	gmass = mass;
	if ( IsValidObjectPointer(s) )
	  {
	    if (s->Type() == colony)
	      {
		s->maxSpeed = vel.Mag();
	      }
	    s->viewObj.ship = s;
	  }
      }

      bus.moduleList[p].workPar[0] = 0;
      bus.moduleList[p].workPar[1] = 0;
      ClearPendingFlag(tractoring);

      /* dock deactivation */
      bus.moduleList[p].deactivate();
    }
  else
    {
      logFile << "ERROR> FreeDock: illegal port number" << endl;
      logFile << "                 port number = " << p << endl;
    }
}


void ShipClass::ReleaseDock(short p) {
  if ( !bus.dockMod[p] )
    {
      return;
    }
  ShipClass* s = (ShipClass*) bus.dockMod[p]->workPar[0];
  if ( IsValidObjectPointer(s,__FILE__,__LINE__) &&
       IsValidObjectPointer(this) )
    {
      if (bus.dockMod[p]->workPar[1] && s /* && !IsWarping() */) { /* port is occupied */
	if (s->dockObj == this) {
	  s->stat = idle;
	  s->ClearAssign();
	  
	  if (s->Type() == ship) {
	    s->FollowObj(this);
	    game->BindShipTo(s, this);
	  }
	  
	  s->FreeFromDock();
	} else {
	  logFile << "ERROR> ReleaseDock: illegal docked object for  " << TypeName() << " " << game->TypeListIndex(this) <<
	    " (" << s->TypeName() << " " << game->TypeListIndex(s) <<
	    "} on frame " << game->frame << endl;
	  s->dockObj = 0;
	  ProcessFreeDockMsg(bus.moduleList.indexOf(bus.dockMod[p]));
	  //FreeDock(bus.dockMod[p]-bus.moduleList);
    }
  }
    }
}

void ShipClass::Explode(char* reason, short spawn, short direct) {

  unsigned short objIndex = game->ObjectListIndex(this);
  short tobjIndex;
  if ( IsValidObjectPointer(lastAttacker.generic,__FILE__,__LINE__) )
    tobjIndex = game->ObjectListIndex(lastAttacker.generic);
  else
    tobjIndex = -1;
  
  if (!IsPending(exploding) && 
      (IsDisconnectedClient() ||
      (IsConnectedClient() && IsLocalPlayer(this)) ||
      (IsServer() && game->IsRobotPlayer(this)))) {

    logFile << "sent> a request to destroy a ";
    ShortProfile(); 
    logFile << " was made on frame " << game->frame <<
      " because " << reason << endl;

    SetPendingFlag(exploding);
    
    retMsgBuffer.PushMsg((uchar)SF_MSG_EXPLODESHIP,
                         objIndex,
			 (unsigned short)tobjIndex,
                         (int)S_CLIENT_TCP);

  } else if (direct) {
    HandleExplodeMsg(spawn);
  }  
}

void ColonyClass::Explode(char* reason, short spawn, short direct) {

  unsigned short objIndex = game->ObjectListIndex(this);
  short tobjIndex;
  if ( IsValidObjectPointer(lastAttacker.generic,__FILE__,__LINE__) )
    tobjIndex = game->ObjectListIndex(lastAttacker.generic);
  else
    tobjIndex = -1;

  if (IsDisconnectedClient() || IsServer()) {
    retMsgBuffer.PushMsg((uchar)SF_MSG_EXPLODESHIP,
                         objIndex,
			 (unsigned short) tobjIndex,
                         (int)S_CLIENT_TCP);

    SetPendingFlag(exploding);

    logFile << "sent> a request to destroy a ";
    ShortProfile(); 
    logFile << " was made on frame " << game->frame <<
      " because " << reason << endl;
  } else if (direct) {
    HandleExplodeMsg(spawn);
  }
  
}

void ShipClass::HandleExplodeMsg(short spawn) {
  slong par[3];

  static char msg[1024];
  static CoordinateClass blastDir;

  ClearPendingFlag(exploding);

  if ( IsExploding() )
    return;

  if ( IsValidObjectPointer(lastAttacker.generic,__FILE__,__LINE__) ) {

    int fratricide = lastAttacker.ship->raceID == raceID;
    
    int sign = 1;
    if ( fratricide )
      {
	sign = 0;
      }
    
    switch (lastAttacker.generic->Type()) {

    case ship: {
      /* a ship destroys a ship */
      if (Type() == ship) {
	
	slong bounty = game->player[playerIndex].activeScore * bounty_pct / 100  * game->bonusFactor;
	slong addScore = (design->reqRank + 1) * game->bonusFactor + bounty;

	/* give credit for bounty */
	slong cBonus = TotalValue() / 100 * game->bountyRightPct;

	game->player[lastAttacker.ship->playerIndex].
	  AddToStat(game->player[lastAttacker.ship->playerIndex].refValue,
		    cBonus*sign);
	//game->player[lastAttacker.ship->playerIndex].refValue += cBonus;

	if ( sf && sf->engine && sf->engine->displayEngine &&
             sf->engine->displayEngine->chat && 
             sf->engine->displayEngine->chat->channels && 
	     !game->IsRobotPlayer(lastAttacker.ship) && 
	     !game->IsRobotPlayer(this))
	  {
	    if ( game->player[lastAttacker.ship->playerIndex].rank >= 0 &&
		 game->player[lastAttacker.ship->playerIndex].rank < game->numRanks &&
		 game->player[playerIndex].rank >= 0 &&
		 game->player[playerIndex].rank < game->numRanks )
	      {
		sprintf(msg,"%s%s <kills> %s%s",
			game->rank[game->player[lastAttacker.ship->playerIndex].rank].name,
			game->player[lastAttacker.ship->playerIndex].name,
			game->rank[game->player[playerIndex].rank].name,
			game->player[playerIndex].name
			);
		sf->engine->displayEngine->chat->
		  channels[SF_CHANNEL_BROADCAST].AppendLine(msg,RGB_GREEN);
	      }
	  }
	
	
	if (IsLocalPlayer(lastAttacker.ship) &&
	    this != lastAttacker.ship)
	  {
	    
	    sprintf(msg, 
		    "1st Officer> We have received a bounty credit "
		    "of %s", Format(cBonus));
	    
	    game->PostGameMessage(msg,
				  game->ship.indexOf(lastAttacker.ship),
				  (short)lastAttacker.ship->raceID,
				  personal);
	  }
	
	int pi = lastAttacker.ship->playerIndex;
	

	//
	// Send a msg to chat console.
	//

	logFile << "note> " << 
	  game->rank[game->player[pi].rank].name <<
	  " " <<
	  game->player[pi].name << " received " <<
	  addScore*sign << " points, which includes a bounty of " <<
	  bounty*sign << " points and " << cBonus*sign << 
	  " credit for defeating " <<
	  game->rank[game->player[playerIndex].rank].name << " " <<
	  game->player[playerIndex].name << endl;
	
	//dlh Create a RAM cheat upon first kill by anybody
	//game->player[pi].numKills += game->bonusFactor;
	
	game->player[pi].AddToStat(game->player[pi].activeScore,
				   (ulong)addScore*sign
				   );
	game->player[pi].AddToStat(game->player[pi].totalScore,
				   (ulong)addScore*sign
				   );
	game->player[pi].AddToStat(game->player[pi].numKills,
				   (ulong)game->bonusFactor*sign
				   );
	game->player[pi].AddToStat(game->player[pi].numKillsTotal,
				   (ulong)game->bonusFactor*sign
				   );
	
      } else if (Type() == colony) {
	/* a ship destroys a colony */
	
	/* calculate bonus points */
	short bonus = (short)(pop.log10() / 2 * game->bonusFactor);
	
	if ( IsValidObjectPointer(lockObj.generic,__FILE__,__LINE__) )
	  bonus += (short)(lockObj.planet->ExploitationPotential() / 100 * game->bonusFactor);
	
	logFile << "note> " << 
	  game->rank[game->player[lastAttacker.ship->playerIndex].rank].name <<
	  " " <<
	  game->player[lastAttacker.ship->playerIndex].name << " received " <<
	  bonus*sign << " points for destroying a ";
	ShortProfile();
	logFile << endl;


	game->player[lastAttacker.ship->playerIndex].
	  AddToStat(game->player[lastAttacker.ship->playerIndex].activeScore,
			 (ulong)bonus*sign
			 );
	game->player[lastAttacker.ship->playerIndex].
	  AddToStat(game->player[lastAttacker.ship->playerIndex].totalScore,
			 (ulong)bonus*sign
			 );
	game->player[lastAttacker.ship->playerIndex].
	  AddToStat(game->player[lastAttacker.ship->playerIndex].strategicPoints,
			 (ulong)bonus*sign
			 );
	game->player[lastAttacker.ship->playerIndex].
	  AddToStat(game->player[lastAttacker.ship->playerIndex].numKills,
			 (ulong)game->bonusFactor*sign
			 );
	game->player[lastAttacker.ship->playerIndex].
	  AddToStat(game->player[lastAttacker.ship->playerIndex].numKillsTotal,
			 (ulong)game->bonusFactor*sign
			 );

      }
      
    }
    break;
    case colony:
      /* a colony destroys a ship or colony */

      /* give the colony credit for the kill */
      /* the number of kills a colony has determines its accuracy */
      lastAttacker.colony->kills++;

      logFile << "note> a ";
      lastAttacker.colony->ShortProfile();
      logFile << " has " << lastAttacker.colony->kills << " kills after destroying a ";
      ShortProfile();
      logFile << endl;

      break;

    default :
      break;

    }
  }
  
  /* increment number of times this player has been killed */
  if (Type() == ship) {

    game->player[playerIndex].
      AddToStat(game->player[playerIndex].numKilled,1);
    game->player[playerIndex].
      AddToStat(game->player[playerIndex].numKilledThisSession,1);

    if (playerIndex >= (int)game->numRobotPlayers) /* this is a human player */
      game->race[raceID].numPlayerDeaths++;
  }

  slong damage = 0;

  if ( IsValidObjectPointer(dockObj,__FILE__,__LINE__) &&
       IsValidObjectPointer(lastAttacker.generic,__FILE__,__LINE__) ) {
    /* blast direction */
    blastDir = *this - *dockObj;
    if ( blastDir.Mag() != 0 )
      {
	blastDir = blastDir * sub_scale / blastDir.Mag();
      }

    /* check for overflow */
    if ( game->damageToMass != 0 )
      {
	if (mass > 100000)
	  {
	    damage = max(mass / game->damageToMass * 10000, 0);
	  }
	else
	  {
	    damage = max(10000*mass/game->damageToMass, 0);
	  }
      }

    logFile << "note> a ";
    dockObj->ShortProfile();
    logFile << " took " << damage << 
      " points of damage in the explosion." << endl;

    /* apply damage */
    dockObj->Hit(damage, lastAttacker.designed,
		 blastDir, 100, 0, bypass_push_hit);
    /* free up port -- break link */
    FreeFromDock();
  } else if ( bus.dockMod ) {
    short i;
    for (i=0; i<bus.numPorts; i++)
      if (bus.dockMod[i]->workPar[1]) {
	ShipClass* docker = (ShipClass*) bus.dockMod[i]->workPar[0];
	if ( IsValidObjectPointer(docker,__FILE__,__LINE__) )
	  {
	    /* blast direction */
	    blastDir = *this - *docker;
	    if ( blastDir.Mag() != 0 )
	      {
		blastDir = blastDir * sub_scale / blastDir.Mag();
	      }
	    
	    /* check for overflow */
	    if ( game->damageToMass != 0 )
	      {
		if (mass > 100000)
		  {
		    damage = max(docker->mass / game->damageToMass * 10000, 0);
		  }
		else
		  {
		    damage = max(10000*docker->mass/game->damageToMass, 0);
		  }
	      }
	    
	    logFile << "note> a ";
	    docker->ShortProfile();
	    logFile << " took " << damage << 
	      " points of damage in the explosion." << endl;
	    
	    /* apply damage */
	    docker->Hit(damage,
			lastAttacker.designed, blastDir, 100, 0, bypass_push_hit);
	    /* free up port -- break link */
	    docker->FreeFromDock();
	  }
      } 
  }

  if (spawn) {
    short i;
    for (i = 0; i < bus.numMod; i++) {
      /* shut off all modules */
      bus.moduleList[i].deactivate();

      /* make sure all sound effects (other than the explosion) are off */
      if (sf && sf->mwl && bus.moduleList[i].activationSoundIndex != -1)
	sf->audio->Stop(sf->mwl->Wave(bus.moduleList[i].activationSoundIndex));

      if (bus.moduleList[i].func > module_function_scale / 2) {
	par[0] = 0;
	if ( bitmap.numFrames != 0 )
	  {
	    par[0] = (frame + 3*i) % bitmap.numFrames;
	  }
	par[2] = par[0];
	par[1] = 0;
	if ( bitmap.size != 0 )
	  {
	    par[1] = (frame + 3*i) % bitmap.size;
	  }
	Spawn(game->missileDesign[0], par, bus.moduleList[i].id, i + bus.moduleList[i].id);
      }
    }
  }

  vis = 1;
  stealth = 1;
  bitmapStartFrame = frame;
  bitmap = game->objectBitmapList[design->expBitmapIndex];
  offset = 0;
  ClearAssign();
  stat = idle;

  /* remove this object from parent variables in missile list */
  game->CleanOrphanObjects(this);
  PlaySoundOnScreen(design->expSoundIndex );

  /* record event in log */
  logFile << "received> a ";
    ShortProfile(); 
  logFile << " was destroyed";
  if ( IsValidObjectPointer(lastAttacker.generic,__FILE__,__LINE__) ) {
    logFile << " by a ";
    lastAttacker.generic->ShortProfile();
  }

  logFile << " on frame " << game->frame << endl;

}

slong ShipClass::PushTo(GameStateBuffer& gb, GameStateClass* g) {

  WRITE_SF_LOG(logFile << "Pushing ship section of " << TypeName() << " " <<
	       g->TypeListIndex(this) << "..." << endl);

  slong subClassSize = DesignedClass::PushTo(gb, g);

  char* bufferPos = gb.BufferPosition();

  CALL_PUSH_OBJ(gb,pop);
  CALL_PUSH_GAME_OBJ(gb, g,avoidPP);
  CALL_PUSH_OBJ(gb,tubeSelector);

  short i;
  for (i=0; i<max_tubes_per_ship; i++)
    CALL_PUSH_OBJ(gb,tubeOffset[i]);

  short designIndex;
  switch (Type()) {
  case ::ship:
    designIndex = g->shipDesign.indexOf(design);
    break;
  case ::colony:
    designIndex = (ColonyDesignStruct *)design-g->colonyDesign;
    break;
  default:
    designIndex = 0;
    break;
  }  

  CALL_PUSH_OBJ(gb,designIndex);
  CALL_PUSH_OBJ(gb,playerIndex);
  CALL_PUSH_OBJ(gb,portNum);
  CALL_PUSH_OBJ(gb,stealthLevel);
  CALL_PUSH_GAME_OBJ(gb, g, parent.generic);
  CALL_PUSH_GAME_OBJ(gb, g, dockObj);
  CALL_PUSH_OBJ(gb,distressFrame);
  CALL_PUSH_OBJ(gb,pending);
  CALL_PUSH_OBJ(gb,blockadeFrames);
  CALL_PUSH_OBJ(gb,accuracy);
  CALL_PUSH_OBJ(gb,videoMode);
  CALL_PUSH_OBJ(gb,lastVideoMode);

  return subClassSize + (gb.BufferPosition() - bufferPos);
}

slong ShipClass::PopFrom(GameStateBuffer& gb, GameStateClass* g) {

  WRITE_SF_LOG(logFile << "Popping ship section of " << TypeName() << " " <<
	       g->TypeListIndex(this) << "..." << endl);

  slong subClassSize = DesignedClass::PopFrom(gb, g);

  char* bufferPos = gb.BufferPosition();

  CALL_POP_OBJ(gb,pop);
  CALL_POP_GAME_OBJ(gb, g,avoidPP);
  CALL_POP_OBJ(gb,tubeSelector);

  short i;
  for (i=0; i<max_tubes_per_ship; i++)
    CALL_POP_OBJ(gb,tubeOffset[i]);

  short designIndex;
  CALL_POP_OBJ(gb,designIndex);

  switch (Type()) {
  case ship:
    design = g->shipDesign + designIndex;
    break;
  case colony:
    design = g->colonyDesign + designIndex;
    break;
  default:
    design = 0;
    break;
  }  

  CALL_POP_OBJ(gb,playerIndex);
  CALL_POP_OBJ(gb,portNum);
  CALL_POP_OBJ(gb,stealthLevel);
  CALL_POP_GAME_OBJ(gb, g,parent.generic);
  CALL_POP_GAME_OBJ(gb, g,dockObj);
  CALL_POP_OBJ(gb,distressFrame);
  CALL_POP_OBJ(gb,pending);
  CALL_POP_OBJ(gb,blockadeFrames);
  CALL_POP_OBJ(gb,accuracy);
  CALL_POP_OBJ(gb,videoMode);
  CALL_POP_OBJ(gb,lastVideoMode);

  return subClassSize + (gb.BufferPosition() - bufferPos);
}

void ShipClass::Acquire(ulong range) {
  static PingPacket nearPP;

  nearPP = Ping(TargetType(ship_target | colony_target), nearest_enemy);

  if (IsValidObjectPointer(nearPP.obj.generic,__FILE__,__LINE__) &&
      nearPP.dist <= range) {
    targetPP.obj.generic = nearPP.obj.generic;
  } else /* if (frame % forget_interval == 0) */ {
    if (bus.impulseMod)
      AcquireNearestLeader(range);
    else
      ClearAssign();
  }
}

void ShipClass::AssignNearestPlanet(short friendly) {
  static PingPacket nearPP;

  nearPP.initialize();
  if (friendly)
    nearPP = Ping(planet_target, nearest_friendly);

  /* in case nearest_friendly fails or friendly is off */
  if ( !IsValidObjectPointer(nearPP.obj.generic) )
    nearPP = Ping(planet_target, nearest);

  if ( IsValidObjectPointer(nearPP.obj.planet) )
    {
      assignPar2 = game->TypeListIndex(nearPP.obj.planet);
    }
}

void ShipClass::SendObjectTrackingMsg() {

  uchar ai1 = uchar( (assign << 4) + stat );
  uchar ai2 = uchar( duty );
  uchar ai3 = uchar( dutyPar );
  uchar ai4 = uchar( assignPar2 );
    
  ushort aiTargetIndex;

  if (stat == DesignedClass::orbiting)
    aiTargetIndex = ushort(game->ObjectListIndex(parent.generic));
  else if (IsValidObjectPointer(targetPP.obj.generic, __FILE__, __LINE__))
    aiTargetIndex=ushort(game->ObjectListIndex(targetPP.obj.generic));
  else
    aiTargetIndex = ushort(-1);
  
  uchar objOffset;
  if (bitmap.IsLoop())
    objOffset = heading;
  else
    objOffset = offset;
  
  retMsgBuffer.PushMsg((uchar)SF_MSG_AIOBJECTTRACK_17,
		       (ushort)game->ObjectListIndex(this),
		       (uchar)frameNumber % 256,
		       (uchar)objOffset,
		       (uchar)Index(),
		       (short)tvel.x,
		       (short)tvel.y,
		       (slong)now.x,
		       (slong)now.y,
		       (uchar)ai1,
		       (uchar)ai2,
		       (uchar)ai3,
		       (uchar)ai4,
		       (ushort)aiTargetIndex,
		       S_CLIENT_UDP);
}

void GenericClass::SendObjectTrackingMsg() {

  uchar objOffset;

  if (!bitmap.IsLoop())
    objOffset = offset;
  else
    objOffset = 0; /* not sure about this case */

  retMsgBuffer.PushMsg((uchar)SF_MSG_OBJECTTRACK_17,
		       (ushort)game->ObjectListIndex(this),
		       (uchar)frameNumber % 256,
		       (uchar)objOffset,
		       (uchar)Index(),
		       (short)tvel.x,
		       (short)tvel.y,
		       (slong)now.x,
		       (slong)now.y,
		       S_CLIENT_UDP);

}

void ShipClass::ReceiveObjectTrackingMsg(SF_Msg* gmsg, short setVel) {

  if (stat == under_construction && (gmsg->ai1 & 0x0F) != under_construction) {
    logFile << "WARNING> a ";
    ShortProfile();
    logFile << " has received a bad stat (" << (gmsg->ai1 & 0x0F) <<
      ") on frame " << game->frame << endl;
  }

  short nassign = (short)(gmsg->ai1 >> 4);
  if (nassign >= 0 && nassign < DesignedClass::last_assignment)
    assign = nassign;

  short nstat = (short)(gmsg->ai1 & 0x0F);
  if (nstat >= 0 && nstat < last_status)
    stat = nstat;

  /* *** Use bitmap index for ping time *** */
  SetShipLag(gmsg->slot,gmsg->bitmap);

  if (bitmap.IsLoop()) {
    heading = (uchar) gmsg->angle;
  } else if (bitmap.IsSpin() && bitmap.index == originalBitmapIndex) {
    heading = (uchar) gmsg->angle;
    offset = heading;
  }

  duty = (short)gmsg->ai2;
  dutyPar = gmsg->ai3;

  if (gmsg->ai4 >= 0 && gmsg->ai4 < game->numPlanets)
    assignPar2 = gmsg->ai4;
	
  if (gmsg->dslot >= 0 && gmsg->dslot < game->NumObjects()) {

    if (stat == orbiting) {
      /* validate the slot number as a planet */
      if (gmsg->dslot >= game->planetObjectIndexStart &&
	  gmsg->dslot <= game->planetObjectIndexEnd) {
	parent.generic = game->objectList[short(gmsg->dslot)];
	seekTurning = 0;
      }
    } else {

      /* any slot number is valid */
      targetPP.obj.generic = game->objectList[short(gmsg->dslot)];

      if ( IsValidObjectPointer(targetPP.obj.generic) &&
	   !targetPP.obj.generic->IsActive()) {
	logFile << "ERROR> OBJECTTRACK sent inactive slot number (" << gmsg->dslot <<
	  ") on frame " << game->frame << endl;
	targetPP.obj.generic = 0;
      }
    }
	  
  } else
    targetPP.obj.generic = 0;

  //logFile << " a ";
  //ShortProfile();
  //logFile << " is " << Query(this, 0, 1) << " on frame " <<
  //game->frame << endl;
  
  GenericClass::ReceiveObjectTrackingMsg(gmsg, !IsWarping());
  
}

void GenericClass::ReceiveObjectTrackingMsg(SF_Msg* gmsg, short setVel) {

  gfresh = TRUE;
  gframe = (short)gmsg->frame;
  
  if (!bitmap.IsTime()) {
    if ( Type() >= designed )
      ((DesignedClass *)this)->heading = (uchar) gmsg->angle;
    else
      offset = (uchar) gmsg->angle;
  } 
  
  /* *** Use bitmap index for ping time *** */
  SetShipLag(gmsg->slot,gmsg->bitmap);
  
  /* *** Only set velocity if ship is not warping *** */
  if (setVel) {
    gvel.x = gmsg->vx;
    gvel.y = gmsg->vy;
  }

  gnow.x  = gmsg->x;
  gnow.y  = gmsg->y;

}

void ShipClass::AcquireNearPoint(CoordinateClass c) {
  /* set offset position */
  static ShipClass s;

  slong dist = c.Mag();
  
  s.now.initializeWith(cX(1) + c.x, cY(1) + c.y);
  
  static PingPacket nearPP;
  
  if (dist < 1000 * sub_scale)
     nearPP = s.Ping(all_target);
  else
     nearPP = s.Ping(TargetType(ship_target + colony_target + planet_target));

  targetPP.obj.generic = nearPP.obj.generic;

  if ( IsValidObjectPointer(targetPP.obj.generic) )
    {
      if (targetPP.obj.generic->Type() == planet)
	assignPar2 = game->TypeListIndex(targetPP.obj.planet);
      else if (targetPP.obj.generic->Type() == colony && targetPP.obj.colony->lockObj.planet)
	assignPar2 = game->TypeListIndex(targetPP.obj.colony->lockObj.planet);
    }
}

void ShipClass::AcquireNext(TargetSelector t, short dir) {

  short i0;
  if (IsValidObjectPointer(targetPP.obj.generic, __FILE__, __LINE__))
    i0 = game->TypeListIndex(targetPP.obj.generic) + dir;
  else
    i0 = game->TypeListIndex(this) + 1;
  
  unsigned short i = i0 + game->numShips;
  
  unsigned short j = 0;
  unsigned short k;
  switch (t) {
    
  case sequential :
    while (j < game->numShips &&
	   !(game->ship[k=i%game->numShips].IsActive() &&
	     game->ship[k].CanBeSeenBy(viewObj.ship))) {
      i += dir;
      j++;
    }
    
    if (j != game->numShips) 
      targetPP.obj.generic = game->ship + (i % game->numShips);
    
    break;
  case sequential_friendly :
    while (j < game->numShips &&
	   !(game->ship[k=i%game->numShips].IsActive() &&
	     game->ship[k].IsFriendlyTo(this))) {
      i += dir;
      j++;
    }
    
    if (j != game->numShips) 
      targetPP.obj.generic = game->ship + (i % game->numShips);
    
    break;
  case sequential_enemy :
    while (j < game->numShips &&
	   !(game->ship[k=i%game->numShips].IsActive() &&
	     !game->ship[k].IsFriendlyTo(this) &&
	     game->ship[k].CanBeSeenBy(viewObj.ship))) {
      i += dir;
      j++;
    }
    
    if (j != game->numShips) 
      targetPP.obj.generic = game->ship + (i % game->numShips);
    
    break;
  case sequential_onscreen :
    while (j < game->numShips &&
	   !(game->ship[k=i%game->numShips].IsActive() &&
	     labs(game->ship[k].now.x - cX(1)) <
	     500*sub_scale &&
	     labs(game->ship[k].now.y - cY(1)) <
	     500 * sub_scale)) {
      i += dir;
      j++;
    }
    
    if (j != game->numShips) 
      targetPP.obj.generic = game->ship + (i % game->numShips);
    
    break;
    
  case nearest_enemy:
    {
      targetPP =
	Ping(TargetType(ship_target | colony_target), nearest_enemy, 0, 0);
    }
    break;
    
  case self:
    {
      targetPP.obj.ship = this;
      
      if ( IsValidObjectPointer(targetPP.obj.ship) && 
	   !targetPP.obj.ship->CanBeSeenBy(viewObj.ship))
	targetPP.obj.generic = 0;
    }
    break;
  default :
    break;
  }
}

void ShipClass::AssignNextPlanet(short dir, TargetSelector t, short all) {

  if (t == sequential) {
    do {
      assignPar2 = ( assignPar2 + dir + game->numPlanets) % game->numPlanets;
      if (assignPar2 == 0 && dir < 0 )
	assignPar2 = game->numPlanets-1; /* don't go to Sun */ 
      if (assignPar2 == 0 && dir > 0 )
	assignPar2 = 1; /* don't go to Sun */
    } while (game->planet[short(assignPar2)].minor && !all);
  } else if (t == sequential_friendly) {
    short i = 0;
    do {
      i++;
      assignPar2 = ( assignPar2 + dir + game->numPlanets) % game->numPlanets;
      if (assignPar2 == 0 && dir < 0 )
	assignPar2 = game->numPlanets-1; /* don't go to Sun */ 
      if (assignPar2 == 0 && dir > 0 )
	assignPar2 = 1; /* don't go to Sun */
    } while (i < game->numPlanets && 
	     !(game->planet[short(assignPar2)].colony && 
	       game->planet[short(assignPar2)].colony->IsFriendlyTo(this)) ); 
    if (i == game->numPlanets)
      assignPar2 = 0; /* default to the Sun if nothing else can be found */
  }
}

short ShipClass::AttackAdvisor() {
  short msg = standby;
  short i = 0;
  
  /* bound colonies cannot flee */
  if (Type() == colony && lockObj.generic != 0 || !AutoResponse())
    return stand;

  if (bus.aveFunc < ship_distress_func_level ||
      pop.InUnitsOf(1) < OptimalCrew().InUnitsOf(2))
    return flee;
  else
    while (i < bus.numMod && msg == standby) {
      if (bus.moduleList[i].moduleFunc == ModuleClass::tube)
	if (bus.moduleList[i].workPar[0])
	  msg = stand;
      i++;
    }
  return msg;
}

void ShipClass::GadgetAdvisor(PingPacket& pp, short& attackOverride,
			      short attackAdvice) {

  /* activate the stealth system under the following circumstances:    */
  /*    o  the target is a ship or colony                              */
  /*    o  the target is armed                                         */
  /*    o  the stealth system is at least 50% charged                  */
  /*                                                                   */

  if (bus.stealthMod && AutoResponse()) {

    /* if a missile is close to hitting this ship then turn off stealth system */
    /* so that shields can protect the ship                                    */
    if ( IsValidObjectPointer(avoidPP.obj.generic,__FILE__,__LINE__) && 
         avoidPP.obj.generic->Type() == missile &&
	avoidPP.dist < avoid_dist + (ulong) bitmap.size/2) {

      bus.stealthMod->deactivate();
      stealth = 1;

    } else if (pp.dist < (int)attack_dist  && pp.dist > (int)min_approach_dist ||
	       attackAdvice == flee) {
      if (!bus.stealthMod->IsActive() && 
	  bus.stealthMod->Storage() > 50 &&
	  IsValidObjectPointer(pp.obj.generic,__FILE__,__LINE__) &&
	  pp.obj.generic->Type() >= ship &&
	  pp.obj.ship->bus.numTubes)
	bus.stealthMod->activate();
    } else {
      bus.stealthMod->deactivate();
      stealth = 1;
    }
  }
  if (bus.scrambleMod && bus.scrambleMod->IsCharged() && numPings) {
    bus.scrambleMod->activate();
  }

  if (bus.boostMod &&
      IsValidObjectPointer(targetPP.obj.generic, __FILE__, __LINE__)) {
    if(pp.dist < (int)attack_dist && pp.dist > (int)min_approach_dist) {
      if (!bus.boostMod->IsActive() && 
	  bus.boostMod->Storage() > 50)
	bus.boostMod->activate();
    } else
      bus.boostMod->deactivate();
  }


  /* attempt to board a disabled ship */
  if (!dockObj && 
      bus.numPorts &&
      stealth == 1 &&
      !IsWarping() &&
      IsValidObjectPointer(targetPP.obj.generic, __FILE__, __LINE__) &&
      targetPP.obj.generic->Type() == ship &&
      targetPP.obj.designed->bus.impulseMod &&
      targetPP.obj.designed->bus.impulseMod->func < module_break_limit &&
      !targetPP.obj.generic->IsExploding() &&
      !targetPP.obj.ship->IsWarping() &&
      targetPP.obj.ship->stealth == 1 &&
      !targetPP.obj.ship->dockObj &&
      !targetPP.obj.ship->IsPending(captured) &&
      targetPP.obj.ship->pop < pop ) {
    short i = 0;
    short noDockFound = 1;
    while( i < bus.numPorts && noDockFound) {

      if (!bus.dockMod[i]->workPar[1] && bus.dockMod[i]->Storage() > 50) {
	noDockFound = 0;
	if (pp.dist < (ulong)bus.dockMod[i]->par[2] * usub_scale) {

	  //*********************************************************************************
	  // Note: the following code governs when a dock will be deactivated in            *
	  //       ModuleClass::Dock.  In order to avoid a feedback loop of messages that   *
	  //       clog the message queue, the activation conditions should match these de- *
	  //       activation conditions.  There was no condition for activating the dock   *
	  //       based on the energy level, so GadgetAdvisor was activating the dock and  *
	  //       ModuleClass::Dock was deactivating just as quickly and the two fought    *
	  //       over the active state indefinitely and the server slowed down.           *
	  //                                                                                *
          // ShipClass* s = (ShipClass*)workPar[0];                                         *
          // if (!surplus || !s || s && (s->IsExploding() ||                                *
          //                             s->IsWarping() ||                                  *
          //                             s->stealth > 1 ||                                  *
          //                             s->dockObj ||                                      *
          //                             (s->Type() == colony &&                            *
          //                              !s->IsFriendlyTo(host.ship))) ||                  *
	  //                             host.ship->stealth > 1 ||                          *
          //                             host.designed->IsWarping())) {                     *
          //   if (workPar[1])                                                              *
          //     s->FreeFromDock();                                                         *
          //   else {                                                                       *
          //     workPar[0] = 0;                                                            *
          //     deactivate();                                                              *
          //   }                                                                            *
          //                                                                                *
          //   /* the docking system has powered down -- send a DockDeactivate message */   *
          //   if (host.generic->Type() == ship &&                                          *
          //       ((IsConnectedClient() && IsLocalPlayer(host.ship)) ||                    *
          //        (IsServer() && game->IsRobotPlayer(host.ship))))                        *
          //     SendDockDeactivationMsg();                                                 *
          //                                                                                *
          // }                                                                              *
	  //
          //*********************************************************************************

	  //bus.dockMod[i]->activate();
	  bus.dockMod[i]->workPar[0] = targetPP.obj.objLong;
	  /* dock activation */
	  bus.dockMod[i]->SendDockActivationMsg();
	  attackOverride = 1;

	} else if (bus.dockMod[i]->IsActive())
	  bus.dockMod[i]->SendDockDeactivationMsg("the advisor shut it down");
      }
      i++;
    }
  }
}

short ShipClass::CanBeSeenBy(DesignedClass* d) {

  if ( !IsValidObjectPointer(d) )
    {
      return 0;
    }

  if (d == this || !d) {
    /* object is looking at itself */
    if (stealth > 1)
      return 0;
    else
      return vis;
  } else if (IsExploding() || IsTransitioning())
    return 1;
  else if (IsWarping())
    return 0;
  else if (d->raceID == raceID)
    return 1;
  else {

    return d->CanSee(this, game->lightSpeed);

  }
}

void ShipClass::WageWar(ShipClass* s) {

  slong sPct = game->Random(100);
  slong tPct = game->Random(100, sPct);

  static PopStruct dPop;
  static PopStruct t;

  t = s->pop;

  /* apply damage to s population */
  if (s->pop.billions)
    dPop = pop / 100 * tPct;
  else
    dPop = pop * tPct / 100;
  if (pop < dPop) {
    /* we win */
    if (s->Type() == colony)
      game->SendDestroyObjectMsg((ColonyClass*)s);
  }

  /* apply damage to this population */
  if (pop.billions)
    dPop = t / 100 * tPct;
  else
    dPop = t * tPct / 100;
  if (pop < dPop) {
    /* the other guy wins */
    if (s->Type() == colony)
      game->SendDestroyObjectMsg((ColonyClass*)this);
  }
  
}
  
void ShipClass::CalculateRemainingValueAndMass(slong& v, slong& m) {
  short i;
  v=0;
  m=0;
  for (i=bus.numMod; i<design->bus.numMod; i++) {
    ModuleClass* mod = 
      game->moduleList + game->LookUpModuleById(design->bus.moduleIdList[i],
						&logFile);
    if ( mod )
      {
	v += mod->value();
	m += mod->mass;
      }
  }
}
  
short ShipClass::SendDistressSignal(short priority, short objType) {
  short numColonies = 0;
  short numShips = 0;

  /************************************************************/
  /* priority: determines the level of response among ships   */
  /*    0: will summon ships that are available at 0 priority */
  /*   >0  will summon all ships within range regardless of   */
  /*       priority                                           */
  /************************************************************/

  /* if no target is assigned then select based on last attacker */
  if (!targetPP.obj.ship) {
    if (!lastAttacker.ship)
      return 0;
    else
      targetPP.obj = lastAttacker;
  }

  if ( !IsValidObjectPointer(targetPP.obj.ship) )
    {
      targetPP.obj.ship = 0;
      return 0;
    }

  short i;
  for (i=0; i < game->numShips; i++) {

    /* the ship will respond if it is available or the requesting */
    /* ship is in range                                           */
    if (game->ship[i].IsAvailableToHelp() &&
	game->ship[i].IsFriendlyTo(this) &&
	game->ship + i != this) {
      
      /* if the ship has already responded then increment the counter */
      if ((game->ship[i].assign == follow_object &&
	  game->ship[i].targetPP.obj.generic == this) ||
	  (game->ship[i].assign == attack &&
	  game->ship[i].targetPP.obj.generic == targetPP.obj.generic))
	numShips++;
      else if (game->ship[i].AutoResponse() &&
	       (game->ship[i].WillHelp(this, priority) ||
		(game->ship[i] - *this).Mag(0) < (int)attack_dist)) {
	
	if ( targetPP.obj.ship->CanBeSeenBy(game->ship+i) ) {
	  
	  game->ship[i].AttackObj(targetPP.obj.generic, priority);
	  if (game->ship[i].CanConfirmHelp(*this))
	    numShips++;

	} else if (game->ship[i].bus.impulseMod) {
	  
	  /* if the target cannot be seen then follow the object in distress */
	  game->ship[i].FollowObj(this, priority);
	  if (game->ship[i].CanConfirmHelp(*this))
	    numShips++;

	  /* logFile << "SendDistressSignal: ship " << i <<
	     " received a priority " << priority << " distress call from " << 
	     TypeName() << " " <<
	     game->TypeListIndex(this) << endl;*/
	  
	}
      }
    }
  }

  for (i=0; i < game->numColonies; i++) {
    if ( IsValidObjectPointer(game->colonyList[i]) ) {
      if (game->colonyList[i]->IsActive() &&
          game->colonyList[i]->IsFriendlyTo(this) &&
	  game->colonyList[i] != this) {
      
        /* if the colony has already responded then increment the counter */
        if ((game->colonyList[i]->assign == follow_object &&
	     game->colonyList[i]->targetPP.obj.generic == this) ||
	    (game->colonyList[i]->assign == attack &&
	     game->colonyList[i]->targetPP.obj.generic == targetPP.obj.generic &&
	     game->colonyList[i]->targetPP.dist < (int)min_attack_dist))
          numColonies++;
        else if ((game->colonyList[i]->WillHelp(this, priority) ||
		  (*game->colonyList[i]-*this).Mag(0) < (int)attack_dist) &&
	         targetPP.obj.ship->CanBeSeenBy(game->colonyList[i])) {
	
	  game->colonyList[i]->AttackObj(targetPP.obj.generic);
	  numColonies++;
	
        }
      }
    } else {
    logFile << "ShipClass::SendDistressSignal colonyList[" 
            << i << "] = NULL" << endl;
    }
  }
  
  /* return number responding to call */
  short n = 0;
  if (objType & ship)
    n += numShips;
  if (objType & colony)
    n += numColonies;

  if (!IsServer() &&
      this == game->ship + localSlotNumber) {
    game->ship[localSlotNumber].PlaySound(game->soundEvent[distress_call_event]);
  }
  
  if (n)
    distressFrame = game->frame;

  return n;
}

slong DesignStruct::PushTo(class GameStateBuffer& gb, GameStateClass* g) {

  char* bufferPos = gb.BufferPosition();

  CALL_PUSH_OBJ(gb,id);
  CALL_PUSH_OBJ(gb,raceID);
  CALL_PUSH_OBJ(gb,mass);
  CALL_PUSH_OBJ(gb,maxSpeed);
  CALL_PUSH_OBJ(gb,bitmapIndex);
  CALL_PUSH_OBJ(gb,expBitmapIndex);
  CALL_PUSH_OBJ(gb,expSoundIndex);
  CALL_PUSH_OBJ(gb,volumeCap);
  CALL_PUSH_GAME_OBJ(gb, g, bus);
  CALL_PUSH_OBJ(gb,name);
  
  return gb.BufferPosition() - bufferPos;

}

slong DesignStruct::PopFrom(class GameStateBuffer& gb, GameStateClass* g) {
  char* bufferPos = gb.BufferPosition();

  CALL_POP_OBJ(gb,id);
  CALL_POP_OBJ(gb,raceID);
  CALL_POP_OBJ(gb,mass);
  CALL_POP_OBJ(gb,maxSpeed);
  CALL_POP_OBJ(gb,bitmapIndex);
  CALL_POP_OBJ(gb,expBitmapIndex);
  CALL_POP_OBJ(gb,expSoundIndex);
  CALL_POP_OBJ(gb,volumeCap);
  CALL_POP_GAME_OBJ(gb, g,bus);
  CALL_POP_OBJ(gb,name);

  return gb.BufferPosition() - bufferPos;
}

slong MissileDesignStruct::PushTo(class GameStateBuffer& gb, GameStateClass* g) {

  slong subClassSize = DesignStruct::PushTo(gb, g);

  char* bufferPos = gb.BufferPosition();

  CALL_PUSH_OBJ(gb,warhead);
  CALL_PUSH_OBJ(gb,trigger);
  CALL_PUSH_OBJ(gb,tracking);
  CALL_PUSH_OBJ(gb,initialVelocityDisplacement);
  CALL_PUSH_OBJ(gb,seekSens);
  CALL_PUSH_OBJ(gb,push);
  CALL_PUSH_OBJ(gb,vortex);
  CALL_PUSH_OBJ(gb,camera);
  CALL_PUSH_OBJ(gb,actRange);

  return subClassSize + (gb.BufferPosition() - bufferPos);

}

slong MissileDesignStruct::PopFrom(class GameStateBuffer& gb, GameStateClass* g) {

  slong subClassSize = DesignStruct::PopFrom(gb, g);

  char* bufferPos = gb.BufferPosition();

  CALL_POP_OBJ(gb,warhead);
  CALL_POP_OBJ(gb,trigger);
  CALL_POP_OBJ(gb,tracking);
  CALL_POP_OBJ(gb,initialVelocityDisplacement);
  CALL_POP_OBJ(gb,seekSens);
  CALL_POP_OBJ(gb, push);
  CALL_POP_OBJ(gb, vortex);
  CALL_POP_OBJ(gb,camera);
  CALL_POP_OBJ(gb,actRange);

  return subClassSize + (gb.BufferPosition() - bufferPos);
}

short DesignStruct::get(ifstream& f, ObjectType ot,
		       SFString* nameList, SFString* valueList,
		       short nList) {
  char busString[1024];
  double mS;
  slong nS, lVolumeCap;
  char c;
  char sBitmapIndex[max_name_char], sExpBitmapIndex[max_name_char], 
    sExpSoundIndex[max_name_char], sRaceID[max_name_char], sID[max_name_char];

  f >> c;
  if (c != '}') {
    f.putback(c);
    f >> mS >> sBitmapIndex >> sExpBitmapIndex >> sExpSoundIndex >> sID >>
      sRaceID >> lVolumeCap >> nS;

    if (!strcmp(sBitmapIndex, "*"))
      strcpy(sBitmapIndex, name);

    if (nameList && valueList && nList) {
      
      Parse(sRaceID, nameList, valueList, nList);
      Parse(sBitmapIndex, nameList, valueList, nList);
      Parse(sExpBitmapIndex, nameList, valueList, nList);
      Parse(sExpSoundIndex, nameList, valueList, nList);
      Parse(sID, nameList, valueList, nList);
      
    }

    id = atol(sID);

    raceID = game->IndexOfRace(sRaceID);
    bitmapIndex = game->IndexOfBitmap(sBitmapIndex, ot, raceID);

    if (lVolumeCap > pc_short_max)
      {
	logFile << "WARNING> volume capacity must be less than " << pc_short_max <<
	  " -- the value has been corrected." << endl;
	SetStat(volumeCap,pc_short_max);
	//volumeCap = pc_short_max;
      }
    else
      {
	SetStat(volumeCap,(short)lVolumeCap);
	//volumeCap = short(lVolumeCap);
      }
    
    if (!f) {
      logFile << "ERROR> DesignStruct::get: input error -- partial line reads:" <<
	endl;
      logFile << mS << " " << bitmapIndex << " " << expBitmapIndex << " " <<
	id << " " << raceID << " " << volumeCap << " " << nS << endl;
    }


    bus.numSlots = char(nS);
    maxSpeed = nlong(mS * game->lightSpeed);

    expBitmapIndex = game->IndexOfBitmap(sExpBitmapIndex, explosion);
    expSoundIndex = game->IndexOfSound(sExpSoundIndex, ot, raceID);

    char* del = " ,";
    char* tok;
    /* break list into tokens */
    f.getline(busString, 1024);

    if (nameList && valueList && nList) {
      
      Parse(busString, nameList, valueList, nList);
      
    }

    char* tline = busString;
    while (*tline == ' ') tline++;
    if (*tline == '(') {
      strcpy(tline, tline+1);
      tline[strlen(tline)-1] = '\0';
    }
    short nTokens = 0, l;
    char* parPtr;
    char* pch;
    while ((tok = strtok(tline, del)) != 0 && nTokens < bus.numSlots) {
      parPtr = tok;
      l = 0;
      while ((parPtr = strchr(parPtr, '*')) != NULL) {
        parPtr++;
	pch = strchr(parPtr,'*');
        if ( pch )
	  *pch = 0;
        bus.moduleParList[nTokens*max_module_spar+l] = atol(parPtr);
        if ( pch )
	  *pch = '*';
	l++;
      }

      parPtr = strchr(tok, '*');
      if (parPtr)
	*parPtr = 0;
      bus.moduleIdList[nTokens] = game->IDOfModule(tok, raceID);

      nTokens++;
      tline = NULL;
    }
    bus.numMod = nTokens;
  }

  return (c != '}' && f.good());
}

short ShipDesignStruct::get(ifstream& f, ObjectType ot,
		       SFString* nameList, SFString* valueList,
		       short nList) {
  char Class[max_name_char];
  char c;

  f >> c;
  if (c != '}') {
    f.putback(c);

    f >> Class >> name;

    if (nameList && valueList && nList) {
      
      Parse(Class, nameList, valueList, nList);
      Parse(name, nameList, valueList, nList);

    }

    switch (ot) {

    case ship:
      
      f >> inFrame >> reqRank;
      
      /* the inFrame parameter is the frame number on which the design becomes */
      /* available for use.  In the game state file is stored in units of      */
      /* minutes, so convert it */
      inFrame = inFrame * 60 * frameRate;
      
      if (!f) {
	logFile << "ERROR> ShipDesignStruct::get: input error -- partial line reads:" <<
	  endl;
	logFile << Class << " " << name << " " << inFrame << " " << reqRank <<
	  endl;
      }

      break;

    case colony:

      if (!f) {
	logFile << "ERROR> ColonyDesignStruct::get: input error -- partial line reads:" <<
	  endl;
	logFile << Class << " " << name << endl;
      }      

      break;

    default :
      break;
    }

    /* get the rest of it -- this is shared with ColonyDesignStruct */
    DesignStruct::get(f, ot, nameList, valueList, nList);

  }

  return (c != '}' && f.good());
}

short MissileDesignStruct::get(ifstream& f,
			       SFString* nameList, SFString* valueList,
			       short nList) {
  char Class[max_name_char];
  char tra[max_name_char];
  char tri[max_name_char];
  slong ss;
  double ivd;
  char c;
  char cam;

  f >> c;
  if (c != '}') {
    f.putback(c);
    f >> Class >> name >> warhead >> ivd >> tra >> 
      tri >> ss >> actRange >> push >> vortex >> cam;

    if (nameList && valueList && nList) {
      
      Parse(Class, nameList, valueList, nList);
      Parse(name, nameList, valueList, nList);

    }

    if (cam == 'y' || cam == 'Y' || cam == '1') 
      camera = 1;
    else
      camera = 0;

    initialVelocityDisplacement = nlong(1.000001 * ivd * game->lightSpeed / sub_scale);

    DesignStruct::get(f, missile, nameList, valueList, nList);

    if (strcmp(tri,"ship") == 0) trigger = on_ship;
    else if (strcmp(tri,"impact") == 0) trigger = on_ship + on_planet;
    else if (strcmp(tri,"heat") == 0) trigger = on_heat;
    else if (strcmp(tri,"missile") == 0) trigger = on_missile;
    else if (strcmp(tri,"planet") == 0) trigger = on_planet;
    else if (strcmp(tri,"prox") == 0) trigger = on_proximity;
    else if (strcmp(tri,"time") == 0) trigger = on_time;
    else if (strcmp(tri,"none") == 0) trigger = 0;

    if (strcmp(tra,"dumb") == 0) tracking = dumb;
    else if (strcmp(tra,"bomb") == 0) tracking = bomb;
    else if (strcmp(tra,"smart") == 0) tracking = smart;

    seekSens = char(ss);



    actRange *= sub_scale;
  }
  return (c != '}' && f.good());
}

void MissileClass::initializeWith(short i, DesignedClass& g, MissileDesignStruct& md,
				  slong* par, slong altModID) {
  
  /* primary missile constructor */

  DesignedClass::initializeWith(g, game->objectBitmapList[md.bitmapIndex], 
				md.initialVelocityDisplacement, par);
  
  typeOfObject = missile;
  
  /* register parent object */
  parent.generic = &g;
  design = &md;
  maxSpeed = design->maxSpeed;
  raceID = design->raceID;
  bus.InitializeWith((short)design->bus.numSlots);

  if (bus.moduleList.rootOf() == 0)
    /* unable to allocate memory for new missile so return early */
    return;

  bus.numMod = (short)design->bus.numMod;

  duty = none;

  if (altModID) { /* this is a debris object */
    duty = plugin;
    triggerObjQualifier = nearest;
    short modIndex = game->ValidateModuleID(altModID);
    if (modIndex != -1)
      bitmap =
	game->objectBitmapList[game->moduleList[modIndex].debrisBitmapIndex];
    else {
      logFile << "ERROR> MissileClass:: uknown module id (" << 
	altModID << ")" << endl;
    }
    PlugInModuleByDesign(0); /* plug in the power supply */
    PlugInModuleByID(1, altModID); /* plug in the alternate module */
    bus.numMod = 2;

    /* randomize the initial angle */
    if (bitmap.numFrames) {
      offset = (char)(altModID % bitmap.numFrames);
      heading = offset;
      frame = offset; /* to ensure that MissileClass::Update does not reset it */
    }

  } else { /* standard missile */

    PlugInModules(); /* plug in all modules */

    triggerObjQualifier = nearest_enemy;

  }

  
  bus.MapFunctions(game->missile + i, this);

  lastBitmap = NoObjectBitmap;

  targetLock = 0;

  /* determine conditions for detonation */
  triggerObjType = 0;
  
  onImpact = 1;

  if (design->trigger & on_ship)
    triggerObjType += ship_target + colony_target;
  if (design->trigger & on_planet)
    triggerObjType += planet_target;
  if (design->trigger & on_missile)
    triggerObjType += missile_target;
  if (design->trigger & on_heat) {
    triggerObjType += ship_target + colony_target;
    triggerObjQualifier = nearest_thrusting_enemy;
  } else if (design->trigger & on_proximity) {
    triggerObjType += ship_target + colony_target;
    onImpact = 0;
  }

  CalculateMass();
  
}

MissileClass::MissileClass() : DesignedClass() {
  /* default missile constructor */
  //parentLock = 0;

  typeOfObject = missile;

  targetLock = 0;
  triggerObjType = 0;
  triggerObjQualifier = 0;
  onImpact = 0;
}

PingPacket& MissileClass::Ping(TargetType tt,
			       TargetSelector ts,
			       short n,
			       ulong range,
			       CoordinateClass lead) {
  /* ping function for missiles -- checks only ships */

  short i;
  static PingPacket minPP;
  static PingPacket thisPP;
  minPP.obj.generic = 0;
  if (ts == nearest || ts == all) {
    if (tt == target_target) {
      if ( IsValidObjectPointer(targetPP.obj.generic) )
	{
	  minPP = PingPacket(this, targetPP.obj.generic, 0, lead);
	}
      return minPP;
    } else if (tt & ship_target) {
      for (i = 0; i < game->N(GameStateClass::ships); i++) {
	if (game->ship[i].IsAvailable() && game->ship + i != parent.generic &&
	    !game->ship[i].IsExploding())
	  thisPP = minPP.Sniff(this, &game->ship[i], range);
      }
    }
    if (tt & planet_target) {
      for (i = 0; i < game->N(GameStateClass::planets); i++) {
	if (!game->planet[i].IsFriendlyTo(this))
	  thisPP = minPP.Sniff(this, &game->planet[i], range);
      }
    }
  } else if (ts == nearest_enemy) {
    if (tt & ship_target) {
      for (i = 0; i < game->N(GameStateClass::ships); i++) {
	if (game->ship[i].IsAvailable() &&
	    !game->ship[i].IsFriendlyTo(this) &&
	    !game->ship[i].IsExploding() &&
	    !game->ship[i].IsWarping(0))
	  thisPP = minPP.Sniff(this, &game->ship[i], range);
      }
    }
    if (tt & planet_target) {
      for (i = 0; i < game->N(GameStateClass::planets); i++) {
	/* dumb missiles should check all planets because they should impact */
	/* on any planet                                                     */
	/* short home = parent.generic &&
	   parent.generic->Type() == colony &&
	   game->planet+i == parent.designed->lockObj.planet;*/
	if (/*!home &&*/ !game->planet[i].IsFriendlyTo(this))
	  thisPP = minPP.Sniff(this, &game->planet[i], /* range */ max_grav_dist);
	if (thisPP.dist < (int)max_grav_dist && 
	    design->tracking == bomb) {
	  /* if within max_grav_dist feel gravity */
	  Attract(thisPP, 0, 1);
	}
      }
    }
    if (tt & colony_target) {
      for (i = 0; i < game->N(GameStateClass::colonies); i++) {
        if ( IsValidObjectPointer(game->colonyList[i]) ) {
	  if (game->colonyList[i]->IsActive() &&
	      !game->colonyList[i]->IsFriendlyTo(this) &&
	      !game->colonyList[i]->IsExploding() &&
	      !game->colonyList[i]->IsWarping(0))
	    thisPP = minPP.Sniff(this, game->colonyList[i], range);
	} else {
	  logFile << "ERROR> MissileClass::Ping colonyList[" << i
		  << "] = NULL" << endl;
	}
      }
    }
    if (tt & missile_target) {

      /**************************************************************************/
      /* if the parent object exists and has a missile target but the missile   */
      /* itself does not have missile_target in triggerObjType, then only sniff */
      /* that object.  Otherwise, allow a scan of the entire missile list       */
      /**************************************************************************/

      if ( IsValidObjectPointer(parent.generic,__FILE__,__LINE__) && 
           parent.generic->Type() >= designed && 
	  /* !(triggerObjType & missile_target) && */
	  IsValidObjectPointer(parent.designed->targetPP.obj.generic, __FILE__, __LINE__) &&
	  parent.designed->targetPP.obj.generic->Type() == missile)
	thisPP = minPP.Sniff(this, parent.designed->targetPP.obj.missile, range);
      else
	for (i = 0; i < game->N(GameStateClass::missiles); i++) {
	  
	  if (IsValidObjectPointer(game->missileList[i]) &&
	      game->missileList[i]->IsActive() &&
	      !game->missileList[i]->IsFriendlyTo(this) &&
	      !game->missileList[i]->IsExploding())
	    thisPP = minPP.Sniff(this, game->missileList[i], range);
	}

    }
  } else if (ts == nearest_thrusting_enemy) {
    if (tt & ship_target) {
      for (i = 0; i < game->N(GameStateClass::ships); i++) {
	if (game->ship[i].IsAvailable() &&
	    game->ship[i].throttle &&
	    !game->ship[i].IsFriendlyTo(this) &&
	    !game->ship[i].IsExploding())
	  thisPP = minPP.Sniff(this, &game->ship[i], range);
      }
    }
    if (tt & colony_target) {
      for (i = 0; i < game->N(GameStateClass::colonies); i++) {
        if ( IsValidObjectPointer(game->colonyList[i]) ) {
	  if (game->colonyList[i]->IsActive() &&
	      game->colonyList[i]->throttle &&
	      !game->colonyList[i]->lockObj.generic &&
	      !game->colonyList[i]->IsFriendlyTo(this) &&
	      !game->colonyList[i]->IsExploding())
	    thisPP = minPP.Sniff(this, game->colonyList[i], range);
	} else {
	  logFile << "ERROR> MissileClass::Ping colonyList[" << i
		  << "] = NULL" << endl;
	}
      }
    }
  } else {
    switch (tt) {
    case planet_target:
      minPP = PingPacket(this,&game->planet[n], 0, lead);
      game->planet[n].numPings++;
      break;
    case ship_target:
      if (game->ship[n].IsAvailable()) {
	minPP = PingPacket(this,&game->ship[n], 0, lead);
	game->ship[n].numPings++;
      }
      break;
    case free_colony_target:
    case colony_target:
      if (game->colony[n].IsActive()) {
	minPP = PingPacket(this,&game->colony[n], 0, lead);
	game->colony[n].numPings++;
      }
      break;
    case no_target:
    default:
      break;
    }
  }
  return minPP;
}

short MissileClass::CanBeSeenBy(DesignedClass* d) {
  slong val;
  
  if ( !IsValidObjectPointer(d) )
    {
      return 0;
    }
  
  if ( !IsValidDesignStructPointer(design,__FILE__,__LINE__,0) )
    {
      return 0;
    }
  
  if (design->tracking == dumb || design->tracking == bomb || IsExploding())
    return 1;
  else {
    if (d->bus.sensorMod) 
      val = min_sensor_value + d->bus.sensorMod->fPar(0);
    else
      val = min_sensor_value;
    slong sense = nlong(val * mass * (1 + double(tvel.Mag()) /
				     game->lightSpeed) /
		       ((*d-*this).Mag() / sensor_dist + 1));
    return sense > sensorThreshold * stealth;
  }
}

void MissileClass::Explode(char* reason, short spawn, short direct) {
  bitmapStartFrame = frame;
  if (bitmap.index != design->expBitmapIndex)
    bitmap = game->objectBitmapList[design->expBitmapIndex];
  offset = 0;
  assign = manual;
  stat = idle;

  if (design->trigger == on_proximity || design->tracking == bomb) {
    /* continue blasting until bitmap terminates */
    game->BlastObjects(this, 2*design->actRange, design->warhead, design->push, design->vortex);
  } else if (design->trigger == on_time) {
    /* continue blasting until bitmap terminates */
    game->BlastObjects(this, design->actRange / 10 * sub_scale, design->warhead, design->push, design->vortex);
  } 

  if (duty != plugin)
    PlaySoundOnScreen(design->expSoundIndex );
}

slong MissileClass::PushTo(class GameStateBuffer& gb, GameStateClass* g) {

  WRITE_SF_LOG(logFile << "Pushing missile section of " << TypeName() << " " <<
	       g->TypeListIndex(this) << "..." << endl);

  slong subClassSize = DesignedClass::PushTo(gb,g);

  char* bufferPos = gb.BufferPosition();
  
  short designIndex = g->missileDesign.indexOf(design);
  CALL_PUSH_OBJ(gb,designIndex);
  CALL_PUSH_OBJ(gb,targetLock);
  CALL_PUSH_OBJ(gb,onImpact);
  CALL_PUSH_OBJ(gb,triggerObjQualifier);
  CALL_PUSH_OBJ(gb,triggerObjType);
  CALL_PUSH_GAME_OBJ(gb, g,parent.generic);

  return subClassSize + (gb.BufferPosition() - bufferPos);

}

slong MissileClass::PopFrom(class GameStateBuffer& gb, GameStateClass* g) {

  WRITE_SF_LOG(logFile << "Popping missile section of " << TypeName() << " " <<
	       g->TypeListIndex(this) << "..." << endl);

  slong subClassSize = DesignedClass::PopFrom(gb,g);

  char* bufferPos = gb.BufferPosition();

  short designIndex;
  CALL_POP_OBJ(gb,designIndex);

  design = g->missileDesign + designIndex;

  CALL_POP_OBJ(gb,targetLock);
  CALL_POP_OBJ(gb,onImpact);
  CALL_POP_OBJ(gb,triggerObjQualifier);
  CALL_POP_OBJ(gb,triggerObjType);
  CALL_POP_GAME_OBJ(gb, g,parent.generic);

  return subClassSize + (gb.BufferPosition() - bufferPos);
}

void MissileClass::Acquire(ulong range) {

  static PingPacket nearPP;

  targetLock = 0;
  targetPP.obj.generic = 0;
  if (design->tracking == smart) {
    if ( IsValidObjectPointer(parent.generic,__FILE__,__LINE__) && 
         parent.designed->assign == attack && 
	IsValidObjectPointer(parent.designed->targetPP.obj.generic, __FILE__, __LINE__) &&
	parent.designed->targetPP.obj.generic->CanBeSeenBy(this) /* &&
	parent.designed->target.generic->Type() == ship*/) {
      targetPP = parent.designed->targetPP;
    } else {
      nearPP = Ping(TargetType(ship_target | colony_target), nearest_enemy);
      if ( IsValidObjectPointer(nearPP.obj.generic,__FILE__,__LINE__) && 
           nearPP.obj.generic->CanBeSeenBy(this)) {
	targetPP = nearPP;
      }
    }
  }
}

void GameStateClass::DetonateMissilesOf(ShipClass* s) {
  short i;

  if ( !IsValidObjectPointer(s) )
    {
      return;
    }

  for (i = 0; i < numMissiles; i++)
    {
      if ( IsValidObjectPointer(missileList[i]) )
	{
	  if ( (missileList[i]->parent.ship == s || 
		!missileList[i]->parent.ship) &&
	       missileList[i]->raceID == s->raceID && 
	       !missileList[i]->IsExploding())
	    missileList[i]->Explode("it was detonated");
	}
    }
  
  /*****************************************/
  /* deactivate all tubes -                */
  /* this is mostly for the beams, so that */
  /* a player has a way of shutting them   */
  /* off.                                  */
  /*****************************************/
  for (i=0; i < s->bus.numTubes; i++)
    s->bus.tubeMod[i]->deactivate();

  s->tubeSelector = 0;
  s->defensiveTubeSelector = 0;
  s->firingTubes = 0;

}

void GameStateClass::CleanOrphanObjects(ShipClass* d) {
  short i;

  if ( !IsValidObjectPointer(d) )
    {
      return;
    }

  if (race[d->raceID].distressObj.ship == d)
    race[d->raceID].distressObj.ship = 0;
  
  for (i = 0; i < numMissiles; i++)
    if ( IsValidObjectPointer(missileList[i]) )
      if (missileList[i]->parent.designed == d)
	missileList[i]->parent.generic = 0;
  
  for (i = 0; i < numShips; i++) {
    if (ship[i].targetPP.obj.ship == d)
      ship[i].targetPP.obj.objLong = 0;

    /* if a factory module was building this object disable it */
    if (ship[i].bus.factoryMod && 
	ship[i].bus.factoryMod->workPar[0] == slong(d)) {
      ship[i].bus.factoryMod->workPar[0] = 0;
      ship[i].bus.factoryMod->deactivate();
    }
  }
  for (i = 0; i < numColonies; i++) {
    if ( IsValidObjectPointer(colonyList[i]) ) {
      if (colonyList[i]->targetPP.obj.ship == d)
	colonyList[i]->targetPP.obj.objLong = 0;
      /* if a factory module was building this object disable it */
      if (colonyList[i]->bus.factoryMod && 
          colonyList[i]->bus.factoryMod->workPar[0] == slong(d)) {
	colonyList[i]->bus.factoryMod->workPar[0] = 0;
	colonyList[i]->bus.factoryMod->deactivate();
      }
    } else {
      logFile << "ERROR> GameStateClass::DetonateMissilesOf colonyList[" << i
	      << "] = NULL" << endl;
    }
  }

  /* if this ship was building something then explode it */
  if (d->bus.factoryMod && IsValidObjectPointer((ShipClass*)d->bus.factoryMod->workPar[0]))
    ((ShipClass*)d->bus.factoryMod->workPar[0])->
      Explode("the building ship was destroyed");

}

void GameStateClass::SendStealthCommandToFleet(GenericClass* s) {
  short i;
  short is = TypeListIndex(s);
  for (i=0; i < is; i++) {
    if (ship[i].assign == DesignedClass::follow_object &&
	ship[i].targetPP.obj.ship == s) {
      if (ship[i].stealth == 1)
	ship[i].Command(BusStruct::fire_cloak);
      else
	ship[i].Command(BusStruct::release_cloak);
      ship[i].bus.ProcessMessage(ship + i);
    }
  }
  for (i=is+1; i < numShips; i++) {
    if (ship[i].assign == DesignedClass::follow_object &&
	ship[i].targetPP.obj.ship == s) {
      if (ship[i].stealth == 1)
	ship[i].Command(BusStruct::fire_cloak);
      else
	ship[i].Command(BusStruct::release_cloak);
      ship[i].bus.ProcessMessage(ship + i);
    }
  }
}

void GameStateClass::SendCommandToFleet(GenericClass* s, short m) {
  short i;
  short is = TypeListIndex(s);
  for (i=0; i < is; i++) {
    if (ship[i].assign == DesignedClass::follow_object &&
	ship[i].targetPP.obj.ship == s) {
      ship[i].Command(m);
      ship[i].bus.ProcessMessage(ship + i);
    }
  }
  for (i=is+1; i < numShips; i++) {
    if (ship[i].assign == DesignedClass::follow_object &&
	ship[i].targetPP.obj.ship == s) {
      ship[i].Command(m);
      ship[i].bus.ProcessMessage(ship + i);
    }
  }
}

void GameStateClass::SendCommandToFleet(GenericClass* s,
					short m,
					CoordinateClass arg) {
  short i;
  short is = TypeListIndex(s);
  for (i=0; i < is; i++) {
    if (ship[i].assign == DesignedClass::follow_object &&
	ship[i].targetPP.obj.ship == s)
      ship[i].Command(m, arg-(ship[i].now-s->now), nullVector,1);
  }
  for (i=is+1; i < numShips; i++) {
    if (ship[i].assign == DesignedClass::follow_object &&
	ship[i].targetPP.obj.ship == s)
      ship[i].Command(m, arg-(ship[i].now-s->now), nullVector, 1);
  }
}

void GameStateClass::SendAssignToFleet(GenericClass* s,
				       DesignedClass::assignment a, 
				       slong par) {
  short i;
  short is = TypeListIndex(s);
  for (i=0; i < is; i++) {
    if (ship[i].assign == DesignedClass::follow_object &&
	ship[i].AutoResponse() &&
	ship[i].targetPP.obj.ship == s)
      ship[i].Assign(a, par, ship[i].assignPriority);
  }
  for (i=is+1; i < numShips; i++) {
    if (ship[i].assign == DesignedClass::follow_object &&
	ship[i].AutoResponse() &&
	ship[i].targetPP.obj.ship == s)
      ship[i].Assign(a, par, ship[i].assignPriority);
  }
}

void GameStateClass::ReleaseFleetFrom(ShipClass* s) {
  short j;
  for (j = 0; j < numShips; j++) {
    if (ship[j].Assign() == DesignedClass::follow_object &&
	ship[j].targetPP.obj.ship == s) {
      ship[j].SwapAssign(DesignedClass::go_nearest_planet);
      ship[j].duty = none;
    }
  }
}

short GameStateClass::SummonFleetTo(ShipClass* s, short priority) {
  short numShipsResponding = 0;
  for (short i = 0; i < numShips; i++) {
    if ((ship[i].assign == DesignedClass::manual &&
	 ship[i].duty == none || priority) &&
	s->IsFriendlyTo(ship+i) && ship + i != s &&
	ship[i].IsAvailable() &&
	ship[i].AutoResponse() &&
	ship[i].IsMobile() && player[s->playerIndex].rank >= 
	player[ship[i].playerIndex].rank) {
      if (ship[i].FollowObj(s)) {
	numShipsResponding++;
	if (priority) {
	  BindShipTo(ship + i, s);
	}
      }
    }
  }
  return numShipsResponding;
}

void GameStateClass::SummonShipTo(ShipClass* s, short priority) {
  if (IsValidObjectPointer(s->targetPP.obj.generic, __FILE__, __LINE__) &&
      s->targetPP.obj.ship->IsFriendlyTo(s) &&
      s->targetPP.obj.ship != s &&
      s->targetPP.obj.ship->AutoResponse() &&
      s->targetPP.obj.ship->IsMobile() &&
      (s->duty == none || priority) &&
      player[s->playerIndex].rank >= 
      player[s->targetPP.obj.ship->playerIndex].rank) {

    if (s->targetPP.obj.ship->FollowObj(s, priority) && priority) {
      BindShipTo(s->targetPP.obj.ship, s);
    }
  }
}

void GameStateClass::GenericRelease(ShipClass* s) {
  short i, j;

  /* reset docking pending flag in case its out of synch */
  s->ClearPendingFlag(ShipClass::docking);

  if ( IsValidObjectPointer(s->dockObj) ) {

    /* reset docking pending flag in case its out of synch */
    s->dockObj->ClearPendingFlag(ShipClass::docking);

    /* if ship thinks it is docked then request freedom from */
    /* the ship that owns the port                           */
    s->FreeFromDock();
  } else {
    /* if ship has ports, release them */
    s->DeactivateDocks();
  }

  /* reset duty mode */
  if (s->duty != none) {
    PostGameMessage("1st Officer> We have been released from my assigned duty.",
		      ship.indexOf(s), (short)s->raceID, personal);
  }
  s->duty = none;
  s->dutyPar = 0;
  
  for (i = 0; i < numShips; i++) {
    for (j = 0; j < ship[i].bus.numPorts; j++) {
      if (ship[i].bus.dockMod[j]->workPar[0] == slong(s) && 
	  ship[i].raceID == s->raceID) {
	if (ship[i].bus.dockMod[j]->workPar[1])
	  /* then the ship is docked -- free it */
	  ship[i].FreeDock(ship[i].bus.moduleList.indexOf(ship[i].bus.dockMod[j]));
	else {
	   /* then the ship is being tractored -- free it */
	  ship[i].bus.dockMod[j]->ReceiveDockDeactivationMsg();
	  /* tell the ship to stop asking for a dock or it will start over */
	  s->Assign(DesignedClass::manual);
	}
      }
      /* if this object is being built release it */
      
      if (ship[i].bus.factoryMod &&
	  ship[i].bus.factoryMod->workPar[0] == slong(s)) {
	ship[i].bus.factoryMod->workPar[1] = 0;
	ship[i].bus.factoryMod->workPar[0] = 0;
	ship[i].bus.factoryMod->deactivate();
      }
    }
  }

  for (i = 0; i < numColonies; i++) {
    if ( IsValidObjectPointer(colonyList[i]) ) {
      for (j = 0; j < colonyList[i]->bus.numPorts; j++) {
        if (colonyList[i]->bus.dockMod[j]->workPar[0] == slong(s) && 
            colonyList[i]->raceID == s->raceID) {
	  if (colonyList[i]->bus.dockMod[j]->workPar[1])
	    /* then the ship is docked -- free it */
          colonyList[i]->FreeDock(colonyList[i]->bus.moduleList.indexOf(colonyList[i]->bus.dockMod[j]));
	  else {
	    /* then the ship is being tractored -- free it */
	    colonyList[i]->bus.dockMod[j]->ReceiveDockDeactivationMsg();
	    /* tell the ship to stop asking for a dock or it will start over */
	    s->Assign(DesignedClass::manual);
	  }
	}

	/* if this object is being built release it */
	
        if (colonyList[i]->bus.factoryMod &&
            colonyList[i]->bus.factoryMod->workPar[0] == slong(s)) {
	  colonyList[i]->bus.factoryMod->workPar[1] = 0;
	  colonyList[i]->bus.factoryMod->workPar[0] = 0;
	  colonyList[i]->bus.factoryMod->deactivate();
        }
      }
    } else {
      logFile << "ERROR> GameStateClass::GenericRelease colonyList[" << i
	      << "] = NULL" << endl;
    }

  }

  /*****************************************/
  /* deactivate all tubes -                */
  /* this is mostly for the beams, so that */
  /* a player has a way of shutting them   */
  /* off.  The detonate button should do   */
  /* the same thing                        */
  /*****************************************/
  for (i=0; i < s->bus.numTubes; i++)
    s->bus.tubeMod[i]->deactivate();

  s->tubeSelector = 0;
  s->firingTubes = 0;

}

short GameStateClass::validate() {
  /* check validity of all ids */

  static PingPacket starPP;


  short i, j, k;
  short numErr = 0, numSecErr;
  ModuleClass* m;

  logFile << "GameState validation begins..." << endl;
  
  logFile << "   race validation...";

  numSecErr = 0;

  /*******************************************************/
  /* step one                                            */
  /* a. all race ids should be between 0 and numRaces-1  */
  /* b. all fullColonyPct values should be between 0 and */
  /*    the minimum colony size after a spawn event (not */
  /*    due to a spawn-to-dock or defection (90%)        */
  /*******************************************************/
  for (i=0; i<numRaces; i++) {
    if (race[i].id < 0 || race[i].id >= numRaces) {
      numSecErr++;
      if (numSecErr == 1)
	logFile << "failed" << endl;
      logFile << "ERROR> race " <<
	race[i].name << " has invalid id " << race[i].id << endl;
    }
    
    if (race[i].fullColonyPct < 0 || race[i].fullColonyPct >= 90) {
      numSecErr++;
      if (numSecErr == 1)
	logFile << "failed" << endl;
      logFile << "ERROR> race " <<
	race[i].name << " has invalid full colony pct (" << race[i].fullColonyPct <<
	"%).  It should be between 0 and 90%" << endl;
    }
  }

  if (!numSecErr)
    logFile << "passed" << endl;
  else
    numErr += numSecErr;

  numSecErr = 0;

  logFile << "   player validation...";

  /* step two */
  /* all players should have valid ship ids */
  for (i=0; i<numStoredPlayers; i++) {
    if (!MapPlayerShipIdToIndex(i)) {
      numSecErr++;
      if (numSecErr == 1)
	logFile << "failed" << endl;
      logFile <<
	"ERROR> found invalid ship id (" <<
	player[i].shipId << ") for " << player[i].name << " (" << i << ")" <<
	endl;
    }
  }

  if (!numSecErr)
    logFile << "passed" << endl;
  else
    numErr += numSecErr;

  numSecErr = 0;

  logFile << "   ship design validation...";

  /* step three */
  /* all ship designs should have valid ids */
  slong vol;
  for (i=0; i<numShipDesigns; i++) {
    vol = 0;
    for (j=0; j<shipDesign[i].bus.numMod; j++) {
      if ((k = ValidateModuleID(shipDesign[i].bus.moduleIdList[j])) == -1) {
	numSecErr++;
	if (numSecErr == 1)
	  logFile << "failed" << endl;
	logFile << "ERROR> found invalid module id (" <<
	  shipDesign[i].bus.moduleIdList[j] << ") in " << 
	  race[shipDesign[i].raceID].name << " " << shipDesign[i].name << endl;
      } else {
	/* volume must be less than or equal to 100% */
	vol += game->moduleList[k].volume;
      }
    }

    if (vol > shipDesign[i].volumeCap) {
      numSecErr++;
      if (numSecErr == 1)
	logFile << "failed" << endl;
      logFile << "ERROR> volume capacity exceeded (" <<
	vol << " > " << shipDesign[i].volumeCap << ") in " << 
	race[shipDesign[i].raceID].name << " " << shipDesign[i].name << endl;
    }
      
  }

  if (!numSecErr)
    logFile << "passed" << endl;
  else
    numErr += numSecErr;

  numSecErr = 0;

  logFile << "   missile design validation...";

  /* step four */
  /* all missile designs should have valid ids */
  for (i=0; i<numMissileDesigns; i++) {
    vol = 0;
    for (j=0; j<missileDesign[i].bus.numMod; j++) {
      if ((k = ValidateModuleID(missileDesign[i].bus.moduleIdList[j])) == -1) {
	numSecErr++;
	if (numSecErr == 1)
	  logFile << "failed" << endl;
	logFile << "ERROR> found invalid module id (" <<
	  missileDesign[i].bus.moduleIdList[j] << ") in " << 
	  race[missileDesign[i].raceID].name << " " << 
	  missileDesign[i].name << endl;
      } else {
	/* volume must be less than or equal to 100% */
	vol += game->moduleList[k].volume;
      }
    }

    if (vol > missileDesign[i].volumeCap) {
      numSecErr++;
      if (numSecErr == 1)
	logFile << "failed" << endl;
      logFile << "ERROR> volume capacity exceeded (" <<
	vol << " > " << missileDesign[i].volumeCap << ") in " << 
	race[missileDesign[i].raceID].name << " " << missileDesign[i].name << endl;
    }
  }

  if (!numSecErr)
    logFile << "passed" << endl;
  else
    numErr += numSecErr;

  numSecErr = 0;

  logFile << "   colony design validation...";

  /* step five */
  /* all colony designs should have valid ids */
  for (i=0; i<numColonyDesigns; i++) {
    vol = 0;
    for (j=0; j<colonyDesign[i].bus.numMod; j++) {
      if ((k = ValidateModuleID(colonyDesign[i].bus.moduleIdList[j])) == -1) {
	numSecErr++;
	if (numSecErr == 1)
	  logFile << "failed" << endl;
	logFile << "ERROR> found invalid module id (" <<
	  colonyDesign[i].bus.moduleIdList[j] << ") in slot " << 
	  j << " in " << race[colonyDesign[i].raceID].name <<
	  " colony design " << i << endl;
      } else {
	/* volume must be less than or equal to 100% */
	vol += game->moduleList[k].volume;
      }
    }

    if (vol > colonyDesign[i].volumeCap) {
      numSecErr++;
      if (numSecErr == 1)
	logFile << "failed" << endl;
      logFile << "ERROR> volume capacity exceeded (" <<
	vol << " > " << colonyDesign[i].volumeCap << ") in " << 
	race[colonyDesign[i].raceID].name << " " << colonyDesign[i].name << endl;
    }
  }

  if (!numSecErr)
    logFile << "passed" << endl;
  else
    numErr += numSecErr;

  numSecErr = 0;

  logFile << "   module list validation...";

  /* step six */
  /* all missile tubes should have valid missile ids */
  for (i=0; i<numModules; i++) {
    m = moduleList + i;
    if ((m->moduleFunc == ModuleClass::tube || m->moduleFunc == ModuleClass::ram) &&
	m->par[3] == -1
	/*LookUpMissileDesignId(m->par[3]) == -1 */) {
      numSecErr++;
      if (numSecErr == 1)
	logFile << "failed" << endl;
      logFile << "ERROR> found invalid missile design id (" <<
	m->par[3] << ") in " << m->name << " " <<
	race[m->raceID].name << " tube (" << m->id << ")" << endl;
    }
  }

  if (!numSecErr)
    logFile << "passed" << endl;
  else
    numErr += numSecErr;

  numSecErr = 0;

  logFile << "   planet list validation...";

  /* step seven */
  /* all planets should have valid revertRaceIndex and valid revertColonyDesignIndex  */
  /* all planets should be outside the damage radius for their star */
  for (i=0; i<numPlanets; i++) {
    if (planet[i].revertRaceIndex < -1 || planet[i].revertRaceIndex >= numRaces ||
	planet[i].revertColonyDesignIndex < 0 || planet[i].revertColonyDesignIndex >= numColonyDesigns) {
      numSecErr++;
      if (numSecErr == 1)
	logFile << "failed" << endl;
      logFile << "ERROR> found invalid revert race id / colony design index (" <<
	planet[i].revertRaceIndex << " / " << planet[i].revertColonyDesignIndex << ") in " <<
	planet[i].name << endl;
    }
    
    
    /* is the planet is close enough to take damage? */
    if (planet[i].minor != -1) {
      starPP = planet[i].Ping(star_target);
      slong dStar = starPP.dist;
      slong rStar = 2 * starPP.obj.planet->bitmap.size * sub_scale;

      slong minR = 2 * rStar + planet[i].bitmap.size * sub_scale / 2;
      
      if ( IsValidObjectPointer(starPP.obj.generic,__FILE__,__LINE__) && 
           /* starPP.obj.planet == planet[i].parent.planet && */ dStar < minR) {
	numSecErr++;
	if (numSecErr == 1)
	  logFile << "failed" << endl;
	logFile << setprecision(3);
	if (starPP.obj.planet == planet[i].parent.planet)
	  logFile << "ERROR> planet " << planet[i].name << " is within the damage radius of " <<
	    starPP.obj.planet->name << ".  Its orbital radius (" <<
	    double(rStar) / au_to_pixel << " AU) should be at least " <<
	    double(minR) / au_to_pixel << " AU" << endl;
	else
	  logFile << "ERROR> planet " << planet[i].name << " is within the damage radius of " <<
	    starPP.obj.planet->name << ".  Its initial angle should be changed to move it away."  << endl;
      }
    }
  }
  
  if (!numSecErr)
    logFile << "passed" << endl;
  else
    numErr += numSecErr;

  if (numErr)
    logFile << "... GameState validation failed" << endl;
  else
    logFile << "... GameState validation passed" << endl;

  return numErr;
}

void GameStateClass::Profile() {
  logFile << "GameStateClass profile for " << instanceName <<
    " on frame " << frameNumber << endl;
  logFile << "number of ships           =   " << numShips << endl;
  logFile << "number of planets         =   " << numPlanets << endl;
  logFile << "number of missiles        =   " << numMissiles << endl;
  logFile << "number of colonies        =   " << numColonies << endl;
  logFile << "number of ship designs    =   " << numShipDesigns <<
    "(" << maxShipDesigns << ")" << endl;
  logFile << "number of missile designs =   " << numMissileDesigns <<
    "(" << maxMissileDesigns << ")" << endl;
  logFile << "number of modules         =   " << numModules <<
    "(" << maxModules << ")" << endl;
  logFile << "number of loaded modules  =   " << numLoadedModules << endl;

  logFile << "number of robot ships in: " << endl;
  logFile << "   planet pool            = " << numPlanetRobots << endl;
  logFile << "   population pool        = " << numPopRobots << endl;

  logFile << "one robot is accumulated per" << endl;
  logFile << "   " << planetsPerRobot << " planets " << endl;
  logFile << "   " << popPercentPerRobot << "% of total population" << endl;

  short i,j;
  logFile.width(27);
  logFile << "planet name";
  for (j=0; j<numRaces; j++) {
    logFile.width(12);
    logFile << race[j].name;
  }
  logFile.width(12);
  logFile << "Exp. Pot.";
  logFile << endl;
  for (i=0; i<numPlanets; i++) {
    logFile.width(3);
    logFile << i;
    logFile.width(24);
    logFile << planet[i].name;
    for (j=0; j<numRaces; j++) {
      logFile.width(12);
      logFile << planet[i].OptimalPopulation(race[j]).print();
    }
    logFile.width(12);
    logFile << Format(planet[i].ExploitationPotential());
    logFile << endl;
  }

  slong size = Size();

  logFile << "Size of Packed Game State =   " << Format(size) <<
    " bytes." << endl;

//    logFile << "ShipClass Section:            " << 
//      (sizeof(GameStateClass) + numShips*sizeof(ShipClass)) * 100 / size <<
//      " %" << endl;

//    logFile << "PlanetClass Section:          " << 
//      numPlanets*sizeof(PlanetClass) * 100 / size <<
//      " %" << endl;
//    logFile << "MissileClass Section:         " << 
//      numMissiles*sizeof(MissileClass) * 100 / size <<
//      " %" << endl;
//    logFile << "GameBitmapClass Section:      " << 
//      numBitmaps*sizeof(GameBitmapClass) * 100 / size <<
//      " %" << endl;
//    logFile << "ShipDesignStruct Section:     " <<  numShipDesigns*
//      sizeof(ShipDesignStruct) * 100 / size <<
//      " %" << endl;
//    logFile << "MissileDesignStruct Section:  " << numMissileDesigns*
//  	  sizeof(MissileDesignStruct) * 100 / size <<
//      " %" << endl; 
//    logFile << "ModuleClass Section:          " << 
//      numModules*sizeof(ModuleClass)  * 100 / size <<
//      " %" << endl;
//    logFile << "Loaded ModuleClass Section:   " <<
//      numLoadedModules*sizeof(ModuleClass) * 100 / size <<
//      " %" << endl;
//    logFile << "RaceClass Section:            " << numRaces *
//  	      sizeof(RaceClass) * 100 / size <<
//      " %" << endl;
//    logFile << "Stored PlayerClass Section:   " <<
//      maxStoredPlayers * sizeof(PlayerClass) * 100 / size <<
//      " %" << endl;
//    logFile << "Active PlayerClass Section:   " <<
//      numShips * sizeof(PlayerClass*) * 100 / size <<
//      " %" << endl;
//    logFile << "RankStruct Section:           " << 
//      numRanks * sizeof(RankStruct) * 100 / size <<
//      " %" << endl;
//    logFile << "ColonyClass Section:          " <<
//      numColonies * (sizeof(ColonyClass)) * 100 / size <<
//      " %" << endl;
//    logFile << "ColonyDesignStruct Section:   " << 
//      numColonyDesigns * (sizeof(ColonyDesignStruct)) * 100 / size <<
//      " %" << endl;

  //for (i=0; i<NumObjects(); i++) {
  //  if (objectList[i]->active) {
  //    logFile << "check sum profile for object " << i << " follows:" << endl;
  //    objectList[i]->ProfileCheckSum(this);
  //  }
  //}
  
  //logFile << "PLAYER PROFILES" << endl;
  
  //for (i=0; i<numStoredPlayers; i++) {
  // player[i].profile(this);
  //} 
}

void GameStateClass::ProfileModuleList() {

  logFile << "*** GameState Module List Profile ***" << endl;

  short i;
  for (i=0; i<numModules; i++) {
    logFile.width(max_name_char+3);
    logFile << race[moduleList[i].raceID].name;
    logFile.width(max_name_char+3);
    logFile << moduleList[i].name;
    logFile.width(12);
    logFile << moduleList[i].value() << endl;
  }
}

short GameStateClass::CreateColonyOnPlanet(short index, short raceIndex,
					   PopStruct p, short designIndex) {
  /* create new colony from colony class object */

  short isNew;
  ColonyClass* c;

  if (planet[index].colony)
    /* destroy it to make room in list */
    ReceiveDestroyObjectMsg(planet[index].colony, 0);

  /* set planet alignment -- this will determine race of colony */
  planet[index].raceID = raceIndex;

  isNew = NewObject(&c) != -1;
  
  if (isNew) {

    /* first call the constructor */
    c->initialize();

    short i = 0;
    if (designIndex == -1) {
      /* find a suitable design */
      while(colonyDesign[i].raceID != planet[index].raceID &&
	    i < numColonyDesigns)
	i++;

      if (i == numColonyDesigns)
	i = 0;

      designIndex = i;
    }

    /* set the design */
    c->design = colonyDesign + designIndex;

    /* then initialize based on the design */
    if (c->InitializeFromDesign(c)) {
      /* decrement the colony counter */
      numColoniesCreated--;
      return -1; /* why not InitializeFromColonyDesign? */
    }

    c->stat = DesignedClass::idle;
    c->viewObj.colony = c;
    c->raceID = planet[index].raceID;
    c->playerIndex = 0;
    //strcpy(c->design->name, "Colony");

    c->pop = p;

    c->now = planet[index].now;
    c->gnow = c->now;
    c->vel = planet[index].vel;
    c->tvel = planet[index].tvel;

    planet[index].colony = c;

    c->lockObj.generic = planet + index;
    c->planetIndex = index;
    c->parent = c->lockObj;

    strcpy(c->name, c->design->name);
    
    c->activate();

    logFile << "note> New " << race[c->raceID].name <<
      " colony placed on " << planet[index].name <<
      " on frame " << frame << endl;
  }
  if (isNew)
    return(colony.indexOf(c));
  else
    return -1;
}

void GameStateClass::ApplyModuleDefaults() {

  /* assign a default name if a "*" is specified */
  /* set debris bitmap index */
  short i, index;
  slong id;

  SetDefaultsApplied();

  for (i=0; i<numModules; i++) {

    moduleList[i].debrisBitmapIndex = (short)((moduleList[i].id % 10) %
      debrisCount + race[moduleList[i].raceID].debrisOffset +
      debrisBitmapIndexStart);


    if (moduleList[i].moduleFunc != ModuleClass::tube && moduleList[i].moduleFunc != ModuleClass::ram) {
      
      /* this is obsolete now -- the code has been moved to ModuleClass::get */
      //if (strcmp(moduleList[i].name, "*") == 0)
      //sprintf(moduleList[i].name, "%s:%d",
      //	funcName[moduleList[i].moduleFunc], moduleList[i].id%10);

    } else {

      id = moduleList[i].par[3];
      index = LookUpMissileDesignId(id);
      
      switch (moduleList[i].moduleFunc) {
	
      case ModuleClass::tube:
	
	if (index == -1) {
	  logFile <<
	    "ERROR> ApplyModuleDefaults: found invalid missile id " <<
	    id << endl;
	  *moduleList[i].name = '\0';
	} else {

	  if (strcmp(moduleList[i].name, "*") == 0)
	    strncpy(moduleList[i].name, missileDesign[index].name, max_module_name_char);
	  
	}
	
	break;
	
      case ModuleClass::ram:

	/* this is obsolete now -- the code has been moved to ModuleClass::get */
	//if (strcmp(moduleList[i].name, "*") == 0)
	// sprintf(moduleList[i].name, "%s:%d",
	//  funcName[moduleList[i].moduleFunc], moduleList[i].id%10);
	
	break;
	
      }
      
      /*********************************************************/
      /* also remap the par[3] parameter to the index value so */
      /* we don't have to keep doing lookups                   */
      /*********************************************************/
	
      /* this mapping now occurs in the PlugInModulesByDesign function */
      moduleList[i].SetStat(moduleList[i].par[3],(slong)index);
      //moduleList[i].par[3] = index;
      
    }
  }
}

void GameStateClass::ProfileObjectList() {
  short i;
  for (i=0; i<NumObjects(); i++) {
    if (objectList[i]->IsActive())
      logFile << i << " --> " << objectList[i]->TypeName() <<
	TypeListIndex(objectList[i]) << " (" <<
	ObjectListIndex(objectList[i]) << ")" << endl;
  }
}

short GameStateClass::PlayerIsUpgradeable(short pI) {
  short i;
  for (i=0; i<numShipDesigns; i++)
    if (shipDesign[i].raceID == player[pI].raceId &&
	shipDesign[i].reqRank > shipDesign[player[pI].shipIndex].reqRank &&
	shipDesign[i].inFrame <= frame &&
	player[pI].rank >= shipDesign[i].reqRank)
      return 1;
  
  return 0;
}
  
slong GameStateClass::PushTo(GameStateBuffer& gb) {

  char* bufferPos = gb.BufferPosition();

  /* takes a game state buffer for storing a copy of the game state */
  
  /* update the missile list */
  MapObjectList();
  
  /* prepare loaded modules for transfer */
  //CreateLoadedModuleList(); /* 5/8 */
  
  /* copy the current game state to shared memory address */
  /* Profile(); */
  /* *g = *this; */
  
  /* this is not transferred through the constructor */
  /* g->numLoadedModules = numLoadedModules; */
  
  /* set up technology lists for new game state */
  /* 
     g->tech = new ListStruct[numRaces];
     short i;
     for (i=0; i<numRaces; i++)
     g->tech[i].InitializeWith(0); */ /* 5/8 */

  /* set object-by-type list pointers to next available s.m. slot */
  
  CALL_PUSH_OBJ(gb,numShips);
  CALL_PUSH_OBJ(gb,numPlanets);
  CALL_PUSH_OBJ(gb,numMissiles);
  CALL_PUSH_OBJ(gb,numBitmaps);
  CALL_PUSH_OBJ(gb,numShipDesigns);
  CALL_PUSH_OBJ(gb,numMissileDesigns);
  CALL_PUSH_OBJ(gb,numModules);
  CALL_PUSH_OBJ(gb,numRaces);
  CALL_PUSH_OBJ(gb,numStoredPlayers);
  CALL_PUSH_OBJ(gb,numRanks);
  CALL_PUSH_OBJ(gb,numColonies);
  CALL_PUSH_OBJ(gb,numColonyDesigns);
  CALL_PUSH_OBJ(gb,numSounds);
  CALL_PUSH_OBJ(gb,maxMissiles);
  CALL_PUSH_OBJ(gb,maxBitmaps);
  CALL_PUSH_OBJ(gb,maxShipDesigns);
  CALL_PUSH_OBJ(gb,maxMissileDesigns);
  CALL_PUSH_OBJ(gb,maxModules);
  CALL_PUSH_OBJ(gb,maxRaces);
  CALL_PUSH_OBJ(gb,maxStoredPlayers);
  CALL_PUSH_OBJ(gb,extraColonies);

  CALL_PUSH_OBJ(gb,year);
  CALL_PUSH_OBJ(gb,numActiveShips);
  CALL_PUSH_OBJ(gb,numMissilesDestroyed);
  CALL_PUSH_OBJ(gb,numRobotPlayers);
  CALL_PUSH_OBJ(gb,numColoniesDestroyed);
  CALL_PUSH_OBJ(gb,numColoniesCreated);
  CALL_PUSH_OBJ(gb,maxHumanShips);
  CALL_PUSH_OBJ(gb,numLoadedModules);
  CALL_PUSH_OBJ(gb,lastMissile);
  CALL_PUSH_OBJ(gb,shipBitmapIndexStart);
  CALL_PUSH_OBJ(gb,planetBitmapIndexStart);
  CALL_PUSH_OBJ(gb,missileBitmapIndexStart);
  CALL_PUSH_OBJ(gb,colonyBitmapIndexStart);
  CALL_PUSH_OBJ(gb,explosionBitmapIndexStart);
  CALL_PUSH_OBJ(gb,transitionBitmapIndexStart);
  CALL_PUSH_OBJ(gb,debrisBitmapIndexStart);
  CALL_PUSH_OBJ(gb,miscellaneousBitmapIndexStart);
  CALL_PUSH_OBJ(gb,frame);
  CALL_PUSH_OBJ(gb,mode);
  CALL_PUSH_OBJ(gb,maxAcc);
  CALL_PUSH_OBJ(gb,maxAngles);
  CALL_PUSH_OBJ(gb,planetShipGFactor);
  CALL_PUSH_OBJ(gb,planetPlanetGFactor);
  CALL_PUSH_OBJ(gb,damageToMass);
  CALL_PUSH_OBJ(gb,offensiveFactor);
  CALL_PUSH_OBJ(gb,salvageRightPct);
  CALL_PUSH_OBJ(gb,landRightFactor);
  CALL_PUSH_OBJ(gb,bountyRightPct);
  CALL_PUSH_OBJ(gb,loyaltyCycle);
  CALL_PUSH_OBJ(gb,surrenderAllowed);
  CALL_PUSH_OBJ(gb,AILevel);
  CALL_PUSH_OBJ(gb,defectionRate);
  CALL_PUSH_OBJ(gb,numPopRobots);
  CALL_PUSH_OBJ(gb,numPlanetRobots);
  CALL_PUSH_OBJ(gb,popPercentPerRobot);
  CALL_PUSH_OBJ(gb,planetsPerRobot);
  CALL_PUSH_OBJ(gb,randomSeed);
  CALL_PUSH_OBJ(gb,debrisCount);
  CALL_PUSH_OBJ(gb,colonyPending);
  CALL_PUSH_OBJ(gb,pluginAction);
  CALL_PUSH_OBJ(gb,pluginPar);
  CALL_PUSH_OBJ(gb,bonusFactor);

  CALL_PUSH_OBJ(gb,totPop);

  CALL_PUSH_OBJ(gb,lightSpeed);

  CALL_PUSH_OBJ(gb,scenarioName);

  /* count number of humans playing and append to description */
  short numHumansOnline = 0;
  short i;
  for (i=0; i<maxHumanShips; i++)
    if (ship[i].IsActive())
      numHumansOnline++;

  char desc[8192];
  if (strlen(scenarioDesc) < 8100)
    sprintf(desc,"%s - There are %d human pilots online (not including you).", scenarioDesc, numHumansOnline);
  else
    strcpy(desc, scenarioDesc);

  CALL_PUSH_OBJ(gb,desc);
  CALL_PUSH_OBJ(gb,fileName);

  short index;
  for (i = 0; i < numMissiles /*lastMissile*/; i++) {
    index = missile.indexOf(missileList[i]);
    if ( !IsValidObjectPointer(missileList[i]) )
      {
	logFile << 
	  "ERROR: Pushing bad missileList object " << i <<
	  " index " << index;
      }
    CALL_PUSH_OBJ(gb, index);
    CALL_PUSH_GAME_OBJ(gb, this,missile[index]);
  }

  for (i = 0; i < numShips; i++) {
    if ( !IsValidObjectPointer(&ship[i]) )
      {
	logFile << "ERROR: Pushing bad ship object " << i;
      }
    CALL_PUSH_GAME_OBJ(gb, this,ship[i]);
  }

  for (i = 0; i < numPlanets; i++) {
    if ( !IsValidObjectPointer(&planet[i]) )
      {
	logFile << "ERROR: Pushing bad planet object " << i;
      }
    CALL_PUSH_GAME_OBJ(gb, this,planet[i]);
  }

  for (i = 0; i < numBitmaps; i++) {
    CALL_PUSH_GAME_OBJ(gb, this,objectBitmapList[i]);
  }

  for (i = 0; i < numShipDesigns; i++) {
    CALL_PUSH_GAME_OBJ(gb, this,shipDesign[i]);
  }

  for (i = 0; i < numMissileDesigns; i++) {
    CALL_PUSH_GAME_OBJ(gb, this,missileDesign[i]);
  }

  for (i = 0; i < numModules; i++) {
    CALL_PUSH_GAME_OBJ(gb, this,moduleList[i]);
  }

  for (i = 0; i < numRaces; i++) {
    CALL_PUSH_GAME_OBJ(gb, this,race[i]);
  }

  /* pack player array */
  for (i=0; i < maxStoredPlayers; i++) {
      CALL_PUSH_GAME_OBJ(gb, this,player[i]);
  }
  
  /* pack playerList arrray */
  for (i = 0; i < numShips; i++) {
    index = player.indexOf(playerList[i]);
    CALL_PUSH_OBJ(gb, index);
  }

  /* pack rank array */
  for (i = 0; i < numRanks; i++) {
    CALL_PUSH_GAME_OBJ(gb, this,rank[i]);
  }

  /* pack colony array */
  for (i = 0; i < numColonies; i++) {
    index = colony.indexOf(colonyList[i]);
    if ( !IsValidObjectPointer(colonyList[i]) )
      {
	logFile << 
	  "ERROR: Pushing bad colony object " << i << 
	  " index " << index;
	index = 0;
      }
    CALL_PUSH_OBJ(gb, index);
    CALL_PUSH_GAME_OBJ(gb, this, colony[index]);
  }

  for (i = 0; i < numColonyDesigns; i++) {
    CALL_PUSH_GAME_OBJ(gb, this, colonyDesign[i]);
  }

  for (i = 0; i < numSounds; i++) {
    CALL_PUSH_GAME_OBJ(gb, this, sound[i]);
  }

  for (i = 0; i < last_event; i++) {
    CALL_PUSH_OBJ(gb, soundEvent[i]);
  }

  for (i = 0; i < numRaces; i++) {
    //ls.copy(tech[i]);
    //ls.Pack();
    //g->tech[i].CopyTo(ls, numModules);
    CALL_PUSH_OBJ(gb,tech[i]);
  }

  //ProfileObjectList();
  
  //Profile();

  logFile << "note> Pushed game state on frame " << frame << endl;

  return gb.BufferPosition() - bufferPos;
}


slong GameStateClass::PopFrom(GameStateBuffer& gb) {

  char* bufferPos = gb.BufferPosition();

  //
  // For some reason this call results in a crash when deleting powermod.
  //
  DestroyCurrentState();

  CALL_POP_OBJ(gb,numShips);
  CALL_POP_OBJ(gb,numPlanets);
  CALL_POP_OBJ(gb,numMissiles);
  CALL_POP_OBJ(gb,numBitmaps);
  CALL_POP_OBJ(gb,numShipDesigns);
  CALL_POP_OBJ(gb,numMissileDesigns);
  CALL_POP_OBJ(gb,numModules);
  CALL_POP_OBJ(gb,numRaces);
  CALL_POP_OBJ(gb,numStoredPlayers);
  CALL_POP_OBJ(gb,numRanks);
  CALL_POP_OBJ(gb,numColonies);
  CALL_POP_OBJ(gb,numColonyDesigns);
  CALL_POP_OBJ(gb,numSounds);
  CALL_POP_OBJ(gb,maxMissiles);
  CALL_POP_OBJ(gb,maxBitmaps);
  CALL_POP_OBJ(gb,maxShipDesigns);
  CALL_POP_OBJ(gb,maxMissileDesigns);
  CALL_POP_OBJ(gb,maxModules);
  CALL_POP_OBJ(gb,maxRaces);
  CALL_POP_OBJ(gb,maxStoredPlayers);
  CALL_POP_OBJ(gb,extraColonies);

  InitializeWith(numShips,
		 numPlanets,
		 numMissiles,
		 numBitmaps,
		 numShipDesigns,
		 numMissileDesigns,
		 numModules,
		 numRaces,
		 numStoredPlayers,
		 numRanks,
		 numColonies,
		 numColonyDesigns,
		 numPlanets, // not maxPl...
		 maxMissiles,
		 maxBitmaps,
		 maxShipDesigns,
		 maxMissileDesigns,
		 maxModules,
		 maxRaces,
		 maxStoredPlayers,
		 extraColonies,
		 numSounds);

  CALL_POP_OBJ(gb,year);
  CALL_POP_OBJ(gb,numActiveShips);
  CALL_POP_OBJ(gb,numMissilesDestroyed);
  CALL_POP_OBJ(gb,numRobotPlayers);
  CALL_POP_OBJ(gb,numColoniesDestroyed);
  CALL_POP_OBJ(gb,numColoniesCreated);
  CALL_POP_OBJ(gb,maxHumanShips);
  CALL_POP_OBJ(gb,numLoadedModules);
  CALL_POP_OBJ(gb,lastMissile);
  CALL_POP_OBJ(gb,shipBitmapIndexStart);
  CALL_POP_OBJ(gb,planetBitmapIndexStart);
  CALL_POP_OBJ(gb,missileBitmapIndexStart);
  CALL_POP_OBJ(gb,colonyBitmapIndexStart);
  CALL_POP_OBJ(gb,explosionBitmapIndexStart);
  CALL_POP_OBJ(gb,transitionBitmapIndexStart);
  CALL_POP_OBJ(gb,debrisBitmapIndexStart);
  CALL_POP_OBJ(gb,miscellaneousBitmapIndexStart);
  CALL_POP_OBJ(gb,frame);
  CALL_POP_OBJ(gb,mode);

  if (!ModeIsSet(betweenFrames)) {
    logFile << "ERROR> game state grabbed mid-frame on frame " << frame << endl;
  }

  CALL_POP_OBJ(gb,maxAcc);
  CALL_POP_OBJ(gb,maxAngles);
  CALL_POP_OBJ(gb,planetShipGFactor);
  CALL_POP_OBJ(gb,planetPlanetGFactor);
  CALL_POP_OBJ(gb,damageToMass);
  CALL_POP_OBJ(gb,offensiveFactor);
  CALL_POP_OBJ(gb,salvageRightPct);
  CALL_POP_OBJ(gb,landRightFactor);
  CALL_POP_OBJ(gb,bountyRightPct);
  CALL_POP_OBJ(gb,loyaltyCycle);
  CALL_POP_OBJ(gb,surrenderAllowed);
  CALL_POP_OBJ(gb,AILevel);
  CALL_POP_OBJ(gb,defectionRate);
  CALL_POP_OBJ(gb,numPopRobots);
  CALL_POP_OBJ(gb,numPlanetRobots);
  CALL_POP_OBJ(gb,popPercentPerRobot);
  CALL_POP_OBJ(gb,planetsPerRobot);
  CALL_POP_OBJ(gb,randomSeed);
  CALL_POP_OBJ(gb,debrisCount);
  CALL_POP_OBJ(gb,colonyPending);
  CALL_POP_OBJ(gb,pluginAction);
  CALL_POP_OBJ(gb,pluginPar);
  CALL_POP_OBJ(gb,bonusFactor);

  CALL_POP_OBJ(gb,totPop);

  CALL_POP_OBJ(gb,lightSpeed);

  CALL_POP_OBJ(gb,scenarioName);
  CALL_POP_OBJ(gb,scenarioDesc);
  CALL_POP_OBJ(gb,fileName);

  InitDir();

  MapObjectList();
  
  short i;
  short index;
  for (i = 0; i < numMissiles; i++) {
    CALL_POP_OBJ(gb,index);
    missileList[i] = missile + index;
    objectList[i + missileObjectIndexStart] = missileList[i];
    CALL_POP_GAME_OBJ(gb, this, missile[index]);
  }

  for (i = 0; i < numShips; i++) {
    CALL_POP_GAME_OBJ(gb, this, ship[i]);
  }

  for (i = 0; i < numPlanets; i++) {
    CALL_POP_GAME_OBJ(gb, this, planet[i]);
  }

  for (i = 0; i < numBitmaps; i++) {
    CALL_POP_GAME_OBJ(gb, this, objectBitmapList[i]);
  }

  for (i = 0; i < numShipDesigns; i++) {
    CALL_POP_GAME_OBJ(gb, this, shipDesign[i]);
  }

  for (i = 0; i < numMissileDesigns; i++) {
    CALL_POP_GAME_OBJ(gb, this, missileDesign[i]);
  }

  for (i = 0; i < numModules; i++) {
    CALL_POP_GAME_OBJ(gb, this, moduleList[i]);
  }

  for (i = 0; i < numRaces; i++) {
    CALL_POP_GAME_OBJ(gb, this, race[i]);
  }

  /* pack player array */
  for (i=0; i < maxStoredPlayers; i++) {
      CALL_POP_GAME_OBJ(gb, this, player[i]);
  }
  
  /* pack playerList arrray */
  for (i = 0; i < numShips; i++) {
    CALL_POP_OBJ(gb, index);
    playerList[i] = player + index;
  }

  /* pack rank array */
  for (i = 0; i < numRanks; i++) {
    CALL_POP_GAME_OBJ(gb, this,rank[i]);
  }

  /* pack colony array */
  for (i = 0; i < numColonies; i++) {
    CALL_POP_OBJ(gb, index);
    colonyList[i] = colony + index;
    CALL_POP_GAME_OBJ(gb, this, colony[index]);
  }

  for (i = 0; i < numColonyDesigns; i++) {
    CALL_POP_GAME_OBJ(gb, this, colonyDesign[i]);
  }

  for (i = 0; i < numSounds; i++) {
    CALL_POP_GAME_OBJ(gb, this, sound[i]);
  }

  for (i = 0; i < last_event; i++) {
    CALL_POP_OBJ(gb, soundEvent[i]);
  }

  for (i = 0; i < numRaces; i++) {
    CALL_POP_OBJ(gb,tech[i]);
  }

  MapObjectList();

  Profile();

  return gb.BufferPosition() - bufferPos;  
}

slong GameStateClass::Size() {
  slong size = sizeof(GameStateClass) + 
    numShips*sizeof(ShipClass) +
    numPlanets*sizeof(PlanetClass) + 
    /*lastMissile*/numMissiles*sizeof(MissileClass) + 
    numMissiles*sizeof(MissileClass*) +
    numBitmaps*sizeof(GameBitmapClass) + 
    numShipDesigns*sizeof(ShipDesignStruct) + 
    numMissileDesigns*sizeof(MissileDesignStruct) + 
    numModules*sizeof(ModuleClass) +
    /*numLoadedModules*sizeof(ModuleClass) + */
    (numShips + numColonies + numMissiles) * max_module_list *
    sizeof(ModuleClass) +
    numRaces*sizeof(RaceClass) + 
    maxStoredPlayers*sizeof(PlayerClass) +
    numShips*sizeof(PlayerClass*) +
    numRanks*sizeof(RankStruct) +
    numColonies*sizeof(ColonyClass) +
    numColonies*sizeof(ColonyClass*) + 
    numColonyDesigns*sizeof(ColonyDesignStruct) +
    numRaces*(sizeof(ListStruct) + 
    numModules*sizeof(char));
   return size;
}


/* GameStateClass* GameStateClass::UnPack() {
  // return an unpacked GameStateClass

  logFile << "GameStateClass::UnPack: unpacking " << 
    ::UnPack(numLoadedModules) << " modules." << endl;

  GameStateClass* g = new GameStateClass(::UnPack(numShips),
					 ::UnPack(numPlanets),
					 ::UnPack(numMissiles),
					 ::UnPack(numBitmaps),
					 ::UnPack(numShipDesigns),
					 ::UnPack(numMissileDesigns),
					 ::UnPack(numModules),
					 ::UnPack(numRaces),
					 ::UnPack(numStoredPlayers),
					 ::UnPack(numRanks),
					 ::UnPack(numColonies),
					 ::UnPack(numColonyDesigns),
					 ::UnPack(numPlanets), // not maxPl...
					 ::UnPack(maxMissiles),
					 ::UnPack(maxBitmaps),
					 ::UnPack(maxShipDesigns),
					 ::UnPack(maxMissileDesigns),
					 ::UnPack(maxModules),
					 ::UnPack(maxRaces),
					 ::UnPack(maxStoredPlayers),
					 ::UnPack(extraColonies));
  g->numRobotPlayers = ::UnPack(numRobotPlayers);
  g->numActiveShips = ::UnPack(numActiveShips);
  g->numMissilesDestroyed = ::UnPack(numMissilesDestroyed);
  g->numColoniesDestroyed = ::UnPack(numColoniesDestroyed);
  g->numColoniesCreated = ::UnPack(numColoniesCreated);
  g->maxHumanShips = ::UnPack(maxHumanShips);
  g->maxColonies = ::UnPack(maxColonies);
  g->lastMissile = ::UnPack(lastMissile);
  g->numLoadedModules = ::UnPack(numLoadedModules);
  g->loadedModuleList = new ModuleClass[g->numLoadedModules];
  //g->shipObjectIndexStart = ::UnPack(shipObjectIndexStart);
  //g->planetObjectIndexStart = ::UnPack(planetObjectIndexStart);
  //g->missileObjectIndexStart = ::UnPack(missileObjectIndexStart);
  g->shipBitmapIndexStart = ::UnPack(shipBitmapIndexStart);
  g->planetBitmapIndexStart = ::UnPack(planetBitmapIndexStart);
  g->missileBitmapIndexStart = ::UnPack(missileBitmapIndexStart);
  g->colonyBitmapIndexStart = ::UnPack(colonyBitmapIndexStart);
  g->explosionBitmapIndexStart = ::UnPack(explosionBitmapIndexStart);
  g->transitionBitmapIndexStart = ::UnPack(transitionBitmapIndexStart);
  g->debrisBitmapIndexStart = ::UnPack(debrisBitmapIndexStart);
  g->miscellaneousBitmapIndexStart = ::UnPack(miscellaneousBitmapIndexStart);
  g->year = ::UnPack(year);
  g->frame = ::UnPack(frame);
  g->mode = ::UnPack(mode);
  g->maxAcc = ::UnPack(maxAcc);
  g->planetShipGFactor = ::UnPack(planetShipGFactor);
  g->planetPlanetGFactor = ::UnPack(planetPlanetGFactor);
  g->damageToMass = ::UnPack(damageToMass);
  g->numPopRobots = ::UnPack(numPopRobots);
  g->numPlanetRobots = ::UnPack(numPlanetRobots);
  g->popPercentPerRobot = ::UnPack(popPercentPerRobot);
  g->planetsPerRobot = ::UnPack(planetsPerRobot);
  g->randomSeed = ::UnPack(randomSeed);
  g->debrisCount = ::UnPack(debrisCount);
  g->colonyPending = ::UnPack(colonyPending);
  g->pluginAction = ::UnPack(pluginAction);
  g->pluginPar = ::UnPack(pluginPar);

  g->totPop = ::UnPack(totPop);

  g->lightSpeed = ::UnPack(lightSpeed);

  strcpy(g->scenarioName, scenarioName);
  strcpy(g->scenarioDesc, scenarioDesc);

  // set object pointers in packed game space
  ShipClass* upShip = (ShipClass*) (this + 1);
  PlanetClass* upPlanet = (PlanetClass*) (upShip + g->numShips);
  MissileClass* upMissile = (MissileClass*) (upPlanet + g->numPlanets);
  MissileClass** upMissileList = (MissileClass**) (upMissile + g->numMissiles);
  GameBitmapClass* upGbmp = (GameBitmapClass*) (upMissileList +
						g->numMissiles);
  ShipDesignStruct* upsd = 
    (ShipDesignStruct*) (upGbmp + g->numBitmaps);
  MissileDesignStruct* upmd = (MissileDesignStruct*) 
    (upsd + g->numShipDesigns);
  ModuleClass* upmc = (ModuleClass*) (upmd + g->numMissileDesigns);
  ModuleClass* uplmc = (ModuleClass*) (upmc + g->numModules);
  RaceClass* upr = (RaceClass*) (uplmc + g->numLoadedModules);
  PlayerClass* upl = (PlayerClass*) (upr + g->numRaces);
  PlayerClass** uapl = (PlayerClass**) (upl + g->maxStoredPlayers);
  RankStruct* upra = (RankStruct*) (uapl + g->numShips);
  ColonyClass* uco = (ColonyClass*) (upra + g->numRanks);
  ColonyClass** ucoList = (ColonyClass**) (uco + g->numColonies);
  ColonyDesignStruct* ucds = (ColonyDesignStruct*) (ucoList + g->numColonies);
  ListStruct* uls = (ListStruct*) (ucds + g->numColonyDesigns);

  // unpack object list pointers
// this information is needed for host and target mapping so it must
// precede the unpacking of all objects                             
  short mI; // missile index
  short cI; // colony index
  short i;
  for (i = 0; i < g->numMissiles; i++) {
    mI = ::UnPack(long(upMissileList[i]));
    g->missileList[i] = g->missile + mI;
  }
  for (i = 0; i < g->numColonies; i++) {
    cI = ::UnPack(long(ucoList[i]));
    g->colonyList[i] = g->colony + cI;
  }

  // now that the missile index list is across we can map the object list
  g->MapObjectListFromMissileList();
  //g->ProfileObjectList();

  //  printf("unpacking ships...");
  ShipClass s;
  for (i = 0; i < g->numShips; i++) {
    s = upShip[i];
    short t = ::UnPack(s.targetPP.obj.objLong);
    short lt = ::UnPack(s.lastAttacker.objLong);
    s.UnPack(g);
    g->ship[i] = s;
  }
  //  printf("done.\n");

  PlanetClass p;
//  printf("unpacking planets...");
  for (i = 0; i < g->numPlanets; i++) {
    p = upPlanet[i];
    p.UnPack(g);
    g->planet[i] = p;
  }
//  printf("done.\n");

  MissileClass m;
  for (i = 0; i < g->numMissiles; i++) {
    m = upMissile[i];
    m.UnPack(g);
    *g->missileList[i] = m;
  }

  GameBitmapClass gbmp;
  for (i = 0; i < g->numBitmaps; i++) {
    gbmp = upGbmp[i];
    gbmp.UnPack();
    g->objectBitmapList[i] = gbmp;
  }
  
  ShipDesignStruct sd;
  for (i = 0; i < g->numShipDesigns; i++) {
    sd = upsd[i];
    sd.UnPack();
    g->shipDesign[i] = sd;
  }

  MissileDesignStruct md;
  for (i = 0; i < g->numMissileDesigns; i++) {
    md = upmd[i];
    md.UnPack();
    g->missileDesign[i] = md;
  }

  ModuleClass mc;
  for (i = 0; i < g->numModules; i++) {
    mc = upmc[i];
    mc.UnPack(g);
    g->moduleList[i] = mc;
  }

  for (i = 0; i < g->numLoadedModules; i++) {
    mc = uplmc[i];
    mc.UnPack(g);
    g->loadedModuleList[i] = mc;
  }

  RaceClass r;
  for (i = 0; i < g->numRaces; i++) {
    r = upr[i];
    r.UnPack(g);
    g->race[i] = r;
  }

  PlayerClass pl;
  for (i = 0; i < g->maxStoredPlayers; i++) {
    pl = upl[i];
    pl.UnPack();
    g->player[i] = pl;
  }

// unpack playerList array
  for (i = 0; i < g->numShips; i++) {
    g->playerList[i] = g->player + ::UnPack(long(uapl[i]));
  }

  RankStruct ra;
  for (i = 0; i < g->numRanks; i++) {
    ra = upra[i];
    ra.UnPack();
    g->rank[i] = ra;
  }

  ColonyClass co;
  for (i = 0; i < g->numColonies; i++) {
    co = uco[i];
    co.UnPack(g);
    *g->colonyList[i] = co;
  }

  ColonyDesignStruct cds;
  for (i = 0; i < g->numColonyDesigns; i++) {
    cds = ucds[i];
    cds.UnPack();
    g->colonyDesign[i] = cds;
  }

  ListStruct cls;
  for (i=0; i < g->numRaces; i++) {
    cls = uls[i];
    cls.list = ((char*) (uls + g->numRaces)) + i * g->numModules;
    cls.UnPack();
    g->tech[i].copy(cls);
cls.list = 0; // to prevent destructor from attacking shared memory 
  }

  //g->PlugInModules();

  //g->UpdateMissileList();
  g->MapObjectList();

  //g->ProfileObjectList();
						
  g->DisperseLoadedModuleList();
  g->DestroyLoadedModuleList();

  g->Profile();

  g->activate();

  return g;
}*/

void GameStateClass::InitializeTechList() {
  short i;
  for (i=0; i<numRaces; i++) {
    tech[i].ReInitializeWith(numModules);
  }

  for (i=0; i<numModules; i++)
    tech[moduleList[i].raceID].set(i);
}

slong ShipDesignStruct::PushTo(class GameStateBuffer& gb, GameStateClass* g) {

  slong subClassSize = DesignStruct::PushTo(gb, g);

  char* bufferPos = gb.BufferPosition();

  CALL_PUSH_OBJ(gb,inFrame);
  CALL_PUSH_OBJ(gb,reqRank);
  CALL_PUSH_OBJ(gb,factory);

  return subClassSize + (gb.BufferPosition() - bufferPos);

}

slong ShipDesignStruct::PopFrom(class GameStateBuffer& gb, GameStateClass* g) {

  slong subClassSize = DesignStruct::PopFrom(gb, g);

  char* bufferPos = gb.BufferPosition();

  CALL_POP_OBJ(gb,inFrame);
  CALL_POP_OBJ(gb,reqRank);
  CALL_POP_OBJ(gb,factory);

  return subClassSize + (gb.BufferPosition() - bufferPos);
}

slong BusIdStruct::PushTo(class GameStateBuffer& gb, GameStateClass* g) {
  
  char* bufferPos = gb.BufferPosition();

  CALL_PUSH_OBJ(gb,numSlots);
  CALL_PUSH_OBJ(gb,numMod);
  short i,l;
  for (i = 0; i < max_module_list; i++) {
    CALL_PUSH_OBJ(gb,moduleIdList[i]);
    for (l = 0; l < max_module_spar; l++) {
      CALL_PUSH_OBJ(gb,moduleParList[i*max_module_spar+l]);
    }
  }

  return gb.BufferPosition() - bufferPos;

}

slong BusIdStruct::PopFrom(class GameStateBuffer& gb, GameStateClass* g) {
  char* bufferPos = gb.BufferPosition();

  CALL_POP_OBJ(gb,numSlots);
  CALL_POP_OBJ(gb,numMod);
  short i,l;
  for (i = 0; i < max_module_list; i++) {
    CALL_POP_OBJ(gb,moduleIdList[i]);
    for (l = 0; l < max_module_spar; l++) {
      CALL_POP_OBJ(gb,moduleParList[i*max_module_spar+l]);
    }
  }

  return gb.BufferPosition() - bufferPos;
}

void BusStruct::RepairSystems(short repairRate, short bypass) {
  if ( check != 0xAAAAAAAA ) return;
  if ( numMod == 0 )
      return;
  short modNum = (short)(frameNumber % numMod);
  slong negDam = -repairRate;
  
  if (moduleList + modNum == armorMod) {
    if (bypass)
      /* cut armor repair rate by factor of three */
      negDam = -repairRate;
    else
      return;
  }
  
  moduleList[modNum].Hit(negDam);
}

slong BusStruct::PushTo(class GameStateBuffer& gb, GameStateClass* g) {
  if ( check != 0xAAAAAAAA ) return 0;
  char* bufferPos = gb.BufferPosition();

  /* push the number of slot first so we can build the structure first */
  CALL_PUSH_OBJ(gb,numSlots);

  CALL_PUSH_OBJ(gb,modPtr);
  CALL_PUSH_OBJ(gb,msg);
  CALL_PUSH_OBJ(gb,msgPriority);
  CALL_PUSH_OBJ(gb,numMod);
  CALL_PUSH_OBJ(gb,numPorts);
  CALL_PUSH_OBJ(gb,numPowerPlants);
  CALL_PUSH_OBJ(gb,numTubes);
  CALL_PUSH_OBJ(gb,numBeams);
  CALL_PUSH_OBJ(gb,powerIn);
  CALL_PUSH_OBJ(gb,powerOut);
  CALL_PUSH_OBJ(gb,powerRate);
  CALL_PUSH_OBJ(gb,aveFunc);

  short i;
  for (i=0; i<numMod; i++) {
    CALL_PUSH_GAME_OBJ(gb, g,moduleList[i]);
  }
  
  return gb.BufferPosition() - bufferPos;
}

slong BusStruct::PopFrom(class GameStateBuffer& gb, GameStateClass* g) {
  if ( check != 0xAAAAAAAA ) return 0;
  char* bufferPos = gb.BufferPosition();

  CALL_POP_OBJ(gb,numSlots);
  InitializeWith(numSlots);

  CALL_POP_OBJ(gb,modPtr);
  CALL_POP_OBJ(gb,msg);
  CALL_POP_OBJ(gb,msgPriority);
  CALL_POP_OBJ(gb,numMod);
  CALL_POP_OBJ(gb,numPorts);
  CALL_POP_OBJ(gb,numPowerPlants);
  CALL_POP_OBJ(gb,numTubes);
  CALL_POP_OBJ(gb,numBeams);
  CALL_POP_OBJ(gb,powerIn);
  CALL_POP_OBJ(gb,powerOut);
  CALL_POP_OBJ(gb,powerRate);
  CALL_POP_OBJ(gb,aveFunc);

  short i;
  for (i=0; i<numMod; i++)
    CALL_POP_GAME_OBJ(gb, g, moduleList[i]);

  return gb.BufferPosition() - bufferPos;
}

void BusStruct::MapPorts() {
  if ( check != 0xAAAAAAAA ) return;
  /* allocate space */

  short i;
    
    short jD = 0;
    short jT = 0;
    short jP = 0;
  short jB = 0;
  for (i = 0; i < numMod; i++) {
    if (moduleList[i].moduleFunc == ModuleClass::dock) {
      dockMod[jD] = moduleList + i;
      jD++;
    } else if (moduleList[i].moduleFunc == ModuleClass::tube ||
	       moduleList[i].moduleFunc == ModuleClass::beam ||
	       moduleList[i].moduleFunc == ModuleClass::ram) {
      tubeMod[jT] = moduleList + i;
      jT++;

      if (moduleList[i].moduleFunc == ModuleClass::beam) {
	beamMod[jB] = moduleList + i;
	jB++;
      }

    } else if (moduleList[i].moduleFunc == ModuleClass::power_plant ||
	       moduleList[i].moduleFunc == ModuleClass::fuel_cell ||
	       moduleList[i].moduleFunc == ModuleClass::battery) {
      powerMod[jP] = moduleList + i;
      jP++;
    }
  }
}


slong ModuleClass::StdPowerStore() {
  if ( check != 0xAAAAAAAA ) return 0;
  return store*100/labs(power);
}

slong ModuleClass::StdPowerReq() {
  if ( check != 0xAAAAAAAA ) return 0;
  return -(power+store);
}

slong ModuleClass::CapPowerReq() {
  if ( check != 0xAAAAAAAA ) return 0;
  /* take power in at rate set by power */
  if (moduleFunc != tube || workPar[0])
    return max(min(labs(power*fPar(0))-store, labs(power) /*par[0]*/), 0);
  else
    return 0;
    
}

slong ModuleClass::CapPowerStore() {
  if ( check != 0xAAAAAAAA ) return 0;
  /* store given by throughput * frame store */
  if (power && fPar(0))
    return store*100/labs(power * par[0] /**fPar(0)*/);
  else
    return 0;
}


void BusStruct::FreeModuleList() {
  if ( check != 0xAAAAAAAA ) return;
  moduleList.destroy();

  modPtr = 0;
  /* single module pointers */
  ClearSingleModuleMaps();
  /* module list pointers */
  numMod = 0;
  numSlots = 0;
  numTubes = 0;
  numBeams = 0;
  numPorts = 0;
  numPowerPlants = 0;
}

void BusStruct::MapFunctions(DesignedClass* d, DesignedClass* t) {
  if ( check != 0xAAAAAAAA ) return;
  numPorts = 0;
  numTubes = 0;
  numBeams = 0;
  numPowerPlants = 0;
  
  ClearSingleModuleMaps();

  short i;
  for (i = 0; i < numMod; i++) {
    /* set bus host pointer */
    moduleList[i].host.designed = d;
    moduleList[i].withdraw = &ModuleClass::WithdrawStdPower;
    moduleList[i].deposit = &ModuleClass::DepositStdPower;
    moduleList[i].requirement = &ModuleClass::StdPowerReq;
    moduleList[i].storage = &ModuleClass::StdPowerStore;
    if (moduleList[i].moduleFunc == ModuleClass::engine) {
      impulseMod = moduleList + i;
      moduleList[i].fire = &ModuleClass::Thrust;
    } else if (moduleList[i].moduleFunc == ModuleClass::qdrv) {
      warpMod = moduleList + i;
      moduleList[i].fire = &ModuleClass::Warp;
    } else if (moduleList[i].moduleFunc == ModuleClass::brake) {
      brakeMod = moduleList + i;
      moduleList[i].fire = &ModuleClass::Brake;
    } else if (moduleList[i].moduleFunc == ModuleClass::tube ||
	       moduleList[i].moduleFunc == ModuleClass::beam ||
	       moduleList[i].moduleFunc == ModuleClass::ram) {
      moduleList[i].requirement = &ModuleClass::CapPowerReq;
      moduleList[i].storage = &ModuleClass::CapPowerStore;
      moduleList[i].deposit = &ModuleClass::DepositCapPower;
      moduleList[i].fire = &ModuleClass::FireTube;
      numTubes++;

      if (moduleList[i].moduleFunc == ModuleClass::beam)
	numBeams++;

    } else if (moduleList[i].moduleFunc == ModuleClass::power_plant) {
      moduleList[i].withdraw = &ModuleClass::WithdrawPower;
      moduleList[i].deposit = &ModuleClass::DepositXSPower;
      moduleList[i].storage = &ModuleClass::CapPowerStore;
      moduleList[i].requirement = &ModuleClass::CapPowerReq;
      numPowerPlants++;
    } else if (moduleList[i].moduleFunc == ModuleClass::fuel_cell) {
      moduleList[i].withdraw = &ModuleClass::WithdrawFuel;
      moduleList[i].deposit = &ModuleClass::DepositFuel;
      numPowerPlants++;
    } else if (moduleList[i].moduleFunc == ModuleClass::battery) {
      moduleList[i].withdraw = &ModuleClass::WithdrawCharge;
      moduleList[i].deposit = &ModuleClass::DepositCharge;
      numPowerPlants++;
    } else if (moduleList[i].moduleFunc == ModuleClass::shield) {
      shieldMod = moduleList + i;
      moduleList[i].requirement = &ModuleClass::CapPowerReq;
      moduleList[i].storage = &ModuleClass::CapPowerStore;
      moduleList[i].deposit = &ModuleClass::DepositCapPower;
    } else if (moduleList[i].moduleFunc == ModuleClass::sensor) {
      sensorMod = moduleList + i;
      /* *** fix me -- the module should always be active */
    } else if (moduleList[i].moduleFunc == ModuleClass::counter_measures) {
      scrambleMod = moduleList + i;
      moduleList[i].requirement = &ModuleClass::CapPowerReq;
      moduleList[i].storage = &ModuleClass::CapPowerStore;
      moduleList[i].deposit = &ModuleClass::DepositCapPower;
      moduleList[i].fire = &ModuleClass::Scramble;
    } else if (moduleList[i].moduleFunc == ModuleClass::cloak) {
      stealthMod = moduleList + i;
      moduleList[i].requirement = &ModuleClass::CapPowerReq;
      moduleList[i].storage = &ModuleClass::CapPowerStore;
      moduleList[i].deposit = &ModuleClass::DepositCapPower;
      moduleList[i].fire = &ModuleClass::Hide;
    } else if (moduleList[i].moduleFunc == ModuleClass::boost) {
      boostMod = moduleList + i;
      moduleList[i].requirement = &ModuleClass::CapPowerReq;
      moduleList[i].storage = &ModuleClass::CapPowerStore;
      moduleList[i].deposit = &ModuleClass::DepositCapPower;
      moduleList[i].fire = &ModuleClass::Boost;
    } else if (moduleList[i].moduleFunc == ModuleClass::dock) {
      moduleList[i].requirement = &ModuleClass::CapPowerReq;
      moduleList[i].storage = &ModuleClass::CapPowerStore;
      moduleList[i].deposit = &ModuleClass::DepositCapPower;
      moduleList[i].fire = &ModuleClass::Dock;
      numPorts++;
    } else if (moduleList[i].moduleFunc == ModuleClass::factory) {
      factoryMod = moduleList + i;
      moduleList[i].fire = &ModuleClass::Build;
    } else if (moduleList[i].moduleFunc == ModuleClass::armor) {
      armorMod = moduleList + i;
      moduleList[i].fire = &ModuleClass::Block;
    }
  }
  MapPorts();

}

void BusStruct::RemapFunctions(DesignedClass* d, DesignedClass* t, char* reason) {
  if ( check != 0xAAAAAAAA ) return;
  ShipClass* s;

  MapFunctions(d, t);

  /* if there is somehow (it shouldn't happen) an object attached to the */
  /* dock, then reassign it's port value                                 */
  short i;
  for (i=0; i<numPorts; i++) {
    if (dockMod[i]->workPar[1] && dockMod[i]->workPar[0]) {
      s = ((ShipClass*)dockMod[i]->workPar[0]);
      if (s->portNum != moduleList.indexOf(dockMod[i])) {
	
	logFile << "WARNING> port " << moduleList.indexOf(dockMod[i]) << ", holding " << 
	  s->TypeName() << " " <<
	  game->TypeListIndex(s) <<
	  ", changed its index during a remap, triggered by " << reason << ", for a ";
	dockMod[i]->host.generic->ShortProfile();
	logFile << " on frame " << game->frame << endl;

	s->portNum = moduleList.indexOf(dockMod[i]);

      }
    }
  }
}
  
void ModuleClass::initialize() {
  check = 0xAAAAAAAA;
  checkSum = 0;
  active = 0;
  store = 0;
  frame = 0;
  fire = 0;
  mass = 0;
  output = 0;
  volume = 0;
  damage = 0;
  damageCap = 0;
  mode = standby;
  func = module_function_scale;
  host.objLong = 0;
  debrisBitmapIndex = 0;
  activationSoundIndex = -1;
  deactivationSoundIndex = -1;
  withdraw = &ModuleClass::WithdrawPower;
  deposit = &ModuleClass::DepositStdPower;
  requirement = &ModuleClass::StdPowerReq;
  short i;
  for (i = 0; i < max_module_par; i++) {
    par[i] = 0;
  }
  for (i = 0; i < max_module_spar; i++) {
    specPar[i] = 0;
  }
  for (i = 0; i < max_module_wpar; i++) {
    workPar[i] = 0;
  }
  nPar = 0;
  nSPar = 0;
  nWPar = 0;
  checkSum = CheckSum();
}

void ModuleClass::profile(GameStateClass* g) {
  if ( check != 0xAAAAAAAA ) return;
  logFile << "Module Profile: id:         " << id << endl;
  logFile << "             power:         " << power << endl;
  logFile << "            output:         " << output << endl;
  logFile << "             store:         " << store << endl;
  logFile << "         damageCap:         " << damageCap << endl;
  logFile << "            damage:         " << damage << endl;
  short i;
  for( i = 0; i < 4; i++)
    logFile << "            par[" << i <<"]:         " << par[i] << endl;
  if (host.generic->Type() == ship)
    logFile << "              host:         ship " << 
      g->ship.indexOf(host.ship) << endl;
  else if (host.generic->Type() == colony) {
    logFile << "              host:         colony ";
    i=0;
    while(g->colonyList[i] != host.colony && i < g->numColonies)
      i++;
    if (i < g->numColonies)
      logFile << i << endl;
    else
      logFile << "<unknown>" << endl;
  } else if (host.generic->Type() == missile) {
    logFile << "              host:         missile ";
    i=0;
    while(g->missileList[i] != host.missile && i < g->numMissiles)
      i++;
    if (i < g->numMissiles)
      logFile << i << endl;
    else
      logFile << "<unknown>" << endl;
  }
  logFile << "              func:         " << func << endl;
  logFile << "        moduleFunc:         " << moduleFunc << endl;
  logFile << "            active:         " << active << endl;
  logFile << "             frame:         " << frame << endl;
  logFile << "              fire:         " ;

  if (fire == &ModuleClass::Thrust)
    logFile << "Thrust()" << endl;
  else if (fire == &ModuleClass::Warp)
    logFile << "Warp()" << endl;
  else if (fire == &ModuleClass::Brake)
    logFile << "Brake()" << endl;
  else if (fire == &ModuleClass::FireTube)
    logFile << "FireTube()" << endl;
  else if (fire == &ModuleClass::Scramble)
    logFile << "Scramble()" << endl;
  else if (fire == &ModuleClass::Hide)
    logFile << "Hide()" << endl;
  else if (fire == &ModuleClass::Boost)
    logFile << "Boost()" << endl;
  else if (fire == &ModuleClass::Dock)
    logFile << "Dock()" << endl;
  else if (fire == &ModuleClass::Build)
    logFile << "Build()" << endl;
  else if (fire == &ModuleClass::Block)
    logFile << "Block()" << endl;
  else
    logFile << "<unknown>" << endl;

  logFile << "           storage:         " ;
  if (storage == &ModuleClass::StdPowerStore)
    logFile << "StdPowerStore()" << endl;
  else if (storage == &ModuleClass::CapPowerStore)
    logFile << "CapPowerStore()" << endl;
  else
    logFile << "<unknown>" << endl;

  logFile << "       requirement:          " ;
  if (requirement == &ModuleClass::StdPowerReq)
    logFile << "StdPowerReq()" << endl;
  else if (requirement == &ModuleClass::CapPowerReq)
    logFile << "CapPowerReq()" << endl;
  else
    logFile << "<unknown>" << endl;
  
  logFile << "          withdraw:          " ;
  if (withdraw == &ModuleClass::WithdrawFuel)
    logFile << "WithdrawFuel()" << endl;
  else if (withdraw == &ModuleClass::WithdrawPower)
    logFile << "WithdrawPower()" << endl;
  else if (withdraw == &ModuleClass::WithdrawStdPower)
    logFile << "WithdrawStdPower()" << endl;
  else if (withdraw == &ModuleClass::WithdrawCharge)
    logFile << "WithdrawCharge()" << endl;
  else
    logFile << "<unknown>" << endl;

  logFile << "           deposit:          " ;
  if (deposit == &ModuleClass::DepositFuel)
    logFile << "DepositFuel()" << endl;
  else if (deposit == &ModuleClass::DepositCapPower)
    logFile << "DepositCapPower()" << endl;
  else if (deposit == &ModuleClass::DepositStdPower)
    logFile << "DepositStdPower()" << endl;
  else if (deposit == &ModuleClass::DepositXSPower)
    logFile << "DepositXSPower()" << endl;
  else if (deposit == &ModuleClass::DepositCharge)
    logFile << "DepositCharge()" << endl;
  else
    logFile << "<unknown>" << endl;
}


slong ModuleClass::TubeEnergyCost()
{

  if ( check != 0xAAAAAAAA )
    {
      return 0;
    }

  short missileIndex = (short)par[3];
  if ( missileIndex < 0 || missileIndex >= game->numMissileDesigns )
    {
      return 0;
    }

  if ( game->missileDesign[missileIndex].trigger != on_proximity )
    {
      return par[1] + 
	game->missileDesign[missileIndex].warhead *
	energy_to_damage / 100;
    }
  else
    {
      return par[1] + 
	game->missileDesign[missileIndex].warhead *
	energy_to_prox_damage / 100 *
	game->objectBitmapList[game->missileDesign[missileIndex].expBitmapIndex].numFrames / 4;
    }
}


slong ModuleClass::value() {
  if ( check != 0xAAAAAAAA ) return 0;

  short missileIndex;

  if ( IsValidObjectPointer(host.generic,__FILE__,__LINE__) && 
       host.generic->Type() == ship && game->player[host.ship->playerIndex].IsRestrictedFromUsing(this))
    return 0;
  
  switch (moduleFunc) {
  case factory: /* scaled */
    return 2000 * par[0];
  break;
  case power_plant: /* scaled */
    return 1000*power;
  break;
  case engine: /* scaled */
    return par[0] * 1000;
  break;
  case qdrv: /* scaled */
    return (par[0] + par[1]) * 20;
  break;
  case tube:
    missileIndex = (short)par[3];
    return (4*TubeBurst() + slong(30*TubeRepeatRate(frameRate)))*
      game->missileDesign[missileIndex].warhead *
      (1 + game->missileDesign[missileIndex].tracking) / sub_scale * 
      game->missileDesign[missileIndex].initialVelocityDisplacement *
      game->missileDesign[missileIndex].maxSpeed;
  break;
  case ram:
    missileIndex = (short)par[3];
    return (slong(20*TubeRepeatRate(frameRate))) *
      (game->missileDesign[missileIndex].warhead + game->lightSpeed / vel_to_damage);
  break;
  case beam:
    return (par[1] * par[1] / 100 * par[1] / 1000) *
      (par[2] * par[2] / 100 * par[2] / 100) / 20 * par[0];
  break;
  case shield: /* scaled */
    return -power*par[0]*2000;
  break;
  case brake: /* scaled */
    return (100-par[0]) * 5;
  break;
  case sensor: /* scaled */
    return par[0] * 20000;
  break;
  case fuel_cell: /* scaled */
    return power * par[0] * 1000;
  break;
  case battery: /* scaled */
    return power * par[0] * 1000;
  break;
  case counter_measures: /* scaled */
    return par[1] * 3000;
  break;
  case cloak: /* scaled */
    return par[1] * 500 - power*par[0]*10;
  break;
  case boost: /* scaled */
    return -power * par[0] / 100 * par[1] * par[2];
  break;
  case dock: /* scaled */
    return -power * par[0] * par[1] / 50000 * par[2];
  break;
  case armor: /* scaled */
    return par[0] * 100000;
  break;
  }
  return 0;
}

void ModuleClass::Thrust() {

  static CoordinateClass ovel;

  host.designed->CheckTransition(nullVector, 1);

  if (!host.designed->IsTransitioning()) {

    /* set the throttle flag */
    if ( IsLocalPlayer(host.ship) ) /* dlh */
      host.designed->throttle = 1;
    
    /* apply thrust to designed object [host] */

    slong thr;
    slong af = host.designed->mass*100/frameRate/frameRate;
    if (host.designed->bus.boostMod && host.designed->bus.boostMod->active)
      thr = (10000 + host.designed->bus.boostMod->cPar(2, host.designed->bus.CalculateSurplusPower())) *
	host.designed->bus.impulseMod->afMaxPar(af,0) / 10000;
    else
      thr = aPar(0);

    /******************************************************************/
    /* do not allow the acceleration to exceed the game state maximum */
    /*                             and                                */
    /* do not allow acceleration to fall below the prescribed minimum */
    /******************************************************************/
    thr = max(min(thr, game->maxAcc * af/100), 0);

    ovel = host.generic->vel;

    host.generic->
      ApplyThrust(host.generic->bitmap.Dir(host.designed->heading+
					    host.designed->thrustOffset) *
		  thr, host.designed->maxSpeed);
    
    slong pct = (host.generic->vel-ovel).Mag() * 100 /
      max(thr*sub_scale / max(host.generic->mass, 1) * dt,1);
    SetStat(store,max(store + power * pct / 100, 0L));
    //store = max(store + power * pct / 100, 0L);
    //store = 0;
  }
}

void ModuleClass::Warp() {
  if ( check != 0xAAAAAAAA ) return;
  /* apply warp to designed object [host] */
  if (host.designed->IsWarping()) {
    if (!host.designed->IsTransitioning() &&
	host.generic->mass < par[1]) {
      host.generic->vel =
	(game->objectBitmapList[host.generic->
				originalBitmapIndex].
	 Dir(host.designed->heading+host.designed->thrustOffset) * 
	 aPar(0) / sub_scale) ;
      WithdrawStdPower();

      if (game->soundEvent[warping_event] != -1)
	host.generic->PlaySoundOnScreen(game->soundEvent[warping_event], 1);

    }
  } else
    host.designed->WarpTransition();
}

void ModuleClass::Brake() {
  /* apply brake to [host] object */
  host.generic->vel *= aPar(0) / 100.0;
  host.designed->CheckTransition(nullVector);
  SetStat(store,0);
  //store = 0;
}

void ModuleClass::Scramble() {
  if ( check != 0xAAAAAAAA ) return;
  /* scramble missiles within range [par[1]] of host */
  static char msg[1024];

  /* automatically deactivate() */
  active = 0;

  slong cost = -power * par[0], surplus, overrun;

  if (store >= cost) {
    SetStat(store,0);
    //store = 0;
    surplus = 0;
  } else {
    surplus = host.designed->bus.CalculateSurplusPower();
    overrun = cost - store;
    if (surplus >= overrun) {
      host.designed->bus.WithdrawSurplusPower(overrun);
      SetStat(store,0);
      //store = 0;
    } else
      return; /* insufficient power -- do nothing */
  }

  host.designed->PlaySoundOnScreen(activationSoundIndex);

  short numMissiles = 0;
  slong dist;
  short i;
  for (i=0; i<game->numMissiles; i++)
    {
      if ( IsValidObjectPointer(game->missileList[i]) )
	{
	  if (game->missileList[i]->design->tracking == smart) {
	    dist = (*game->missileList[i] - *host.generic).Mag() /
	      sub_scale;
	    if (dist < fPar(1)) {
	      game->missileList[i]->raceID = host.designed->raceID;
	      game->missileList[i]->parent.designed = host.designed;
	      game->missileList[i]->Acquire();
	      
	      numMissiles++;
	    }
	  }
	}
      SetStat(store,0);
      //store = 0;
    }
  
  sprintf(msg, "1st Officer> %d missiles scrambled.", numMissiles);
  game->PostGameMessage(msg, game->ship.indexOf(host.ship), (short)host.generic->raceID,
			  personal);
}

void ModuleClass::Hide() {
  if ( check != 0xAAAAAAAA ) return;
  /* [host] object lowers detection threshold */
  slong cost = -2 * power, surplus;

  if (store >= cost) {
    AddToStat(store,-cost);
    //store -= cost;
    surplus = store;
  } else {
    host.designed->bus.WithdrawSurplusPower(cost - store);
    SetStat(store,0);
    //store = 0;
    surplus = host.designed->bus.CalculateSurplusPower();
  }
  if (surplus) {
    if (host.ship->stealthLevel < max_stealth_level) {
      if (frame % 2 == 0)
	host.ship->stealthLevel++;
      host.designed->stealth = 1;
    } else
      host.designed->stealth = (short)max(cPar(1, host.designed->bus.CalculateSurplusPower()),1);
  } else {
    host.designed->stealth = 1;
    host.ship->stealthLevel = 0;

    /* the stealth system has powered down -- send an UnHide message */
    if (host.generic->Type() == ship &&
	(IsConnectedClient() && IsLocalPlayer(host.ship)) ||
	(IsServer() && game->IsRobotPlayer(host.ship)))
      SendUnHideMsg();

    deactivate();
  }
}

void ModuleClass::Dock() {
  if ( check != 0xAAAAAAAA ) return;
  /* tractor the target object into position */
  static CoordinateClass offset;
  static PingPacket thisPP;
  slong surplus, cost = -2 * power;

  if (store >= cost) {
    AddToStat(store,-cost);
    //store -= cost;
    surplus = store;
  } else {
    //host.designed->bus.WithdrawSurplusPower(cost - store);
    //store = 0;
    //surplus = host.designed->bus.CalculateSurplusPower();
    surplus = 0;
  }

  ShipClass* s = (ShipClass*)workPar[0];
  if ( !IsValidObjectPointer(s) )
    {
      s = NULL;
    }

  if ( IsInInternetCafe(s) || IsInInternetCafe(host.designed) )
    {
      workPar[0] = 0;
      workPar[1] = 0;
      return;
    }

  if (!surplus || !s || s && (s->IsExploding() ||
			      s->IsWarping() ||
			      s->stealth > 1 ||
			      s->dockObj ||
			      (s->Type() == colony &&
			       !s->IsFriendlyTo(host.ship))) ||
      host.ship->stealth > 1 ||
      host.designed->IsWarping()) {
    
    if (s && workPar[1])
      s->FreeFromDock();
    else {
      workPar[0] = 0;
      WRITE_SF_LOG(logFile << "note> " <<
		   host.generic->TypeName() << " " << game->TypeListIndex(host.generic) <<
		    "/" << game->ObjectListIndex(host.generic) << " deactivated port " <<
		   this-host.designed->bus.moduleList << " because ");
      WRITE_SF_LOG(
		   if (!surplus)
		   logFile << " it ran out of power";
		   else if (!s)
		   logFile << " it did not have a target";
		   else if (s->IsExploding())
		   logFile << " the target was exploding";
		   else if (s->IsWarping())
		   logFile << " the target had engaged q drive";
		   else if (s->stealth > 1)
		   logFile << " the target had activated stealth";
		   else if (s->dockObj)
		   logFile << " the target was already docked";
		   else if (host.ship->stealth > 1)
		   logFile << " the host ship had activated stealth";
		   else if (host.ship->IsWarping())
		   logFile << " the host ship had engaged q drive";
		   else
		   logFile << " of an unknown reason";
		   logFile << " on frame " << game->frame << endl;
		   ) {
	/* dock deactivation */
	deactivate();
	host.ship->ClearPendingFlag(ShipClass::tractoring);
      }
    }
    
    /* the docking system has powered down -- send a DockDeactivate message */
    if (host.generic->Type() == ship &&
	((IsConnectedClient() && IsLocalPlayer(host.ship)) ||
	 (IsServer() && game->IsRobotPlayer(host.ship)))) {

      if (!surplus)
	SendDockDeactivationMsg("it ran out of power.");
      else if (!s)
	SendDockDeactivationMsg("it didn't have a target.");
      else if (s->IsExploding())
	SendDockDeactivationMsg("the target exploded.");
      else if (s->IsWarping())
	SendDockDeactivationMsg("the target engaged q drive.");
      else if (s->stealth > 1)
	SendDockDeactivationMsg("the target engaged stealth system.");
      else if (s->dockObj)
	SendDockDeactivationMsg("the target is already docked.");
      else if (s->Type() == colony && !s->IsFriendlyTo(host.ship))
	SendDockDeactivationMsg("the target is a nonfriendly colony.");
      else if (host.ship->stealth > 1)
	SendDockDeactivationMsg("the host ship engaged stealth system.");
      else if (host.generic->Type() >= designed &&
	       host.designed->IsWarping())
	SendDockDeactivationMsg("the host ship engaged q drive.");
      else 
	SendDockDeactivationMsg("<unknown>");
    }

  } else if (!workPar[1] && s && s->bus.impulseMod) {

    ulong dockRange;
    slong dockStrength;
    if (s && s->IsFriendlyTo(host.ship)) {
      dockRange = colony_dock_range;
      dockStrength = 10;
    }
    else {
      dockRange = ship_dock_range;
      dockStrength = 1;
    }

    slong h = specPar[1];
    if (!h)
      h = host.generic->bitmap.size / 2;

    if ( s )
      {
	if (host.generic->bitmap.IsTime() &&
	    host.generic->bitmap.numFrames < game->maxAngles) {
	  slong heading = (frameNumber / host.generic->bitmap.numFrames) %
	    (game->maxAngles % host.generic->bitmap.numFrames) *
	    host.generic->bitmap.numFrames + host.generic->offset;
	  offset = host.generic->
	    bitmap.dir[(heading + specPar[0] + 
			2*game->maxAngles) % game->maxAngles] *
	    (h + s->bitmap.size/2);
	} else
	  offset = host.generic->
	    bitmap.dir[(host.designed->heading + specPar[0] + 
			2*host.generic->bitmap.numFrames) % 
		      host.generic->bitmap.numFrames] *
	    (h + s->bitmap.size/2);
	thisPP = s->Ping(host.generic->TType(),
			 specific, game->TypeListIndex(host.ship),
			 pc_long_max,
			 offset);
      }
    if (thisPP.dist < (ulong) par[2] * usub_scale) {

      if (frame == 1)
	host.designed->PlaySoundOnScreen(activationSoundIndex);      

      slong af = host.designed->mass*100 / frameRate / frameRate;
      slong thr = max(min(max(par[1]*dockStrength, min_acc * af), 
                             game->maxAcc * af) * sub_scale / 100, 0);

      if ( s )
	{
	  s->SeekThrust(thisPP, 250, 10, 0, thr, 0);
	}
      host.designed->seekTurning = 0;
    } else { /* the target is out of range -- deactivate */
      if (host.generic->Type() == ship &&
	  (IsDisconnectedClient() || IsConnectedClient() && IsLocalPlayer(host.ship) ||
	  IsServer() && game->IsRobotPlayer(host.ship)))
	SendDockDeactivationMsg("the target moved out of range.");
    }
    /* if the target is unfriendly it attacks and calls for help */
    if (s && !s->IsFriendlyTo(host.generic) && s->AutoResponse()) {
      s->AttackObj(host.generic);
      s->SendDistressSignal();
    }

    if (s && thisPP.dist < dockRange) {
      //AssignPortToShip(s);
      
      /* the docking system has successfully docked the target object */
      /* send a DockingSuccessful message */
      if (IsDisconnectedClient() ||
	  IsConnectedClient() &&
	  (IsLocalPlayer(s) ||
	   (IsLocalPlayer(host.ship) && game->IsRobotPlayer(s))) ||
	  (IsServer() && (game->IsRobotPlayer(s) &&
			  game->IsRobotPlayer(host.ship))))
	SendDockingSuccessfulMsg();
      
    }
  }
}

void ModuleClass::SendHideMsg() {
  if ( check != 0xAAAAAAAA ) return;
  if (!host.ship->IsPending(ShipClass::disappearing)) {

      host.ship->SetPendingFlag(ShipClass::disappearing);
  retMsgBuffer.PushMsg((uchar)SF_MSG_CLOAK,
		       (ushort)game->ObjectListIndex(host.generic),
		       store,
		       damage,
		       (int)S_CLIENT_TCP); 
  }
}

void ModuleClass::ReceiveHideMsg(slong newStore, slong newDamage) {
  if ( check != 0xAAAAAAAA ) return;
  host.ship->ClearPendingFlag(ShipClass::disappearing);
  SetStat(store,newStore);
  //store = newStore;
  SetStat(damage,newDamage);
  //damage = newDamage;
  activate();
  host.generic->PlaySoundOnScreen(activationSoundIndex);
}

void ModuleClass::SendUnHideMsg() {
  if ( check != 0xAAAAAAAA ) return;

  if (!host.ship->IsPending(ShipClass::disappearing)) {

    retMsgBuffer.PushMsg((uchar)SF_MSG_UNCLOAK,
			 (ushort)game->ObjectListIndex(host.generic),
			 (int)S_CLIENT_TCP);  

  }
}

void ModuleClass::ReceiveUnHideMsg() {
  if ( check != 0xAAAAAAAA ) return;
  host.ship->ClearPendingFlag(ShipClass::disappearing);
  deactivate();
  host.designed->stealth = 1;
  host.generic->PlaySoundOnScreen(deactivationSoundIndex);
}

void ModuleClass::SendDockActivationMsg() {
  if ( check != 0xAAAAAAAA ) return;

  /***************************************************************************/
  /* check for the following:                                                */
  /* !game->IsPending() will verify that there are no FILLSLOT messages      */
  /* pending that could lead to a conflict over a port.  NOTE: there is      */
  /* still a possible loophole - if a human player attempts to activate a    */
  /* port the server has a FILLSLOT message pending then we could end up     */
  /* with a conflict over the port.  To fix this we could disallow human     */
  /* players from constructing new ships.                                    */
  /*                                                                         */
  /* if two human players activate a port at the same time then they can     */
  /* both send dockingsuccessful messages for the same port at the same time */
  /*    to prevent this we do not allow one ship to have more than one active*/
  /*    tractor beam at one time.  This translates into the following.  The  */
  /*    server should always check the tractoring flag.  This flag should    */
  /*    only be reset when the tractor beam is turned off.                   */
  /***************************************************************************/

  int dockIsActive = IsActive();
  int dockIsBeingActivated = host.ship->IsPending(ShipClass::activating_dock);
  int tractorBeamIsOn = host.ship->IsPending(ShipClass::tractoring);
  int fillSlotMsgIsPending = game->IsPending();
  int isOfflinePlayer = IsDisconnectedClient();
  int isLocalPlayer = IsConnectedClient() && IsLocalPlayer(host.ship);
  int isAIPlayer = game->IsRobotPlayer(host.ship);
  int isAIOnServer = IsServer() && isAIPlayer;

  int isControllingPlayer = isOfflinePlayer || isLocalPlayer || isAIOnServer;

  ShipClass *dockee = (ShipClass *)workPar[0];
  if ( !dockIsActive && !dockIsBeingActivated && !tractorBeamIsOn &&
       !fillSlotMsgIsPending && isControllingPlayer )
    {
      
      host.ship->SetPendingFlag(ShipClass::activating_dock);

      retMsgBuffer.PushMsg((uchar)SF_MSG_DOCK,
			   (ushort)game->ObjectListIndex(host.generic),
			   (ushort)game->ObjectListIndex(dockee),
			   (uchar)(host.ship->bus.moduleList.indexOf(this)),
			   (ulong)store,
			   (ulong)damage,
			   (int)S_CLIENT_TCP); 

      WRITE_SF_LOG(logFile << "sent> a ");
      WRITE_SF_LOG(host.ship->ShortProfile());
      WRITE_SF_LOG(logFile << " activated port " << 
		   this - host.ship->bus.moduleList <<
		   " on frame " << game->frame << endl);
    }
  else if ( !dockIsActive && 
	    (dockIsBeingActivated || tractorBeamIsOn || fillSlotMsgIsPending) &&
	    !isAIPlayer)
    {
      logFile << "note> ";
      
      ((ShipClass*)workPar[0])->ShortProfile();
      
      logFile << " has failed to activate the dock of ";
      
      host.ship->ShortProfile();
      
      if ( dockIsBeingActivated )
	logFile << " because a dock activation message is already pending";
      if ( fillSlotMsgIsPending )
	logFile << " because a fill slot msg is pending";
      else if ( tractorBeamIsOn )
	{
	  logFile << " because a dock is already active";
	  short i = 0;
	  while (i < host.ship->bus.numPorts &&
		 !host.ship->bus.dockMod[i]->IsActive())
	    i++;
	  if (i == host.ship->bus.numPorts) {
	    logFile << " (no active docks found - condition corrected)" << endl;
	    host.ship->ClearPendingFlag(ShipClass::tractoring);
	  }
	  
	}
      
      logFile << " on frame " << game->frame << endl;
      
    }
  
}

void
ModuleClass::ReceiveDockActivationMsg(GenericClass* g, 
				      slong newStore, 
				      slong newDamage) {
  if ( check != 0xAAAAAAAA ) return;

  host.ship->ClearPendingFlag(ShipClass::activating_dock);
  host.ship->SetPendingFlag(ShipClass::tractoring);

  SetStat(store,newStore);
  //store = newStore;
  SetStat(damage,newDamage);
  //damage = newDamage;
  /* dock activation */
  activate();
  workPar[0] = slong(g);
  workPar[1] = 0;
  frame = 0;

  WRITE_SF_LOG(logFile << "received> a ");
  WRITE_SF_LOG(host.ship->ShortProfile());
  WRITE_SF_LOG(logFile << " activated port " << this - host.ship->bus.moduleList <<
    " on frame " << game->frame << endl);
}

void ModuleClass::SendDockDeactivationMsg(char* reason) {
  if ( check != 0xAAAAAAAA ) return;

  if (IsActive() && !host.ship->IsPending(ShipClass::deactivating_dock) &&
      (IsDisconnectedClient() ||
       (IsConnectedClient() && IsLocalPlayer(host.ship)) ||
       (IsServer() && game->IsRobotPlayer(host.ship)))) {
    
    WRITE_SF_LOG(logFile << "sent> a ");
    WRITE_SF_LOG(host.ship->ShortProfile());
    WRITE_SF_LOG(logFile << " deactivated port " << this - host.ship->bus.moduleList <<
      " on frame " << game->frame << " because " << reason << endl);

    host.ship->SetPendingFlag(ShipClass::deactivating_dock);
    
    retMsgBuffer.PushMsg((uchar)SF_MSG_DEACTIVATEDOCK,
			 (ushort)game->ObjectListIndex(host.generic),
			 (uchar)(host.ship->bus.moduleList.indexOf(this)),
			 (int)S_CLIENT_TCP);  

    /* local object will deactivate now; remote object will deactivate on receipt of message */
    /* dock deactivation */
    deactivate();
  }

}

void ModuleClass::ReceiveDockDeactivationMsg() {
  if ( check != 0xAAAAAAAA ) return;

  WRITE_SF_LOG(logFile << "received> a ");
  WRITE_SF_LOG(host.ship->ShortProfile());
  WRITE_SF_LOG(logFile << " deactivated port " << this - host.ship->bus.moduleList <<
    " on frame " << game->frame << endl);

  if (frame > 3)
    host.generic->PlaySoundOnScreen(deactivationSoundIndex);

  host.ship->ClearPendingFlag(ShipClass::deactivating_dock);
  host.ship->ClearPendingFlag(ShipClass::tractoring);

  /* dock deactivation */
  deactivate();
}

void ShipClass::SendUnDockMsg() {
  /* this object undocks from its dockObj */

  if (!IsPending(undocking) &&
      !IsPending(refitting) &&
      !IsPending(captured) &&
      dockObj) {

    if (IsDisconnectedClient() ||
	(IsConnectedClient() && (IsLocalPlayer(this) &&
				 game->IsRobotPlayer(dockObj))) ||
	(IsConnectedClient() && IsLocalPlayer(dockObj)) ||
	(IsServer() && (game->IsRobotPlayer(dockObj) &&
			game->IsRobotPlayer(this)))) {
    
      SetPendingFlag(undocking);
      
      retMsgBuffer.PushMsg((uchar)SF_MSG_UNDOCK,
			   (ushort)game->ObjectListIndex(dockObj),
			   (ushort)game->ObjectListIndex(this),
			   (int)S_CLIENT_TCP);  
    }
    
    if (IsDisconnectedClient() || IsConnectedClient() && IsLocalPlayer(this) ||
	IsServer() && game->IsRobotPlayer(this)) {
      ReceiveUnDockMsg();
    }
  }
  
}

void ModuleClass::SendDockingSuccessfulMsg() {
  if ( check != 0xAAAAAAAA ) return;
  /* send a message indicating which object has docked, which    */
  /* port it is attached to, how much energy the dock has stored */
  /* and how much damage it has sustained                        */

  ShipClass* s = (ShipClass*) workPar[0];

  if (host.ship->IsPending(ShipClass::docking) ||
      s->IsPending(ShipClass::docking))
    return;

  /***************************************************************************/
  /* set docking flags on both ships to lock out other attempts to send      */
  /* docking messages involving these two ships                              */
  /***************************************************************************/

  host.ship->SetPendingFlag(ShipClass::docking);

  s->SetPendingFlag(ShipClass::docking);

  logFile << "sent> a ";
  s->ShortProfile();
  logFile << " docked successfully with a ";
  host.ship->ShortProfile();
  logFile << " at port " << host.ship->bus.moduleList.indexOf(this) <<
    " on frame " << game->frame <<
    endl;

  if (game->IsRobotPlayer(s)) {

    /************************************************************************/
    /* situation: a robot player has been captured by either another robot  */
    /*            player or a human player.  Since the docking message has  */
    /*            to originate with the player that has captured, except in */
    /*            the case of a human host, so that it is consistent with   */
    /*            what that player sees on his screen then we need to send  */
    /*		  a SF_MSG_ISDOCKED.                                        */
    /************************************************************************/
  
    retMsgBuffer.PushMsg((uchar)SF_MSG_ISDOCKED,
			 (ushort)game->ObjectListIndex(host.generic),
			 (ushort)game->ObjectListIndex(s),
			 (uchar)(host.ship->bus.moduleList.indexOf(this)),
			 (int)S_CLIENT_TCP);

  } else {

    /***********************************************************************/
    /* situation: a human player has been capatured by either a robot or   */
    /*            another human player.  Since the docking message has to  */
    /*            originate with the player that is captured so that it is */
    /*            consistent with what that player sees on his screen then */
    /*            we need to send a SF_MSG_HUMANISDOCKED.                  */
    /***********************************************************************/
  
    retMsgBuffer.PushMsg((uchar)SF_MSG_HUMANISDOCKED,
			 (ushort)game->ObjectListIndex(s),
			 (ushort)game->ObjectListIndex(host.generic),
			 (uchar)(host.ship->bus.moduleList.indexOf(this)),
			 (int)S_CLIENT_TCP);  

  }

}

void ModuleClass::ReceiveDockingSuccessfulMsg(short objIndex) {
  if ( check != 0xAAAAAAAA ) return;

  static char msg[1024];

  ObjectUnion s;
  s.ship = game->ObjectListObj(objIndex).ship;

  host.ship->ClearPendingFlag(ShipClass::docking);
  host.ship->ClearPendingFlag(ShipClass::tractoring);
  s.ship->ClearPendingFlag(ShipClass::docking);

  AssignPortToShip(s.ship);

  logFile << "received> a ";
  s.ship->ShortProfile();
  logFile << " docked successfully with a ";
  host.ship->ShortProfile();
  logFile << " at port " << s.ship->portNum << " on frame " << game->frame <<
    endl;

  /* the parent object should be reset so that spawned colonies */
  /* can be dropped back on their original planets              */
  s.ship->parent.generic = 0;

  host.generic->PlaySoundOnScreen(game->soundEvent[successful_docking_event]);
  
  if (s.ship->Type() == ship &&
      game->TypeListIndex(s.ship) == localSlotNumber) {
    if (/*host.designed->bus.numPorts > 1 */
	host.designed->bus.factoryMod
	/* multi-port --> factory */ && 
	game->PlayerIsUpgradeable(s.ship->playerIndex)) {
      strcpy(msg,
	     "1st Officer> Docking successful.  Press backspace to upgrade ship.");
    } else {
      strcpy(msg, "1st Officer> Docking successful.");
    }

    game->PostGameMessage(msg, localSlotNumber, (short)raceID, personal);
    
  }
  
  if (!host.ship->IsFriendlyTo(s.ship)) {
    
    if (s.ship->pop > host.ship->pop) {

      /* s captures host */

      if (s.generic->Type() == ship &&
	  (IsDisconnectedClient() ||
	   (IsConnectedClient() && (IsLocalPlayer(s.ship) &&
				    game->IsRobotPlayer(host.ship))) ||
	   (IsConnectedClient() && IsLocalPlayer(host.ship)) ||
	   (IsServer() && (game->IsRobotPlayer(s.ship) &&
			   game->IsRobotPlayer(host.ship)))))
	s.ship->SendCaptureMsg(host.ship);
      else if (s.generic->Type() == colony &&
	       (IsDisconnectedClient() || IsServer()))
	s.ship->SendCaptureMsg(host.ship);
      
    } else {

      /* host captures s */

      if (host.generic->Type() == ship &&
	  (IsDisconnectedClient() ||
	   (IsConnectedClient() && (IsLocalPlayer(host.ship) &&
				    game->IsRobotPlayer(s.ship))) ||
	   (IsConnectedClient() && IsLocalPlayer(s.ship)) ||
	   (IsServer() && (game->IsRobotPlayer(host.ship) &&
			   game->IsRobotPlayer(s.ship)))))
	host.ship->SendCaptureMsg(s.ship);
      else if (s.generic->Type() == colony &&
	       (IsDisconnectedClient() || IsServer()))
	host.ship->SendCaptureMsg(s.ship);
      

    }
  }

  if (s.ship->Type() == colony) {
    
    /* set the lastDockObjIndex so that the player can get credit for */
    /* for dropping this colony on a planet                           */
    s.colony->lastDockObjIndex = game->ObjectListIndex(host.generic);
    /* reset the frame counter so that we can tell when */
    /* lastDockObjIndex should expire                   */
    s.colony->frame = 1;

    if (s.colony->lockObj.generic) {
      /* colony was stolen right off the planet */
      
      /* inform planet that it has no colony */
      s.colony->lockObj.planet->colony = 0;
	
      /* make it neutral */
      s.colony->lockObj.planet->raceID = -1;
      
      /* reset planet pointers */
      s.colony->planetIndex = 0;
      s.colony->lockObj.generic = 0;
      s.colony->parent.generic = 0;
    }
  }
  
}

short ModuleClass::IsReady() {
  if ( check != 0xAAAAAAAA ) return 0;
  switch (moduleFunc) {
  case qdrv:
    return func > module_break_limit && par[1] >= host.generic->mass;
    break;
  case factory:
    return 1;
  }
  return func > module_break_limit;
}

void ModuleClass::IncreaseValue(slong v, slong p) {
  if ( check != 0xAAAAAAAA ) return;
  /* increase the value of the module by the amount v */

  short nf;
  slong f, df, t;
  
  switch (moduleFunc) {

  case power_plant:
    AddToStat(power,v/1000);
    //power += v / 1000;
    break;
  case engine:
    SetStat(par[0],min(par[0] + v/1000, 100000 /* max_engine_thr */));
    //par[0] = min(par[0] + v/1000, 100000 /* max_engine_thr */);
    break;
  case qdrv: 
    nf = 2;
    SetStat(par[0],min(par[0] + v/20/nf, 10*game->lightSpeed /* max_qspeed */));
    //par[0] = min(par[0] + v/20/nf, 10*game->lightSpeed /* max_qspeed */);
    SetStat(par[1],min(par[1] + v/20/nf, 100000 /* max_towable_qmass */));
    //par[1] = min(par[1] + v/20/nf, 100000 /* max_towable_qmass */);
    break;
  case tube: 
    nf = 2;
    /* increase the tube burst by increasing the amount of power it */
    /* can store: par[0]                                            */
    /* increase the repeat rate by increasing the power recharge    */
    /* rate: power                                                  */
    SetStat(par[0],min(par[0] * (100 + p/nf) / 100, 100 /* max_tube_cap */));
    //par[0] = min(par[0] * (100 + p/nf) / 100, 100 /* max_tube_cap */);
    SetStat(power,-min(-power * (100 + p/nf) / 100, 50 /* max_tube_power */));
    //power = -min(-power * (100 + p/nf) / 100, 50 /* max_tube_power */);
    break;
  case shield: 
    /* increase the capacity */
    nf = 2;
    f = -power * par[0];
    df = max(min(v/2000, 5000 - f /* max_shield_cap */), 0);
    if ((f + df/nf) / max(par[0],1) != -power) {
      /* try to divide it evenly */
      t = (f + df/nf) / max(-power,1);
      SetStat(power,-(f + df/nf) / max(par[0],1));
      //power = -(f + df/nf) / max(par[0],1);
      SetStat(par[0],t);
      //par[0] = t;
    } else
      /* take it all out of par[0] */
      SetStat(par[0],(f + df) / max(-power,1));
    //par[0] = (f + df) / max(-power,1);
    break;
  case brake:
    /* decrease the braking factor */
    SetStat(par[0],max(par[0] + v/5, 50 /* max_brake_factor */));
    //par[0] = max(par[0] + v/5, 50 /* max_brake_factor */);
    break;
  case sensor:
    SetStat(par[0],min(par[0] + v/20000, 100 /* max_sensor_par */)); 
    //par[0] = min(par[0] + v/20000, 100 /* max_sensor_par */);
    break;
  case fuel_cell: 
    /* do nothing */
    break;
  case battery: 
    /* do nothing */
    break;
  case counter_measures:
    SetStat(par[1],min(par[1] + v/3000, 500 /* max_cm_range */));
    par[1] = min(par[1] + v/3000, 500 /* max_cm_range */);
    break;
  case cloak: 
    /* increase the capacity */
    nf = 2;
    f = -power * par[0];
    df = min(v/10, 40000 - f /* max_stealth_cap */);
    if ((f + df/nf) / max(par[0],1) != -power) {
      /* try to divide it evenly */
      t = (f + df/nf) / max(-power,1);
      SetStat(power,-(f + df/nf) / max(par[0],1));
      //power = -(f + df/nf) / max(par[0],1);
      SetStat(par[0],t);
      //par[0] = t;
    } else
      /* take it all out of par[0] */
      SetStat(par[0],(f + df) / max(-power,1));
    //par[0] = (f + df) / max(-power,1);
    break;
  case boost: 
    /* increase the capacity first, then maxSpeed and acceleration */
    nf = 4;
    f = -power * par[0];
    df = min(v / max(par[1]*par[2]/100,1), 500000 - f /* max_boost_cap */);
    t = (f + df/nf) / max(-power,1);
    SetStat(power,-(f + df/nf) / max(par[0],1));
    //power = -(f + df/nf) / max(par[0],1);
    SetStat(par[0],t);
    //par[0] = t;
    /* now adjust maxSpeed and accel */
    SetStat(par[1],min(par[1] * (100 + p/nf)/100, 300 /* max_boost_vpct */));
    //par[1] = min(par[1] * (100 + p/nf)/100, 300 /* max_boost_vpct */);
    SetStat(par[2],min(par[2] * (100 + p/nf)/100, 80 /* max_boost_apct */));
    //par[2] = min(par[2] * (100 + p/nf)/100, 80 /* max_boost_apct */);
    break;
  case dock: 
    /* increase the tractor capacity first, then range and strength */
    nf = 4;
    f = -power * par[0];
    df = min(v / max(par[1]*par[2]/50000,1), 100000 - f /* max_dock_cap */);
    t = (f + df/nf) / max(-power,1);
    SetStat(power,-(f + df/nf) / max(par[0],1));
    //power = -(f + df/nf) / max(par[0],1);
    SetStat(par[0],t);
    //par[0] = t;
    /* now adjust maxSpeed and accel */
    SetStat(par[1],min(par[1] * (100 + p/nf)/100,
		 5000*unit_thrust/g_scale /* max_tractor_thr */));
    //par[1] = min(par[1] * (100 + p/nf)/100,
    //	 5000*unit_thrust/g_scale /* max_tractor_thr */);
    SetStat(par[2],min(par[2] * (100 + p/nf)/100, 400 /* max_tractor_range */));
    //par[2] = min(par[2] * (100 + p/nf)/100, 400 /* max_tractor_range */);
    break;
  case factory: 
    /* do nothing */
    break;
  case armor: 
    /* do nothing */
    break;
  }
}

void ModuleClass::AssignPortToShip(ShipClass* s) {
  if ( check != 0xAAAAAAAA ) return;

  if (!s)
    return;
  else if (s->bus.numPorts) 
    /* release ships already docked at the docking ship */
    s->ReleaseDocks();

  /* set docking flag to indicate this port is taken */
  workPar[1] = 1;
  /* set docking pointer to object address */
  workPar[0] = slong(s);
  /* dock deactivation */
  deactivate();

  /* increase mass of towing ship */
  host.generic->mass += s->mass;
  host.generic->gmass = host.generic->mass;
  
  s->stat = DesignedClass::docked;
  s->assign = DesignedClass::dry_dock;
  s->assignOverRide = DesignedClass::go_home;
  s->portNum = host.designed->bus.moduleList.indexOf(this);
  s->dockObj = host.ship;

  s->turning = 0;
  s->seekTurning = 0;

  if (host.designed->bus.factoryMod)
    /* multi-port (changed to factory) */
    s->bus.ReplenishSystems(); /* factory facilities can replenish tubes */

  if (host.generic->parent.planet && host.generic->parent.planet->colony &&
      s->Type() == ship) {

    PopStruct nCrew = s->OptimalCrew();
    if (host.generic->parent.planet->colony->pop < nCrew - s->pop) {
      s->pop += host.generic->parent.planet->colony->pop / 2;
      host.generic->parent.planet->colony->pop -= 
	host.generic->parent.planet->colony->pop / 2;
    } else {
      host.generic->parent.planet->colony->pop -= nCrew - s->pop;
      s->pop = nCrew;

      /* make sure that the population is not negative */
      if (host.generic->parent.planet->colony->pop < PopStruct())
	host.generic->parent.planet->colony->pop = PopStruct(0,1);
      
    }
  }

  logFile << "note> a ";
  host.ship->ShortProfile();
  logFile << " docked a ";
  s->ShortProfile();
  logFile << " at port " << s->portNum << " on frame " << game->frame << endl;  

}

void ModuleClass::Block() {
  if ( check != 0xAAAAAAAA ) return;
  /* this module does not consume power, and its function is already */
  /* handled by DesignedClass::Hit()                                 */

  return;

}

void ModuleClass::Build() {
  if ( check != 0xAAAAAAAA ) return;
  /* workPar[0] is the object pointer to the object being created */
  /* workPar[1] is the current stage in the construction project  */

  static CoordinateClass cOffset;

  ShipClass* s = (ShipClass*) workPar[0];
  if (!s || s && s->IsExploding() && workPar[1]) {

    if (!s) {
      logFile << "ERROR> factory object for a";
      host.generic->ShortProfile();
      logFile << "was activated without a build assignment on frame " <<
	game->frame << endl;
    }

    workPar[1] = 0;
    workPar[0] = 0;
    deactivate();

    return;
  }

  SetStat(store,1);
  //store = 1; /* this is a test -- remove me */

  if (store) {
    if (workPar[1] == 0) {
      /* start building a new ship */
      /* this stage requires that workPar[0] be set to the object   */
      /* pointer for object being built and that the factory module */
      /* is activated                                               */
      
      //short buildPort = 0;
  
      //if (host.generic->Type() == ship) {
      //buildPort = host.ship->FindNextFreePort();
      //}
      short buildPort = 0;
      if (host.generic->Type() == ship) {
	/* extract the build port number from specPar[3] */
	buildPort = short(specPar[3]);
      }

      if (buildPort || host.generic->Type() == colony) {
	//logFile << host.generic->TypeName() << " " << buildPort << endl;
	ShipDesignStruct* sd =
	  game->shipDesign + game->player[s->playerIndex].shipIndex;

	ModuleClass* m = game->moduleList + 
	  game->LookUpModuleById(sd->bus.moduleIdList[0], &logFile);
	slong mv = 0;
	if ( m )
	  {
	    mv = m->value();
	  }
	/* if (specPar[0] > mv && specPar[1] > m->mass) {*/
	s->initializeWith(game->TypeListIndex(s), s->playerIndex, 1);

	//specPar[0] -= mv;
	specPar[0] = 0; /* remove all storage production */
	/* deduct mass of module from raw materials storage */
	specPar[1] -= m->mass;
	
	/* refit ship if player's ship does not match player's race or */
	/* player's rank is greater than required for current ship     */
	game->CheckRank(s->playerIndex, s, 1 /* build */);
	
	s->activate();
	workPar[1] = 1;
	if (host.generic->Type() == ship && buildPort) {
	  /* assign the available port to the new ship */
	  host.ship->bus.dockMod[buildPort-1]->workPar[0] = slong(s);
	  host.ship->bus.dockMod[buildPort-1]->AssignPortToShip(s);

	}

	if (host.generic->parent.generic)
	  s->parent = host.generic->parent;
	s->Assign(DesignedClass::manual);
	/* this line must follow AssignPortToShip because it sets stat to "docked" */
	s->stat = DesignedClass::under_construction;

	/* record build event in log */
	/* logFile << "Build: " << host.generic->TypeName() << " " <<
	   game->TypeListIndex(host.generic) <<
	   " started building a new " << game->race[s->raceID].name << " " <<
	   s->design->name << " ( " << s->now.x << " , " << s->now.y <<
	   " ) "; 
	   if (host.generic->Type() == ship)
	   logFile << "at port " << buildPort;
	   logFile << " on frame " <<
	   game->frame << " using slot " << game->TypeListIndex(s) << endl; */
	/* } */
      }
    } else if (s->bus.numMod < s->design->bus.numMod) {

      /* subtract the current mass prior to adding a new module */
      if (s->dockObj == host.ship) {
	host.generic->mass -= s->mass;
	host.generic->gmass = host.generic->mass;
      }

      ShipDesignStruct* sd = s->design;
      
      ModuleClass* m = game->moduleList + 
	game->LookUpModuleById(sd->bus.moduleIdList[s->bus.numMod], &logFile);
      slong mv = 0;
      if ( m )
	{
	  mv = m->value();
	}
	
      if (specPar[0] > mv && specPar[1] > m->mass) {
	/* plug in the module before incrementing the number of modules */
	s->PlugInModuleByDesign(s->bus.numMod);

	specPar[0] -= mv;
	/* deduct mass of module from raw materials storage */
	specPar[1] -= m->mass;

	/* logFile << "Build: " << host.generic->TypeName() << " " <<
	   game->TypeListIndex(host.generic) <<
	   " plugged in module " << s->bus.numMod << " (" <<
	   s->design->bus.moduleIdList[s->bus.numMod] << ")" <<
	   " in a new " << game->race[s->raceID].name << " " <<
	   s->design->name << " ( " << s->now.x << " , " << s->now.y <<
	   " ) on frame " << game->frame << endl; */
	
	s->bus.numMod++;

	s->bus.MapFunctions(s, s);

	s->stat = DesignedClass::under_construction;

	if (s->dockObj == host.ship) {
	  host.generic->mass += s->mass;
	  host.generic->gmass = host.generic->mass;
	}
	
	workPar[1] = s->bus.numMod;
      }
    }
  }

  /* stay locked on factory object */
  if (!s->dockObj || s->dockObj && s->dockObj != host.ship) {

    cOffset.initializeWith(0,(s->bitmap.size + host.generic->bitmap.size) * 
			   sub_scale / 2);

    s->now = host.generic->now + cOffset;
      
    s->gnow = s->now;
    s->vel = host.generic->vel;
    s->tvel = host.generic->tvel;
  }

  if (!host.ship->IsPending(ShipClass::completing_build) &&
      s->bus.numMod == s->design->bus.numMod &&
      (IsDisconnectedClient() || IsServer())) {

    host.ship->SetPendingFlag(ShipClass::completing_build);

    /* send a message to release the ship from construction */
    retMsgBuffer.PushMsg((uchar)SF_MSG_COMPLETEBUILD,
			 (ushort)game->ObjectListIndex(s),
			 (ushort)game->ObjectListIndex(host.generic),
			 S_CLIENT_TCP);

    logFile << "sent> a ";
    host.designed->ShortProfile();
    logFile << " has completed construction of a ";
    s->ShortProfile();
    logFile << " on frame " << game->frame << endl;

  }

  host.designed->CalculateMass();

  if (host.generic->mass == 0)
    logFile << "ERROR> ModuleClass::Build: zero mass detected." << endl;

}

void ModuleClass::ReceiveCompleteBuildMsg(ShipClass* s) {
  if ( check != 0xAAAAAAAA ) return;

  host.ship->ClearPendingFlag(ShipClass::completing_build);
  
  deactivate();
  workPar[0] = 0;
  workPar[1] = 0;
  specPar[1] = 0; /* reset raw materials storage to zero */

  /* make sure that the build process is complete */
  if (s->bus.numMod < s->design->bus.numMod) {
    /* the build process was not complete when this message was received */
    /* the ship should be rebuild quickly now to sychronize it with the  */
    /* server.                                                           */

    logFile << "note> the build process for a ";
    s->ShortProfile();
    logFile << " under construction by a ";
    host.ship->ShortProfile();
    logFile << " was interrupted by the arrival of a COMPLETEBUILD message.  Completing build on frame " <<
      game->frame << endl;
    
    if ( IsValidObjectPointer(s->dockObj,__FILE__,__LINE__) )
      {
	s->dockObj->ProcessFreeDockMsg(s->portNum);
      }

    s->bus.FreeModuleList();
    s->initializeWith(game->TypeListIndex(s), s->playerIndex);
    s->activate();
  }

  s->assignPriority = 0;
  /* reset status from under construction */
  /* the Assign function is prevented from resetting status if the */
  /* object is under construction */
  s->stat = DesignedClass::idle;
  s->FollowObj(host.generic);
  
  logFile << "received> a ";
  
  host.designed->ShortProfile();
  
  logFile << " has completed construction of a ";
  
  s->ShortProfile();
  
  logFile << " on frame " << game->frame << endl;
  
}

void ModuleClass::Boost() {
  if ( check != 0xAAAAAAAA ) return;

  /* [host] object increases maximum speed */
  slong cost = -2 * power, surplus;

  if (store >= cost) {
    AddToStat(store,-cost);
    //store -= cost;
    surplus = store;
  } else {
    host.designed->bus.WithdrawSurplusPower(cost - store);
    SetStat(store,0);
    //store = 0;
    surplus = host.designed->bus.CalculateSurplusPower();
  }
  if (surplus) {
    /* deactivate module if valve shuts off */
    if (host.generic->Type() == ship) 
      host.ship->maxSpeed = (100 + cPar(1, host.designed->bus.CalculateSurplusPower())) *
	host.ship->design->maxSpeed / 100;
    else if (host.generic->Type() == missile) 
      host.missile->maxSpeed = (100 + cPar(1, host.designed->bus.CalculateSurplusPower())) *
	host.missile->design->maxSpeed / 100;
  } else {
    host.designed->PlaySoundOnScreen(deactivationSoundIndex);
    //if (sf && sf->mwl && activationSoundIndex != -1)
    //  sf->audio->Stop(sf->mwl->Wave(activationSoundIndex));
    deactivate();
  }
}

void ModuleClass::FireBeam(PingPacket* pp) {
  if ( check != 0xAAAAAAAA ) return;

  /******************************************************/
  /* for beams:                                         */
  /* [host] object spawns a beam from all active beams  */
  /* in the direction of the [host] object's target, if */
  /* in range                                           */
  /******************************************************/

  slong cost;

  short fired = 0;

  /* moduleFunc == beam */
  
  if ((fired = host.designed->SpawnBeam(this, pp)) == 1) {
    
    /* sound_beam_firing */
    host.generic->PlaySoundOnScreen(activationSoundIndex);
    
  } else
    deactivate();
  
  if (fired) {
    
    cost = par[1]; //TubeEnergyCost();
    if (cost <= store)
      {
	AddToStat(store,-cost);
	//store -= cost;
      }
    else
      {
	host.designed->bus.WithdrawSurplusPower(cost-store);
	SetStat(store,0);
	//store = 0;
      }
    
  }
}

void ModuleClass::FireTube() {
  if ( check != 0xAAAAAAAA ) return;

  /******************************************************/
  /* for tubes:                                         */
  /* [host] object spawns missile from a tube in the    */
  /* default direction                                  */
  /*                                                    */
  /* for beams:                                         */
  /* [host] object spawns a beam from all active beams  */
  /* in the direction of the [host] object's target, if */
  /* in range                                           */
  /******************************************************/

  slong cost;
  
  if ( (!CheckTubeMode(auto_fire_mode) || game->IsRobotPlayer(host.ship)) && 
       moduleFunc != beam )
    /* par[4] & auto_fire_mode ==> auto-repeat */
    active = 0; /* auto-shut-off */
  
  short missileIndex = (short)par[3];
  short fired = 0;
  
  if (moduleFunc == tube) {
    if ( (fired = host.designed->Spawn(game->missileDesign[missileIndex], specPar)) != -1) {

      host.generic->PlaySoundOnScreen(activationSoundIndex);

      workPar[0]--;
    }
  } else { /* moduleFunc == beam */

    if ((fired = host.designed->SpawnBeam(this)) == 1) {

      /* sound_beam_firing */
      host.generic->PlaySoundOnScreen(activationSoundIndex);

    } else
      deactivate();

  }

  if (fired) {

    cost = par[1]; //TubeEnergyCost();
    if (cost <= store)
      {
	AddToStat(store,-cost);
	//store -= cost;
      }
    else
      {
	host.designed->bus.WithdrawSurplusPower(cost-store);
	SetStat(store,0);
	//store = 0;
      }

  }
}

void ModuleClass::FireTubeWithIndex(short missileIndex, 
				    GenericClass* targetOverride) {
  if ( check != 0xAAAAAAAA ) return;

  /******************************************************/
  /* for tubes:                                         */
  /* [host] object spawns a missile with a specified    */
  /* design index from a tube in the default direction  */
  /*                                                    */
  /* for beams:                                         */
  /* [host] object spawns a beam in the direction of    */
  /* the [host] object's target, if in range            */
  /******************************************************/

  slong cost;

  if ((!CheckTubeMode(auto_fire_mode) || game->IsRobotPlayer(host.ship)) && moduleFunc != beam) /* par[4] & auto_fire_mode ==> auto-repeat */
    active = 0; /* auto-shut-off */

  short fired = 0;

  if (moduleFunc == tube) {
    if ((fired = host.designed->Spawn(game->missileDesign[missileIndex], specPar)) != -1) {

      host.generic->PlaySoundOnScreen(activationSoundIndex);

      if (targetOverride && fired != -1) {
	/* the spawning object has specified a new target for the missile */
	game->missile[fired].targetPP.obj.generic = targetOverride;
      }

      workPar[0] = max(workPar[0]-1, 0);
    }
  } else { /* moduleFunc == beam */
    if ((fired = host.designed->SpawnBeam(this)) == 1) {
      host.generic->PlaySoundOnScreen(activationSoundIndex);
    } else
      deactivate();
  }

  if (fired) {
    cost = par[1]; //TubeEnergyCost();
    if (cost <= store)
      {
	AddToStat(store,-cost);
	//store -= cost;
      }
    else
      {
	host.designed->bus.WithdrawSurplusPower(cost-store);
	SetStat(store,0);
	//store = 0;
      }

  }

}

void 
ModuleClass::FireTubeAtOffset(short offset, 
			      short missileIndex, 
			      GenericClass* targetOverride)
{
  if ( check != 0xAAAAAAAA ) return;

  if ( !IsValidObjectPointer(host.generic,__FILE__,__LINE__) )
    {
      return;
    }

  /******************************************************/
  /* for tubes:                                         */
  /* [host] object spawns a missile at a specified      */
  /* angle                                              */
  /*                                                    */
  /* for beams:                                         */
  /* [host] object spawns a beam in the direction of    */
  /* the [host] object's target, if in range            */
  /******************************************************/
  
  slong cost;
  
  if ( (!CheckTubeMode(auto_fire_mode) || game->IsRobotPlayer(host.ship)) && 
       moduleFunc != beam )
    {
      /* par[4] & auto_fire_mode ==> auto-repeat */
      active = 0; /* auto-shut-off */
    }
  
  slong tpar = specPar[2];
  if ( host.generic->bitmap.dir != ::Dir )
    {
      specPar[2] = 
	(offset + host.generic->bitmap.numFrames) %
	host.generic->bitmap.numFrames;
    }
  else
    {
      specPar[2] = 
	(offset + game->maxAngles) % game->maxAngles; /* numAngles */
    }
  
  short fired = 0;

  switch (moduleFunc) {
    
  case tube:
    if ( (fired = host.designed->Spawn(game->missileDesign[missileIndex], 
				       specPar, 
				       0, 
				       -1, 
				       targetOverride) ) != -1 )
      {
	if ( targetOverride && fired >= 0 && fired < game->maxMissiles )
	  {
	    /* Spawning object has specified a new target for the missile */
	    game->missile[fired].targetPP.obj.generic = targetOverride;
	  }
	
	host.generic->PlaySoundOnScreen(activationSoundIndex);
	workPar[0]--;
      }
    break; /* tube */
    
  case beam:
    if ( (fired = host.designed->SpawnBeam(this)) == 1 )
      {
	host.generic->PlaySoundOnScreen(activationSoundIndex);
      } 
    else
      {
	deactivate();
      }
    
    break; /* beam */
    
  case ram:
    
    if ( (fired = host.designed->Spawn(game->missileDesign[missileIndex], 
				       specPar) ) != 1 )
      {
	
	if( fired >= 0 && fired < game->maxMissiles )
	  {	
	    host.generic->PlaySoundOnScreen(activationSoundIndex);
	    
	    workPar[0]--;
	    
	    /* the value of fired should be the index of the missile slot */
	    game->missile[fired].lockObj = host;
	    
	    /* assignPar1 holds the slot number of the tube that fired it */
	    game->missile[fired].assignPar1 = 
	      host.designed->bus.moduleList.indexOf(this);
	    
	    /* duty is used to identify that this missile is a ram weapon */
	    game->missile[fired].duty = ram_weapon;
	    
	    /***************************************************************/
	    /* use specPar[3] to keep track of the last missile fired.  If */
	    /* it is still active then remove it                           */
	    /***************************************************************/
	    if (game->missile[short(specPar[3])].IsActive() &&
		game->missile[short(specPar[3])].parent.generic==host.generic&&
		game->missile[short(specPar[3])].duty == ram_weapon)
	      {
		game->DestroyObject(game->missile + short(specPar[3]));
	      }
	    
	    /* store the index of the new missile */
	    specPar[3] = fired;
	  }
      }
    
    break; /* ram */
    
  }
  
  if ( fired )
    {
      cost = par[1]; //TubeEnergyCost();
      if (cost <= store)
	{
	  AddToStat(store,-cost);
	  //store -= cost;
	}
      else
	{
	  host.designed->bus.WithdrawSurplusPower(cost-store);
	  SetStat(store,0);
	  //store = 0;
	} 
    }
  
  specPar[2] = tpar;

  return;
}


slong ModuleClass::WithdrawFuel() {
  if ( check != 0xAAAAAAAA ) return 0;

  /* withdraw power from a fuel power supply (fuel cell) */
  /* it will continue to work fully until destroyed */
  SetStat(output,min(store, power) * (func > 0));
  //output = min(store, power) * (func > 0);
  AddToStat(store,-output);
  //store -= output;
  return output;
}

slong ModuleClass::WithdrawPower() {
  if ( check != 0xAAAAAAAA ) return 0;

  /* withdraw power from a power supply */
  /* it will produce power in proportion to its functionality */
  slong popBonus = 0;
  //if (host.generic->Type() == colony) {
  //  long popLog = host.colony->pop.log10();
  //  popBonus = popLog;
  //}
  SetStat(store,min(store + (power + popBonus)  * func / module_function_scale, par[0]*power * func / module_function_scale));
  //store = min(store + (power + popBonus)  * func / module_function_scale, par[0]*power * func / module_function_scale);
  SetStat(output,store * func / module_function_scale);
  //output = store * func / module_function_scale;
  SetStat(store,max(store-output, 0));
  //store = max(store-output, 0);
  return output;
}

slong ModuleClass::WithdrawStdPower() {
  if ( check != 0xAAAAAAAA ) return 0;
  /* withdraw power from a standard module store */
  /* it will draw power in proportion to its functionality */
  SetStat(store,0);
  //store = 0;
  return output;
}

slong ModuleClass::WithdrawCharge() {
  if ( check != 0xAAAAAAAA ) return 0;
  /* withdraw power from a charge power supply (battery) */
  /* it will continue to work fully until destroyed */
  SetStat(output,min(store, power) * (func > 0));
  //output = min(store, power) * (func > 0);
  AddToStat(store,-output);
  //store -= output;
  return output;
}

slong ModuleClass::DepositFuel(slong& xs) {
  if ( check != 0xAAAAAAAA ) return 0;

  /* cannot retrieve beyond what was burned this frame */
  slong input = min(xs, /*output*/ labs(par[0]*power)-store); 
  AddToStat(store,input);
  //store += input;
  xs -= input;
  return input;
}

slong ModuleClass::DepositStdPower(slong& rate) {
  if ( check != 0xAAAAAAAA ) return 0;
  /* standard deposit into storage */
  slong input = /*max( */ StdPowerReq() * rate / max_power_level /*, 1)*/ ; 
  AddToStat(store,input);
  //store += input;
  return input;
}

slong ModuleClass::DepositCapPower(slong& rate) {
  if ( check != 0xAAAAAAAA ) return 0;
  /* standard deposit into storage */
  slong req = CapPowerReq(), input = 0;
  if (req > 0) {
    input = /*max(*/ req * rate / max_power_level /*, 1)*/ ; 
    AddToStat(store,input);
    //store += input;
  }
  return input;
}

slong ModuleClass::DepositXSPower(slong& xs) {
  if ( check != 0xAAAAAAAA ) return 0;
  /* can retrieve up to the storage capacity */
  slong input = min(xs, labs(par[0]*power)-store);
  AddToStat(store,input);
  //store += input;
  xs -= input;
  return input;
  /* cannot retrieve excess */
  //return 0;
}

slong ModuleClass::DepositCharge(slong& xs) {
  if ( check != 0xAAAAAAAA ) return 0;
  /* can retrieve up to the storage capacity */
  slong input = min(xs, /*min(power,*/ abs(par[0]*power)-store /*)*/ );
  AddToStat(store,input);
  //store += input;
  xs -= input;
  return input;
}

void BusStruct::FireModules(DesignedClass* d) {
  if ( check != 0xAAAAAAAA ) return;
  short i;
  for (i = 0; i < numMod; i++) 
    if (moduleList[i].IsActive() && moduleList[i].IsReady() && 
	moduleList[i].moduleFunc != ModuleClass::tube &&
	moduleList[i].fire ) {
      /* if (game->IsValidDesignedObject(moduleList[i].host.generic)) */
      if (moduleList[i].host.designed == d)
	moduleList[i].Fire();
       else {
	 //DosBeep(1000,25);
	 logFile << "ERROR> FireModules: Illegal host found." << endl;
	 logFile << "                    found " << 
	   slong(moduleList[i].host.designed) << endl;
	 logFile << "                    should be " << slong(d) << endl;
	 d->Explode("       it was found as an illegal module host");
       }
    }
}

void BusStruct::FireWeaponModules(ShipClass* s,
				  CoordinateClass& disp,
				  CoordinateClass& dvel,
				  short locked) {

  if ( check != 0xAAAAAAAA )
    {
      return;
    }

  if ( !IsValidObjectPointer(s) )
    {
      return;
    }

  if ( s->IsAttachedToSB() )
    {
      return;
    }

  DecideWhichWeaponsToFire(s, disp, dvel, locked);
  FireSelectedWeapons(s);

}

int
ShipClass::IsAttachedToSB()
{
  //
  // Dockee is the pointer to the ship that is docked.
  //
  ShipClass *dockee = this;
  if ( !IsValidObjectPointer(dockee) )
    {
      return FALSE;
    }
  
  //
  // Docker is the point to the ship that being docked to.
  //
  ShipClass* docker = dockee->dockObj;
  if ( !IsValidObjectPointer(docker) )
    {
      docker = NULL;
    }
  
  //
  // Should be true when the ship is docked to a Starbase.  If so, don't let
  // ship fire weapons.
  //
  int isAttachedToSB = docker && docker->bus.numPorts > 1;

  return isAttachedToSB;
}


void BusStruct::FireDefensiveWeaponModules(ShipClass* s,
				  CoordinateClass& disp,
				  CoordinateClass& dvel,
				  short locked) {

  if ( check != 0xAAAAAAAA ) return;
  DecideWhichDefensiveWeaponsToFire(s, disp, dvel, locked);
  FireSelectedDefensiveWeapons(s);

}

slong BusStruct::CalculateSurplusPower() {
  if ( check != 0xAAAAAAAA ) return 0;
  slong surplus = 0;
  short i;
  for (i=0; i<numPowerPlants; i++)
    surplus += powerMod[i]->store;

  return surplus;

}

slong BusStruct::CalculatePowerCapacity() {
  if ( check != 0xAAAAAAAA ) return 0;

  slong cap = 0;
  short i;
  for (i=0; i<numPowerPlants; i++)
    cap += powerMod[i]->power * powerMod[i]->fPar(0);

  return cap;

}

void BusStruct::WithdrawSurplusPower(slong deficit) {
  if ( check != 0xAAAAAAAA ) return;
  short i = 0;
  while (deficit && i < numPowerPlants) {
    if (powerMod[i]->store >= deficit) {
      powerMod[i]->AddToStat(powerMod[i]->store,-deficit);
      //powerMod[i]->store -= deficit;
      deficit = 0;
    } else {
      deficit -= powerMod[i]->store;
      powerMod[i]->SetStat(powerMod[i]->store,0);
      //powerMod[i]->store = 0;
    }
    i++;
  }
}

void BusStruct::DecideWhichWeaponsToFire(ShipClass* s,
					 CoordinateClass& disp,
					 CoordinateClass& dvel,
					 short locked) {
  if ( check != 0xAAAAAAAA ) return;
  short i;
  slong cost;
  slong availableStore = 0;
  slong h;
  static char msg[128];

  ObjectUnion host;
  host.ship = s;

  /*********************************************************************************/
  /* robot ships should not use up their power reserve unless they are shooting at */
  /* an inbound proximity weapon, in which case all available reserves should be   */
  /* used to detonate the target                                                   */
  /*********************************************************************************/
  if (!game->IsRobotPlayer(s) ||
      IsValidObjectPointer(s->targetPP.obj.generic, __FILE__, __LINE__) &&
      s->targetPP.obj.generic->Type() == missile)
    availableStore = min(CalculateSurplusPower(), CalculatePowerCapacity() / 2);
  //else
  //  availableStore = min(CalculateSurplusPower(), CalculatePowerCapacity());
  
  static CoordinateClass mvel;
  static CoordinateClass df;
  static CoordinateClass hUnit;

  short nF;
  short isRestricted;
  short missileIndex;
  short isShip = s->Type() == ship;

  for (i = 0; i < numTubes; i++) {

    ModuleClass* t = tubeMod[i];

    isRestricted = (isShip && game->player[s->playerIndex].IsRestrictedFromUsing(t));
    
    missileIndex = (short)t->par[3];
    
    cost = t->par[1]; //t->TubeEnergyCost();

    if (t->IsActive() && !isRestricted) {

      if (((t->moduleFunc == ModuleClass::tube || t->moduleFunc == ModuleClass::ram) &&
	   t->workPar[0] > 0 ||
	   t->moduleFunc == ModuleClass::beam) &&
	  t->store + availableStore >= cost) {
	
	/* subtract the deficit from the available reserves */
	if (cost > t->store)
	  availableStore -= cost - t->store;
	
	if (host.generic->bitmap.dir != ::Dir)
	  nF = host.generic->bitmap.numFrames;
	else
	  nF = game->maxAngles; /* numAngles */
      
	/**********************************************************/
	/* if the tubes are not being directed or they are firing */
	/* self-guided (smart) missiles, then set tube to fire    */
	/* - also set rams to fire automatically if active        */
	/**********************************************************/
	if (t->moduleFunc == ModuleClass::ram ||
	    t->moduleFunc == ModuleClass::tube &&
	    (!locked && disp == nullVector ||
	     game->missileDesign[missileIndex].tracking == smart)) {
	  
	  /* set the tube offset to the default value */
	  host.ship->tubeOffset[i] = (short)t->specPar[2];
	  /* set the decision bit for the tube */
	  host.ship->SetTubeToFire(i);
	  
	} else {
	
	  if (host.generic->bitmap.IsSpin()) {

	    if (t->moduleFunc == ModuleClass::tube) {

	      hUnit = host.generic->bitmap.Dir((short)t->specPar[0]+
					       host.designed->heading);
	  
	      mvel = hUnit *
		game->missileDesign[missileIndex].initialVelocityDisplacement;
	      
	      df = dvel * disp.Mag() / ((mvel - dvel).Mag() + 1) *
		host.ship->accuracy / 100;

	      h = t->specPar[1];
	      if (!h)
		h = host.ship->bitmap.size/2;
	      
	      /* set the tube offset based on predicted angle to target */
	      host.ship->tubeOffset[i] = ((disp + df - hUnit * h).Heading(nF) - 
					  host.designed->heading + 2*nF) % nF;

	      /* set the decision bit for the tube if the target is in */
	      /* the allowable firing arc for the tube                 */
	      if (disp.IsInFiringArc(host.ship->tubeOffset[i],
				     t->specPar[3],
				     t->specPar[4],
				     nF))
		host.ship->SetTubeToFire(i);
	      
	    } else {
	      
	      if (IsValidObjectPointer(host.ship->targetPP.obj.generic, __FILE__, __LINE__) &&
		  host.ship->targetPP.obj.generic->IsActive()) { /* beam */
		
		if (t->CheckTubeMode(ModuleClass::auto_fire_mode)) {
		  if (IsLocalPlayer(s)) {
		    strcpy(msg, "1st Officer> laser has set to manual-fire mode");
		    game->PostGameMessage(msg, game->ObjectListIndex(s), (short)s->raceID, personal, 0);
		  }

		  t->ClearTubeMode(ModuleClass::auto_fire_mode);

		}
		
		if (!t->CheckTubeMode(ModuleClass::auto_fire_mode)) {
		  
		  /****************************************************************/
		  /* if the host ship has beam weapons, make sure the distance to */
		  /* target is updated for the range checking in SpawnBeam        */
		  /****************************************************************/
		  if (host.ship->targetPP.frame != game->frame) {
		    host.ship->targetPP.disp = host.ship->targetPP.obj.generic->now - host.ship->now;
		    host.ship->targetPP.dist = host.ship->targetPP.disp.Mag(0);
		    host.ship->targetPP.frame = game->frame;
		  }
		  
		  /* set the beam offset based on actual angle to target */
		  host.ship->tubeOffset[i] =
		    (host.ship->targetPP.disp.Heading(nF) - host.ship->heading + 3*nF) % nF;
		  
		  /* set the decision bit for the tube if the target is in */
		  /* the allowable firing arc for the tube                 */
		  if (disp.IsInFiringArc(host.ship->tubeOffset[i],
					 t->specPar[3],
					 t->specPar[4],
					 nF))
		    host.ship->SetTubeToFire(i);
		  else {
		    if (IsLocalPlayer(s)) {
		      strcpy(msg, "1st Officer> laser deactivated: target not in firing arc.");
		      game->PostGameMessage(msg, game->ObjectListIndex(s), (short)s->raceID, personal, 0);
		    }
		    host.ship->BlockTubeFromFiring(i);
		  }
		  
		} else {
		  if (IsLocalPlayer(s)) {
		    strcpy(msg, "1st Officer> laser deactivated: laser in auto-fire mode");
		    game->PostGameMessage(msg, game->ObjectListIndex(s), (short)s->raceID, personal, 0);
		  }
		}
	      } else {

		if (IsLocalPlayer(s)) {
		  sprintf(msg, "1st Officer> We must assign a target to use the %s.", t->name);
		  game->PostGameMessage(msg, game->ObjectListIndex(s), (short)s->raceID, personal, 0);
		}

	      }
	    }
	  
	  } else {
	  
	    if (t->moduleFunc == ModuleClass::tube) {

	      hUnit = Dir[(t->specPar[0]+
			   host.designed->heading +
			   + game->maxAngles) % game->maxAngles];
	  
	      mvel = hUnit *
		game->missileDesign[missileIndex].initialVelocityDisplacement;
	      
	      df = dvel * disp.Mag() / ((mvel - dvel).Mag() + 1) *
		host.ship->accuracy / 100;

	      h = t->specPar[1];
	      if (!h)
		h = host.ship->bitmap.size/2;
	      
	      /* set the tube offset based on predicted angle to target */
	      host.ship->tubeOffset[i] =
		((disp + df - hUnit * h).Heading(game->maxAngles) - 
		 host.designed->heading + 2*game->maxAngles) % game->maxAngles;
	      
	      /* set the decision bit for the tube if the target is in */
	      /* the allowable firing arc for the tube                 */
	      if (disp.IsInFiringArc(host.ship->tubeOffset[i],
				     t->specPar[3],
				     t->specPar[4],
				     game->maxAngles))
		host.ship->SetTubeToFire(i);

	    } else if (IsValidObjectPointer(host.ship->targetPP.obj.generic, __FILE__, __LINE__) &&
		       host.ship->targetPP.obj.generic->IsActive() &&
		  !t->CheckTubeMode(ModuleClass::auto_fire_mode)) { /* beam */

	      /****************************************************************/
	      /* if the host ship has beam weapons, make sure the distance to */
	      /* target is updated for the range checking in SpawnBeam        */
	      /****************************************************************/

	      if (host.ship->targetPP.frame != game->frame) {
		host.ship->targetPP.disp = host.ship->targetPP.obj.generic->now - host.ship->now;
		host.ship->targetPP.dist = host.ship->targetPP.disp.Mag(0);
		host.ship->targetPP.frame = game->frame;
	      }

	      /* set the tube offset based on predicted angle to target */
	      host.ship->tubeOffset[i] =
		(host.ship->targetPP.disp.Heading(game->maxAngles) -
		 host.ship->heading + 3*game->maxAngles) % game->maxAngles;

	      /* set the decision bit for the tube if the target is in */
	      /* the allowable firing arc for the tube                 */
	      if (disp.IsInFiringArc(host.ship->tubeOffset[i],
				     t->specPar[3],
				     t->specPar[4],
				     game->maxAngles))
		host.ship->SetTubeToFire(i);
	      else
		host.ship->BlockTubeFromFiring(i);
	    }


	  }
	}
      } else {
	/************************************************************/
	/* tube is active, but does not have enough energy to fire  */
	/* if it is not on auto-fire, then shut it down...          */
	/************************************************************/

	if (!t->CheckTubeMode(ModuleClass::auto_fire_mode)) {
	  t->deactivate();
	}

	host.ship->BlockTubeFromFiring(i);

      }
    }
  }
}


void BusStruct::DecideWhichDefensiveWeaponsToFire(ShipClass* s,
					 CoordinateClass& disp,
					 CoordinateClass& dvel,
					 short locked) {
  if ( check != 0xAAAAAAAA ) return;
  short i;
  slong cost;
  slong availableStore = 0;
  slong h;

  ObjectUnion host;
  host.ship = s;

  /* bombs, proximity and smart */
  if (!(host.ship->avoidPP.obj.generic && host.ship->avoidPP.obj.generic->Type() == missile &&
	host.ship->avoidPP.obj.generic->IsActive() &&
	(host.ship->avoidPP.obj.missile->design->tracking == bomb ||
	 host.ship->avoidPP.obj.missile->design->tracking == smart || 
	 host.ship->avoidPP.obj.missile->design->trigger == on_proximity || 
	 host.ship->avoidPP.obj.missile->design->trigger == on_time)))
    return; /* nothing to do */
  
  availableStore = min(CalculateSurplusPower(), CalculatePowerCapacity() / 2);
  
  static CoordinateClass mvel;
  static CoordinateClass df;
  static CoordinateClass hUnit;

  short nF;
  
  /* check criteria for firing */
  short isHumanControlledShip = 0;
  short isNormalShip = 0;
  short isSpaceStation = 0;
  short isFreeColony = 0;
  short isPlanetBoundColony = 0;

  switch (host.ship->Type()) {
  case ship:
    isHumanControlledShip = !game->IsRobotPlayer(host.ship); 
    isNormalShip = host.ship->Type() == ship && !host.ship->bus.factoryMod;
    isSpaceStation = host.ship->Type() == ship && host.ship->bus.factoryMod && host.ship->bus.numPorts;
    break;
  case colony:
    isFreeColony = host.ship->Type() == colony && !host.colony->lockObj.planet;
    isPlanetBoundColony = host.ship->Type() == colony && host.colony->lockObj.planet;
    break;
  default:
    break;
  }

  short isValidWeapon =
    host.ship->avoidPP.obj.missile->design->tracking == smart ||
    host.ship->avoidPP.obj.missile->design->tracking == bomb;
  short isAllowed = host.ship->AutoResponse() &&
    (isNormalShip && game->AILevel >= high_AI_level ||
     isSpaceStation && game->AILevel >= medium_AI_level ||
     isFreeColony && game->AILevel >= high_AI_level ||
     isPlanetBoundColony && game->AILevel >= medium_AI_level) ||
    isHumanControlledShip && game->ModeIsSet(GameStateClass::laserDefense) ||
    isValidWeapon;
  short thisTubeWillFire = 0;
  short isRestricted;
  short isShip = s->Type() == ship;

  if (isAllowed) {

    for (i = 0; i < numTubes; i++) {

      ModuleClass* t = tubeMod[i];

      isRestricted = (isShip && game->player[s->playerIndex].IsRestrictedFromUsing(t));

      short missileIndex = (short)t->par[3];
    
      cost = t->par[1]; //t->TubeEnergyCost();

      thisTubeWillFire =
	t->moduleFunc == ModuleClass::beam && (t->CheckTubeMode(ModuleClass::auto_fire_mode) ||
					       host.ship->AutoResponse()) ||
	t->moduleFunc != ModuleClass::beam && !t->CheckTubeMode(ModuleClass::manual_fire_mode) &&
	host.ship->AutoResponse();

      if (thisTubeWillFire && !isRestricted) {

	if (((t->moduleFunc == ModuleClass::tube || t->moduleFunc == ModuleClass::ram) &&
	     t->workPar[0] > 0 ||
	     t->moduleFunc == ModuleClass::beam) &&
	    t->store + availableStore >= cost) {
	
	  /* subtract the deficit from the available reserves */
	  if (cost > t->store)
	    availableStore -= cost - t->store;
	
	  if (host.generic->bitmap.dir != ::Dir)
	    nF = host.generic->bitmap.numFrames;
	  else
	    nF = game->maxAngles; /* numAngles */
      
	  if (host.generic->bitmap.IsSpin()) {

	    if (t->moduleFunc == ModuleClass::tube) {

	      hUnit = host.generic->bitmap.Dir((short)t->specPar[0]+
					       host.designed->heading);
	  
	      mvel = hUnit *
		game->missileDesign[missileIndex].initialVelocityDisplacement;
	      
	      df = dvel * disp.Mag() / ((mvel - dvel).Mag() + 1) *
		host.ship->accuracy / 100;

	      h = t->specPar[1];
	      if (!h)
		h = host.ship->bitmap.size/2;
	      
	      /* set the tube offset based on predicted angle to target */
	      host.ship->tubeOffset[i] = ((disp + df - hUnit * h).Heading(nF) - 
					  host.designed->heading + 2*nF) % nF;

	      /* set the decision bit for the tube if the target is in */
	      /* the allowable firing arc for the tube                 */
	      if (disp.IsInFiringArc(host.ship->tubeOffset[i],
				     t->specPar[3],
				     t->specPar[4],
				     nF))
		host.ship->SetDefensiveTubeToFire(i);
	      
	    } else { /* beam */

	      /****************************************************************/
	      /* if the host ship has beam weapons, make sure the distance to */
	      /* target is updated for the range checking in SpawnBeam        */
	      /****************************************************************/
	      if (host.ship->avoidPP.frame != game->frame ||
		  host.ship->avoidPP.frame != game->frame-1) {
		host.ship->avoidPP.disp = host.ship->avoidPP.obj.generic->now - host.ship->now;
		host.ship->avoidPP.dist = host.ship->avoidPP.disp.Mag(0);
		host.ship->avoidPP.frame = game->frame;
	      }
	      
	      /* set the beam offset based on actual angle to target */
	      host.ship->beamOffset[i] =
		(host.ship->avoidPP.disp.Heading(nF) - host.ship->heading + 3*nF) % nF;

	      /* set the decision bit for the beam if the target is in */
	      /* the allowable firing arc for the beam                 */
	      if (disp.IsInFiringArc(host.ship->beamOffset[i],
				     t->specPar[3],
				     t->specPar[4],
				     nF))
		host.ship->SetDefensiveTubeToFire(i);
	      else {
		host.ship->BlockDefensiveTubeFromFiring(i);
	      }
	      
	    }
	  
	  } else {
	  
	    if (t->moduleFunc == ModuleClass::tube) {

	      hUnit = Dir[(t->specPar[0]+
			   host.designed->heading +
			   + game->maxAngles) % game->maxAngles];
	  
	      mvel = hUnit *
		game->missileDesign[missileIndex].initialVelocityDisplacement;
	      
	      df = dvel * disp.Mag() / ((mvel - dvel).Mag() + 1) *
		host.ship->accuracy / 100;

	      h = t->specPar[1];
	      if (!h)
		h = host.ship->bitmap.size/2;
	      
	      /* set the tube offset based on predicted angle to target */
	      host.ship->tubeOffset[i] =
		((disp + df - hUnit * h).Heading(game->maxAngles) - 
		 host.designed->heading + 2*game->maxAngles) % game->maxAngles;
	      
	      /* set the decision bit for the tube if the target is in */
	      /* the allowable firing arc for the tube                 */
	      if (disp.IsInFiringArc(host.ship->tubeOffset[i],
				     t->specPar[3],
				     t->specPar[4],
				     game->maxAngles))
		host.ship->SetDefensiveTubeToFire(i);

	    } else { /* beam */

	      /****************************************************************/
	      /* if the host ship has beam weapons, make sure the distance to */
	      /* target is updated for the range checking in SpawnBeam        */
	      /****************************************************************/
	    
	      if (host.ship->avoidPP.frame != game->frame ||
		  host.ship->avoidPP.frame != game->frame-1) {
		host.ship->avoidPP.disp = host.ship->avoidPP.obj.generic->now - host.ship->now;
		host.ship->avoidPP.dist = host.ship->avoidPP.disp.Mag(0);
		host.ship->avoidPP.frame = game->frame;
	      }

	      /* set the beam offset based on predicted angle to target */
	      host.ship->beamOffset[i] =
		(host.ship->avoidPP.disp.Heading(game->maxAngles) -
		 host.ship->heading + 3*game->maxAngles) % game->maxAngles;

	      /* set the decision bit for the beam if the target is in */
	      /* the allowable firing arc for the beam                 */
	      if (disp.IsInFiringArc(host.ship->beamOffset[i],
				     t->specPar[3],
				     t->specPar[4],
				     game->maxAngles))
		host.ship->SetDefensiveTubeToFire(i);
	      else
		host.ship->BlockDefensiveTubeFromFiring(i);
	    }

	  }
	
	} else {
	  /************************************************************/
	  /* tube is active, but does not have enough energy to fire  */
	  /* if it is not on auto-fire, then shut it down...          */
	  /************************************************************/

	  if (!t->CheckTubeMode(ModuleClass::auto_fire_mode)) {
	    t->deactivate();
	  }

	  host.ship->BlockDefensiveTubeFromFiring(i);

	}
      }
    }
  }
}

void BusStruct::FireSelectedWeapons(ShipClass* s) {
  if ( check != 0xAAAAAAAA ) return;
  if ( s->bus.stealthMod && s->IsCloaked()) {
    /* probably does not need a SendUnHideMsg */
    s->bus.stealthMod->deactivate();
    s->stealth = 1;
    return;
  } else if (s->IsWarping())
    return;

  short i;

  for (i = 0; i < numTubes; i++)
    if (s->IsTubeSetToFire(i)) {

      /* store the missile index */
      short missileIndex = (short)tubeMod[i]->par[3];
      if (s->bus.tubeMod[i]->moduleFunc == ModuleClass::beam) {

	/* activate the selected beam */
	s->bus.tubeMod[i]->activate();
	
	tubeMod[i]->FireTube();

	if (!tubeMod[i]->IsActive())
	  s->BlockTubeFromFiring(i);

      } else if (missileIndex != -1) {
	/* activate the selected tube */
	s->bus.tubeMod[i]->activate();
	
	if (game->missileDesign[missileIndex].tracking == dumb ||
	    game->missileDesign[missileIndex].tracking == bomb) {

	  tubeMod[i]->FireTubeAtOffset(s->tubeOffset[i],
				       missileIndex);
	  
	} else if (game->missileDesign[missileIndex].tracking == smart) {
	  
	  tubeMod[i]->FireTubeWithIndex(missileIndex);
	  
	} else {
	  
	  tubeMod[i]->deactivate();
	  
	}

	if (!tubeMod[i]->CheckTubeMode(ModuleClass::auto_fire_mode) ||
	    !tubeMod[i]->IsActive())
	  s->BlockTubeFromFiring(i);
	
      } else {

	tubeMod[i]->deactivate();
	s->BlockTubeFromFiring(i);

      }
      
    }
}

void BusStruct::FireSelectedDefensiveWeapons(ShipClass* s) {
  if ( check != 0xAAAAAAAA ) return;
  if ( s->bus.stealthMod && s->IsCloaked()) {
    /* probably does not need a SendUnHideMsg */
    s->bus.stealthMod->deactivate();
    s->stealth = 1;
    return;
  } else if (s->IsWarping())
    return;

  short i;

  for (i = 0; i < numTubes; i++)
    if (s->IsDefensiveTubeSetToFire(i)) {
      
      /* store the missile index */
      short missileIndex = (short)tubeMod[i]->par[3];
      if (s->bus.tubeMod[i]->moduleFunc == ModuleClass::beam) {

	/* activate the selected beam */
	s->bus.tubeMod[i]->activate();
      
	tubeMod[i]->FireBeam(&(s->avoidPP));

	if (!tubeMod[i]->IsActive())
	  s->BlockDefensiveTubeFromFiring(i);

      } else if (missileIndex != -1) {
	/* activate the selected tube */
	s->bus.tubeMod[i]->activate();
	
	if (game->missileDesign[missileIndex].tracking == dumb ||
	    game->missileDesign[missileIndex].tracking == bomb) {

	  tubeMod[i]->FireTubeAtOffset(s->tubeOffset[i],
				       missileIndex, s->avoidPP.obj.generic);
	  
	} else if (game->missileDesign[missileIndex].tracking == smart) {
	  
	  tubeMod[i]->FireTubeWithIndex(missileIndex, s->avoidPP.obj.generic);
	  
	} else {
	  
	  tubeMod[i]->deactivate();
	  
	}

	s->BlockDefensiveTubeFromFiring(i);

      } else {

	tubeMod[i]->deactivate();
	s->BlockDefensiveTubeFromFiring(i);

      }

    }
}

void BusStruct::ReplenishSystems() {
  if ( check != 0xAAAAAAAA ) return;
  ReloadTubes();
  
  short i;

  //for (i = 0; i < numTubes; i++) {
  //  tubeMod[i]->Replenish();
  //}

  for (i = 0; i < numPorts; i++) {
    dockMod[i]->Replenish();
  }

  if (boostMod)
    boostMod->Replenish();

  if (stealthMod)
    stealthMod->Replenish();

  if (scrambleMod)
    scrambleMod->Replenish();

  if (shieldMod)
    {
    shieldMod->SetStat(shieldMod->store,-shieldMod->par[0]*shieldMod->power);
    //shieldMod->Replenish();
    }

}

void BusStruct::ReloadTubes() {
  if ( check != 0xAAAAAAAA ) return;
  short i;
  for (i = 0; i < numTubes; i++)
    if ( tubeMod[i] )
      {
	/* reload tubes with full complement of missiles */
	if (tubeMod[i]->moduleFunc == ModuleClass::tube ||
	    tubeMod[i]->moduleFunc == ModuleClass::ram)
	  tubeMod[i]->workPar[0] = tubeMod[i]->par[2];
      }
}

void BusStruct::BudgetPower() {
  if ( check != 0xAAAAAAAA ) return;
  powerIn = 0;
  powerOut = 0;
  short i;
  for (i = 0; i < numMod; i++) {
    if (moduleList[i].power < 0) /* object draws power */
      powerIn += moduleList[i].Requirement();
    else                         /* object provides power */
      powerOut += moduleList[i].Withdraw();
  }
  slong powerRefund = powerOut;
  /* power overhead for object */
  slong overhead = numMod * power_overhead_per_slot;
  /* power overhead for object */
  powerRefund -= overhead;
  if (powerRefund < 0) powerRefund = 0;
  if (powerIn > powerRefund) { /* not enough power to fill all systems */
    powerRate = powerRefund * max_power_level / max(1,powerIn);
    /* powerIn = powerRefund; */
  } else if (powerOut)
    powerRate = max_power_level;
  else 
    powerRate = 0;
  for (i = 0; i < numMod; i++) {
    if (moduleList[i].power < 0) {
      moduleList[i].frame++;
      powerRefund -= moduleList[i].Deposit(powerRate);
    }
    
  }
  /* attempt to recover excess allocation */
  i = 0;
  if ( powerMod && powerMod[i] )
    {
      while (powerRefund && i < numPowerPlants)
	{
	  powerMod[i]->Deposit(powerRefund);
	  i++;
	}
    }
  aveFunc = 0;
  for (i = 0; i < numMod; i++) 
    aveFunc += moduleList[i].func;
  aveFunc /= max(1,numMod);
}

void BusStruct::ProcessMessage(DesignedClass* host) {
  if ( check != 0xAAAAAAAA ) return;

  if ( !IsValidObjectPointer(host) )
    {
      return;
    }

  short i;
  short noThrust = IsConnectedClient() &&
    !IsLocalPlayer((ShipClass*)host) &&
    !game->IsRobotPlayer((ShipClass*)host);

  //noThrust = 0;

  switch (msg) {
  case fire_brake:
    if (brakeMod && !noThrust) {
      brakeMod->activate();
      host->PlaySoundOnScreen(brakeMod->activationSoundIndex);
    }
    break;
  case release_brake:
    if (brakeMod) {
      brakeMod->deactivate();
      host->PlaySoundOnScreen(brakeMod->deactivationSoundIndex);
    }
    break;
  case fire_thrust_left:
    if (impulseMod && !noThrust) {
      impulseMod->activate();
      impulseMod->host.designed->AccLeft();
      host->PlaySoundOnScreen(impulseMod->activationSoundIndex);
    }
    break;
  case fire_thrust_right:
    if (impulseMod && !noThrust) {
      impulseMod->activate();
      impulseMod->host.designed->AccRight();
      host->PlaySoundOnScreen(impulseMod->activationSoundIndex);
    }
    break;
  case fire_thrust_reverse:
    if (impulseMod && !noThrust) {
      impulseMod->activate();
      impulseMod->host.designed->AccReverse();
      host->PlaySoundOnScreen(impulseMod->activationSoundIndex);
    }
    break;
  case fire_dock:

    if (dockMod && dockMod[0] && numPorts > 0 && 
	IsValidObjectPointer(dockMod[0]->host.designed, __FILE__, __LINE__) &&
	IsValidObjectPointer(dockMod[0]->host.designed->targetPP.obj.generic, __FILE__, __LINE__) &&
	dockMod[0]->host.designed->targetPP.obj.generic->Type() >= ship) {
      
      dockMod[0]->frame = 0;

      dockMod[0]->activate();
      
      dockMod[0]->workPar[0] = dockMod[0]->host.designed->targetPP.obj.objLong;
    }

    break;

  case fire_boost:
    if (boostMod && !noThrust) {
      boostMod->activate();
      host->PlaySoundOnScreen(boostMod->activationSoundIndex);
    }
  case fire_thrust_forward:
    if (impulseMod && !noThrust) {
      impulseMod->activate();
      impulseMod->host.designed->AccForward();
      host->PlaySoundOnScreen(impulseMod->activationSoundIndex);
    }
    break;
  case release_boost:
    if (boostMod && boostMod->IsActive()) {
      boostMod->deactivate();
      host->PlaySoundOnScreen(boostMod->deactivationSoundIndex);
      //if (sf && sf->mwl && boostMod->activationSoundIndex != -1)
      //	sf->audio->Stop(sf->mwl->Wave(boostMod->activationSoundIndex));
    }
  case release_thrust:
    if (impulseMod) {
      impulseMod->deactivate();
      impulseMod->host.designed->thrustOffset = 0;
      host->PlaySoundOnScreen(impulseMod->deactivationSoundIndex);
      //if (sf && sf->mwl && impulseMod->activationSoundIndex != -1)
      //sf->audio->Stop(sf->mwl->Wave(impulseMod->activationSoundIndex));
    }
    break;
  case release_dock:
    /* dock deactivation */
    if (dockMod && dockMod[0] && numPorts > 0 ) {

      host->PlaySoundOnScreen(dockMod[0]->deactivationSoundIndex);

      dockMod[0]->ReceiveDockDeactivationMsg();
    }
    break;
  case scramble:
    if (scrambleMod) {
      scrambleMod->activate();

    }
    break;
  case fire_cloak:
    if (stealthMod) {
      stealthMod->activate();
      host->PlaySoundOnScreen(stealthMod->activationSoundIndex);
    }
    break;
  case release_cloak:
    if (stealthMod) {
      stealthMod->deactivate();
      stealthMod->host.designed->stealth = 1;
      host->PlaySoundOnScreen(stealthMod->deactivationSoundIndex);
    }
    break;
  case fire_tubes:
    for (i = 0; i < numTubes; i++)
      /* par[4] & manual_fire_mode ==> manual fire only */
      if (tubeMod[i] && 
	  !tubeMod[i]->CheckTubeMode(ModuleClass::manual_fire_mode)) {
	tubeMod[i]->activate();
	host->firingTubes = 1;
      }
    break;
  case fire_tubes_at_angle:
    for (i = 0; i < numTubes; i++) {
      /* par[4] & manual_fire_mode ==> manual fire only */
      if (tubeMod[i] && 
	  !tubeMod[i]->CheckTubeMode(ModuleClass::manual_fire_mode)) {
	tubeMod[i]->activate();
	host->firingTubes = 1;
      }
    }
    break;
  case fire_warp:
    if (warpMod) warpMod->activate();
    break;
  case release_warp:
    if (warpMod) {
      warpMod->deactivate();
      warpMod->host.designed->CheckTransition();
    }
    break;
  case no_message:
  default:
    break;
  }
  msg = no_message;
  msgPriority = 0;
}

slong ModuleClass::PushTo(class GameStateBuffer& gb, GameStateClass* g) {
  if ( check != 0xAAAAAAAA ) return 0;
  char* bufferPos = gb.BufferPosition();

  CALL_PUSH_OBJ(gb,id);
  CALL_PUSH_OBJ(gb,raceID);
  CALL_PUSH_OBJ(gb,power);
  CALL_PUSH_OBJ(gb,output);
  CALL_PUSH_OBJ(gb,volume);
  CALL_PUSH_OBJ(gb,mass);
  CALL_PUSH_OBJ(gb,store);
  CALL_PUSH_OBJ(gb,damageCap);
  CALL_PUSH_OBJ(gb,damage);
  CALL_PUSH_OBJ(gb,nPar);
  CALL_PUSH_OBJ(gb,nSPar);
  CALL_PUSH_OBJ(gb,nWPar);
  CALL_PUSH_OBJ(gb,mode);
  CALL_PUSH_OBJ(gb,debrisBitmapIndex);
  CALL_PUSH_OBJ(gb,activationSoundIndex);
  CALL_PUSH_OBJ(gb,deactivationSoundIndex);
  CALL_PUSH_OBJ(gb,moduleFunc);

  short i;
  for ( i =0; i < nPar; i++) {
    CALL_PUSH_OBJ(gb,par[i]);
  }

  for (i =0; i < nSPar; i++) {
    CALL_PUSH_OBJ(gb,specPar[i]);
  }

  if (!(moduleFunc == dock || moduleFunc == factory)) {
    for (i =0; i < nWPar; i++) {
      CALL_PUSH_OBJ(gb,workPar[i]);
    }
  } else {

    GenericClass* obj = (GenericClass*) workPar[0];
    CALL_PUSH_GAME_OBJ(gb, g, obj);

    for (i = 1; i < nWPar; i++) {
      CALL_PUSH_OBJ(gb,workPar[i]);
    }
  }

  CALL_PUSH_OBJ(gb,active);
  CALL_PUSH_OBJ(gb,frame);
  CALL_PUSH_OBJ(gb,func);
  CALL_PUSH_OBJ(gb,name);

  return gb.BufferPosition() - bufferPos;
}

slong ModuleClass::PopFrom(class GameStateBuffer& gb, GameStateClass* g) {
  if ( check != 0xAAAAAAAA ) return 0;
  char* bufferPos = gb.BufferPosition();

  CALL_POP_OBJ(gb,id);
  CALL_POP_OBJ(gb,raceID);
  CALL_POP_OBJ(gb,power);
  CALL_POP_OBJ(gb,output);
  CALL_POP_OBJ(gb,volume);
  CALL_POP_OBJ(gb,mass);
  CALL_POP_OBJ(gb,store);
  CALL_POP_OBJ(gb,damageCap);
  CALL_POP_OBJ(gb,damage);
  CALL_POP_OBJ(gb,nPar);
  CALL_POP_OBJ(gb,nSPar);
  CALL_POP_OBJ(gb,nWPar);
  CALL_POP_OBJ(gb,mode);
  CALL_POP_OBJ(gb,debrisBitmapIndex);
  CALL_POP_OBJ(gb,activationSoundIndex);
  CALL_POP_OBJ(gb,deactivationSoundIndex);
  CALL_POP_OBJ(gb,moduleFunc);

  short i;
  for ( i =0; i < nPar; i++) {
    CALL_POP_OBJ(gb, par[i]);
  }

  for (i =0; i < nSPar; i++) {
    CALL_POP_OBJ(gb, specPar[i]);
  }

  if (!(moduleFunc == dock || moduleFunc == factory))
    for (i =0; i < nWPar; i++) {
      CALL_POP_OBJ(gb, workPar[i]);
    }
  else {

    GenericClass* obj;
    CALL_POP_GAME_OBJ(gb, g, obj);

    workPar[0] = slong(obj);

    for (i = 1; i < nWPar; i++)
      CALL_POP_OBJ(gb, workPar[i]);
  }

  CALL_POP_OBJ(gb,active);
  CALL_POP_OBJ(gb,frame);
  CALL_POP_OBJ(gb,func);
  CALL_POP_OBJ(gb,name);

  checkSum = CheckSum();

  return gb.BufferPosition() - bufferPos;
}

short ModuleClass::get(ifstream& f, SFString* nameList,
		       SFString* valueList, short nList) {
  if ( check != 0xAAAAAAAA ) return 0;
  char Class[max_name_char];
  char func[max_name_char];
  char activationSoundStr[max_name_char], deactivationSoundStr[max_name_char];
  //char pfunc[max_name_char];
  char c;
  char* s;
  char cId[max_name_char];

  f >> c;
  if (c != '}') {
    f.putback(c);
    f >> Class >> name >> volume >> mass >> power >> damageCap >>
      activationSoundStr >> deactivationSoundStr >> func;
    f >> cId;

    checkSum = CheckSum();

    if (nameList && valueList && nList) {
     
      Parse(name, nameList, valueList, nList);
      Parse(func, nameList, valueList, nList);
      Parse(cId, nameList, valueList, nList);

    }

    id = atol(cId);

    raceID = id / 10000;
    activationSoundIndex = game->IndexOfSound(activationSoundStr, module_type, raceID);
    deactivationSoundIndex = game->IndexOfSound(deactivationSoundStr, module_type, raceID);

    short i = 0;
    char* pfunc = func;
    /* strip off the module function name */
    if ((s = strstr(pfunc,"*")) != 0) {
      *s = '\0';
      s++;
    }
    /* strip off the module function parameters */
    while (s && i < max_module_par) {
      pfunc = s;
      s = strstr(pfunc,"*");
      if (s) {
	*s = '\0';
	s++;
      }
      SetStat(par[i],atol(pfunc));
      //par[i] = atol(pfunc);
      i++;
    }

    if (strcmp(func,"power") == 0) {
      moduleFunc = power_plant;
      SetStat(store,par[0] * power);
      //store = par[0] * power;
      nPar = 1;
      nSPar = 0;
      nWPar = 0;
      checkSum = CheckSum();
    }
    else if (strcmp(func,"fuel") == 0) {
      moduleFunc = fuel_cell;
      SetStat(store,par[0] * power);
      //store = par[0] * power;
      nPar = 1;
      nSPar = 0;
      nWPar = 0;
      checkSum = CheckSum();
    } else if (strcmp(func,"batt") == 0) {
      moduleFunc = battery;
      SetStat(store,par[0] * power);
      //store = par[0] * power;
      nPar = 1;
      nSPar = 0;
      nWPar = 0;
      checkSum = CheckSum();
    } else if (strcmp(func,"eng") == 0) {
      moduleFunc = engine;
      MultToStat(par[0],unit_thrust / g_scale);
      //par[0] *= unit_thrust / g_scale;
      nPar = 1;
      nSPar = 0;
      nWPar = 0;
      checkSum = CheckSum();
    } else if (strcmp(func,"qdrv") == 0) {
      moduleFunc = qdrv;
      MultToStat(par[0],game->lightSpeed);
      //par[0] *= game->lightSpeed;
      nPar = 2;
      nSPar = 0;
      nWPar = 0;
      checkSum = CheckSum();
    } else if (strcmp(func,"tube") == 0) {
      /************************************************/
      /* tube weapon:                                 */
      /************************************************/
      /* par:                                         */
      /* par[0]: number of frames of power in storage */
      /* par[1]: cost per frame to fire               */
      /* par[2]: number of missiles                   */
      /* par[3]: missile index                        */
      /* par[4]: mode (same as tube)                  */
      /************************************************/
      /* specPar:                                     */
      /* par[0]: angular position on bitmap           */
      /* par[1]: radial position on bitmap            */
      /* par[2]: angle of fire                        */
      /* par[3]: starting angle in firing arc         */
      /* par[4]: ending angle in firing arc           */
      /************************************************/
      moduleFunc = tube;
      workPar[0] = par[2];  /* load tube with full complement of ammo */
      nPar = 5; 
      nSPar = 5;
      nWPar = 1; //check this
      checkSum = CheckSum();
      Replenish();
    } else if (strcmp(func,"beam") == 0) {
      /************************************************/
      /* beam weapon:                                 */
      /* par[0]: number of frames of power in storage */
      /* par[1]: cost per frame to fire               */
      /* par[2]: maximum range                        */
      /* par[3]: (not used)                           */
      /* par[4]: mode (same as tube)                  */
      /************************************************/
      /* specPar:                                     */
      /* par[0]: angular position on bitmap           */
      /* par[1]: radial position on bitmap            */
      /* par[2]: angle of fire (not used)             */
      /* par[3]: starting angle in firing arc         */
      /*         (not used)                           */
      /* par[4]: ending angle in firing arc           */
      /*         (not used)                           */
      /************************************************/
      moduleFunc = beam;
      nPar = 5;  //check this
      nSPar = 5; //check this
      nWPar = 2; //check this
      checkSum = CheckSum();
      Replenish();
    } else if (strcmp(func,"ram") == 0) {
      /************************************************/
      /* ram weapon:                                  */
      /* par[0]: number of frames of power in storage */
      /* par[1]: cost per frame to fire               */
      /* par[2]: number of rams                       */
      /* par[3]: missile index                        */
      /* par[4]: mode (same as tube)                  */
      /************************************************/
      /* specPar:                                     */
      /* par[0]: angular position on bitmap           */
      /* par[1]: radial position on bitmap            */
      /* par[2]: angle of fire                        */
      /* par[3]: starting angle in firing arc         */
      /*         (not used)                           */
      /* par[4]: ending angle in firing arc           */
      /*         (not used)                           */
      /************************************************/
      moduleFunc = ram;
      workPar[0] = par[2];  /* load tube with full complement of ammo */
      nPar = 5;  //check this
      nSPar = 3; //do not need firing arc
      nWPar = 1; //check this
      checkSum = CheckSum();
      Replenish();
    } else if (strcmp(func,"shield") == 0) {
      moduleFunc = shield;
      nPar = 1;
      nSPar = 0;
      nWPar = 0;
      checkSum = CheckSum();
      Replenish();
    } else if (strcmp(func,"armor") == 0) {
      moduleFunc = armor;
      nPar = 1;
      nSPar = 0;
      nWPar = 0;
      checkSum = CheckSum();
      Replenish();
    } else if (strcmp(func,"brake") == 0) {
      moduleFunc = brake;
      nPar = 1;
      nSPar = 0;
      nWPar = 0;
      checkSum = CheckSum();
    } else if (strcmp(func,"sensor") == 0) {
      moduleFunc = sensor;
      nPar = 1;
      nSPar = 0;
      nWPar = 0;
      checkSum = CheckSum();
    } else if (strcmp(func,"cm") == 0) {
      moduleFunc = counter_measures;
      nPar = 2;
      nSPar = 0;
      nWPar = 0;
      checkSum = CheckSum();
      Replenish(); 
    } else if (strcmp(func,"stealth") == 0) {
      moduleFunc = cloak;
      nPar = 2;
      nSPar = 0;
      nWPar = 0;
      checkSum = CheckSum();
      Replenish();
    } else if (strcmp(func,"boost") == 0) {
      moduleFunc = boost;
      nPar = 3;
      nSPar = 0;
      nWPar = 0;
      checkSum = CheckSum();
      Replenish();
    } else if (strcmp(func,"dock") == 0) {
      /************************************************/
      /* dock:                                        */
      /************************************************/
      /* par:                                         */
      /* par[0]: number of frames of power in storage */
      /* par[1]: thrust parameter (same units as      */
      /*         engine)                              */
      /* par[2]: range in pixels                      */
      /************************************************/
      /* specPar:                                     */
      /* par[0]: angular position on bitmap           */
      /* par[1]: radial position on bitmap            */
      /* par[2]: angle of docked target               */
      /************************************************/
      moduleFunc = dock;
      MultToStat(par[1],unit_thrust / g_scale);
      //par[1] *= unit_thrust / g_scale;
      nPar = 3;
      nSPar = 3;
      nWPar = 2;
      checkSum = CheckSum();
      Replenish();
    } else if (strcmp(func,"factory") == 0) {
      moduleFunc = factory;
      nPar = 1;
      nSPar = 4; /* 1: production ; 3: temporary port number assignment */
      nWPar = 2;
      checkSum = CheckSum();
    }
  }

  /***********************************************************************/
  /* assign a defualt name                                               */
  /* note: the names of tubes should not be assign default values until  */
  /*       the missile designs are loaded                                */
  /***********************************************************************/

  if (moduleFunc != tube && strcmp(name, "*") == 0)
    sprintf(name, "%s:%ld", funcName[moduleFunc], id%10);
  
  return (c != '}' && f.good());
}

slong RankStruct::PushTo(class GameStateBuffer& gb, GameStateClass* g) {

  char* bufferPos = gb.BufferPosition();

  CALL_PUSH_OBJ(gb,activeScore);
  CALL_PUSH_OBJ(gb,totalScore);
  CALL_PUSH_OBJ(gb,totalKills);
  CALL_PUSH_OBJ(gb,duty);
  CALL_PUSH_OBJ(gb,dutyInterval);
  CALL_PUSH_OBJ(gb,accBonus);
  CALL_PUSH_OBJ(gb,name);

  return gb.BufferPosition() - bufferPos;

}

slong RankStruct::PopFrom(class GameStateBuffer& gb, GameStateClass* g) {
  char* bufferPos = gb.BufferPosition();

  CALL_POP_OBJ(gb,activeScore);
  CALL_POP_OBJ(gb,totalScore);
  CALL_POP_OBJ(gb,totalKills);
  CALL_POP_OBJ(gb,duty);
  CALL_POP_OBJ(gb,dutyInterval);
  CALL_POP_OBJ(gb,accBonus);
  CALL_POP_OBJ(gb,name);

  return gb.BufferPosition() - bufferPos;
}

short RankStruct::get(ifstream& f,
		      SFString* nameList, SFString* valueList,
		      short nList) {

  char c;
  short level;
  slong longInterval, longAccBonus;
  char dutyStr[16];

  f >> c;
  if (c != '}') {
    f.putback(c);
    f >> name >> level >> activeScore >> totalScore >> totalKills >> dutyStr >>
      longInterval >> longAccBonus;
    
    dutyInterval = (short)longInterval;
    accBonus = (short)longAccBonus;

    short i=0;
    while (i < last_detail && strcmp(dutyStr, detailName[i]))
      i++;
	  
    if (i == last_detail) {
      logFile << "ERROR> unknown duty (" << dutyStr << ") assigned to rank " << name << endl;
      return 0;
    } else
      duty = i;

  }
  return (c != '}' && f.good());
}

void RaceClass::profile(GameStateClass* g, ostream* o) {
  if (!o)
    o = &logFile;

  *o << "Profile of   " << name << endl;
  *o << "id:          " << id << endl;
  *o << "home:        " << home << " ("  << 
    g->planet[home].name << ")" << endl;
  
  /* convert the growth rate to a doubling time */
  slong dt = nlong(log((double)2) / log(growthRate / 1e5) / 60); /* in seconds */
  
  *o << "doubling time: " << dt << " seconds" << endl;
}

char* RaceClass::ProfileCondition() {

  static char line[80], line1[80];
  switch (conditionType) {
  case hold_num:
    sprintf(line1, "hold %ld planets", conditionNumPar);
    break;
  case hold_list:
    sprintf(line1, "hold (%s) planets", GetPlanetList(0));
    break;
  case conquer_num:
     sprintf(line1, "conquer %ld planets", conditionNumPar);
     break;
  case conquer_list:
    sprintf(line1, "conquer (%s) planets", GetPlanetList(0));
    break;
  }

  /* now concatenate the timing conditions */
  switch (conditionTiming) {
  case within_time:
    if (conditionTimePar < 0) /* time unit is "lives" */
      sprintf(line, "\"%s within %d lives\"", line1, -conditionTimePar);
    else if ((conditionTimePar / 60) * 60 == conditionTimePar)
      /* give time in minutes */
      sprintf(line, "\"%s within %d minutes\"", line1, conditionTimePar/60);
    else
      /* give time in seconds */
      sprintf(line, "\"%s within %d seconds\"", line1, conditionTimePar);
    break;
  case for_time:
    if ((conditionTimePar / 60) * 60 == conditionTimePar) /* give time in min */
      sprintf(line, "\"%s for %d minutes\"", line1, conditionTimePar/60);
    else
      sprintf(line, "\"%s for %d seconds\"", line1, conditionTimePar);
    break;
  case unlimited_time:
     sprintf(line, "\"%s\"", line1);
    break;
  }

  return line;
}

short RaceClass::CalculateVictoryPct() {
  if (status == hopeless)
    return 0;

  slong pc = 0;
  short held, n;

  switch (conditionType) {
  case hold_num:
  case conquer_num:
    if (conditionNumPar)
      pc = numPlanets * 100 / conditionNumPar;
    else
      return 100;
    break;
  case hold_list:
  case conquer_list:
    n = NumPlanetsInVictoryList(&held);
    if (n)
      pc = held * 100 / n;
    else
      return 100;
    break;
  }

  return (short)pc;
  
}

short RaceClass::NumPlanetsInVictoryList(short* held) {
  if (conditionType == hold_list || conditionType == conquer_list) {
    short i=0, j=0;
    slong n;
    if (held)
      *held = 0;
    slong listFlags = conditionNumPar;
    while (listFlags) {
      n = 1L << i;
      if (conditionNumPar & n) {
	j++;
	listFlags = listFlags ^ (listFlags & n);
	if (held && game->planet[i].raceID == id)
	  (*held)++;
      }
      i++;
    }
    return j;
  } else
    return 0;
}

char* RaceClass::GetPlanetList(short num) {
  static char list[256], element[max_name_char];
  *list = '\0';
  short i;
  slong n;
  slong listFlags = conditionNumPar;
  for (i=0; i<game->numPlanets; i++) {
    n = 1L << i;
    if (conditionNumPar & n) {
      listFlags = listFlags ^ (listFlags & n);
      if (num)
	sprintf(element, "%d", i);
      else
	strcpy(element, game->planet[i].name);
      strcat(list, element);
      if (listFlags) /* there are more elements in the list */
	strcat(list, ",");
    }
  }
  return list;
}

short RaceClass::get(ifstream& f,
		     SFString* nameList, SFString* valueList,
		     short nList) {

  char c;
  static char hm[max_name_char];
  static char debOff[max_name_char];
  static char cBonus[max_name_char];
  slong ltemp, lFullColonyPct;
  double dt, datmP, dgrav;

  f >> c;

  if (c != '}') {

      f.putback(c);

      f >> name >> id >> hm >> dt >> debOff >> ltemp >> datmP >> dgrav >> cBonus >> lFullColonyPct;

      if (nameList && valueList && nList) {
	
	Parse(name, nameList, valueList, nList);
	Parse(hm, nameList, valueList, nList);
	
      }

      fullColonyPct = short(lFullColonyPct);

      SFString conditionString;
      conditionString.ReadFrom(f);

      if (nameList && valueList && nList) {
	
	Parse(hm, nameList, valueList, nList);
	Parse(debOff, nameList, valueList, nList);
	Parse(cBonus, nameList, valueList, nList);
	Parse(conditionString, nameList, valueList, nList);
	
      }

      conditionString = conditionString.strip();

      bonus = atol(cBonus);

      /* type of victory conditions */

      SFString parString;

      if (conditionString.match("conquer * planet*")) {
	conditionString = conditionString.rem("conquer ");
	/* read number of planets to conquer */
	parString.ReadFrom(conditionString);
	if (parString.match("(*)")) {
	  conditionType = conquer_list;
	} else {
	  conditionType = conquer_num;
	  conditionNumPar = parString.Short();
	}
	conditionString = conditionString.rem("planets");
	if (conditionString.Index("within")) {
	  /* conditions are timed */
	  conditionTiming = within_time;
	  conditionString = conditionString.rem("within");
	  parString = SFString();
	  parString.ReadFrom(conditionString);
	  conditionTimePar = parString.Short();
	  parString = SFString();
	  parString.ReadFrom(conditionString);
	  if (parString == "minutes")
	    conditionTimePar = conditionTimePar * 60;
	  else if (parString == "seconds")
	    conditionTimePar = parString.Short();
	  else if (parString == "lives")
	    /*                                                     */
	    /* if the conditions must be met within a set number   */
            /* of player lives then we will use a negative timePar */
	    /* condition to signify that the time unit is "lives"  */
	    /*                                                     */
	    conditionTimePar = -conditionTimePar;
	} else
	  conditionTiming = unlimited_time;
	
      } else if (conditionString.match("hold * planet*")) {
	conditionString = conditionString.rem("hold ");
	/* read number of planets to hold */
	parString.ReadFrom(conditionString);
	if (parString.match("(*)")) {
	  SFString* tokenList = 0;
	  short nplan = parString.TokenList(tokenList, " ,()");
	  short i,n;
	  for (i=0; i<nplan; i++) {

            n = 0;
            char *out = tokenList[i].out();
            if ( out ) {
	      n = game->IndexOfPlanet(out);
	      delete out;
	      out = NULL;
	    }

	    if (n)
	      conditionNumPar = conditionNumPar | (1L << n);
	    else
	      logFile << "ERROR> victory list planet index of zero detected for planet " <<
		tokenList[i] << " and race " << name << endl;
	  }
	  delete[] tokenList;
	  conditionType = hold_list;
	  
	} else {
	  conditionType = hold_num;
	  conditionNumPar = parString.Short();
	}
	conditionString = conditionString.rem("planets");
	if (conditionString.Index("for")) {
	  /* conditions have time limit */
	  conditionTiming = for_time;
	  conditionString = conditionString.rem("for");
	  parString = SFString();
	  parString.ReadFrom(conditionString);
	  conditionTimePar = parString.Short();
	  parString = SFString();
	  parString.ReadFrom(conditionString);
	  if (parString == "minutes")
	    conditionTimePar = conditionTimePar * 60;
	  else if (parString == "seconds")
	    conditionTimePar = parString.Short();
	} else
	  conditionTiming = unlimited_time;
      }

    }
    
  /* convert the doubling time into a growth rate */
  growthRate =
    max(nlong(exp(1.0 / (dt * 60 /* convert minutes to seconds */ 
			 / log((double)2) /* divide by log of growth factor */ )) *
	      100000 /* growth rate base = 100000 */),
	100001 /* make sure that numerical resolution doesn't cause trouble*/
	); 
  
  home = game->IndexOfPlanet(hm);
  
  if (home == 0)
    logFile << "ERROR> home planet index of zero detected for race " <<
      name << endl;
  
  prefTemp = short(ltemp);
  prefAtmP = short(datmP * 100.00001);
  prefGrav = short(dgrav * 100.00001);
  
  short l = strlen(debOff);
  
  if (debOff[l-1] == '*') {
    debOff[l-1] = '\0';
    slong offSetM = atol(debOff);
    debrisOffset = short(game->debrisCount * offSetM);
  } else
    debrisOffset = short(atol(debOff));
  
  if (!f) {
    logFile << "ERROR> RaceClass::get: input error -- partial line reads:" << endl;
    logFile << "  " << name << " " << id << " " <<
      slong(hm) << " " << dt << endl;
  }

  return (c != '}' && f.good());
}

slong RaceClass::PushTo(class GameStateBuffer& gb, GameStateClass* g) {

  char* bufferPos = gb.BufferPosition();

  CALL_PUSH_OBJ(gb,id);
  CALL_PUSH_OBJ(gb,home);
  CALL_PUSH_OBJ(gb,growthRate);
  CALL_PUSH_OBJ(gb,totPop);
  CALL_PUSH_OBJ(gb,targetIndex);
  CALL_PUSH_OBJ(gb,targetIndexRank);
  CALL_PUSH_OBJ(gb,harvestIndex);
  CALL_PUSH_OBJ(gb,numRobots);
  CALL_PUSH_OBJ(gb,numPlanets);
  CALL_PUSH_OBJ(gb,maxPlanetRobots);
  CALL_PUSH_OBJ(gb,maxPopRobots);
  CALL_PUSH_OBJ(gb,debrisOffset);
  CALL_PUSH_OBJ(gb,prefTemp);
  CALL_PUSH_OBJ(gb,prefAtmP);
  CALL_PUSH_OBJ(gb,prefGrav);
  CALL_PUSH_OBJ(gb,condition);
  CALL_PUSH_OBJ(gb,conditionType);
  CALL_PUSH_OBJ(gb,conditionTiming);
  CALL_PUSH_OBJ(gb,conditionSeconds);
  CALL_PUSH_OBJ(gb,conditionNumPar);
  CALL_PUSH_OBJ(gb,conditionTimePar);
  CALL_PUSH_OBJ(gb,numPlayerDeaths);
  CALL_PUSH_OBJ(gb,status);
  CALL_PUSH_OBJ(gb,bonus);
  CALL_PUSH_OBJ(gb,strategicPoints);
  CALL_PUSH_OBJ(gb,activeFrames);
  CALL_PUSH_OBJ(gb,fullColonyPct);
  CALL_PUSH_GAME_OBJ(gb, g,distressObj.generic);
  CALL_PUSH_OBJ(gb,name);

  return gb.BufferPosition() - bufferPos;
}

slong RaceClass::PopFrom(class GameStateBuffer& gb, GameStateClass* g) {

  char* bufferPos = gb.BufferPosition();

  CALL_POP_OBJ(gb,id);
  CALL_POP_OBJ(gb,home);
  CALL_POP_OBJ(gb,growthRate);
  CALL_POP_OBJ(gb,totPop);
  CALL_POP_OBJ(gb,targetIndex);
  CALL_POP_OBJ(gb,targetIndexRank);
  CALL_POP_OBJ(gb,harvestIndex);
  CALL_POP_OBJ(gb,numRobots);
  CALL_POP_OBJ(gb,numPlanets);
  CALL_POP_OBJ(gb,maxPlanetRobots);
  CALL_POP_OBJ(gb,maxPopRobots);
  CALL_POP_OBJ(gb,debrisOffset);
  CALL_POP_OBJ(gb,prefTemp);
  CALL_POP_OBJ(gb,prefAtmP);
  CALL_POP_OBJ(gb,prefGrav);
  CALL_POP_OBJ(gb,condition);
  CALL_POP_OBJ(gb,conditionType);
  CALL_POP_OBJ(gb,conditionTiming);
  CALL_POP_OBJ(gb,conditionSeconds);
  CALL_POP_OBJ(gb,conditionNumPar);
  CALL_POP_OBJ(gb,conditionTimePar);
  CALL_POP_OBJ(gb,numPlayerDeaths);
  CALL_POP_OBJ(gb,status);
  CALL_POP_OBJ(gb,bonus);
  CALL_POP_OBJ(gb,strategicPoints);
  CALL_POP_OBJ(gb,activeFrames);
  CALL_POP_OBJ(gb,fullColonyPct);
  CALL_POP_GAME_OBJ(gb, g,distressObj.generic);
  CALL_POP_OBJ(gb,name);

  return gb.BufferPosition() - bufferPos;
}

PlayerClass::PlayerClass() {
  *name = '\0';
  *login = '\0';
  *shipName = '\0';
  *ipAddress = '\0';
  serial = 0;
  raceId = 0;
  revertRaceId = 0;
  shipId = 0;
  shipIndex = 0;
  numMissions = 0;
  numKills = 0;
  numKillsTotal = 0;
  numKilled = 0;
  numKilledThisSession = 0;
  rank = 0;
  mode = 0; /* slot is available */
  totalScore = 0;
  activeScore = 0;
  activeFrames = 0;
  secondsOnline = 0;
  strategicPoints = 0;
  refValue = 0;
  checkSum = 0;
}

PlayerClass::PlayerClass(char* na, char* li, char* sn, char *ip,slong iserial,
			 slong rid, slong sid, slong tscore,
			 ulong to, slong nk, slong nkd,
			 char r, char demo,slong rv) {
  if (na)
    strcpy(name, na);
  if (li)
    strcpy(login, li);
  if (sn)
    strcpy(shipName, sn);
  if ( ip )
    {
      strcpy(ipAddress,ip);
    }

  serial = iserial;
  raceId = rid;
  revertRaceId = raceId;
  shipId = sid;
  shipIndex = 0; /* to be mapped later */
  numMissions = 0; /* this information is no longer passed */
  numKills = 0;
  numKillsTotal = nk;
  numKilled = nkd;
  numKilledThisSession = 0;
  rank = r;
  if (demo == TRUE)
    mode = demo_mode;
  else
    mode = 0;
  totalScore = tscore;
  activeScore = 0;
  activeFrames = 0;
  secondsOnline = to;
  strategicPoints = 0;
  refValue = rv;
  checkSum = CheckSum();
  //profile(game);
}



void
ZeroLocalStats()
{
  int pi = game->ship[localSlotNumber].playerIndex;
  game->player[pi].activeScore = 0;
  game->player[pi].totalScore = 0;
  game->player[pi].strategicPoints = 0;
  game->player[pi].numKills = 0;
  game->player[pi].numKillsTotal = 0;
  game->player[pi].numKilled = 0;
  game->player[pi].numKilledThisSession = 0;
  game->player[pi].refValue = 0;
  game->player[pi].rank = 0;
  sf->connectedThroughNexus = FALSE;
}


void
SelfKick()
{
  S_String msg("RAM Cheat Detected!!!");
  retMsgBuffer.PushMsg((uchar)SF_MSG_DISCONNECT,
		       (ushort)localSlotNumber,
		       msg.str,
		       (int)S_CLIENT_TCP);
}


void
PrintRAMCheatMsg()
{
  PushMessageNew(FROM_GOD, TO_BROADCAST, "RAM Cheat detected.  Zeroing stats");
  SetScreenMessage("*",CLR_YELLOW);
  SetScreenMessage("* ZEROING STATS",CLR_YELLOW);
  SetScreenMessage("* RAM CHEAT DETECTED",CLR_YELLOW);
  SetScreenMessage("*",CLR_YELLOW);

  int pi = game->ship[localSlotNumber].playerIndex;
  S_String tmp("Server> %s (slot %d) using RAM Cheat.  Zeroing stats...",
	       game->player[pi].name,
	       localSlotNumber);
  retMsgBuffer.PushMsg((uchar)SF_MSG_PLAYERCHAT_N,
		       (ushort)0,
		       (uchar)SF_MSG_CHAT_TOALL,
		       (uchar)0,
		       tmp.str,
		       (int)S_CLIENT_TCP);
}


void HandleCheckSumFail() {
  if ( IsConnectedClient() )
    {
      ZeroLocalStats();
      PrintRAMCheatMsg();
      SelfKick();
    }
}

void PlayerClass::profile(GameStateClass* g) {
  logFile << "Player Profile (slot " << g->player.indexOf(this) << ")" << endl;
  logFile << "name:          " << name << endl;
  logFile << "login:         " << login << endl;
  logFile << "ship name:     " <<  shipName << endl;
  logFile << "race:          " << g->race[raceId].name << endl;
  logFile << "original race: " << g->race[raceId].name << endl;
  logFile << "ship ID:       " << shipId << " (" <<
    g->shipDesign[shipIndex].name << ")" << endl;
  logFile << "ship index:    " << shipIndex << endl;
  logFile << "rank:          " << g->rank[rank].name << endl;
  logFile << "missions:      " << numMissions << endl;
  logFile << "active kills:  " << numKills << endl;
  logFile << "total kills:   " << numKillsTotal << endl;
  logFile << "ship losses:   " << numKilled << endl;
  logFile << "recent losses: " << numKilledThisSession << endl;
  logFile << "total score:   " << totalScore << endl;
  logFile << "active score:  " << activeScore << endl;
  logFile << "active frames: " << activeFrames << endl;
  logFile << "mode flags:    " << short(mode) << endl;
}

short PlayerClass::get(ifstream& f,
		       SFString* nameList, SFString* valueList,
		       short nList) {

  char c, raceStr[max_name_char];

  f >> c;
  if (c != '}') {
    f.putback(c);
    f >> name >> login >> rank >> raceStr >> shipId >> shipName >>
      numMissions >> numKills;

    raceId = game->IndexOfRace(raceStr);

    mode = 0;

    SetStat(numKilled,0);
    //numKilled = 0;

    /* set the original alignment so that when it can be reset when the */
    /* the game state resets                                            */
    revertRaceId = raceId;

  }
  return (c != '}' && f.good());
}

int PlayerClass::CalculateRank(short slot, short resetRank) {
  /* determine the rank a player should have based on score */
  /* active score counts twice                              */

  //long points = numKills + max(numKillsTotal - numKilled, 0);

  if ( game->ship[slot].IsPending(ShipClass::promotion) )
    {
      return rank;
    }

  if ( (IsDisconnectedClient() ||
	(IsConnectedClient() && IsLocalPlayer(game->ship + slot)) ||
	(IsServer() && game->IsRobotPlayer(game->ship + slot))) ) {
    
    ulong delta = totalScore - min(numKilled, totalScore);
    ulong points = activeScore + delta;
    
    if ( rank > game->numRanks )
      {
	SetStat(rank,game->numRanks-1);
      }
    
    short newRank = rank;
    if (resetRank)
      newRank = 0; /* this allows for demotions */
    while (newRank < game->numRanks-1 &&
	   numKillsTotal >= game->rank[newRank+1].totalKills &&
	   (activeScore >= game->rank[newRank+1].activeScore && 
	    !resetRank || resetRank) &&
	   points >= game->rank[newRank+1].totalScore) {
      
      newRank++;
      
    }
      
    if (newRank != rank) {
      
      logFile << "sent> the rank of " << game->rank[rank].name << " " << name << " has been changed to " <<
	game->rank[newRank].name << " on frame " << game->frame << endl;

      game->ship[slot].SetPendingFlag(ShipClass::promotion);

      retMsgBuffer.PushMsg((uchar)SF_MSG_SETRANKINDEX,
			   (ushort)slot,
			   (uchar)min(newRank, game->numRanks),
			   S_CLIENT_TCP);
    }
    return newRank;
  }
  return rank;
}

void PlayerClass::ReceiveRankingMsg(short slot, short rankIndex) {

  if ( raceId < 0 || raceId >= game->numRaces ) {
    logFile << "ERROR> PlayerClass::ReceiveRankingMsg bad raceId "
	    << raceId << endl;
    return;
  }
  
  if ( rankIndex < 0 || rankIndex >= game->numRanks ) {
    logFile << "ERROR> PlayerClass::ReceiveRankingMsg bad rankIndex "
	    << rankIndex << endl;
    return;
  }


  static char msg[1024];  

  game->ship[slot].ClearPendingFlag(ShipClass::promotion);

  game->ship[slot].PlaySoundOnScreen(game->soundEvent[player_promoted_event]);

  if (slot == localSlotNumber) {
    if (rank < rankIndex) {
      if (game->PlayerIsUpgradeable(game->ship[slot].playerIndex))
	sprintf(msg, "Server> You have been promoted to %s.  Proceed to space station to refit",
		game->rank[rankIndex].name);
      else
	sprintf(msg, "Server> You have been promoted to %s.",
		game->rank[rankIndex].name);
    } else {

      sprintf(msg, "Server> You have been demoted to %s.",
	      game->rank[rankIndex].name);
      
    }
    
    game->PostGameMessage(msg, slot, (short)raceId, personal);
    
  } else {

    if (rank < rankIndex) {
      sprintf(msg, "%s has been promoted to %s", name,
	      game->rank[rankIndex].name);
    } else {
      sprintf(msg, "%s has been demoted to %s", name,
	      game->rank[rankIndex].name);
    }

    game->PostGameMessage(msg, (short)raceId, (short)raceId, broadcast);
    
  }
  
  logFile << "received> the rank of " << game->race[game->playerList[slot]->raceId].name <<
    " " << game->rank[game->playerList[slot]->rank].name <<
    " " << game->playerList[slot]->name << " has been changed to " <<
    game->rank[rankIndex].name << " on frame " << game->frame << endl;
  
  SetStat(rank, min(rankIndex, game->numRanks-1));
  //rank = min(rankIndex, game->numRanks-1);
}

slong PlayerClass::PushTo(class GameStateBuffer& gb, GameStateClass* g) {

  char* bufferPos = gb.BufferPosition();

  CALL_PUSH_OBJ(gb,raceId);  
  CALL_PUSH_OBJ(gb,revertRaceId);
  CALL_PUSH_OBJ(gb,rank);    
  CALL_PUSH_OBJ(gb,shipId);  
  CALL_PUSH_OBJ(gb,shipIndex);  
  CALL_PUSH_OBJ(gb,numMissions);
  CALL_PUSH_OBJ(gb,numKills);
  CALL_PUSH_OBJ(gb,numKillsTotal);
  CALL_PUSH_OBJ(gb,numKilled);
  CALL_PUSH_OBJ(gb,numKilledThisSession);
  CALL_PUSH_OBJ(gb,activeFrames);
  CALL_PUSH_OBJ(gb,secondsOnline);
  CALL_PUSH_OBJ(gb,strategicPoints);
  CALL_PUSH_OBJ(gb,refValue);
  CALL_PUSH_OBJ(gb,activeScore);
  CALL_PUSH_OBJ(gb,totalScore);
  CALL_PUSH_OBJ(gb,mode);
  CALL_PUSH_OBJ(gb,name);
  CALL_PUSH_OBJ(gb,login);
  CALL_PUSH_OBJ(gb,shipName);

  return gb.BufferPosition() - bufferPos;

}

slong PlayerClass::PopFrom(class GameStateBuffer& gb, GameStateClass* g) {

  char* bufferPos = gb.BufferPosition();

  CALL_POP_OBJ(gb,raceId);  
  CALL_POP_OBJ(gb,revertRaceId);
  CALL_POP_OBJ(gb,rank);    
  CALL_POP_OBJ(gb,shipId);  
  CALL_POP_OBJ(gb,shipIndex);  
  CALL_POP_OBJ(gb,numMissions);
  CALL_POP_OBJ(gb,numKills);
  CALL_POP_OBJ(gb,numKillsTotal);
  CALL_POP_OBJ(gb,numKilled);
  CALL_POP_OBJ(gb,numKilledThisSession);
  CALL_POP_OBJ(gb,activeFrames);
  CALL_POP_OBJ(gb,secondsOnline);
  CALL_POP_OBJ(gb,strategicPoints);
  CALL_POP_OBJ(gb,refValue);
  CALL_POP_OBJ(gb,activeScore);
  CALL_POP_OBJ(gb,totalScore);
  CALL_POP_OBJ(gb,mode);
  CALL_POP_OBJ(gb,name);
  CALL_POP_OBJ(gb,login);
  CALL_POP_OBJ(gb,shipName);

  return gb.BufferPosition() - bufferPos;
}

short PopStruct::get(ifstream& f,
		     SFString* nameList, SFString* valueList,
		     short nList) {

  char c;
  double p;

  f >> c;
  if (c != '}') {
    f.putback(c);
    f >> p;
    *this = PopStruct(p);
  }
  return (c != '}' && f.good());
}

short ColonyClass::get(ifstream& f,
		       SFString* nameList, SFString* valueList,
		       short nList) {

  char c;

  f >> c;
  if (c != '}') {
    f.putback(c);

    GetSharedData(f, colony, nameList, valueList, nList);

  }
  return (c != '}' && f.good());
}

short ShipClass::get(ifstream& f,
		     SFString* nameList, SFString* valueList,
		     short nList) {

  char c;
  char playerStr[max_name_char];

  f >> c;
  if (c != '}') {
    f.putback(c);

    f >> playerStr;

    playerIndex = game->IndexOfPlayer(playerStr);

    if (playerIndex < 0)
      {
	logFile << "ERROR> unable to map " << playerStr << " to a valid player." << endl;
      }

    GetSharedData(f, ship, nameList, valueList, nList);

  }
  return (c != '}' && f.good());
  }

void ShipClass::AssignDesign(short index) {
  design = game->shipDesign + index;
}

void ColonyClass::AssignDesign(short index) {
  design = game->colonyDesign + index;
}

void ColonyClass::AssignLocation(short index) {
  planetIndex = index;
  lockObj.planet = game->planet + index;
}

short ShipClass::GetSharedData(ifstream& f, ObjectType ot,
			       SFString* nameList, SFString* valueList,
			       short nList) {

  char c;
  SFString locationString;
  SFString* token = 0;
  char raceStr[max_name_char];
  char designStr[max_name_char];
  short nTok;
  short planIndex, designIndex;
  static CoordinateClass cOffset;

  f >> c;
  if (c != '}') {
    f.putback(c);
    f >> raceStr;

    raceID = game->IndexOfRace(raceStr);

    pop.get(f, nameList, valueList, nList);

    f >> designStr >> locationString;

    designIndex = game->IndexOfDesign(designStr, ot, raceID);
    AssignDesign(designIndex);

    nTok = locationString.TokenList(token," (),+");

    char *out;

    switch (nTok) {
    case 1:
      /**********************************************************************/
      /* one token                                                          */
      /*   example: Earth                                                   */
      /* assume a planet name was specified, match the object's coordinates */
      /* to the specified planet.                                           */
      /**********************************************************************/

      planIndex = 0;
      out = token[0].out();
      if ( out ) {
	planIndex = game->IndexOfPlanet(out);
	delete out;
	out = NULL;
      }

      if (planIndex == 0)
	logFile << "ERROR> planet index of zero detected during ship initialization: " <<
	  token[0] << endl;

      now = game->planet[planIndex].now;

      AssignLocation(planIndex);

      /* assign revertRaceIndex and revertColonyDesignIndex based on current values */
      if (ot == colony) {
	game->planet[planIndex].revertRaceIndex = short(raceID);
	game->planet[planIndex].revertColonyDesignIndex = designIndex;
      }

      break;
    case 2:
      /*********************************************************************/
      /* two tokens                                                        */
      /*   example: (1053,-5663)                                           */
      /* assume absolute coordinates are specified.                        */
      /*********************************************************************/
      now.initializeWith(token[0].Long()*sub_scale, token[1].Long()*sub_scale);
      break;
    case 3:
      /*********************************************************************/
      /* three tokens                                                      */
      /*   example: Earth+(7011,1885)                                      */
      /* assume a planet and relative coordinates are specified.           */
      /*********************************************************************/

      planIndex = 0;
      out = token[0].out();
      if ( out ) {
	planIndex = game->IndexOfPlanet(out);
	delete out;
	out = NULL;
      }

      if (planIndex == 0)
	logFile << "ERROR> planet index of zero detected during ship initialization: " <<
	  token[0] << endl;

      cOffset.initializeWith(token[1].Long()*sub_scale, token[2].Long()*sub_scale);

      now = game->planet[planIndex].now;

      now += cOffset;

      break;

    default:
      
      logFile << "ERROR> unknown location specifier: " << locationString;
      break;
    }

    gnow = now;

    activate();

    if (token)
      delete[] token;

  }
  return (c != '}' && f.good());
}

slong ColonyClass::PushTo(class GameStateBuffer& gb, GameStateClass* g) {

  WRITE_SF_LOG(logFile << "Pushing colony section of " << TypeName() << " " <<
	       g->TypeListIndex(this) << "..." << endl);

  slong subClassSize = ShipClass::PushTo(gb, g);

  char* bufferPos = gb.BufferPosition();

  CALL_PUSH_OBJ(gb,planetIndex);
  CALL_PUSH_OBJ(gb,kills);
  CALL_PUSH_OBJ(gb,lastDockObjIndex);
  CALL_PUSH_OBJ(gb,supply);
  CALL_PUSH_OBJ(gb,name);

  return subClassSize + (gb.BufferPosition() - bufferPos);

}

slong ColonyClass::PopFrom(class GameStateBuffer& gb, GameStateClass* g) {

  WRITE_SF_LOG(logFile << "Popping colony section of " << TypeName() << " " <<
	       g->TypeListIndex(this) << "..." << endl);
  
  slong subClassSize = ShipClass::PopFrom(gb,g);

  char* bufferPos = gb.BufferPosition();

  CALL_POP_OBJ(gb,planetIndex);
  CALL_POP_OBJ(gb,kills);
  CALL_POP_OBJ(gb,lastDockObjIndex);
  CALL_POP_OBJ(gb,supply);
  CALL_POP_OBJ(gb,name);

  return subClassSize + gb.BufferPosition() - bufferPos;
}

slong PopStruct::PushTo(class GameStateBuffer& gb) {
  char* bufferPos = gb.BufferPosition();

  CALL_PUSH_OBJ(gb,billions);
  CALL_PUSH_OBJ(gb,lBillions);

  return gb.BufferPosition() - bufferPos;
}

slong PopStruct::PopFrom(class GameStateBuffer& gb) {

  char* bufferPos = gb.BufferPosition();

  CALL_POP_OBJ(gb,billions);
  CALL_POP_OBJ(gb,lBillions);

  return gb.BufferPosition() - bufferPos;
}

slong GameStateBuffer::Push(GenericClass* o, GameStateClass* gm) {
  short objIndex;
  slong size;

  if (IsValidObjectPointer(o,__FILE__,__LINE__)) {
    objIndex = gm->ObjectListIndex(o);
    size = Push(objIndex);
    WRITE_SF_LOG(logFile << "pushing object " << objIndex << " (" << size << " bytes)" << endl);
  } else {
    objIndex = -1;
    size = Push(objIndex);
    WRITE_SF_LOG(logFile << "pushing null object (" << size << " bytes)" << endl);
  }

  return sizeof(short);
}
 
slong GameStateBuffer::Pop(GenericClass*& o, GameStateClass* gm) {
  short objIndex;
  slong size = Pop(objIndex);
  if (objIndex != -1) {
    WRITE_SF_LOG(logFile << "popping object " << objIndex << " (" << sizeof(short) << " bytes)" << endl);
    o = gm->objectList[objIndex];
    if ( !IsValidObjectPointer(o) )
      {
	o = 0L;
	WRITE_SF_LOG(logFile << "popping BAD object " << objIndex << " (" << sizeof(short) << " bytes)" << endl);
      }
  } else {
    WRITE_SF_LOG(logFile << "popping null object (" << sizeof(short) << " bytes)" << endl);
    o = 0L;
  }
  return size;
}


slong ListStruct::PushTo(class GameStateBuffer& gb) {
 
  char* bufferPos = gb.BufferPosition();

  CALL_PUSH_OBJ(gb,nTotal);
  CALL_PUSH_OBJ(gb,nSet);

  short i;
  for (i=0; i<nTotal; i++)
    CALL_PUSH_OBJ(gb,list[i]);

  return gb.BufferPosition() - bufferPos;

}

slong ListStruct::PopFrom(class GameStateBuffer& gb) {

  char* bufferPos = gb.BufferPosition();

  CALL_POP_OBJ(gb,nTotal);
  ReInitializeWith(nTotal);

  CALL_POP_OBJ(gb,nSet);

  short i;
  for (i=0; i<nTotal; i++)
    CALL_POP_OBJ(gb,list[i]);

  return gb.BufferPosition() - bufferPos;
}
