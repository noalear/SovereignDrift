/* ************************************************************************* */
/* w32_user.cpp -- USER emulation core: window classes, windows, message     */
/* queues (one per thread, with cross-thread SendMessage), timers, geometry, */
/* z-order, activation, focus, capture, painting and window DCs.             */
/*                                                                           */
/* NEW FILE, added 2026-10-12 for the Stellar Frontier Godot port.           */
/*                                                                           */
/* Every window owns a backing surface the size of its window rectangle.     */
/* Applications draw into it through window DCs; the desktop is composed     */
/* from these surfaces (w32_host.cpp).  Pixels whose top byte is 0xFF are    */
/* "never painted" and let whatever is underneath show through, which is     */
/* how transparent controls such as group boxes look right.                  */
/* ************************************************************************* */
#include "w32_internal.h"
#include <unordered_map>
#include <chrono>
#include <thread>
#include <cstring>

using namespace w32;

namespace w32 {

std::recursive_mutex g_wm;
std::condition_variable_any g_wm_cv W32_EARLY;
std::vector<Wnd *> g_top W32_EARLY;
int g_screen_w = 1024, g_screen_h = 768;
HWND g_focus = 0, g_active = 0, g_capture = 0;
POINT g_mouse = {512, 384};
uint8_t g_keys[256];

const uint32_t TRANSPARENT_PX = 0xFF000000u;

Wnd *wnd(HWND h) { return H<Wnd>(h, HT_WND); }

void wake_all() { g_wm_cv.notify_all(); }

/* ================================ classes ================================ */
static std::vector<WndClass *> g_classes W32_EARLY;

static WndClass *find_class(const char *name)
{
  if (!name) return nullptr;
  if (IS_INTRESOURCE(name)) return nullptr;
  for (auto *c : g_classes)
    if (strcasecmp(c->name.c_str(), name) == 0) return c;
  return nullptr;
}

static void add_builtin(const char *name, WNDPROC proc, UINT style, HBRUSH bg)
{
  WndClass *c = new WndClass();
  c->name = name;
  c->proc = proc;
  c->style = style;
  c->bg = bg;
  c->builtin = true;
  g_classes.push_back(c);
}

LRESULT CALLBACK menu_popup_proc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK combolbox_proc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK msgbox_proc(HWND, UINT, WPARAM, LPARAM);

void user_init()
{
  static std::once_flag once;
  std::call_once(once, []() {
    memset(g_keys, 0, sizeof(g_keys));
    gdi_init();
    add_builtin("BUTTON", button_proc, CS_DBLCLKS | CS_PARENTDC | CS_HREDRAW | CS_VREDRAW, 0);
    add_builtin("STATIC", static_proc, CS_DBLCLKS | CS_PARENTDC, 0);
    add_builtin("EDIT", edit_proc, CS_DBLCLKS | CS_PARENTDC, 0);
    add_builtin("LISTBOX", listbox_proc, CS_DBLCLKS | CS_PARENTDC, 0);
    add_builtin("COMBOBOX", combobox_proc, CS_DBLCLKS | CS_PARENTDC | CS_HREDRAW | CS_VREDRAW, 0);
    add_builtin("SCROLLBAR", scrollbar_proc, CS_DBLCLKS | CS_PARENTDC | CS_HREDRAW | CS_VREDRAW, 0);
    add_builtin("ComboLBox", combolbox_proc, CS_DBLCLKS | CS_SAVEBITS, 0);
    add_builtin("#32768", menu_popup_proc, CS_DBLCLKS | CS_SAVEBITS, 0);
    add_builtin("#32770", msgbox_proc, CS_DBLCLKS | CS_SAVEBITS, (HBRUSH)(uintptr_t)(COLOR_BTNFACE + 1));
  });
}

}  /* namespace w32 */

ATOM RegisterClass(const WNDCLASS *wc)
{
  user_init();
  if (!wc || !wc->lpszClassName) return 0;
  std::lock_guard<std::recursive_mutex> lk(g_wm);
  if (WndClass *c = find_class(wc->lpszClassName)) {
    if (!c->builtin) { SetLastError(1410 /*ERROR_CLASS_ALREADY_EXISTS*/); return 0; }
  }
  WndClass *c = new WndClass();
  c->name = wc->lpszClassName;
  c->style = wc->style;
  c->proc = wc->lpfnWndProc;
  c->bg = wc->hbrBackground;
  c->icon = wc->hIcon;
  c->cursor = wc->hCursor;
  g_classes.push_back(c);
  return (ATOM)(0xC000 + g_classes.size());
}

BOOL UnregisterClass(LPCSTR name, HINSTANCE)
{
  std::lock_guard<std::recursive_mutex> lk(g_wm);
  for (size_t i = 0; i < g_classes.size(); i++)
    if (!g_classes[i]->builtin && strcasecmp(g_classes[i]->name.c_str(), name) == 0) {
      /* classes stay allocated: windows may still reference them */
      g_classes.erase(g_classes.begin() + i);
      return TRUE;
    }
  return FALSE;
}

int GetClassName(HWND h, LPSTR buf, int n)
{
  Wnd *w = wnd(h);
  if (!w || !buf || n <= 0) return 0;
  snprintf(buf, n, "%s", w->cls->name.c_str());
  return (int)strlen(buf);
}

ULONG_PTR GetClassLong(HWND h, int idx)
{
  Wnd *w = wnd(h);
  if (!w) return 0;
  switch (idx) {
  case GCL_HICON: return (ULONG_PTR)w->cls->icon;
  case GCL_HCURSOR: return (ULONG_PTR)w->cls->cursor;
  case GCL_HBRBACKGROUND: return (ULONG_PTR)w->cls->bg;
  case GCL_STYLE: return w->cls->style;
  default: return 0;
  }
}

ULONG_PTR SetClassLong(HWND h, int idx, LONG_PTR v)
{
  Wnd *w = wnd(h);
  if (!w) return 0;
  std::lock_guard<std::recursive_mutex> lk(g_wm);
  ULONG_PTR old = GetClassLong(h, idx);
  switch (idx) {
  case GCL_HICON: w->cls->icon = (HICON)v; break;
  case GCL_HCURSOR: w->cls->cursor = (HCURSOR)v; break;
  case GCL_HBRBACKGROUND: w->cls->bg = (HBRUSH)v; break;
  case GCL_STYLE: w->cls->style = (UINT)v; break;
  }
  /* the caption shows the class icon */
  for (Wnd *t : g_top) if (t->cls == w->cls) nc_paint(t);
  return old;
}

/* ============================= thread queues ============================= */
namespace {

struct SentMsg {
  HWND h; UINT m; WPARAM wp; LPARAM lp;
  LRESULT r = 0;
  bool done = false;
};

struct Timer {
  HWND h; UINT_PTR id; UINT ms;
  std::chrono::steady_clock::time_point next;
};

struct TQ {
  DWORD tid = 0;
  std::deque<MSG> posted;
  std::deque<SentMsg *> sent;
  std::vector<Timer> timers;
  bool quit = false;
  int quitCode = 0;
  MSG last = {};
};

std::unordered_map<DWORD, TQ *> g_tqs W32_EARLY;

TQ *tq(DWORD tid)
{
  auto it = g_tqs.find(tid);
  if (it != g_tqs.end()) return it->second;
  TQ *q = new TQ();
  q->tid = tid;
  g_tqs[tid] = q;
  return q;
}

DWORD msg_time() { return GetTickCount(); }

}  /* namespace */

namespace w32 {

void post_msg(Wnd *w, UINT m, WPARAM wp, LPARAM lp, DWORD extra)
{
  std::lock_guard<std::recursive_mutex> lk(g_wm);
  TQ *q = tq(w ? w->tid : cur_tid());
  MSG msg;
  memset(&msg, 0, sizeof(msg));
  msg.hwnd = w ? w->hwnd : 0;
  msg.message = m;
  msg.wParam = wp;
  msg.lParam = lp;
  msg.time = msg_time();
  msg.pt = g_mouse;
  msg.lPrivate = extra;
  /* WM_MOUSEMOVE is coalesced, as Windows does */
  if (m == WM_MOUSEMOVE && !q->posted.empty() && q->posted.back().message == WM_MOUSEMOVE &&
      q->posted.back().hwnd == msg.hwnd) {
    q->posted.back() = msg;
  } else {
    q->posted.push_back(msg);
  }
  wake_all();
}

LRESULT call_proc(Wnd *w, UINT m, WPARAM wp, LPARAM lp)
{
  if (!w) return 0;
  WNDPROC p = w->proc;
  HWND h = w->hwnd;
  if (!p) return DefWindowProc(h, m, wp, lp);
  return p(h, m, wp, lp);
}

}  /* namespace w32 */

/* Process messages other threads have sent to us.  Lock held on entry and
   exit, released while each window procedure runs. */
static void process_sent(std::unique_lock<std::recursive_mutex> &lk, TQ *q)
{
  while (!q->sent.empty()) {
    SentMsg *s = q->sent.front();
    q->sent.pop_front();
    Wnd *w = wnd(s->h);
    lk.unlock();
    LRESULT r = 0;
    try {
      r = w ? call_proc(w, s->m, s->wp, s->lp) : 0;
    } catch (...) {
      lk.lock();
      s->r = 0;
      s->done = true;
      wake_all();
      throw;
    }
    lk.lock();
    s->r = r;
    s->done = true;
    wake_all();
  }
}

