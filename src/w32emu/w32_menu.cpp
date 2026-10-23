/* ************************************************************************* */
/* w32_menu.cpp -- menus: menu objects, the menu bar in the non-client area, */
/* drop-down/popup menu windows and the modal menu tracking loop.            */
/*                                                                           */
/* NEW FILE, added 2026-10-12 for the Stellar Frontier Godot port.           */
/* ************************************************************************* */
#include "w32_internal.h"
#include <cstring>
#include <chrono>
#include <thread>

using namespace w32;

static MenuObj *menu_of(HMENU h) { return H<MenuObj>(h, HT_MENU); }

static const SfFontData *menu_font()
{
  LOGFONT lf;
  memset(&lf, 0, sizeof(lf));
  lf.lfHeight = 13;
  lf.lfWeight = FW_NORMAL;
  strcpy(lf.lfFaceName, "Tahoma");
  return font_match(lf);
}

static std::string strip_prefix(const std::string &s)
{
  std::string o;
  for (size_t i = 0; i < s.size(); i++) {
    if (s[i] == '&') { if (i + 1 < s.size() && s[i + 1] == '&') { o.push_back('&'); i++; } continue; }
    if (s[i] == '\t') break;
    o.push_back(s[i]);
  }
  return o;
}

/* ============================== menu objects ============================= */
HMENU CreateMenu(void)
{
  MenuObj *m = new MenuObj();
  m->popup = false;
  return (HMENU)hnew(m, HT_MENU);
}

HMENU CreatePopupMenu(void)
{
  MenuObj *m = new MenuObj();
  m->popup = true;
  return (HMENU)hnew(m, HT_MENU);
}

BOOL DestroyMenu(HMENU h)
{
  MenuObj *m = menu_of(h);
  if (!m) return FALSE;
  std::vector<HMENU> subs;
  for (auto &it : m->items) if (it.sub) subs.push_back(it.sub);
  hdel(h);
  delete m;
  for (HMENU s : subs) DestroyMenu(s);
  return TRUE;
}

BOOL IsMenu(HMENU h) { return menu_of(h) != nullptr; }

static MenuItem make_item(UINT flags, UINT_PTR id, LPCSTR text)
{
  MenuItem it;
  if (flags & MF_SEPARATOR) it.type = MFT_SEPARATOR;
  else it.type = MFT_STRING;
  if (flags & MF_POPUP) { it.sub = (HMENU)id; it.id = id; }
  else it.id = id;
  it.state = flags & (MF_GRAYED | MF_DISABLED | MF_CHECKED);
  if (text && !(flags & (MF_SEPARATOR | MF_BITMAP | MF_OWNERDRAW))) it.text = text;
  return it;
}

static void bar_changed(HMENU h)
{
  /* menus are drawn from their owner windows' frames */
  std::lock_guard<std::recursive_mutex> lk(g_wm);
  for (Wnd *t : g_top) if (t->menu == h) nc_paint(t);
}

BOOL AppendMenu(HMENU h, UINT flags, UINT_PTR id, LPCSTR text)
{
  MenuObj *m = menu_of(h);
  if (!m) return FALSE;
  {
    std::lock_guard<std::recursive_mutex> lk(g_wm);
    m->items.push_back(make_item(flags, id, text));
  }
  return TRUE;
}

/* locate an item: by position, or by command id (searching submenus) */
static bool find_item(HMENU h, UINT item, bool byPos, MenuObj **owner, size_t *index, int depth = 0)
{
  MenuObj *m = menu_of(h);
  if (!m || depth > 8) return false;
  if (byPos) {
    if (item >= m->items.size()) return false;
    *owner = m;
    *index = item;
    return true;
  }
  for (size_t i = 0; i < m->items.size(); i++) {
    MenuItem &it = m->items[i];
    if (!it.sub && it.type != MFT_SEPARATOR && (UINT)it.id == item) { *owner = m; *index = i; return true; }
  }
  for (size_t i = 0; i < m->items.size(); i++)
    if (m->items[i].sub && find_item(m->items[i].sub, item, false, owner, index, depth + 1)) return true;
  for (size_t i = 0; i < m->items.size(); i++)
    if (m->items[i].sub && (UINT)(UINT_PTR)m->items[i].sub == item) { *owner = m; *index = i; return true; }
  return false;
}

