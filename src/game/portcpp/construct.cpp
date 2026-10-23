/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
#include <S_Lib.h>

#include <time.h>
#include <limits.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#if defined(OS2) || defined(WIN32)
#  include "../gui/sf.h"
#  include "../gui/SF_Player.h"
#  include "../gui/SF_Msg.h"
#  include "../gui/SF_MsgBuffer.h"
#  include "../gui/SF_VideoConsole.h"
#  include "../gui/SF_Globals.h"
#  include "../gui/SF_Engine.h"
#  include "../gui/SF_ClientWindow.h"
#  include "../gui/SF_Client.h"
#endif
#ifdef UNIX
#  include "../gui/sf.h"
#  include "../gui/SF_Player.h"
#  include "../gui/SF_Msg.h"
#  include "../gui/SF_MsgBuffer.h"
#  include "../gui/SF_VideoConsole.h"
#  include "../gui/SF_Globals.h"
#  include "../gui/SF_Engine.h"
#  include "../gui/SF_ClientWindow.h"
#  include "../gui/SF_Client.h"
#endif

#include "classes.h"
#include "inc_portcpp.h"
#include "enginepp.h"

/* global longs */
slong warningMessagePostedFrame = 0;
slong warningBottomMessagePostedFrame = 0;

/* global shorts */
short warningMessagePosted = 0;
short warningBottomMessagePosted = 0;
short numPlanetRobotsSet = 0;
short numPopRobotsSet = 0;

/* global chars */
char instanceName[8];

/* global strings */
SFString nullString;

const char* ModuleFunctionName[ModuleClass::last_module] = {
  "power", "eng", "qdrv", "tube", "shield", "brake", "sensor",
  "fuel", "batt", "cm", "stealth", "boost",
  "dock", "factory", "beam", "ram"};

void Parse(char*, SFString*, SFString*, short, short =0);
void Parse(SFString&, SFString*, SFString*, short, short =0);


/***************************************/
/* external objects -- not widely used */
/*				       */
/***************************************/
extern const char* detailName[];
extern ObjectBitmapClass NoObjectBitmap;

#define OFFSET_FROM_ROOT(x,c) sizeDump << "ModuleClass." #x " " << slong(&(c.x))-slong(&c) << endl;

void DumpSizeData() {
  ofstream sizeDump("classes.dat");
  sizeDump << "GameStateClass:      " << sizeof(GameStateClass) << endl;
  sizeDump << "ShipClass:           " << sizeof(ShipClass) << endl;
  sizeDump << "PlanetClass:         " << sizeof(PlanetClass) << endl;
  sizeDump << "MissileClass:        " << sizeof(MissileClass) << endl;
  sizeDump << "GameBitmapClass:     " << sizeof(GameBitmapClass) << endl;
  sizeDump << "ShipDesignStruct:    " << sizeof(ShipDesignStruct) << endl;
  sizeDump << "MissileDesignStruct: " << sizeof(MissileDesignStruct) << endl;
  sizeDump << "ModuleClass:         " << sizeof(ModuleClass) << endl;
  sizeDump << "RaceClass:           " << sizeof(RaceClass) << endl;
  sizeDump << "PlayerClass:         " << sizeof(PlayerClass) << endl;
  sizeDump << "ColonyClass:         " << sizeof(ColonyClass) << endl;

  ModuleClass m;

  /* special section for ModuleClass specifics */
  OFFSET_FROM_ROOT(name[0],m);
  OFFSET_FROM_ROOT(id,m);
  OFFSET_FROM_ROOT(raceID,m);
  OFFSET_FROM_ROOT(power,m);
  OFFSET_FROM_ROOT(mass,m);
  OFFSET_FROM_ROOT(output,m);
  OFFSET_FROM_ROOT(store,m);
  OFFSET_FROM_ROOT(damageCap,m);
  OFFSET_FROM_ROOT(damage,m);
  OFFSET_FROM_ROOT(par[0],m);
  OFFSET_FROM_ROOT(specPar[0],m);
  OFFSET_FROM_ROOT(workPar[0],m);
  OFFSET_FROM_ROOT(host,m);
  OFFSET_FROM_ROOT(func,m);
  OFFSET_FROM_ROOT(moduleFunc,m);
  OFFSET_FROM_ROOT(active,m);
  OFFSET_FROM_ROOT(frame,m);
  OFFSET_FROM_ROOT(volume,m);
  OFFSET_FROM_ROOT(debrisBitmapIndex,m);
  OFFSET_FROM_ROOT(nPar,m);
  OFFSET_FROM_ROOT(nSPar,m);
  OFFSET_FROM_ROOT(nWPar,m);
  OFFSET_FROM_ROOT(mode,m);
  OFFSET_FROM_ROOT(fire,m);
  OFFSET_FROM_ROOT(storage,m);
  OFFSET_FROM_ROOT(requirement,m);
  OFFSET_FROM_ROOT(withdraw,m);
  OFFSET_FROM_ROOT(deposit,m);

  sizeDump.close();
}

enum DataSection {game_data_section, race_data_section, rank_data_section,
		  player_data_section, sound_data_section, sound_event_section,
		  ship_bitmap_section, ship_design_section, ship_data_section,
		  missile_bitmap_section, missile_design_section, planet_bitmap_section,
		  planet_data_section, module_data_section, colony_data_section,
		  colony_bitmap_section, colony_design_section, explosion_bitmap_section,
		  transition_bitmap_section, miscellaneous_bitmap_section, debris_bitmap_section,
		  skip_section};