namespace w32 {

LRESULT send_msg(HWND h, UINT m, WPARAM wp, LPARAM lp)
{
  Wnd *w = wnd(h);
  if (!w) return 0;
  DWORD me = cur_tid();
  if (w->tid == me) return call_proc(w, m, wp, lp);
  /* inter-thread send: queue it to the owner and wait, while still
     answering messages sent to us (Windows does the same) */
  SentMsg s;
  s.h = h; s.m = m; s.wp = wp; s.lp = lp;
  std::unique_lock<std::recursive_mutex> lk(g_wm);
  TQ *dst = tq(w->tid);
  dst->sent.push_back(&s);
  wake_all();
  TQ *mine = tq(me);
  while (!s.done) {
    process_sent(lk, mine);
    if (s.done) break;
    if (!wnd(h)) {   /* destroyed meanwhile */
      auto &d = dst->sent;
      d.erase(std::remove(d.begin(), d.end(), &s), d.end());
      return 0;
    }
    g_wm_cv.wait_for(lk, std::chrono::milliseconds(50));
    lk.unlock();
    check_terminated();
    lk.lock();
  }
  return s.r;
}

}  /* namespace w32 */

LRESULT SendMessage(HWND h, UINT m, WPARAM wp, LPARAM lp)
{
  if (h == HWND_BROADCAST) {
    std::vector<HWND> all;
    {
      std::lock_guard<std::recursive_mutex> lk(g_wm);
      for (Wnd *w : g_top) all.push_back(w->hwnd);
    }
    for (HWND x : all) send_msg(x, m, wp, lp);
    return 0;
  }
  return send_msg(h, m, wp, lp);
}

LRESULT CallWindowProc(WNDPROC p, HWND h, UINT m, WPARAM wp, LPARAM lp)
{
  return p ? p(h, m, wp, lp) : DefWindowProc(h, m, wp, lp);
}

BOOL PostMessage(HWND h, UINT m, WPARAM wp, LPARAM lp)
{
  if (!h) { post_msg(nullptr, m, wp, lp); return TRUE; }
  Wnd *w = wnd(h);
  if (!w) { SetLastError(1400 /*ERROR_INVALID_WINDOW_HANDLE*/); return FALSE; }
  post_msg(w, m, wp, lp);
  return TRUE;
}

BOOL PostThreadMessage(DWORD tid, UINT m, WPARAM wp, LPARAM lp)
{
  std::lock_guard<std::recursive_mutex> lk(g_wm);
  TQ *q = tq(tid);
  MSG msg;
  memset(&msg, 0, sizeof(msg));
  msg.message = m; msg.wParam = wp; msg.lParam = lp;
  msg.time = msg_time(); msg.pt = g_mouse;
  q->posted.push_back(msg);
  wake_all();
  return TRUE;
}

void PostQuitMessage(int code)
{
  std::lock_guard<std::recursive_mutex> lk(g_wm);
  TQ *q = tq(cur_tid());
  q->quit = true;
  q->quitCode = code;
  wake_all();
}

/* ------------------------------- timers ---------------------------------- */
UINT_PTR SetTimer(HWND h, UINT_PTR id, UINT ms, void *)
{
  std::lock_guard<std::recursive_mutex> lk(g_wm);
  Wnd *w = wnd(h);
  TQ *q = tq(w ? w->tid : cur_tid());
  for (auto &t : q->timers)
    if (t.h == h && t.id == id) {
      t.ms = ms;
      t.next = std::chrono::steady_clock::now() + std::chrono::milliseconds(ms);
      return id;
    }
  q->timers.push_back(Timer{h, id, ms, std::chrono::steady_clock::now() + std::chrono::milliseconds(ms)});
  wake_all();
  return id;
}

BOOL KillTimer(HWND h, UINT_PTR id)
{
  std::lock_guard<std::recursive_mutex> lk(g_wm);
  Wnd *w = wnd(h);
  TQ *q = tq(w ? w->tid : cur_tid());
  for (size_t i = 0; i < q->timers.size(); i++)
    if (q->timers[i].h == h && q->timers[i].id == id) {
      q->timers.erase(q->timers.begin() + i);
      return TRUE;
    }
  return FALSE;
}

/* -------------------------- message retrieval ---------------------------- */
static bool belongs(HWND filter, HWND h)
{
  if (!filter) return true;
  if (filter == (HWND)(LONG_PTR)-1) return h == 0;
  for (Wnd *w = wnd(h); w; w = w->parent)
    if (w->hwnd == filter) return true;
  return false;
}

static bool in_range(UINT m, UINT lo, UINT hi)
{
  if (lo == 0 && hi == 0) return true;
  return m >= lo && m <= hi;
}

static Wnd *find_paint(DWORD tid, HWND filter)
{
  /* parents before children, top-level windows in z-order */
  std::function<Wnd *(Wnd *)> visit = [&](Wnd *w) -> Wnd * {
    if (!w->visible()) return nullptr;
    if (w->tid == tid && w->hasUpdate && belongs(filter, w->hwnd)) return w;
    for (Wnd *c : w->children)
      if (Wnd *r = visit(c)) return r;
    return nullptr;
  };
  for (Wnd *t : g_top)
    if (Wnd *r = visit(t)) return r;
  return nullptr;
}

/* Returns 1 message, 0 nothing (non-blocking), -1 WM_QUIT. */
static int fetch(std::unique_lock<std::recursive_mutex> &lk, TQ *q, LPMSG msg,
                 HWND filter, UINT lo, UINT hi, bool remove)
{
  process_sent(lk, q);
  for (auto it = q->posted.begin(); it != q->posted.end(); ++it) {
    if (!belongs(filter, it->hwnd) || !in_range(it->message, lo, hi)) continue;
    *msg = *it;
    if (remove) q->posted.erase(it);
    q->last = *msg;
    return msg->message == WM_QUIT ? -1 : 1;
  }
  if (q->quit && (!filter || filter == 0)) {
    memset(msg, 0, sizeof(*msg));
    msg->message = WM_QUIT;
    msg->wParam = (WPARAM)q->quitCode;
    if (remove) q->quit = false;
    return -1;
  }
  if (in_range(WM_PAINT, lo, hi)) {
    if (Wnd *w = find_paint(q->tid, filter)) {
      memset(msg, 0, sizeof(*msg));
      msg->hwnd = w->hwnd;
      msg->message = WM_PAINT;
      msg->time = msg_time();
      msg->pt = g_mouse;
      q->last = *msg;
      return 1;
    }
  }
  if (in_range(WM_TIMER, lo, hi)) {
    auto now = std::chrono::steady_clock::now();
    for (auto &t : q->timers) {
      if (t.next > now || !belongs(filter, t.h)) continue;
      memset(msg, 0, sizeof(*msg));
      msg->hwnd = t.h;
      msg->message = WM_TIMER;
      msg->wParam = t.id;
      msg->time = msg_time();
      msg->pt = g_mouse;
      if (remove) t.next = now + std::chrono::milliseconds(t.ms ? t.ms : 1);
      q->last = *msg;
      return 1;
    }
  }
  return 0;
}

static std::chrono::milliseconds next_timer_wait(TQ *q)
{
  auto now = std::chrono::steady_clock::now();
  std::chrono::milliseconds best(100);
  for (auto &t : q->timers) {
    auto d = std::chrono::duration_cast<std::chrono::milliseconds>(t.next - now);
    if (d < best) best = d;
  }
  if (best.count() < 1) best = std::chrono::milliseconds(1);
  return best;
}

BOOL GetMessage(LPMSG msg, HWND filter, UINT lo, UINT hi)
{
  user_init();
  check_terminated();
  std::unique_lock<std::recursive_mutex> lk(g_wm);
  TQ *q = tq(cur_tid());
  for (;;) {
    int r = fetch(lk, q, msg, filter, lo, hi, true);
    if (r == 1) return TRUE;
    if (r == -1) return FALSE;
    g_wm_cv.wait_for(lk, next_timer_wait(q));
    lk.unlock();
    check_terminated();
    lk.lock();
  }
}

BOOL PeekMessage(LPMSG msg, HWND filter, UINT lo, UINT hi, UINT flags)
{
  user_init();
  check_terminated();
  std::unique_lock<std::recursive_mutex> lk(g_wm);
  TQ *q = tq(cur_tid());
  int r = fetch(lk, q, msg, filter, lo, hi, (flags & PM_REMOVE) != 0);
  return r != 0;
}

BOOL WaitMessage(void)
{
  check_terminated();
  std::unique_lock<std::recursive_mutex> lk(g_wm);
  TQ *q = tq(cur_tid());
  MSG m;
  while (fetch(lk, q, &m, 0, 0, 0, false) == 0)
    g_wm_cv.wait_for(lk, next_timer_wait(q));
  return TRUE;
}

DWORD GetMessageTime(void)
{
  std::lock_guard<std::recursive_mutex> lk(g_wm);
  return tq(cur_tid())->last.time;
}

