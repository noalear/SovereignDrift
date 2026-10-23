/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
#include <S_Lib.h>

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
#endif
#ifdef UNIX
#  include "../gui/sf.h"
#  include "../gui/SF_Player.h"
#  include "../gui/SF_Msg.h"
#  include "../gui/SF_MsgBuffer.h"
#  include "../gui/SF_VideoConsole.h"
#  include "../gui/SF_ChatConsole.h"
#  include "../gui/SF_Globals.h"
#  include"../gui/SF_Engine.h"
#  include "../gui/SF_ClientWindow.h"
#  include "../gui/SF_Client.h"
#  include "../gui/SF_DisplayEngine.h"
#  include "../gui/SF_Engine.h"
#  include "../gui/SF_Button.h"
#endif

#include "classes.h"
#include "inc_portcpp.h"
#include "enginepp.h"

char* Repeat(char* s, char* t, short n) {
  *t = '\0';
  
  short i;
  for (i=0; i<n; i++) {
    strcat(t, s);
  }

  return t;
}

char* ConcatToFixedLength(char* s, char* t, short n) {

  short ls = strlen(s);
  short lt = strlen(t);

  static char sp[80];

  if (ls + lt < n) {
    /* add space in middle */
    strcat(s, Repeat(" ", sp, n - (ls+lt)));
    strcat(s, t);
  } else
    strcat(s, t);

  return s;
}

int inServer = 0;
short subMode = 0;
short changeMode = 0;
short selPointer = 0;
short selIndex = 0;
slong  selID = 0;
short selType = 0;
short selOffset = 0;
short maxScroll = 0;
slong numSpawnMessagesSent = 0;
slong numSpawnMessagesReceived = 0;
slong numSpawnMessagesDiscarded = 0;

extern char frameRate;
extern slong frameNumber;
extern short localSlotNumber;
extern SF_VideoConsole *console;
extern slong raceColor[];
extern slong raceTextColor[];
extern slong raceBlipColor[];

int ConsoleLineClicked(int line) {

  switch ( game->ship[localSlotNumber].videoMode ) {
  case VIDEO_SELECT :
    selPointer = line+1;
    break;
  case VIDEO_STATUS :
    
    if ((short)line >= game->ship[localSlotNumber].bus.numMod)
      break;
    
    game->ship[localSlotNumber].bus.modPtr = (char)line;
    /* broadcast SETMODULEPOINTER */
    if (IsConnectedClient()) {
      
      game->ship[localSlotNumber].SendSetModulePointerMsg();
      
    }
    
    
    break;
  case VIDEO_RACELIST :
    if ( !subMode )
      selPointer = line+1;
    break;
  case VIDEO_RANKLIST :
    selPointer = line;
    break;
  case VIDEO_PLAYERLIST :
    selPointer = line;
    break;
  case VIDEO_PLANETLIST :
    selPointer = line+1;
    break;
  }
  return 0;
}


/* from construct.cpp */
extern GameStateClass* game;
extern short numPlanetRobotsSet;
extern short numPopRobotsSet;
extern char instanceName[];
extern short warningMessagePosted;
extern slong warningMessagePostedFrame;
extern short warningBottomMessagePosted;
extern slong warningBottomMessagePostedFrame;

// Standard Bitmaps as of 12/10/95
#define MAX_BITMAPS 1024

inline int StatusColor(slong s) {
  if (s <= module_break_limit)
    return RGB_DARKRED;
  else if (s <= module_warning_limit)
    return RGB_BROWN;
  else
    return RGB_DARKGREEN;
}

/********************************/

int SetupConsole();
void SetScreenBottomMessage( char *, int );
void SetScreenMessage( char *, int );

void EngineResetObjectList( char *objectBuffer ) {
#ifdef UNIX
  if (game)
    delete game;

  game = (GameStateClass*) objectBuffer;
#endif
}

slong EngineGetGameState(char* ShmPointer) {
  inServer = 1;
  strcpy(instanceName, "server");
  GameStateBuffer gb = GameStateBuffer(ShmPointer);
  logFile << "note> game state requested on frame " << game->frame << endl;
  return gb.Push(*game);
}


SF_MsgBuffer retMsgBuffer;



int
SendObjectTrackMsg(int i) {
  ObjectUnion g;
  
  if ( !IsServer() )
    return 0;
  
  g = game->ObjectListObj(i);
  
  if ( !g.generic->IsActive() || g.generic->IsExploding() )
    return 0;
  
  int ti;
  
  switch (g.generic->Type()) {
    
  case colony:
    /* ***************************************************************** */
    /* *** Don't send msgs for colonies on planets or colonies being *** */
    /* *** carried by ships.                                         *** */
    /* ***************************************************************** */
    if ( g.colony->dockObj || g.colony->lockObj.generic )
       break;

    /* ************************************************************* */
    /* *** Send msgs for one colony each second.  Each colony is *** */
    /* *** updated once every max_colonies seconds.              *** */
    /* ************************************************************* */
    ti = game->TypeListIndex(g.colony);
    if ( frameNumber % frameRate == 0 && 
	(frameNumber/frameRate) % game->maxColonies == ti)
      g.colony->SendObjectTrackingMsg();
    break;

  case ship:
    /* *************************************************************** */
    /* *** Don't send msgs for Warping ships or human player ships *** */
    /* *************************************************************** */
    if ( g.ship->IsWarping() || !game->IsRobotPlayer(g.ship) )
       break;

    /* ************************************************************* */
    /* *** Send msg for each ship once per second.  Each ship is *** */
    /* *** updated once per second.                              *** */
    /* ************************************************************* */
    ti = game->TypeListIndex(g.ship);
    if ( (frameNumber + ti) % frameRate == 0)
      g.ship->SendObjectTrackingMsg();
    break;

  case planet:
    break;

  default :
    break;

  }
  
  return 0;
}


void CorrectShipPosition( int i,int snap ){

  static CoordinateClass r2;
  static CoordinateClass v;
  static CoordinateClass dv;

  /* ***************************************** */
  /* *** Don't correct local client player *** */
  /* ***************************************** */
  if ( !IsServer() && i == localSlotNumber )
    return;

  GenericClass *g = (GenericClass *)game->objectList[i];

  if ( !IsValidObjectPointer(g,__FILE__,__LINE__) )
  {
	  return;
  }

  ShipClass *s = NULL;
  if ( g->Type() >= designed )
    s = (ShipClass *)g;

  /* ***************************************************************** */
  /* *** Don't correct inactive objects or objects that have never *** */
  /* *** rcvd an AIOBJECTTRACK msg (gframe<0).                     *** */
  /* ***************************************************************** */
  if ( !g->IsActive() || g->gframe < 0 )
    return;

  /* ************************************************ */
  /* *** Don't correct position for warping ships *** */
  /* ************************************************ */
  if ( s && s->IsWarping() )
    return;

  //long amax = 10*game->maxAcc*sub_scale/frameRate/frameRate;
  slong vmax = game->lightSpeed/7;
  ulong cap = 100 * (ulong) game->lightSpeed;

  /* ************************************************************* */
  /* *** numFrames is how many frames we are ahead of the data *** */
  /* ************************************************************* */
  int numFrames = (uchar) ((frameNumber % 256) - g->gframe);
  if ( numFrames > 127 )
    numFrames -= 256;

  //if (g->gfresh)
  //  SetShipLag(i, numFrames);

  /* ****************************************************** */
  /* *** Little patch to prevent severe lag corrections *** */
  /* ****************************************************** */
  if ( numFrames > 2*frameRate ) numFrames = 2*frameRate;
  if ( numFrames < 0 ) numFrames = 0;
  /* ****************************************************** */

  //
  // If a ship has a nonzero lag and it is less than numFrames, the  use it
  //
  int lag = GetShipLag(i);
  if ( lag > 0 && lag < numFrames )
    {
      numFrames = lag;
    }

  /* ***************************************************************** */
  /* *** Predicted position of the object based on last fresh data *** */
  /* ***************************************************************** */
  r2 = g->gnow + g->gvel * numFrames;

  /* ******************************************************** */
  /* *** Correct everything with no smoothness and return *** */
  /* ******************************************************** */
  if ( snap  ) {
    if ( g->gfresh ) {
      g->now = r2;
      g->vel = g->gvel;
      g->tvel = g->gvel;
      g->gfresh = FALSE;
    }
    return;
  }
  /* *********************************************************** */

  /* *********************************************************** */
  /* *** Calculate a new position changes smoothly by        *** */
  /* *** capping the apparent acceleration that results from *** */
  /* *** moving to the new spot.                             *** */
  /* *********************************************************** */
  v = r2 - g->now;

  /*vmax *= (1 + 2*g->gvel->Mag()/game->lightSpeed);*/

  if (v.Mag(pc_ulong_max) < cap)
    v.AddSubjectToLimit(nullVector, vmax);

  g->now += v;
  /* *********************************************************** */

  /********************************************************************* */
  /* *** If the network data is fresh, then just set the velocity    *** */
  /* *** to what comes over the network.  Limits on the acceleration *** */
  /* *** will keep it from looking bad.                              *** */
  /* ******************************************************************* */
  if ( g->gfresh ) {

    if ( s && s->IsWarping()) {
      s->vel = nullVector;
      s->tvel = nullVector;
    } else {

      dv = g->gvel - g->vel;
      /*
	dv.AddSubjectToLimit(nullVector, amax);
      */
      g->vel += dv;

      if (g->parent.generic) {
	if (g->parent.generic->parent.generic)
	  g->tvel = g->vel + g->parent.generic->vel +
	    g->parent.generic->parent.generic->vel;
	else g->tvel = g->vel + g->parent.generic->vel;
      } else
	g->tvel = g->vel;

    }

    /* *** Set the network data to unfresh *** */
    g->gfresh = FALSE;
  }
  /* *********************************************************** */

}

void CorrectShipPositions() {

  /* ***************************************************************** */
  /* *** Only server and network clients need to correct positions *** */
  /* ***************************************************************** */
  if ( IsDisconnectedClient() )
      return;

  /* ************************* */
  /* *** Correct all ships *** */
  /* ************************* */
  int i;
  for ( i=0; i<game->numShips; i++ )
     CorrectShipPosition(i,FALSE);

  /* ************************************** */
  /* *** Correct free floating colonies *** */
  /* ************************************** */
  for ( i=game->colonyObjectIndexStart; i<game->colonyObjectIndexEnd; i++ ) {
     ColonyClass *c = &game->colony[i-game->colonyObjectIndexStart];
     if ( !c->lockObj.generic && !c->dockObj )
        CorrectShipPosition(i,FALSE);
     }
  }


void  EngineStartFrame() {
  retMsgBuffer.Flush();
  game->ClearMode(GameStateClass::betweenFrames);
  game->Update();
}

SF_MsgBuffer *EngineCloseFrame() {
  game->Recycle();
  game->UpdateDockedObjects();
  SetupConsole();
  CorrectShipPositions();
  game->SetMode(GameStateClass::betweenFrames);
  return &retMsgBuffer;
}


void ScreenKeyStrokesForMessage( slong,uchar,uchar,
                                 unsigned short s, ulong l);