short ReadGameData(DataSection sec, SFString classStr, ifstream& file,
		   SFString* nameList =0, SFString* valueList =0, short nList =0, short skip =0) {

  char peek;
  short done;
  SFString* tokenList = 0;
  SFString* dirList = 0;
  SFString* argList = 0;
  SFString* newNameList = 0;
  SFString* newValueList = 0;
  SFString* elementList = 0;
  short nArg = 0, i, nElement;
  char line[1024];
  SFString sLine;
  short nTokens, nDir;
  char* filename;

  short inIfBlock = 0;

  do {

    done = 0;

    if (file.eof()) {
      //logFile << "note> closing input file." << endl;
      return 1;
    }

    peek = file.peek();

    while (peek == '\n' && !file.eof()) {
      file.get(peek);
    }

    if (file.eof() || peek == char(EOF)) {
      //logFile << "note> closing input file." << endl;
      return 1;
    }

    switch (peek) {

    case '*':
      file.getline(line, 1024);
      done = 0;
      break;

    case '}':
      file.getline(line, 1024);
      done = 1;
      break;

    case '#':

      file.getline(line, 1024);
      sLine = line;
      
      nTokens = sLine.TokenList(tokenList, "#()");

      nDir = tokenList[0].TokenList(dirList, " ");

      if (dirList[0] == "INCLUDE") {

	if (skip) {
          if ( tokenList ) {
            delete[] tokenList;
            tokenList = NULL;
	  }
          if ( dirList ) {
            delete[] dirList;
            dirList = NULL;
	  }
	  break;
	}

	filename = (SFString("..\\dat\\") + dirList[1]).out();

        if (filename) {

	  ifstream file2;

          _OsFilename(filename);
	  
	  //logFile << "note> opening " << filename << "..." << endl;
	  file2.open(filename);

	  /* parse arguments, if given */
	  if (nTokens == 2) {
	    
	    nArg = tokenList[1].TokenList(argList, ",") + nList;
	    
	    if (nArg) {
	      newNameList = new SFString[nArg];
	      newValueList = new SFString[nArg];
	    }
	
	    for (i=0; i<nArg - nList; i++) {
	      nElement = argList[i].TokenList(elementList, "= ");
	      if (nElement) {
		newNameList[i] = elementList[0];
		if (nElement > 1)
		  newValueList[i] = elementList[1];
	      }
	      delete[] elementList;
	      
	    }
	    
            delete[] argList;

	    /**********************************************************/
	    /* append the contents of the old list onto the new one   */
	    /* note: anything in common between the old and new lists */
	    /* will be superceded by the new value since it will ap-  */
	    /* pear in the list first.                                */
	    /**********************************************************/
	    if (nameList && valueList && nList) {
	      for (i=nArg - nList; i < nArg; i++) {
		newNameList[i] = nameList[i - nArg + nList];
		newValueList[i] = valueList[i - nArg + nList];
	      }
	    }

	  } /* *** if ( nTokens == 2 ) *** */

	  ReadGameData(sec, classStr, file2, newNameList, newValueList, nArg);

	  file2.close();

          if (newNameList) {
            delete[] newNameList;
            newNameList = NULL;
	  }
	  
          if (newValueList) {
            delete[] newValueList;
            newValueList = NULL;
	  }

	  delete filename;

	  /************************/
	  /* end of if (filename) */
	  /************************/
	}

	/******************************/
	/* end of "INCLUDE" directive */
	/******************************/

      } else if (dirList[0] == "IF") {

	inIfBlock++;

	if (skip) {
          if ( tokenList ) {
            delete[] tokenList;
            tokenList = 0;
	  }
          if ( dirList ) {
            delete[] dirList;
            dirList = 0;
	  }
	  break;
	}

	if (nDir == 2) { /* example: #IF special */

	  /***************************************************************/
	  /* simple one-term expression                                  */
	  /* scan the nameList for the expression and return TRUE if the */
	  /* name is in the list.                                        */
	  /***************************************************************/

	  i = 0;
	  while (i < nList && nameList[i] != dirList[1])
	    i++;

	  if (i == nList) {
	    if (ReadGameData(skip_section, classStr, file, nameList, valueList, nList, 1) == -1)
	      /* the current if block has been processed but was terminated by an ELSE directive */
	      /* since the current section condition failed then process the section following   */
	      /* the ELSE until an ENDIF is encountered                                          */
	      ReadGameData(sec, classStr, file, nameList, valueList, nList, 0);
	  } else {
	    if (ReadGameData(sec, classStr, file, nameList, valueList, nList, 0) == -1)
	      /* the current if block has been processed but was terminated by an ELSE directive */
	      /* since the current section condition succeeded then ignore the section following */
	      /* the ELSE until an ENDIF is encountered                                          */
	      ReadGameData(skip_section, classStr, file, nameList, valueList, nList, 1);
	  }
	  
	} else if (nDir == 3) { /* example: #IF NOT special */

	  if (dirList[1] == "NOT") {

	    /***************************************************************/
	    /* simple one-term expression with NOT preceding it.           */
	    /* scan the nameList for the expression and return TRUE if the */
	    /* name is NOT in the list.                                    */
	    /***************************************************************/
	    i = 0;
	    while (i < nList && nameList[i] != dirList[2])
	      i++;
	    
	    if (i < nList) {
	      if (ReadGameData(skip_section, classStr, file, nameList, valueList, nList, 1) == -1) {
		/* the current if block has been processed but was terminated by an ELSE directive */
		/* since the current section condition failed then process the section following   */
		/* the ELSE until an ENDIF is encountered                                          */
		ReadGameData(sec, classStr, file, nameList, valueList, nList, 0);
	      }
	    } else {
	      if (ReadGameData(sec, classStr, file, nameList, valueList, nList, 0) == -1)
		/* the current if block has been processed but was terminated by an ELSE directive */
		/* since the current section condition succeeded then ignore the section following */
		/* the ELSE until an ENDIF is encountered                                          */
		ReadGameData(skip_section, classStr, file, nameList, valueList, nList, 1);
	    }
	  } 
	} else if (nDir == 4) { /* example: #IF variable == value */
   
	  if (dirList[2] == "==" || dirList[2] == "!=")  {
	    
	    /* example: #IF expression [ == | != ] value */
	    
	    /***************************************************************/
	    /* two simple one-term expressions separated by a logical op-  */
	    /* erator.  Scan the nameList for the expression, compare it   */
	    /* to the value and return TRUE if they satisfy the logical    */
	    /* relationship implied by the operator.                       */
	    /***************************************************************/
	    
	    /* find the matching name in the nameList */
	    i = 0;
	    while (i < nList && nameList[i] != dirList[1])
	      i++;

	    /* assuming a matching name is found, do the comparison */
	    /* if no matching name is found and the operator is != then read as TRUE */

	    if (i < nList && (valueList[i] == dirList[3] && dirList[2] == "==" ||
			      valueList[i] != dirList[3] && dirList[2] == "!=" ) ||
		i == nList && dirList[2] == "!=") {
     
	      if (ReadGameData(sec, classStr, file, nameList, valueList, nList, 0) == -1) {
		/* the current if block has been processed but was terminated by an ELSE directive */
		/* since the current section condition succeeded then ignore the section following */
		/* the ELSE until an ENDIF is encountered                                          */
		ReadGameData(skip_section, classStr, file, nameList, valueList, nList, 1);
	      }
	    } else {

	      if (ReadGameData(skip_section, classStr, file, nameList, valueList, nList, 1) == -1) {
		/* the current if block has been processed but was terminated by an ELSE directive */
		/* since the current section condition failed then ignore the section following    */
		/* the ELSE until an ENDIF is encountered                                          */
		ReadGameData(sec, classStr, file, nameList, valueList, nList, 0);
	      }
	    }

	  } else {
	    
	    logFile << "ERROR> unknown syntax found; line reads: " << line << endl;
	    
	  }
	}
	
	/*************************/
	/* end of "IF" directive */
	/*************************/
	
      } else if (dirList[0] == "ECHO") {
	
	/* example: #ECHO text... */
	
	if (skip) {
	  if ( tokenList ) {
	    delete[] tokenList;
	    tokenList = 0;
	  }
	  if ( dirList ) {
	    delete[] dirList;
	    dirList = 0;
	  }
	  break;
	}
	
	sLine = line;
	sLine = sLine.rem("#ECHO ");
	if (nameList && valueList && nList)
	  Parse(sLine, nameList, valueList, nList, 1);
	sLine = sLine.subs("\\", "");
	
	logFile << sLine << endl;
	
	/****************************/
	/* end of "ECHO" directive */
	/****************************/
	
      } else if (dirList[0] == "ARG") {
	
	/* example: #ARG */
	/* lists the arguments passed to this context/section */
	
	if (skip) {
	  if ( tokenList ) {
	    delete[] tokenList;
	    tokenList = 0;
	  }
	  if ( dirList ) {
	    delete[] dirList;
	    dirList = 0;
	  }
	  break;
	}
      
	for (i=0; i<nList; i++) {
	
	  logFile << i << "> "<< nameList[i] << " --> ";
	
	  if (!valueList[i].IsNull())
	    logFile << valueList[i] << endl;
	  else
	    logFile << "<TRUE>" << endl;
	
	}
      
	/**************************/
	/* end of "ARG" directive */
	/**************************/
      
      } else if (dirList[0] == "ELSE") {

	/* example: #ELSE */

	/* if the preceding logic block failed, then return a -1 code to the outer section */
	if (!inIfBlock) {
	  if ( tokenList ) {
	    delete[] tokenList;
	    tokenList = 0;
	  }
	  if ( dirList ) {
	    delete[] dirList;
	    dirList = 0;
	  }
	  return -1;
	}
	      
	/****************************/
	/* end of "ELSE" directive */
	/****************************/
      
      } else if (dirList[0] == "ENDIF") {
      
	/* example: #ENDIF */

	/********************************************************************************/
	/* if inIfBlock is set then an IF block has been visited in the current section */
	/* and the current ENDIF should be ignored because it is closing an enclosed IF */
	/* block.                                                                       */
	/********************************************************************************/
	if (inIfBlock) {
	  inIfBlock--;
	} else {

	  //if (skip) {
	  if ( tokenList ) {
	    delete[] tokenList;
	    tokenList = 0;
	  }
	  if ( dirList ) {
	    delete[] dirList;
	  dirList = 0;
	  }
	  return 0;
	  //}
	}
      
	/****************************/
	/* end of "ENDIF" directive */
	/****************************/
	
      }
    
      if (tokenList) {
	delete[] tokenList;
	tokenList = 0;
      }
    
      if (dirList) {
	delete[] dirList;
	dirList = 0;
      }
    
      done = 0;
      break;
    
    default:
    
      switch (sec) {
      
      case game_data_section:
	done = !game->ReadData(file, nameList, valueList, nList);
	break;
      
      case race_data_section:
	done = !game->AddRace(file, nameList, valueList, nList);
	break;
      
      case rank_data_section:
	done = !game->AddRank(file, nameList, valueList, nList);
	break;
      
      case player_data_section:
	done = !game->AddPlayer(file, nameList, valueList, nList);
	break;
      
      case sound_data_section:
	done = !game->AddSound(file, nameList, valueList, nList);
	break;
      
      case sound_event_section:
	done = !game->AddSoundEvent(file, nameList, valueList, nList);
	break;
      
      case ship_bitmap_section:
	done = !game->AddBitmap(file, "ship", nameList, valueList, nList);
	break;
      
      case ship_design_section:
	done = !game->AddShipDesign(file, nameList, valueList, nList);
	break;
      
      case ship_data_section:
	done = !game->AddShip(file, nameList, valueList, nList);
	break;
      
      case missile_bitmap_section:
	done = !game->AddBitmap(file, "missile", nameList, valueList, nList);
	break;
      
      case missile_design_section:
	done = !game->AddMissileDesign(file, nameList, valueList, nList);
	break;
      
      case planet_bitmap_section:
	done = !game->AddBitmap(file, "planet", nameList, valueList, nList);
	break;
      
      case planet_data_section:
	done = !game->AddPlanet(file, nameList, valueList, nList);
	break;
      
      case module_data_section:
	done = !game->AddModule(file, nameList, valueList, nList);
	break;
      
      case colony_data_section:
	done = !game->AddColony(file, nameList, valueList, nList);
	break;
      
      case colony_bitmap_section:
	done = !game->AddBitmap(file, "colony", nameList, valueList, nList);
	break;
      
      case colony_design_section:
	done = !game->AddColonyDesign(file, nameList, valueList, nList);
	break;
      
      case explosion_bitmap_section:
	done = !game->AddBitmap(file, "explosion", nameList,
				valueList, nList);
	break;
      
      case transition_bitmap_section:
	done = !game->AddBitmap(file, "transition", nameList,
				valueList, nList);
	break;
      
      case miscellaneous_bitmap_section:
	done = !game->AddBitmap(file, "miscellaneous", nameList,
				valueList, nList);
	break;
      
      case debris_bitmap_section:
	done = !game->AddBitmap(file, "debris", nameList, valueList, nList);
	break;
      
      case skip_section:
	file.getline(line, 1024);
	break;
      
      default:
	file.getline(line, 1024);
	logFile << "ERROR> unknown data section found." << endl;
      }
    }
  
  } while (!done);

  if ( tokenList ) {
    delete[] tokenList;
    tokenList = 0;
  }
  if ( dirList ) {
    delete[] dirList;
    dirList = 0;
  }

  return done;

}