DWORD GetMessagePos(void)
{
  std::lock_guard<std::recursive_mutex> lk(g_wm);
  POINT p = tq(cur_tid())->last.pt;
  return MAKELONG(p.x, p.y);
}

LRESULT DispatchMessage(const MSG *msg)
{
  if (!msg || !msg->hwnd) return 0;
  Wnd *w = wnd(msg->hwnd);
  if (!w) return 0;
  return call_proc(w, msg->message, msg->wParam, msg->lParam);
}

/* US keyboard layout, for keys the host did not translate */
static unsigned vk_to_char(UINT vk, bool shift, bool caps)
{
  if (vk >= 'A' && vk <= 'Z') {
    bool upper = shift != caps;
    return upper ? vk : vk + 32;
  }
  if (vk >= '0' && vk <= '9') return shift ? ")!@#$%^&*("[vk - '0'] : vk;
  if (vk >= VK_NUMPAD0 && vk <= VK_NUMPAD9) return '0' + (vk - VK_NUMPAD0);
  switch (vk) {
  case VK_SPACE: return ' ';
  case VK_RETURN: return '\r';
  case VK_BACK: return 8;
  case VK_TAB: return 9;
  case VK_ESCAPE: return 27;
  case VK_OEM_1: return shift ? ':' : ';';
  case VK_OEM_PLUS: return shift ? '+' : '=';
  case VK_OEM_COMMA: return shift ? '<' : ',';
  case VK_OEM_MINUS: return shift ? '_' : '-';
  case VK_OEM_PERIOD: return shift ? '>' : '.';
  case VK_OEM_2: return shift ? '?' : '/';
  case VK_OEM_3: return shift ? '~' : '`';
  case VK_OEM_4: return shift ? '{' : '[';
  case VK_OEM_5: return shift ? '|' : '\\';
  case VK_OEM_6: return shift ? '}' : ']';
  case VK_OEM_7: return shift ? '"' : '\'';
  case VK_MULTIPLY: return '*';
  case VK_ADD: return '+';
  case VK_SUBTRACT: return '-';
  case VK_DECIMAL: return '.';
  case VK_DIVIDE: return '/';
  default: return 0;
  }
}

BOOL TranslateMessage(const MSG *msg)
{
  if (!msg) return FALSE;
  if (msg->message != WM_KEYDOWN && msg->message != WM_SYSKEYDOWN) return FALSE;
  unsigned ch = msg->lPrivate;
  bool ctrl = (g_keys[VK_CONTROL] & 0x80) != 0;
  bool shift = (g_keys[VK_SHIFT] & 0x80) != 0;
  if (!ch) ch = vk_to_char((UINT)msg->wParam, shift, (g_keys[VK_CAPITAL] & 1) != 0);
  if (ctrl && !(g_keys[VK_MENU] & 0x80)) {
    UINT vk = (UINT)msg->wParam;
    if (vk >= 'A' && vk <= 'Z') ch = vk - 'A' + 1;      /* control characters */
    else if (vk == VK_RETURN) ch = 10;
    else if (vk == VK_BACK) ch = 127;
    else if (ch > 127 || ch == 0) ch = 0;
  }
  if (!ch || ch > 255) return FALSE;
  UINT cm = msg->message == WM_SYSKEYDOWN ? WM_SYSCHAR : WM_CHAR;
  std::lock_guard<std::recursive_mutex> lk(g_wm);
  Wnd *w = wnd(msg->hwnd);
  if (!w) return FALSE;
  TQ *q = tq(w->tid);
  MSG c = *msg;
  c.message = cm;
  c.wParam = ch;
  c.lPrivate = 0;
  q->posted.push_front(c);   /* retrieved next, before later input */
  wake_all();
  return TRUE;
}

/* ================================ geometry =============================== */
namespace w32 {

void wnd_screen_origin(Wnd *w, int *x, int *y)
{
  int ox = w->rc.left, oy = w->rc.top;
  if (w->parent) {
    int px, py;
    wnd_client_origin(w->parent, &px, &py);
    ox += px;
    oy += py;
  }
  *x = ox;
  *y = oy;
}

void wnd_client_origin(Wnd *w, int *x, int *y)
{
  wnd_screen_origin(w, x, y);
  *x += w->client.left;
  *y += w->client.top;
}

bool wnd_is_visible_chain(Wnd *w)
{
  for (; w; w = w->parent)
    if (!w->visible() || w->minimized) return false;
  return true;
}

static Wnd *toplevel(Wnd *w)
{
  while (w && w->parent) w = w->parent;
  return w;
}

void wnd_invalidate(Wnd *w, const RECT *r, bool erase)
{
  RECT c = {0, 0, w->client.right - w->client.left, w->client.bottom - w->client.top};
  RECT a = r ? *r : c;
  RECT i;
  if (!IntersectRect(&i, &a, &c)) return;
  if (w->hasUpdate) UnionRect(&w->upd, &w->upd, &i);
  else { w->upd = i; w->hasUpdate = true; }
  if (erase) w->erase = true;
  wake_all();
}

static void resize_surface(Wnd *w)
{
  int cw = w->rc.right - w->rc.left, ch = w->rc.bottom - w->rc.top;
  if (!w->surf) {
    w->surf = std::make_shared<Surface>();
    w->surf->resize(std::max(cw, 0), std::max(ch, 0), TRANSPARENT_PX, false);
    return;
  }
  if (w->surf->w == cw && w->surf->h == ch) return;
  /* a new buffer: a drawing thread may still hold the old one */
  SurfaceP n = std::make_shared<Surface>();
  n->resize(std::max(cw, 0), std::max(ch, 0), TRANSPARENT_PX, false);
  Surface &o = *w->surf;
  int mw = std::min(o.w, n->w), mh = std::min(o.h, n->h);
  for (int y = 0; y < mh; y++) memcpy(n->row(y), o.row(y), mw * 4);
  w->surf = n;
}

/* Recompute the client rectangle (WM_NCCALCSIZE as DefWindowProc does it). */
static void recalc_client(Wnd *w)
{
  RECT c;
  nc_calc(w, w->style, w->exstyle, w->menu != 0 && !w->is_child(),
          w->rc.right - w->rc.left, w->rc.bottom - w->rc.top, &c);
  w->client = c;
}

}  /* namespace w32 */

BOOL GetWindowRect(HWND h, LPRECT r)
{
  std::lock_guard<std::recursive_mutex> lk(g_wm);
  Wnd *w = wnd(h);
  if (!w || !r) return FALSE;
  int x, y;
  wnd_screen_origin(w, &x, &y);
  r->left = x;
  r->top = y;
  r->right = x + (w->rc.right - w->rc.left);
  r->bottom = y + (w->rc.bottom - w->rc.top);
  return TRUE;
}

BOOL GetClientRect(HWND h, LPRECT r)
{
  std::lock_guard<std::recursive_mutex> lk(g_wm);
  Wnd *w = wnd(h);
  if (!w || !r) return FALSE;
  r->left = 0;
  r->top = 0;
  r->right = std::max(0, (int)(w->client.right - w->client.left));
  r->bottom = std::max(0, (int)(w->client.bottom - w->client.top));
  return TRUE;
}

BOOL ClientToScreen(HWND h, LPPOINT p)
{
  std::lock_guard<std::recursive_mutex> lk(g_wm);
  Wnd *w = wnd(h);
  if (!w || !p) return FALSE;
  int x, y;
  wnd_client_origin(w, &x, &y);
  p->x += x;
  p->y += y;
  return TRUE;
}

BOOL ScreenToClient(HWND h, LPPOINT p)
{
  std::lock_guard<std::recursive_mutex> lk(g_wm);
  Wnd *w = wnd(h);
  if (!w || !p) return FALSE;
  int x, y;
  wnd_client_origin(w, &x, &y);
  p->x -= x;
  p->y -= y;
  return TRUE;
}

int MapWindowPoints(HWND from, HWND to, LPPOINT pts, UINT n)
{
  std::lock_guard<std::recursive_mutex> lk(g_wm);
  int fx = 0, fy = 0, tx = 0, ty = 0;
  if (Wnd *w = wnd(from)) wnd_client_origin(w, &fx, &fy);
  if (Wnd *w = wnd(to)) wnd_client_origin(w, &tx, &ty);
  for (UINT i = 0; i < n; i++) { pts[i].x += fx - tx; pts[i].y += fy - ty; }
  return MAKELONG(fx - tx, fy - ty);
}

BOOL AdjustWindowRectEx(LPRECT r, DWORD style, BOOL menu, DWORD ex)
{
  if (!r) return FALSE;
  Wnd tmp;
  RECT c;
  nc_calc(&tmp, style, ex, menu != 0, 1000, 1000, &c);
  r->left -= c.left;
  r->top -= c.top;
  r->right += 1000 - c.right;
  r->bottom += 1000 - c.bottom;
  return TRUE;
}

BOOL AdjustWindowRect(LPRECT r, DWORD style, BOOL menu) { return AdjustWindowRectEx(r, style, menu, 0); }

/* ============================ window creation ============================ */
static int g_cascade = 0;

