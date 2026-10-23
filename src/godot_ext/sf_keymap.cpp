/* ************************************************************************* */
/* sf_keymap.cpp -- Godot keys to Win32 virtual keys and set-1 scan codes.   */
/*                                                                           */
/* NEW FILE, added 2026-10-12 for the Stellar Frontier Godot port.           */
/* The game reads both: menus use virtual keys and characters, flight       */
/* controls use scan codes (SF_Client.cpp, PushKeyboardMsg(scancode)).       */
/* ************************************************************************* */
#include "sf_keymap.h"

using namespace godot;

namespace {

struct KeyMap { Key key; int vk; int sc; bool ext; };

const KeyMap kMap[] = {
  {KEY_ESCAPE, 0x1B, 1, false},     {KEY_TAB, 0x09, 15, false},       {KEY_BACKTAB, 0x09, 15, false},
  {KEY_BACKSPACE, 0x08, 14, false}, {KEY_ENTER, 0x0D, 28, false},     {KEY_KP_ENTER, 0x0D, 28, true},
  {KEY_INSERT, 0x2D, 82, true},     {KEY_DELETE, 0x2E, 83, true},     {KEY_PAUSE, 0x13, 69, false},
  {KEY_PRINT, 0x2C, 55, true},      {KEY_SYSREQ, 0x2C, 55, true},     {KEY_CLEAR, 0x0C, 76, false},
  {KEY_HOME, 0x24, 71, true},       {KEY_END, 0x23, 79, true},        {KEY_LEFT, 0x25, 75, true},
  {KEY_UP, 0x26, 72, true},         {KEY_RIGHT, 0x27, 77, true},      {KEY_DOWN, 0x28, 80, true},
  {KEY_PAGEUP, 0x21, 73, true},     {KEY_PAGEDOWN, 0x22, 81, true},   {KEY_SHIFT, 0x10, 42, false},
  {KEY_CTRL, 0x11, 29, false},      {KEY_META, 0x5B, 91, true},       {KEY_ALT, 0x12, 56, false},
  {KEY_CAPSLOCK, 0x14, 58, false},  {KEY_NUMLOCK, 0x90, 69, true},    {KEY_SCROLLLOCK, 0x91, 70, false},
  {KEY_F1, 0x70, 59, false},  {KEY_F2, 0x71, 60, false},  {KEY_F3, 0x72, 61, false},
  {KEY_F4, 0x73, 62, false},  {KEY_F5, 0x74, 63, false},  {KEY_F6, 0x75, 64, false},
  {KEY_F7, 0x76, 65, false},  {KEY_F8, 0x77, 66, false},  {KEY_F9, 0x78, 67, false},
  {KEY_F10, 0x79, 68, false}, {KEY_F11, 0x7A, 87, false}, {KEY_F12, 0x7B, 88, false},
  {KEY_KP_MULTIPLY, 0x6A, 55, false}, {KEY_KP_DIVIDE, 0x6F, 53, true},
  {KEY_KP_SUBTRACT, 0x6D, 74, false}, {KEY_KP_PERIOD, 0x6E, 83, false},
  {KEY_KP_ADD, 0x6B, 78, false},
  {KEY_KP_0, 0x60, 82, false}, {KEY_KP_1, 0x61, 79, false}, {KEY_KP_2, 0x62, 80, false},
  {KEY_KP_3, 0x63, 81, false}, {KEY_KP_4, 0x64, 75, false}, {KEY_KP_5, 0x65, 76, false},
  {KEY_KP_6, 0x66, 77, false}, {KEY_KP_7, 0x67, 71, false}, {KEY_KP_8, 0x68, 72, false},
  {KEY_KP_9, 0x69, 73, false},
  {KEY_MENU, 0x5D, 93, true},  {KEY_SPACE, 0x20, 57, false},
  {KEY_APOSTROPHE, 0xDE, 40, false}, {KEY_COMMA, 0xBC, 51, false}, {KEY_MINUS, 0xBD, 12, false},
  {KEY_PERIOD, 0xBE, 52, false},     {KEY_SLASH, 0xBF, 53, false}, {KEY_SEMICOLON, 0xBA, 39, false},
  {KEY_EQUAL, 0xBB, 13, false},      {KEY_BRACKETLEFT, 0xDB, 26, false},
  {KEY_BACKSLASH, 0xDC, 43, false},  {KEY_BRACKETRIGHT, 0xDD, 27, false},
  {KEY_QUOTELEFT, 0xC0, 41, false},
  {KEY_0, 0x30, 11, false}, {KEY_1, 0x31, 2, false}, {KEY_2, 0x32, 3, false}, {KEY_3, 0x33, 4, false},
  {KEY_4, 0x34, 5, false},  {KEY_5, 0x35, 6, false}, {KEY_6, 0x36, 7, false}, {KEY_7, 0x37, 8, false},
  {KEY_8, 0x38, 9, false},  {KEY_9, 0x39, 10, false},
  {KEY_A, 0x41, 30, false}, {KEY_B, 0x42, 48, false}, {KEY_C, 0x43, 46, false}, {KEY_D, 0x44, 32, false},
  {KEY_E, 0x45, 18, false}, {KEY_F, 0x46, 33, false}, {KEY_G, 0x47, 34, false}, {KEY_H, 0x48, 35, false},
  {KEY_I, 0x49, 23, false}, {KEY_J, 0x4A, 36, false}, {KEY_K, 0x4B, 37, false}, {KEY_L, 0x4C, 38, false},
  {KEY_M, 0x4D, 50, false}, {KEY_N, 0x4E, 49, false}, {KEY_O, 0x4F, 24, false}, {KEY_P, 0x50, 25, false},
  {KEY_Q, 0x51, 16, false}, {KEY_R, 0x52, 19, false}, {KEY_S, 0x53, 31, false}, {KEY_T, 0x54, 20, false},
  {KEY_U, 0x55, 22, false}, {KEY_V, 0x56, 47, false}, {KEY_W, 0x57, 17, false}, {KEY_X, 0x58, 45, false},
  {KEY_Y, 0x59, 21, false}, {KEY_Z, 0x5A, 44, false},
};

const KeyMap *find(Key k)
{
  for (const KeyMap &m : kMap)
    if (m.key == k) return &m;
  return nullptr;
}

}  /* namespace */