void EngineReadGameState(char *filename) {
  char line[256];
  ifstream file;
  
  file.open(filename);

  numPlanetRobotsSet = 0;
  numPopRobotsSet = 0;
  
  char* del = " \t";
  SFString* tokenList = 0;
  SFString sLine;

  while ( file.getline(line, 256) ) {

    sLine = line;

    /* break line into tokens */

    if (tokenList) {
      delete[] tokenList;
      tokenList = 0;
    }

    short nTokens = 0;
    if (!sLine.IsNull())
      nTokens = sLine.TokenList(tokenList, del);

    if (nTokens) {
      if (tokenList[0] == "*") {
	/* do nothing -- comment line */
      } else if (tokenList[0] == "game") {
	ReadGameData(game_data_section, nullString, file);
      } else if (tokenList[0] == "race") {
	ReadGameData(race_data_section, nullString, file);
      } else if (tokenList[0] == "rank") {
        ReadGameData(rank_data_section, nullString, file);
      } else if (tokenList[0] == "robot") {
        ReadGameData(player_data_section, nullString, file);
      } else if (tokenList[0] == "sound") {
	if (tokenList[1] == "data") {
	  ReadGameData(sound_data_section, nullString, file);
	} else if (tokenList[1] == "events") {
	  ReadGameData(sound_event_section, nullString, file);
	}
      } else if (tokenList[0] == "ship") {
	if (tokenList[1] == "bitmaps") {
	  game->shipBitmapIndexStart = game->N(GameStateClass::bitmaps);
	  ReadGameData(ship_bitmap_section, nullString, file);
	  game->shipBitmapIndexEnd = game->N(GameStateClass::bitmaps) - 1;
	} else if (tokenList[1] == "designs") {
	  ReadGameData(ship_design_section, nullString, file);
	} else if (tokenList[1] == "data") {
	  ReadGameData(ship_data_section, nullString, file);
	}
      } else if (tokenList[0] == "missile") {
	if (tokenList[1] == "bitmaps") {
	  game->missileBitmapIndexStart = game->N(GameStateClass::bitmaps);
	  ReadGameData(missile_bitmap_section, nullString, file);
	  game->missileBitmapIndexEnd = game->N(GameStateClass::bitmaps) - 1;
	} else if (tokenList[1] == "designs") {
	  ReadGameData(missile_design_section, nullString, file);
	}
      } else if (tokenList[0] == "planet") {
	if (tokenList[1] == "bitmaps") {
	  game->planetBitmapIndexStart = game->N(GameStateClass::bitmaps);
	  ReadGameData(planet_bitmap_section, nullString, file);
	  game->planetBitmapIndexEnd = game->N(GameStateClass::bitmaps) - 1;
	} else if (tokenList[1] == "data") {
	  ReadGameData(planet_data_section, nullString, file);
	}
      } else if (tokenList[0] == "module") {
	if (tokenList[1] == "data") {
	  ReadGameData(module_data_section, nullString, file);

	}
      } else if (tokenList[0] == "colony") {
	if (tokenList[1] == "data") {

	  ReadGameData(colony_data_section, nullString, file);

	} else if (tokenList[1] == "bitmaps") {
	  game->colonyBitmapIndexStart = game->N(GameStateClass::bitmaps);
	  ReadGameData(colony_bitmap_section, nullString, file);
	  game->colonyBitmapIndexEnd = game->N(GameStateClass::bitmaps) - 1;
	} else  if (tokenList[1] == "designs") {
	  ReadGameData(colony_design_section, nullString, file);
	}
      } else if (tokenList[0] == "explosion") {
	if (tokenList[1] == "bitmaps") {
	  game->explosionBitmapIndexStart = game->N(GameStateClass::bitmaps);
	  ReadGameData(explosion_bitmap_section, nullString, file);
	  game->explosionBitmapIndexEnd = game->N(GameStateClass::bitmaps) - 1;
	}
      } else if (tokenList[0] == "transition") {
	if (tokenList[1] == "bitmaps") {
	  game->transitionBitmapIndexStart = game->N(GameStateClass::bitmaps);
	  ReadGameData(transition_bitmap_section, nullString, file);
	}
      } else if (tokenList[0] == "miscellaneous") {
	if (tokenList[1] == "bitmaps") {
	  game->miscellaneousBitmapIndexStart =
	    game->N(GameStateClass::bitmaps);
	  ReadGameData(miscellaneous_bitmap_section, nullString, file);
	}
      } else if (tokenList[0] == "debris") {
	if (tokenList[1] == "bitmaps") {
	  game->debrisBitmapIndexStart = game->N(GameStateClass::bitmaps);
	  ReadGameData(debris_bitmap_section, nullString, file);
	}
      }

      if (tokenList) {
	delete[] tokenList;
	tokenList = 0;
      }
    }
  }
  file.close();

  if (tokenList) {
    delete[] tokenList;
    tokenList = 0;
  }
  
}

