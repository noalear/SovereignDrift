/* ************************************************************************* */
/* w32_host.cpp -- the host side of the emulation: starting the program,     */
/* composing the virtual desktop, and turning host mouse/keyboard input into */
/* Win32 input messages (the job of the Windows raw input thread).           */
/*                                                                           */
/* NEW FILE, added 2026-10-12 for the Stellar Frontier Godot port.           */
/* ************************************************************************* */
#include "w32_internal.h"
#include <thread>
#include <chrono>
#include <filesystem>
#include <cstdarg>
#include <cstring>

using namespace w32;

#define WM_SF_CLICKACTIVATE 0x0399

namespace {
W32_EARLY std::string g_install; W32_EARLY std::string g_exe; W32_EARLY std::string g_zap; W32_EARLY std::string g_icon;
std::atomic<bool> g_running{false};
std::atomic<int> g_exit_code{0};
std::mutex g_warp_mtx;
bool g_warp = false;
POINT g_warp_pt;
}

namespace w32 {
sf_host_callbacks &host_callbacks();
void icon_set_file(const char *path);

std::string host_install_dir() { return g_install; }
const char *host_exe_name() { return g_exe.c_str(); }

void host_log(const char *fmt, ...)
{
  char buf[2048];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);
  if (host_callbacks().log) host_callbacks().log(buf);
  else fprintf(stderr, "[w32] %s\n", buf);
}

void host_notify_minimized()
{
  /* without a task bar, a minimised program can only come back through the
     host window: tell the host when nothing is left on the desktop */
  bool any = false;
  {
    std::lock_guard<std::recursive_mutex> lk(g_wm);
    for (Wnd *t : g_top) if (t->visible() && !t->minimized && t->cls->name != "#32768") any = true;
  }
  if (!any && host_callbacks().minimize) host_callbacks().minimize(1);
}
}  /* namespace w32 */

namespace w32 { std::string host_zap_path() { return g_zap; } }

extern "C" void sf_host_log(const char *fmt, ...)
{
  char buf[2048];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);
  w32::host_log("%s", buf);
}

extern "C" void sf_w32_set_nt(int nt);

/* exit() inside the program ends only the program, not the host */
struct ProgramExit { int code; };

extern "C" void sf_exit(int code)
{
  g_exit_code = code;
  throw ProgramExit{code};
}

extern "C" int sf_host_start(const sf_host_config *cfg, sf_host_main_fn fn, int argc, char **argv)
{
  if (!cfg || !fn) return -1;
  g_screen_w = cfg->screen_w > 0 ? cfg->screen_w : 1024;
  g_screen_h = cfg->screen_h > 0 ? cfg->screen_h : 768;
  g_mouse.x = g_screen_w / 2;
  g_mouse.y = g_screen_h / 2;
  g_install = cfg->install_dir ? cfg->install_dir : ".";
  while (g_install.size() > 1 && (g_install.back() == '/' || g_install.back() == '\\')) g_install.pop_back();
  g_exe = cfg->exe_name ? cfg->exe_name : "frontier.exe";
  g_zap = cfg->zap_archive ? cfg->zap_archive : "";
  g_icon = cfg->icon_file ? cfg->icon_file : "";
  icon_set_file(g_icon.c_str());
  sf_w32_set_nt(cfg->emulate_nt);
  user_init();
  std::error_code ec;
  std::filesystem::current_path(std::filesystem::path(g_install) / "binwin", ec);
  if (ec) host_log("cannot enter %s/binwin: %s", g_install.c_str(), ec.message().c_str());
  std::vector<std::string> args(argv, argv + argc);
  g_running = true;
  std::thread([fn, args]() {
    fpu_msvc_default();
    std::vector<std::string> a = args;
    std::vector<char *> av;
    for (auto &s : a) av.push_back(&s[0]);
    av.push_back(nullptr);
    int rc = 0;
    try {
      rc = fn((int)a.size(), av.data());
    } catch (ProgramExit &e) {
      rc = e.code;
    } catch (ThreadTerminated &) {
      rc = 0;
    } catch (...) {
      host_log("program main thread ended with an exception");
      rc = 255;
    }
    g_exit_code = rc;
    g_running = false;
    host_log("program exited with code %d", rc);
  }).detach();
  return 0;
}

extern "C" int sf_host_running(void) { return g_running ? 1 : 0; }
extern "C" int sf_host_exit_code(void) { return g_exit_code; }