HWND CreateWindowEx(DWORD ex, LPCSTR clsName, LPCSTR name, DWORD style, int x, int y,
                    int cx, int cy, HWND hParent, HMENU hMenu, HINSTANCE, LPVOID param)
{
  user_init();
  Wnd *w = new Wnd();
  {
    std::lock_guard<std::recursive_mutex> lk(g_wm);
    WndClass *cls = find_class(clsName);
    if (!cls) { delete w; SetLastError(1407 /*ERROR_CANNOT_FIND_WND_CLASS*/); return 0; }
    w->cls = cls;
    w->proc = cls->proc;
    w->tid = cur_tid();
    Wnd *pw = wnd(hParent);
    if (style & WS_CHILD) {
      if (!pw) { delete w; SetLastError(1406 /*ERROR_TLW_WITH_WSCHILD*/); return 0; }
      w->parent = pw;
      w->id = (UINT_PTR)hMenu;
    } else {
      w->owner = toplevel(pw);
      w->menu = hMenu;
      if (!(style & WS_POPUP)) {
        style |= WS_CAPTION | WS_CLIPSIBLINGS;   /* overlapped windows always have a caption */
        ex |= WS_EX_WINDOWEDGE;
      }
      if ((style & WS_CAPTION) == WS_CAPTION || (style & (WS_DLGFRAME | WS_THICKFRAME)))
        ex |= WS_EX_WINDOWEDGE;
      /* default position and size */
      if (x == CW_USEDEFAULT) {
        if (style & WS_POPUP) { x = 0; y = 0; }
        else {
          x = 22 + 26 * (g_cascade % 8);
          y = 22 + 26 * (g_cascade % 8);
          g_cascade++;
        }
      }
      if (cx == CW_USEDEFAULT) {
        if (style & WS_POPUP) { cx = 0; cy = 0; }
        else { cx = g_screen_w * 3 / 4; cy = g_screen_h * 3 / 4; }
      }
    }
    if (ex & WS_EX_DLGMODALFRAME) ex |= WS_EX_WINDOWEDGE;
    w->style = style & ~WS_VISIBLE;   /* shown below, after WM_CREATE */
    w->exstyle = ex;
    w->rc = RECT{x, y, x + cx, y + cy};
    if (name && !IS_INTRESOURCE(name)) w->text = name;
    w->hwnd = (HWND)hnew(w, HT_WND);
    recalc_client(w);
    resize_surface(w);
    if (w->parent) w->parent->children.push_back(w);     /* bottom of the z-order */
    else {
      size_t pos = 0;
      if (!(ex & WS_EX_TOPMOST))
        while (pos < g_top.size() && (g_top[pos]->exstyle & WS_EX_TOPMOST)) pos++;
      g_top.insert(g_top.begin() + pos, w);
    }
  }
  HWND h = w->hwnd;

  CREATESTRUCT cs;
  memset(&cs, 0, sizeof(cs));
  cs.lpCreateParams = param;
  cs.hMenu = hMenu;
  cs.hwndParent = hParent;
  cs.x = x; cs.y = y; cs.cx = cx; cs.cy = cy;
  cs.style = (LONG)style;
  cs.lpszName = name;
  cs.lpszClass = clsName;
  cs.dwExStyle = ex;
  if (!call_proc(w, WM_NCCREATE, 0, (LPARAM)&cs)) { DestroyWindow(h); return 0; }
  if (!wnd(h)) return 0;
  {
    std::lock_guard<std::recursive_mutex> lk(g_wm);
    recalc_client(w);
  }
  if (call_proc(w, WM_CREATE, 0, (LPARAM)&cs) == -1) { DestroyWindow(h); return 0; }
  if (!wnd(h)) return 0;
  if (w->parent && !(ex & WS_EX_NOPARENTNOTIFY))
    send_msg(w->parent->hwnd, WM_PARENTNOTIFY, MAKEWPARAM(WM_CREATE, w->id), (LPARAM)h);
  if (!wnd(h)) return 0;
  if (style & WS_VISIBLE) {
    if (w->is_child()) {
      {
        std::lock_guard<std::recursive_mutex> lk(g_wm);
        w->style |= WS_VISIBLE;
        wnd_invalidate(w, nullptr, true);
        nc_paint(w);
        screen_dirty();
      }
      /* first WM_SIZE / WM_MOVE */
      if (!w->sentSizeMove) {
        w->sentSizeMove = true;
        RECT c;
        GetClientRect(h, &c);
        call_proc(w, WM_SIZE, SIZE_RESTORED, MAKELPARAM(c.right, c.bottom));
        call_proc(w, WM_MOVE, 0, MAKELPARAM(w->rc.left + w->client.left, w->rc.top + w->client.top));
      }
    } else {
      ShowWindow(h, SW_SHOW);
    }
  }
  return wnd(h) ? h : 0;
}

/* ============================ window destruction ========================= */
static void unlink_wnd(Wnd *w)
{
  if (w->parent) {
    auto &c = w->parent->children;
    c.erase(std::remove(c.begin(), c.end(), w), c.end());
  } else {
    g_top.erase(std::remove(g_top.begin(), g_top.end(), w), g_top.end());
  }
}

static void send_destroy_recursive(Wnd *w)
{
  HWND h = w->hwnd;
  call_proc(w, WM_DESTROY, 0, 0);
  if (!wnd(h)) return;
  std::vector<HWND> kids;
  {
    std::lock_guard<std::recursive_mutex> lk(g_wm);
    for (Wnd *c : w->children) kids.push_back(c->hwnd);
  }
  for (HWND k : kids)
    if (Wnd *c = wnd(k)) { c->destroying = true; send_destroy_recursive(c); }
}

static void send_ncdestroy_recursive(Wnd *w)
{
  std::vector<HWND> kids;
  {
    std::lock_guard<std::recursive_mutex> lk(g_wm);
    for (Wnd *c : w->children) kids.push_back(c->hwnd);
  }
  for (HWND k : kids)
    if (Wnd *c = wnd(k)) send_ncdestroy_recursive(c);
  call_proc(w, WM_NCDESTROY, 0, 0);
  std::lock_guard<std::recursive_mutex> lk(g_wm);
  /* drop timers and queued messages for this window */
  for (auto &kv : g_tqs) {
    TQ *q = kv.second;
    q->timers.erase(std::remove_if(q->timers.begin(), q->timers.end(),
                                   [&](const Timer &t) { return t.h == w->hwnd; }),
                    q->timers.end());
    q->posted.erase(std::remove_if(q->posted.begin(), q->posted.end(),
                                   [&](const MSG &m) { return m.hwnd == w->hwnd; }),
                    q->posted.end());
  }
  if (w->ctl && w->ctl_free) { w->ctl_free(w->ctl); w->ctl = nullptr; }
  unlink_wnd(w);
  if (g_focus == w->hwnd) g_focus = 0;
  if (g_capture == w->hwnd) g_capture = 0;
  if (g_active == w->hwnd) g_active = 0;
  hdel(w->hwnd);
  /* the Wnd itself is kept as a zombie: a DC may still refer to its surface */
  w->children.clear();
  screen_dirty();
}

static void activate_next(Wnd *gone);

BOOL DestroyWindow(HWND h)
{
  Wnd *w = wnd(h);
  if (!w || w->destroying) return FALSE;
  w->destroying = true;
  /* owned windows go first */
  std::vector<HWND> owned;
  {
    std::lock_guard<std::recursive_mutex> lk(g_wm);
    for (Wnd *t : g_top) if (t->owner == w) owned.push_back(t->hwnd);
  }
  for (HWND o : owned) DestroyWindow(o);
  bool wasActive = false;
  {
    std::lock_guard<std::recursive_mutex> lk(g_wm);
    wasActive = (g_active == h);
  }
  if (w->visible())
    SetWindowPos(h, 0, 0, 0, 0, 0, SWP_HIDEWINDOW | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                 SWP_NOACTIVATE);
  {
    std::lock_guard<std::recursive_mutex> lk(g_wm);
    /* focus inside the dying window moves away */
    for (Wnd *f = wnd(g_focus); f; f = f->parent)
      if (f == w) { g_focus = w->parent ? w->parent->hwnd : 0; break; }
    if (g_capture == h) g_capture = 0;
  }
  if (w->parent && !(w->exstyle & WS_EX_NOPARENTNOTIFY))
    send_msg(w->parent->hwnd, WM_PARENTNOTIFY, MAKEWPARAM(WM_DESTROY, w->id), (LPARAM)h);
  send_destroy_recursive(w);
  send_ncdestroy_recursive(w);
  if (wasActive) activate_next(w);
  return TRUE;
}

BOOL IsWindow(HWND h) { return wnd(h) != nullptr; }

/* =========================== activation and focus ======================== */
/* Owned top-level windows always stay above their owner: after the owner
   moved in the z-order, put them (recursively, keeping their relative
   order) directly above it.  Caller holds g_wm. */
static void raise_owned(Wnd *w)
{
  std::vector<Wnd *> ownedw;
  for (Wnd *t : g_top) if (t->owner == w) ownedw.push_back(t);
  if (ownedw.empty()) return;
  for (Wnd *t : ownedw) g_top.erase(std::remove(g_top.begin(), g_top.end(), t), g_top.end());
  auto it = std::find(g_top.begin(), g_top.end(), w);
  size_t pos = it == g_top.end() ? 0 : (size_t)(it - g_top.begin());
  for (size_t k = 0; k < ownedw.size(); k++) g_top.insert(g_top.begin() + pos + k, ownedw[k]);
  for (Wnd *t : ownedw) raise_owned(t);
}