void InitDir() {
  const double pi = 3.141592654;
  slong x,y;

  if (Dir) {
    delete[] Dir;
    Dir = 0;
  }
  Dir = new CoordinateClass[game->maxAngles];

  short i;
  for(i = 0; i< game->maxAngles; i++) {
    x = nlong(-sin(i*2*pi/game->maxAngles) * sub_scale);
    y = nlong( cos(i*2*pi/game->maxAngles) * sub_scale);
    Dir[i] = CoordinateClass(x,y);
  }

  /* resconstruct all global bitmap direction arrays */
  NoObjectBitmap.init();

}

short EngineConstructGameState(char *gameStateFilename,char *bitmapsFilename) {
  
  if (game)
    delete game;
  
  game = new GameStateClass(max_players, 0, 0, 0, 0,
			    0, 0, 0, 0, 0, 0, 0, 0, max_missiles,
			    0,
			    max_ship_designs, max_missile_designs,
			    max_modules, max_races, max_stored_players,
			    num_extra_colonies, 0);

  strcpy(game->fileName, gameStateFilename);
  
  /* obtain a random seed value */
  time_t ltime;
  time(&ltime);
  
  game->randomSeed = 0/*gtime->tm_sec + gtime->tm_min + gtime->tm_hour*/;
  
  //DumpSizeData();

  InitDir();

  EngineReadGameState(gameStateFilename);

  /* apply module defaults */
  game->ApplyModuleDefaults();

  /* set default values */
  game->SetDefaults();
  
  short i;
  for (i = 0; i < game->N(GameStateClass::stored_players); i++)
    game->MapPlayerShipIdToIndex(i);
  
  game->MapObjectList();
  
  for (i=0; i < game->N(GameStateClass::races); i++) 
    game->planet[game->race[i].home].raceID = i;
  
  short j;
  for (i=0; i<game->N(GameStateClass::statics) + 
	 game->N(GameStateClass::colonies); i++ ) {

    if (i >= game->N(GameStateClass::first_ship) && 
	i <= game->N(GameStateClass::last_ship) ) {
      j = i - game->N(GameStateClass::first_ship);

      if (j >= game->N(GameStateClass::human_ships)) {

	if (!game->ship[j].IsActive()) {

	  game->ship[j].initializeWith(j, j-game->N(GameStateClass::human_ships));
	  game->playerList[j] = game->player + game->ship[j].playerIndex;
	  game->CheckRank(game->ship[j].playerIndex, game->ship +
			  (j - game->N(GameStateClass::first_ship)));
	}

      } else {

	game->ship[j].initializeWith(j, 0);
	game->ship[j].deactivate();
	game->playerList[j] = game->player + game->ship[j].playerIndex;

      }
      //FILE* fptr = fopen("test.dat","a");
      //fprintf(fptr, "%d %d\n", i, game->ship[i].Assign());
      //fclose(fptr);
    } else if (i >= game->N(GameStateClass::first_planet) && 
	i <= game->N(GameStateClass::last_planet) ) {
      j = i - game->N(GameStateClass::first_planet);
      game->planet[j].activate();
    } else if (i >= game->N(GameStateClass::first_colony) && 
	       i <= game->N(GameStateClass::last_colony) ) {
      //j = i - game->N(GameStateClass::first_colony);
      
      /* initialize the new colony object */
      //if (j < game->N(GameStateClass::colonies)) {
      //	game->colony[j].InitializeFromColonyDesign(game->colony + j);
      //	game->colony[j].activate();
      //} else
      //	game->ReceiveDestroyObjectMsg(game->colony+j,0);
      
    }
  }

  for (i=0; i < game->N(GameStateClass::robot_players); i++)
    {
      game->player[i].SetStat(game->player[i].totalScore,
			      game->rank[game->player[i].rank].totalScore);
      game->player[i].SetStat(game->player[i].numKillsTotal,
			      game->rank[game->player[i].rank].totalKills);
      
      //game->player[i].totalScore =
      //  game->rank[game->player[i].rank].totalScore;
      //game->player[i].numKillsTotal =
      // game->rank[game->player[i].rank].totalKills;
    }
  
  /* initialize technology lists */
  game->InitializeTechList();

  /* flag all ship designs that have factory modules */
  game->MarkFactoryDesigns();

  //char* tgame = new char[game->Size()];
  //EngineGetGameState(tgame);
  //EngineSetGameState(tgame);
  //delete tgame;

  game->MapObjectList();

  short numErr;

  if ((numErr = game->validate()) > 0)
    return numErr;

  //game->ship[0].diff(game->ship+1);

  game->activate();

  game->Profile();

  /* dump value list to log */
  game->ProfileModuleList();

  char htmlFilename[256];
  char dataFilename[256];
  char logFilename[256];
  strcpy(htmlFilename, gameStateFilename);

  i = strlen(htmlFilename);
  while ( htmlFilename[i] != '.' && i >= 0 )
    i--;
  htmlFilename[i] = 0;
  //*strstr(htmlFilename, ".dat") = '\0';

  strcpy(dataFilename, htmlFilename);
  strcpy(logFilename, htmlFilename);
  strcat(htmlFilename, ".html");
  if (IsServer()) {
    strcat(logFilename, ".server-log");
  } else {
    strcat(logFilename, ".log");
  }
  //strcat(dataFilename, ".dat");

#if defined(WIN32) || defined(OS2)
  strcpy(dataFilename, "..\\dat\\AutoGenScenario.dat");
#else
  strcpy(dataFilename, "../dat/AutoGenScenario.dat");
#endif


  /* close out the current log file and open a new one with the same */
  /* name as the scenario file                                       */
  logFile.close();
  logFile.open(logFilename);

  //eventFile.open("event.log");

  /* write out html file */
  //game->WriteFile(html_file, htmlFilename);
  //game->WriteFile(data_file, dataFilename);

#if defined(SF_DEVMODE)
  game->player[game->ship[localSlotNumber].playerIndex].SetDeveloperMode();
#endif  

  return 0;
}


