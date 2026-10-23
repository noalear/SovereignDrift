STELLAR FRONTIER - Godot edition (Windows, 64-bit)
==================================================

This is the original Stellar Frontier program (Stardock, v1.2rc7 source
release) running inside a Godot 4 application.

HOW TO PLAY
  1. Keep all of these files and folders together. Do not install them
     under C:\Program Files, because the game saves your pilot in its own
     folder.
  2. Double-click StellarFrontier.exe.
  3. Wait through the intro (about 20 seconds the first time). Then click
     "Play Game without Stellar Aces" and "Play against the computer".
  4. Pick a scenario. Then create a player. Your name must be one word,
     e.g. Pilot1. Untick "Save player data on Stardock.net", because the
     Stardock servers are gone.

  Alt+Enter     switch between a window and full screen
  Close button  quits the game, the same as Game > Exit

  The game runs at 1024x768, the size it was designed for. It is scaled up
  to fit your screen.

WHAT IS WHERE
  StellarFrontier.exe           the Godot application
  sfport.windows.x86_64.dll     its bridge to the original program
  game\                         the original program (frontier.exe,
                                server.exe, nexus.exe) and its data.
                                Your pilots are saved in game\binwin\sf.prf
  source-code.zip               the full source code of this port

  Starting the game with  StellarFrontier.exe -- -log  makes the original
  program write its own log, game\binwin\frontier.log. Godot's log is in
  %APPDATA%\Godot\app_userdata\Stellar Frontier\logs.

WHAT IS NOT DONE YET
  - Playing on the Internet or a local network has not been tested yet.
  - The Stardock.net / Nexus online services no longer exist.

LICENSE
  This software is distributed under the Stardock Shared Source Stellar
  Frontier License; a verbatim copy is included in
  stellar_frontier_shared_source_eula.txt. Non-commercial use only.
  THE SOFTWARE COMES "AS IS", WITH NO WARRANTIES, and Stardock is not
  liable for any damages related to it (see the license for the full
  terms). Original source files changed for this port carry dated notices
  saying so.

  Third-party parts: Godot Engine and godot-cpp (MIT license), zlib, the
  Liberation fonts (SIL Open Font License). Their notices are in
  third-party-notices.txt.