extern "C" void sf_host_request_close(void)
{
  std::vector<HWND> tops;
  {
    std::lock_guard<std::recursive_mutex> lk(g_wm);
    for (Wnd *t : g_top) if (!t->owner && t->visible()) tops.push_back(t->hwnd);
  }
  for (HWND h : tops) PostMessage(h, WM_CLOSE, 0, 0);
}

extern "C" void sf_host_screen_size(int *w, int *h)
{
  if (w) *w = g_screen_w;
  if (h) *h = g_screen_h;
}

/* ================================ compose ================================ */
static void blit_window(uint32_t *dst, Wnd *w, int x, int y, RECT clip)
{
  Surface *s = w->surf.get();
  if (!s) return;
  RECT wr = {x, y, x + s->w, y + s->h}, r;
  if (IntersectRect(&r, &wr, &clip)) {
    for (int yy = r.top; yy < r.bottom; yy++) {
      const uint32_t *sp = &s->px[(size_t)(yy - y) * s->w + (r.left - x)];
      uint32_t *dp = &dst[(size_t)yy * g_screen_w + r.left];
      for (int xx = r.left; xx < r.right; xx++, sp++, dp++)
        if ((*sp >> 24) != 0xFF) *dp = *sp & 0xFFFFFF;
    }
  }
  /* children, bottom of the z-order first, clipped to our client area */
  RECT cc = {x + w->client.left, y + w->client.top, x + w->client.right, y + w->client.bottom};
  RECT cclip;
  if (!IntersectRect(&cclip, &cc, &clip)) return;
  for (auto it = w->children.rbegin(); it != w->children.rend(); ++it) {
    Wnd *c = *it;
    if (!c->visible()) continue;
    blit_window(dst, c, cc.left + c->rc.left, cc.top + c->rc.top, cclip);
  }
}

extern "C" int sf_host_frame(uint32_t *dst, unsigned *serial)
{
  uint32_t now = g_screen_serial.load();
  if (serial && *serial == now) return 0;
  std::lock_guard<std::recursive_mutex> lk(g_wm);
  now = g_screen_serial.load();
  uint32_t bg = syscolor_px(COLOR_BACKGROUND);
  size_t n = (size_t)g_screen_w * g_screen_h;
  for (size_t i = 0; i < n; i++) dst[i] = bg;
  RECT screen = {0, 0, g_screen_w, g_screen_h};
  for (auto it = g_top.rbegin(); it != g_top.rend(); ++it) {
    Wnd *t = *it;
    if (!t->visible() || t->minimized) continue;
    blit_window(dst, t, t->rc.left, t->rc.top, screen);
  }
  if (serial) *serial = now;
  return 1;
}