/* GameStateClass additions for writing game state data to a file */
void GameStateClass::WriteFile(int fileType, char* fileName) {
  short i;
  ofstream fout;
  Document* doc = NULL;

  /* preamble */
  switch (fileType) {
  case data_file:
    doc = new DataDocument(fileName);
    break;
  case html_file:
    doc = new HTMLDocument(fileName);
    break;
  }

  doc->OpenBody();
  if (doc->type() == html_file)
    doc->WriteLine("<basefont face=\"arial,sans-serif\" size=2>");

  WriteData(doc);

  short tableOpen = 0;
  for (i=0; i<numBitmaps; i++) {
    if (i == shipBitmapIndexStart) {
      if (tableOpen) {
	doc->CloseTable();
	doc->WriteLine("}");
      }
      doc->WriteLine("ship bitmaps {");
      doc->OpenTable();
      tableOpen = 1;
    } else if (i == colonyBitmapIndexStart) {
      if (tableOpen) {
	doc->CloseTable();
	doc->WriteLine("}");
      }
      doc->WriteLine("colony bitmaps {");
      doc->OpenTable();
      tableOpen = 1;
    } else if (i == missileBitmapIndexStart) {
      if (tableOpen) {
	doc->CloseTable();
	doc->WriteLine("}");
      }
      doc->WriteLine("missile bitmaps {");
      doc->OpenTable();
      tableOpen = 1;
    } else if (i == planetBitmapIndexStart) {
      if (tableOpen) {
	doc->CloseTable();
	doc->WriteLine("}");
      }
      doc->WriteLine("planet bitmaps {");
      doc->OpenTable();
      tableOpen = 1;
    } else if (i == explosionBitmapIndexStart) {
      if (tableOpen) {
	doc->CloseTable();
	doc->WriteLine("}");
      }
      doc->WriteLine("explosion bitmaps {");
      doc->OpenTable();
      tableOpen = 1;
    } else if (i == transitionBitmapIndexStart) {
      if (tableOpen) {
	doc->CloseTable();
	doc->WriteLine("}");
      }
      doc->WriteLine("transition bitmaps {");
      doc->OpenTable();
      tableOpen = 1;
    } else if (i == debrisBitmapIndexStart) {
      if (tableOpen) {
	doc->CloseTable();
	doc->WriteLine("}");
      }
      doc->WriteLine("debris bitmaps {");
      doc->OpenTable();
      tableOpen = 1;
    } else if (i == miscellaneousBitmapIndexStart) {
      if (tableOpen) {
	doc->CloseTable();
	doc->WriteLine("}");
      }
      doc->WriteLine("miscellaneous bitmaps {");
      doc->OpenTable();
      tableOpen = 1;
    }
    objectBitmapList[i].put(doc);
  }
  doc->CloseTable();
  doc->WriteLine("}");
  
  doc->WriteLine("planet data {");
  doc->OpenTable(" BORDER CELLSPACING=0 CELLPADDING=10 WIDTH=\"100%\" BGCOLOR=\"#FFFFCC\"");
  for (i=0; i<numPlanets; i++)
    planet[i].put(doc);
  doc->CloseTable();
  doc->WriteLine("}");

  doc->WriteLine("module data {");
  doc->OpenTable(" BORDER CELLSPACING=0 CELLPADDING=10 WIDTH=\"100%\" BGCOLOR=\"#FFFFCC\"");
  for (i=0; i<numModules; i++)
    moduleList[i].put(doc);
  doc->CloseTable();
  doc->WriteLine("}");

  doc->WriteLine("robot data {");
  doc->OpenTable(" BORDER CELLSPACING=0 CELLPADDING=10 WIDTH=\"100%\" BGCOLOR=\"#FFFFCC\"");
  for (i=0; i<numStoredPlayers; i++)
    player[i].put(doc);
  doc->CloseTable();
  doc->WriteLine("}");

  doc->WriteLine("race data {");
  doc->OpenTable(" BORDER CELLSPACING=0 CELLPADDING=10 WIDTH=\"100%\" BGCOLOR=\"#FFFFCC\"");
  for (i=0; i<numRaces; i++)
    race[i].put(doc);
  doc->CloseTable();
  doc->WriteLine("}");

  doc->WriteLine("rank data {");
  doc->OpenTable(" BORDER CELLSPACING=0 CELLPADDING=10 WIDTH=\"100%\" BGCOLOR=\"#FFFFCC\"");
  for (i=0; i<numRanks; i++)
    rank[i].put(doc);
  doc->CloseTable();
  doc->WriteLine("}");

  doc->WriteLine("ship designs {");
  doc->OpenTable(" BORDER CELLSPACING=0 CELLPADDING=10 WIDTH=\"100%\" BGCOLOR=\"#FFFFCC\"");
  for (i=0; i<numShipDesigns; i++)
    shipDesign[i].put(::ship, doc);
  doc->CloseTable();
  doc->WriteLine("}");

  doc->WriteLine("missile designs {");
  doc->OpenTable(" BORDER CELLSPACING=0 CELLPADDING=10 WIDTH=\"100%\" BGCOLOR=\"#FFFFCC\"");
  for (i=0; i<numMissileDesigns; i++)
    missileDesign[i].put(doc);
  doc->CloseTable();
  doc->WriteLine("}");

  doc->WriteLine("colony data {");
  doc->OpenTable(" BORDER CELLSPACING=0 CELLPADDING=10 WIDTH=\"100%\" BGCOLOR=\"#FFFFCC\"");
  for (i=0; i<numColonies; i++)
    colonyList[i]->put(doc);
  doc->CloseTable();
  doc->WriteLine("}");

  doc->CloseBody();
  
  delete doc;
}

