STELLAR FRONTIER - Enhanced Edition (Windows, 64-bit)
=====================================================

This is the original Stellar Frontier program (Stardock, v1.2rc7 source
release) running inside a Godot 4 application, with any screen resolution
(16:9 too) and new music.

HOW TO PLAY
  1. Keep all of these files and folders together. Do not install them
     under C:\Program Files, because the game saves your pilot in its own
     folder.
  2. Double-click StellarFrontierEnhanced.exe.
  3. The launcher opens. Settings sets the resolution, full screen or
     window, the music and the volumes. Click Play.
  4. Wait through the intro (about 20 seconds the first time). Then click
     "Play against the computer", pick a scenario, and log in. A new pilot
     needs a one-word name, e.g. Pilot1, and "Save player data on
     Stardock.net" unticked, because the Stardock servers are gone.

  Alt+Enter     switch between a window and full screen
  Close button  quits the game and returns to the launcher

MUSIC (Settings > Music)
  Remixes + new tracks (shuffle)  the eight songs remixed in different
                                  genres, plus ten new tracks built from
                                  unused parts of the original songs
  Remixes                         each song replaced by its remix
  Original, high fidelity         the original songs, re-rendered
  Original MIDI                   the original songs through Windows' MIDI
  Song 8 remix                    dubstep or darkwave

  The remixes and new tracks are based on the original Stellar Frontier
  music by Stardock. They were made for this port with synthesizers written
  in code, and with orchestral samples from the VS Chamber Orchestra:
  Community Edition (CC0, public domain). The high-fidelity renders use the
  GeneralUser GS SoundFont by S. Christian Collins.

WHAT IS WHERE
  StellarFrontierEnhanced.exe   the Godot application
  sfport.windows.x86_64.dll     its bridge to the original program
  game\                         the original program (frontier.exe,
                                server.exe, nexus.exe) and its data.
                                Your pilots are saved in game\binwin\sf.prf
  game\music*                   the music recordings
  source-code.zip               the full source code of this port

WHAT IS NOT DONE YET
  - Hosting and joining network games from the launcher. The original
    "Play on local intranet" menu works with a server.exe running.
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