/* ================================= input ================================= */
namespace {

DWORD g_lastClickTime = 0;
POINT g_lastClickPt = {-100, -100};
HWND g_lastClickWnd = 0;
UINT g_lastClickBtn = 0;

WPARAM mk_flags()
{
  WPARAM f = 0;
  if (g_keys[VK_LBUTTON] & 0x80) f |= MK_LBUTTON;
  if (g_keys[VK_RBUTTON] & 0x80) f |= MK_RBUTTON;
  if (g_keys[VK_MBUTTON] & 0x80) f |= MK_MBUTTON;
  if (g_keys[VK_SHIFT] & 0x80) f |= MK_SHIFT;
  if (g_keys[VK_CONTROL] & 0x80) f |= MK_CONTROL;
  return f;
}

bool disabled_chain(Wnd *w)
{
  for (; w; w = w->parent) if (!w->enabled()) return true;
  return false;
}

Wnd *top_of(Wnd *w) { while (w && w->parent) w = w->parent; return w; }

/* the enabled popup a disabled owner is waiting for (MessageBox etc.) */
Wnd *modal_popup_of(Wnd *owner)
{
  for (Wnd *t : g_top)
    if (t->owner == owner && t->visible() && t->enabled()) return t;
  return nullptr;
}

void update_cursor(Wnd *target, int hit)
{
  int shape = 0;
  if (target && hit == HTCLIENT && target->cls->name == "EDIT") shape = 1;
  if (target && target->cls->cursor) {
    /* keep whatever the program set with SetCursor for its own windows */
    return;
  }
  g_cursor_shape = shape;
}

/* button: 0 move, 1 left, 2 right, 3 middle; down: 1/0 */
void route_mouse(int button, bool down)
{
  std::lock_guard<std::recursive_mutex> lk(g_wm);
  int sx = g_mouse.x, sy = g_mouse.y;
  Wnd *cap = wnd(g_capture);
  Wnd *target = cap ? cap : window_at(sx, sy, true);
  if (!target) return;
  if (!cap && disabled_chain(target)) {
    if (button && down) {
      Wnd *t = top_of(target);
      if (t && !t->enabled()) {
        if (Wnd *p = modal_popup_of(t)) post_msg(p, WM_SF_CLICKACTIVATE, 0, 0);
      }
    }
    return;
  }
  int hit = cap ? HTCLIENT : nc_hittest(target, sx, sy);
  if (!cap && hit == HTNOWHERE) return;
  update_cursor(target, hit);
  UINT msg;
  if (button == 0) msg = WM_MOUSEMOVE;
  else {
    static const UINT downs[4] = {0, WM_LBUTTONDOWN, WM_RBUTTONDOWN, WM_MBUTTONDOWN};
    static const UINT ups[4] = {0, WM_LBUTTONUP, WM_RBUTTONUP, WM_MBUTTONUP};
    msg = down ? downs[button] : ups[button];
  }
  /* double clicks */
  bool dbl = false;
  if (button && down) {
    DWORD t = GetTickCount();
    if (g_lastClickWnd == target->hwnd && g_lastClickBtn == (UINT)button &&
        t - g_lastClickTime <= 500 && std::abs(sx - g_lastClickPt.x) <= 2 && std::abs(sy - g_lastClickPt.y) <= 2) {
      dbl = true;
      g_lastClickWnd = 0;
    } else {
      g_lastClickWnd = target->hwnd;
      g_lastClickBtn = button;
      g_lastClickTime = t;
      g_lastClickPt = g_mouse;
    }
  }
  if (button && down && !cap) {
    Wnd *t = top_of(target);
    if (t && g_active != t->hwnd && t->cls->name != "#32768" && t->cls->name != "ComboLBox")
      post_msg(target, WM_SF_CLICKACTIVATE, 0, MAKELPARAM(hit, msg));
  }
  if (hit == HTCLIENT) {
    if (dbl && (target->cls->style & CS_DBLCLKS)) msg += 2;   /* xBUTTONDOWN -> xBUTTONDBLCLK */
    int cx, cy;
    wnd_client_origin(target, &cx, &cy);
    post_msg(target, msg, mk_flags(), MAKELPARAM((short)(sx - cx), (short)(sy - cy)));
  } else {
    UINT nm = msg - WM_MOUSEMOVE + WM_NCMOUSEMOVE;
    if (dbl) nm += 2;
    post_msg(target, nm, (WPARAM)hit, MAKELPARAM((short)sx, (short)sy));
  }
}

void release_all_keys();

}  /* namespace */

extern "C" void sf_host_mouse_move(int x, int y)
{
  {
    std::lock_guard<std::recursive_mutex> lk(g_wm);
    if (x == g_mouse.x && y == g_mouse.y) return;
    g_mouse.x = x;
    g_mouse.y = y;
  }
  route_mouse(0, false);
}

extern "C" void sf_host_mouse_button(int x, int y, int button, int down)
{
  if (button < 1 || button > 3) return;
  {
    std::lock_guard<std::recursive_mutex> lk(g_wm);
    g_mouse.x = x;
    g_mouse.y = y;
    static const int vks[4] = {0, VK_LBUTTON, VK_RBUTTON, VK_MBUTTON};
    if (down) g_keys[vks[button]] |= 0x80;
    else g_keys[vks[button]] &= ~0x80;
  }
  route_mouse(button, down != 0);
}

extern "C" void sf_host_mouse_wheel(int x, int y, int delta)
{
  std::lock_guard<std::recursive_mutex> lk(g_wm);
  g_mouse.x = x;
  g_mouse.y = y;
  Wnd *t = wnd(g_focus);
  if (!t) t = wnd(g_active);
  if (!t) return;
  post_msg(t, WM_MOUSEWHEEL, MAKEWPARAM(mk_flags(), (short)delta), MAKELPARAM(x, y));
}