void EnginePlusPlus( SF_Msg *gmsg ) {

  short i = (short)gmsg->slot,j,k;

  if ( i < 0 || i >= game->NumObjects() )
    {
      logFile << "ERROR> EnginePlusPlus received invalid slot number " <<
	i << " on frame " << game->frame << endl;
      return;
    }

  ObjectUnion tShip;
  tShip.generic = game->objectList[i];
  if ( !IsValidObjectPointer(tShip.generic,__FILE__,__LINE__) )
    {
      return;
    }

  if (gmsg->msg == SF_MSG_SPAWNCOLONY)
    logFile << "received> (preprocess) spawn_colony msg (" << 
      numSpawnMessagesReceived+1 << " received) on frame " <<
      game->frame << ": " <<
      gmsg->slot << " " <<
      gmsg->dslot << " " <<
      gmsg->bslot << " " <<
      gmsg->race << " " <<
      gmsg->pop1 << " " <<
      gmsg->pop2 <<  " " << endl;
  
  short currentPlayer = 0;
  static char temp[80], msg[80], temp2[80], str[16];
  GenericClass* viewObj;
  ObjectUnion obj, tobj;
  
  obj.generic = game->objectList[short(gmsg->slot)];
  if ( !IsValidObjectPointer(obj.generic) )
    {
      return;
    }

  if ( IsValidObjectPointer(game->objectList[i]) &&
       game->objectList[i]->active == 0 && 
       gmsg->msg != SF_MSG_FILLSLOT )
    {
      return;
    }
  
  
  if ( i <= game->FirstMissileObjectIndex() )
    {
      currentPlayer = i - game->shipObjectIndexStart == localSlotNumber;
      switch (gmsg->msg) {
	
      case SF_MSG_KEYPRESSED_4:
	
	if ( !IsValidObjectPointer(tShip.ship) )
	  {
	    return;
	  }
	
	switch(gmsg->scancode) {
	case left_arrow_down:
	case alt_left_arrow_down:
	  if (!tShip.ship->HasShift()) {
	    tShip.ship->BreakOrbitalLock();
	    tShip.ship->TurnLeft();
	    tShip.ship->SetMode(DesignedClass::mode_left, op_on);
	  } else {
	    if (currentPlayer && tShip.ship->bus.impulseMod &&
		tShip.ship->bus.impulseMod->check == 0xAAAAAAAA &&
		tShip.ship->bus.impulseMod->func < module_break_limit) {
	      
	      strcpy(temp, "1st Officer> the engines are offline!");
	      game->PostGameMessage(temp,i,(short)tShip.ship->raceID,personal);
	      
	    }
	    tShip.ship->ClearAIParams();
	    tShip.ship->Command(BusStruct::fire_thrust_left);
	  }
	  break;
	case right_arrow_down:
	case alt_right_arrow_down:
	  if (!tShip.ship->HasShift()) {
	    tShip.ship->BreakOrbitalLock();
	    tShip.ship->TurnRight();
	    tShip.ship->SetMode(DesignedClass::mode_right, op_on);
	  } else {
	    if (currentPlayer && tShip.ship->bus.impulseMod &&
		tShip.ship->bus.impulseMod->check == 0xAAAAAAAA &&
		tShip.ship->bus.impulseMod->func < module_break_limit) {
	      
	      strcpy(temp, "1st Officer> the engines are offline!");
	      game->PostGameMessage(temp,i,(short)tShip.ship->raceID,personal);
	      
	    }
	    tShip.ship->ClearAIParams();
	    tShip.ship->Command(BusStruct::fire_thrust_right);
	  }
	  break;	
	case all_stop_button_down:
	  if (currentPlayer && tShip.ship->bus.brakeMod &&
	      tShip.ship->bus.brakeMod->check == 0xAAAAAAAA &&
	      tShip.ship->bus.brakeMod->func < module_break_limit) {
	    
	    strcpy(temp, "1st Officer> the brakes are offline");
	    game->PostGameMessage(temp,i,(short)tShip.ship->raceID,personal);
	    
	  }
	  tShip.ship->Command(BusStruct::fire_brake);
	  break;
	case thrust_button:
	  if (currentPlayer && tShip.ship->bus.impulseMod &&
	      tShip.ship->bus.impulseMod->check == 0xAAAAAAAA &&
	      tShip.ship->bus.impulseMod->func < module_break_limit) {
	    
	    strcpy(temp, "1st Officer> the engines are offline!");
	    game->PostGameMessage(temp,i,(short)tShip.ship->raceID,personal);
	    
	  }
	  tShip.ship->ClearAIParams();
	  tShip.ship->Command(BusStruct::fire_thrust_forward);
	  
	  if ( tShip.ship->dockObj )
	    /* if ship thinks it is docked then request freedom from */
	    /* the ship that owns the port                           */
	    tShip.ship->FreeFromDock();
	  
	  break;
	case alt_thrust_reverse_button:
	case thrust_reverse_button:
	  if (currentPlayer && tShip.ship->bus.impulseMod &&
	      tShip.ship->bus.impulseMod->check == 0xAAAAAAAA &&
	      tShip.ship->bus.impulseMod->func < module_break_limit) {
	    
	    strcpy(temp, "1st Officer> the engines are offline!");
	    game->PostGameMessage(temp, i,(short)tShip.ship->raceID, personal);
	    
	  }
	  tShip.ship->ClearAIParams();
	  tShip.ship->Command(BusStruct::fire_thrust_reverse);
	  
	  if (tShip.ship->dockObj)
	    /* if ship thinks it is docked then request freedom from */
	    /* the ship that owns the port                           */
	    tShip.ship->FreeFromDock();
	  
	  break;
	case thrust_left_button:
	  if (currentPlayer && tShip.ship->bus.impulseMod &&
	      tShip.ship->bus.impulseMod->check == 0xAAAAAAAA &&
	      tShip.ship->bus.impulseMod->func < module_break_limit) {
	    
	    strcpy(temp, "1st Officer> the engines offline!");
	    game->PostGameMessage(temp, i,(short)tShip.ship->raceID, personal);
	    
	  }
	  tShip.ship->ClearAIParams();
	  tShip.ship->Command(BusStruct::fire_thrust_left);
	  break;
	case thrust_right_button:
	  if (currentPlayer && tShip.ship->bus.impulseMod &&
	      tShip.ship->bus.impulseMod->check == 0xAAAAAAAA &&
	      tShip.ship->bus.impulseMod->func < module_break_limit) {
	    
	    strcpy(temp, "1st Officer> the engines offline!");
	    game->PostGameMessage(temp, i,(short)tShip.ship->raceID, personal);
	    
	  }
	  tShip.ship->ClearAIParams();
	  tShip.ship->Command(BusStruct::fire_thrust_right);
	  break;
	case scram_button_down:
	  if (tShip.ship->HasControl())
	    game->SendCommandToFleet(game->ship+i, BusStruct::scramble);
	  else {
	    if (currentPlayer && tShip.ship->bus.scrambleMod &&
		tShip.ship->bus.scrambleMod->check == 0xAAAAAAAA &&
		tShip.ship->bus.scrambleMod->func < module_break_limit) {
	      
	      strcpy(temp, "1st Officer> counter measures are offline!");
	      game->PostGameMessage(temp,i,(short)tShip.ship->raceID,personal);
	      
	    }
	    tShip.ship->Command(BusStruct::scramble);
	  }
	  break;
	case stealth_button_down:
	  if (tShip.ship->HasControl())
	    game->SendStealthCommandToFleet(game->ship+i);
	  else {
	    if (tShip.ship->stealth == 1) {
	      if (currentPlayer && tShip.ship->bus.stealthMod &&
		  tShip.ship->bus.stealthMod->check == 0xAAAAAAAA &&
		  tShip.ship->bus.stealthMod->func < module_break_limit) {
		
		strcpy(temp, "1st Officer> the stealth system is offline!");
		game->PostGameMessage(temp,i,(short)tShip.ship->raceID,personal);
		
	      }
	      tShip.ship->Command(BusStruct::fire_cloak);
	    } else {
	      tShip.ship->Command(BusStruct::release_cloak);
	    }
	    
	  }
	  break;
	case alt_thrust_button:
	case boost_button_down:
	  if (currentPlayer && tShip.ship->bus.boostMod &&
	      tShip.ship->bus.boostMod->check == 0xAAAAAAAA &&
	      tShip.ship->bus.boostMod->func < module_break_limit) {
	    
	    if (currentPlayer && tShip.ship->bus.impulseMod &&
		tShip.ship->bus.impulseMod->check == 0xAAAAAAAA &&
		tShip.ship->bus.impulseMod->func < module_break_limit)
	      strcpy(temp, "engine and booster offline");
	    else
	      strcpy(temp, "1st Officer> the boosters are offline!");
	    game->PostGameMessage(temp, i,(short)tShip.ship->raceID, personal);
	    
	  }
	  tShip.ship->ClearAIParams();
	  tShip.ship->Command(BusStruct::fire_boost);
	  
	  if (tShip.ship->dockObj)
	    /* if ship thinks it is docked then request freedom from */
	    /* the ship that owns the port                           */
	    tShip.ship->FreeFromDock();
	  
	  break;
	case bind_button:
	  break;
	  if (tShip.ship->videoMode == VIDEO_NAVIGATE) {
	    viewObj = game->planet + short(tShip.ship->assignPar2);
	    strcpy(str, game->planet[short(tShip.ship->assignPar2)].name);
	  } else if (tShip.ship->videoMode == VIDEO_TARGET &&
		     tShip.ship->targetPP.obj.generic) {
	    viewObj = tShip.ship->targetPP.obj.generic;
	    strcpy(str, tShip.ship->targetPP.obj.generic->TypeName());
	  } else
	    viewObj = 0;
	  if ( IsValidObjectPointer(viewObj,__FILE__,__LINE__) ) {
	    
	    /* if (tShip.ship->HasControl()) { */
	    game->BindFleetTo(game->ship+i, viewObj);
	    sprintf(temp, "1st Officer> the fleet is bound to %s.", str);
	    /*} else {
	      sprintf(temp, "1st Officer> we are bound to %s.", str);
	      game->BindShipTo(game->ship+i, viewObj); 
	      } */
	    game->PostGameMessage(temp, i,(short)tShip.ship->raceID, personal);
	  }
	  break;
	case respond_button_down:
	  game->RespondToLastDistress(game->ship+i);
	  break;
	case join_button_down:
	  
	  logFile << "JOIN: Player " << i << " pressed j" << endl;
	  logFile << "HasControl = " << tShip.ship->HasControl() << endl;
	  logFile << "tShip.ship->targetPP.obj.generic = " << tShip.ship->targetPP.obj.generic << endl;
	  if ( IsValidObjectPointer(tShip.ship->targetPP.obj.generic) )
	    logFile << "tShip.ship->targetPP.obj.generic->Type() = " << 
	      tShip.ship->targetPP.obj.generic->Type() << endl;
	  
	  if (tShip.ship->HasControl()) {
	    j = game->SummonFleetTo(game->ship+i, tShip.ship->HasShift());
	    if (currentPlayer && j) {
	      sprintf(temp, "1st Officer> %d friendly ships are responding", j);
	      game->PostGameMessage(temp,i,(short)tShip.ship->raceID,personal);
	    }
	  } else if ( IsValidObjectPointer(tShip.ship->targetPP.obj.generic) &&
		      tShip.ship->targetPP.obj.generic->Type() == ship)
	    game->SummonShipTo(game->ship+i, tShip.ship->HasShift());
	  else if ( IsValidObjectPointer(tShip.ship->targetPP.obj.generic) &&
		    tShip.ship->targetPP.obj.generic->Type() == colony) {
	    logFile << "JOIN: Target is a colony for player " << i << endl;
	    
	    if ( tShip.ship->bus.numPorts < 1 ) {
	      strcpy(temp,"1st Officer> we need a tractor to pick up colonists.");
	      game->PostGameMessage(temp,i,(short)tShip.ship->raceID,personal);
	      break;
	    } else if (!tShip.ship->bus.IsDockFree(0) && 
		       tShip.ship->bus.dockMod[0] &&
		       tShip.ship->bus.numPorts > 0 && 
		       IsValidObjectPointer((ShipClass *)tShip.ship->bus.dockMod[0]->workPar[0],__FILE__,__LINE__) &&
		       ((ShipClass*) tShip.ship->bus.dockMod[0]->workPar[0])->Type() != colony) {
	      strcpy(temp,"1st Officer> we don't have an available port.");
	      game->PostGameMessage(temp,i,(short)tShip.ship->raceID,personal);
	      break;
	    }
	    
	    if ( IsValidObjectPointer(tShip.ship->targetPP.obj.colony) &&
		 IsValidObjectPointer(tShip.ship->targetPP.obj.colony->lockObj.generic) &&
		 tShip.ship->parent.objLong ==
		 tShip.ship->targetPP.obj.colony->lockObj.objLong) {
	      logFile << "JOIN: Calling SpawnToDock for player " << i << endl;
	      tShip.ship->targetPP.obj.colony->SpawnToDock(game->ship+i);
	    } else { /* tell the player to orbit first */
	      /*
		if ( !tShip.ship->targetPP.obj.colony && 
		tShip.ship->targetPP.obj.colony->lockObj.planet)
		sprintf(temp,"1st Officer> There are no colonists on %s.",
		tShip.ship->targetPP.obj.colony->lockObj.planet->name);
		else
	      */
	      strcpy(temp,"1st Officer> you must orbit the planet before "
		     "you can pick up a new colony.");
	      game->PostGameMessage(temp,i,(short)tShip.ship->raceID,personal);
	      
	    }
	  }
	  break;
	case release_button_down:
	  if (tShip.ship->HasControl()) {
	    game->ReleaseFleetFrom(game->ship+i);
	  } else {
	    game->GenericRelease(game->ship+i);
	  }
	  break;
	case page_up_button:
	  if (tShip.ship->videoMode == VIDEO_VIEW)
	    tShip.ship->viewObj.generic =
	      game->GetNextViewObj(tShip.ship, 1);
	  else if (tShip.ship->videoMode == VIDEO_NAVIGATE) {
	    if (tShip.ship->HasShift())
	      tShip.ship->AssignNextPlanet(1, sequential_friendly);
	    else
	      tShip.ship->AssignNextPlanet(1, sequential,
					   tShip.ship->HasControl());
	  } else if (tShip.ship->videoMode == VIDEO_TARGET) {
	    
	    if (tShip.ship->HasShift())
	      tShip.ship->AcquireNext(sequential_onscreen,1);
	    else if (tShip.ship->HasControl())
	      tShip.ship->AcquireNext(sequential_enemy,1);
	    else
	      tShip.ship->AcquireNext(sequential,1);
	    
	    /* broadcast target assignment and request an update from the server */
	    if (IsConnectedClient() && IsLocalPlayer(tShip.ship))
	      tShip.ship->SendSetTargetMsg(tShip.ship->targetPP.obj);
	    
	  } else if (tShip.ship->videoMode == VIDEO_RACELIST) {
	    if ( console && i == localSlotNumber ) {
	      switch (subMode) {
	      case 0:
		
		k = game->numRaces;
		
		selPointer++;
		
		if (selPointer >= min(console->chy - 1, k + console->numHeaderLines)) {
		  
		  maxScroll = k - min(console->chy - 1 - console->numHeaderLines, k) + console->numHeaderLines;
		  
		  if (console->scroll < maxScroll)
		    console->ScrollUp();
		  else
		    console->scroll = maxScroll;
		  
		  selPointer = min(console->chy - 1, k + console->numHeaderLines) - 1;
		  
		}
		
		break;
	      case 1:
		
		k = game->race[(selPointer-1)%game->numRaces].NumPlanetsInVictoryList();
		
		maxScroll = k - min(console->chy - console->numHeaderLines-1, k) +
		  console->numHeaderLines;
		
		if (console->scroll < maxScroll)
		  console->ScrollUp();
		else
		  console->scroll = maxScroll;
		
		break;
	      }
	    }
	    
	  } else if (tShip.ship->videoMode == VIDEO_RANKLIST) {
	    if ( console && i == localSlotNumber ) {
	      
	      k = game->numRanks;
	      
	      maxScroll = k - min(console->chy - console->numHeaderLines, k) +
		console->numHeaderLines;
	      
	      if (console->scroll < maxScroll)
		console->ScrollUp();
	      else
		console->scroll = maxScroll;
	      
	      break;
	      
	    }
	    
	  } else if (tShip.ship->videoMode == VIDEO_SELECT) {
	    if ( console && i == localSlotNumber ) {
	      
	      k = game->NumModulesByType((short)tShip.ship->raceID, selType);
	      
	      if (selType != -1)
		selOffset = 6;
	      else
		selOffset = 3;
	      
	      selPointer++;
	      
	      if (selPointer >= min(console->chy - 1, k + selOffset)) {
		
		maxScroll = k - min(console->chy - 1 - console->numHeaderLines, k) + console->numHeaderLines;
		
		if (console->scroll < maxScroll)
		  console->ScrollUp();
		else
		  console->scroll = maxScroll;
		
		selPointer = min(console->chy - 1, k + console->numHeaderLines) - 1;
		
	      } 
	    }
	  } else if (tShip.ship->videoMode == VIDEO_PLAYERLIST) {
	    if ( console && i == localSlotNumber) {
	      
	      k = game->numActiveShips;
	      
	      maxScroll = k - min(console->chy - console->numHeaderLines, k) +
		console->numHeaderLines;
	      
	      if (console->scroll < maxScroll)
		console->ScrollUp();
	      else
		console->scroll = maxScroll;
	      
	    }
	  } else if (tShip.ship->videoMode == VIDEO_PLANETLIST) {
	    
	    if ( console && i == localSlotNumber ) {
	      
	      changeMode = 1;
	      
	      k = game->numPlanets;
	      
	      selPointer++;
	      
	      if (selPointer > min(console->chy - 1, k + console->numHeaderLines)) {
		
		maxScroll = k - min(console->chy - console->numHeaderLines, k) + console->numHeaderLines;
		
		if (console->scroll < maxScroll)
		  console->ScrollUp();
		else
		  console->scroll = maxScroll;
		
		selPointer = min(console->chy - 1, k + console->numHeaderLines);
		
	      }
	      
	    }
	    
	  } else if (tShip.ship->videoMode == VIDEO_STATUS) {
	    if (tShip.ship->HasShift()) {
	      
	      if (IsDisconnectedClient() ||
		  IsConnectedClient() && IsLocalPlayer(tShip.ship)) {
		
		tShip.ship->SwapModuleByType(1);
		
	      }
	      
	    } else {
	      
	      /* broadcast SETMODULEPOINTER */
	      if (IsDisconnectedClient() ||
		  IsConnectedClient() && IsLocalPlayer(tShip.ship)) {
		
		if (tShip.ship->bus.numMod)
		  tShip.ship->bus.modPtr = (tShip.ship->bus.modPtr + 1) %
		    tShip.ship->bus.numMod;
		
		tShip.ship->SendSetModulePointerMsg();
		
	      }
	      
	    }
	  }
	  break;
	case page_down_button:
	  if (tShip.ship->videoMode == VIDEO_VIEW) 
	    tShip.ship->viewObj.generic =
	      game->GetNextViewObj(tShip.ship, -1);
	  else if (tShip.ship->videoMode == VIDEO_NAVIGATE) {
	    if (tShip.ship->HasShift())
	      tShip.ship->AssignNextPlanet(-1, sequential_friendly);
	    else
	      tShip.ship->AssignNextPlanet(-1, sequential,
					   tShip.ship->HasControl());
	  } else if (tShip.ship->videoMode == VIDEO_TARGET) {
	    
	    if (tShip.ship->HasShift())
	      tShip.ship->AcquireNext(sequential_onscreen,-1);
	    else if (tShip.ship->HasControl())
	      tShip.ship->AcquireNext(sequential_enemy,-1);
	    else
	      tShip.ship->AcquireNext(sequential,-1);
	    
	    /* request an update from the server */
	    if (IsConnectedClient() && IsLocalPlayer(tShip.ship))
	      tShip.ship->SendSetTargetMsg(tShip.ship->targetPP.obj);
	    
	  } else if (tShip.ship->videoMode == VIDEO_PLAYERLIST) {
	    if (IsLocalPlayer(tShip.ship) && console)
	      console->ScrollDown();
	    changeMode = 1;
	  } else if (tShip.ship->videoMode == VIDEO_PLANETLIST) {
	    
	    if (IsLocalPlayer(tShip.ship) && console) {
	      
	      selPointer--;
	      
	      if (selPointer < console->numHeaderLines) {
		console->ScrollDown();
		selPointer = console->numHeaderLines;
	      }
	      
	    }
	    
	    changeMode = 1;
	    
	  } else if (tShip.ship->videoMode == VIDEO_SELECT) {
	    if (IsLocalPlayer(tShip.ship) && console) {
	      
	      if (selType != -1)
		selOffset = 6;
	      else
		selOffset = 3;
	      
	      selPointer--;
	      
	      if (selPointer < selOffset) {
		console->ScrollDown();
		selPointer = selOffset;
	      }
	    }
	  } else if (tShip.ship->videoMode == VIDEO_RACELIST) {
	    if (IsLocalPlayer(tShip.ship) && console) {
	      switch (subMode) {
	      case 0:
		selPointer--;
		if ( selPointer < console->numHeaderLines) {
		  console->ScrollDown();
		  selPointer = console->numHeaderLines;
		}
		break;
	      case 1:
		console->ScrollDown();
		break;
	      }	  
	    }
	  } else if (tShip.ship->videoMode == VIDEO_RANKLIST) {
	    if (IsLocalPlayer(tShip.ship) && console) {
	      selPointer--;
              if ( selPointer < console->numHeaderLines) {
		console->ScrollDown();
		selPointer = console->numHeaderLines;
	      }
	    }
	  } else if (tShip.ship->videoMode == VIDEO_STATUS) {
	    if (tShip.ship->HasShift()) {
	      
	      
	      if (IsDisconnectedClient() ||
		  IsConnectedClient() && IsLocalPlayer(tShip.ship)) {
		
		tShip.ship->SwapModuleByType(-1);
		
	      }
	      
	    } else {
	      
	      /* broadcast SETMODULEPOINTER */
	      if (IsDisconnectedClient() ||
		  IsConnectedClient() && IsLocalPlayer(tShip.ship)) {
		
		tShip.ship->bus.modPtr =
		  (tShip.ship->bus.modPtr - 1 + tShip.ship->bus.numMod) %
		  tShip.ship->bus.numMod;
		
		tShip.ship->SendSetModulePointerMsg();
		
	      }
	    }
	  }
	  break;
	case acquire_button:
	  if (!tShip.ship->HasShift()) {
	    
	    tShip.ship->AcquireNext(nearest_enemy);
	    
	  } else {
	    
	    if ( IsValidObjectPointer(tShip.ship) )
	      {
		tShip.ship->targetPP = 
		  tShip.ship->Ping(TargetType(ship_target + 
					      colony_target + 
					      missile_target), 
				   nearest_enemy);
		if ( !IsValidObjectPointer(tShip.ship->targetPP.obj.generic) )
		  {
		    tShip.ship->targetPP.obj.generic = 0;
		  }
	      }
	  }
	  
	  /* request an update from the server */
	  if (IsConnectedClient() && IsLocalPlayer(tShip.ship)) {
	    tShip.ship->SendSetTargetMsg(tShip.ship->targetPP.obj);
	  }
	  
	  tShip.ship->videoMode = VIDEO_TARGET;
	  
	  if (console && IsLocalPlayer(tShip.ship)) {
	    console->SetMode(VIDEO_TARGET);
	    console->SetNumHeaderLines(0);
	  }
	  
	  break;
	  
	case z_button_down:
	  if ( IsValidObjectPointer(tShip.ship) && !tShip.ship->HasShift() ) {
	    tShip.ship->targetPP = 
	      tShip.ship->Ping(TargetType(ship_target + colony_target), nearest);
	    
	    if ( !IsValidObjectPointer(tShip.ship->targetPP.obj.generic) )
	      {
		tShip.ship->targetPP.obj.generic = 0;
	      }
	    
	    /* request an update from the server */
	    if (IsConnectedClient() && IsLocalPlayer(tShip.ship))
	      tShip.ship->SendSetTargetMsg(tShip.ship->targetPP.obj);
	    
	  } else
	    tShip.ship->AcquireNext(self);
	  
	  tShip.ship->videoMode = VIDEO_TARGET;
	  
	  if (console && IsLocalPlayer(tShip.ship)) {
	    console->SetMode(VIDEO_TARGET);
	    console->SetNumHeaderLines(0);
	  }
	  
	  break;
	  
	case target_button:
	  
	  tShip.ship->videoMode = VIDEO_TARGET;
	  
	  if (console && IsLocalPlayer(tShip.ship)) {
	    console->SetMode(VIDEO_TARGET);
	    console->SetNumHeaderLines(0);
	  }
	  
	  if (tShip.ship->HasShift())
	    if (IsValidObjectPointer(tShip.ship->targetPP.obj.generic) &&
		IsValidObjectPointer(tShip.ship->targetPP.obj.colony->parent.generic) )
	      tShip.ship->targetPP.obj =
		tShip.ship->targetPP.obj.colony->parent;
	  
	  /* request an update from the server */
	  if (IsConnectedClient() && IsLocalPlayer(tShip.ship))
	    tShip.ship->SendSetTargetMsg(tShip.ship->targetPP.obj);
	  
	  break;
	case query_button:
	  if ( IsValidObjectPointer(tShip.ship->targetPP.obj.generic) ) {
	    if (tShip.ship->HasShift() &&
		game->player[tShip.ship->playerIndex].IsDeveloper()) {
	      logFile << "*** user-requested self-object profile..." << endl;
	      tShip.ship->profile(game);
	      tShip.ship->ProfileCheckSum(game);
	      logFile << "*** user-requested target-object profile..." << endl;
	      tShip.ship->targetPP.obj.generic->profile(game);
	      tShip.ship->targetPP.obj.generic->ProfileCheckSum(game);
	      
	    } else
	      tShip.ship->targetPP.obj.generic->Query(game->ship+i);
	  }
	  break;
	case attack_button_down:
	  if ( !IsValidObjectPointer(tShip.ship) )
	    {
	      break;
	    }
	  if (!tShip.ship->dockObj) {
	    if (!tShip.ship->HasShift())
	      tShip.ship->AttackTarget();
	    else
	      tShip.ship->FollowTarget();
	    
	    if ((IsDisconnectedClient() ||
		 IsConnectedClient() && IsLocalPlayer(tShip.ship)) &&
		IsValidObjectPointer(tShip.ship->targetPP.obj.generic)) {
	      PingPacket pp = tShip.ship->Ping(target_target);
	      if (pp.dist > (slong)min_warp_dist &&
		  tShip.ship->bus.warpMod &&
		  tShip.ship->bus.warpMod->check == 0xAAAAAAAA &&
		  tShip.ship->bus.warpMod->par[1] < tShip.ship->mass) {
		
		/* the ship will use q drive to go to target */
		strcpy(temp, "1st Officer> Our ship is too massive to engage q drive.");
		game->PostGameMessage(temp,i,(short)tShip.ship->raceID,personal);
	      }
	    }
	    
	  } else {
	    
	    strcpy(temp2, 
		   "1st Officer> We must undock before we can use the Q Drive!");
	    game->PostGameMessage(temp2,i,(short)tShip.ship->raceID,personal);
	    
	  }
	  
	  break;
	  
	case fire_tube_a:
	  if (tShip.ship->HasControl())
	    game->SendCommandToFleet(game->ship+i, BusStruct::fire_tubes, 
				     nullVector);
	  else
	    if (currentPlayer && tShip.ship->bus.tubeMod) {
	      int n1 = 0;
	      int n2 = 0;
	      for ( j=0; j<tShip.ship->bus.numTubes; j++) {
		if (tShip.ship->bus.tubeMod[j]->func < module_break_limit)
		  n1++;
		else if (tShip.ship->bus.tubeMod[j]->moduleFunc != ModuleClass::beam &&
			 tShip.ship->bus.tubeMod[j]->workPar[0] == 0)
		  n2++;
	      }
	      
	      *temp = '\0';
	      *msg = '\0';
	      
	      if (n1) {
		if (n1 == 1)
		  sprintf(temp, "1 tube offline!");
		else if (n1 < tShip.ship->bus.numTubes)
		  sprintf(temp, "%d tubes offline!", n1);
		else
		  strcpy(temp, "all tubes offline!");
	      }
	      if (n2) {
		if (n2 == 1)
		  sprintf(msg, "1 tube out of missiles!");
		else if (n2 < tShip.ship->bus.numTubes)
		  sprintf(msg, "%d tubes out of missiles!", n2);
		else
		  strcpy(msg, "all tubes out of missiles!");
	      }
	      
	      if (n1 || n2) {
		sprintf(temp2, "1st Officer> %s %s", temp, msg);
		game->PostGameMessage(temp2,i,(short)tShip.ship->raceID,personal);
	      }
	      
	    }
	  tShip.ship->Command(BusStruct::fire_tubes, nullVector, nullVector);
	  break;
	case one_button:
	case two_button:
	case three_button:
	case four_button:
	case five_button:
	case six_button:
	case seven_button:
	case eight_button:
	case nine_button:
	  {
	    int tubeNum = gmsg->scancode - one_button;
	    if ( tubeNum < 0 || tubeNum >= tShip.ship->bus.numTubes )
	      {
		return;
	      }
	    
	    ModuleClass *tubeMod = tShip.ship->bus.tubeMod[tubeNum];
	    if ( !tubeMod )
	      {
		return;
	      }

	    int index = tubeMod->par[3];
	    if ( index < 0 || index >= game->numMissileDesigns )
	      {
		return;
	      }

	    MissileDesignStruct *md = &game->missileDesign[index];
	    if ( !md )
	      {
		return;
	      }

	    int dontDoAutoFire = 
	      md->trigger == on_proximity || 
	      md->trigger == bomb || 
	      tubeMod->moduleFunc == ModuleClass::ram ;

	    if ( tShip.ship->HasShift() )
	      {
		if ( tubeMod->CheckTubeMode(ModuleClass::auto_fire_mode) )
		  {
		    tubeMod->ClearTubeMode(ModuleClass::auto_fire_mode);
		    sprintf(temp, "tube %d has been configured for normal-fire.",tubeNum+1);
		  }
		else if ( !dontDoAutoFire )
		  {
		    tubeMod->SetTubeMode(ModuleClass::auto_fire_mode);
		    sprintf(temp, "1st Officer> tube %d has been configured for auto-fire.",tubeNum+1);
		  }
		game->PostGameMessage(temp,i,(short)tShip.ship->raceID,personal);
	      }
	    else if ( tShip.ship->HasControl() )
	      {
		if ( tubeMod->CheckTubeMode(ModuleClass::manual_fire_mode) )
		  {
		    tubeMod->ClearTubeMode(ModuleClass::manual_fire_mode);
		    sprintf(temp, "1st Officer> tube %d has been configured for normal-fire.",tubeNum+1);
		  }
		else
		  {
		    tubeMod->SetTubeMode(ModuleClass::manual_fire_mode);
		    sprintf(temp, "1st Officer> tube %d has been configured for manual-fire.",tubeNum+1);
		  }
		game->PostGameMessage(temp,i,(short)tShip.ship->raceID,personal);
	      }
	    else
	      {
		if (!game->player[tShip.ship->playerIndex].IsRestrictedFromUsing(tubeMod) )
		  {
		    tubeMod->activate();
		    tShip.ship->firingTubes = 1;
		    /* send a dummy message to the object to reset comVec */
		    tShip.ship->Command(BusStruct::no_message, nullVector,nullVector);
		  }
		else
		  {
		    sprintf(temp, "1st Officer> only "SF_REGISTERED_CLUB" may use that system.");
		    game->PostGameMessage(temp,i,(short)tShip.ship->raceID,personal);
		  }
	      }
	  }
	  break;
	case tractor_button_down:
	  if (IsValidObjectPointer(tShip.ship,__FILE__,__LINE__) &&
	      tShip.ship->bus.dockMod[0] &&
	      tShip.ship->bus.numPorts > 0 ) {
	    if (tShip.ship->bus.IsDockFree(0)) {
	      if (currentPlayer && 
		  tShip.ship->bus.dockMod &&
		  tShip.ship->bus.dockMod[0] &&
		  tShip.ship->bus.dockMod[0]->func < module_break_limit) {
		
		strcpy(temp,
		       "1st Officer> the primary docking system is offline!");
		game->PostGameMessage(temp,i,(short)tShip.ship->raceID,personal);
		
	      }
	      
	      if (tShip.ship != tShip.ship->targetPP.obj.ship) 
		tShip.ship->Command(BusStruct::fire_dock);
	      else if (currentPlayer) {
		strcpy(temp, "1st Officer> we can't tractor our own ship");
		game->PostGameMessage(temp,i,(short)tShip.ship->raceID,personal);
		
	      }
	      
	      
	    } else
	      if ( IsValidObjectPointer(tShip.ship) )
		{
		  tShip.ship->ReleaseDock(0);
		}
	  }
	  break;
	case go_dock_button_down:
	  if (tShip.ship->HasControl())
	    game->SendAssignToFleet(game->ship+i, DesignedClass::go_multi_dock);
	  else if (tShip.ship->dockObj)
	    game->GenericRelease(game->ship+i);
	  else if (tShip.ship->HasShift())
	    tShip.ship->Assign(DesignedClass::go_dock);
	  else
	    tShip.ship->Assign(DesignedClass::go_multi_dock);
	  break;
	case left_shift_button:
	case right_shift_button:
	  tShip.ship->SetMode(DesignedClass::mode_shift, op_on);
	  break;
	case establish_orbit_button:
	  /* reset planet pointer to force ship to go to nearest planet */
	  tShip.ship->assignPar2 = 0;
	  tShip.ship->EstablishOrbit();
	  break;
	case go_planet_button_down:
	  if ( !IsValidObjectPointer(tShip.ship) )
	    {
	      break;
	    }
	  if (!tShip.ship->dockObj) {

	    if (IsLocalPlayer(tShip.ship)) {
	  
	      PingPacket pp = tShip.ship->Ping(planet_target, specific, short(tShip.ship->assignPar2));
	      if (pp.dist > (slong)min_warp_dist &&
		  tShip.ship->bus.warpMod && 
		  tShip.ship->bus.warpMod->check == 0xAAAAAAAA && 
		  tShip.ship->bus.warpMod->par[1] < tShip.ship->mass) {

		/* the ship will use q drive to go to target */
		strcpy(temp, "1st Officer> Our ship is too massive to engage q drive.");
		game->PostGameMessage(temp,i,(short)tShip.ship->raceID,personal);
	      }

	    }

	    tShip.ship->GoPlanet();
	    if (tShip.ship->HasShift() &&
		game->planet[short(tShip.ship->assignPar2)].colony)
	      tShip.ship->targetPP.obj.colony = 
		game->planet[short(tShip.ship->assignPar2)].colony;
	  } else {

	    strcpy(temp2, "1st Officer> To engage the Q-Drive, we must undock!");
	    game->PostGameMessage(temp2,i,(short)tShip.ship->raceID,personal);
	  
	  }
	  break;
	case distress_button_down:
	  /* standard call:                                    */
	  /*    send a low priority distress call to all ships */
	  /* shift button pressed:                             */
	  /*    send a priority distress call to all ships     */
	  /*    priority is based on rank                      */
	  /* control button pressed:                           */
	  /*    send an attack message to the fleet            */

	  j = 0;

	  if ( IsValidObjectPointer(tShip.ship->targetPP.obj.generic) ) {
	    if (!tShip.ship->HasControl()) {
	      j =
		tShip.ship->SendDistressSignal(tShip.ship->HasShift()*
					       game->playerList[i]->rank,
					       ship);
	    } else {
	      game->SendAssignToFleet(game->ship+i,
				      DesignedClass::attack,
				      tShip.ship->targetPP.obj.objLong);
	    }
	  }

	  if (currentPlayer && j) {
	  
	    sprintf(temp, "1st Officer> %d friendly ships are responding", j);
	    game->PostGameMessage(temp, i,(short)tShip.ship->raceID, personal);
	  
	  }
	  break;
	
	case system_button:

	  tShip.ship->videoMode = VIDEO_STATUS;

	  if (console && IsLocalPlayer(tShip.ship)) {
	    console->SetMode(VIDEO_STATUS);
	    console->SetNumHeaderLines(0);

	    if (tShip.ship->videoMode == VIDEO_STATUS) {
	      if (tShip.ship->HasShift())
		subMode = 1;
	      else
		subMode = 0;
	    }
	  }

	  break;
	case message_button:
	  tShip.ship->videoMode = VIDEO_MESSAGE;

	  if (console && IsLocalPlayer(tShip.ship)) {
	    console->SetMode(VIDEO_MESSAGE);
	    console->SetNumHeaderLines(0);
	  }

	  break;
	case query_mark_button:
	  if (game->race[tShip.ship->raceID].targetIndex) {

	    if (tShip.ship->HasShift()) {
	      j = tShip.ship->PlanetAdvisor(ShipClass::planet_to_harvest);
	      sprintf(temp,
		      "1st Officer> %s is the next planet to be harvested.",
		      game->planet[j].name);
	      if (j) {
		tShip.ship->assignPar2 = j;

		tShip.ship->videoMode = VIDEO_NAVIGATE;

		if (console && IsLocalPlayer(tShip.ship)) {
		  console->mode = VIDEO_NAVIGATE;
		  console->SetNumHeaderLines(0);
		}

	      }
	    } else {
	      sprintf(temp, "1st Officer> %s is our current colonization target.",
		      game->planet[game->race[tShip.ship->raceID].targetIndex].name);
	      if (game->race[tShip.ship->raceID].targetIndex) {
		tShip.ship->assignPar2 = game->race[tShip.ship->raceID].targetIndex;
		tShip.ship->videoMode = VIDEO_NAVIGATE;

		if (console && IsLocalPlayer(tShip.ship)) {
		  console->SetMode(VIDEO_NAVIGATE);
		  console->SetNumHeaderLines(0);
		}

	      }
	    }

	  } else {
	    if (tShip.ship->HasShift()) {
	      j = tShip.ship->PlanetAdvisor(ShipClass::planet_to_harvest);
	      sprintf(temp,
		      "1st Officer> %s is the next planet to be harvested.",
		      game->planet[j].name);
	      if (j) {
		tShip.ship->assignPar2 = j;

		tShip.ship->videoMode = VIDEO_NAVIGATE;

		if (console && IsLocalPlayer(tShip.ship)) {
		  console->SetMode(VIDEO_NAVIGATE);
		  console->SetNumHeaderLines(0);
		}

	      }
	    } else {
	      j = tShip.ship->PlanetAdvisor(ShipClass::planet_to_colonize);
	      sprintf(temp,
		      "1st Officer> %s is the next target for colonization.",
		      game->planet[j].name);
	      if (j) {
		tShip.ship->assignPar2 = j;

		tShip.ship->videoMode = VIDEO_NAVIGATE;

		if (console && IsLocalPlayer(tShip.ship)) {
		  console->SetMode(VIDEO_NAVIGATE);
		  console->SetNumHeaderLines(0);
		}

	      }
	    }

	  }
	  
	  game->PostGameMessage(temp,
				i,
				(short)tShip.ship->raceID,
				personal);
	  break;
	case mark_button:
	  if (tShip.ship->videoMode == VIDEO_NAVIGATE) {
	    if (game->player[tShip.ship->playerIndex].rank >=
		game->race[tShip.ship->raceID].targetIndexRank) {
	      game->race[tShip.ship->raceID].targetIndex = (short)tShip.ship->assignPar2;
	      sprintf(temp, "%s has been targeted for colonization.",
		      game->planet[game->race[tShip.ship->raceID].targetIndex].name);
  
	      game->PostGameMessage(temp,
				    (short)tShip.ship->raceID,
				    (short)tShip.ship->raceID,
				    broadcast);
	    } else {
	      sprintf(temp, "1st Officer> %s has already been targeted by a %s",
		      game->planet[game->race[tShip.ship->raceID].targetIndex].
		      name,
		      game->rank[game->race[tShip.ship->raceID].
				targetIndexRank].name);
	    
	      game->PostGameMessage(temp,
				    i,
				    (short)tShip.ship->raceID,
				    personal);
	    }
	  }
	  break;
	case racelist_button:
	
	  /* if the shift button is not pressed then switch to RaceList */
	  if (!tShip.ship->HasShift()) {
	    tShip.ship->videoMode = VIDEO_RACELIST;

	    if (console && i == localSlotNumber) {
	    
	      console->SetMode(VIDEO_RACELIST);
	      console->SetNumHeaderLines(0);
	    
	      subMode = 0;
	      //if (tShip.ship->videoMode == VIDEO_RACELIST)
	      // subMode = (subMode + 1) % 2;
	      //else
	      /* adjust the submode if necessary; currently there are 2 submodes */
	      //  subMode = subMode % 2;
	    }
	  
	  } else {

	    tShip.ship->videoMode = VIDEO_RANKLIST;

	    if (console && i == localSlotNumber) {
	    
	      console->SetMode(VIDEO_RANKLIST);
	      console->SetNumHeaderLines(1);
	      subMode = 0;

	    }

	  }

	  break;
	case playerlist_button:
	
	  if (!tShip.ship->HasShift()) {

	    tShip.ship->videoMode = VIDEO_PLAYERLIST;

	    if (console && i == localSlotNumber) {
	    
	      console->SetMode(VIDEO_PLAYERLIST);
	      console->SetNumHeaderLines(1);
	      subMode = 0;

	    }
	  
	    changeMode = 1;
	  } else {

	    tShip.ship->videoMode = VIDEO_PLANETLIST;
	  
	    if (console && i == localSlotNumber) {
	    
	      console->SetMode(VIDEO_PLANETLIST);
	      console->SetNumHeaderLines(0);
	      subMode = 0;

	      if (selPointer == 0)
		selPointer = 1;
	    
	      if (tShip.ship->assignPar2) {

		console->scroll = max(0, tShip.ship->assignPar2 - console->chy + 1 +
				      console->numHeaderLines);
		selPointer = (short)(max(tShip.ship->assignPar2 - console->scroll, 0) + console->scroll + 1);

	      }

	    }
	  
	    changeMode = 1;

	  }
	  break;
	case navigate_button:
	  tShip.ship->videoMode = VIDEO_NAVIGATE;

	  if ( console && i == localSlotNumber) {
	    console->SetMode(VIDEO_NAVIGATE);
	    console->SetNumHeaderLines(0);
	  }

	  tShip.ship->AssignNearestPlanet(tShip.ship->HasShift());
	  break;
	case exchange_button:
	  game->ExchangeShips(i);
	  break;
	case view_button_down:
	  tShip.ship->lastVideoMode = tShip.ship->videoMode;
	  tShip.ship->videoMode = VIDEO_VIEW;

	  if ( console && i == localSlotNumber) {
	    console->SetMode(VIDEO_VIEW);
	    console->SetNumHeaderLines(0);
	  }

	  //viewObj = game->GetNextViewObj(tShip.ship, 1);
	  //if (viewObj)
	  // tShip.ship->View(viewObj);

	  break;
	case auto_button_down:
	  tShip.ship->SetMode(DesignedClass::mode_auto, op_toggle);
	  if (i == localSlotNumber) {
	    if (tShip.ship->AutoResponse()) 
	      strcpy(temp, "1st Officer> I have the con.");
	    else
	      strcpy(temp, "1st Officer> you have the con.");

	    game->PostGameMessage(temp, i,(short)tShip.ship->raceID, personal);

	  }
	  break;
	case destruct_button_down:
	  if (tShip.ship->HasControl() &&
	      game->player[tShip.ship->playerIndex].IsDeveloper() &&
	      IsValidObjectPointer(tShip.ship->targetPP.obj.generic) )
	    tShip.ship->targetPP.obj.generic->Explode("it self-destructed.");
	  else
	    tShip.ship->Assign(DesignedClass::self_destruct);
	  break;
	case detonate_button:
	case alt_detonate_button:
	  game->DetonateMissilesOf(game->ship+i);
	  if (tShip.ship->HasShift() &&
	      tShip.ship->HasControl() &&
	      game->player[tShip.ship->playerIndex].IsDeveloper()) {
	    for (j=0; j < game->N(GameStateClass::missiles); j++)
	      {
		if ( IsValidObjectPointer(game->missileList[j]) )
		  {
		    game->missileList[j]->Explode("it was detonated");
		  }
	      }
	  }
	  break;
	case refit_button:
	  /* *** This code is never run in response to a user key event anymore *** */
	  if (!tShip.ship->HasControl())
	    game->DecideToRefitShip(i);
	  else if ( IsValidObjectPointer(tShip.ship->targetPP.obj.generic) &&
		    tShip.ship->targetPP.obj.generic->IsFriendlyTo(game->ship+i) &&
		    tShip.ship->targetPP.obj.generic->Type() == ship &&
		    tShip.ship->targetPP.obj.ship->stat == DesignedClass::under_construction) {
	    if (game->player[tShip.ship->playerIndex].rank >
		game->player[tShip.ship->targetPP.obj.ship->playerIndex].rank) {
	      game->DecideToRefitShip(game->TypeListIndex(tShip.ship->targetPP.obj.ship),
				      1 /* forced */ ,1 /* under construction */, tShip.ship);
	      sprintf(temp, "1st Officer> refitting the %s to a %s...", 
		      game->player[tShip.ship->targetPP.obj.ship->playerIndex].
		      shipName,
		      game->shipDesign[game->player[tShip.ship->targetPP.obj.ship->
						   playerIndex].shipIndex].name);
	    } else
	      strcpy(temp,
		     "1st Officer> you do not outrank the targeted player");

	    game->PostGameMessage(temp, i,(short)tShip.ship->raceID, personal);

	  }
	  break;
	case alt_drop_button:
	case drop_button:
	  //game->Defect(i);
	  if (tShip.ship->HasShift())
	    tShip.ship->DropModule();
	  else if (tShip.ship->videoMode == VIDEO_STATUS)
	    tShip.ship->ToggleModuleMode();
	  break;
#ifdef OS2
	case ctl_button_right:
#endif
	case ctl_button_left:
	  tShip.ship->SetMode(DesignedClass::mode_control, op_on);
	  break;
	default:
	  tShip.ship->Command(BusStruct::no_message);
	  break;
	}
	break;	

      case SF_MSG_KEYRELEASED_4:
	switch(gmsg->scancode) {
	case all_stop_button_down:
	  tShip.ship->Command(BusStruct::release_brake);
	  break;
	case thrust_button: 
	case thrust_reverse_button:
	case alt_thrust_reverse_button:
	case thrust_left_button:
	case thrust_right_button:
	  tShip.ship->Command(BusStruct::release_thrust);
	  break;
	case right_arrow_down:
	case alt_right_arrow_down:
	  tShip.ship->SetMode(DesignedClass::mode_right, op_off);
	  if (!tShip.ship->HasLeftButtonDown())
	    tShip.ship->Straight();
	  else
	    tShip.ship->TurnLeft();
	  break;
	  tShip.ship->Straight();
	  break;
	case left_arrow_down:
	case alt_left_arrow_down:
	  tShip.ship->SetMode(DesignedClass::mode_left, op_off);
	  if (!tShip.ship->HasRightButtonDown())
	    tShip.ship->Straight();
	  else
	    tShip.ship->TurnRight();
	  break;
	  tShip.ship->Straight();
	  break;
	case alt_thrust_button: 
	case boost_button_down:
	  tShip.ship->Command(BusStruct::release_boost);
	  break;
	  /* case tractor_button_down:
	     tShip.ship->Command(BusStruct::release_dock);
	     break; */
	case right_shift_button:
	case left_shift_button:
	  tShip.ship->SetMode(DesignedClass::mode_shift, op_off);
	  break;
	case view_button_down:
	  tShip.ship->videoMode = tShip.ship->lastVideoMode;

	  if (console && IsLocalPlayer(tShip.ship))
	    console->SetMode(tShip.ship->videoMode);

	  tShip.ship->View(tShip.ship);
	  break;
#ifdef OS2
	case ctl_button_right:
#endif
	case ctl_button_left:
	  tShip.ship->SetMode(DesignedClass::mode_control, op_off);
	  break;
	case destruct_button_down:
	  tShip.ship->Assign(DesignedClass::manual);
	  break;

	case save_button:
	  break;
	  if (IsLocalPlayer(tShip.ship)) {

	    char htmlFilename[256];
	    char dataFilename[256];
	    strcpy(htmlFilename, game->fileName);

	    j = strlen(htmlFilename);
	    while ( htmlFilename[i] != '.' && j >= 0 )
	      j--;
	    htmlFilename[i] = 0;
	    //*strstr(htmlFilename, ".dat") = '\0';

	    strcpy(dataFilename, htmlFilename);
	    strcat(htmlFilename, ".html");
	    strcat(dataFilename, ".dat");

#if defined(WIN32) || defined(OS2)
	    strcpy(dataFilename, "..\\dat\\Scenario.dat");
#else
	    strcpy(dataFilename, "../dat/Scenario.dat");
#endif

	    /* write out html or data file */
	    if (tShip.ship->HasShift())
	      game->WriteFile(html_file, htmlFilename);
	    else
	      game->WriteFile(data_file, dataFilename);

	  }
	  break;
	case alt_select_button:
	case select_button:
	  if (tShip.ship->videoMode == VIDEO_STATUS) {

	    tShip.ship->videoMode = VIDEO_SELECT;
	  
	    if (IsLocalPlayer(tShip.ship)) {
	      if (tShip.ship->HasShift()) {
		selType = -1;
		selPointer = 3;
	      } else {
		selType = tShip.ship->bus.moduleList[tShip.ship->bus.modPtr].moduleFunc;
		selPointer = 6;
	      }
	    }

	    if (console &&  i == localSlotNumber) {
	      console->mode = VIDEO_SELECT;
	      console->SetNumHeaderLines(int(selPointer));
	    }
	  
	    /* set the pointer so it points to the current system in the list */
	    /* j = k = 0;
	     
	       while (j < game->numModules &&
	       game->moduleList[j].id !=
	       tShip.ship->bus.moduleList[tShip.ship->bus.modPtr].id) {
	       if (game->tech[tShip.ship->raceID].IsSet(j) &&
	       game->moduleList[j].moduleFunc == selType)
	       k++;
	       j++;
	       }
	     
	       selPointer = k; */
	  } else if (tShip.ship->videoMode == VIDEO_RACELIST) {
	    if (console &&  i == localSlotNumber) {
	      console->scroll = 0;
	      if (subMode == 1) {
		subMode = 0;
	      } else {
		subMode = 1;
	      }
	    }
	  }  else if (tShip.ship->videoMode == VIDEO_PLANETLIST) {
	    tShip.ship->assignPar2 = selIndex;
	    tShip.ship->videoMode = VIDEO_NAVIGATE;
	    if (IsLocalPlayer(tShip.ship)) {
	      
	      if (console) {
		console->SetMode(VIDEO_NAVIGATE);
		console->SetNumHeaderLines(0);
	      }
	      
	      if (IsConnectedClient())
		retMsgBuffer.PushMsg((uchar)SF_MSG_SETNAVTARGET,
				     (ushort)game->ObjectListIndex(tShip.ship),
				     (ushort)game->ObjectListIndex(game->planet + selIndex),
				     (int)S_CLIENT_TCP);
	      
	    }
	  } else if (tShip.ship->videoMode == VIDEO_SELECT) {
	  
	    tShip.ship->videoMode = VIDEO_STATUS;
	  
	    if (console && i == localSlotNumber) {
	      console->mode = VIDEO_STATUS;
	      console->SetNumHeaderLines(0);
	      console->scroll = console->numHeaderLines;

	      slong credit;

	      short isUpgrade =
		(selType != -1 && selPointer > 2 && 
		 /* to prevent players from adding systems that don't match the existing type... */
		 tShip.ship->bus.moduleList[tShip.ship->bus.modPtr].
		 IsEquivalentTo(game->moduleList + selIndex)) || (selType == -1);
	    
	      if (isUpgrade && 
		  !game->player[tShip.ship->playerIndex].IsRestrictedFromUsing(game->moduleList + selIndex) &&
		  tShip.ship->dockObj && tShip.ship->dockObj->bus.factoryMod &&
		  (credit = game->playerList[i]->refValue - game->moduleList[selIndex].value() +
		   tShip.ship->bus.moduleList[tShip.ship->bus.modPtr].value()) >= 0 &&
		  tShip.ship->Volume() + game->moduleList[selIndex].volume -
		  tShip.ship->bus.moduleList[tShip.ship->bus.modPtr].volume <=
		  tShip.ship->design->volumeCap) {

		tShip.ship->PlaySoundOnScreen(game->soundEvent[upgrade_event]);

		/* a system is being replaced */
		if (selType != -1) {

		  /* retMsgBuffer.PushMsg((uchar)SF_MSG_GETPOWERUP,
		     (ushort)game->ObjectListIndex(tShip.ship),
		     (ushort)tShip.ship->bus.modPtr,
		     selID,
		     credit,
		     (int)S_CLIENT_TCP); */
		  tShip.ship->SendGetPowerUpMsg(tShip.ship->bus.modPtr, selID, credit);
		  //tShip.ship->ReceiveGetPowerUpMsg(tShip.ship->bus.modPtr, selID, credit);

		} else {
		  short typeMatched, volumeExceeded, iMin; 
		  if ( selIndex >= 0 && selIndex < game->numModules &&
		       game->moduleList[selIndex].check == 0xAAAAAAAA )
		    { 
		      iMin = tShip.ship->FindModuleToReplace(game->moduleList[selIndex],
							     typeMatched, volumeExceeded, 1, 1, 1);
		      tShip.ship->PlaceModule(game->moduleList[selIndex],
					      iMin, typeMatched, volumeExceeded, 1);
		    }
		}

	      } else if (game->player[tShip.ship->playerIndex].IsRestrictedFromUsing(game->moduleList + selIndex)) {
		sprintf(temp, "1st Officer> only "SF_REGISTERED_CLUB" may use that system.");
		game->PostGameMessage(temp,
				      i,
				      (short)tShip.ship->raceID,
				      personal);	      

	      }
	    }

	  }
	  //if (currentPlayer && tShip.ship->bus.warpMod &&
	  //    tShip.ship->bus.warpMod->func < module_break_limit) {

	  //strcpy(temp, "1st Officer> the Q drive is offline!");
	  //  game->PostGameMessage(temp, i, tShip.ship->raceID, personal);

	  //}
	  //tShip.ship->ClearAIParams();
	  //tShip.ship->Command(BusStruct::fire_warp);
	  break;

	default:
	  if ( !IsValidObjectPointer(tShip.ship) )
	    {
	      return;
	    }
	  tShip.ship->Command(BusStruct::no_message);
	  break;
	}

	/* if a warning is posted and a key is released then remove warning */
	if (currentPlayer && warningMessagePosted) {
	  *temp = '\0';
	  SetScreenMessage(temp, 0);
	  warningMessagePosted = 0;
	}
	break;

    case SF_MSG_DISCONNECT:

      if ( !IsValidObjectPointer(tShip.ship) )
	{
	  return;
	}

      //
      // Check for RAM cheat first.
      //
      game->player[tShip.ship->playerIndex].
	AddToStat(game->player[tShip.ship->playerIndex].totalScore,
		  (ulong)0
		  );
       SetShipLag(gmsg->slot,0);

       sprintf(temp,"Goodbye %s",game->player[tShip.ship->playerIndex].name);
       PushMessageNew(FROM_GOD,TO_BROADCAST,temp);

       tShip.ship->PlaySound(game->soundEvent[player_logoff_event]);

       if ( game->playerList[i] )
	 {
	   logFile << 
	     "received> Client Logout: " << 
	     tShip.ship->playerIndex << 
	     " out of " << 
	     game->numStoredPlayers <<
	     " - " << 
	     game->playerList[i]->name <<
	     " on frame " << 
	     game->frame << 
	     endl;
	 }

       if (tShip.ship->playerIndex >= game->N(GameStateClass::robot_players))
	  game->RemovePlayer(tShip.ship->playerIndex);
       else
	  game->player[tShip.ship->playerIndex].deactivate();
       if ( IsValidObjectPointer(tShip.ship->dockObj))
          game->GenericRelease(game->ship+i);

       /* clear the last attacker to prevent them from receiving */
       /* credit for the kill                                    */
       tShip.ship->lastAttacker.generic = 0;

       game->CleanOrphanObjects(tShip.ship);
       //tShip.ship->Explode("the client logged out", 0, 1);
       tShip.ship->deactivate();
       tShip.ship->View(game->planet.rootOf());
       break;

    case SF_MSG_PLAYERINFO:

      if ( !IsValidObjectPointer(tShip.ship) )
	{
	  return;
	}

      //
      // Reset the checksum so we aren't penalized by others past sins.
      //
      game->player[tShip.ship->playerIndex].CheckSum();
      
      tShip.ship->initializeWith(i-game->N(GameStateClass::first_ship), 
				 tShip.ship->playerIndex); 

      tShip.ship->PlaySound(game->soundEvent[player_logon_event]);

       /* reset credit value */
      //game->playerList[i]->refValue = 0;

       game->player[tShip.ship->playerIndex].numMissions++;
       game->player[tShip.ship->playerIndex].CalculateRank(game->TypeListIndex(tShip.ship), 1);
       game->CheckRank(tShip.ship->playerIndex, tShip.ship -
	               game->N(GameStateClass::first_ship));
       tShip.ship->activate();
       game->player[tShip.ship->playerIndex].activate();

       logFile << 
	 "received> Client Login: " << 
	 tShip.ship->playerIndex << 
	 " out of " << 
	 game->numStoredPlayers <<
	 " (" <<
	 game->player[tShip.ship->playerIndex].ipAddress <<
	 ") (" <<
	 game->player[tShip.ship->playerIndex].serial <<
	 ") (slot " << 
	 game->TypeListIndex(game->ship+i) << 
	 "/" <<
	 game->ObjectListIndex(game->ship+i) << 
	 ") " <<
	 game->playerList[i]->name <<
	 " on frame " << 
	 game->frame << 
	 endl;

       game->BalanceNewPlayerOnServer(i);

       game->player[tShip.ship->playerIndex].profile(game);

       sprintf(temp,"%s logged in as a %s",
	       game->player[tShip.ship->playerIndex].name,
	       game->race[game->player[tShip.ship->playerIndex].
			  raceId].name);
       game->PostGameMessage(temp, 0, 0, broadcast, 1);

       break;

    case SF_MSG_REL_MOUSEBUTTONDOWN_12 :

      if ( !IsValidObjectPointer(tShip.ship) )
      {
        return;
      }

      switch(gmsg->scancode) {
         case 1 :
            if ( tShip.ship->HasShift() ) {
	       tShip.ship->AcquireNearPoint(CoordinateClass( gmsg->x,
                                                              -gmsg->y) );
               tShip.ship->videoMode = VIDEO_TARGET;

	       if (console && IsLocalPlayer(tShip.ship)) {
		 console->SetMode(VIDEO_TARGET);
		 console->SetNumHeaderLines(0);
	       }

	       /* request an update from the server */
	       if (IsConnectedClient() && IsLocalPlayer(tShip.ship))
		 tShip.ship->SendSetTargetMsg(tShip.ship->targetPP.obj);
	       
	    } else if ( tShip.ship->HasControl() )
	      game->SendCommandToFleet(game->ship+i,
				       BusStruct::fire_tubes_at_angle,
				       CoordinateClass(gmsg->x,-gmsg->y)
				       );
            else {

               if (currentPlayer && tShip.ship->bus.tubeMod) {
                  int n1 = 0;
                  int n2 = 0;
                  for (j=0; j<tShip.ship->bus.numTubes; j++) {
                     if (tShip.ship->bus.tubeMod[j]->func<module_break_limit)
                        n1++;
                     else if (tShip.ship->bus.tubeMod[j]->moduleFunc != ModuleClass::beam &&
			      tShip.ship->bus.tubeMod[j]->workPar[0] == 0)
                        n2++;
                     }
	  
                  *temp = '\0';
                  *msg = '\0';
	  
                  if (n1) {
		    if (n1 == 1)
		      strcpy(temp, "1 tube offline!");
		    else if (n1 < tShip.ship->bus.numTubes)
		      sprintf(temp, "%d tubes offline!", n1);
		    else
		      strcpy(temp, "all tubes offline!");
		  }
                  if (n2) {
		    if (n2 == 1)
		      strcpy(msg, "1 tube out of missiles!");
		    else if (n2 < tShip.ship->bus.numTubes)
		      sprintf(msg, "%d tubes out of missiles!", n2);
		    else
		      strcpy(msg, "all tubes out of missiles!");
		  }
		  
                  if (n1 || n2) {
		    sprintf(temp2, "1st Officer> %s %s", temp, msg);
		    game->PostGameMessage(temp2, i, 
					  (short)tShip.ship->raceID,
					  personal);
		  }
	       }
               tShip.ship->Command(BusStruct::fire_tubes_at_angle,
                                     CoordinateClass(gmsg->x,-gmsg->y),
	                             nullVector);
	    }
            break;
	    
         case 2 :
            tShip.ship->Command(BusStruct::fire_boost);
            tShip.ship->Assign(DesignedClass::seek_heading,
		                 CoordinateClass(gmsg->x,-gmsg->y).
		                 Heading(tShip.ship->bitmap.N()));

	    if (tShip.ship->dockObj)
	      /* if ship thinks it is docked then request freedom from */
	      /* the ship that owns the port                           */
	      tShip.ship->FreeFromDock();

            break;

         case 3 :
            tShip.ship->AcquireNearPoint(CoordinateClass(gmsg->x,-gmsg->y));
            break;
         }

      break;

    case SF_MSG_REL_MOUSEBUTTONUP_12 :

      if ( !IsValidObjectPointer(tShip.ship) )
	{
	  return;
	}

      switch(gmsg->scancode) {
         case 1 :
         case 3 : tShip.ship->Command(BusStruct::no_message);
                  break;
         case 2 : tShip.ship->Command(BusStruct::release_boost);
                  break;
         }
      break;
    case SF_MSG_ADDTOPLAYERSCORE :
      {
	short shipIndex = (short)(gmsg->dslot - game->shipObjectIndexStart);

	if (shipIndex >= 0 && shipIndex < game->numShips) {
          if ( gmsg->bonus > max_bonus )
	    {
              gmsg->bonus = max_bonus;
	    }

          
	  game->playerList[shipIndex]->AddToStat(game->playerList[shipIndex]->totalScore,
						      (ulong)gmsg->bonus
						      );
	  game->playerList[shipIndex]->AddToStat(game->playerList[shipIndex]->activeScore,
						      (ulong)gmsg->bonus
						      );
	  game->playerList[shipIndex]->AddToStat(game->playerList[shipIndex]->strategicPoints,
						      (ulong)gmsg->bonus
						      );
	  game->playerList[shipIndex]->SetStat(game->playerList[shipIndex]->refValue,
						   min(game->playerList[shipIndex]->refValue + gmsg->credit,
						       max_credit)
						   );
	  
	  //game->playerList[shipIndex]->totalScore += gmsg->bonus;
	  //game->playerList[shipIndex]->activeScore += gmsg->bonus;
	  //game->playerList[shipIndex]->strategicPoints += gmsg->bonus;
	  //game->playerList[shipIndex]->refValue = min(game->playerList[shipIndex]->refValue + gmsg->credit,
	  //					      max_credit);

	  if (!IsServer()) {

	    if (IsLocalPlayer(game->ship + shipIndex)) {
	      if (gmsg->bonus) {
		sprintf(temp, "1st Officer> we have received a bonus score of %s",
			Format(gmsg->bonus));
	    
		game->PostGameMessage(temp, shipIndex, short(game->playerList[shipIndex]->raceId),
				      personal);
	      }
	      if (gmsg->credit) {
		sprintf(temp, "1st Officer> we have received a bonus credit of %s",
			Format(gmsg->credit));
		
		game->PostGameMessage(temp, shipIndex, short(game->playerList[shipIndex]->raceId),
				      personal);
	      }
	    } else {
	      sprintf(temp, "1st Officer> %s has received a bonus score of %s",
		      game->playerList[shipIndex]->name, Format(gmsg->bonus));
	    
	      game->PostGameMessage(temp, short(game->playerList[shipIndex]->raceId),
				    short(game->playerList[shipIndex]->raceId),
				    broadcast);
	      
	    }
	  }

	} else
	  logFile << "ERROR> invalid ship index (" << shipIndex
		  << ") received while receiving end-of-game bonus." << endl;
      }
      break;
    case SF_MSG_RESETGAME :
      logFile << "received> game over (winner = " << 
	game->race[gmsg->race].name << ") on frame " << game->frame << endl;
      game->race[gmsg->race].status = RaceClass::victorious;
      game->RandomizeGameState(0);
      break;

    case SF_MSG_PLANETSIEGESUCCEEDS :

      game->colonyPending = 0;

      if ( !IsValidObjectPointer(obj.generic) )
	{
	  return;
	}

      if (obj.generic->Type() == planet) {

	if (gmsg->race < 0 || gmsg->race >= game->numRaces) {
	  
	  logFile << "ERROR> PLANETSIEGESUCCEEDS received race index (" <<
	    gmsg->race << ") out of range on frame " <<
	    game->frame << endl;

	}

	logFile << "received> a ";
	obj.planet->ShortProfile();
	logFile << " has been successfully besieged by the " << game->race[gmsg->race].name <<
	  " on frame " << game->frame << endl;

	sprintf(temp, "1st Officer> %s has surrendered to the %ss",
		obj.planet->name, game->race[gmsg->race].name);
	    
	game->PostGameMessage(temp, short(gmsg->race), short(gmsg->race), broadcast);

	obj.planet->raceID = gmsg->race;

	if (obj.planet->colony) {
	  obj.planet->colony->raceID = gmsg->race;
	  obj.planet->colony->ClearPendingFlag(ShipClass::captured);
	  /* if this object was building something convert it */
	  obj.planet->colony->ConvertConstructionProject();

	  /* give the planet ecology a boost to make the new population more loyal */
	  obj.planet->terraForm = (obj.planet->terraForm) +
	    (terraform_scale - obj.planet->terraForm) / 5;
	  
	}

      } else {

	logFile << "ERROR> PLANETSIEGESUCCEEDS received a message for a ";
	obj.generic->ShortProfile();
	logFile << " on frame " << game->frame << endl;

      }

      break;

    case SF_MSG_CAPTURE :

      if ((short)gmsg->dslot != -1)
	tobj.generic = game->objectList[short(gmsg->dslot)];
      else
	tobj.objLong = 0L;
      
      if ( IsValidObjectPointer(obj.ship) && IsValidObjectPointer(tobj.ship) )
	{
	  tobj.ship->ReceiveCaptureMsg(obj.ship, (short)gmsg->index);
	}
      break;

    case SF_MSG_HUMANCAPTURE :

      if ((short)gmsg->dslot != -1)
	tobj.generic = game->objectList[short(gmsg->dslot)];
      else
	tobj.objLong = 0L;
      
      if ( IsValidObjectPointer(obj.ship) && IsValidObjectPointer(tobj.ship) )
	{
	  obj.ship->ReceiveCaptureMsg(tobj.ship, (short)gmsg->index);
	}
      break;

    case SF_MSG_SETTARGET :

      if ( IsValidObjectPointer(obj.ship) )
	{
	  obj.ship->targetPP.obj.generic = game->objectList[(short)gmsg->tslot];
	}      
      break;

    case SF_MSG_SETNAVTARGET :

      if ( IsValidObjectPointer(obj.ship) )
	{
	  obj.ship->assignPar2 = game->TypeListIndex(game->ObjectListObj((short)gmsg->tslot).planet);
	}
      break;

    case SF_MSG_SETRANKINDEX :
     
      game->playerList[short(gmsg->slot)]->ReceiveRankingMsg((short)gmsg->slot,
							     (short)gmsg->index);

      break;
    case SF_MSG_SETMODULEPOINTER :

      if ( IsValidObjectPointer(tShip.ship) )
      {
	if (gmsg->index >= 0 && gmsg->index < tShip.ship->bus.numMod)
	  tShip.ship->bus.modPtr = (uchar)gmsg->index;
	else {
	  logFile << "ERROR> SETMODULEPOINTER received bad pointer value (" <<
	    gmsg->index << ") for a ";
	  tShip.ship->ShortProfile();
	  logFile << " with " << tShip.ship->bus.numMod << " slots on frame " << game->frame << endl;
	  
	}
      }
      
      break;
    case SF_MSG_EXPLODEME :
    case SF_MSG_EXPLODESHIP :
      
      if ( IsValidObjectPointer(obj.generic,__FILE__,__LINE__) )
	{      
	  
	  if ((short)gmsg->dslot != -1)
	    tobj.generic = game->objectList[short(gmsg->dslot)];
	  else
	    tobj.objLong = 0L;
	  
	  if ( obj.generic->Type() == ::ship ||
	       obj.generic->Type() == ::colony ||
	       obj.generic->Type() == ::missile
	       )
	    {
	      if ( IsValidObjectPointer(tobj.generic) && 
		   tobj.generic->IsActive() )
		obj.ship->lastAttacker = tobj;
	      if (obj.generic->active)
		obj.ship->HandleExplodeMsg();
	    }
	}
      break;
      
    case SF_MSG_EXPLODECOLONY :
       break;

    case SF_MSG_INITIATEWARP :
      {
	ShipClass* s = game->ObjectListObj((short)gmsg->slot).ship;
	
	if ( IsValidObjectPointer(s) && s->Type() == ::ship )
	  {      

	    s->tubeSelector = 0;	    

	    s->assign = (short)gmsg->ai1;
	    
	    if ((short)gmsg->dslot != -1)
	      tobj.generic = game->objectList[short(gmsg->dslot)];
	    else
	      tobj.objLong = 0L;
	    
	    if ( IsValidObjectPointer(tobj.generic) )
	      {
		
		if ((s->assign == DesignedClass::follow_object ||
		     s->assign == DesignedClass::attack))
		  {
		    
		    if (!tobj.generic->IsActive())
		      {
			logFile << 
			  "ERROR> INITIATEWARP received inactive slot number (" <<
			  gmsg->dslot <<
			  ") on frame " << game->frame << endl;
			break;
		      }
		    
		    s->targetPP.obj.generic = game->objectList[short(gmsg->dslot)];
		  } else
		    {
		      s->assignPar2 = gmsg->dslot;
		    }
		WRITE_SF_LOG(logFile << "received> a ");
		WRITE_SF_LOG(s->ShortProfile()); 
		WRITE_SF_LOG(logFile << 
			     " has received a warp initiation message with the "
			     "target a ");
		WRITE_SF_LOG(game->objectList[(short)gmsg->dslot]->ShortProfile());
		WRITE_SF_LOG(logFile << " on frame " <<
			     game->frame << endl);
		
	      }
	    
	    s->ReceiveInitiateWarpMsg();
	  }
      }
      break;
      
      case SF_MSG_TERMINATEWARP :
	{
	  ShipClass* s = game->ObjectListObj((short)gmsg->slot).ship;
	  
	  if ( IsValidObjectPointer(s) && s->Type() == ::ship )
	    {      

	      s->tubeSelector = 0;	      

	      s->assign = (short)gmsg->ai1;
	      
	      if ( (s->assign == DesignedClass::follow_object ||
		    s->assign == DesignedClass::attack) &&
		   gmsg->dslot != ushort(-1) )
		{
		  s->targetPP.obj.generic = 
		    game->objectList[short(gmsg->dslot)];
		}
	      else
		{
		s->assignPar2 = gmsg->dslot;
		}
	      
	      s->now.x = gmsg->x;
	      s->now.y = gmsg->y;
	      s->gnow.x = gmsg->x;
	      s->gnow.y = gmsg->y;
	      
	      s->ReceiveTerminateWarpMsg();
	    }
	}
	break;

      case SF_MSG_UNDOCK :
	{
	  /* the slot object undocks from its dockObj */
	  
	  if ((short)gmsg->dslot != -1)
	    {
	      tobj.generic = game->objectList[short(gmsg->dslot)];
	    }
	  else
	    {
	      tobj.objLong = 0L;
	    }
	  
	  if ( !IsValidObjectPointer(tobj.generic) )
	    {
	      break;
	    }
	  
	  if ( IsValidObjectPointer(tobj.ship) )
	    {
	      tobj.ship->ReceiveUnDockMsg();
	    }
	  else
	    {
	      logFile << 
		"EnginePlusPlus: received a null pointer in SF_MSG_UNDOCK." << 
		endl;
	    }
	  
	}
	break;
	
      case SF_MSG_HUMANISDOCKED :
	{
	  
	  if ( (short)gmsg->dslot != -1 )
	    {
	      tobj.generic = game->objectList[short(gmsg->dslot)];
	    }
	  else
	    {
	      tobj.objLong = 0L;
	    }
	  
	  if ( !IsValidObjectPointer(tobj.generic,__FILE__,__LINE__) )
	    {
	      break;
	    }
	  
	  if ( !IsValidObjectPointer(obj.generic,__FILE__,__LINE__) )
	    {
	      break;
	    }
	  
	  if ( !tobj.generic->IsActive() )
	    {
	      logFile << 
		"ERROR> HUMANISDOCKED received inactive slot number (" << 
		gmsg->dslot <<
		") on frame " << 
		game->frame << 
		endl;
	      break;
	    }
	  
	  if ( gmsg->dslot>=0 && 
	       gmsg->dslot < game->FirstMissileObjectIndex() &&
	       gmsg->slot>=0 && 
	       gmsg->slot < game->FirstMissileObjectIndex() &&
	       !tobj.generic->IsExploding() &&
	       tobj.designed->bus.moduleList.IsNotNull() &&
	       gmsg->port >= 0 && 
	       gmsg->port < tobj.designed->bus.numMod &&
	       tobj.designed->bus.moduleList[short(gmsg->port)].moduleFunc == 
	       ModuleClass::dock) {
	    
	    tobj.designed->bus.moduleList[short(gmsg->port)].
	      ReceiveDockingSuccessfulMsg((short)gmsg->slot);
	    
	    if (gmsg->slot == localSlotNumber &&
		tShip.ship->videoMode != VIDEO_STATUS ) {
	      retMsgBuffer.PushMsg((uchar)SF_MSG_KEYPRESSED_4,
				   (ushort)gmsg->slot,
				   (uchar)frameNumber,
				   (uchar)SF_KEY_S,
				   (int)S_CLIENT_TCP);
	    }
	    
	  } else {
	    
	    logFile << 
	      "ERROR> illegal HUMANISDOCKED message received on frame " << 
	      game->frame << 
	      ".  slot = " <<
	      obj.generic->TypeName() << 
	      " " << 
	      game->TypeListIndex(obj.generic) << 
	      " dslot = " <<
	      tobj.generic->TypeName() << 
	      " " << 
	      game->TypeListIndex(tobj.generic) << 
	      " port = " << 
	      gmsg->port << 
	      endl;
	    
	  }
	  
	}
	break;
    
      case SF_MSG_ISDOCKED :
	{
	  
	  if ( (short)gmsg->dslot != -1 )
	    {
	      tobj.generic = game->objectList[short(gmsg->dslot)];
	    }
	  else
	    {
	      tobj.objLong = 0L;
	    }
	  
	  if ( !IsValidObjectPointer(tobj.generic,__FILE__,__LINE__) )
	    {
	      break;
	    }
	  
	  if ( !IsValidObjectPointer(obj.generic,__FILE__,__LINE__) )
	    {
	      break;
	    }
	  
	  if ( !tobj.generic->IsActive() ) {
	    logFile << 
	      "ERROR> ISDOCKED received inactive slot number (" << 
	      gmsg->dslot <<
	      ") on frame " << 
	      game->frame << 
	      endl;
	    break;
	  }
	  
	  if ( gmsg->slot>=0 && 
	       gmsg->slot < game->FirstMissileObjectIndex() &&
	       gmsg->dslot>=0 && 
	       gmsg->dslot < game->FirstMissileObjectIndex() &&
	       obj.ship->IsActive() && 
	       !obj.generic->IsExploding() &&
	       obj.designed->bus.moduleList.IsNotNull() &&
	       gmsg->port >= 0 && 
	       gmsg->port < obj.designed->bus.numMod &&
	       obj.designed->bus.moduleList[short(gmsg->port)].moduleFunc == 
	       ModuleClass::dock )
	    {
	      
	      obj.designed->bus.moduleList[short(gmsg->port)].
		ReceiveDockingSuccessfulMsg((short)gmsg->dslot);
	      
	      // Using IsLocalPlayer(game->ship + gmsg->dslot) can crash
	      if ( gmsg->dslot == localSlotNumber &&
		   game->ship[localSlotNumber].videoMode != VIDEO_STATUS ) {
		retMsgBuffer.PushMsg((uchar)SF_MSG_KEYPRESSED_4,
				     (ushort)gmsg->dslot,
				     (uchar)frameNumber,
				     (uchar)SF_KEY_S,
				     (int)S_CLIENT_TCP);
	      }
	      
	    } else {
	      
	      logFile << 
		"ERROR> illegal ISDOCKED message received on frame " << 
		game->frame << 
		".  slot = " <<
		obj.generic->TypeName() << 
		" " << 
		game->TypeListIndex(obj.generic) << 
		" dslot = " <<
		tobj.generic->TypeName() << 
		" " << 
		game->TypeListIndex(tobj.generic) << 
		" port = " << 
		gmsg->port << 
		endl;
	      
	    }
	  
	}
	break;
						      
    case SF_MSG_DOCK : {

      if ((short)gmsg->dslot != -1)
	tobj.generic = game->objectList[short(gmsg->dslot)];
      else
	tobj.objLong = 0L;

      if ( !IsValidObjectPointer(tobj.generic,__FILE__,__LINE__) )
      {
	break;
      }

      if (!tobj.generic->IsActive()) {
	logFile << 
	  "ERROR> DOCK received inactive slot number (" << 
	  gmsg->dslot <<
	  ") on frame " << 
	  game->frame << 
	  endl;
	break;
      }

      if (IsValidObjectPointer(obj.generic) && 
	  obj.generic->IsActive() &&
          !obj.generic->IsExploding() &&
	  obj.designed->bus.moduleList.IsNotNull() &&
          gmsg->port >= 0 && 
          gmsg->port < obj.designed->bus.numMod &&
	  obj.designed->bus.moduleList[short(gmsg->port)].moduleFunc == 
          ModuleClass::dock ) 
	obj.designed->bus.moduleList[short(gmsg->port)].
            ReceiveDockActivationMsg(tobj.generic,gmsg->mp1,gmsg->mp2);
    }
    break;
    case SF_MSG_DEACTIVATEDOCK : {

      if ( IsValidObjectPointer(obj.generic) && 
	   obj.generic->IsActive() && 
	   !obj.generic->IsExploding() &&
	   obj.designed->bus.moduleList.IsNotNull() &&
	   gmsg->port >= 0 && 
	   gmsg->port < obj.designed->bus.numMod &&
	   obj.designed->bus.moduleList[short(gmsg->port)].moduleFunc == 
	   ModuleClass::dock )
	obj.designed->bus.moduleList[short(gmsg->port)].ReceiveDockDeactivationMsg();
    }
    break;
    case SF_MSG_CLOAK : {

      if ( IsValidObjectPointer(obj.ship) && 
	   obj.ship->bus.stealthMod &&
	   obj.ship->bus.stealthMod->check == 0xAAAAAAAA )
	{
	  for (i=0; i < obj.ship->bus.numTubes; i++)
	    {
	      obj.ship->bus.tubeMod[i]->deactivate();
	    }
	  obj.ship->bus.stealthMod->ReceiveHideMsg(gmsg->mp1, gmsg->mp2);
	}
    }
      break;

      case SF_MSG_UNCLOAK :
	{
	  
	  if ( IsValidObjectPointer(obj.ship) && 
	       obj.ship->bus.stealthMod &&
	       obj.ship->bus.stealthMod->check == 0xAAAAAAAA )
	    obj.ship->bus.stealthMod->ReceiveUnHideMsg();
	  
	}
	break;
	
      case SF_MSG_REMOVECOLONY: {
	if ( IsValidObjectPointer(obj.colony) )
	  {
	    game->ReceiveDestroyObjectMsg(obj.colony);
	  }
      }
      break;
      
      case SF_MSG_SPAWNCOLONY:
	{
	  
	  game->colonyPending = 0;
	  
	  numSpawnMessagesReceived++;
	  
	  logFile << "received> spawn_colony msg (" << 
	    numSpawnMessagesReceived << " received) on frame " <<
	    game->frame << ": " <<
	    gmsg->slot << " " <<
	    gmsg->dslot << " " <<
	    gmsg->bslot << " " <<
	    gmsg->race << " " <<
	    gmsg->pop1 << " " <<
	    gmsg->pop2 <<  " " << endl;
	  
	  if ((short)gmsg->dslot != -1)
	    tobj.generic = game->objectList[short(gmsg->dslot)];
	  else
	    tobj.objLong = 0L;
	  
	  if ( !IsValidObjectPointer(tobj.generic) )
	    {
	      break;
	    }
	  
	  if ( !IsValidObjectPointer(obj.generic) )
	    {
	      break;
	    }
	  
	  if ( !tobj.generic->IsActive() )
	    {
	      logFile << 
		"ERROR> SPAWNCOLONY received inactive slot number (" << 
		gmsg->dslot <<
		") on frame " << 
		game->frame << 
		endl;
	      break;
	    }
	  
	  tobj.colony->pending = 0;
	  
	  PopStruct pop((short)gmsg->pop1, gmsg->pop2);
	  
	  switch ( obj.generic->Type() ) {
	    
	  case ship: {
	    
	    /*******************************/
	    /*    spawn-to-dock message    */
	    /*******************************/
	    
	    /************************************************************/
	    /*                                                          */
	    /* a spawn-to-dock message consists of the following:       */
	    /* o  object index of ship with dock                        */
	    /* o  object index of colony that spawned                   */
	    /* o  race of spawned colony                                */
	    /* o  population of colony (as two longs)                   */
	    /*                                                          */
	    /************************************************************/
	    
	    /* a spawn-to-dock message  */
	    
	    if (IsDisconnectedClient() || IsServer()) {
	      /***************************************************************/
	      /* if this is the server (or single-player client) then bypass */
	      /* this message, since it has already been processed           */
	      /***************************************************************/
	      numSpawnMessagesDiscarded++;
	      logFile << "discarded> spawn_colony msg (" << 
		numSpawnMessagesDiscarded << " discarded) on frame " <<
		game->frame << ": " <<
		gmsg->slot << " " <<
		gmsg->dslot << " " <<
		gmsg->bslot << " " <<
		gmsg->race << " " <<
		gmsg->pop1 << " " <<
		gmsg->pop2 <<  " " << endl;
	      break;
	    }
	    
	    //
	    // the spawn event has already occurred on the server 
	    // (that originated the msg)
	    //
	    j = tobj.colony->Spawn( pop,
				    (short)gmsg->race,
				    (short)gmsg->slot,
				    short(gmsg->bslot) );
	    
	    if (j != -1) {
	      logFile << "received> a ";
	      tobj.colony->ShortProfile();
	      logFile << " has spawned a " << game->race[gmsg->race].name <<
		" colony with " << pop.print() << " " <<
		game->race[gmsg->race].name << "s to a ";
	      obj.ship->ShortProfile();
	      logFile << " on frame " << game->frame << endl;
	    }
	    
	    break;
	    
	  }  
	  case colony: {
	    /*******************************/
	    /*  colony consolidation msg   */
	    /*             or              */
	    /*   standard spawn message    */
	    /*             or              */
	    /* colony-changes-race message */
	    /*******************************/
	    
	    /*************************************************************/
	    /*                                                           */
	    /* a spawn colony message consists of the following:         */
	    /* o  object index of colony that spawned                    */
	    /* o  object index of colony that spawned                    */
	    /* o  race of spawned colony                                 */
	    /* o  population of colony (as two longs)                    */
	    /*                                                           */
	    /* comment: if slot and dslot are equal, then the colony is  */
	    /*          spawning and new colony                          */
	    /*                                                           */
	    /* a colony-consolidation message consists of the following: */
	    /* o  object index of colony that is removed (obj)           */
	    /* o  object index of colony that remains (tobj)             */
	    /* o  race of colony                                         */
	    /* o  population of colony (as two longs)                    */
	    /*                                                           */
	    /* comment: if one of the colonies is attached to a planet   */
	    /*          then the resulting colony should be attached to  */
	    /*          the same planet                                  */
	    /*                                                           */
	    /* a colony-changes-race message consists of the following:  */
	    /* o  object index of lastAttacker (for credit)              */
	    /* o  object index of colony                                 */
	    /* o  new race of colony                                     */
	    /* o  zero population                                        */
	    /*                                                           */
	    /*************************************************************/
	    
	    if (!pop.lBillions  && !pop.billions) {
	      /* colony-changes-race message */
	      
	      tobj.colony->pending = 0;
	      
	      logFile << "received> a ";
	      obj.colony->ShortProfile();
	      logFile << " has forced a ";
	      tobj.colony->ShortProfile();
	      logFile << " to surrender on frame" << game->frame << endl;
	      
	      if ( IsValidObjectPointer(tobj.colony->lockObj.generic) ) {
		sprintf(msg, "A %s colony on %s has surrendered to the %ss.", 
			game->race[tobj.colony->raceID].name,
			tobj.colony->lockObj.planet->name,
			game->race[gmsg->race].name);
		/* claim planet */
		tobj.colony->lockObj.generic->raceID = gmsg->race;
		
	      } else
		sprintf(msg, "A %s colony has surrendered to the %ss.", 
			game->race[tobj.colony->raceID].name,
			game->race[gmsg->race].name);
	      
	      /* notify old race */
	      game->PostGameMessage( msg,
				     (short)tobj.colony->raceID,
				     (short)tobj.colony->raceID,
				     broadcast );
	      
	      /* change alliance */
	      tobj.colony->raceID = gmsg->race;
	      
	      /* if this object was building something convert it */
	      tobj.colony->ConvertConstructionProject();
	      
	      /* notify new race */
	      game->PostGameMessage( msg,
				     (short)gmsg->race,
				     (short)gmsg->race,
				     broadcast );
	      
	    } else if (obj.objLong == tobj.objLong) {
	      /* standard spawn message: */
	      
	      if ( !IsConnectedClient() )
		{
		  //
		  // if this is the server (or sending client) then bypass 
		  // this message, since it has already been processed
		  //
		  numSpawnMessagesDiscarded++;
		  logFile << "discarded> spawn_colony msg (" << 
		    numSpawnMessagesDiscarded << " discarded) on frame " <<
		    game->frame << ": " <<
		    gmsg->slot << " " <<
		    gmsg->dslot << " " <<
		    gmsg->bslot << " " <<
		    gmsg->race << " " <<
		    gmsg->pop1 << " " <<
		    gmsg->pop2 <<  " " << endl;
		  break;
		}
	      
	      tobj.colony->Spawn(pop,
				 (short)gmsg->race,
				 -1,
				 short(gmsg->bslot));
	      
	      logFile << "received> a ";
	      tobj.colony->ShortProfile();
	      logFile << " has spawned a new colony  with " << pop.print() <<
		" " << game->race[gmsg->race].name << "s on frame " <<
		game->frame << endl;
	      
	    } else {
	      /* colony consolidation msg */
	      
	      short conquered = 0;
	      
	      obj.colony->pending = 0;
	      
	      tobj.colony->pop = pop;
	      
	      if ( IsValidObjectPointer(obj.colony->lockObj.generic) ) {
		//
		// the obj colony failed to defend its planet -- it is
		// destroyed and the tobj colony takes it place
		// give the planet ecology a boost to make the new population 
		// more loyal
		//
		obj.colony->lockObj.planet->terraForm = 
		  (obj.colony->lockObj.planet->terraForm) +
		  (terraform_scale - obj.colony->lockObj.planet->terraForm) / 
		  5;
		
		tobj.generic->now = obj.generic->now;
		
		tobj.colony->targetPP.obj = obj;
		obj.colony->targetPP.obj = tobj;
		tobj.colony->
		  SendDistressSignal((short)tobj.colony->pop.InUnitsOf(1000000));
		obj.colony->
		  SendDistressSignal((short)obj.colony->pop.InUnitsOf(1000000));
		
		tobj.colony->
		  PlaySoundOnScreen(game->soundEvent[conquer_colony_event]);
		
		/* spawn a number of powerups as a reward */
		tobj.colony->SpawnRandomModules(num_reward_plugins);
		
		tobj.colony->lockObj = obj.colony->lockObj;
		tobj.colony->lockObj.planet->colony = tobj.colony;
		tobj.colony->parent = obj.colony->parent;
		tobj.colony->lockObj.planet->raceID = gmsg->race;
		
		//
		// transfer construction project
		// if you want to allow an enemy colony to capture a 
		// construction project from an existing colony then 
		// uncomment the line below
		//
		//obj.colony->TransferConstructionProjectTo(tobj.colony);

		/* check to see if a player can take credit for this event */
		if (tobj.colony->lastDockObjIndex != -1) {
		  ObjectUnion dObj;
		  dObj = game->ObjectListObj(tobj.colony->lastDockObjIndex);
		  
		  slong bonus = tobj.colony->pop.log10() / 2 +
		    tobj.colony->lockObj.planet->
		    ExploitationPotential() / 100 * game->bonusFactor;
		  
		  slong cBonus = tobj.colony->lockObj.planet->
		    ExploitationPotential() *
		    game->landRightFactor;
		  
		  game->player[dObj.ship->playerIndex].
		    AddToStat(game->player[dObj.ship->playerIndex].refValue,
			      cBonus);
		  
		  sprintf(temp,
			  "1st Officer> we have received a bonus credit of %s",
			  Format(cBonus));
		  
		  game->PostGameMessage(temp, dObj.ship - game->ship.rootOf(),
					(short)gmsg->race, personal);
		  
		  game->player[dObj.ship->playerIndex].
		    AddToStat(game->player[dObj.ship->playerIndex].activeScore,
			      (ulong)bonus);
		  game->player[dObj.ship->playerIndex].
		    AddToStat(game->player[dObj.ship->playerIndex].totalScore,
			      (ulong)bonus);
		  game->player[dObj.ship->playerIndex].
		    AddToStat(game->player[dObj.ship->playerIndex].strategicPoints,
			      (ulong)bonus);
		  game->player[dObj.ship->playerIndex].
		    AddToStat(game->player[dObj.ship->playerIndex].numKills,
			      (ulong)game->bonusFactor);
		  game->player[dObj.ship->playerIndex].
		    AddToStat(game->player[dObj.ship->playerIndex].numKillsTotal,
			      (ulong)game->bonusFactor);
		  
		}
		
		logFile << "received> a ";
		tobj.colony->ShortProfile();
		logFile << " has conquered a ";
		obj.ship->ShortProfile();
		logFile << " on frame " << game->frame << endl;
		
		conquered = 1;
		
	      } else if ( IsValidObjectPointer(tobj.colony->lockObj.generic)) {
		
		if ( obj.generic->raceID  == tobj.generic->raceID) {
		  //
		  // two friendly colonies are joining
		  // transfer construction project
		  //
		  obj.colony->TransferConstructionProjectTo(tobj.colony);
		  
		  logFile << "received> a ";
		  tobj.colony->ShortProfile();
		  logFile << " has absorbed a ";
		  obj.ship->ShortProfile();
		  logFile << " on frame " << game->frame << endl;
		  
		  obj.colony->
		    PlaySoundOnScreen(game->soundEvent[supplement_colony_event]);
		  
		} else {
		  /* the tobj colony successfully defended its planet */
		  tobj.colony->targetPP.obj = obj;
		  obj.colony->targetPP.obj = tobj;
		  tobj.colony->
		    SendDistressSignal((short)tobj.colony->pop.InUnitsOf(1000000));
		  obj.colony->
		    SendDistressSignal((short)obj.colony->pop.InUnitsOf(1000000));
		  conquered = 1;
		  
		  obj.colony->PlaySoundOnScreen(game->soundEvent[fail_to_conquer_event]);
		  
		  logFile << "received> a ";
		  tobj.colony->ShortProfile();
		  logFile << " has destroyed a ";
		  obj.ship->ShortProfile();
		  logFile << " on frame " << game->frame << endl;
		  
		}
	      }
	      
	      tobj.colony->raceID = gmsg->race;
	      
	      tobj.colony->planetIndex =
		game->TypeListIndex(tobj.colony->lockObj.planet);
	      
	      if (tobj.colony->planetIndex == -1) {
		
		logFile << 
		  "ERROR> colony consolidation could not occur for a ";
		tobj.colony->ShortProfile();
		logFile << " on an undefined planet (slot ";
		j = game->ObjectListIndex(tobj.generic);
		if (j != -1) {
		  logFile << j;
		  if (tobj.generic->IsActive())
		    logFile << 
		      " = " << 
		      tobj.generic->TypeName() << 
		      " " << game->TypeListIndex(tobj.generic);
		  else
		    logFile << " = inactive";
		} else
		  logFile << "unknown";
		logFile << ") on frame " <<
		  game->frame << endl;
		
		break;
		
	      }
	      
	      if (tobj.colony->planetIndex == 0)
		game->ReceiveDestroyObjectMsg(tobj.colony);
	      else
		sprintf(msg, "A colony has arrived at %s",
			game->planet[tobj.colony->planetIndex].name);
	      
	      if (tobj.colony->planetIndex ==
		  game->race[gmsg->race].targetIndex &&
		  !conquered) {
		/* reset the colonization marker if the planet is conquered */
		/* or colonized for the first time                          */
		game->race[gmsg->race].targetIndex = 0;
		game->race[gmsg->race].targetIndexRank = 0;
	      }
	      
	      game->PostGameMessage(msg, (short)gmsg->race, (short)gmsg->race, broadcast);
	      
	      game->ReceiveDestroyObjectMsg(obj.colony);
	    }
	    
	    break;
	    
	  }
	  case planet: {
	    
	    /***************************************************************/
	    /* comment: this message is used for an uncontested landing.   */
	    /*          if the planet is already occupied then the server  */
	    /*          should send a colony-consolidation msg instead     */
	    /*                                                             */
	    /* o  object index of colony that landed                       */
	    /* o  object index of planet                                   */
	    /* o  race of colony                                           */
	    /* o  population of colony (as two longs)                      */
	    /***************************************************************/
	    
	    tobj.colony->pop = pop;
	    
	    tobj.colony->raceID = gmsg->race;
	    
	    logFile << "received> a ";
	    tobj.colony->ShortProfile();
	    logFile << " has landed on " <<
	      obj.planet->name << " on frame " << game->frame << endl;
	    
	    tobj.colony->lockObj = obj;
	    tobj.colony->parent = obj;
	    tobj.colony->planetIndex = game->TypeListIndex(obj.planet);
	    obj.planet->raceID = gmsg->race;
	    obj.planet->colony = tobj.colony;
	    
	    tobj.colony->PlaySoundOnScreen(game->soundEvent[colonize_event]);
	    
	    break;
	    
	  }
	  default:
	    break;
	  }
	  
	}
	break;
	
      case SF_MSG_SETSHIPINDEX :
	{
	  
	  if (gmsg->dslot != ushort(-1))
	    tobj.generic = game->objectList[(short)gmsg->dslot];
	  else
	    tobj.generic = 0L;
	  
	  if ( !IsValidObjectPointer(tobj.generic) )
	    {
	      return;
	    }
	  
	  game->RefitShip( (short)gmsg->slot,
			   (short)gmsg->index,
			   (short)gmsg->flag,
			   (short)gmsg->port,
			   tobj.ship );
	  
	}
	break;
	
      case SF_MSG_GETPOWERUP :
	{
	  
	  if ( !IsValidObjectPointer(obj.ship) )
	    {
	      return;
	    }
	  
	  //	  
	  // if this is the server or client that sent the messages then it has
	  // already been processed.
	  //
	  if ( IsDisconnectedClient() ||
	       IsConnectedClient() && 
	       IsLocalPlayer(obj.ship) ||
	       IsServer() && 
	       game->IsRobotPlayer(obj.ship) )
	    break;
	  
	  obj.ship->ReceiveGetPowerUpMsg( (short)gmsg->bslot,
					  gmsg->id,
					  gmsg->mp1 );
	  
	}
	break;

      case SF_MSG_COMPLETEBUILD :
	{
	  
	  if ((short)gmsg->dslot != -1)
	    tobj.generic = game->objectList[short(gmsg->dslot)];
	  else
	    tobj.objLong = 0L;
	  
	  if ( IsValidObjectPointer(tobj.generic,__FILE__,__LINE__) )
	    {
	      if ( tobj.generic->IsActive() && 
		   tobj.designed->bus.factoryMod &&
		   tobj.designed->bus.factoryMod->check == 0xAAAAAAAA )
		tobj.designed->bus.factoryMod->ReceiveCompleteBuildMsg(obj.ship);
	      else
		logFile << "ERROR> COMPLETEBUILD received inactive slot (" <<
		  gmsg->dslot << ") on frame " << game->frame << endl;
	    }
	  
	}
	break;
	
      case SF_MSG_FILLSLOT :
	{
	  if ( IsServer() || IsDisconnectedClient() ) {
	    //
	    // this event has already been pre-processed on the server so 
	    // don't run but first reset the player pending flag
	    //
	    game->player[(short)gmsg->index].ClearPendingFlag();
	    break;
	  }
	  
	  if ((short)gmsg->dslot != -1)
	    tobj.generic = game->objectList[short(gmsg->dslot)];
	  else
	    tobj.objLong = 0L;
	  
	  if ( !tobj.generic || (tobj.generic && !tobj.generic->IsActive()) ) {
	    logFile << "ERROR> FILLSLOT received inactive slot (" <<
	      gmsg->dslot << ") on frame " << game->frame << endl;
	  } else
	    game->FillSlot((short)gmsg->slot, 
			   (short)gmsg->index, 
			   (short)gmsg->race,
			   (short)gmsg->flag,
			   tobj.ship, 
			   (short)gmsg->port);
	  
	  break;
	}
	
      case SF_MSG_FIRETUBES :
	{
	  if (i < 0 || i > game->colonyObjectIndexEnd)
	    break;
	  
	  tShip.ship->Command(BusStruct::no_message, nullVector, nullVector);
	  
	  if ( IsServer() || (!IsServer() && !IsLocalPlayer(tShip.ship)) )
	    tShip.ship->heading = (uchar) gmsg->angle;
	  
	  tShip.ship->gfresh = TRUE;
	  tShip.ship->gframe = (short)gmsg->frame;
	  tShip.ship->tubeSelector |= (short)gmsg->mask;
	  
	  /* ************************************************************* */
	  /* *** numFrames is how many frames we are ahead of the data *** */
	  /* ************************************************************* */
	  slong numFrames = (uchar) ((frameNumber % 256) - tShip.ship->gframe);
	  if ( numFrames > 127 )
	    numFrames -= 256;
	  
	  /* ****************************************************** */
	  /* *** Little patch to prevent severe lag corrections *** */
	  /* ****************************************************** */
	  if ( numFrames > 2*frameRate ) numFrames = frameRate;
	  if ( numFrames < 0 ) numFrames = 0;
	  /* ****************************************************** */
	  
	  //
	  // If a ship has a nonzero lag and it is less than numFrames, 
	  // the use it
	  //
	  int lag = GetShipLag(i);
	  if ( lag > 0 && lag < numFrames )
	    {
	      numFrames = lag;
	    }
	  
	  static CoordinateClass nhat;
	  nhat = tShip.ship->bitmap.Dir(tShip.ship->heading);
	  
	  tShip.ship->gvel = tShip.ship->tvel;
	  tShip.ship->gnow = tShip.ship->now;
	  
	  tShip.ship->tvel.x = gmsg->vx;
	  tShip.ship->tvel.y = gmsg->vy;
	  
	  tShip.ship->numLagFrames = numFrames;

	  if ( !tShip.ship->IsAttachedToSB() )
	    {
	      tShip.ship->bus.FireSelectedWeapons(&game->ship[i]);
	    }
	  
	  tShip.ship->tvel = tShip.ship->gvel;
	  tShip.ship->now = tShip.ship->gnow;
	  
	  tShip.ship->gvel.x = gmsg->vx;
	  tShip.ship->gvel.y = gmsg->vy;
	  tShip.ship->gnow.x = gmsg->x;
	  tShip.ship->gnow.y = gmsg->y;
	  
	  break;
	}
	
      case SF_MSG_AIOBJECTTRACK_17 :
	{
	  if ( i >= 0 && i <= game->colonyObjectIndexEnd )
	    if ( IsValidObjectPointer(game->objectList[i]) )
	      {
		game->objectList[i]->ReceiveObjectTrackingMsg(gmsg);
	      }
	}
	break;
	
      case SF_MSG_OBJECTTRACK_17 :
	{
	  if ( i >= 0 && i <= game->colonyObjectIndexEnd )
	    if ( IsValidObjectPointer(game->objectList[i]) )
	      {
		game->objectList[i]->GenericClass::
		  ReceiveObjectTrackingMsg(gmsg);
	      }
	}
	break;
	
      case SF_MSG_SETPOPULATION :
	{
	  if ( IsValidObjectPointer(obj.ship) )
	    {
	      obj.ship->ReceiveConfirmationOfPopulation(short(gmsg->pop1),
							slong(gmsg->pop2),
							uchar(gmsg->ai1),
							uchar(gmsg->ai2),
							uchar(gmsg->ai3),
							uchar(gmsg->ai4),
							uchar(gmsg->ai5),
							uchar(gmsg->ai6),
							gmsg->x,
							gmsg->y);
	      
	    }
	}
	break;
	
      case SF_MSG_REQUESTPOPULATION : {
	
	if ( !IsValidObjectPointer(obj.generic) )
	  {
	    return;
	  }
	
	int type = obj.generic->Type();
	switch ( type )
	  {
	    
	  case ship:
	    if ( IsLocalPlayer(obj.ship) || IsServer() && 
		 game->IsRobotPlayer(obj.ship) )
	      {
		obj.ship->SendConfirmationOfPopulation();
	      }
	    break;
	    
	  case colony:
	    if ( IsServer() )
	      {
		obj.colony->SendConfirmationOfPopulation();
	      }
	    break;
	    
	  default:
	    logFile << 
	      "ERROR> REQUESTPOPULATION rcvd pop request for illegal type " << 
	      type << 
	      " on frame " << 
	      game->frame << 
	      endl;
	    
	  }
	
      }
      break;
      
      default:
	if (i < game->FirstPlanetObjectIndex())
	  tShip.ship->Command(BusStruct::no_message);
	break;
      }
    }

  if (currentPlayer && warningMessagePosted &&
      game->frame == warningMessagePostedFrame + 100) {
    *temp = '\0';
    SetScreenMessage(temp, 0);
    warningMessagePosted = 0;
  }
  if (currentPlayer && warningBottomMessagePosted &&
      game->frame == warningBottomMessagePostedFrame + 100) {
    *temp = '\0';
    SetScreenBottomMessage(temp, 0);
    warningBottomMessagePosted = 0;
  }

}