BOOL InsertMenu(HMENU h, UINT pos, UINT flags, UINT_PTR id, LPCSTR text)
{
  MenuObj *m = menu_of(h);
  if (!m) return FALSE;
  std::lock_guard<std::recursive_mutex> lk(g_wm);
  MenuItem it = make_item(flags & ~MF_BYPOSITION, id, text);
  if (flags & MF_BYPOSITION) {
    if (pos >= m->items.size()) m->items.push_back(it);
    else m->items.insert(m->items.begin() + pos, it);
    return TRUE;
  }
  MenuObj *o; size_t i;
  if (!find_item(h, pos, false, &o, &i)) { m->items.push_back(it); return TRUE; }
  o->items.insert(o->items.begin() + i, it);
  return TRUE;
}

static void apply_mii(MenuItem &it, LPCMENUITEMINFO mii)
{
  if (mii->fMask & MIIM_TYPE) {
    it.type = mii->fType;
    if ((mii->fType & MFT_SEPARATOR) || (mii->fType & (MFT_BITMAP | MFT_OWNERDRAW))) it.text.clear();
    else if (mii->dwTypeData) it.text = mii->dwTypeData;
  }
  if (mii->fMask & MIIM_STRING) it.text = mii->dwTypeData ? mii->dwTypeData : "";
  if (mii->fMask & MIIM_FTYPE) it.type = mii->fType;
  if (mii->fMask & MIIM_STATE) it.state = mii->fState & (MFS_GRAYED | MFS_CHECKED | MFS_HILITE | MFS_DEFAULT);
  if (mii->fMask & MIIM_ID) it.id = mii->wID;
  if (mii->fMask & MIIM_SUBMENU) it.sub = mii->hSubMenu;
  if (mii->fMask & MIIM_DATA) it.data = mii->dwItemData;
}

BOOL InsertMenuItem(HMENU h, UINT item, BOOL byPos, LPCMENUITEMINFO mii)
{
  MenuObj *m = menu_of(h);
  if (!m || !mii) return FALSE;
  std::lock_guard<std::recursive_mutex> lk(g_wm);
  MenuItem it;
  apply_mii(it, mii);
  if (byPos) {
    if (item >= m->items.size()) m->items.push_back(it);
    else m->items.insert(m->items.begin() + item, it);
    return TRUE;
  }
  MenuObj *o; size_t i;
  if (!find_item(h, item, false, &o, &i)) { m->items.push_back(it); return TRUE; }
  o->items.insert(o->items.begin() + i, it);
  return TRUE;
}

BOOL SetMenuItemInfo(HMENU h, UINT item, BOOL byPos, LPCMENUITEMINFO mii)
{
  if (!mii) return FALSE;
  std::lock_guard<std::recursive_mutex> lk(g_wm);
  MenuObj *o; size_t i;
  if (!find_item(h, item, byPos != 0, &o, &i)) return FALSE;
  apply_mii(o->items[i], mii);
  return TRUE;
}

BOOL GetMenuItemInfo(HMENU h, UINT item, BOOL byPos, LPMENUITEMINFO mii)
{
  if (!mii) return FALSE;
  std::lock_guard<std::recursive_mutex> lk(g_wm);
  MenuObj *o; size_t i;
  if (!find_item(h, item, byPos != 0, &o, &i)) return FALSE;
  MenuItem &it = o->items[i];
  if (mii->fMask & (MIIM_TYPE | MIIM_FTYPE)) mii->fType = it.type;
  if (mii->fMask & MIIM_STATE) mii->fState = it.state;
  if (mii->fMask & MIIM_ID) mii->wID = (UINT)it.id;
  if (mii->fMask & MIIM_SUBMENU) mii->hSubMenu = it.sub;
  if (mii->fMask & MIIM_DATA) mii->dwItemData = it.data;
  if ((mii->fMask & (MIIM_TYPE | MIIM_STRING)) && mii->dwTypeData && mii->cch) {
    snprintf(mii->dwTypeData, mii->cch, "%s", it.text.c_str());
    mii->cch = (UINT)it.text.size();
  }
  return TRUE;
}