static void set_active(Wnd *w, int how)
{
  HWND old, nh = w ? w->hwnd : 0;
  {
    std::lock_guard<std::recursive_mutex> lk(g_wm);
    old = g_active;
    if (old == nh) {
      return;
    }
    g_active = nh;
    if (trace('a')) {
      Wnd *o = wnd(old);
      host_log("activate %p '%s' (tid %u) -> %p '%s' (tid %u), caller tid %u", (void *)old,
               o ? o->text.c_str() : "", o ? (unsigned)o->tid : 0, (void *)nh, w ? w->text.c_str() : "",
               w ? (unsigned)w->tid : 0, (unsigned)GetCurrentThreadId());
    }
    if (Wnd *o = wnd(old)) { o->active = false; nc_paint(o); }
    if (w) {
      w->active = true;
      /* bring to the top of the z-order, owned windows above it */
      g_top.erase(std::remove(g_top.begin(), g_top.end(), w), g_top.end());
      size_t pos = 0;
      if (!(w->exstyle & WS_EX_TOPMOST))
        while (pos < g_top.size() && (g_top[pos]->exstyle & WS_EX_TOPMOST)) pos++;
      g_top.insert(g_top.begin() + pos, w);
      raise_owned(w);
      nc_paint(w);
    }
    screen_dirty();
  }
  if (Wnd *o = wnd(old)) {
    send_msg(old, WM_NCACTIVATE, FALSE, 0);
    if (wnd(old)) send_msg(old, WM_ACTIVATE, MAKEWPARAM(WA_INACTIVE, o->minimized), (LPARAM)nh);
  }
  if (w && wnd(nh)) {
    send_msg(nh, WM_NCACTIVATE, TRUE, 0);
    if (wnd(nh)) send_msg(nh, WM_ACTIVATE, MAKEWPARAM(how, w->minimized), (LPARAM)old);
  }
}

static void activate_next(Wnd *gone)
{
  Wnd *next = nullptr;
  {
    std::lock_guard<std::recursive_mutex> lk(g_wm);
    if (gone && gone->owner && wnd(gone->owner->hwnd) && gone->owner->visible() &&
        gone->owner->enabled())
      next = gone->owner;
    else
      for (Wnd *t : g_top)
        if (t != gone && t->visible() && !t->minimized && t->enabled() &&
            t->cls->name != "#32768" && t->cls->name != "ComboLBox") { next = t; break; }
  }
  if (next) set_active(next, WA_ACTIVE);
}

HWND SetActiveWindow(HWND h)
{
  Wnd *w = wnd(h);
  HWND old;
  {
    std::lock_guard<std::recursive_mutex> lk(g_wm);
    old = g_active;
    w = toplevel(w);
  }
  if (w) set_active(w, WA_ACTIVE);
  return old;
}

HWND GetActiveWindow(void) { std::lock_guard<std::recursive_mutex> lk(g_wm); return g_active; }
BOOL SetForegroundWindow(HWND h) { SetActiveWindow(h); return TRUE; }
HWND GetForegroundWindow(void) { return GetActiveWindow(); }

HWND SetFocus(HWND h)
{
  Wnd *w = wnd(h);
  HWND old;
  {
    std::lock_guard<std::recursive_mutex> lk(g_wm);
    old = g_focus;
    if (h && !w) return 0;
    if (old == h) return old;
  }
  if (w) {
    Wnd *top;
    {
      std::lock_guard<std::recursive_mutex> lk(g_wm);
      top = toplevel(w);
      if (top->minimized) return old;
    }
    if (top && g_active != top->hwnd) set_active(top, WA_ACTIVE);
    {
      std::lock_guard<std::recursive_mutex> lk(g_wm);
      if (g_focus == h) return old;    /* activation already focused it */
      old = g_focus;
      g_focus = h;
    }
  } else {
    std::lock_guard<std::recursive_mutex> lk(g_wm);
    g_focus = 0;
  }
  if (wnd(old)) send_msg(old, WM_KILLFOCUS, (WPARAM)h, 0);
  if (h && wnd(h) && g_focus == h) send_msg(h, WM_SETFOCUS, (WPARAM)old, 0);
  return old;
}

HWND GetFocus(void) { std::lock_guard<std::recursive_mutex> lk(g_wm); return g_focus; }

HWND SetCapture(HWND h)
{
  std::lock_guard<std::recursive_mutex> lk(g_wm);
  HWND old = g_capture;
  g_capture = wnd(h) ? h : 0;
  return old;
}

BOOL ReleaseCapture(void)
{
  HWND old;
  {
    std::lock_guard<std::recursive_mutex> lk(g_wm);
    old = g_capture;
    g_capture = 0;
  }
  if (wnd(old)) send_msg(old, WM_CAPTURECHANGED, 0, 0);
  return TRUE;
}

HWND GetCapture(void) { std::lock_guard<std::recursive_mutex> lk(g_wm); return g_capture; }

BOOL EnableWindow(HWND h, BOOL en)
{
  Wnd *w = wnd(h);
  if (!w) return FALSE;
  bool wasDisabled;
  {
    std::lock_guard<std::recursive_mutex> lk(g_wm);
    wasDisabled = !w->enabled();
    if (en) w->style &= ~WS_DISABLED;
    else w->style |= WS_DISABLED;
    if (wasDisabled == !en) return wasDisabled;
    wnd_invalidate(w, nullptr, true);
    nc_paint(w);
  }
  if (!en) {
    bool focusInside = false;
    {
      std::lock_guard<std::recursive_mutex> lk(g_wm);
      for (Wnd *f = wnd(g_focus); f; f = f->parent) if (f == w) focusInside = true;
    }
    if (focusInside) SetFocus(0);
  }
  send_msg(h, WM_ENABLE, en, 0);
  return wasDisabled;
}

BOOL IsWindowEnabled(HWND h) { Wnd *w = wnd(h); return w && w->enabled(); }
BOOL IsWindowVisible(HWND h)
{
  std::lock_guard<std::recursive_mutex> lk(g_wm);
  Wnd *w = wnd(h);
  for (; w; w = w->parent) if (!w->visible()) return FALSE;
  return h && wnd(h) ? TRUE : FALSE;
}
BOOL IsIconic(HWND h) { Wnd *w = wnd(h); return w && w->minimized; }
BOOL IsZoomed(HWND h) { Wnd *w = wnd(h); return w && w->maximized; }