void EngineSetGameState(char *buffer ) {
  inServer = 0;
  logFile.close();
  strcpy(instanceName, "client");
  GameStateBuffer gb;
  gb.InitializeWith(buffer);
  char *logFileName = clientLogFilename;
  logFile.open(logFileName);
  gb.Pop(*game);
}

void EngineDestroyObjectList( void ) {
}

char* GetDiagnosticString( slong i ) {
  return 0;
}

slong EngineQueryNumObjects(slong& ns, slong& np, slong& nm) {

  ns = game->N(GameStateClass::ships);
  np = game->N(GameStateClass::planets);
  nm = game->N(GameStateClass::missiles);
  return game->NumObjects();
}

#define MAXMSGCHAR 129
//  char playerMessageBuffer[max_players][MAXMSGCHAR];
//  int playerMessageCharIndex[max_players];
//  void ScanInterfaceFile() {

//    short j,k;
//    static char msg[80];
//    static char line[80];
//    char cmdStr[16];
//    char valStr[16];
//    char* pStr;
//    ifstream cmdFile("SFcmd.dat");

//    while(cmdFile.getline(line, 80)) {
    
//      pStr = strchr(line, '=');
//      if (pStr) {
//        strncpy(cmdStr, playerMessageBuffer[i], pStr-playerMessageBuffer[i]);
//        strncpy(valStr, pStr+1, strlen(pStr+1));
//        /* null-terminate the strings */
//        *(cmdStr + (pStr - playerMessageBuffer[i])) = '\0';
//        *(valStr + strlen(pStr)-1) = '\0';
//      } else {
//        *cmdStr = '\0';
//        *valStr = '\0';
//      }
    