BOOL RemoveMenu(HMENU h, UINT item, UINT flags)
{
  std::lock_guard<std::recursive_mutex> lk(g_wm);
  MenuObj *o; size_t i;
  if (!find_item(h, item, (flags & MF_BYPOSITION) != 0, &o, &i)) return FALSE;
  o->items.erase(o->items.begin() + i);
  return TRUE;
}

BOOL DeleteMenu(HMENU h, UINT item, UINT flags)
{
  HMENU sub = 0;
  {
    std::lock_guard<std::recursive_mutex> lk(g_wm);
    MenuObj *o; size_t i;
    if (!find_item(h, item, (flags & MF_BYPOSITION) != 0, &o, &i)) return FALSE;
    sub = o->items[i].sub;
    o->items.erase(o->items.begin() + i);
  }
  if (sub) DestroyMenu(sub);
  return TRUE;
}

DWORD CheckMenuItem(HMENU h, UINT item, UINT flags)
{
  std::lock_guard<std::recursive_mutex> lk(g_wm);
  MenuObj *o; size_t i;
  if (!find_item(h, item, (flags & MF_BYPOSITION) != 0, &o, &i)) return (DWORD)-1;
  DWORD old = o->items[i].state & MF_CHECKED;
  if (flags & MF_CHECKED) o->items[i].state |= MF_CHECKED;
  else o->items[i].state &= ~MF_CHECKED;
  return old;
}

BOOL EnableMenuItem(HMENU h, UINT item, UINT flags)
{
  DWORD old;
  {
    std::lock_guard<std::recursive_mutex> lk(g_wm);
    MenuObj *o; size_t i;
    if (!find_item(h, item, (flags & MF_BYPOSITION) != 0, &o, &i)) return (BOOL)-1;
    old = o->items[i].state & (MF_GRAYED | MF_DISABLED);
    o->items[i].state &= ~(MF_GRAYED | MF_DISABLED);
    o->items[i].state |= flags & (MF_GRAYED | MF_DISABLED);
  }
  bar_changed(h);
  return (BOOL)old;
}

UINT GetMenuState(HMENU h, UINT item, UINT flags)
{
  std::lock_guard<std::recursive_mutex> lk(g_wm);
  MenuObj *o; size_t i;
  if (!find_item(h, item, (flags & MF_BYPOSITION) != 0, &o, &i)) return (UINT)-1;
  MenuItem &it = o->items[i];
  UINT s = it.state & (MF_GRAYED | MF_DISABLED | MF_CHECKED | MF_HILITE);
  if (it.type & MFT_SEPARATOR) s |= MF_SEPARATOR;
  if (it.sub) {
    MenuObj *sm = menu_of(it.sub);
    s |= MF_POPUP | ((sm ? (UINT)sm->items.size() : 0) << 8);
  }
  return s;
}

int GetMenuString(HMENU h, UINT item, LPSTR buf, int n, UINT flags)
{
  std::lock_guard<std::recursive_mutex> lk(g_wm);
  MenuObj *o; size_t i;
  if (!find_item(h, item, (flags & MF_BYPOSITION) != 0, &o, &i)) return 0;
  const std::string &t = o->items[i].text;
  if (!buf || n <= 0) return (int)t.size();
  snprintf(buf, n, "%s", t.c_str());
  return (int)strlen(buf);
}

int GetMenuItemCount(HMENU h)
{
  MenuObj *m = menu_of(h);
  return m ? (int)m->items.size() : -1;
}

UINT GetMenuItemID(HMENU h, int pos)
{
  MenuObj *m = menu_of(h);
  if (!m || pos < 0 || pos >= (int)m->items.size() || m->items[pos].sub) return (UINT)-1;
  return (UINT)m->items[pos].id;
}

HMENU GetSubMenu(HMENU h, int pos)
{
  MenuObj *m = menu_of(h);
  if (!m || pos < 0 || pos >= (int)m->items.size()) return 0;
  return m->items[pos].sub;
}