extern "C" void sf_host_key(int vk, int sc, int ext, int down, unsigned ch)
{
  if (vk <= 0 || vk > 255) return;
  std::lock_guard<std::recursive_mutex> lk(g_wm);
  bool prev = (g_keys[vk] & 0x80) != 0;
  if (down) {
    if (!prev && (vk == VK_CAPITAL || vk == VK_NUMLOCK || vk == VK_SCROLL)) g_keys[vk] ^= 1;
    g_keys[vk] |= 0x80;
  } else {
    g_keys[vk] &= ~0x80;
  }
  /* keep the generic and left/right modifier states consistent */
  if (vk == VK_SHIFT) {
    int lr = sc == 54 ? VK_RSHIFT : VK_LSHIFT;
    if (down) g_keys[lr] |= 0x80; else g_keys[lr] &= ~0x80;
  } else if (vk == VK_CONTROL) {
    int lr = ext ? VK_RCONTROL : VK_LCONTROL;
    if (down) g_keys[lr] |= 0x80; else g_keys[lr] &= ~0x80;
  } else if (vk == VK_MENU) {
    int lr = ext ? VK_RMENU : VK_LMENU;
    if (down) g_keys[lr] |= 0x80; else g_keys[lr] &= ~0x80;
  }
  bool alt = (g_keys[VK_MENU] & 0x80) != 0 || (vk == VK_MENU && !down);
  bool ctrl = (g_keys[VK_CONTROL] & 0x80) != 0;
  Wnd *t = wnd(g_focus);
  bool sys = (alt && !ctrl) || vk == VK_F10 || (vk == VK_MENU);
  if (!t) { t = wnd(g_active); sys = true; }
  if (!t) return;
  UINT msg = down ? (sys ? WM_SYSKEYDOWN : WM_KEYDOWN) : (sys ? WM_SYSKEYUP : WM_KEYUP);
  LPARAM lp = 1 | ((LPARAM)(sc & 0xFF) << 16) | (ext ? (1 << 24) : 0) |
              ((alt && msg != WM_KEYDOWN && msg != WM_KEYUP) ? (1 << 29) : 0) |
              ((down ? prev : 1) ? (1 << 30) : 0) | (down ? 0 : (1u << 31));
  post_msg(t, msg, (WPARAM)vk, lp, down ? ch : 0);
}

namespace {
void release_all_keys()
{
  std::vector<int> down;
  {
    std::lock_guard<std::recursive_mutex> lk(g_wm);
    for (int k = 8; k < 256; k++)
      if ((g_keys[k] & 0x80) && k != VK_LSHIFT && k != VK_RSHIFT && k != VK_LCONTROL &&
          k != VK_RCONTROL && k != VK_LMENU && k != VK_RMENU)
        down.push_back(k);
  }
  for (int k : down) {
    int sc = 0;
    switch (k) {
    case VK_SHIFT: sc = 42; break;
    case VK_CONTROL: sc = 29; break;
    case VK_MENU: sc = 56; break;
    default: break;
    }
    sf_host_key(k, sc, 0, 0, 0);
  }
}
}  /* namespace */

extern "C" void sf_host_focus(int has_focus)
{
  if (!has_focus) release_all_keys();
}

extern "C" int sf_host_take_warp(int *x, int *y)
{
  std::lock_guard<std::mutex> lk(g_warp_mtx);
  if (!g_warp) return 0;
  g_warp = false;
  if (x) *x = g_warp_pt.x;
  if (y) *y = g_warp_pt.y;
  return 1;
}

extern "C" int sf_host_cursor(void) { return g_cursor_shape.load(); }

extern "C" void sf_host_restore(void)
{
  std::vector<HWND> mins;
  {
    std::lock_guard<std::recursive_mutex> lk(g_wm);
    for (Wnd *t : g_top)
      if (t->visible() && t->minimized && !t->owner) mins.push_back(t->hwnd);
  }
  /* processed by each window's own thread, as a task-bar click would be */
  for (HWND h : mins) PostMessage(h, WM_SYSCOMMAND, SC_RESTORE, 0);
}

/* ---------------------------- pointer API -------------------------------- */
BOOL GetCursorPos(LPPOINT p)
{
  if (!p) return FALSE;
  std::lock_guard<std::recursive_mutex> lk(g_wm);
  *p = g_mouse;
  return TRUE;
}

BOOL SetCursorPos(int x, int y)
{
  x = std::max(0, std::min(x, g_screen_w - 1));
  y = std::max(0, std::min(y, g_screen_h - 1));
  {
    std::lock_guard<std::mutex> lk(g_warp_mtx);
    g_warp = true;
    g_warp_pt.x = x;
    g_warp_pt.y = y;
  }
  {
    std::lock_guard<std::recursive_mutex> lk(g_wm);
    g_mouse.x = x;
    g_mouse.y = y;
  }
  route_mouse(0, false);
  return TRUE;
}