//      /* check for mode settings */
//      if (strcmp(cmdStr, "mode") == 0) {
      
//        if (strcmp(valStr,"dev") == 0) {
//  	game->player[tShip.ship->playerIndex].SetDeveloperMode();
//  	strcpy(msg, "you are now in developer mode");
//  	PushMessageNew((uchar)i,(uchar)i, msg);
//        } else if (strcmp(valStr,"play") == 0) {
//  	game->player[tShip.ship->playerIndex].SetPlayerMode();
//  	strcpy(msg, "you are now in player mode");
//  	PushMessageNew((uchar)i,(uchar)i, msg);
//        }
      
//      } else if (strcmp(cmdStr, "kills") == 0) {
//  	if (game->player[tShip.ship->playerIndex].IsDeveloper()) {
	  
//  	  /* set number of kills for this player to given number */
//  	  game->player[tShip.ship->playerIndex].numKills = 
//  	    game->player[tShip.ship->playerIndex].numKillsTotal =
//  	    (short)atol(valStr);
//  	}
//      } else if (strcmp(cmdStr,"glitch") == 0) {
//        if (game->player[tShip.ship->playerIndex].IsDeveloper()) {
//  	k = (short)atol(valStr);
//  	for (j=0; j<k; j++) {
//  	  i = game->Random(game->NumObjects(), j);
//  	  game->objectList[i]->now.x += long(instanceName[0]);
//  	}
//        }
//      } else if (strcmp(cmdStr, "cmd") == 0) {
//        if (game->player[tShip.ship->playerIndex].IsDeveloper()) {
//  	if (strcmp(valStr,"randomize") == 0) {
//  	  game->RandomizeGameState(0);
//  	} else if (strcmp(valStr,"fill") == 0) {
//  	  for (j = 0; j < game->numPlanets; j++) {
//  	    if (game->planet[j].colony)
//  	      game->planet[j].colony->pop =
//  		game->planet[j].
//  		  OptimalPopulation(game->
//  				    race[game->planet[j].colony->raceID]);
//  	  }
//  	} else if (strcmp(valStr,"clear") == 0) {
//  	  for (j = game->maxHumanShips; j < game->numShips; j++) {
//  	    game->ship[j].Explode("a clear command was issued");
//  	  }
//  	  for (j = 0; j < game->numColonies; j++) {
//  	    game->colonyList[j]->Explode("a clear command was issued");
//  	  }
//  	  for (j = 0; j < game->numMissiles; j++) {
//  	    game->missileList[j]->Explode("a clear command was issued");
//  	  }
//  	}
//        }
//      }
//  }