BOOL SetMenu(HWND hw, HMENU hm)
{
  Wnd *w = wnd(hw);
  if (!w || w->is_child()) return FALSE;
  {
    std::lock_guard<std::recursive_mutex> lk(g_wm);
    w->menu = hm;
  }
  SetWindowPos(hw, 0, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
  return TRUE;
}

HMENU GetMenu(HWND hw)
{
  Wnd *w = wnd(hw);
  return (w && !w->is_child()) ? w->menu : 0;
}

BOOL DrawMenuBar(HWND hw)
{
  Wnd *w = wnd(hw);
  if (!w) return FALSE;
  std::lock_guard<std::recursive_mutex> lk(g_wm);
  nc_paint(w);
  return TRUE;
}

HMENU GetSystemMenu(HWND, BOOL)
{
  static HMENU sys = 0;
  if (!sys) sys = CreatePopupMenu();
  return sys;
}

/* ================================ menu bar =============================== */
namespace w32 {

static const int BAR_PAD = 7;

/* item rectangles relative to the bar origin */
static std::vector<RECT> bar_layout(MenuObj *m, int cx)
{
  std::vector<RECT> r;
  const SfFontData *fd = menu_font();
  int x = 0;
  for (auto &it : m->items) {
    std::string t = strip_prefix(it.text);
    int w = (it.type & MFT_SEPARATOR) ? 6 : text_width(fd, t.c_str(), (int)t.size()) + 2 * BAR_PAD;
    r.push_back(RECT{x, 1, x + w, M_CYMENU - 1});
    x += w;
  }
  (void)cx;
  return r;
}

int menubar_height(Wnd *w, int cx)
{
  (void)w; (void)cx;
  return M_CYMENU;
}

void menubar_paint(Wnd *w, Target &t, int x, int y, int cx)
{
  fill_rect(t, x, y, x + cx, y + M_CYMENU, syscolor_px(COLOR_MENU));
  MenuObj *m = menu_of(w->menu);
  if (!m) return;
  const SfFontData *fd = menu_font();
  std::vector<RECT> rs = bar_layout(m, cx);
  for (size_t i = 0; i < m->items.size(); i++) {
    MenuItem &it = m->items[i];
    if (it.type & MFT_SEPARATOR) continue;
    RECT r = rs[i];
    OffsetRect(&r, x, y);
    if (r.left >= x + cx) break;
    bool hot = (int)i == w->menu_hot;
    int off = 0;
    if (hot) {
      draw_3d_rect(t, r.left, r.top, r.right, r.bottom, syscolor_px(COLOR_BTNSHADOW), syscolor_px(COLOR_BTNHIGHLIGHT));
      off = 1;
    }
    RECT tr = {r.left + off, r.top + off, r.right + off, r.bottom + off};
    bool gray = (it.state & (MF_GRAYED | MF_DISABLED)) != 0;
    if (gray && (it.state & MF_GRAYED)) {
      RECT sr = tr;
      OffsetRect(&sr, 1, 1);
      draw_text_in(t, fd, it.text.c_str(), sr, DT_CENTER | DT_VCENTER | DT_SINGLELINE,
                   syscolor_px(COLOR_BTNHIGHLIGHT));
      draw_text_in(t, fd, it.text.c_str(), tr, DT_CENTER | DT_VCENTER | DT_SINGLELINE,
                   syscolor_px(COLOR_GRAYTEXT));
    } else {
      draw_text_in(t, fd, it.text.c_str(), tr, DT_CENTER | DT_VCENTER | DT_SINGLELINE,
                   syscolor_px(COLOR_MENUTEXT));
    }
  }
}

/* screen point -> bar item index or -1 */
int menubar_item_at(Wnd *w, int sx, int sy, RECT *itemScreen)
{
  MenuObj *m = menu_of(w->menu);
  if (!m || w->is_child()) return -1;
  int ox, oy;
  wnd_screen_origin(w, &ox, &oy);
  int b = nc_border(w->style, w->exstyle);
  int top = b + (((w->style & WS_CAPTION) == WS_CAPTION) ? M_CYCAPTION : 0);
  int bx = ox + b, by = oy + top;
  int cx = (w->rc.right - w->rc.left) - 2 * b;
  std::vector<RECT> rs = bar_layout(m, cx);
  for (size_t i = 0; i < rs.size(); i++) {
    RECT r = rs[i];
    r.top = 0; r.bottom = M_CYMENU;
    OffsetRect(&r, bx, by);
    POINT p = {sx, sy};
    if (PtInRect(&r, p) && !(m->items[i].type & MFT_SEPARATOR)) {
      if (itemScreen) *itemScreen = r;
      return (int)i;
    }
  }
  return -1;
}

}  /* namespace w32 */

/* ============================== popup windows ============================ */
namespace {

const int POPUP_BORDER = 3;
const int ITEM_H = 17;
const int SEP_H = 9;
const int CHECK_W = 18;

struct PopupState {
  HMENU menu = 0;
  int hot = -1;
};

int item_height(const MenuItem &it) { return (it.type & MFT_SEPARATOR) ? SEP_H : ITEM_H; }

SIZE popup_size(MenuObj *m)
{
  const SfFontData *fd = menu_font();
  int w = 0, h = 0;
  for (auto &it : m->items) {
    std::string t = strip_prefix(it.text);
    w = std::max(w, text_width(fd, t.c_str(), (int)t.size()));
    h += item_height(it);
  }
  SIZE s;
  s.cx = w + CHECK_W + 20 + 2 * POPUP_BORDER;
  s.cy = h + 2 * POPUP_BORDER;
  return s;
}

int popup_item_at(MenuObj *m, int y)
{
  int yy = POPUP_BORDER;
  for (size_t i = 0; i < m->items.size(); i++) {
    int ih = item_height(m->items[i]);
    if (y >= yy && y < yy + ih) return (int)i;
    yy += ih;
  }
  return -1;
}

void popup_paint(HWND h, PopupState *st)
{
  PAINTSTRUCT ps;
  HDC dc = BeginPaint(h, &ps);
  Wnd *w = wnd(h);
  MenuObj *m = menu_of(st->menu);
  Target t;
  DCObj *d = dc_from(dc);
  if (w && m && d && resolve_dc(d, t)) {
    int W = w->client.right - w->client.left, Hh = w->client.bottom - w->client.top;
    fill_rect(t, 0, 0, W, Hh, syscolor_px(COLOR_MENU));
    draw_edge(t, RECT{0, 0, W, Hh}, EDGE_RAISED, BF_RECT);
    const SfFontData *fd = menu_font();
    int y = POPUP_BORDER;
    for (size_t i = 0; i < m->items.size(); i++) {
      MenuItem &it = m->items[i];
      int ih = item_height(it);
      if (it.type & MFT_SEPARATOR) {
        hline(t, POPUP_BORDER + 1, W - POPUP_BORDER - 1, y + SEP_H / 2 - 1, syscolor_px(COLOR_BTNSHADOW));
        hline(t, POPUP_BORDER + 1, W - POPUP_BORDER - 1, y + SEP_H / 2, syscolor_px(COLOR_BTNHIGHLIGHT));
        y += ih;
        continue;
      }
      bool gray = (it.state & (MF_GRAYED | MF_DISABLED)) != 0;
      bool hot = (int)i == st->hot;
      uint32_t tc = syscolor_px(COLOR_MENUTEXT);
      if (hot) {
        fill_rect(t, POPUP_BORDER, y, W - POPUP_BORDER, y + ih, syscolor_px(COLOR_HIGHLIGHT));
        tc = syscolor_px(COLOR_HIGHLIGHTTEXT);
      }
      RECT tr = {POPUP_BORDER + CHECK_W, y, W - POPUP_BORDER - 4, y + ih};
      if (gray) {
        if (!hot) {
          RECT sr = tr;
          OffsetRect(&sr, 1, 1);
          draw_text_in(t, fd, it.text.c_str(), sr, DT_LEFT | DT_VCENTER | DT_SINGLELINE, syscolor_px(COLOR_BTNHIGHLIGHT));
        }
        tc = syscolor_px(COLOR_GRAYTEXT);
      }
      draw_text_in(t, fd, it.text.c_str(), tr, DT_LEFT | DT_VCENTER | DT_SINGLELINE, tc);
      if (it.state & MF_CHECKED) draw_check(t, POPUP_BORDER + 5, y + (ih - 7) / 2, tc);
      if (it.sub) draw_arrow(t, W - POPUP_BORDER - 8, y + ih / 2, 3, tc, 4);
      y += ih;
    }
    screen_dirty();
  }
  EndPaint(h, &ps);
}

}  /* namespace */

namespace w32 {
LRESULT CALLBACK menu_popup_proc(HWND h, UINT m, WPARAM wp, LPARAM lp)
{
  Wnd *w = wnd(h);
  PopupState *st = w ? (PopupState *)w->ctl : nullptr;
  switch (m) {
  case WM_NCCREATE:
    w->ctl = new PopupState();
    w->ctl_free = [](void *p) { delete (PopupState *)p; };
    return TRUE;
  case WM_PAINT:
    if (st) popup_paint(h, st);
    else { PAINTSTRUCT ps; BeginPaint(h, &ps); EndPaint(h, &ps); }
    return 0;
  case WM_MOUSEACTIVATE:
    return MA_NOACTIVATE;
  default:
    return DefWindowProc(h, m, wp, lp);
  }
}
}  /* namespace w32 */

/* ============================ tracking loop ============================== */
namespace {

struct Level {
  HMENU menu;
  HWND win;
  int parentItem;
};

struct Tracker {
  Wnd *owner = nullptr;
  HWND ownerH = 0;
  bool bar = false;
  int barItem = -1;
  std::vector<Level> levels;
  UINT_PTR cmd = 0;
  bool selected = false;
};

PopupState *pstate(HWND h)
{
  Wnd *w = wnd(h);
  return w ? (PopupState *)w->ctl : nullptr;
}

void set_hot(HWND h, int i)
{
  PopupState *st = pstate(h);
  if (!st || st->hot == i) return;
  st->hot = i;
  InvalidateRect(h, nullptr, FALSE);
}

HWND open_popup(Tracker &tr, HMENU menu, int x, int y, int parentItem)
{
  MenuObj *m = menu_of(menu);
  if (!m) return 0;
  send_msg(tr.ownerH, WM_INITMENUPOPUP, (WPARAM)menu, MAKELPARAM(parentItem < 0 ? 0 : parentItem, 0));
  SIZE s = popup_size(m);
  if (x + s.cx > g_screen_w) x = g_screen_w - s.cx;
  if (y + s.cy > g_screen_h) y = std::max(0, (int)(y - s.cy));
  if (x < 0) x = 0;
  HWND h = CreateWindowEx(WS_EX_TOOLWINDOW | WS_EX_TOPMOST, "#32768", "", WS_POPUP, x, y, s.cx, s.cy,
                          tr.ownerH, 0, 0, nullptr);
  if (!h) return 0;
  pstate(h)->menu = menu;
  ShowWindow(h, SW_SHOWNA);
  tr.levels.push_back(Level{menu, h, parentItem});
  return h;
}

void close_levels(Tracker &tr, size_t keep)
{
  while (tr.levels.size() > keep) {
    DestroyWindow(tr.levels.back().win);
    tr.levels.pop_back();
  }
}

void set_bar_hot(Tracker &tr, int item)
{
  std::lock_guard<std::recursive_mutex> lk(g_wm);
  if (tr.owner->menu_hot != item) {
    tr.owner->menu_hot = item;
    nc_paint(tr.owner);
  }
}

void open_bar_item(Tracker &tr, int item)
{
  close_levels(tr, 0);
  tr.barItem = item;
  set_bar_hot(tr, item);
  MenuObj *m = menu_of(tr.owner->menu);
  if (!m || item < 0 || item >= (int)m->items.size()) return;
  MenuItem it = m->items[item];
  if (!it.sub || (it.state & (MF_GRAYED | MF_DISABLED))) return;
  RECT r;
  int idx;
  {
    std::lock_guard<std::recursive_mutex> lk(g_wm);
    idx = -1;
    int ox, oy;
    wnd_screen_origin(tr.owner, &ox, &oy);
    int b = nc_border(tr.owner->style, tr.owner->exstyle);
    int top = b + (((tr.owner->style & WS_CAPTION) == WS_CAPTION) ? M_CYCAPTION : 0);
    std::vector<RECT> rs = bar_layout(m, 0);
    r = rs[item];
    r.top = 0; r.bottom = M_CYMENU;
    OffsetRect(&r, ox + b, oy + top);
    (void)idx;
  }
  open_popup(tr, it.sub, r.left, r.bottom, item);
}

/* which level/item is under the screen point; level -1 = none */
void hit(Tracker &tr, int sx, int sy, int *level, int *item)
{
  *level = -1;
  *item = -1;
  for (int l = (int)tr.levels.size() - 1; l >= 0; l--) {
    RECT r;
    GetWindowRect(tr.levels[l].win, &r);
    POINT p = {sx, sy};
    if (PtInRect(&r, p)) {
      *level = l;
      MenuObj *m = menu_of(tr.levels[l].menu);
      *item = m ? popup_item_at(m, sy - r.top) : -1;
      return;
    }
  }
}

bool item_selectable(HMENU menu, int item)
{
  MenuObj *m = menu_of(menu);
  if (!m || item < 0 || item >= (int)m->items.size()) return false;
  const MenuItem &it = m->items[item];
  return !(it.type & MFT_SEPARATOR) && !(it.state & (MF_GRAYED | MF_DISABLED));
}

void run(Tracker &tr)
{
  HWND cap = tr.ownerH;
  SetCapture(cap);
  bool done = false;
  bool firstUp = true;
  while (!done) {
    MSG msg;
    if (!PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
      if (GetCapture() != cap || !wnd(tr.ownerH)) break;
      std::this_thread::sleep_for(std::chrono::milliseconds(5));
      continue;
    }
    if (msg.message == WM_QUIT) { PostQuitMessage((int)msg.wParam); break; }
    UINT mm = msg.message;
    if (mm >= WM_NCMOUSEMOVE && mm <= WM_NCMBUTTONDBLCLK) mm = mm - WM_NCMOUSEMOVE + WM_MOUSEMOVE;
    POINT p;
    { std::lock_guard<std::recursive_mutex> lk(g_wm); p = g_mouse; }
    if (mm >= WM_MOUSEFIRST && mm <= WM_MOUSELAST) {
      int level, item;
      hit(tr, p.x, p.y, &level, &item);
      int barItem = -1;
      if (tr.bar) {
        std::lock_guard<std::recursive_mutex> lk(g_wm);
        barItem = menubar_item_at(tr.owner, p.x, p.y, nullptr);
      }
      switch (mm) {
      case WM_MOUSEMOVE:
        if (level >= 0) {
          set_hot(tr.levels[level].win, item);
          /* open or close submenus following the pointer */
          MenuObj *m = menu_of(tr.levels[level].menu);
          if (m && item >= 0 && m->items[item].sub && item_selectable(tr.levels[level].menu, item)) {
            if ((int)tr.levels.size() <= level + 1 || tr.levels[level + 1].parentItem != item) {
              close_levels(tr, level + 1);
              RECT r;
              GetWindowRect(tr.levels[level].win, &r);
              int y = r.top + POPUP_BORDER;
              for (int k = 0; k < item; k++) y += item_height(m->items[k]);
              open_popup(tr, m->items[item].sub, r.right - 3, y - POPUP_BORDER, item);
            }
          } else if (level + 1 < (int)tr.levels.size()) {
            close_levels(tr, level + 1);
          }
        } else if (barItem >= 0 && barItem != tr.barItem) {
          open_bar_item(tr, barItem);
        }
        break;
      case WM_LBUTTONDOWN: case WM_RBUTTONDOWN:
        firstUp = false;
        if (level < 0 && barItem < 0) { done = true; break; }
        if (level < 0 && barItem >= 0) {
          if (barItem == tr.barItem && !tr.levels.empty()) { done = true; break; }
          open_bar_item(tr, barItem);
        }
        break;
      case WM_LBUTTONUP: case WM_RBUTTONUP:
        if (level >= 0 && item_selectable(tr.levels[level].menu, item)) {
          MenuObj *m = menu_of(tr.levels[level].menu);
          if (m && !m->items[item].sub) {
            tr.cmd = m->items[item].id;
            tr.selected = true;
            done = true;
          }
        } else if (level < 0 && barItem < 0 && !firstUp) {
          done = true;
        }
        firstUp = false;
        break;
      default:
        break;
      }
      continue;
    }
    if (mm == WM_KEYDOWN || mm == WM_SYSKEYDOWN) {
      int lvl = (int)tr.levels.size() - 1;
      PopupState *st = lvl >= 0 ? pstate(tr.levels[lvl].win) : nullptr;
      MenuObj *m = lvl >= 0 ? menu_of(tr.levels[lvl].menu) : nullptr;
      switch (msg.wParam) {
      case VK_ESCAPE:
        if (lvl > 0) close_levels(tr, lvl);
        else done = true;
        break;
      case VK_UP: case VK_DOWN:
        if (st && m && !m->items.empty()) {
          int n = (int)m->items.size(), i = st->hot;
          for (int k = 0; k < n; k++) {
            i = msg.wParam == VK_DOWN ? (i + 1 + n) % n : (i - 1 + n) % n;
            if (!(m->items[i].type & MFT_SEPARATOR)) break;
          }
          set_hot(tr.levels[lvl].win, i);
        }
        break;
      case VK_LEFT: case VK_RIGHT:
        if (tr.bar) {
          MenuObj *bm = menu_of(tr.owner->menu);
          if (bm && !bm->items.empty()) {
            int n = (int)bm->items.size();
            int i = msg.wParam == VK_RIGHT ? (tr.barItem + 1) % n : (tr.barItem - 1 + n) % n;
            open_bar_item(tr, i);
          }
        }
        break;
      case VK_RETURN:
        if (st && m && item_selectable(tr.levels[lvl].menu, st->hot) && !m->items[st->hot].sub) {
          tr.cmd = m->items[st->hot].id;
          tr.selected = true;
          done = true;
        }
        break;
      default:
        break;
      }
      continue;
    }
    if (mm == WM_KEYUP || mm == WM_SYSKEYUP || mm == WM_CHAR || mm == WM_SYSCHAR) continue;
    DispatchMessage(&msg);
  }
  if (GetCapture() == cap) ReleaseCapture();
  close_levels(tr, 0);
  if (tr.bar && wnd(tr.ownerH)) set_bar_hot(tr, -1);
}

}  /* namespace */

namespace w32 {

void menu_track_bar(Wnd *w, int startItem, bool keyboard)
{
  if (!w || !w->menu) return;
  Tracker tr;
  tr.owner = w;
  tr.ownerH = w->hwnd;
  tr.bar = true;
  send_msg(tr.ownerH, WM_ENTERMENULOOP, FALSE, 0);
  send_msg(tr.ownerH, WM_INITMENU, (WPARAM)w->menu, 0);
  if (startItem < 0) startItem = 0;
  if (!wnd(tr.ownerH)) return;
  open_bar_item(tr, startItem);
  if (keyboard && !tr.levels.empty()) set_hot(tr.levels[0].win, 0);
  run(tr);
  if (wnd(tr.ownerH)) send_msg(tr.ownerH, WM_EXITMENULOOP, FALSE, 0);
  if (tr.selected && wnd(tr.ownerH)) PostMessage(tr.ownerH, WM_COMMAND, MAKEWPARAM(tr.cmd, 0), 0);
}

BOOL menu_track_popup(HMENU m, UINT flags, int x, int y, HWND owner)
{
  Wnd *w = wnd(owner);
  if (!w || !menu_of(m)) return FALSE;
  Tracker tr;
  tr.owner = w;
  tr.ownerH = owner;
  tr.bar = false;
  send_msg(owner, WM_ENTERMENULOOP, TRUE, 0);
  send_msg(owner, WM_INITMENU, (WPARAM)m, 0);
  open_popup(tr, m, x, y, -1);
  run(tr);
  if (wnd(owner)) send_msg(owner, WM_EXITMENULOOP, TRUE, 0);
  if (flags & TPM_RETURNCMD) return tr.selected ? (BOOL)tr.cmd : 0;
  if (tr.selected && !(flags & TPM_NONOTIFY) && wnd(owner))
    PostMessage(owner, WM_COMMAND, MAKEWPARAM(tr.cmd, 0), 0);
  return TRUE;
}

}  /* namespace w32 */

BOOL TrackPopupMenu(HMENU m, UINT flags, int x, int y, int, HWND owner, const RECT *)
{
  return menu_track_popup(m, flags, x, y, owner);
}