/* ========================== position and z-order ========================= */
BOOL SetWindowPos(HWND h, HWND after, int x, int y, int cx, int cy, UINT flags)
{
  Wnd *w = wnd(h);
  if (!w) return FALSE;
  WINDOWPOS wp = {h, after, x, y, cx, cy, flags};
  if (!(flags & SWP_NOSENDCHANGING)) {
    send_msg(h, WM_WINDOWPOSCHANGING, 0, (LPARAM)&wp);
    if (!wnd(h)) return FALSE;
  }
  flags = wp.flags;
  bool moved = false, sized = false, shown = false, hidden = false;
  RECT oldrc;
  {
    std::lock_guard<std::recursive_mutex> lk(g_wm);
    oldrc = w->rc;
    RECT nr = w->rc;
    if (!(flags & SWP_NOMOVE)) {
      nr.right = wp.x + (nr.right - nr.left);
      nr.bottom = wp.y + (nr.bottom - nr.top);
      nr.left = wp.x;
      nr.top = wp.y;
    }
    if (!(flags & SWP_NOSIZE)) {
      nr.right = nr.left + std::max(0, wp.cx);
      nr.bottom = nr.top + std::max(0, wp.cy);
    }
    moved = nr.left != oldrc.left || nr.top != oldrc.top;
    sized = (nr.right - nr.left) != (oldrc.right - oldrc.left) ||
            (nr.bottom - nr.top) != (oldrc.bottom - oldrc.top);
    w->rc = nr;
    if (sized || (flags & SWP_FRAMECHANGED)) {
      RECT oc = w->client;
      recalc_client(w);
      if (oc.left != w->client.left || oc.top != w->client.top ||
          oc.right - oc.left != w->client.right - w->client.left ||
          oc.bottom - oc.top != w->client.bottom - w->client.top)
        sized = true;
      resize_surface(w);
    }
    if ((flags & SWP_SHOWWINDOW) && !w->visible()) { w->style |= WS_VISIBLE; shown = true; }
    if ((flags & SWP_HIDEWINDOW) && w->visible()) { w->style &= ~WS_VISIBLE; hidden = true; }
    if (!(flags & SWP_NOZORDER)) {
      std::vector<Wnd *> &list = w->parent ? w->parent->children : g_top;
      list.erase(std::remove(list.begin(), list.end(), w), list.end());
      if (wp.hwndInsertAfter == HWND_BOTTOM) list.push_back(w);
      else if (wp.hwndInsertAfter == HWND_TOP || wp.hwndInsertAfter == HWND_TOPMOST ||
               wp.hwndInsertAfter == HWND_NOTOPMOST) {
        if (wp.hwndInsertAfter == HWND_TOPMOST) w->exstyle |= WS_EX_TOPMOST;
        if (wp.hwndInsertAfter == HWND_NOTOPMOST) w->exstyle &= ~WS_EX_TOPMOST;
        size_t pos = 0;
        if (!w->parent && !(w->exstyle & WS_EX_TOPMOST))
          while (pos < list.size() && (list[pos]->exstyle & WS_EX_TOPMOST)) pos++;
        list.insert(list.begin() + pos, w);
      } else {
        Wnd *a = wnd(wp.hwndInsertAfter);
        auto it = std::find(list.begin(), list.end(), a);
        if (it == list.end()) list.insert(list.begin(), w);
        else list.insert(it + 1, w);
      }
    }
    if (!(flags & SWP_NOZORDER) && !w->parent) raise_owned(w);
    if (sized || shown || (flags & SWP_FRAMECHANGED)) {
      wnd_invalidate(w, nullptr, true);
      nc_paint(w);
    }
    /* windows below may need repainting where this one was */
    if (moved || sized || hidden || shown) screen_dirty();
  }
  /* As in Windows: without SWP_NOACTIVATE, a top-level window is activated
     (SetForegroundWindow) even if it was already visible. */
  if (!w->is_child() && w->visible() && !(flags & (SWP_NOACTIVATE | SWP_HIDEWINDOW)))
    set_active(w, WA_ACTIVE);
  if (hidden) {
    bool wasActive;
    {
      std::lock_guard<std::recursive_mutex> lk(g_wm);
      wasActive = g_active == h;
      bool focusInside = false;
      for (Wnd *f = wnd(g_focus); f; f = f->parent) if (f == w) focusInside = true;
      if (focusInside && w->is_child()) g_focus = 0;
    }
    if (wasActive && !(flags & SWP_NOACTIVATE)) activate_next(w);
    else if (wasActive) {
      std::lock_guard<std::recursive_mutex> lk(g_wm);
      g_active = 0;
      w->active = false;
    }
  }
  if (!wnd(h)) return TRUE;
  wp.flags = flags;
  if (!moved) wp.flags |= SWP_NOMOVE;
  if (!sized) wp.flags |= SWP_NOSIZE;
  wp.x = w->rc.left; wp.y = w->rc.top;
  wp.cx = w->rc.right - w->rc.left; wp.cy = w->rc.bottom - w->rc.top;
  send_msg(h, WM_WINDOWPOSCHANGED, 0, (LPARAM)&wp);
  return TRUE;
}

BOOL MoveWindow(HWND h, int x, int y, int cx, int cy, BOOL repaint)
{
  return SetWindowPos(h, 0, x, y, cx, cy, SWP_NOZORDER | SWP_NOACTIVATE | (repaint ? 0 : SWP_NOREDRAW));
}

BOOL BringWindowToTop(HWND h) { return SetWindowPos(h, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE); }