//    return;
//  }

char message[MESSAGE_BUFFER_SIZE][MESSAGE_STRING_LENGTH+1];
int  messageColor[MESSAGE_BUFFER_SIZE];
char messagePushed = FALSE;
char fromName[80];
char tempMsg[MAXMSGCHAR];

void
PushMessageNew( uchar from,uchar to,char *tmsg )
{
  int i,lenMsg;
  int maxMsgCharsToUse ;
  int color;
  int numMsgCharsToUse;
  int showName;
  char msg[4096];
  
  if ( !tmsg )
    {
      return;
    }
  
  strcpy(msg,tmsg);
  
  color = 0;
  switch (from) {
  case FROM_GOD   : strcpy(fromName,"All"); color = 0; break;
  case FROM_RACE0 : strcpy(fromName,"Dem"); color = 0; break;
  case FROM_RACE1 : strcpy(fromName,"Tor"); color = 1; break;
  case FROM_RACE2 : strcpy(fromName,"Ter"); color = 2; break;
  case FROM_RACE3 : strcpy(fromName,"Dre"); color = 3; break;
  default :
    fromName[0] = 0;
    if ( from < game->numShips &&
	 IsValidObjectPointer(game->objectList[from]) )
      {
	color = game->objectList[from]->raceID;
	strcpy(fromName,game->player[game->ship[from].playerIndex].name);
      }
    fromName[12] = 0;
    break;
  }
  
  if ( !IsValidObjectPointer(game->objectList[localSlotNumber]) )
    {
      return;
    }
  
  int raceID = game->objectList[localSlotNumber]->raceID;
  
  switch (to)
    {
    case TO_BROADCAST :
      break;
    case TO_RACE0 :
      if ( raceID != 0 )
	{
	  return;
	}
      break;
    case TO_RACE1 :
      if ( raceID != 1 )
	{
	  return;
	}
      break;
    case TO_RACE2 :
      if ( raceID != 2 )
	{
	  return;
	}
      break;
    case TO_RACE3 :
      if ( raceID != 3 )
	{
	  return;
	}
      break;
    default :
      if ( to != localSlotNumber )
	{
	  return;
	}
      break;
    }
  
  sprintf(tempMsg,"%s> %s",fromName,msg);
  SetScreenBottomMessage(tempMsg,color);
  warningBottomMessagePosted = 1;
  warningBottomMessagePostedFrame = game->frame;
  
  showName = TRUE;
  do
    {
      strcpy(tempMsg,msg);
      lenMsg = strlen(msg);
      
      if ( showName )
	maxMsgCharsToUse = MESSAGE_STRING_LENGTH-strlen(fromName)-2;      
      else
	maxMsgCharsToUse = MESSAGE_STRING_LENGTH;
      
      numMsgCharsToUse = maxMsgCharsToUse;
      if ( lenMsg > numMsgCharsToUse ) {
	
	while ( numMsgCharsToUse>0 && tempMsg[numMsgCharsToUse--] != ' ' );
	numMsgCharsToUse++;
	if ( numMsgCharsToUse <= 1 )
	  numMsgCharsToUse = maxMsgCharsToUse;
	
	tempMsg[numMsgCharsToUse] = 0;
	for ( i=numMsgCharsToUse; i<lenMsg; i++ )
	  msg[i-numMsgCharsToUse] = msg[i];
	msg[i-numMsgCharsToUse] = 0;
      }
      
      for ( i=MESSAGE_BUFFER_SIZE-1; i>=1; i-- )
	{
	  sprintf(message[i],"%s",message[i-1]);
	  messageColor[i] = messageColor[i-1];
	}
      
      if ( showName )
	{
	  sprintf(message[0],"%s> %s",fromName,tempMsg);
	  showName = FALSE;
	}
      else
	{
	  sprintf(message[0],"%s",tempMsg);
	}
      messageColor[0] = color;
      messagePushed = TRUE;
    } while( lenMsg > maxMsgCharsToUse );

}



