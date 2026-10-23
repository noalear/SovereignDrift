/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
/* global function definitions */

extern SF_Client *sf;
extern short localSlotNumber;

/**** local inline functions ****/
inline slong IsServer() {
  return sf && sf->engine == NULL;
}

inline slong IsLocalPlayer(ShipClass* s) {
return short(s - game->ship.rootOf() == localSlotNumber &&
	     s->Type() == ship);
}

inline slong IsConnectedClient() {
return sf && sf->engine && sf->engine->state == SF_CLIENT_CONNECTED;
}

inline slong IsDisconnectedClient() {
  return sf && sf->engine && sf->engine->state==SF_CLIENT_SINGLEPLAYER;
}

/*
inline int
IsValidObjectPointer(GenericClass *g,char *file =0, int line =0, short logNull=FALSE)
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
      isValid = g->check == 0xAAAAAAAA;
    }

  if ( !isValid && file && line)
    {
      logFile << "ERROR> Invalid GenericClass pointer " << file << 
                 " line " << line << endl;
    }

  return isValid;
}
*/