BOOL ShowWindow(HWND h, int cmd)
{
  Wnd *w = wnd(h);
  if (!w) return FALSE;
  bool was = w->visible();
  switch (cmd) {
  case SW_HIDE:
    if (!was) return FALSE;
    send_msg(h, WM_SHOWWINDOW, FALSE, 0);
    SetWindowPos(h, 0, 0, 0, 0, 0, SWP_HIDEWINDOW | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                 (w->is_child() ? SWP_NOACTIVATE : 0));
    return was;
  case SW_MINIMIZE: case SW_SHOWMINIMIZED: case SW_SHOWMINNOACTIVE: {
    if (!was) send_msg(h, WM_SHOWWINDOW, TRUE, 0);
    {
      std::lock_guard<std::recursive_mutex> lk(g_wm);
      if (!w->minimized) {
        w->restoreToMax = w->maximized;
        if (!w->maximized) w->rcNormal = w->rc;
        w->minimized = true;
        w->maximized = false;
        w->style &= ~WS_MAXIMIZE;
      }
      w->style |= WS_VISIBLE | WS_MINIMIZE;
      screen_dirty();
    }
    if (!w->is_child()) {
      bool active;
      { std::lock_guard<std::recursive_mutex> lk(g_wm); active = g_active == h; }
      if (active) activate_next(w);
      host_notify_minimized();
    }
    send_msg(h, WM_SIZE, SIZE_MINIMIZED, 0);
    return was;
  }
  case SW_MAXIMIZE: {
    {
      std::lock_guard<std::recursive_mutex> lk(g_wm);
      if (!w->maximized) w->rcNormal = w->rc;
      w->maximized = true;
      w->minimized = false;
      w->style |= WS_MAXIMIZE;
      w->style &= ~WS_MINIMIZE;
    }
    if (!was) send_msg(h, WM_SHOWWINDOW, TRUE, 0);
    int bx = nc_border(w->style, w->exstyle);
    SetWindowPos(h, HWND_TOP, -bx, -bx, g_screen_w + 2 * bx, g_screen_h + 2 * bx, SWP_SHOWWINDOW | SWP_FRAMECHANGED);
    return was;
  }
  case SW_RESTORE: case SW_SHOWNORMAL: {
    if (cmd == SW_RESTORE && w->minimized && w->restoreToMax) {
      /* a window minimised while maximised comes back maximised */
      {
        std::lock_guard<std::recursive_mutex> lk(g_wm);
        w->minimized = false;
        w->restoreToMax = false;
        w->style &= ~WS_MINIMIZE;
      }
      return ShowWindow(h, SW_MAXIMIZE);
    }
    bool wasMin = w->minimized, wasMax = w->maximized;
    RECT r = w->rcNormal;
    {
      std::lock_guard<std::recursive_mutex> lk(g_wm);
      w->minimized = false;
      w->maximized = false;
      w->style &= ~(WS_MINIMIZE | WS_MAXIMIZE);
      screen_dirty();
    }
    if (!was) send_msg(h, WM_SHOWWINDOW, TRUE, 0);
    if (wasMin || wasMax)
      SetWindowPos(h, HWND_TOP, r.left, r.top, r.right - r.left, r.bottom - r.top,
                   SWP_SHOWWINDOW | SWP_FRAMECHANGED);
    else
      SetWindowPos(h, HWND_TOP, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOSIZE);
    break;
  }
  case SW_SHOWNA: case SW_SHOWNOACTIVATE:
    if (!was) send_msg(h, WM_SHOWWINDOW, TRUE, 0);
    SetWindowPos(h, 0, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    break;
  default:   /* SW_SHOW, SW_SHOWDEFAULT, ... */
    if (was && cmd == SW_SHOW) return was;     /* already visible: no change */
    if (!was) send_msg(h, WM_SHOWWINDOW, TRUE, 0);
    SetWindowPos(h, 0, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOSIZE |
                 (w->is_child() ? SWP_NOZORDER | SWP_NOACTIVATE : 0));
    break;
  }
  if (!wnd(h)) return was;
  if (!w->sentSizeMove) {
    w->sentSizeMove = true;
    RECT c;
    GetClientRect(h, &c);
    send_msg(h, WM_SIZE, SIZE_RESTORED, MAKELPARAM(c.right, c.bottom));
    int ox = w->rc.left + w->client.left, oy = w->rc.top + w->client.top;
    send_msg(h, WM_MOVE, 0, MAKELPARAM(ox, oy));
  }
  return was;
}

BOOL SetWindowPlacement(HWND h, const WINDOWPLACEMENT *wp)
{
  Wnd *w = wnd(h);
  if (!w || !wp) return FALSE;
  {
    std::lock_guard<std::recursive_mutex> lk(g_wm);
    w->rcNormal = wp->rcNormalPosition;
  }
  switch (wp->showCmd) {
  case SW_MINIMIZE: case SW_SHOWMINIMIZED: case SW_SHOWMINNOACTIVE:
    ShowWindow(h, SW_MINIMIZE);
    break;
  case SW_SHOWMAXIMIZED:
    ShowWindow(h, SW_MAXIMIZE);
    break;
  default: {
    RECT r = wp->rcNormalPosition;
    SetWindowPos(h, 0, r.left, r.top, r.right - r.left, r.bottom - r.top, SWP_NOZORDER);
    if (wp->showCmd != SW_HIDE) ShowWindow(h, wp->showCmd);
  }
  }
  return TRUE;
}

BOOL GetWindowPlacement(HWND h, WINDOWPLACEMENT *wp)
{
  Wnd *w = wnd(h);
  if (!w || !wp) return FALSE;
  wp->flags = 0;
  wp->showCmd = w->minimized ? SW_SHOWMINIMIZED : (w->maximized ? SW_SHOWMAXIMIZED : SW_SHOWNORMAL);
  wp->rcNormalPosition = (w->minimized || w->maximized) ? w->rcNormal : w->rc;
  return TRUE;
}

/* ============================== window data ============================== */
LONG_PTR GetWindowLong(HWND h, int idx)
{
  Wnd *w = wnd(h);
  if (!w) { SetLastError(1400); return 0; }
  switch (idx) {
  case GWL_STYLE: return (LONG_PTR)(DWORD)w->style;
  case GWL_EXSTYLE: return (LONG_PTR)(DWORD)w->exstyle;
  case GWL_USERDATA: return w->user;
  case GWL_ID: return (LONG_PTR)w->id;
  case GWL_WNDPROC: return (LONG_PTR)w->proc;
  case GWL_HWNDPARENT: return (LONG_PTR)(w->parent ? w->parent->hwnd : (w->owner ? w->owner->hwnd : 0));
  case GWL_HINSTANCE: return (LONG_PTR)0x00400000;
  default: return 0;
  }
}

LONG_PTR SetWindowLong(HWND h, int idx, LONG_PTR v)
{
  Wnd *w = wnd(h);
  if (!w) { SetLastError(1400); return 0; }
  std::lock_guard<std::recursive_mutex> lk(g_wm);
  LONG_PTR old = 0;
  switch (idx) {
  case GWL_STYLE:
    old = (LONG_PTR)(DWORD)w->style;
    /* WS_VISIBLE is not changed this way in practice; keep the bit */
    w->style = ((DWORD)v & ~WS_VISIBLE) | (w->style & WS_VISIBLE);
    break;
  case GWL_EXSTYLE: old = w->exstyle; w->exstyle = (DWORD)v; break;
  case GWL_USERDATA: old = w->user; w->user = v; break;
  case GWL_ID: old = (LONG_PTR)w->id; w->id = (UINT_PTR)v; break;
  case GWL_WNDPROC: old = (LONG_PTR)w->proc; w->proc = (WNDPROC)v; break;
  default: break;
  }
  return old;
}

BOOL SetWindowText(HWND h, LPCSTR s) { return (BOOL)send_msg(h, WM_SETTEXT, 0, (LPARAM)s); }
int GetWindowText(HWND h, LPSTR buf, int n) { return (int)send_msg(h, WM_GETTEXT, n, (LPARAM)buf); }
int GetWindowTextLength(HWND h) { return (int)send_msg(h, WM_GETTEXTLENGTH, 0, 0); }

/* ============================= window lookup ============================= */
HWND GetParent(HWND h)
{
  std::lock_guard<std::recursive_mutex> lk(g_wm);
  Wnd *w = wnd(h);
  if (!w) return 0;
  if (w->style & WS_CHILD) return w->parent ? w->parent->hwnd : 0;
  if (w->style & WS_POPUP) return w->owner ? w->owner->hwnd : 0;
  return 0;
}

HWND SetParent(HWND h, HWND p)
{
  std::lock_guard<std::recursive_mutex> lk(g_wm);
  Wnd *w = wnd(h), *np = wnd(p);
  if (!w) return 0;
  HWND old = w->parent ? w->parent->hwnd : 0;
  if (np && (w->style & WS_CHILD)) {
    unlink_wnd(w);
    w->parent = np;
    np->children.insert(np->children.begin(), w);
    screen_dirty();
  }
  return old;
}

HWND GetWindow(HWND h, UINT cmd)
{
  std::lock_guard<std::recursive_mutex> lk(g_wm);
  Wnd *w = wnd(h);
  if (!w) return 0;
  std::vector<Wnd *> &sib = w->parent ? w->parent->children : g_top;
  auto it = std::find(sib.begin(), sib.end(), w);
  switch (cmd) {
  case GW_HWNDFIRST: return sib.empty() ? 0 : sib.front()->hwnd;
  case GW_HWNDLAST:  return sib.empty() ? 0 : sib.back()->hwnd;
  case GW_HWNDNEXT:  return (it == sib.end() || it + 1 == sib.end()) ? 0 : (*(it + 1))->hwnd;
  case GW_HWNDPREV:  return (it == sib.end() || it == sib.begin()) ? 0 : (*(it - 1))->hwnd;
  case GW_OWNER:     return w->owner ? w->owner->hwnd : 0;
  case GW_CHILD:     return w->children.empty() ? 0 : w->children.front()->hwnd;
  default:           return 0;
  }
}

HWND GetTopWindow(HWND h)
{
  std::lock_guard<std::recursive_mutex> lk(g_wm);
  if (!h) return g_top.empty() ? 0 : g_top.front()->hwnd;
  Wnd *w = wnd(h);
  if (!w || w->children.empty()) return 0;
  return w->children.front()->hwnd;
}

HWND GetDesktopWindow(void) { return 0; }

HWND GetDlgItem(HWND h, int id)
{
  std::lock_guard<std::recursive_mutex> lk(g_wm);
  Wnd *w = wnd(h);
  if (!w) return 0;
  for (Wnd *c : w->children) if ((int)c->id == id) return c->hwnd;
  return 0;
}

int GetDlgCtrlID(HWND h) { Wnd *w = wnd(h); return w ? (int)w->id : 0; }

BOOL IsChild(HWND p, HWND c)
{
  std::lock_guard<std::recursive_mutex> lk(g_wm);
  Wnd *w = wnd(c);
  for (w = w ? w->parent : nullptr; w; w = w->parent) if (w->hwnd == p) return TRUE;
  return FALSE;
}

namespace w32 {
/* Deepest visible window at a screen point (children included). */
Wnd *window_at(int sx, int sy, bool includeDisabledChildren)
{
  for (Wnd *t : g_top) {
    if (!t->visible() || t->minimized) continue;
    int x, y;
    wnd_screen_origin(t, &x, &y);
    if (sx < x || sy < y || sx >= x + (t->rc.right - t->rc.left) || sy >= y + (t->rc.bottom - t->rc.top))
      continue;
    Wnd *cur = t;
    for (;;) {
      int cx, cy;
      wnd_client_origin(cur, &cx, &cy);
      int lx = sx - cx, ly = sy - cy;
      if (lx < 0 || ly < 0 || lx >= cur->client.right - cur->client.left ||
          ly >= cur->client.bottom - cur->client.top)
        break;
      Wnd *hit = nullptr;
      for (Wnd *c : cur->children) {
        if (!c->visible()) continue;
        if (lx >= c->rc.left && lx < c->rc.right && ly >= c->rc.top && ly < c->rc.bottom) {
          /* transparent static controls and group boxes don't take the mouse */
          if (c->cls->name == "BUTTON" && ((c->style & BS_TYPEMASK) == BS_GROUPBOX)) continue;
          if (!includeDisabledChildren && !c->enabled()) { hit = c; break; }
          hit = c;
          break;
        }
      }
      if (!hit) break;
      cur = hit;
    }
    return cur;
  }
  return nullptr;
}
}  /* namespace w32 */

HWND WindowFromPoint(POINT p)
{
  std::lock_guard<std::recursive_mutex> lk(g_wm);
  Wnd *w = window_at(p.x, p.y, true);
  return w ? w->hwnd : 0;
}

HWND ChildWindowFromPoint(HWND h, POINT p)
{
  std::lock_guard<std::recursive_mutex> lk(g_wm);
  Wnd *w = wnd(h);
  if (!w) return 0;
  for (Wnd *c : w->children)
    if (p.x >= c->rc.left && p.x < c->rc.right && p.y >= c->rc.top && p.y < c->rc.bottom)
      return c->hwnd;
  return h;
}

/* ================================ painting =============================== */
BOOL InvalidateRect(HWND h, const RECT *r, BOOL erase)
{
  if (!h) {   /* the whole desktop */
    std::lock_guard<std::recursive_mutex> lk(g_wm);
    std::function<void(Wnd *)> all = [&](Wnd *w) {
      wnd_invalidate(w, nullptr, true);
      for (Wnd *c : w->children) all(c);
    };
    for (Wnd *t : g_top) all(t);
    return TRUE;
  }
  std::lock_guard<std::recursive_mutex> lk(g_wm);
  Wnd *w = wnd(h);
  if (!w) return FALSE;
  wnd_invalidate(w, r, erase != 0);
  return TRUE;
}

BOOL ValidateRect(HWND h, const RECT *r)
{
  std::lock_guard<std::recursive_mutex> lk(g_wm);
  Wnd *w = wnd(h);
  if (!w) return FALSE;
  if (!r) { w->hasUpdate = false; w->erase = false; return TRUE; }
  /* only full validation of the bounding box is supported */
  if (w->hasUpdate && r->left <= w->upd.left && r->top <= w->upd.top &&
      r->right >= w->upd.right && r->bottom >= w->upd.bottom)
    w->hasUpdate = false;
  return TRUE;
}

BOOL UpdateWindow(HWND h)
{
  Wnd *w = wnd(h);
  if (!w) return FALSE;
  if (w->hasUpdate && w->visible() && w->tid == cur_tid()) call_proc(w, WM_PAINT, 0, 0);
  return TRUE;
}

BOOL RedrawWindow(HWND h, const RECT *r, HRGN, UINT flags)
{
  if (flags & RDW_INVALIDATE) InvalidateRect(h, r, (flags & RDW_ERASE) != 0);
  if (flags & RDW_FRAME) { std::lock_guard<std::recursive_mutex> lk(g_wm); if (Wnd *w = wnd(h)) nc_paint(w); }
  if (flags & RDW_UPDATENOW) UpdateWindow(h);
  return TRUE;
}

namespace w32 {
HDC dc_new_window(HWND hwnd, bool client, bool paint, const RECT *paintRect);
HDC dc_new_screen();
void dc_free(HDC h);

bool wnd_target(HWND h, bool client, bool paint, const RECT &pr, Target &t)
{
  std::lock_guard<std::recursive_mutex> lk(g_wm);
  Wnd *w = wnd(h);
  if (!w || !w->surf) return false;
  t.hold = w->surf;
  t.s = t.hold.get();
  RECT area;
  if (client) {
    t.ox = w->client.left;
    t.oy = w->client.top;
    area = w->client;
  } else {
    t.ox = 0;
    t.oy = 0;
    area = RECT{0, 0, t.s->w, t.s->h};
  }
  if (paint) {
    RECT p = {pr.left + t.ox, pr.top + t.oy, pr.right + t.ox, pr.bottom + t.oy};
    RECT i;
    if (!IntersectRect(&i, &p, &area)) return false;
    area = i;
  }
  RECT s = {0, 0, t.s->w, t.s->h};
  RECT i;
  if (!IntersectRect(&i, &area, &s)) return false;
  t.clips.push_back(i);
  return true;
}
}  /* namespace w32 */

HDC GetDC(HWND h)
{
  user_init();
  if (!h) return dc_new_screen();
  if (!wnd(h)) return 0;
  return dc_new_window(h, true, false, nullptr);
}

HDC GetWindowDC(HWND h)
{
  user_init();
  if (!h) return dc_new_screen();
  if (!wnd(h)) return 0;
  return dc_new_window(h, false, false, nullptr);
}

int ReleaseDC(HWND, HDC hdc)
{
  DCObj *dc = dc_from(hdc);
  if (!dc || dc->kind == DCObj::MEM) return 0;
  dc_free(hdc);
  return 1;
}

HDC BeginPaint(HWND h, LPPAINTSTRUCT ps)
{
  Wnd *w = wnd(h);
  if (!w || !ps) return 0;
  RECT r;
  bool erase;
  {
    std::lock_guard<std::recursive_mutex> lk(g_wm);
    if (w->hasUpdate) r = w->upd;
    else r = RECT{0, 0, 0, 0};
    erase = w->erase;
    w->hasUpdate = false;
    w->erase = false;
  }
  memset(ps, 0, sizeof(*ps));
  ps->rcPaint = r;
  HDC dc = dc_new_window(h, true, true, &r);
  ps->hdc = dc;
  if (erase) ps->fErase = !send_msg(h, WM_ERASEBKGND, (WPARAM)dc, 0);
  return dc;
}

BOOL EndPaint(HWND, const PAINTSTRUCT *ps)
{
  if (ps && ps->hdc) dc_free(ps->hdc);
  return TRUE;
}

/* =============================== rectangles ============================== */
BOOL SetRect(LPRECT r, int l, int t, int rr, int b) { if (!r) return FALSE; r->left = l; r->top = t; r->right = rr; r->bottom = b; return TRUE; }
BOOL SetRectEmpty(LPRECT r) { return SetRect(r, 0, 0, 0, 0); }
BOOL CopyRect(LPRECT d, const RECT *s) { if (!d || !s) return FALSE; *d = *s; return TRUE; }
BOOL InflateRect(LPRECT r, int dx, int dy) { if (!r) return FALSE; r->left -= dx; r->right += dx; r->top -= dy; r->bottom += dy; return TRUE; }
BOOL OffsetRect(LPRECT r, int dx, int dy) { if (!r) return FALSE; r->left += dx; r->right += dx; r->top += dy; r->bottom += dy; return TRUE; }
BOOL IsRectEmpty(const RECT *r) { return !r || r->left >= r->right || r->top >= r->bottom; }
BOOL PtInRect(const RECT *r, POINT p) { return r && p.x >= r->left && p.x < r->right && p.y >= r->top && p.y < r->bottom; }
BOOL EqualRect(const RECT *a, const RECT *b) { return a && b && !memcmp(a, b, sizeof(RECT)); }
BOOL IntersectRect(LPRECT d, const RECT *a, const RECT *b)
{
  RECT r = { std::max(a->left, b->left), std::max(a->top, b->top),
             std::min(a->right, b->right), std::min(a->bottom, b->bottom) };
  if (r.left >= r.right || r.top >= r.bottom) { SetRectEmpty(d); return FALSE; }
  *d = r;
  return TRUE;
}
BOOL UnionRect(LPRECT d, const RECT *a, const RECT *b)
{
  if (IsRectEmpty(a)) { *d = *b; return !IsRectEmpty(b); }
  if (IsRectEmpty(b)) { *d = *a; return TRUE; }
  RECT r = { std::min(a->left, b->left), std::min(a->top, b->top),
             std::max(a->right, b->right), std::max(a->bottom, b->bottom) };
  *d = r;
  return TRUE;
}

/* ============================ system parameters ========================== */
int GetSystemMetrics(int i)
{
  switch (i) {
  case SM_CXSCREEN: case SM_CXFULLSCREEN: case SM_CXMAXIMIZED: return g_screen_w;
  case SM_CYSCREEN: case SM_CYMAXIMIZED: return g_screen_h;
  case SM_CYFULLSCREEN: return g_screen_h - M_CYCAPTION;
  case SM_CXVSCROLL: case SM_CXHTHUMB: case SM_CXHSCROLL: return M_CXVSCROLL;
  case SM_CYHSCROLL: case SM_CYVTHUMB: case SM_CYVSCROLL: return M_CYHSCROLL;
  case SM_CYCAPTION: return M_CYCAPTION;
  case SM_CXBORDER: case SM_CYBORDER: return M_CXBORDER;
  case SM_CXDLGFRAME: case SM_CYDLGFRAME: return M_CXDLGFRAME;
  case SM_CXFRAME: case SM_CYFRAME: return M_CXFRAME;
  case SM_CXICON: case SM_CYICON: case SM_CXCURSOR: case SM_CYCURSOR: return 32;
  case SM_CXSMICON: case SM_CYSMICON: return 16;
  case SM_CYMENU: return M_CYMENU;
  case SM_CXSIZE: case SM_CYSIZE: return M_CXSIZE;
  case SM_CXMIN: return 112;
  case SM_CYMIN: return 27;
  case SM_CXMINTRACK: return 112;
  case SM_CYMINTRACK: return 27;
  case SM_CXDOUBLECLK: case SM_CYDOUBLECLK: return 4;
  case SM_CXEDGE: case SM_CYEDGE: return M_CXEDGE;
  case SM_CYSMCAPTION: return 16;
  case SM_CXMENUSIZE: case SM_CYMENUSIZE: return 18;
  case SM_CXMENUCHECK: case SM_CYMENUCHECK: return 13;
  case SM_CMOUSEBUTTONS: return 3;
  case SM_MOUSEPRESENT: return 1;
  case SM_SWAPBUTTON: return 0;
  default: return 0;
  }
}

BOOL SystemParametersInfo(UINT action, UINT, LPVOID pv, UINT)
{
  if (action == SPI_GETWORKAREA && pv) {
    RECT *r = (RECT *)pv;
    r->left = 0; r->top = 0; r->right = g_screen_w; r->bottom = g_screen_h;
    return TRUE;
  }
  return FALSE;
}

namespace w32 {
/* Windows 2000 / XP "Windows Classic" colour scheme */
static const uint32_t g_syscolors[31] = {
  0xD4D0C8, 0x3A6EA5, 0x0A246A, 0x808080, 0xD4D0C8, 0xFFFFFF, 0x000000, 0x000000,
  0x000000, 0xFFFFFF, 0xD4D0C8, 0xD4D0C8, 0x808080, 0x0A246A, 0xFFFFFF, 0xD4D0C8,
  0x808080, 0x808080, 0x000000, 0xD4D0C8, 0xFFFFFF, 0x404040, 0xD4D0C8, 0x000000,
  0xFFFFE1, 0xB5B5B5, 0x000080, 0xA6CAF0, 0xC0C0C0, 0x316AC5, 0xECE9D8 };

uint32_t syscolor_px(int i)
{
  if (i < 0 || i > 30) return 0;
  return g_syscolors[i];
}
}  /* namespace w32 */

DWORD GetSysColor(int i) { return px_to_cref(syscolor_px(i)); }

HBRUSH GetSysColorBrush(int i)
{
  static HBRUSH cache[31];
  static std::mutex m;
  std::lock_guard<std::mutex> lk(m);
  if (i < 0 || i > 30) return 0;
  if (!cache[i]) {
    cache[i] = CreateSolidBrush(GetSysColor(i));
    if (BrushObj *b = H<BrushObj>(cache[i], HT_BRUSH)) b->stock = true;
  }
  return cache[i];
}

SHORT GetKeyState(int vk)
{
  if (vk < 0 || vk > 255) return 0;
  uint8_t s = g_keys[vk];
  return (SHORT)(((s & 0x80) ? 0x8000 : 0) | (s & 1));
}

SHORT GetAsyncKeyState(int vk) { return (SHORT)(GetKeyState(vk) & 0x8000); }

BOOL MessageBeep(UINT) { return TRUE; }

LONG ChangeDisplaySettings(LPDEVMODE, DWORD) { return DISP_CHANGE_FAILED; }