/* ************************************** */
/* ***** Sample Function to call APIs *** */
/* ************************************** */

int 
SetupConsole() {
   static char name[80], temp[80], temp2[80], temp3[80], temp4[80];
   int i,j,k,n,dx;
   short index, tubeNum;
   ulong hours, minutes, seconds;
   ModuleClass* mod;

   SF_DisplayEngine *de = NULL;
   if ( sf->engine && sf->engine->check == 0xAAAAAAAA )
     de = sf->engine->displayEngine;

   if ( !de )
     {
      return 0;
     }

   if ( de->check != 0xAAAAAAAA )
     {
       return 0;
     }

   PlayerClass* pl = game->playerList[localSlotNumber];
   ShipClass* sh = game->ship + localSlotNumber;

   if ( !IsValidObjectPointer(sh) )
     {
       return 0;
     }

   ObjectUnion obj;
   if ( !console )
      return 0;

   if ( console->check != 0xAAAAAAAA )
     {
       return 0;
     }

   switch(console->mode) {
      case VIDEO_NAVIGATE :

	obj.planet = game->planet + short(sh->assignPar2);

	if ( !IsValidObjectPointer(obj.planet) )
	  {
	    break;
	  }

         de->button[16]->
         SetProperties("< Prev",SF_KEY_LT,0,0,0,0,SF_MSG_KEYPRESSED_4,0,0,0,0);
         de->button[17]->
          SetProperties("Go",SF_KEY_G,0,0,0,0,SF_MSG_KEYPRESSED_4,0,0,0,0);
         de->button[18]->
          SetProperties("Conquer",mark_button,0,0,0,0,SF_MSG_KEYPRESSED_4,0,0,0,0);
         de->button[19]->
          SetProperties("Orbit",SF_KEY_G,0,0,0,0,SF_MSG_KEYPRESSED_4,0,0,0,0);
         de->button[20]->
          SetProperties("",0,0,0,0,0,0,0,0,0,0);
         de->button[21]->
          SetProperties("Next >",SF_KEY_GT,0,0,0,0,SF_MSG_KEYPRESSED_4,0,0,0,0);

         dx = 2*console->cy/3 - console->cy/10;
         console->SetIcon( obj.planet->bitmap.index,
			   console->cx - dx ,console->cy - dx,
                           console->cx - console->cx/15,
                           console-> cy - console->cy/15 );

	 sprintf(temp, "destination: %s",
		 obj.planet->name);
	 console->SetLine(0,temp,
			  raceTextColor[obj.planet->raceID]);
	 sprintf(temp, "class: %s",
		 PlanetClassName[obj.planet->minor+1]);
	 console->SetLine(1,temp,RGB_GREEN);
	 sprintf(temp, "mass: %7.3g", obj.planet->mass /
		 1000.0);
	 console->SetLine(2,temp,RGB_GREEN,console->cx/2);

	 sprintf(temp, "temp: %d K", int(obj.planet->temp));
	 console->SetLine(3,temp,RGB_GREEN,console->cx/2);

	 sprintf(temp, "hydro: %d%%",
		 int(obj.planet->hydpc));
	 console->SetLine(4,temp,RGB_GREEN,console->cx/2);

	 sprintf(temp, "atm press: %7.2fa",
		 obj.planet->atmP/100.);
	 console->SetLine(5,temp,RGB_GREEN,console->cx/2);
	 
	 sprintf(temp, "surf grav: %7.2fg",
		 obj.planet->grav/100.);
	 console->SetLine(6,temp,RGB_GREEN,console->cx/2);

	 sprintf(temp, "optimal: %s",
		 obj.planet->
		 OptimalPopulation(game->race[sh->raceID]).print());
	 console->SetLine(7,temp,RGB_GREEN,
		   console->cx/2);

	 if (obj.planet->raceID != -1) {
	   if (obj.planet->Habitability(game->race[obj.planet->raceID],0) >= self_sustain_limit)
	     strcpy(temp2, "S");
	   else
	     strcpy(temp2, "B");
	   sprintf(temp, "habitability: %d%% (%d%%/%d%%)",
		   obj.planet->
		   Habitability(game->race[sh->raceID]),
		   obj.planet->
		   Habitability(game->race[sh->raceID],0),
		   obj.planet->
		   Habitability(game->race[obj.planet->raceID],0));
	   ConcatToFixedLength(temp, temp2, 33);
	 } else
	   sprintf(temp, "habitability: %d%% (%d%%)",
		   obj.planet->
		   Habitability(game->race[sh->raceID]),
		   obj.planet->
		   Habitability(game->race[sh->raceID],0));
	 console->SetLine(12, temp, RGB_GREEN);

	 if ( IsValidObjectPointer(obj.planet->colony) ) {

	   if (game->PlanetIsSpawnable((short)sh->assignPar2, game))
	     sprintf(temp, "population: %s*",
		     obj.planet->colony->pop.print());
	   else
	     sprintf(temp, "population: %s",
		     obj.planet->colony->pop.print());

	   console->SetLine(8,temp,RGB_GREEN,
			    5*console->cx/8);
	   sprintf(temp, "under %s control",
		   game->race[obj.planet->raceID].name);
	   console->SetLine(9,temp,
                           raceTextColor[obj.planet->raceID]);
	   sprintf(temp, "growth/min: %5.1f%%",
		   ((game->race[obj.planet->raceID].growthRate-100000) *
		    obj.planet->colony->Loyalty() /
		    1e5 * 60)); /* growth rate per minute */
	   console->SetLine(10,temp,RGB_GREEN);
	   sprintf(temp, "loyalty: %ld%%",
		   min(obj.planet->colony->Loyalty(), 100));

	   sprintf(temp2, "supply: %3d%%",
		   obj.planet->colony->supply * 100 / max_supply);
	   ConcatToFixedLength(temp, temp2, 32);
	   console->SetLine(11,temp,RGB_GREEN);

	 } else {

	   strcpy(temp, "population: uninhabited");
	   console->SetLine(8,temp,RGB_GREEN, console->cx/2);
	   strcpy(temp, "neutral");
	   console->SetLine(9,temp,RGB_GREEN, console->cx/2);
	   *temp = '\0';
	   console->SetLine(10, temp, RGB_GREEN);
	   console->SetLine(11, temp, RGB_GREEN);
	 }
	 break;

   case VIDEO_TARGET : {

     de->button[16]->
       SetProperties("<Prev",SF_KEY_LT,0,0,0,0,SF_MSG_KEYPRESSED_4,0,0,0,0);
     de->button[17]->
       SetProperties("Fight",SF_KEY_F,0,0,0,0,SF_MSG_KEYPRESSED_4,0,0,0,0);
     de->button[18]->
       SetProperties("Tractor",SF_KEY_Y,0,0,0,0,SF_MSG_KEYPRESSED_4,0,0,0,0);
     de->button[19]->
       SetProperties("Attack",SF_KEY_H,0,0,0,0,SF_MSG_KEYPRESSED_4,0,0,0,0);
     de->button[20]->
       SetProperties("Acquire",SF_KEY_A,0,0,0,0,SF_MSG_KEYPRESSED_4,0,0,0,0);
     de->button[21]->
       SetProperties("Next>",SF_KEY_GT,0,0,0,0,SF_MSG_KEYPRESSED_4,0,0,0,0);

     int iconIndex = -1;

     dx = console->cy/2;
     if ( IsValidObjectPointer(sh->targetPP.obj.generic) ) {
       
       ObjectUnion t = sh->targetPP.obj;

       if ( !IsValidObjectPointer(t.ship) )
	 {
	   break;
	 }

       PlayerClass* tPl = game->player + t.ship->playerIndex;
     
       ObjectType tt = t.generic->Type();

       if ( !(tt >= designed && t.ship->stealth > 1) )
	 iconIndex =
	   sh->targetPP.obj.generic->originalBitmapIndex;

       if (tt >= designed && t.ship->stealth > 1) {
	 /* clear console */
	 iconIndex = -1;
	 console->SetLine(0, "target not visible", RGB_GREEN);
	 /* clear the remaining lines */
	 *temp = '\0';
	 for (i = 1; i<=12; i++)
	   console->SetLine(i, temp, RGB_GREEN);
       } else if (tt >= designed) {
	 if (t.generic->IsFriendlyTo(sh)) {
	   sprintf(temp, "power rate: %ld %%",
		   t.designed->bus.powerRate / 10);
	   console->SetLine(2, temp, RGB_GREEN, console->cx/2);
	 } else {
	   *temp = '\0';
	   console->SetLine(2, temp,RGB_GREEN, console->cx/2);
	 }

	 if (t.designed->bus.numMod)
	   console->
	     SetLine(3,
		     t.designed->bus.moduleList.rootOf()->name,
		     RGB_GREEN,
		     t.designed->bus.moduleList.rootOf()->func,
		     StatusColor(t.designed->
				 bus.moduleList.rootOf()->func));
	 if (t.designed->bus.shieldMod && 
	     t.designed->bus.shieldMod->check == 0xAAAAAAAA )
	   console->
	     SetLine(4,
		     t.designed->bus.shieldMod->name,
		     RGB_GREEN,
		     t.designed->bus.shieldMod->XStorage(),
		     StatusColor(t.designed->bus.shieldMod->XStorage()));
	 if (t.designed->bus.impulseMod &&
	     t.designed->bus.impulseMod->check == 0xAAAAAAAA )
	   console->
	     SetLine(5,
		     t.designed->bus.impulseMod->name,
		     RGB_GREEN,
		     t.designed->bus.impulseMod->func,
		     StatusColor(t.designed->
				 bus.impulseMod->func));

	 if (tt == ship) {

	   sprintf(temp, "target: %s (%s)",
		   tPl->shipName,
		   game->race[t.ship->raceID].name);
	   console->SetLine(0, temp, raceTextColor[t.ship->raceID]);

	   sprintf(temp, "class: %s",
		   t.ship->design->name);
	   console->SetLine(1, temp,
			    raceTextColor[t.ship->design->raceID]);
	   
	   if (IsLocalPlayer(t.ship)) {
	     sprintf(temp, "credit: %s", Format(tPl->refValue));
	   } else if (t.ship->bus.armorMod &&
		      t.ship->bus.armorMod->check == 0xAAAAAAAA ) {
	     sprintf(temp, "armor: %s (%d%%)", Format(t.ship->bus.armorMod->fPar(0)),
		     t.colony->bus.armorMod->func * 100 / module_function_scale);
	   } else {
	     *temp = '\0';
	   }
	   console->SetLine(6,temp,RGB_GREEN,console->cx/2);

	   strcpy(temp2, t.ship->OptimalCrew().print());
	   sprintf(temp, "crew: %s/%s",
		   t.ship->pop.print(),
		   temp2 );
	   console->SetLine(7,temp,raceTextColor[t.ship->raceID],
			    console->cx/2);

	   sprintf(temp, "mass: %s", Format(t.designed->mass));
	   sprintf(temp2, "vol.: %d%%", t.designed->Volume() * 100 /
		   t.ship->design->volumeCap);

	   ConcatToFixedLength(temp, temp2, 32);

	   console->SetLine(8,temp,RGB_GREEN);

	   sprintf(temp, "commanded by %s %s",
		   game->rank[tPl->rank].name,
		   tPl->name);
	   console->SetLine(9,temp,raceTextColor[t.ship->raceID]);

	   sprintf(temp, "kills: %ld (%ld active)",
		   tPl->numKillsTotal,
		   tPl->numKills);
	   console->SetLine(10,temp,RGB_GREEN);

	   seconds = (ulong)(tPl->activeFrames / frameRate);
	   if (seconds >= 60) {
	     minutes = seconds / 60;
	     seconds = seconds % 60;

	     if (minutes > 60) {
	       hours = minutes / 60;
	       minutes = minutes % 60;
	     } else
	       hours = 0;

	   } else {

	     minutes = 0;
	     hours = 0;

	   }

	   sprintf(temp, "time: %ld:%02ld:%02ld (%d%% bonus)",
		   hours, minutes, seconds, game->BonusPctOf(game->player +
							     t.ship->playerIndex));
	   console->SetLine(11,temp,RGB_GREEN);

	   sprintf(temp, "status: %s", t.colony->Query(sh, 0));
	   console->SetLine(12,temp,RGB_GREEN);

	 } else if (tt == colony) {

	   sprintf(temp, "class: colony (%s)",
		   t.colony->design->name);
	   console->SetLine(1, temp, RGB_GREEN);

	   if (t.colony->bus.armorMod && 
	       t.colony->bus.armorMod->check == 0xAAAAAAAA ) {
	     sprintf(temp, "armor: %s (%d%%)", Format(t.colony->bus.armorMod->fPar(0)),
		     t.colony->bus.armorMod->func * 100 / module_function_scale);
	   } else
	     *temp = '\0';
	   console->SetLine(6,temp,RGB_GREEN);

	   sprintf(temp, "population: %s %ss",
		   t.ship->pop.print(),
		   game->race[t.ship->raceID].name);
	   console->SetLine(7,temp,RGB_GREEN);

	   if ( IsValidObjectPointer(t.colony->lockObj.generic) ) {
	     sprintf(temp, "location: %s",
		     game->
		     planet[game->
			   TypeListIndex(t.colony->lockObj.generic)].
		     name);
	     console->SetLine(8,temp,RGB_GREEN);

	   } else {
	     sprintf(temp, "supply: %3d%%", t.colony->supply);
	     console->SetLine(8,temp,RGB_GREEN);
	   }

	   sprintf(temp, "mass: %s", Format(t.designed->mass));
	   sprintf(temp2, "kills: %ld", t.colony->kills);

	   ConcatToFixedLength(temp, temp2, 32);

	   console->SetLine(9,temp,RGB_GREEN);

	   sprintf(temp, "race: %s",
		   game->race[t.ship->raceID].name);
	   console->SetLine(10,temp,raceTextColor[t.ship->raceID]);

	   sprintf(temp, "status: %s", t.colony->Query(sh, 0));
	   console->SetLine(12,temp,RGB_GREEN);

	   *temp = '\0';

	   console->SetLine(0, temp, RGB_GREEN);
	   console->SetLine(11, temp, RGB_GREEN);

	 } else if (tt == missile) {

	   sprintf(temp, "class: missile (%s)",
		   t.missile->design->name);
	   console->SetLine(1, temp,

			    raceTextColor[t.missile->design->raceID]);

	   sprintf(temp, "race: %s",
		   game->race[t.missile->raceID].name);
	   console->SetLine(10,temp,raceTextColor[t.missile->raceID]);

	   *temp = '\0';

	   console->SetLine(7, temp, RGB_GREEN);
	   console->SetLine(8, temp, RGB_GREEN);
	   console->SetLine(11, temp, RGB_GREEN);
	   console->SetLine(12, temp, RGB_GREEN);

	 }


       } else if (tt == planet) {
	 dx = 2*console->cy/3 - console->cy/10;

	 iconIndex = t.planet->bitmap.index;

	 sprintf(temp, "target: %s", t.planet->name);
	 console->SetLine(0,temp,RGB_GREEN);

	 sprintf(temp, "class: %s",
		 PlanetClassName[t.planet->minor+1]);
	 console->SetLine(1,temp,RGB_GREEN);

	 sprintf(temp, "mass: %7.3g", t.planet->mass /
		 1000.0);
	 console->SetLine(2,temp,RGB_GREEN,console->cx/2);

	 sprintf(temp, "temp: %d K", int(t.planet->temp));
	 console->SetLine(3,temp,RGB_GREEN,console->cx/2);

	 sprintf(temp, "hydro: %d%%", int(t.planet->hydpc));
	 console->SetLine(4,temp,RGB_GREEN,console->cx/2);

	 sprintf(temp, "optimal: %s",
		 t.planet->OptimalPopulation(game->race[sh->raceID]).
		 print());
	 console->SetLine(9,temp,RGB_GREEN);

	 if ( IsValidObjectPointer(t.planet->colony) ) {
	   sprintf(temp, "population: %s",
		   t.planet->colony->pop.print());
	   console->SetLine(10,temp,RGB_GREEN);
	   sprintf(temp, "under %s control",
		   game->race[t.planet->raceID].name);
	   console->SetLine(11,temp,raceTextColor[t.planet->raceID]);
	 } else {
	   strcpy(temp, "population: uninhabited");
	   console->SetLine(10,temp,RGB_GREEN);
	   strcpy(temp, "neutral");
	   console->SetLine(11,temp,RGB_GREEN);
	 }

	 sprintf(temp, "atm press: %7.2fa",
		 t.planet->atmP/100.);
	 console->SetLine(6,temp,RGB_GREEN);

	 sprintf(temp, "surf grav: %7.2fg",
		 t.planet->grav/100.);
	 console->SetLine(7,temp,RGB_GREEN);

	 sprintf(temp, "habitability: %d%%",
		 t.planet->
		 Habitability(game->race[sh->raceID]));
	 console->SetLine(12, temp, RGB_GREEN);

	 *temp = '\0';

	 console->SetLine(5, temp, RGB_GREEN);
	 console->SetLine(8, temp, RGB_GREEN);

       } else {

	 sprintf(temp, "class: %s",
		 t.generic->TypeName());

	 console->SetLine(1, temp, RGB_GREEN);

	 *temp = '\0';

	 console->SetLine(2, temp, RGB_GREEN);
	 console->SetLine(3, temp, RGB_GREEN);
	 console->SetLine(4, temp, RGB_GREEN);
	 console->SetLine(5, temp, RGB_GREEN);
	 console->SetLine(7, temp, RGB_GREEN);
	 console->SetLine(8, temp, RGB_GREEN);
	 console->SetLine(10, temp, RGB_GREEN);
	 console->SetLine(11, temp, RGB_GREEN);
	 console->SetLine(12, temp, RGB_GREEN);

       }
     } else {
       /* clear console */
       console->SetLine(0, "no target selected", RGB_GREEN);
       iconIndex = -1;

     /* clear the remaining lines */
       *temp = '\0';
       for (i = 1; i<=12; i++)
	 console->SetLine(i, temp, RGB_GREEN);
     }

     console->SetIcon( iconIndex,
		       console->cx - dx ,console->cy - dx,
		       console->cx - console->cx/15,
		       console-> cy - console->cy/15 );
   }
   break;


   case VIDEO_STATUS :
     {
       console->SetIcon();
       
       de->button[16]->
         SetProperties("< Up",SF_KEY_LT,0,0,0,0,SF_MSG_KEYPRESSED_4,0,0,0,0);
       de->button[17]->
	 SetProperties("",0,0,0,0,0,0,0,0,0,0);
       de->button[18]->
	 SetProperties("",0,0,0,0,0,0,0,0,0,0);
       de->button[19]->
         SetProperties("Down >",SF_KEY_GT,0,0,0,0,SF_MSG_KEYPRESSED_4,0,0,0,0);

	 if (!subMode) {

	   if ( !( IsValidObjectPointer(sh->dockObj) && 
		   sh->dockObj->bus.factoryMod)) {
	     console->SetLine(0,"     sys:level      damage    power",RGB_GREEN);
           de->button[17]->
             SetProperties("View",
                           SF_KEY_LSHIFT,SF_KEY_S,SF_KEY_LSHIFT,0,0,
                           SF_MSG_KEYPRESSED_4,SF_MSG_KEYPRESSED_4,SF_MSG_KEYRELEASED_4,0,0);
           de->button[18]->
             SetProperties("Drop",
                           SF_KEY_LSHIFT,SF_KEY_BACKSLASH,
                           SF_KEY_LSHIFT,0,0,
                           SF_MSG_KEYPRESSED_4,SF_MSG_KEYPRESSED_4,
                           SF_MSG_KEYRELEASED_4,0,0);
	       }
	   else {
           de->button[17]->
             SetProperties("Upgrade",SF_KEY_ENTER,SF_KEY_ENTER,0,0,0,
                           SF_MSG_KEYPRESSED_4,SF_MSG_KEYRELEASED_4,0,0,0);
           de->button[18]->
             SetProperties("Drop",
                           SF_KEY_LSHIFT,SF_KEY_BACKSLASH,
                           SF_KEY_LSHIFT,0,0,
                           SF_MSG_KEYPRESSED_4,SF_MSG_KEYPRESSED_4,
                           SF_MSG_KEYRELEASED_4,0,0);
	     strcpy(temp, "Select System / Press Enter to upgrade");
             console->SetLine(0,temp,RGB_GREEN);
	   }

	   tubeNum = 0;

	   for ( i=0; i<sh->bus.numMod; i++ ) {
	     
	     mod = sh->bus.moduleList + i;

	     if ( mod && mod->check == 0xAAAAAAAA )
	       {
		 
		 j = mod->GetMode();
		 
		 if (mod->moduleFunc != ModuleClass::tube &&
		     mod->moduleFunc != ModuleClass::ram)
		   strcpy(name, mod->name);
		 else
		   sprintf(name, "%s(%ld)",
			   mod->name,
			   mod->workPar[0]);
		 
		 if (j == ModuleClass::standby) 
		   sprintf(temp, " %s %s", mod->KeyOf(tubeNum), name);
		 else if (j == ModuleClass::locked)
		   sprintf(temp, " %s+%s", mod->KeyOf(tubeNum), name);
		 else
		   sprintf(temp, " %s-%s", mod->KeyOf(tubeNum), name);
		 
		 if (i == sh->bus.modPtr)
		   *temp = '>';
		 
		 if ( pl->IsRestrictedFromUsing(&sh->bus.moduleList[i]) ) {
		   char temp2[MESSAGE_STRING_LENGTH+1];
		   sprintf(temp2,"%-11s%27s",temp,"("SF_REGISTERED_CLUB" Only)");
		   console->SetLine(i+1,temp2,
				    raceTextColor[mod->raceID]);
		 } else {
		   console->SetLine(i+1,temp,
				    raceTextColor[mod->raceID],
				    mod->func,
				    StatusColor(mod->func),
				    mod->XStorage(),
				    StatusColor(mod->XStorage()));
		 }
	       }
	   }
	   
	   /* clear the remining lines */
	   *temp = '\0';

	   for ( i=sh->bus.numMod; i<sh->bus.numSlots; i++ )
	     console->SetLine(i+1,temp, RGB_GREEN);

	   } else {

	     console->SetLine(0, temp, RGB_GREEN);
             de->button[17]->
               SetProperties("Back",SF_KEY_S,0,0,0,0,
                             SF_MSG_KEYPRESSED_4,0,0,0,0);
             de->button[18]->
               SetProperties("",0,0,0,0,0,0,0,0,0,0);

	   mod = sh->bus.moduleList + sh->bus.modPtr;

	   if ( mod && mod->check == 0xAAAAAAAA )
	     {	   
	       
	       sprintf(temp, "system: %s", mod->name);
	       
	       console->SetLine(0, temp, raceTextColor[mod->raceID]);
	       
	       *temp2 = '\0';
	       *temp3 = '\0';
	       *temp4 = '\0';
	       
	       if (mod->moduleFunc == ModuleClass::power_plant) {
		 strcpy(temp, "class: power planet");
		 sprintf(temp2, "power req.: %s",Format(sh->bus.powerIn));
		 sprintf(temp3, "power out: %s",Format(sh->bus.powerOut));
	       } else if (mod->moduleFunc == ModuleClass::engine) {
		 strcpy(temp, "class: engine");
		 sprintf(temp2, "acceleration: %5.2f pix/s/s",
			 mod->par[0] / float(sh->mass) * frameRate * frameRate);
	       } else if (mod->moduleFunc == ModuleClass::qdrv) {
		 strcpy(temp, "class: Q drive");
		 sprintf(temp2, "velocity: %4.1f c",
			 mod->par[0] / float(game->lightSpeed));
		 sprintf(temp3, "mass limit: %s", Format(mod->par[1]));
	       } else if (mod->moduleFunc == ModuleClass::tube) {
		 strcpy(temp, "class: missile tube");
		 index = (short)mod->par[3];
		 sprintf(temp2, "warhead: %ld ", game->missileDesign[index].warhead);
		 sprintf(temp3, "capacity: %ld missiles", mod->par[2]);
		 sprintf(temp4, "load/burst/repeat: %ld/%ld/%4.2f/s", mod->workPar[0],
			 mod->TubeBurst(sh->bus.CalculateSurplusPower()), mod->TubeRepeatRate(frameRate));
	       } else if (mod->moduleFunc == ModuleClass::ram) {
		 strcpy(temp, "class: ram");
		 index = (short)mod->par[3];
		 sprintf(temp2, "warhead: %ld ", game->missileDesign[index].warhead);
		 sprintf(temp3, "capacity: %ld missiles", mod->par[2]);
		 sprintf(temp4, "load: %ld", mod->workPar[0]);
	       } else if (mod->moduleFunc == ModuleClass::beam) {
		 strcpy(temp, "class: beam weapon");
		 index = (short)mod->par[3];
		 sprintf(temp2, "damage/s: %ld ", 100 * mod->par[1] * frameRate / energy_to_damage);
		 sprintf(temp3, "range: %ld pix", mod->par[2]);
		 sprintf(temp4, "recharge time: %4.1f sec", mod->par[0] / double(frameRate));
	       } else if (mod->moduleFunc == ModuleClass::shield) {
		 strcpy(temp, "class: shield");
		 sprintf(temp2, "recharge time: %ld sec", mod->par[0] /
			 frameRate);
		 sprintf(temp3, "capacity: %ld", -mod->power*mod->par[0]);
	       } else if (mod->moduleFunc == ModuleClass::brake)
		 strcpy(temp, "class: brake");
	       else if (mod->moduleFunc == ModuleClass::sensor) {
		 strcpy(temp, "class: sensor");
	         
		 sprintf(temp2, "effective range: %ld pixels",
			 (((10 + mod->fPar(0)) * sh->mass) /
			  sh->sensorThreshold - 1) * sensor_dist / sub_scale);
	       } else if (mod->moduleFunc == ModuleClass::fuel_cell)
		 strcpy(temp, "class: fuel cell");
	       else if (mod->moduleFunc == ModuleClass::battery)
		 strcpy(temp, "class: battery");
	       else if (mod->moduleFunc == ModuleClass::counter_measures) {
		 strcpy(temp, "class: counter measures");
		 sprintf(temp2, "recharge time: %ld sec", mod->par[0] /
			 frameRate);
		 sprintf(temp3, "range: %ld pixels", mod->par[1]);
	       } else if (mod->moduleFunc == ModuleClass::cloak) {
		 strcpy(temp, "class: stealth");
		 sprintf(temp2, "discharge/recharge time: %ld sec", mod->par[0] /
			 frameRate);
	       } else if (mod->moduleFunc == ModuleClass::boost) {
		 strcpy(temp, "class: booster");
		 sprintf(temp2, "discharge/recharge time: %ld sec", mod->par[0] /
			 frameRate);
	       } else if (mod->moduleFunc == ModuleClass::dock) {
		 strcpy(temp, "class: dock");
		 sprintf(temp2, "discharge/recharge time: %ld sec", mod->par[0] /
			 frameRate);
		 sprintf(temp3, "range: %ld pixels", mod->par[2]);
		 if (sh->bus.impulseMod && 
		     sh->bus.impulseMod->check == 0xAAAAAAAA &&
		     sh->bus.impulseMod->par[0])
		   sprintf(temp4, "trac: %5.2f pix/s/s",
			   mod->par[1] / float(sh->mass) * frameRate * frameRate);
	       } else if (mod->moduleFunc == ModuleClass::factory)
		 strcpy(temp, "class: factory");
	       
	       console->SetLine(1, temp, RGB_GREEN);
	       
	       if (mod->moduleFunc != ModuleClass::tube)
		 sprintf(temp, "power: %s", Format(mod->power));
	       else {
		 if (mod->moduleFunc == ModuleClass::beam)
		   sprintf(temp, "power/shot: %s", Format(mod->par[1]/*TubeEnergyCost()*/));
		 else
		   sprintf(temp, "power/s: %s", Format(mod->power*frameRate));
		 
	       }
	       console->SetLine(3, temp, RGB_GREEN);
	       
	       sprintf(temp, "storage: %ld%%", mod->XStorage());
	       console->SetLine(4, temp, RGB_GREEN);
	       
	       sprintf(temp, "functionality: %d%%", mod->func);
	       console->SetLine(5, temp, RGB_GREEN);
	       
	       sprintf(temp, "mass: %s", Format(mod->mass));
	       console->SetLine(6, temp, RGB_GREEN);
	       
	       sprintf(temp, "volume: %d%%", mod->volume * 100 /
		       sh->design->volumeCap);
	       console->SetLine(7, temp, RGB_GREEN);
	       
	       sprintf(temp, "origin: %s", game->race[mod->raceID].name);
	       console->SetLine(8, temp, raceTextColor[mod->raceID]);
	       
	       j = mod->GetMode();
	       if (j == ModuleClass::standby) 
		 sprintf(temp, "status: standby");
	       else if (j == ModuleClass::locked)
		 sprintf(temp, "status: locked");
	       else
		 sprintf(temp, "status: unlocked");
	       
	       console->SetLine(9, temp, RGB_GREEN);
	       
	       *temp = '\0';
	     }

	   console->SetLine(2, temp, RGB_GREEN);
	   console->SetLine(10, temp2, RGB_GREEN);
	   console->SetLine(11, temp3, RGB_GREEN);
	   console->SetLine(12, temp4, RGB_GREEN);
	 }
         break;
   }
   case VIDEO_MESSAGE :
     console->SetIcon();
     de->button[16]->
       SetProperties("Home",SF_KEY_HOME,0,0,0,0,SF_MSG_KEYPRESSED_4,0,0,0,0);
     de->button[17]->
       SetProperties("<< PgUp",
                     SF_KEY_LT,SF_KEY_LT,SF_KEY_LT,SF_KEY_LT,0,
                     SF_MSG_KEYPRESSED_4,SF_MSG_KEYPRESSED_4,
                     SF_MSG_KEYPRESSED_4,SF_MSG_KEYPRESSED_4,0);
     de->button[18]->
       SetProperties("< Up",SF_KEY_LT,0,0,0,0,SF_MSG_KEYPRESSED_4,0,0,0,0);
     de->button[19]->SetProperties("",0,0,0,0,0,0,0,0,0,0);
     de->button[20]->
       SetProperties("Down >",SF_KEY_GT,0,0,0,0,SF_MSG_KEYPRESSED_4,0,0,0,0);
     de->button[21]->
       SetProperties("PgDn >>",SF_KEY_GT,SF_KEY_GT,SF_KEY_GT,SF_KEY_GT,0,
                     SF_MSG_KEYPRESSED_4,SF_MSG_KEYPRESSED_4,
                     SF_MSG_KEYPRESSED_4,SF_MSG_KEYPRESSED_4,0);
     for ( i=0; i<console->cyBuffer; i++ )
        console->SetLine(i,message[i],
                         raceTextColor[messageColor[i] % 8]);
     break;
   case VIDEO_HELP :
     console->SetIcon();

     de->button[16]->
       SetProperties("Home",SF_KEY_HOME,0,0,0,0,SF_MSG_KEYPRESSED_4,0,0,0,0);
     de->button[17]->
       SetProperties("<< PgUp",
                     SF_KEY_LT,SF_KEY_LT,SF_KEY_LT,SF_KEY_LT,0,
                     SF_MSG_KEYPRESSED_4,SF_MSG_KEYPRESSED_4,
                     SF_MSG_KEYPRESSED_4,SF_MSG_KEYPRESSED_4,0);
     de->button[18]->
       SetProperties("< Up",SF_KEY_LT,0,0,0,0,SF_MSG_KEYPRESSED_4,0,0,0,0);
     de->button[19]->SetProperties("",0,0,0,0,0,0,0,0,0,0);
     de->button[20]->
       SetProperties("Down >",SF_KEY_GT,0,0,0,0,SF_MSG_KEYPRESSED_4,0,0,0,0);
     de->button[21]->
       SetProperties("PgDn >>",SF_KEY_GT,SF_KEY_GT,SF_KEY_GT,SF_KEY_GT,0,
                     SF_MSG_KEYPRESSED_4,SF_MSG_KEYPRESSED_4,
                     SF_MSG_KEYPRESSED_4,SF_MSG_KEYPRESSED_4,0);

     i = 0;

     console->SetLine(i++,"Scroll Up <             > Scroll Down ",RGB_GREEN);
     console->SetLine(i++,"                                      ",RGB_YELLOW);
     console->SetLine(i++,"Ship Control (Inverted T)             ",RGB_GREEN);
     console->SetLine(i++,"   Left Arrow  - Rotate CCW           ",RGB_YELLOW);
     console->SetLine(i++,"   Right Arrow - Rotate CW            ",RGB_YELLOW);
     console->SetLine(i++,"   Up Arrow    - Forward Booster      ",RGB_YELLOW);
     console->SetLine(i++,"   Down Arrow  - Reverse Thrust       ",RGB_YELLOW);
     console->SetLine(i++,"                                      ",RGB_YELLOW);
     console->SetLine(i++,"Ship Control (Numeric Keypad)         ",RGB_GREEN);
     console->SetLine(i++,"   1) End         - Thrust Left       ",RGB_YELLOW);
     console->SetLine(i++,"   2) Down Arrow  - Reverse Thrust    ",RGB_YELLOW);
     console->SetLine(i++,"   3) Pg Dn       - Thrust Right      ",RGB_YELLOW);
     console->SetLine(i++,"   4) Left Arrow  - Rotate CCW        ",RGB_YELLOW);
     console->SetLine(i++,"   5)             - Forward Thrust    ",RGB_YELLOW);
     console->SetLine(i++,"   6) Right Arrow - Rotate CW         ",RGB_YELLOW);
     console->SetLine(i++,"   7) Home                            ",RGB_YELLOW);
     console->SetLine(i++,"   8) Up Arrow    - Forward Booster   ",RGB_YELLOW);
     console->SetLine(i++,"   9) Up Arrow                        ",RGB_YELLOW);
     console->SetLine(i++,"                                      ",RGB_YELLOW);
     console->SetLine(i++,"Weapon Control                        ",RGB_GREEN);
     console->SetLine(i++,"   Space - Fire All Standard Weapons  ",RGB_YELLOW);
     console->SetLine(i++,"   1     - Fire Tube 1                ",RGB_YELLOW);
     console->SetLine(i++,"   2     - Fire Tube 2                ",RGB_YELLOW);
     console->SetLine(i++,"   3     - Fire Tube 3                ",RGB_YELLOW);
     console->SetLine(i++,"   4     - Fire Tube 4                ",RGB_YELLOW);
     console->SetLine(i++,"   5     - Fire Tube 5                ",RGB_YELLOW);
     console->SetLine(i++,"   Shift+Number - Set auto fire mode  ",RGB_YELLOW);
     console->SetLine(i++,"   Shift+Number - Set manual fire mode",RGB_YELLOW);
     console->SetLine(i++,"                                      ",RGB_YELLOW);
     console->SetLine(i++,"Nav Computer                          ",RGB_GREEN);
     console->SetLine(i++,"   N       - Navigational Computer    ",RGB_YELLOW);
     console->SetLine(i++,"   G       - Go to Planet             ",RGB_YELLOW);
     console->SetLine(i++,"   ]       - Colonize Planet          ",RGB_YELLOW);
     console->SetLine(i++,"   ,       - scroll down major planets",RGB_YELLOW);
     console->SetLine(i++,"   .       - scroll up major planets  ",RGB_YELLOW);
     console->SetLine(i++,"   Shift , - scroll down all planets  ",RGB_YELLOW);
     console->SetLine(i++,"   Shift . - scroll up all planets    ",RGB_YELLOW);
     console->SetLine(i++,"   Ctrl ,  - scroll down friendly plan",RGB_YELLOW);
     console->SetLine(i++,"   Ctrl .  - scroll up friendly plan  ",RGB_YELLOW);
     console->SetLine(i++,"Target Computer                       ",RGB_YELLOW);
     console->SetLine(i++,"   T       - Targeting Computer       ",RGB_YELLOW);
     console->SetLine(i++,"   A       - Acquire Nearest Enemy    ",RGB_YELLOW);
     console->SetLine(i++,"             Ship or Colony           ",RGB_YELLOW);
     console->SetLine(i++,"   Shift A - Acquire Nearest Enemy    ",RGB_YELLOW);
     console->SetLine(i++,"             Ship, Colony or Weapon   ",RGB_YELLOW);
     console->SetLine(i++,"   F       - Fight Target             ",RGB_YELLOW);
     console->SetLine(i++,"   Shift F - Follow Target            ",RGB_YELLOW);
     console->SetLine(i++,"   H       - Request Help w/ Target   ",RGB_YELLOW);
     console->SetLine(i++,"   Shift H - Priority Help Request    ",RGB_YELLOW);
     console->SetLine(i++,"   Y       - Tractor Target           ",RGB_YELLOW);
     console->SetLine(i++,"   ,       - scroll down ship targets ",RGB_YELLOW);
     console->SetLine(i++,"   .       - scroll up ship targets   ",RGB_YELLOW);
     console->SetLine(i++,"   Shift , - scroll down onscreen tar-",RGB_YELLOW);
     console->SetLine(i++,"             gets                     ",RGB_YELLOW);
     console->SetLine(i++,"   Shift . - scroll up onscreen tar-  ",RGB_YELLOW);
     console->SetLine(i++,"             gets                     ",RGB_YELLOW);
     console->SetLine(i++,"   Ctrl ,  - scroll down enemy ships  ",RGB_YELLOW);
     console->SetLine(i++,"   Ctrl .  - scroll up enemy ships    ",RGB_YELLOW);
     console->SetLine(i++,"Ship Status Computer                  ",RGB_YELLOW);
     console->SetLine(i++,"   S       - Ship Status/Damage/Refit ",RGB_YELLOW);
     console->SetLine(i++,"   Shift S - Detailed Status info.    ",RGB_YELLOW);
     console->SetLine(i++,"   Shift \\ - Drop Selected System     ",RGB_YELLOW);
     console->SetLine(i++,"   ,       - scroll down ship systems ",RGB_YELLOW);
     console->SetLine(i++,"   .       - scroll up ship systems   ",RGB_YELLOW);
     console->SetLine(i++,"   Shift , - scroll upgrade system @ >",RGB_YELLOW);
     console->SetLine(i++,"   Shift . - scroll downgrade system  ",RGB_YELLOW);
     console->SetLine(i++,"             at >                     ",RGB_YELLOW);
     console->SetLine(i++,"   Enter   - upgrade menu for system  ",RGB_YELLOW);
     console->SetLine(i++,"             at >                     ",RGB_YELLOW);
     console->SetLine(i++,"   Sh-Enter- upgrade menu for all sys-",RGB_YELLOW);
     console->SetLine(i++,"             tems                     ",RGB_YELLOW);
     console->SetLine(i++,"Miscellaneous Computers               ",RGB_YELLOW);
     console->SetLine(i++,"   M       - Incoming Communications  ",RGB_YELLOW);
     console->SetLine(i++,"   P       - Pilots                   ",RGB_YELLOW);
     console->SetLine(i++,"   ,       - scroll down pilot list   ",RGB_YELLOW);
     console->SetLine(i++,"   .       - scroll up pilot list     ",RGB_YELLOW);
     console->SetLine(i++,"   R       - Census                   ",RGB_YELLOW);
     console->SetLine(i++,"   ,       - scroll down race list    ",RGB_YELLOW);
     console->SetLine(i++,"   .       - scroll up race lsit      ",RGB_YELLOW);
     console->SetLine(i++,"   Enter   - get detailed info on race",RGB_YELLOW);
     console->SetLine(i++,"             at >                     ",RGB_YELLOW);
     console->SetLine(i++,"   Shift R - Rank Information         ",RGB_YELLOW);
     console->SetLine(i++,"   Shift P - Planetary Status         ",RGB_YELLOW);
     console->SetLine(i++,"                                      ",RGB_YELLOW);
     console->SetLine(i++,"Function Keys                         ",RGB_GREEN);
     console->SetLine(i++,"   ESC - Main Window Menu             ",RGB_YELLOW);
     console->SetLine(i++,"   F1  - Show Help                    ",RGB_YELLOW);
     console->SetLine(i++,"   F2  - Broadcast Chat               ",RGB_YELLOW);
     console->SetLine(i++,"   F3  - Team Chat                    ",RGB_YELLOW);
     console->SetLine(i++,"   F4  - IRC Chat #sf/irc.stardock.com",RGB_YELLOW);
     console->SetLine(i++,"   F5  - Lock Chat Window             ",RGB_YELLOW);
     console->SetLine(i++,"   F6  -                              ",RGB_YELLOW);
     console->SetLine(i++,"   F7  - Show Debug Info              ",RGB_YELLOW);
     console->SetLine(i++,"   F8  - Save Data to Nexus           ",RGB_YELLOW);
     console->SetLine(i++,"   F9  - Resync With Server           ",RGB_YELLOW);
     console->SetLine(i++,"   F10 - Exit Game                    ",RGB_YELLOW);
     console->SetLine(i++,"   F11 -                              ",RGB_YELLOW);
     console->SetLine(i++,"   F12 -                              ",RGB_YELLOW);
     break;
   case VIDEO_SELECT :
     de->button[16]->
      SetProperties("< Up",SF_KEY_LT,0,0,0,0,SF_MSG_KEYPRESSED_4,0,0,0,0);
     if ( ( IsValidObjectPointer(sh->dockObj) && 
	    sh->dockObj->bus.factoryMod) ) {
        de->button[17]->
         SetProperties("Back",SF_KEY_S,0,0,0,0,SF_MSG_KEYPRESSED_4,0,0,0,0);
        de->button[18]->
         SetProperties("Upgrade",SF_KEY_ENTER,SF_KEY_ENTER,0,0,0,SF_MSG_KEYPRESSED_4,SF_MSG_KEYRELEASED_4,0,0,0);
        }
     de->button[19]->SetProperties("",0,0,0,0,0,0,0,0,0,0);
     de->button[20]->SetProperties("",0,0,0,0,0,0,0,0,0,0);
     de->button[21]->
      SetProperties("Down >",SF_KEY_GT,0,0,0,0,SF_MSG_KEYPRESSED_4,0,0,0,0);
     {
     console->SetIcon();

     sprintf(temp, "cr: %s", Format(game->player[sh->playerIndex].refValue));
     sprintf(temp2, "vol: %s", Format(sh->design->volumeCap));
     ConcatToFixedLength(temp, temp2, 38);
     console->SetLine(0, temp, raceTextColor[sh->bus.moduleList[sh->bus.modPtr].raceID]);

     n = game->NumModulesByType((short)sh->raceID, selType) + 3;

     *temp = '\0';
     console->SetLine(1, temp, RGB_GREEN);

     short selOffset = 2;
     if (selType != -1) {
       /* the first line should show the currently installed system */

       console->SetLine(4, temp, RGB_GREEN);

       strcpy(temp, " trade in           vol          value");
       console->SetLine(2, temp, RGB_GREEN);
       sprintf(temp, " %s", sh->bus.moduleList[sh->bus.modPtr].name);
       ConcatToFixedLength(temp, Format(sh->bus.moduleList[sh->bus.modPtr].volume), 23);
       ConcatToFixedLength(temp, Format(sh->bus.moduleList[sh->bus.modPtr].value()), 38);
       console->SetLine(3, temp, raceTextColor[sh->bus.moduleList[sh->bus.modPtr].raceID]);

       if (selPointer == 2)
	 *temp = '>';

       selOffset = 5;

     }

     n += selOffset;

     strcpy(temp, " purchase           vol          value");
     console->SetLine(selOffset, temp, RGB_GREEN);
     
     short inList = 0;
     slong vol = 0, val = 0, svol, mval = 0, mvol = 0;
     selIndex = -1;
     /********************************************************************/
     /*               i: module index in game state (0 - numModules-1)   */
     /*               j: line in display            (0 - 12)             */
     /*               k: index of module in list of displayed modules    */
     /*         selType: type of module selected                         */
     /*      selPointer: display line where the pointer is located       */
     /*       selOffset: number of extra lines to leave at the top of    */
     /*                  the display                                     */
     /* console->scroll: the current offset (in lines) of the display    */
     /*    console->chy: number of lines in the display window           */
     /*          inList: flag that indicates whether a module should be  */
     /*                  included in the display (independent of scroll) */
     /********************************************************************/

     for (i=0,j=1+selOffset,k=0; j<console->chy-1 && i<game->numModules; i++) {

       mod = game->moduleList + i;

       if ( mod && mod->check == 0xAAAAAAAA )
	 {
	   inList = 0;
	   if (selType != -1)
	     inList = mod->IsEquivalentTo(selType);
	   else
	     inList = mod->moduleFunc != ModuleClass::power_plant &&
	       mod->moduleFunc != ModuleClass::tube &&
	       mod->moduleFunc != ModuleClass::beam &&
	       mod->moduleFunc != ModuleClass::ram;
	 }
	   
	   if (inList && game->tech[sh->raceID].IsSet(i)) {
	     
	     if (k + selOffset + 1 >= console->scroll) {
	       
	       if (selType != -1 && mod ) {
		 vol = sh->bus.moduleList[sh->bus.modPtr].volume;
		 val = sh->bus.moduleList[sh->bus.modPtr].value();
	       } else {
		 vol = 0L;
		 val = 0L;
	       }
	       
	       if ( mod && mod->check == 0xAAAAAAAA )
		 {
		   sprintf(temp, " %s", mod->name);
		   ConcatToFixedLength(temp, Format(mod->volume), 23);
		   ConcatToFixedLength(temp, Format(mod->value()), 38);	
		   
		   if (j == selPointer) {
		     *temp = '>';
		     selID = mod->id;
		     selIndex = i;
		     mval = mod->value();
		     mvol = mod->volume;
		   }
		   
		   console->SetLine(k + selOffset + 1, temp, raceTextColor[mod->raceID]);
		 }
	       
	       j++;
	     }
	     
	     k++;
	   }
     }
     
     /* clear the remaining lines */
     *temp = '\0';
     for (i = j; i<=console->chy - 2; i++)
       console->SetLine(i, temp, RGB_GREEN);

     if (selIndex == -1)
       break;

     svol = sh->Volume();

     /* use the last line to indicate that part is too big or expensive to upgrade to */
     if (svol - vol + mvol > sh->design->volumeCap) 
       sprintf(temp, "[exceeds volume limits by %s]", Format(sh->design->volumeCap - svol - mvol + vol));
     else if (game->player[sh->playerIndex].refValue + val - mval < 0)
       sprintf(temp, "[you need %s more credit]", Format(mval - game->player[sh->playerIndex].refValue- val));
     else if (val <= mval)
	 strcpy(temp, "[you may upgrade to this item]");
     else
       strcpy(temp, "[you may downgrade to this item]");

     console->SetLine(console->chy - 1 + console->scroll - console->numHeaderLines, temp, RGB_GREEN);

     }

     break;
   case VIDEO_RANKLIST :
     console->SetIcon();

     console->SetNumHeaderLines(1);

     de->button[16]->
       SetProperties("<< PgUp",
                     SF_KEY_LT,SF_KEY_LT,SF_KEY_LT,SF_KEY_LT,0,
                     SF_MSG_KEYPRESSED_4,SF_MSG_KEYPRESSED_4,
                     SF_MSG_KEYPRESSED_4,SF_MSG_KEYPRESSED_4,0);
     de->button[17]->
       SetProperties("< Up",SF_KEY_LT,0,0,0,0,SF_MSG_KEYPRESSED_4,0,0,0,0);
     de->button[18]->SetProperties("",0,0,0,0,0,0,0,0,0,0);
     de->button[19]->SetProperties("",0,0,0,0,0,0,0,0,0,0);
     de->button[20]->
       SetProperties("Down >",SF_KEY_GT,0,0,0,0,SF_MSG_KEYPRESSED_4,0,0,0,0);
     de->button[21]->
       SetProperties("PgDn >>",SF_KEY_GT,SF_KEY_GT,SF_KEY_GT,SF_KEY_GT,0,
                     SF_MSG_KEYPRESSED_4,SF_MSG_KEYPRESSED_4,
                     SF_MSG_KEYPRESSED_4,SF_MSG_KEYPRESSED_4,0);
     
     /* show rank stats */
     
     strcpy(temp, " rank     AS       TS  Kills      Duty");
     console->SetLine(0, temp, RGB_GREEN);
     
     for ( i=0,j=console->scroll; i<game->numRanks; i++ ) {
       
       if ( i >= console->scroll - console->numHeaderLines ) {
	 
	 //if (selPointer == i)
	 // sprintf(temp, ">%s", game->rank[i].name);
	 //else
	 //  sprintf(temp, " %s", game->rank[i].name);

	 strcpy(temp, game->rank[i].name);

	 sprintf(temp2,"%s", Format(game->rank[i].activeScore));
	 
	 ConcatToFixedLength(temp, temp2, 12);
	 
	 sprintf(temp2,"%s", Format(game->rank[i].totalScore));
	 
	 ConcatToFixedLength(temp, temp2, 21);

	 sprintf(temp2,"%s", Format(game->rank[i].totalKills));
	 
	 ConcatToFixedLength(temp, temp2, 28);
	 
	 sprintf(temp2,"%s", detailName[game->rank[i].duty]);
	 
	 ConcatToFixedLength(temp, temp2, 38);
	 
	 console->SetLine(j,temp, RGB_GREEN);
	 
	 j++;
	 
       }
     }
     
     /* clear the remaining lines */
     *temp = '\0';
     for (i=j; i<console->chy; i++)
       console->SetLine(i,temp,RGB_GREEN);
     
     break;
   case VIDEO_RACELIST :
     console->SetIcon();

     de->button[16]->
      SetProperties("< Up",SF_KEY_LT,0,0,0,0,SF_MSG_KEYPRESSED_4,0,0,0,0);
     de->button[17]->SetProperties("",0,0,0,0,0,0,0,0,0,0);
     de->button[18]->SetProperties("",0,0,0,0,0,0,0,0,0,0);
     de->button[19]->SetProperties("",0,0,0,0,0,0,0,0,0,0);
     de->button[20]->SetProperties("",0,0,0,0,0,0,0,0,0,0);
     de->button[21]->
      SetProperties("Down >",SF_KEY_GT,0,0,0,0,SF_MSG_KEYPRESSED_4,0,0,0,0);

     switch (subMode) {
     case 0:
       console->SetNumHeaderLines(1);
       de->button[17]->
        SetProperties("View",SF_KEY_ENTER,SF_KEY_ENTER,0,0,0,SF_MSG_KEYPRESSED_4,SF_MSG_KEYRELEASED_4,0,0,0);
       {
	 /* show planet and population totals */
	     
	 strcpy(temp, " race   ships/max plan pop% vic% time");
	 console->SetLine(0, temp, RGB_GREEN);

	 RaceClass* r;
	 
	 for ( i=0,j=1; i<min(console->chy - 1, game->numRaces); i++ ) {
	   
	   if ( i >= console->scroll - console->numHeaderLines ) {
	     
	     r = game->race + i;
	     
	     if (selPointer == j)
	       sprintf(temp, ">%ss", r->name);
	     else
	       sprintf(temp, " %ss", r->name);

	     sprintf(temp2,"%d/%d",
		     r->numRobots,
		     min(r->maxPlanetRobots + r->maxPopRobots,
			 game->numShips - game->maxHumanShips));
	     
	     ConcatToFixedLength(temp, temp2, 14);
	     
	     sprintf(temp2,"%d",
		     r->numPlanets);
	     
	     ConcatToFixedLength(temp, temp2, 20);
	     
	     sprintf(temp2,"%ld%%",
		     r->totPop * 100 / max(game->totPop,1));
	     
	     ConcatToFixedLength(temp, temp2, 25);


	     sprintf(temp2, "%d%%", r->CalculateVictoryPct());
	     
	     ConcatToFixedLength(temp, temp2, 31);

	     if (r->conditionTiming != unlimited_time) {

	       seconds = r->conditionTimePar - r->conditionSeconds;

	       if (seconds >= 60) {

		 minutes = seconds / 60;
		 seconds = seconds % 60;
	       
	       } else {

		 minutes = 0;

	       }

	       sprintf(temp2, "%ld:%02ld", minutes, seconds);
	       ConcatToFixedLength(temp, temp2, 38);

	     }

	     console->SetLine(j,temp,raceTextColor[i]);

	     j++;
	     
	   }
	 }
	 
	 /* clear the remaining lines */
	 *temp = '\0';
	 for (i=j; i<console->chy; i++)
	   console->SetLine(i,temp,RGB_GREEN);
	 
       }
       break;
     case 1:
       console->SetNumHeaderLines(9);
       de->button[17]->
        SetProperties("Back",SF_KEY_ENTER,SF_KEY_ENTER,0,0,0,SF_MSG_KEYPRESSED_4,SF_MSG_KEYRELEASED_4,0,0,0);
       {

	 /* show race detail data */
	 
	 uint selection = (selPointer-1) % game->numRaces;
	 
	 RaceClass* r = game->race + selection;
	 
	 sprintf(temp, "name: %ss", r->name);
	 console->SetLine(0, temp, raceTextColor[selection]);

	 sprintf(temp, "home: %s",
		 game->planet[r->home].name);
	 console->SetLine(1, temp, RGB_GREEN);
	 
	 console->SetLine(2, "preferences:", RGB_GREEN);

	 sprintf(temp, "  temperature:     %6d K", r->prefTemp);
	 console->SetLine(3, temp, RGB_GREEN);

	 sprintf(temp, "  Atm. pressure:   %6.2f Atm", r->prefAtmP/100.);
	 console->SetLine(4, temp, RGB_GREEN);

	 sprintf(temp, "  Surface gravity: %6.2f g", r->prefGrav/100.);
	 console->SetLine(5, temp, RGB_GREEN);

	 sprintf(temp, "victory condition: %d%%", r->CalculateVictoryPct());
	 console->SetLine(6, temp, RGB_GREEN);

	 sprintf(temp, "victory timer: %d s", r->conditionSeconds);
	 console->SetLine(7, temp, RGB_GREEN);

	 switch (r->conditionType) {
	 case hold_num:
	   sprintf(temp2, "hold %ld planets", r->conditionNumPar);
	   break;
	 case hold_list:
	   sprintf(temp2, "hold listed planets");
	   break;
	 case conquer_num:
	   sprintf(temp2, "conquer %ld planets", r->conditionNumPar);
	   break;
	 case conquer_list:
	   sprintf(temp2, "conquer listed planets");
	   break;
	 }

	 /* now concatenate the timing conditions */
	 switch (r->conditionTiming) {
	 case within_time:
	   if (r->conditionTimePar < 0) /* time unit is "lives" */
	     sprintf(temp, "%s within %d lives", temp2, -r->conditionTimePar);
	   else if ((r->conditionTimePar / 60) * 60 == r->conditionTimePar)
	     /* give time in minutes */
	     sprintf(temp, "%s within %d min.", temp2,
		     r->conditionTimePar/60);
	   else
	     /* give time in seconds */
	     sprintf(temp, "%s within %d sec.", temp2,
		     r->conditionTimePar);
	   break;
	 case for_time:
	   if ((r->conditionTimePar / 60) * 60 == r->conditionTimePar)
	     /* give time in min */
	     sprintf(temp, "%s for %d min.", temp2,
		     r->conditionTimePar/60);
	   else
	     sprintf(temp, "%s for %d sec.", temp2, r->conditionTimePar);
	   break;
	 case unlimited_time:
	   strcpy(temp, temp2);
	   break;
	 }
	 console->SetLine(8, temp, RGB_GREEN);
	 
	 if (r->conditionType == hold_list ||
	     r->conditionType == conquer_list) {
	   /* print planet list if necessary */
	   slong listFlags = r->conditionNumPar;
	   short j = 0, k = 8;
	   slong n;
	   while (listFlags && k < console->chy - 1 + console->scroll) {
	     n = 1 << j;
	     if (r->conditionNumPar & n) {
	       k++;
	       listFlags = listFlags ^ (listFlags & n);
	       if (k-8 > console->scroll - console->numHeaderLines) {
		 sprintf(temp, " %s", game->planet[j].name);
		 console->SetLine(k/*-console->scroll*/, temp,
				  raceTextColor[game->planet[j].raceID]);
	       }
	     }
	     j++;
	   }
	 }
       }
       break;
     }
     break;
   case VIDEO_PLANETLIST :
     /* show planet list */
     
     i = 0;
     
     console->SetNumHeaderLines(1);

     de->button[16]->
       SetProperties("<< PgUp",
                     SF_KEY_LT,SF_KEY_LT,SF_KEY_LT,SF_KEY_LT,0,
                     SF_MSG_KEYPRESSED_4,SF_MSG_KEYPRESSED_4,
                     SF_MSG_KEYPRESSED_4,SF_MSG_KEYPRESSED_4,0);
     de->button[17]->
       SetProperties("< Up",SF_KEY_LT,0,0,0,0,SF_MSG_KEYPRESSED_4,0,0,0,0);
     de->button[18]->
       SetProperties("View",SF_KEY_ENTER,SF_KEY_ENTER,0,0,0,SF_MSG_KEYPRESSED_4,SF_MSG_KEYRELEASED_4,0,0,0);
     de->button[19]->SetProperties("",0,0,0,0,0,0,0,0,0,0);
     de->button[20]->
       SetProperties("Down >",SF_KEY_GT,0,0,0,0,SF_MSG_KEYPRESSED_4,0,0,0,0);
     de->button[21]->
       SetProperties("PgDn >>",SF_KEY_GT,SF_KEY_GT,SF_KEY_GT,SF_KEY_GT,0,
                     SF_MSG_KEYPRESSED_4,SF_MSG_KEYPRESSED_4,
                     SF_MSG_KEYPRESSED_4,SF_MSG_KEYPRESSED_4,0);

     console->SetLine(0," name          sup  hab    pop    stat",RGB_GREEN);
     
     for ( i=0,j=1,k=0; i<game->numPlanets; i++ ) {
       
	 sprintf(temp," %s", game->planet[i].name);
	 
	 if (j == console->scroll + selPointer - console->numHeaderLines) {
	   *temp = '>';
	   selIndex = i;
           }

	 if ( IsValidObjectPointer(game->planet[i].colony)/*&&!changeMode*/) {
	   
	   if (/*game->planet[i].colony->frame % frameRate ==*/ 1) {
	     
	     sprintf(temp2,"%d%%",
		     min(game->planet[i].colony->supply,100));
	     
	     ConcatToFixedLength(temp, temp2, 18);
	     
	     sprintf(temp2,"%d%%",
		     game->planet[i].
		     Habitability(game->race[game->
					    ship[localSlotNumber].
					    raceID]));
	     
	     ConcatToFixedLength(temp, temp2, 23);
	     
	     sprintf(temp2,"%s",game->planet[i].colony->pop.print());
	     
	     ConcatToFixedLength(temp, temp2, 33);
	     
	     if (game->planet[i].colony->supply < max_supply-1)
	       strcpy(temp2, "blkd");
	     else if (game->planet[i].colony->frame - game->planet[i].colony->distressFrame <
		      60 * frameRate && game->planet[i].colony->distressFrame)
	       strcpy(temp2, "help");
	     else
	       strcpy(temp2, "ok");
	     
	     
	     ConcatToFixedLength(temp, temp2, 38);
	     
	     console->SetLine(j,temp,
			      raceTextColor[game->planet[i].raceID]);
	   }
	 } else {
	   
	   sprintf(temp2,"%d%%",
		   game->planet[i].
		   Habitability(game->race[game->
					  ship[localSlotNumber].
					  raceID]));
	   
	   ConcatToFixedLength(temp, temp2, 18);
	   
	   console->SetLine(j,temp,
			    raceTextColor[game->planet[i].raceID]);
	 }
	 
	 j++;

     }
     break;
   case VIDEO_PLAYERLIST :
     /* show player win/loss stats */

     console->SetNumHeaderLines(1);

     console->SetIcon();

     de->button[16]->
       SetProperties("<< PgUp",
                     SF_KEY_LT,SF_KEY_LT,SF_KEY_LT,SF_KEY_LT,0,
                     SF_MSG_KEYPRESSED_4,SF_MSG_KEYPRESSED_4,
                     SF_MSG_KEYPRESSED_4,SF_MSG_KEYPRESSED_4,0);
     de->button[17]->
       SetProperties("< Up",SF_KEY_LT,0,0,0,0,SF_MSG_KEYPRESSED_4,0,0,0,0);
     de->button[18]->SetProperties("",0,0,0,0,0,0,0,0,0,0);
     de->button[19]->SetProperties("",0,0,0,0,0,0,0,0,0,0);
     de->button[20]->
       SetProperties("Down >",SF_KEY_GT,0,0,0,0,SF_MSG_KEYPRESSED_4,0,0,0,0);
     de->button[21]->
       SetProperties("PgDn >>",SF_KEY_GT,SF_KEY_GT,SF_KEY_GT,SF_KEY_GT,0,
                     SF_MSG_KEYPRESSED_4,SF_MSG_KEYPRESSED_4,
                     SF_MSG_KEYPRESSED_4,SF_MSG_KEYPRESSED_4,0);
     
     i = 0;
     
     console->SetLine(0,"name                   K     L       S",RGB_GREEN);
     
     for ( i=0,j=console->numHeaderLines; i<game->numShips; i++ ) {
       
       if (game->ship[i].IsActive()) {
	   sprintf(temp,"%2d %s %s",i,
		   game->rank[game->playerList[i]->rank].name, 
		   game->playerList[i]->name);
	   
	   /* truncate long names at the 15th character */
	   temp[19] = '\0';
	   
	   sprintf(temp2,"%ld",
		   game->playerList[i]->numKillsTotal);
	   
	   ConcatToFixedLength(temp, temp2, 24);
	   
	   sprintf(temp2,"%ld",
		   game->playerList[i]->numKilled);
	   
	   ConcatToFixedLength(temp, temp2, 30);
	   
	   sprintf(temp2,"%s",
		   Format(game->playerList[i]->totalScore));
	   
	   ConcatToFixedLength(temp, temp2, 38);
	   
	   console->SetLine(j, temp,
			    raceTextColor[game->playerList[i]->raceId]);
	   
	   j++;
	 }
     }
     
     break;
     
   }

   /* reset changeMode flag to allow more information to fill in */
   /* in the playerlist mode                                     */
   changeMode = 0;
   return 0;
}

