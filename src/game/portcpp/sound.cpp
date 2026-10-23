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
#  include "../gui/SF_Engine.h"
#  include "../gui/SF_ClientWindow.h"
#  include "../gui/SF_Client.h"
#  include "../gui/SF_DisplayEngine.h"
#  include "../gui/SF_Engine.h"
#  include "../gui/SF_Button.h"
#endif

#include "classes.h"
#include "inc_portcpp.h"
#include "enginepp.h"


SoundClass::SoundClass() {
  initialize();
}

void SoundClass::initialize() {
  
  *name = '\0';
  *file = '\0';
  
  priority = 0;
  volume = 0;
  raceID = -1;
  loop = 0;
  objectType = all_type;

}

short SoundClass::get(ifstream& f, ostream* o) {
  char c;
  char raceStr[max_name_char], loopChar, Class[max_name_char];

  f >> c;
  if (c != '}') {
    f.putback(c);
    f >> Class >> raceStr >> name >> file >> volume >> priority >> loopChar;

    if (strcmp(Class, "game") == 0)
      objectType = game_type;
    else if (strcmp(Class, "module") == 0)
      objectType = module_type;
    else if (strcmp(Class, "ship") == 0)
      objectType = ship;
    else if (strcmp(Class, "missile") == 0)
      objectType = missile;
    else if (strcmp(Class, "colony") == 0)
      objectType = colony;

    if (loopChar == 'y' || loopChar == 'Y'||
	loopChar == '1')
      loop = 1;
    else if (loopChar == 'n' || loopChar == 'N'||
	loopChar == '0')
      loop = 0;
    else {
      logFile << "ERROR> unrecognized value for loop -- assuming \"N\" -- on " <<
	Class << ":" << name << endl;
    }

    if (strcmp(raceStr, "all"))
      raceID = game->IndexOfRace(raceStr);
    else
      raceID = -1;
    
    return (f.good());
  }

  return (c != '}' && f.good());
}

void SoundClass::put(Document* doc) const {

  if (doc->type() == html_file)
    return;

  doc->OpenRow();
  doc->WriteCell("sound");
  doc->WriteCell(game->race[raceID].name);
  doc->WriteCell(file);
  doc->WriteCell(volume);
  doc->WriteCell(priority);
  doc->WriteCell(loop);
  doc->CloseRow();

}

slong SoundClass::PushTo(class GameStateBuffer& gb, GameStateClass* g) {

  char* bufferPos = gb.BufferPosition();

  CALL_PUSH_OBJ(gb,raceID);
  CALL_PUSH_OBJ(gb,name);
  CALL_PUSH_OBJ(gb,file);
  CALL_PUSH_OBJ(gb,priority);
  CALL_PUSH_OBJ(gb,volume);
  CALL_PUSH_OBJ(gb,objectType);
  CALL_PUSH_OBJ(gb,loop);

  return gb.BufferPosition() - bufferPos;

}

slong SoundClass::PopFrom(class GameStateBuffer& gb, GameStateClass* g) {

  char* bufferPos = gb.BufferPosition();

  CALL_POP_OBJ(gb,raceID);
  CALL_POP_OBJ(gb,name);
  CALL_POP_OBJ(gb,file);
  CALL_POP_OBJ(gb,priority);
  CALL_POP_OBJ(gb,volume);
  CALL_POP_OBJ(gb,objectType);
  CALL_POP_OBJ(gb,loop);

  return gb.BufferPosition() - bufferPos;

}