void GameStateClass::WriteData(class Document* doc) {

  if (doc->type() == data_file)
    doc->WriteLine("game data {");
  else
    doc->WriteLine("server settings");

  doc->OpenTable(" BORDER CELLSPACING=0 CELLPADDING=10 WIDTH=\"100%\" BGCOLOR=\"#FFFFCC\"");
  doc->OpenRow();
  doc->WriteCell("name");

  doc->WriteCell(SFString(scenarioName) + "(autoGen)");

  doc->CloseRow();
  doc->OpenRow();
  doc->WriteCell("desc");
  doc->WriteCell(scenarioDesc);
  doc->CloseRow();
  doc->OpenRow();
  doc->WriteCell("year");
  doc->WriteCell(year);
  doc->CloseRow();
  doc->OpenRow();
  doc->WriteCell("players");
  doc->WriteCell(numShips);
  doc->CloseRow();
  doc->OpenRow();
  doc->WriteCell("robots");
  doc->WriteCell(numShips - maxHumanShips);
  doc->CloseRow();
  doc->OpenRow();
  doc->WriteCell("max_acc");
  doc->WriteCell(maxAcc);
  doc->CloseRow();
  doc->OpenRow();
  doc->WriteCell("ship_g");
  doc->WriteCell(planetShipGFactor);
  doc->CloseRow();
  doc->OpenRow();
  doc->WriteCell("planet_g");
  doc->WriteCell(planetPlanetGFactor);
  doc->CloseRow();
  doc->OpenRow();
  doc->WriteCell("damage_mass");
  doc->WriteCell(damageToMass);
  doc->CloseRow();
  doc->OpenRow();
  doc->WriteCell("robot_population_total");
  doc->WriteCell(numPopRobots);
  doc->CloseRow();
  doc->OpenRow();
  doc->WriteCell("robot_population_rate");
  doc->WriteCell(popPercentPerRobot);
  doc->CloseRow();
  doc->OpenRow();
  doc->WriteCell("robot_planet_total");
  doc->WriteCell(numPlanetRobots);
  doc->CloseRow();
  doc->OpenRow();
  doc->WriteCell("robot_planet_rate");
  doc->WriteCell(planetsPerRobot);
  doc->CloseRow();
  doc->OpenRow();
  doc->WriteCell("debris_count");
  doc->WriteCell(debrisCount);
  doc->CloseRow();
  doc->OpenRow();
  doc->WriteCell("extra_colonies");
  doc->WriteCell(extraColonies);
  doc->CloseRow();
  doc->OpenRow();
  doc->WriteCell("light_speed");
  doc->WriteCell(lightSpeed * frameRate / sub_scale);
  doc->CloseRow();
  doc->CloseTable();
  if (doc->type() == data_file)

    doc->WriteLine("}");

}

void GameBitmapClass::put(Document* doc) {
  static char seqT[max_name_char];

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

  if (doc->type() == html_file)
    return;

  doc->OpenRow();
  doc->WriteCell("bitmap");
  doc->WriteCell(raceName);
  doc->WriteCell(name);
  doc->WriteCell(file);
  doc->WriteCell(numFrames);
  doc->WriteCell(size);

  switch (seq) {
  case ObjectBitmapClass::spin:
    strcpy(seqT,"spin");
    break;
  case ObjectBitmapClass::time:
    strcpy(seqT,"time");
    break;
  case ObjectBitmapClass::stationary:
    strcpy(seqT,"stationary");
    break;
  case ObjectBitmapClass::terminal:
    strcpy(seqT,"terminal");
    break;
  case ObjectBitmapClass::transitional:
    strcpy(seqT,"transitional");
    break;
  }

  doc->WriteCell(seqT);
  doc->WriteCell(alpha);
  doc->WriteCell(gamma);
  doc->CloseRow();

}

void PopStruct::put(Document* doc) {

  if (doc->type() == data_file) {
    double d = billions * double(long_billion) + lBillions;
    doc->WriteCell(d, "%10.0f");
  } else if (doc->type() == html_file) {
    doc->WriteCell(print());
  } 
}

void ColonyClass::put(Document* doc) {

  doc->OpenRow();

  if (doc->type() == data_file) {
  } else if (doc->type() == html_file) {
  }
  
  doc->WriteCell(name);
  doc->WriteCell(game->planet[planetIndex].name);

  pop.put(doc);

  design->DesignStruct::put(colony, doc);

  doc->CloseRow();

}

/*** object class additions for writing data to a stream */
void ShipDesignStruct::put(ObjectType ot, Document* doc) {


  doc->OpenRow();

  if (doc->type() == data_file) {

    doc->WriteCell(game->race[raceID].name);
    doc->WriteCell(name);
    /* the inFrame parameter is the frame number on which the design becomes */
    /* available for use.  In the game state file is stored in units of      */
  /* minutes, so convert it */
    doc->WriteCell(inFrame / 60 / frameRate);
    doc->WriteCell(reqRank);
    
  } else if (doc->type() == html_file) {

    doc->WriteCell(game->race[raceID].name);
    doc->WriteCell(name);
    /* the inFrame parameter is the frame number on which the design becomes */
    /* available for use.  In the game state file is stored in units of      */
    /* minutes, so convert it */
    doc->WriteCell(inFrame / 60 / frameRate);
    doc->WriteCell(game->rank[reqRank].name);
    
  }

  /* put the rest of it -- this is shared with ColonyClass */
  DesignStruct::put(ot, doc);
  doc->CloseRow();
}