bool sf_key_to_win32(Key logical, Key physical, KeyLocation loc, int *vk, int *sc, bool *ext)
{
  const KeyMap *l = find(logical);
  const KeyMap *p = find(physical);
  if (!l && !p) return false;
  *vk = l ? l->vk : p->vk;
  const KeyMap *s = p ? p : l;
  *sc = s->sc;
  *ext = s->ext;
  if (loc == KEY_LOCATION_RIGHT) {
    Key k = p ? p->key : l->key;
    if (k == KEY_SHIFT) { *sc = 54; *ext = false; }
    else if (k == KEY_CTRL || k == KEY_ALT) *ext = true;
    else if (k == KEY_META) { *vk = 0x5C; *sc = 92; *ext = true; }
  }
  return true;
}

int sf_key_char(Key logical, int unicode)
{
  switch (logical) {
  case KEY_ENTER: case KEY_KP_ENTER: return 13;
  case KEY_BACKSPACE: return 8;
  case KEY_TAB: case KEY_BACKTAB: return 9;
  case KEY_ESCAPE: return 27;
  default: break;
  }
  if (unicode >= 32 && unicode < 256 && unicode != 127) return unicode;
  /* a few Windows-1252 characters above Latin-1 */
  switch (unicode) {
  case 0x20AC: return 0x80;   /* euro */
  case 0x2019: return 0x92;
  default: return 0;
  }
}
