/* sf_keymap.h -- NEW FILE (2026-10-12), Stellar Frontier Godot port.
   Godot key codes to Win32 virtual keys / scan codes (see sf_keymap.cpp). */
#ifndef SF_KEYMAP_H
#define SF_KEYMAP_H

#include <godot_cpp/classes/global_constants.hpp>

bool sf_key_to_win32(godot::Key logical, godot::Key physical, godot::KeyLocation loc,
                     int *vk, int *sc, bool *ext);
/* The character a key press types, in the Windows ANSI code page; 0 if none. */
int sf_key_char(godot::Key logical, int unicode);

#endif