/* ************************************************************************* */

#define NUM_TANS 5
#define SCALE_FACTOR 10

/* ***** Angles 10,20,30,40 ***** */
double    dcotan[NUM_TANS] = { 11.4300, 3.7320, 2.1445, 1.4281, 1.0 };
slong      lcotan[NUM_TANS] = {     114,     37,     21,     14, 10 };
slong cotanangles[NUM_TANS] = {      10,     20,     30,     40,  50 };

/* ***** Angles 50,60,70,80 ***** */
double     dtan[NUM_TANS] = {    1.0, 1.4281, 2.1445, 3.7320, 11.4300 };
slong       ltan[NUM_TANS] = {     10,     14,     21,     37,     114 };
slong  tanangles[NUM_TANS] = {     50,     60,     70,     80,      90 };

slong Atan2( slong y, slong x ) {

   slong signx,signy,xovery = 0,yoverx = 0;
   slong i,angle;

   signx = (x>0)?1:-1;
   signy = (y>0)?1:-1;

   x = labs(x);
   y = labs(y);

   if ( y != 0 )
      xovery = SCALE_FACTOR*x/y;
   if ( x != 0 )
      yoverx = SCALE_FACTOR*y/x;

   if ( x == 0 && y == 0 )
      angle = 0;
   else if ( x == 0 )
      angle = 90;
   else if ( y == 0 )
      angle = 0;
   else if ( xovery > SCALE_FACTOR ) { /* **** Use lcotan ***** */
      angle = 0;
      i = 0;
      while ( xovery < lcotan[i] ) {
         angle = cotanangles[i++];
         }
      }
   else { /* ***** Use ltan ***** */
      angle = 40;
      i = 0;
      while ( yoverx >= ltan[i] && angle != 90 ) {
         angle = tanangles[i++];
         }
      }

   if ( signx < 0 && signy > 0 )
     angle = 180 - angle;
   else if ( signy < 0 && signx < 0 )
      angle  = 180 + angle;
   else if ( signx > 0 && signy < 0 )
     angle = 360 - angle;

   if ( angle >= 360 )
      angle -= 360;

   return( angle );

   }

char* Format(slong l) {

  static char s[24];
  static char t[24];

  if (l >= 0) {

    sprintf(s, "%ld", l);
    return Format(s);

  } else {
    sprintf(s, "%ld", -l);
    t[0] = '-';
    t[1] = '\0';
    return strcat(t,Format(s));
  }

}

char* Format(char* s) {

  static char out[24];

  short i,j;
  short n = strlen(s);
  short nout = n + (n-1)/3;
  for (j=i=0; i<n; j++,i++) {
    if (i%3 == 0 && i != 0) {
      out[nout-j-1] = ',';
      j++;
    }
    out[nout-j-1] = s[n-i-1];
  }
  out[nout] = '\0';
  return out;
}
