/* SF_PortMissing.cpp -- NEW FILE, added 2026-10-12 for the Stellar Frontier
   Godot port.

   SF_ChatConsole::HandleDevCommands() is declared in SF_ChatConsole.h and
   called from SF_ChatConsole::KeyPressed() for every line typed into the
   chat console, but its definition is not part of Stardock's shared source
   release (the developer chat commands were left out).  The released server
   still answers SF_MSG_REQUESTDEV and the client still prints the "/kick"
   help when it is granted dev status.

   Until the dev commands are reimplemented (see the bug-fix task for the
   "DEV hack"), no chat line is treated as a dev command: the function
   returns FALSE, so the text is sent as an ordinary chat message. */
#include "sf.h"
#include "SF_ChatConsole.h"

int
SF_ChatConsole::HandleDevCommands(char *)
{
  return FALSE;
}