void DesignStruct::put(ObjectType ot, Document* doc) {
  char busString[1024];
  char idString[16];

  if (doc->type() == data_file) {
    doc->WriteCell(maxSpeed / double(game->lightSpeed), "%5.2f");
    
    short bi = 0;

    switch (ot) {
    case ship:
      bi = bitmapIndex - game->shipBitmapIndexStart + 1;
      break;
    case missile:
      bi = bitmapIndex - game->missileBitmapIndexStart + 1;
      break;
    case colony:
      bi = bitmapIndex - game->colonyBitmapIndexStart + 1;
      break;
    default :
      break;
    }
    
    doc->WriteCell(bi);
    doc->WriteCell(expBitmapIndex - game->explosionBitmapIndexStart + 1);
    doc->WriteCell(id);
    doc->WriteCell(raceID);
    doc->WriteCell(volumeCap);
    doc->WriteCell(bus.numSlots);

    sprintf(busString, "(%ld", bus.moduleIdList[0]);
    short i,j;
    for (i=1; i<bus.numMod; i++) {
      sprintf(idString, ",%ld", bus.moduleIdList[i]);
      strcat(busString, idString);
      for (j=max_module_spar-1; j >= 0; j--) {
	if (bus.moduleParList[max_module_spar*i+j])
	  break;
      }
      if (j || bus.moduleParList[max_module_spar*i]) {
	short jmax = j;
	for (j=0; j<=jmax; j++) {
	  sprintf(idString, "*%ld", bus.moduleParList[max_module_spar*i+j]);
	  strcat(busString, idString);
	}
      }
    }
    strcat(busString, ")");

    doc->WriteCell(busString);
  } else if (doc->type() == html_file) {

    doc->WriteCell(maxSpeed / double(game->lightSpeed), "%5.2f c");
    doc->WriteCell(id);
    doc->WriteCell(volumeCap);
    doc->WriteCell(bus.numSlots);

    short i;
    ModuleClass* m;
    for (i=0; i<bus.numMod; i++) {
      m = game->moduleList + game->LookUpModuleById(bus.moduleIdList[i]);
      strcpy(idString, m->name);
      doc->WriteToCell(idString);
    }

    doc->WriteCell("");
  }
}
void MissileDesignStruct::put(Document* doc) {

  doc->OpenRow();

  if (doc->type() == data_file) {

    doc->WriteCell(game->race[raceID].name);
    doc->WriteCell(name);
    doc->WriteCell(warhead);
    doc->WriteCell(initialVelocityDisplacement * sub_scale /
		   double(game->lightSpeed), "%5.2f");
    
    switch (tracking) {
    case dumb:
      doc->WriteCell("dumb");
      break;
    case bomb:
      doc->WriteCell("bomb");
      break;
    case smart:
      doc->WriteCell("smart");
      break;
    }
    
    if (trigger & on_ship && trigger & on_planet)
      doc->WriteCell("impact");
    else if (trigger & on_ship)
      doc->WriteCell("ship");
    else if (trigger & on_planet)
      doc->WriteCell("planet");
    else if (trigger & on_missile)
      doc->WriteCell("missile");
    else if (trigger & on_proximity)
      doc->WriteCell("prox");
    else if (trigger & on_time)
      doc->WriteCell("time");
    else if (trigger & on_heat)
    doc->WriteCell("heat");
    else if (trigger)
      doc->WriteCell("none");
    
    doc->WriteCell(seekSens);
    
    doc->WriteCell(actRange/sub_scale);
    
    if (camera)
      doc->WriteCell("Y");
    else
      doc->WriteCell("N");

  } else if (doc->type() == html_file) {

    doc->WriteCell(game->race[raceID].name);
    doc->WriteCell(name);
    doc->WriteCell(warhead);
    doc->WriteCell(initialVelocityDisplacement * sub_scale /
		   double(game->lightSpeed), "%5.2f c");
    
    switch (tracking) {
    case dumb:
      doc->WriteCell("dumb");
      break;
    case bomb:
      doc->WriteCell("bomb");
      break;
    case smart:
      doc->WriteCell("smart");
      break;
    }
    
    if (trigger & on_ship && trigger & on_planet)
      doc->WriteCell("impact");
    else if (trigger & on_ship)
      doc->WriteCell("ship");
    else if (trigger & on_planet)
      doc->WriteCell("planet");
    else if (trigger & on_missile)
      doc->WriteCell("missile");
    else if (trigger & on_proximity)
      doc->WriteCell("proximity");
    else if (trigger & on_time)
      doc->WriteCell("time");
    else if (trigger & on_heat)
    doc->WriteCell("heat");
    else if (trigger)
      doc->WriteCell("none");
    
    doc->WriteCell(actRange/sub_scale);
    
    if (camera)
      doc->WriteCell("Y");
    else
      doc->WriteCell("N");

  }
    
  DesignStruct::put(missile, doc);

  doc->CloseRow();

}

void RankStruct::put(Document* doc) {

  char dutyStr[16];

  doc->OpenRow();
  doc->WriteCell(name);
  doc->WriteCell(game->rank.indexOf(this));
  doc->WriteCell(activeScore);
  doc->WriteCell(totalScore);

  strcpy(dutyStr, detailName[duty]);

  doc->WriteCell(dutyStr);

  doc->WriteCell(dutyInterval);
  doc->WriteCell(accBonus);
}

void RaceClass::put(Document* doc) {
  doc->OpenRow();

  if (doc->type() == data_file) {

    doc->WriteCell(name);
    doc->WriteCell(id);
    doc->WriteCell(game->planet[home].name);
    
    /* convert the growth rate to a doubling time */
    slong dt = nlong(log((double)2) / log((double)(growthRate / 1e5)) / 60); /* in seconds */
    
    doc->WriteCell(dt);
    
    char debOff[16];
    
    slong d = debrisOffset / game->debrisCount;
    if (d && debrisOffset / d == game->debrisCount)
      sprintf(debOff, "%ld*", d);
    else
      sprintf(debOff, "%d", debrisOffset);
    
    doc->WriteCell(debOff);
    doc->WriteCell(prefTemp);
    doc->WriteCell(prefAtmP/100., "%6.2f");
    doc->WriteCell(prefGrav/100., "%6.2f");
    doc->WriteCell(ProfileCondition());

  } else if (doc->type() == html_file) {

    doc->WriteCell(name);
    doc->WriteCell(id);
    doc->WriteCell(game->planet[home].name);
    
    /* convert the growth rate to a doubling time */
    slong dt = nlong(log((double)2) / log((double)(growthRate / 1e5)) / 60); /* in seconds */
    
    doc->WriteCell(dt);
    
    doc->WriteCell(prefTemp);
    doc->WriteCell(prefAtmP/100., "%6.2f");
    doc->WriteCell(prefGrav/100., "%6.2f");

  }

  doc->CloseRow();
}

void PlayerClass::put(Document* doc) {

  doc->OpenRow();

  if (doc->type() == data_file) {

    doc->WriteCell(name);
    doc->WriteCell(login);
    doc->WriteCell(rank);
    doc->WriteCell(raceId);
    doc->WriteCell(shipId);
    doc->WriteCell(shipName);
    doc->WriteCell(numMissions);
    doc->WriteCell(numKills);
  
  } else if (doc->type() == html_file) {

    doc->WriteCell(name);
    doc->WriteCell(login);
    doc->WriteCell(game->rank[rank].name);
    doc->WriteCell(game->race[raceId].name);
    doc->WriteCell(game->shipDesign[shipIndex].name);
    doc->WriteCell(shipName);
    doc->WriteCell(numMissions);
    doc->WriteCell(numKills);
  
  }

  doc->CloseRow();

}

