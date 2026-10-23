/* sf_music.h -- NEW FILE (2026-10-12), Stellar Frontier Godot port.
   MIDI music for the original programs.  On Windows the songs play through
   the Windows MIDI synthesizer (MCI), as they did in 1997; the MCI calls run
   on their own thread because opening a MIDI file can take a second.  On
   other platforms this does nothing (the Godot side may play pre-rendered
   music instead). */
#ifndef SF_MUSIC_H
#define SF_MUSIC_H

#include <string>

namespace sfmusic {
/* path as the program wrote it, relative to dir (the program's binwin) */
void play(const std::string &dir, const std::string &path, bool restart);
void stop();
void shutdown();
}

#endif