void ModuleClass::put(Document* doc) {
  
  char func[2*max_name_char];
  short i;

  if (doc->type() == data_file) {
    doc->OpenRow();
    doc->WriteCell(game->race[raceID].name);
    if (moduleFunc != tube) {
      sprintf(func, "%s:%ld", funcName[moduleFunc], id%10);
      if (strcmp(func, name) == 0)
	strcpy(func, "*");
      else
	strcpy(func, name);
    } else {
      if (strcmp(name, game->missileDesign[short(par[3])].name) == 0)
	strcpy(func,"*");
      else
	strcpy(func, name);
    }

    doc->WriteCell(func);
    doc->WriteCell(volume); 
    doc->WriteCell(mass);
    doc->WriteCell(power);
    doc->WriteCell(damageCap);

    strcpy(func, ModuleFunctionName[moduleFunc]);

    short imax;
    for (imax=max_module_par-1; imax >= 0; imax--) {
      if (par[imax])
	break;
    }

    if (imax || par[0]) {
      for (i=0; i<=imax; i++)
	if (par[i]) {
	  /* some of the parameters need to be reset to their original values */
	  if (i == 0) {
	    if (moduleFunc == qdrv)
	      sprintf(func, "%s*%ld", func, par[i] / game->lightSpeed); 
	    else if (moduleFunc == engine)
	      sprintf(func, "%s*%ld", func, par[i] * g_scale / unit_thrust);
	    else
	      sprintf(func, "%s*%ld", func, par[i]); 
	  } else if (i == 1 && moduleFunc == dock) {
	      sprintf(func, "%s*%ld", func, par[i] * g_scale / unit_thrust); 
	  } else if (i == 3 && moduleFunc == tube) {

	    /* the par[3] parameter contains the missile id -- it is */
	    /* remapped to the missile index in                      */
	    /* GameStateClass::ApplyModuleDefaults                   */
	    sprintf(func, "%s*%ld", func,
		    game->missileDesign[short(par[i])].id); 

	  } else
	    sprintf(func, "%s*%ld", func, par[i]); 
	}
    }

    doc->WriteCell(func);
    doc->WriteCell(id);
    doc->CloseRow();
  } else if (doc->type() == html_file) {

    doc->OpenRow();
    doc->WriteCell(name);
    doc->WriteCell(game->race[raceID].name);
    doc->WriteCell(funcName[moduleFunc]);
    doc->WriteCell(volume); 
    doc->WriteCell(mass);
    doc->WriteCell(power);
    doc->WriteCell(damageCap);

    switch (moduleFunc) {
    case tube:
      sprintf(func, "capacity: %ld missiles", par[2]);
      doc->WriteToCell(func);
      sprintf(func, "burst: %ld missiles", TubeBurst());
      doc->WriteToCell(func);
      sprintf(func, "repeat rate: %5.2f missiles/sec",
	      TubeRepeatRate(frameRate));
      doc->WriteToCell(func);
      break;
    case brake:
      sprintf(func, "braking factor: %ld%%", par[0]);
      doc->WriteToCell(func);
      break;
    case power_plant:
      doc->WriteToCell("--");
      break;
    case engine:
      sprintf(func, "acceleration: %5.2f pix/s/s",
	      par[0] / float(typical_ship_mass) * frameRate * frameRate);
      doc->WriteToCell(func);
      break;
    case qdrv:
      sprintf(func, "velocity: %4.1f c",
	      par[0] / float(game->lightSpeed));
      doc->WriteToCell(func);
      break;
    case shield:
      sprintf(func, "recharge time: %ld sec", par[0] /
	      frameRate);
      doc->WriteToCell(func);
      break;
    case sensor:
      sprintf(func, "effective range: %ld pixels",
	      (((10 + par[0]) * typical_ship_mass) /
	       sensor_threshold - 1) * sensor_dist / sub_scale);
      doc->WriteToCell(func);
      break;
    case counter_measures:
      sprintf(func, "recharge time: %ld sec", par[0] / frameRate);
      doc->WriteToCell(func);
      sprintf(func, "range: %ld pixels", par[1]);
      doc->WriteToCell(func);
      break;
    case cloak:
      sprintf(func, "discharge/recharge time: %ld sec", par[0] /
	      frameRate);
      doc->WriteToCell(func);
      break;
    case boost:
      sprintf(func, "discharge/recharge time: %ld sec", par[0] /
	      frameRate);
      doc->WriteToCell(func);
      break;
    case dock:
      sprintf(func, "discharge/recharge time: %ld sec", par[0] / frameRate);
      doc->WriteToCell(func);
      sprintf(func, "range: %ld pixels", par[2]);
      doc->WriteToCell(func);
     break;
    }
    doc->WriteCell("");
    doc->CloseRow();
  }
}

void PlanetClass::put(Document* doc) {

  const double pi = 3.141592654;

  char Class[max_name_char];
  double rad = 0.0;
  double dmass = 0.0;

  switch (minor) {
  case -1:
    strcpy(Class, "Star");
    dmass = mass / 3e7;
    rad = double(orbitalRadius) / double(au_to_pixel);
    break;
  case 0:
    strcpy(Class, "Planet");
    dmass = mass / 1000.0;
    rad = double(orbitalRadius) / au_to_pixel;
    break;
  case 1:
    strcpy(Class, "Moon");
    dmass = mass / 7.3e22 * 6e24 / 1000;
    rad = double(orbitalRadius) / mo_to_pixel;
    break;
  case 2:
    strcpy(Class, "Minor");
    dmass = mass / 1000.0;
    rad = double(orbitalRadius) / au_to_pixel;
    break;
  }

  doc->OpenRow();

  if (doc->type() == data_file) {
    
    doc->WriteCell(Class);
    doc->WriteCell(name);
    doc->WriteCell(rad, "%5.2f");
    doc->WriteCell(double(iPhase) / 10000.0 * 180.0 / pi, "%7.2f");
    doc->WriteCell(dmass, "%5.2f");
    doc->WriteCell(atmP / 100.0, "%12.3f");
    doc->WriteCell(bitmap.index-game->planetBitmapIndexStart+1);
    doc->WriteCell(slong(temp));
    doc->WriteCell(slong(hydpc));
    doc->WriteCell(grav / 100.0, "%12.3f");
    doc->WriteCell(revertRaceIndex);
    doc->WriteCell(accBonus);
    doc->WriteCell(terraForm / terraform_base);

  } else {
    /* HTML */
    doc->WriteCell(Class);
    doc->WriteCell(name);
    doc->WriteCell((slong)rad);
    doc->WriteCell((slong)dmass);
    doc->WriteCell(slong(hydpc));
    doc->WriteCell(atmP / 100.0, "%12.3f");
    doc->WriteCell(slong(temp));
    doc->WriteCell(grav / 100.0, "%12.3f");
  }

  doc->CloseRow();

}


void Parse(SFString& name, SFString* nameList, SFString* valueList, short nList, short test) {
  
  short i;
  for (i=0; i<nList; i++) {
    if (name.Index(nameList[i])) {
      if (!valueList[i].IsNull())
	name = name.subs(nameList[i], valueList[i]);
      else if (test)
	name = name.subs(nameList[i], "TRUE");
    }
  }

}

void Parse(char* name, SFString* nameList, SFString* valueList, short nList, short test) {
  
  SFString sName = name;
  char* out;
  
  Parse(sName, nameList, valueList, nList, test);

  out = sName.out();

  strcpy(name, out);

  if (out)
    delete out;

}
