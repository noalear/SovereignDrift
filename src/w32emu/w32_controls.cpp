/* ************************************************************************* */
/* w32_controls.cpp -- the standard Windows controls used by the program:    */
/* BUTTON (push, check box, radio, group box), STATIC, EDIT (single and      */
/* multi-line), LISTBOX, COMBOBOX (drop-down list) and SCROLLBAR, with the   */
/* Windows 2000 "Classic" look and the documented notification messages.    */
/*                                                                           */
/* NEW FILE, added 2026-10-12 for the Stellar Frontier Godot port.           */
/* ************************************************************************* */
#include "w32_internal.h"
#include <cstring>

using namespace w32;

namespace {

/* ------------------------------- helpers --------------------------------- */
const SfFontData *ctl_font(Wnd *w)
{
  if (FontObj *f = H<FontObj>(w->font, HT_FONT)) if (f->fd) return f->fd;
  return font_default_gui_bold();   /* the System font, as when no WM_SETFONT */
}

HWND notify_target(Wnd *w)
{
  if (w->parent) return w->parent->hwnd;
  if (w->owner) return w->owner->hwnd;
  return 0;
}

void notify(Wnd *w, WORD code)
{
  HWND p = notify_target(w);
  if (p) send_msg(p, WM_COMMAND, MAKEWPARAM((WORD)w->id, code), (LPARAM)w->hwnd);
}

uint32_t brush_px(HBRUSH b, bool *ok)
{
  *ok = true;
  uintptr_t v = (uintptr_t)b;
  if (v && v <= 31) return syscolor_px((int)v - 1);
  if (BrushObj *br = H<BrushObj>(b, HT_BRUSH)) {
    if (br->style == BS_NULL) { *ok = false; return 0; }
    return cref_to_px(br->color);
  }
  *ok = false;
  return 0;
}

struct Colors {
  uint32_t bg = 0;
  bool hasBg = false;
  uint32_t text = 0;
  int bkMode = OPAQUE;
};

/* Ask the parent for colours (WM_CTLCOLORxxx), falling back to defaults. */
Colors ctl_colors(Wnd *w, HDC dc, UINT msg)
{
  Colors c;
  HWND p = notify_target(w);
  LRESULT r = 0;
  SetTextColor(dc, GetSysColor(COLOR_WINDOWTEXT));
  if (p) r = send_msg(p, msg, (WPARAM)dc, (LPARAM)w->hwnd);
  bool ok = false;
  if (r) c.bg = brush_px((HBRUSH)r, &ok);
  if (!r || !ok) {
    LRESULT d = DefWindowProc(w->hwnd, msg, (WPARAM)dc, (LPARAM)w->hwnd);
    c.bg = brush_px((HBRUSH)d, &ok);
  }
  c.hasBg = ok;
  c.text = cref_to_px(GetTextColor(dc));
  c.bkMode = GetBkMode(dc);
  return c;
}

/* Begin painting: DC + resolved target in client coordinates of t. */
struct Paint {
  HWND h;
  PAINTSTRUCT ps;
  HDC dc = 0;
  Target t;
  bool ok = false;
  int W = 0, Hh = 0;
  explicit Paint(HWND hw) : h(hw) {
    dc = BeginPaint(h, &ps);
    Wnd *w = wnd(h);
    DCObj *d = dc_from(dc);
    if (w && d) {
      /* paint the whole client area regardless of the update rectangle */
      Target tt;
      ok = resolve_dc(d, tt);
      if (ok) {
        t = tt;
        t.clips.clear();
        RECT c = {t.ox, t.oy, t.ox + (w->client.right - w->client.left), t.oy + (w->client.bottom - w->client.top)};
        RECT s = {0, 0, t.s->w, t.s->h}, i;
        if (IntersectRect(&i, &c, &s)) t.clips.push_back(i);
        ok = !t.clips.empty();
      }
      W = w->client.right - w->client.left;
      Hh = w->client.bottom - w->client.top;
    }
  }
  ~Paint() { if (ok) screen_dirty(); EndPaint(h, &ps); }
  /* client-coordinate wrappers */
  void fill(int x0, int y0, int x1, int y1, uint32_t c) { fill_rect(t, x0 + t.ox, y0 + t.oy, x1 + t.ox, y1 + t.oy, c); }
  void px(int x, int y, uint32_t c, int rop = R2_COPYPEN) { plot(t, x + t.ox, y + t.oy, c, rop); }
  void text(const SfFontData *fd, int x, int y, const std::string &s, uint32_t c) {
    text_draw(t, fd, x + t.ox, y + t.oy, s.c_str(), (int)s.size(), c);
  }
  void textin(const SfFontData *fd, const char *s, RECT r, UINT f, uint32_t c) {
    OffsetRect(&r, t.ox, t.oy);
    draw_text_in(t, fd, s, r, f, c);
  }
  void edge(RECT r, UINT e, UINT f) { OffsetRect(&r, t.ox, t.oy); draw_edge(t, r, e, f); }
  void rect3d(int x0, int y0, int x1, int y1, uint32_t a, uint32_t b) {
    draw_3d_rect(t, x0 + t.ox, y0 + t.oy, x1 + t.ox, y1 + t.oy, a, b);
  }
  void focus(RECT r) {
    for (int x = r.left; x < r.right; x++)
      if (((x - r.left) & 1) == 0) { px(x, r.top, 0, R2_NOT); px(x, r.bottom - 1, 0, R2_NOT); }
    for (int y = r.top + 1; y < r.bottom - 1; y++)
      if (((y - r.top) & 1) == 0) { px(r.left, y, 0, R2_NOT); px(r.right - 1, y, 0, R2_NOT); }
  }
  void clear_transparent() {
    for (auto &c : t.clips)
      for (int y = c.top; y < c.bottom; y++)
        for (int x = c.left; x < c.right; x++) t.s->px[(size_t)y * t.s->w + x] = 0xFF000000u;
  }
};

void grayed_text(Paint &p, const SfFontData *fd, const char *s, RECT r, UINT f)
{
  RECT sr = r;
  OffsetRect(&sr, 1, 1);
  p.textin(fd, s, sr, f, syscolor_px(COLOR_BTNHIGHLIGHT));
  p.textin(fd, s, r, f, syscolor_px(COLOR_GRAYTEXT));
}

bool has_focus(HWND h) { return GetFocus() == h; }

LRESULT text_msgs(Wnd *w, UINT m, WPARAM wp, LPARAM lp, bool *handled)
{
  *handled = true;
  switch (m) {
  case WM_SETFONT:
    w->font = (HFONT)wp;
    if (LOWORD(lp)) InvalidateRect(w->hwnd, nullptr, TRUE);
    return 0;
  case WM_GETFONT:
    return (LRESULT)w->font;
  case WM_ENABLE:
    InvalidateRect(w->hwnd, nullptr, TRUE);
    return 0;
  default:
    *handled = false;
    return 0;
  }
}

/* ================================ BUTTON ================================= */
struct BtnState {
  int check = 0;
  bool pressed = false;
  bool tracking = false;
  bool spaceDown = false;
};

BtnState *btn(Wnd *w) { return w ? (BtnState *)w->ctl : nullptr; }

int btype(Wnd *w) { return (int)(w->style & BS_TYPEMASK); }

void btn_paint(Wnd *w)
{
  BtnState *b = btn(w);
  Paint p(w->hwnd);
  if (!p.ok || !b) return;
  const SfFontData *fd = ctl_font(w);
  int type = btype(w);
  bool en = w->enabled();
  bool focus = has_focus(w->hwnd);
  if (type == BS_GROUPBOX) {
    Colors c = ctl_colors(w, p.dc, WM_CTLCOLORSTATIC);
    p.clear_transparent();     /* a group box only draws its frame and text */
    int ty = fd->height / 2;
    RECT fr = {0, ty, p.W, p.Hh};
    p.rect3d(fr.left, fr.top, fr.right - 1, fr.bottom - 1, syscolor_px(COLOR_BTNSHADOW), syscolor_px(COLOR_BTNSHADOW));
    p.rect3d(fr.left + 1, fr.top + 1, fr.right, fr.bottom, syscolor_px(COLOR_BTNHIGHLIGHT), syscolor_px(COLOR_BTNHIGHLIGHT));
    if (!w->text.empty()) {
      int tw = text_width(fd, w->text.c_str(), (int)w->text.size());
      if (c.hasBg) p.fill(7, 0, 7 + tw + 4, fd->height, c.bg);
      RECT tr = {9, 0, 9 + tw + 2, fd->height};
      if (en) p.textin(fd, w->text.c_str(), tr, DT_LEFT | DT_SINGLELINE, c.text);
      else grayed_text(p, fd, w->text.c_str(), tr, DT_LEFT | DT_SINGLELINE);
    }
    return;
  }
  if (type == BS_CHECKBOX || type == BS_AUTOCHECKBOX || type == BS_3STATE || type == BS_AUTO3STATE ||
      type == BS_RADIOBUTTON || type == BS_AUTORADIOBUTTON) {
    Colors c = ctl_colors(w, p.dc, WM_CTLCOLORSTATIC);
    if (c.hasBg) p.fill(0, 0, p.W, p.Hh, c.bg);
    bool radio = type == BS_RADIOBUTTON || type == BS_AUTORADIOBUTTON;
    int bx = 0, by = (p.Hh - 13) / 2;
    uint32_t inside = (b->pressed || !en) ? syscolor_px(COLOR_BTNFACE) : syscolor_px(COLOR_WINDOW);
    if (radio) {
      static const char *ring[12] = {
        "....aaaa....", "..aa....aa..", ".a........a.", ".a........a.", "a..........a",
        "a..........a", "a..........a", "a..........a", ".a........a.", ".a........a.",
        "..aa....aa..", "....aaaa...." };
      for (int y = 0; y < 12; y++) {
        int x0 = -1, x1 = -1;
        for (int x = 0; x < 12; x++) if (ring[y][x] == 'a') { if (x0 < 0) x0 = x; x1 = x; }
        for (int x = x0 + 1; x < x1; x++) p.px(bx + x, by + y, inside);
        for (int x = 0; x < 12; x++)
          if (ring[y][x] == 'a') p.px(bx + x, by + y, (y < 6 && x < 9) || x < 3 ? syscolor_px(COLOR_BTNSHADOW) : syscolor_px(COLOR_BTNHIGHLIGHT));
      }
      if (b->check & BST_CHECKED) {
        p.fill(bx + 4, by + 5, bx + 8, by + 7, 0);
        p.fill(bx + 5, by + 4, bx + 7, by + 8, 0);
      }
    } else {
      p.edge(RECT{bx, by, bx + 13, by + 13}, EDGE_SUNKEN, BF_RECT);
      p.fill(bx + 2, by + 2, bx + 11, by + 11, inside);
      if (b->check & BST_CHECKED) {
        Target &t = p.t;
        draw_check(t, bx + 3 + t.ox, by + 3 + t.oy, en ? 0 : syscolor_px(COLOR_GRAYTEXT));
      }
    }
    RECT tr = {17, 0, p.W, p.Hh};
    int tw = text_width(fd, w->text.c_str(), (int)w->text.size());
    if (en) p.textin(fd, w->text.c_str(), tr, DT_LEFT | DT_VCENTER | DT_SINGLELINE, c.text);
    else grayed_text(p, fd, w->text.c_str(), tr, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    if (focus) {
      int ty = (p.Hh - fd->height) / 2;
      p.focus(RECT{16, ty - 1, std::min(p.W, 17 + tw + 2), ty + fd->height + 1});
    }
    return;
  }
  /* push button */
  ctl_colors(w, p.dc, WM_CTLCOLORBTN);
  bool def = type == BS_DEFPUSHBUTTON || (focus && type == BS_PUSHBUTTON && false);
  RECT r = {0, 0, p.W, p.Hh};
  p.fill(0, 0, p.W, p.Hh, syscolor_px(COLOR_BTNFACE));
  if (def || focus) {
    p.rect3d(0, 0, p.W, p.Hh, 0, 0);
    InflateRect(&r, -1, -1);
  }
  if (b->pressed) {
    p.rect3d(r.left, r.top, r.right, r.bottom, syscolor_px(COLOR_BTNSHADOW), syscolor_px(COLOR_BTNSHADOW));
  } else {
    p.edge(r, EDGE_RAISED, BF_RECT);
  }
  RECT tr = r;
  if (b->pressed) OffsetRect(&tr, 1, 1);
  if (en) p.textin(fd, w->text.c_str(), tr, DT_CENTER | DT_VCENTER | DT_SINGLELINE, syscolor_px(COLOR_BTNTEXT));
  else grayed_text(p, fd, w->text.c_str(), tr, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
  if (focus) {
    RECT fr = r;
    InflateRect(&fr, -3, -3);
    if (b->pressed) OffsetRect(&fr, 1, 1);
    p.focus(fr);
  }
}

void btn_click(Wnd *w)
{
  BtnState *b = btn(w);
  int type = btype(w);
  if (type == BS_AUTOCHECKBOX) b->check = (b->check & BST_CHECKED) ? 0 : BST_CHECKED;
  else if (type == BS_AUTO3STATE) b->check = (b->check + 1) % 3;
  else if (type == BS_AUTORADIOBUTTON && w->parent) {
    /* uncheck the other auto radio buttons of the group */
    std::vector<HWND> sibs;
    {
      std::lock_guard<std::recursive_mutex> lk(g_wm);
      auto &ch = w->parent->children;
      auto it = std::find(ch.begin(), ch.end(), w);
      /* group start: go back to the WS_GROUP window */
      auto s = it;
      while (s != ch.begin() && !((*s)->style & WS_GROUP)) --s;
      for (auto k = s; k != ch.end(); ++k) {
        if (k != s && ((*k)->style & WS_GROUP)) break;
        if (*k != w && (*k)->cls->name == "BUTTON" && btype(*k) == BS_AUTORADIOBUTTON) sibs.push_back((*k)->hwnd);
      }
    }
    for (HWND s : sibs) send_msg(s, BM_SETCHECK, BST_UNCHECKED, 0);
    b->check = BST_CHECKED;
  }
  InvalidateRect(w->hwnd, nullptr, TRUE);
  notify(w, BN_CLICKED);
}

}  /* namespace */

namespace w32 {

LRESULT CALLBACK button_proc(HWND h, UINT m, WPARAM wp, LPARAM lp)
{
  Wnd *w = wnd(h);
  if (!w) return 0;
  BtnState *b = btn(w);
  bool handled;
  LRESULT r = text_msgs(w, m, wp, lp, &handled);
  if (handled) return r;
  switch (m) {
  case WM_NCCREATE:
    w->ctl = new BtnState();
    w->ctl_free = [](void *p) { delete (BtnState *)p; };
    return DefWindowProc(h, m, wp, lp);
  case WM_PAINT:
    btn_paint(w);
    return 0;
  case WM_ERASEBKGND:
    return 1;
  case WM_SETTEXT:
    DefWindowProc(h, m, wp, lp);
    InvalidateRect(h, nullptr, TRUE);
    return TRUE;
  case WM_GETDLGCODE: {
    int t = btype(w);
    if (t == BS_GROUPBOX) return DLGC_STATIC;
    if (t == BS_RADIOBUTTON || t == BS_AUTORADIOBUTTON) return DLGC_BUTTON | DLGC_RADIOBUTTON;
    if (t == BS_PUSHBUTTON) return DLGC_BUTTON | DLGC_UNDEFPUSHBUTTON;
    if (t == BS_DEFPUSHBUTTON) return DLGC_BUTTON | DLGC_DEFPUSHBUTTON;
    return DLGC_BUTTON;
  }
  case WM_LBUTTONDOWN: case WM_LBUTTONDBLCLK:
    if (btype(w) == BS_GROUPBOX) return 0;
    SetCapture(h);
    b->tracking = true;
    b->pressed = true;
    if (GetFocus() != h) SetFocus(h);
    InvalidateRect(h, nullptr, TRUE);
    return 0;
  case WM_MOUSEMOVE:
    if (b->tracking) {
      RECT c;
      GetClientRect(h, &c);
      POINT pt = {GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};
      bool in = PtInRect(&c, pt) != 0;
      if (in != b->pressed) { b->pressed = in; InvalidateRect(h, nullptr, TRUE); }
    }
    return 0;
  case WM_LBUTTONUP:
    if (b->tracking) {
      b->tracking = false;
      bool was = b->pressed;
      b->pressed = false;
      ReleaseCapture();
      InvalidateRect(h, nullptr, TRUE);
      if (was && wnd(h)) btn_click(w);
    }
    return 0;
  case WM_CAPTURECHANGED:
    if (b->tracking) {
      b->tracking = false;
      b->pressed = false;
      InvalidateRect(h, nullptr, TRUE);
    }
    return 0;
  case WM_KEYDOWN:
    if (wp == VK_SPACE && !(lp & (1 << 30))) {
      b->spaceDown = true;
      b->pressed = true;
      InvalidateRect(h, nullptr, TRUE);
    }
    return 0;
  case WM_KEYUP:
    if (wp == VK_SPACE && b->spaceDown) {
      b->spaceDown = false;
      b->pressed = false;
      InvalidateRect(h, nullptr, TRUE);
      btn_click(w);
    }
    return 0;
  case WM_CHAR:
    return 0;
  case WM_SETFOCUS: case WM_KILLFOCUS:
    if (m == WM_KILLFOCUS && (b->spaceDown || b->tracking)) {
      b->spaceDown = false;
      b->pressed = false;
      if (b->tracking) { b->tracking = false; ReleaseCapture(); }
    }
    InvalidateRect(h, nullptr, TRUE);
    return 0;
  case BM_GETCHECK:
    return b->check & 3;
  case BM_SETCHECK:
    if (b->check != (int)(wp & 3)) { b->check = (int)(wp & 3); InvalidateRect(h, nullptr, TRUE); }
    return 0;
  case BM_GETSTATE:
    return (b->check & 3) | (b->pressed ? BST_PUSHED : 0) | (has_focus(h) ? BST_FOCUS : 0);
  case BM_SETSTATE:
    b->pressed = wp != 0;
    InvalidateRect(h, nullptr, TRUE);
    return 0;
  case BM_SETSTYLE: {
    std::lock_guard<std::recursive_mutex> lk(g_wm);
    w->style = (w->style & ~0xFFFF) | (DWORD)(wp & 0xFFFF);
    if (LOWORD(lp)) wnd_invalidate(w, nullptr, true);
    return 0;
  }
  case BM_CLICK:
    btn_click(w);
    return 0;
  default:
    return DefWindowProc(h, m, wp, lp);
  }
}

/* ================================ STATIC ================================= */
LRESULT CALLBACK static_proc(HWND h, UINT m, WPARAM wp, LPARAM lp)
{
  Wnd *w = wnd(h);
  if (!w) return 0;
  bool handled;
  LRESULT r = text_msgs(w, m, wp, lp, &handled);
  if (handled) return r;
  switch (m) {
  case WM_PAINT: {
    Paint p(h);
    if (!p.ok) return 0;
    Colors c = ctl_colors(w, p.dc, WM_CTLCOLORSTATIC);
    if (c.hasBg) p.fill(0, 0, p.W, p.Hh, c.bg);
    DWORD ss = w->style & 0x1F;
    UINT f = DT_EXPANDTABS;
    if (w->style & SS_NOPREFIX) f |= DT_NOPREFIX;
    if (ss == SS_CENTER) f |= DT_CENTER | DT_WORDBREAK;
    else if (ss == SS_RIGHT) f |= DT_RIGHT | DT_WORDBREAK;
    else if (ss == SS_SIMPLE || ss == SS_LEFTNOWORDWRAP) f |= DT_LEFT | DT_SINGLELINE;
    else f |= DT_LEFT | DT_WORDBREAK;
    const SfFontData *fd = ctl_font(w);
    RECT tr = {0, 0, p.W, p.Hh};
    if (w->enabled()) p.textin(fd, w->text.c_str(), tr, f, c.text);
    else grayed_text(p, fd, w->text.c_str(), tr, f);
    return 0;
  }
  case WM_ERASEBKGND:
    return 1;
  case WM_SETTEXT:
    DefWindowProc(h, m, wp, lp);
    InvalidateRect(h, nullptr, TRUE);
    return TRUE;
  case WM_NCHITTEST:
    return (w->style & SS_NOTIFY) ? HTCLIENT : HTTRANSPARENT;
  case WM_GETDLGCODE:
    return DLGC_STATIC;
  default:
    return DefWindowProc(h, m, wp, lp);
  }
}

}  /* namespace w32 */

/* ================================== EDIT ================================= */
namespace {

std::string g_clipboard W32_EARLY;

struct EditState {
  std::string text;
  int anchor = 0, caret = 0;
  int limit = 0x7FFFFFFE;
  bool modified = false;
  char pw = 0;
  int scrollX = 0;              /* single line, pixels */
  int firstLine = 0;            /* multi line */
  bool caretOn = false;
  bool selecting = false;
  std::vector<int> lines;       /* start index of each visual line */
};

EditState *ed(Wnd *w) { return w ? (EditState *)w->ctl : nullptr; }
bool multiline(Wnd *w) { return (w->style & ES_MULTILINE) != 0; }
bool readonly(Wnd *w) { return (w->style & ES_READONLY) != 0; }
const int EDIT_MARGIN = 1;

std::string shown(Wnd *w, EditState *e)
{
  if (!multiline(w) && (e->pw || (w->style & ES_PASSWORD)))
    return std::string(e->text.size(), e->pw ? e->pw : '*');
  return e->text;
}

/* Lay out multi-line text into visual lines (word wrap unless the control
   scrolls horizontally). */
void edit_layout(Wnd *w, EditState *e)
{
  e->lines.clear();
  e->lines.push_back(0);
  if (!multiline(w)) return;
  const SfFontData *fd = ctl_font(w);
  int width = (w->client.right - w->client.left) - 2 * EDIT_MARGIN;
  bool wrap = !(w->style & (ES_AUTOHSCROLL | WS_HSCROLL));
  const std::string &t = e->text;
  size_t i = 0, lineStart = 0;
  int x = 0;
  size_t lastSpace = std::string::npos;
  while (i < t.size()) {
    if (t[i] == '\r' && i + 1 < t.size() && t[i + 1] == '\n') {
      i += 2;
      e->lines.push_back((int)i);
      lineStart = i; x = 0; lastSpace = std::string::npos;
      continue;
    }
    if (t[i] == '\n') {
      i += 1;
      e->lines.push_back((int)i);
      lineStart = i; x = 0; lastSpace = std::string::npos;
      continue;
    }
    int cw = fd->glyphs[(uint8_t)t[i]].adv;
    if (wrap && width > 0 && x + cw > width && i > lineStart) {
      size_t br = (lastSpace != std::string::npos && lastSpace >= lineStart) ? lastSpace + 1 : i;
      e->lines.push_back((int)br);
      lineStart = br;
      x = 0;
      for (size_t k = br; k < i; k++) x += fd->glyphs[(uint8_t)t[k]].adv;
      lastSpace = std::string::npos;
    }
    if (t[i] == ' ') lastSpace = i;
    x += cw;
    i++;
  }
}

int line_of(EditState *e, int pos)
{
  int l = 0;
  for (size_t k = 0; k < e->lines.size(); k++)
    if (e->lines[k] <= pos) l = (int)k;
  return l;
}

int line_end(EditState *e, int line)
{
  int end = (line + 1 < (int)e->lines.size()) ? e->lines[line + 1] : (int)e->text.size();
  /* exclude the line break */
  while (end > e->lines[line] && (e->text[end - 1] == '\n' || e->text[end - 1] == '\r')) end--;
  return end;
}

int visible_lines(Wnd *w)
{
  const SfFontData *fd = ctl_font(w);
  int h = w->client.bottom - w->client.top;
  return std::max(1, h / std::max(1, fd->height));
}

void edit_update_scrollbar(Wnd *w, EditState *e)
{
  if (!multiline(w) || !(w->style & WS_VSCROLL)) return;
  SCROLLINFO si;
  si.cbSize = sizeof(si);
  si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
  si.nMin = 0;
  si.nMax = (int)e->lines.size() - 1;
  si.nPage = visible_lines(w);
  si.nPos = e->firstLine;
  SetScrollInfo(w->hwnd, SB_VERT, &si, TRUE);
}

int x_of(Wnd *w, EditState *e, int pos)
{
  const SfFontData *fd = ctl_font(w);
  std::string s = shown(w, e);
  int ls = multiline(w) ? e->lines[line_of(e, pos)] : 0;
  return text_width(fd, s.c_str() + ls, pos - ls);
}

void edit_scroll_to_caret(Wnd *w, EditState *e)
{
  if (multiline(w)) {
    int l = line_of(e, e->caret);
    int vis = visible_lines(w);
    if (l < e->firstLine) e->firstLine = l;
    else if (l >= e->firstLine + vis) e->firstLine = l - vis + 1;
    if (e->firstLine < 0) e->firstLine = 0;
    edit_update_scrollbar(w, e);
  } else {
    int width = (w->client.right - w->client.left) - 2 * EDIT_MARGIN;
    int cx = x_of(w, e, e->caret);
    if (cx - e->scrollX > width - 1) e->scrollX = cx - width + width / 4;
    if (cx < e->scrollX) e->scrollX = std::max(0, cx - width / 4);
    if (!(w->style & ES_AUTOHSCROLL)) e->scrollX = 0;
  }
}

int pos_from_point(Wnd *w, EditState *e, int x, int y)
{
  const SfFontData *fd = ctl_font(w);
  std::string s = shown(w, e);
  int line = 0;
  if (multiline(w)) {
    line = e->firstLine + y / std::max(1, fd->height);
    if (line < 0) line = 0;
    if (line >= (int)e->lines.size()) line = (int)e->lines.size() - 1;
  }
  int start = multiline(w) ? e->lines[line] : 0;
  int end = multiline(w) ? line_end(e, line) : (int)s.size();
  int px = x - EDIT_MARGIN + (multiline(w) ? 0 : e->scrollX);
  int acc = 0;
  for (int i = start; i < end; i++) {
    int cw = fd->glyphs[(uint8_t)s[i]].adv;
    if (px < acc + cw / 2) return i;
    acc += cw;
  }
  return end;
}

void edit_changed(Wnd *w, EditState *e, bool notifyChange)
{
  edit_layout(w, e);
  edit_scroll_to_caret(w, e);
  InvalidateRect(w->hwnd, nullptr, TRUE);
  if (notifyChange) {
    e->modified = true;
    notify(w, EN_UPDATE);
    if (wnd(w->hwnd)) notify(w, EN_CHANGE);
  }
}

void edit_replace(Wnd *w, EditState *e, const std::string &in, bool notifyChange)
{
  int a = std::min(e->anchor, e->caret), b = std::max(e->anchor, e->caret);
  std::string ins = in;
  if (!multiline(w)) {
    /* single line: stop at the first line break */
    size_t nl = ins.find_first_of("\r\n");
    if (nl != std::string::npos) ins = ins.substr(0, nl);
  }
  if (w->style & ES_UPPERCASE) for (auto &c : ins) c = (char)toupper((unsigned char)c);
  if (w->style & ES_LOWERCASE) for (auto &c : ins) c = (char)tolower((unsigned char)c);
  int room = e->limit - ((int)e->text.size() - (b - a));
  bool truncated = false;
  if ((int)ins.size() > room) { ins = ins.substr(0, std::max(0, room)); truncated = true; }
  e->text.replace(a, b - a, ins);
  e->caret = e->anchor = a + (int)ins.size();
  edit_changed(w, e, notifyChange);
  if (truncated) notify(w, EN_MAXTEXT);
}

void edit_paint(Wnd *w)
{
  EditState *e = ed(w);
  Paint p(w->hwnd);
  if (!p.ok || !e) return;
  bool ro = readonly(w) || !w->enabled();
  Colors c = ctl_colors(w, p.dc, ro ? WM_CTLCOLORSTATIC : WM_CTLCOLOREDIT);
  p.fill(0, 0, p.W, p.Hh, c.hasBg ? c.bg : syscolor_px(COLOR_WINDOW));
  const SfFontData *fd = ctl_font(w);
  std::string s = shown(w, e);
  bool focus = has_focus(w->hwnd);
  bool showSel = focus || (w->style & ES_NOHIDESEL);
  int sa = std::min(e->anchor, e->caret), sb = std::max(e->anchor, e->caret);
  uint32_t tc = w->enabled() ? c.text : syscolor_px(COLOR_GRAYTEXT);
  int first = multiline(w) ? e->firstLine : 0;
  int nl = multiline(w) ? (int)e->lines.size() : 1;
  int y = multiline(w) ? 0 : std::max(0, (p.Hh - fd->height) / 2);
  for (int l = first; l < nl && y < p.Hh; l++) {
    int st = multiline(w) ? e->lines[l] : 0;
    int en = multiline(w) ? line_end(e, l) : (int)s.size();
    int x = EDIT_MARGIN - (multiline(w) ? 0 : e->scrollX);
    for (int i = st; i < en; i++) {
      const SfGlyph &g = fd->glyphs[(uint8_t)s[i]];
      bool sel = showSel && i >= sa && i < sb;
      if (sel) {
        p.fill(x, y, x + g.adv, y + fd->height, syscolor_px(COLOR_HIGHLIGHT));
        p.text(fd, x, y, std::string(1, s[i]), syscolor_px(COLOR_HIGHLIGHTTEXT));
      } else {
        p.text(fd, x, y, std::string(1, s[i]), tc);
      }
      x += g.adv;
    }
    if (focus && e->caretOn && line_of(e, e->caret) == l) {
      int cx = EDIT_MARGIN + x_of(w, e, e->caret) - (multiline(w) ? 0 : e->scrollX);
      p.fill(cx, y, cx + 1, y + fd->height, 0);
    }
    y += fd->height;
    if (!multiline(w)) break;
  }
  if (focus && e->caretOn && e->text.empty()) {
    int cy = multiline(w) ? 0 : std::max(0, (p.Hh - fd->height) / 2);
    p.fill(EDIT_MARGIN, cy, EDIT_MARGIN + 1, cy + fd->height, 0);
  }
}

void edit_move(Wnd *w, EditState *e, int pos, bool extend)
{
  if (pos < 0) pos = 0;
  if (pos > (int)e->text.size()) pos = (int)e->text.size();
  /* never stop inside a CR LF pair */
  if (pos > 0 && pos < (int)e->text.size() && e->text[pos - 1] == '\r' && e->text[pos] == '\n')
    pos += (pos > e->caret) ? 1 : -1;
  e->caret = pos;
  if (!extend) e->anchor = pos;
  e->caretOn = true;
  edit_scroll_to_caret(w, e);
  InvalidateRect(w->hwnd, nullptr, TRUE);
}

void edit_copy(EditState *e)
{
  int a = std::min(e->anchor, e->caret), b = std::max(e->anchor, e->caret);
  if (b > a) g_clipboard = e->text.substr(a, b - a);
}

}  /* namespace */

namespace w32 {

LRESULT CALLBACK edit_proc(HWND h, UINT m, WPARAM wp, LPARAM lp)
{
  Wnd *w = wnd(h);
  if (!w) return 0;
  EditState *e = ed(w);
  switch (m) {
  case WM_NCCREATE: {
    EditState *ne = new EditState();
    if (!(w->style & ES_MULTILINE)) ne->limit = 0x7FFFFFFE;
    if (w->style & ES_PASSWORD) ne->pw = '*';
    w->ctl = ne;
    w->ctl_free = [](void *p) { delete (EditState *)p; };
    CREATESTRUCT *cs = (CREATESTRUCT *)lp;
    if (cs && cs->lpszName && !IS_INTRESOURCE(cs->lpszName)) ne->text = cs->lpszName;
    ne->lines.push_back(0);
    return TRUE;
  }
  case WM_CREATE:
    edit_layout(w, e);
    edit_update_scrollbar(w, e);
    return 0;
  case WM_SIZE:
    edit_layout(w, e);
    edit_scroll_to_caret(w, e);
    return 0;
  case WM_SETFONT:
    w->font = (HFONT)wp;
    edit_layout(w, e);
    edit_update_scrollbar(w, e);
    if (LOWORD(lp)) InvalidateRect(h, nullptr, TRUE);
    return 0;
  case WM_GETFONT:
    return (LRESULT)w->font;
  case WM_PAINT:
    edit_paint(w);
    return 0;
  case WM_ERASEBKGND:
    return 1;
  case WM_ENABLE:
    InvalidateRect(h, nullptr, TRUE);
    return 0;
  case WM_SETTEXT:
    e->text = lp ? (const char *)lp : "";
    if ((int)e->text.size() > e->limit) e->text.resize(e->limit);
    e->anchor = e->caret = 0;
    e->scrollX = 0;
    e->firstLine = 0;
    e->modified = false;
    edit_layout(w, e);
    edit_update_scrollbar(w, e);
    InvalidateRect(h, nullptr, TRUE);
    if (!multiline(w)) notify(w, EN_CHANGE);
    return TRUE;
  case WM_GETTEXT: {
    if (!lp || wp == 0) return 0;
    size_t n = std::min(e->text.size(), (size_t)wp - 1);
    memcpy((char *)lp, e->text.data(), n);
    ((char *)lp)[n] = 0;
    return (LRESULT)n;
  }
  case WM_GETTEXTLENGTH:
    return (LRESULT)e->text.size();
  case WM_GETDLGCODE: {
    LRESULT r = DLGC_WANTCHARS | DLGC_HASSETSEL | DLGC_WANTARROWS;
    if (multiline(w)) r |= DLGC_WANTALLKEYS;
    return r;
  }
  case WM_SETFOCUS:
    e->caretOn = true;
    SetTimer(h, 1, 530, nullptr);
    InvalidateRect(h, nullptr, TRUE);
    notify(w, EN_SETFOCUS);
    return 0;
  case WM_KILLFOCUS:
    e->caretOn = false;
    KillTimer(h, 1);
    if (e->selecting) { e->selecting = false; ReleaseCapture(); }
    InvalidateRect(h, nullptr, TRUE);
    notify(w, EN_KILLFOCUS);
    return 0;
  case WM_TIMER:
    if (wp == 1) {
      e->caretOn = !e->caretOn;
      InvalidateRect(h, nullptr, FALSE);
    }
    return 0;
  case WM_LBUTTONDOWN: {
    if (GetFocus() != h) SetFocus(h);
    int pos = pos_from_point(w, e, GET_X_LPARAM(lp), GET_Y_LPARAM(lp));
    edit_move(w, e, pos, (wp & MK_SHIFT) != 0);
    e->selecting = true;
    SetCapture(h);
    return 0;
  }
  case WM_MOUSEMOVE:
    if (e->selecting) {
      int pos = pos_from_point(w, e, GET_X_LPARAM(lp), GET_Y_LPARAM(lp));
      if (pos != e->caret) edit_move(w, e, pos, true);
    }
    return 0;
  case WM_LBUTTONUP:
    if (e->selecting) { e->selecting = false; ReleaseCapture(); }
    return 0;
  case WM_LBUTTONDBLCLK: {
    int pos = pos_from_point(w, e, GET_X_LPARAM(lp), GET_Y_LPARAM(lp));
    int a = pos, b = pos;
    while (a > 0 && !isspace((unsigned char)e->text[a - 1])) a--;
    while (b < (int)e->text.size() && !isspace((unsigned char)e->text[b])) b++;
    e->anchor = a;
    e->caret = b;
    InvalidateRect(h, nullptr, TRUE);
    return 0;
  }
  case WM_CAPTURECHANGED:
    e->selecting = false;
    return 0;
  case WM_CHAR: {
    unsigned ch = (unsigned)wp;
    if (ch == 3) { edit_copy(e); return 0; }
    if (readonly(w)) return 0;
    if (ch == 22) { edit_replace(w, e, g_clipboard, true); return 0; }
    if (ch == 24) { edit_copy(e); edit_replace(w, e, "", true); return 0; }
    if (ch == 8) {
      if (e->anchor == e->caret) {
        if (e->caret == 0) return 0;
        int k = e->caret - 1;
        if (k > 0 && e->text[k] == '\n' && e->text[k - 1] == '\r') k--;
        e->anchor = k;
      }
      edit_replace(w, e, "", true);
      return 0;
    }
    if (ch == '\r' || ch == '\n') {
      if (multiline(w) && (w->style & ES_WANTRETURN)) edit_replace(w, e, "\r\n", true);
      return 0;
    }
    if (ch == '\t') return 0;
    if (ch < 32 || ch == 127) return 0;
    if ((w->style & ES_NUMBER) && !isdigit((int)ch)) { MessageBeep(0); return 0; }
    edit_replace(w, e, std::string(1, (char)ch), true);
    return 0;
  }
  case WM_KEYDOWN: {
    bool shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
    bool ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
    int l = line_of(e, e->caret);
    switch (wp) {
    case VK_LEFT:
      if (ctrl) {
        int k = e->caret;
        while (k > 0 && isspace((unsigned char)e->text[k - 1])) k--;
        while (k > 0 && !isspace((unsigned char)e->text[k - 1])) k--;
        edit_move(w, e, k, shift);
      } else if (!shift && e->anchor != e->caret) edit_move(w, e, std::min(e->anchor, e->caret), false);
      else edit_move(w, e, e->caret - 1, shift);
      return 0;
    case VK_RIGHT:
      if (ctrl) {
        int k = e->caret, n = (int)e->text.size();
        while (k < n && !isspace((unsigned char)e->text[k])) k++;
        while (k < n && isspace((unsigned char)e->text[k])) k++;
        edit_move(w, e, k, shift);
      } else if (!shift && e->anchor != e->caret) edit_move(w, e, std::max(e->anchor, e->caret), false);
      else edit_move(w, e, e->caret + 1, shift);
      return 0;
    case VK_HOME:
      edit_move(w, e, (ctrl || !multiline(w)) ? 0 : e->lines[l], shift);
      return 0;
    case VK_END:
      edit_move(w, e, (ctrl || !multiline(w)) ? (int)e->text.size() : line_end(e, l), shift);
      return 0;
    case VK_UP: case VK_DOWN: case VK_PRIOR: case VK_NEXT:
      if (multiline(w)) {
        int step = (wp == VK_UP || wp == VK_DOWN) ? 1 : visible_lines(w);
        int nlRow = l + ((wp == VK_DOWN || wp == VK_NEXT) ? step : -step);
        if (nlRow < 0) nlRow = 0;
        if (nlRow >= (int)e->lines.size()) nlRow = (int)e->lines.size() - 1;
        int x = x_of(w, e, e->caret);
        const SfFontData *fd = ctl_font(w);
        int pos = pos_from_point(w, e, x + EDIT_MARGIN, (nlRow - e->firstLine) * fd->height);
        edit_move(w, e, pos, shift);
      }
      return 0;
    case VK_DELETE:
      if (readonly(w)) return 0;
      if (shift) { edit_copy(e); edit_replace(w, e, "", true); return 0; }
      if (e->anchor == e->caret) {
        if (e->caret >= (int)e->text.size()) return 0;
        int k = e->caret + 1;
        if (e->text[e->caret] == '\r' && k < (int)e->text.size() && e->text[k] == '\n') k++;
        e->anchor = k;
      }
      edit_replace(w, e, "", true);
      return 0;
    case VK_INSERT:
      if (ctrl) edit_copy(e);
      else if (shift && !readonly(w)) edit_replace(w, e, g_clipboard, true);
      return 0;
    default:
      return 0;
    }
  }
  case WM_VSCROLL: {
    if (!multiline(w)) return 0;
    int vis = visible_lines(w), n = (int)e->lines.size();
    int fl = e->firstLine;
    switch (LOWORD(wp)) {
    case SB_LINEUP: fl--; break;
    case SB_LINEDOWN: fl++; break;
    case SB_PAGEUP: fl -= vis; break;
    case SB_PAGEDOWN: fl += vis; break;
    case SB_THUMBTRACK: case SB_THUMBPOSITION: fl = (short)HIWORD(wp); break;
    case SB_TOP: fl = 0; break;
    case SB_BOTTOM: fl = n; break;
    default: return 0;
    }
    fl = std::max(0, std::min(fl, std::max(0, n - vis)));
    if (fl != e->firstLine) {
      e->firstLine = fl;
      edit_update_scrollbar(w, e);
      InvalidateRect(h, nullptr, TRUE);
    }
    return 0;
  }
  case WM_MOUSEWHEEL:
    if (multiline(w)) {
      int d = (short)HIWORD(wp) / 120;
      for (int k = 0; k < 3 * std::abs(d); k++)
        edit_proc(h, WM_VSCROLL, d > 0 ? SB_LINEUP : SB_LINEDOWN, 0);
    }
    return 0;
  case EM_GETSEL: {
    int a = std::min(e->anchor, e->caret), b = std::max(e->anchor, e->caret);
    if (wp) *(DWORD *)wp = a;
    if (lp) *(DWORD *)lp = b;
    return MAKELONG(a, b);
  }
  case EM_SETSEL: {
    int a = (int)(INT_PTR)wp, b = (int)(INT_PTR)lp, n = (int)e->text.size();
    if (a == -1) { e->anchor = e->caret; InvalidateRect(h, nullptr, TRUE); return 0; }
    if (a < 0 || a > n) a = n;
    if (b < 0 || b > n) b = n;
    e->anchor = a;
    e->caret = b;
    edit_scroll_to_caret(w, e);
    InvalidateRect(h, nullptr, TRUE);
    return 0;
  }
  case EM_REPLACESEL:
    edit_replace(w, e, lp ? (const char *)lp : "", true);
    return 0;
  case EM_SETLIMITTEXT:
    e->limit = wp ? (int)wp : (multiline(w) ? 0x7FFFFFFE : 0x7FFFFFFE);
    return 0;
  case EM_GETLIMITTEXT:
    return e->limit;
  case EM_SETREADONLY: {
    std::lock_guard<std::recursive_mutex> lk(g_wm);
    if (wp) w->style |= ES_READONLY;
    else w->style &= ~ES_READONLY;
    wnd_invalidate(w, nullptr, true);
    return TRUE;
  }
  case EM_GETLINECOUNT:
    return multiline(w) ? (LRESULT)e->lines.size() : 1;
  case EM_GETFIRSTVISIBLELINE:
    return multiline(w) ? e->firstLine : 0;
  case EM_LINEINDEX: {
    int l = (int)(INT_PTR)wp;
    if (l == -1) l = line_of(e, e->caret);
    if (l < 0 || l >= (int)e->lines.size()) return -1;
    return e->lines[l];
  }
  case EM_LINELENGTH: {
    int pos = (int)(INT_PTR)wp;
    if (pos == -1) pos = e->caret;
    int l = line_of(e, pos);
    return line_end(e, l) - e->lines[l];
  }
  case EM_LINEFROMCHAR: {
    int pos = (int)(INT_PTR)wp;
    if (pos == -1) pos = e->caret;
    return line_of(e, pos);
  }
  case EM_GETLINE: {
    int l = (int)wp;
    if (l < 0 || l >= (int)e->lines.size() || !lp) return 0;
    WORD max = *(WORD *)lp;
    int n = std::min((int)max, line_end(e, l) - e->lines[l]);
    memcpy((char *)lp, e->text.data() + e->lines[l], n);
    return n;
  }
  case EM_SCROLL: {
    if (!multiline(w)) return 0;
    int before = e->firstLine;
    edit_proc(h, WM_VSCROLL, wp, 0);
    return MAKELRESULT(e->firstLine - before, TRUE);
  }
  case EM_LINESCROLL: {
    if (!multiline(w)) return FALSE;
    int vis = visible_lines(w), n = (int)e->lines.size();
    e->firstLine = std::max(0, std::min(e->firstLine + (int)(INT_PTR)lp, std::max(0, n - vis)));
    edit_update_scrollbar(w, e);
    InvalidateRect(h, nullptr, TRUE);
    return TRUE;
  }
  case EM_SCROLLCARET:
    edit_scroll_to_caret(w, e);
    InvalidateRect(h, nullptr, TRUE);
    return TRUE;
  case EM_SETPASSWORDCHAR:
    e->pw = (char)wp;
    InvalidateRect(h, nullptr, TRUE);
    return 0;
  case EM_GETPASSWORDCHAR:
    return (unsigned char)e->pw;
  case EM_GETMODIFY:
    return e->modified;
  case EM_SETMODIFY:
    e->modified = wp != 0;
    return 0;
  case EM_CANUNDO:
    return FALSE;
  case EM_UNDO: case WM_UNDO:
    return FALSE;
  case WM_COPY: edit_copy(e); return 0;
  case WM_CUT: if (!readonly(w)) { edit_copy(e); edit_replace(w, e, "", true); } return 0;
  case WM_PASTE: if (!readonly(w)) edit_replace(w, e, g_clipboard, true); return 0;
  case WM_CLEAR: if (!readonly(w)) edit_replace(w, e, "", true); return 0;
  default:
    return DefWindowProc(h, m, wp, lp);
  }
}

}  /* namespace w32 */

/* ================================ LISTBOX ================================ */
namespace {

struct LbItem { std::string text; LPARAM data = 0; };

struct ListState {
  std::vector<LbItem> items;
  int cur = -1;      /* selection (single-selection list boxes) */
  int caret = 0;     /* focus item */
  int top = 0;
  int itemH = 0;
  int colW = 0;
  int hext = 0;
  bool tracking = false;
  /* drop-down list of a combo box */
  HWND combo = 0;
  int hot = -1;
};

ListState *ls(Wnd *w) { return w ? (ListState *)w->ctl : nullptr; }

int lb_item_h(Wnd *w, ListState *s)
{
  return s->itemH ? s->itemH : ctl_font(w)->height;
}

int lb_visible(Wnd *w, ListState *s)
{
  int h = w->client.bottom - w->client.top;
  return std::max(1, h / std::max(1, lb_item_h(w, s)));
}

bool lb_multicol(Wnd *w) { return (w->style & LBS_MULTICOLUMN) != 0; }

int lb_rows(Wnd *w, ListState *s) { return lb_visible(w, s); }

void lb_fix_scroll(Wnd *w, ListState *s)
{
  int n = (int)s->items.size();
  if (lb_multicol(w)) {
    int rows = lb_rows(w, s);
    int cols = rows ? (n + rows - 1) / rows : 0;
    if (s->top < 0) s->top = 0;
    if (s->top > std::max(0, cols - 1) * rows) s->top = std::max(0, cols - 1) * rows;
    return;
  }
  int vis = lb_visible(w, s);
  if (s->top > n - vis) s->top = n - vis;
  if (s->top < 0) s->top = 0;
  /* standard scroll bar: shown only when needed */
  if (w->style & WS_VSCROLL || (n > vis)) {
    bool need = n > vis;
    bool always = (w->style & LBS_DISABLENOSCROLL) != 0;
    HWND h = w->hwnd;
    if (need || always) {
      SCROLLINFO si;
      si.cbSize = sizeof(si);
      si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
      si.nMin = 0;
      si.nMax = std::max(0, n - 1);
      si.nPage = vis;
      si.nPos = s->top;
      SetScrollInfo(h, SB_VERT, &si, TRUE);
    }
  }
}

void lb_update_bars(Wnd *w, ListState *s)
{
  if (s->combo) { lb_fix_scroll(w, s); return; }
  int n = (int)s->items.size();
  int vis = lb_visible(w, s);
  bool need = n > vis && !lb_multicol(w);
  bool have = (w->style & WS_VSCROLL) != 0;
  bool always = (w->style & LBS_DISABLENOSCROLL) != 0;
  if (!always && need != have && w->cls) {
    /* only list boxes created with WS_VSCROLL get the bar */
    if (!(w->exstyle & 0x80000000u) /* never set: WS_VSCROLL was requested at create */) {}
  }
  lb_fix_scroll(w, s);
}

void lb_ensure_visible(Wnd *w, ListState *s, int i)
{
  if (i < 0) return;
  int vis = lb_visible(w, s);
  if (lb_multicol(w)) {
    int rows = lb_rows(w, s);
    int col = i / rows, topCol = s->top / rows;
    int cw = s->colW ? s->colW : (w->client.right - w->client.left);
    int visCols = std::max(1, (int)(w->client.right - w->client.left) / std::max(1, cw));
    if (col < topCol) s->top = col * rows;
    else if (col >= topCol + visCols) s->top = (col - visCols + 1) * rows;
    return;
  }
  if (i < s->top) s->top = i;
  else if (i >= s->top + vis) s->top = i - vis + 1;
  lb_fix_scroll(w, s);
}

int lb_item_at(Wnd *w, ListState *s, int x, int y)
{
  int ih = lb_item_h(w, s);
  if (lb_multicol(w)) {
    int rows = lb_rows(w, s);
    int cw = s->colW ? s->colW : (w->client.right - w->client.left);
    int col = x / std::max(1, cw), row = y / std::max(1, ih);
    if (row >= rows) row = rows - 1;
    int i = s->top + col * rows + row;
    return (i >= 0 && i < (int)s->items.size()) ? i : -1;
  }
  int i = s->top + y / std::max(1, ih);
  if (y < 0) i = s->top - 1;
  return i;
}

void lb_paint(Wnd *w)
{
  ListState *s = ls(w);
  Paint p(w->hwnd);
  if (!p.ok || !s) return;
  Colors c = ctl_colors(w, p.dc, WM_CTLCOLORLISTBOX);
  uint32_t bg = c.hasBg ? c.bg : syscolor_px(COLOR_WINDOW);
  if (!w->enabled() && !s->combo) bg = syscolor_px(COLOR_BTNFACE);
  p.fill(0, 0, p.W, p.Hh, bg);
  const SfFontData *fd = ctl_font(w);
  int ih = lb_item_h(w, s);
  bool focus = has_focus(w->hwnd);
  int rows = lb_rows(w, s);
  int cw = lb_multicol(w) ? (s->colW ? s->colW : p.W) : p.W;
  for (int i = s->top; i < (int)s->items.size(); i++) {
    int k = i - s->top;
    int x = 0, y = k * ih;
    if (lb_multicol(w)) { x = (k / rows) * cw; y = (k % rows) * ih; if (x >= p.W) break; }
    else if (y >= p.Hh) break;
    bool sel = s->combo ? (i == s->hot) : (i == s->cur);
    uint32_t tc = w->enabled() ? c.text : syscolor_px(COLOR_GRAYTEXT);
    if (s->combo) tc = syscolor_px(COLOR_WINDOWTEXT);
    if (sel) {
      p.fill(x, y, x + cw, y + ih, syscolor_px(COLOR_HIGHLIGHT));
      tc = syscolor_px(COLOR_HIGHLIGHTTEXT);
    }
    RECT tr = {x + 2, y, x + cw, y + ih};
    p.textin(fd, s->items[i].text.c_str(), tr, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX, tc);
    if (focus && i == s->caret && !s->combo) p.focus(RECT{x, y, x + cw, y + ih});
  }
  if (focus && s->items.empty() && !s->combo) p.focus(RECT{0, 0, p.W, ih});
}

void lb_select(Wnd *w, ListState *s, int i, bool notifySel)
{
  int n = (int)s->items.size();
  if (n == 0) return;
  if (i < 0) i = 0;
  if (i >= n) i = n - 1;
  s->caret = i;
  bool changed = s->cur != i;
  s->cur = i;
  lb_ensure_visible(w, s, i);
  InvalidateRect(w->hwnd, nullptr, TRUE);
  if (changed && notifySel && (w->style & LBS_NOTIFY)) notify(w, LBN_SELCHANGE);
}

int lb_find(ListState *s, int after, const char *str, bool exact)
{
  int n = (int)s->items.size();
  if (!str || !n) return LB_ERR;
  size_t len = strlen(str);
  int start = (after < 0 || after >= n) ? 0 : after + 1;
  for (int k = 0; k < n; k++) {
    int i = (start + k) % n;
    const std::string &t = s->items[i].text;
    if (exact) { if (strcasecmp(t.c_str(), str) == 0) return i; }
    else if (strncasecmp(t.c_str(), str, len) == 0) return i;
  }
  return LB_ERR;
}

}  /* namespace */

namespace w32 {

LRESULT CALLBACK listbox_proc(HWND h, UINT m, WPARAM wp, LPARAM lp)
{
  Wnd *w = wnd(h);
  if (!w) return 0;
  ListState *s = ls(w);
  switch (m) {
  case WM_NCCREATE: {
    ListState *ns = new ListState();
    w->ctl = ns;
    w->ctl_free = [](void *p) { delete (ListState *)p; };
    return DefWindowProc(h, m, wp, lp);
  }
  case WM_CREATE: {
    /* the standard bar appears only when the list needs it */
    if ((w->style & WS_VSCROLL) && !(w->style & LBS_DISABLENOSCROLL)) ShowScrollBar(h, SB_VERT, FALSE);
    if ((w->style & WS_HSCROLL)) ShowScrollBar(h, SB_HORZ, FALSE);
    return 0;
  }
  case WM_SETFONT:
    w->font = (HFONT)wp;
    if (LOWORD(lp)) InvalidateRect(h, nullptr, TRUE);
    return 0;
  case WM_GETFONT:
    return (LRESULT)w->font;
  case WM_SIZE:
    lb_fix_scroll(w, s);
    return 0;
  case WM_PAINT:
    lb_paint(w);
    return 0;
  case WM_ERASEBKGND:
    return 1;
  case WM_ENABLE:
    InvalidateRect(h, nullptr, TRUE);
    return 0;
  case WM_GETDLGCODE:
    return DLGC_WANTARROWS | DLGC_WANTCHARS;
  case WM_SETFOCUS:
    InvalidateRect(h, nullptr, TRUE);
    notify(w, LBN_SETFOCUS);
    return 0;
  case WM_KILLFOCUS:
    InvalidateRect(h, nullptr, TRUE);
    notify(w, LBN_KILLFOCUS);
    return 0;
  case WM_LBUTTONDOWN: case WM_LBUTTONDBLCLK: {
    if (!s->combo && GetFocus() != h) SetFocus(h);
    int i = lb_item_at(w, s, GET_X_LPARAM(lp), GET_Y_LPARAM(lp));
    if (i >= 0 && i < (int)s->items.size()) lb_select(w, s, i, true);
    if (m == WM_LBUTTONDOWN) { s->tracking = true; SetCapture(h); }
    else if (w->style & LBS_NOTIFY) notify(w, LBN_DBLCLK);
    return 0;
  }
  case WM_MOUSEMOVE:
    if (s->tracking) {
      int i = lb_item_at(w, s, GET_X_LPARAM(lp), GET_Y_LPARAM(lp));
      if (i >= (int)s->items.size()) i = (int)s->items.size() - 1;
      if (i != s->cur && !s->items.empty()) lb_select(w, s, i, true);
    }
    return 0;
  case WM_LBUTTONUP:
    if (s->tracking) { s->tracking = false; ReleaseCapture(); }
    return 0;
  case WM_CAPTURECHANGED:
    s->tracking = false;
    return 0;
  case WM_KEYDOWN: {
    int n = (int)s->items.size();
    if (!n) return 0;
    int vis = lb_visible(w, s);
    int i = s->cur < 0 ? 0 : s->cur;
    switch (wp) {
    case VK_UP: i--; break;
    case VK_DOWN: i++; break;
    case VK_PRIOR: i -= vis - 1; break;
    case VK_NEXT: i += vis - 1; break;
    case VK_HOME: i = 0; break;
    case VK_END: i = n - 1; break;
    case VK_LEFT: i -= lb_multicol(w) ? lb_rows(w, s) : 1; break;
    case VK_RIGHT: i += lb_multicol(w) ? lb_rows(w, s) : 1; break;
    default: return 0;
    }
    lb_select(w, s, i, true);
    return 0;
  }
  case WM_CHAR: {
    char str[2] = {(char)wp, 0};
    if ((unsigned char)wp < 32) return 0;
    int i = lb_find(s, s->cur, str, false);
    if (i != LB_ERR) lb_select(w, s, i, true);
    return 0;
  }
  case WM_VSCROLL: case WM_HSCROLL: {
    int vis = lb_visible(w, s), n = (int)s->items.size();
    int t = s->top;
    switch (LOWORD(wp)) {
    case SB_LINEUP: t--; break;
    case SB_LINEDOWN: t++; break;
    case SB_PAGEUP: t -= vis - 1; break;
    case SB_PAGEDOWN: t += vis - 1; break;
    case SB_THUMBTRACK: case SB_THUMBPOSITION: t = (short)HIWORD(wp); break;
    case SB_TOP: t = 0; break;
    case SB_BOTTOM: t = n; break;
    default: return 0;
    }
    if (lb_multicol(w)) t = s->top + (t - s->top) * lb_rows(w, s);
    s->top = t;
    lb_fix_scroll(w, s);
    InvalidateRect(h, nullptr, TRUE);
    return 0;
  }
  case WM_MOUSEWHEEL: {
    int d = (short)HIWORD(wp) / 120;
    s->top -= 3 * d;
    lb_fix_scroll(w, s);
    InvalidateRect(h, nullptr, TRUE);
    return 0;
  }
  case LB_ADDSTRING: case LB_INSERTSTRING: {
    LbItem it;
    it.text = lp ? (const char *)lp : "";
    int idx;
    if (m == LB_ADDSTRING && (w->style & LBS_SORT)) {
      idx = 0;
      while (idx < (int)s->items.size() && strcasecmp(s->items[idx].text.c_str(), it.text.c_str()) <= 0) idx++;
    } else if (m == LB_ADDSTRING || (int)(INT_PTR)wp < 0 || (int)wp > (int)s->items.size()) {
      idx = (int)s->items.size();
    } else {
      idx = (int)wp;
    }
    s->items.insert(s->items.begin() + idx, it);
    if (s->cur >= idx && m == LB_INSERTSTRING) s->cur++;
    lb_fix_scroll(w, s);
    InvalidateRect(h, nullptr, TRUE);
    return idx;
  }
  case LB_DELETESTRING: {
    int i = (int)wp;
    if (i < 0 || i >= (int)s->items.size()) return LB_ERR;
    s->items.erase(s->items.begin() + i);
    if (s->cur == i) s->cur = -1;
    else if (s->cur > i) s->cur--;
    if (s->caret >= (int)s->items.size()) s->caret = std::max(0, (int)s->items.size() - 1);
    lb_fix_scroll(w, s);
    InvalidateRect(h, nullptr, TRUE);
    return (LRESULT)s->items.size();
  }
  case LB_RESETCONTENT:
    s->items.clear();
    s->cur = -1;
    s->caret = 0;
    s->top = 0;
    lb_fix_scroll(w, s);
    InvalidateRect(h, nullptr, TRUE);
    return 0;
  case LB_GETCOUNT:
    return (LRESULT)s->items.size();
  case LB_GETCURSEL:
    return s->cur;
  case LB_SETCURSEL: {
    int i = (int)(INT_PTR)wp;
    if (i < 0 || i >= (int)s->items.size()) { s->cur = -1; InvalidateRect(h, nullptr, TRUE); return LB_ERR; }
    s->cur = i;
    s->caret = i;
    lb_ensure_visible(w, s, i);
    InvalidateRect(h, nullptr, TRUE);
    return i;
  }
  case LB_GETSEL:
    return (int)wp == s->cur;
  case LB_SETSEL:
    if (wp) return listbox_proc(h, LB_SETCURSEL, (WPARAM)lp, 0);
    if ((int)(INT_PTR)lp == s->cur || (int)(INT_PTR)lp == -1) { s->cur = -1; InvalidateRect(h, nullptr, TRUE); }
    return 0;
  case LB_GETTEXT: {
    int i = (int)wp;
    if (i < 0 || i >= (int)s->items.size() || !lp) return LB_ERR;
    strcpy((char *)lp, s->items[i].text.c_str());
    return (LRESULT)s->items[i].text.size();
  }
  case LB_GETTEXTLEN: {
    int i = (int)wp;
    if (i < 0 || i >= (int)s->items.size()) return LB_ERR;
    return (LRESULT)s->items[i].text.size();
  }
  case LB_GETTOPINDEX:
    return s->top;
  case LB_SETTOPINDEX:
    s->top = (int)wp;
    lb_fix_scroll(w, s);
    InvalidateRect(h, nullptr, TRUE);
    return 0;
  case LB_FINDSTRING:
    return lb_find(s, (int)(INT_PTR)wp, (const char *)lp, false);
  case LB_FINDSTRINGEXACT:
    return lb_find(s, (int)(INT_PTR)wp, (const char *)lp, true);
  case LB_SELECTSTRING: {
    int i = lb_find(s, (int)(INT_PTR)wp, (const char *)lp, false);
    if (i != LB_ERR) listbox_proc(h, LB_SETCURSEL, i, 0);
    return i;
  }
  case LB_GETITEMDATA: {
    int i = (int)wp;
    if (i < 0 || i >= (int)s->items.size()) return LB_ERR;
    return s->items[i].data;
  }
  case LB_SETITEMDATA: {
    int i = (int)wp;
    if (i < 0 || i >= (int)s->items.size()) return LB_ERR;
    s->items[i].data = lp;
    return 0;
  }
  case LB_GETITEMHEIGHT:
    return lb_item_h(w, s);
  case LB_SETITEMHEIGHT:
    s->itemH = (int)LOWORD(lp);
    InvalidateRect(h, nullptr, TRUE);
    return 0;
  case LB_GETITEMRECT: {
    RECT *r = (RECT *)lp;
    if (!r) return LB_ERR;
    int ih = lb_item_h(w, s);
    int k = (int)wp - s->top;
    r->left = 0;
    r->right = w->client.right - w->client.left;
    r->top = k * ih;
    r->bottom = r->top + ih;
    return 0;
  }
  case LB_SETCOLUMNWIDTH:
    s->colW = (int)wp;
    InvalidateRect(h, nullptr, TRUE);
    return 0;
  case LB_SETHORIZONTALEXTENT:
    s->hext = (int)wp;
    return 0;
  case LB_GETHORIZONTALEXTENT:
    return s->hext;
  case LB_GETSELCOUNT:
    return s->cur >= 0 ? 1 : 0;
  default:
    return DefWindowProc(h, m, wp, lp);
  }
}

}  /* namespace w32 */

/* =============================== COMBOBOX ================================ */
namespace {

struct ComboState {
  std::vector<LbItem> items;
  int cur = -1;
  bool dropped = false;
  bool btnPressed = false;
  HWND list = 0;
  int dropH = 0;
  int itemH = 0;
  int selH = 0;
};

ComboState *cs_of(Wnd *w) { return w ? (ComboState *)w->ctl : nullptr; }

int cb_item_h(Wnd *w, ComboState *c) { return c->itemH ? c->itemH : ctl_font(w)->height; }

int cb_field_h(Wnd *w, ComboState *c) { return c->selH ? c->selH : ctl_font(w)->height + 2; }

void cb_resize(Wnd *w, ComboState *c)
{
  int want = cb_field_h(w, c) + 6;
  int cur = w->rc.bottom - w->rc.top;
  if (cur != want)
    SetWindowPos(w->hwnd, 0, 0, 0, w->rc.right - w->rc.left, want,
                 SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
}

void cb_paint(Wnd *w)
{
  ComboState *c = cs_of(w);
  Paint p(w->hwnd);
  if (!p.ok || !c) return;
  Colors col = ctl_colors(w, p.dc, WM_CTLCOLORLISTBOX);
  bool en = w->enabled();
  uint32_t bg = en ? (col.hasBg ? col.bg : syscolor_px(COLOR_WINDOW)) : syscolor_px(COLOR_BTNFACE);
  int bw = M_CXVSCROLL;
  p.fill(0, 0, p.W - bw, p.Hh, bg);
  /* the drop button */
  RECT br = {p.W - bw, 0, p.W, p.Hh};
  p.fill(br.left, br.top, br.right, br.bottom, syscolor_px(COLOR_BTNFACE));
  if (c->btnPressed) p.rect3d(br.left, br.top, br.right, br.bottom, syscolor_px(COLOR_BTNSHADOW), syscolor_px(COLOR_BTNSHADOW));
  else p.edge(br, EDGE_RAISED, BF_RECT);
  {
    int ax = (br.left + br.right) / 2 + (c->btnPressed ? 1 : 0), ay = (br.top + br.bottom) / 2 + (c->btnPressed ? 1 : 0);
    Target &t = p.t;
    if (en) draw_arrow(t, ax + t.ox, ay + t.oy, 1, syscolor_px(COLOR_BTNTEXT), 4);
    else {
      draw_arrow(t, ax + 1 + t.ox, ay + 1 + t.oy, 1, syscolor_px(COLOR_BTNHIGHLIGHT), 4);
      draw_arrow(t, ax + t.ox, ay + t.oy, 1, syscolor_px(COLOR_BTNSHADOW), 4);
    }
  }
  bool focus = has_focus(w->hwnd) && !c->dropped;
  RECT fr = {1, 1, p.W - bw - 1, p.Hh - 1};
  uint32_t tc = en ? (col.text) : syscolor_px(COLOR_GRAYTEXT);
  if (focus) {
    p.fill(fr.left, fr.top, fr.right, fr.bottom, syscolor_px(COLOR_HIGHLIGHT));
    tc = syscolor_px(COLOR_HIGHLIGHTTEXT);
  }
  if (c->cur >= 0 && c->cur < (int)c->items.size()) {
    RECT tr = {fr.left + 2, fr.top, fr.right, fr.bottom};
    p.textin(ctl_font(w), c->items[c->cur].text.c_str(), tr, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX, tc);
  }
  if (focus) p.focus(fr);
}

void cb_close(Wnd *w, ComboState *c, bool accept);

void cb_drop(Wnd *w, ComboState *c)
{
  if (c->dropped || !w->enabled()) return;
  notify(w, CBN_DROPDOWN);
  if (!wnd(w->hwnd)) return;
  RECT r;
  GetWindowRect(w->hwnd, &r);
  int ih = cb_item_h(w, c);
  int n = std::max(1, (int)c->items.size());
  int maxH = std::max(ih + 2, c->dropH);
  int h = std::min(n * ih + 2, maxH);
  h = std::max(h, ih + 2);
  int y = r.bottom;
  if (y + h > g_screen_h) y = r.top - h;
  HWND lb = CreateWindowEx(WS_EX_TOOLWINDOW | WS_EX_TOPMOST, "ComboLBox", "",
                           WS_POPUP | WS_BORDER | (n * ih + 2 > maxH ? WS_VSCROLL : 0),
                           r.left, y, r.right - r.left, h, w->hwnd, 0, 0, nullptr);
  if (!lb) return;
  c->list = lb;
  c->dropped = true;
  Wnd *lw = wnd(lb);
  ListState *s = ls(lw);
  s->combo = w->hwnd;
  s->items = c->items;
  s->itemH = ih;
  s->cur = c->cur;
  s->hot = c->cur;
  lw->font = w->font;
  lb_ensure_visible(lw, s, c->cur);
  ShowWindow(lb, SW_SHOWNA);
  InvalidateRect(w->hwnd, nullptr, TRUE);
  SetCapture(lb);
}

void cb_set(Wnd *w, ComboState *c, int i, bool notifySel)
{
  if (i < 0 || i >= (int)c->items.size()) return;
  bool changed = c->cur != i;
  c->cur = i;
  InvalidateRect(w->hwnd, nullptr, TRUE);
  if (changed && notifySel) notify(w, CBN_SELCHANGE);
}

void cb_close(Wnd *w, ComboState *c, bool accept)
{
  if (!c->dropped) return;
  HWND lb = c->list;
  int hot = -1;
  if (Wnd *lw = wnd(lb)) hot = ls(lw)->hot;
  c->dropped = false;
  c->list = 0;
  c->btnPressed = false;
  if (GetCapture() == lb) ReleaseCapture();
  DestroyWindow(lb);
  InvalidateRect(w->hwnd, nullptr, TRUE);
  if (accept && hot >= 0 && hot != c->cur) {
    c->cur = hot;
    notify(w, CBN_SELCHANGE);
  }
  if (!wnd(w->hwnd)) return;
  notify(w, accept ? CBN_SELENDOK : CBN_SELENDCANCEL);
  if (!wnd(w->hwnd)) return;
  notify(w, CBN_CLOSEUP);
}

}  /* namespace */

namespace w32 {

LRESULT CALLBACK combolbox_proc(HWND h, UINT m, WPARAM wp, LPARAM lp)
{
  Wnd *w = wnd(h);
  if (!w) return 0;
  ListState *s = ls(w);
  switch (m) {
  case WM_MOUSEACTIVATE:
    return MA_NOACTIVATE;
  case WM_MOUSEMOVE: case WM_LBUTTONDOWN: case WM_LBUTTONUP: case WM_LBUTTONDBLCLK: {
    Wnd *cw = wnd(s ? s->combo : 0);
    ComboState *c = cs_of(cw);
    if (!cw || !c) return 0;
    int x = GET_X_LPARAM(lp), y = GET_Y_LPARAM(lp);
    RECT cl;
    GetClientRect(h, &cl);
    POINT pt = {x, y};
    bool inside = PtInRect(&cl, pt) != 0;
    if (inside) {
      int i = lb_item_at(w, s, x, y);
      if (i >= 0 && i < (int)s->items.size() && i != s->hot) { s->hot = i; InvalidateRect(h, nullptr, TRUE); }
    }
    if (m == WM_LBUTTONUP) {
      if (inside) cb_close(cw, c, true);
      else {
        /* released over the combo box itself (the click that dropped us): stay */
        RECT cr;
        GetWindowRect(cw->hwnd, &cr);
        POINT sp = g_mouse;
        if (!PtInRect(&cr, sp)) { /* keep open: Windows waits for a click */ }
        c->btnPressed = false;
        InvalidateRect(cw->hwnd, nullptr, TRUE);
      }
    } else if (m == WM_LBUTTONDOWN && !inside) {
      /* a click outside the list closes it */
      RECT sb;
      GetWindowRect(h, &sb);
      cb_close(cw, c, false);
    }
    return 0;
  }
  case WM_CAPTURECHANGED: {
    Wnd *cw = wnd(s ? s->combo : 0);
    ComboState *c = cs_of(cw);
    if (cw && c && c->dropped && c->list == h) {
      /* lost capture: e.g. the scroll bar took it; take it back later */
    }
    return 0;
  }
  case WM_KEYDOWN: {
    Wnd *cw = wnd(s ? s->combo : 0);
    ComboState *c = cs_of(cw);
    if (!cw || !c) return 0;
    if (wp == VK_ESCAPE) { cb_close(cw, c, false); return 0; }
    if (wp == VK_RETURN || wp == VK_F4) { cb_close(cw, c, true); return 0; }
    int n = (int)s->items.size();
    if (!n) return 0;
    int i = s->hot < 0 ? 0 : s->hot;
    if (wp == VK_UP) i--;
    else if (wp == VK_DOWN) i++;
    else if (wp == VK_HOME) i = 0;
    else if (wp == VK_END) i = n - 1;
    else return 0;
    i = std::max(0, std::min(n - 1, i));
    s->hot = i;
    lb_ensure_visible(w, s, i);
    InvalidateRect(h, nullptr, TRUE);
    cb_set(cw, c, i, true);
    return 0;
  }
  default:
    return listbox_proc(h, m, wp, lp);
  }
}

LRESULT CALLBACK combobox_proc(HWND h, UINT m, WPARAM wp, LPARAM lp)
{
  Wnd *w = wnd(h);
  if (!w) return 0;
  ComboState *c = cs_of(w);
  switch (m) {
  case WM_NCCREATE: {
    ComboState *nc = new ComboState();
    CREATESTRUCT *cs = (CREATESTRUCT *)lp;
    nc->dropH = cs ? cs->cy : 100;
    /* as in Windows, WS_VSCROLL/WS_HSCROLL belong to the drop-down list,
       not to the combo box window itself */
    {
      std::lock_guard<std::recursive_mutex> lk(g_wm);
      w->style &= ~(WS_VSCROLL | WS_HSCROLL);
    }
    w->ctl = nc;
    w->ctl_free = [](void *p) { delete (ComboState *)p; };
    return DefWindowProc(h, m, wp, lp);
  }
  case WM_CREATE:
    c->dropH = std::max(0, c->dropH - (cb_field_h(w, c) + 6));
    cb_resize(w, c);
    return 0;
  case WM_SETFONT:
    w->font = (HFONT)wp;
    cb_resize(w, c);
    if (LOWORD(lp)) InvalidateRect(h, nullptr, TRUE);
    return 0;
  case WM_GETFONT:
    return (LRESULT)w->font;
  case WM_PAINT:
    cb_paint(w);
    return 0;
  case WM_ERASEBKGND:
    return 1;
  case WM_ENABLE:
    InvalidateRect(h, nullptr, TRUE);
    return 0;
  case WM_GETDLGCODE:
    return DLGC_WANTARROWS | DLGC_WANTCHARS;
  case WM_SETFOCUS:
    InvalidateRect(h, nullptr, TRUE);
    notify(w, CBN_SETFOCUS);
    return 0;
  case WM_KILLFOCUS:
    if (c->dropped) cb_close(w, c, false);
    if (!wnd(h)) return 0;
    InvalidateRect(h, nullptr, TRUE);
    notify(w, CBN_KILLFOCUS);
    return 0;
  case WM_LBUTTONDOWN: case WM_LBUTTONDBLCLK:
    if (GetFocus() != h) SetFocus(h);
    if (!wnd(h)) return 0;
    if (c->dropped) cb_close(w, c, false);
    else { c->btnPressed = true; cb_drop(w, c); }
    return 0;
  case WM_LBUTTONUP:
    c->btnPressed = false;
    InvalidateRect(h, nullptr, TRUE);
    return 0;
  case WM_KEYDOWN: case WM_SYSKEYDOWN: {
    if (c->dropped) return send_msg(c->list, WM_KEYDOWN, wp, lp);
    if (wp == VK_F4 || ((wp == VK_DOWN || wp == VK_UP) && (GetKeyState(VK_MENU) & 0x8000))) {
      cb_drop(w, c);
      return 0;
    }
    if (m == WM_SYSKEYDOWN) return DefWindowProc(h, m, wp, lp);
    int n = (int)c->items.size();
    if (!n) return 0;
    int i = c->cur < 0 ? 0 : c->cur;
    switch (wp) {
    case VK_UP: case VK_LEFT: i--; break;
    case VK_DOWN: case VK_RIGHT: i++; break;
    case VK_HOME: case VK_PRIOR: i = 0; break;
    case VK_END: case VK_NEXT: i = n - 1; break;
    default: return 0;
    }
    cb_set(w, c, std::max(0, std::min(n - 1, i)), true);
    return 0;
  }
  case WM_CHAR: {
    if ((unsigned char)wp < 32) return 0;
    char str[2] = {(char)wp, 0};
    ListState tmp;
    tmp.items = c->items;
    int i = lb_find(&tmp, c->cur, str, false);
    if (i != LB_ERR) cb_set(w, c, i, true);
    return 0;
  }
  case WM_GETTEXT: {
    if (!lp || !wp) return 0;
    std::string t = (c->cur >= 0 && c->cur < (int)c->items.size()) ? c->items[c->cur].text : "";
    size_t n = std::min(t.size(), (size_t)wp - 1);
    memcpy((char *)lp, t.data(), n);
    ((char *)lp)[n] = 0;
    return (LRESULT)n;
  }
  case WM_GETTEXTLENGTH:
    return (c->cur >= 0 && c->cur < (int)c->items.size()) ? (LRESULT)c->items[c->cur].text.size() : 0;
  case CB_ADDSTRING: case CB_INSERTSTRING: {
    LbItem it;
    it.text = lp ? (const char *)lp : "";
    int idx;
    if (m == CB_ADDSTRING && (w->style & CBS_SORT)) {
      idx = 0;
      while (idx < (int)c->items.size() && strcasecmp(c->items[idx].text.c_str(), it.text.c_str()) <= 0) idx++;
    } else if (m == CB_ADDSTRING || (int)(INT_PTR)wp < 0 || (int)wp > (int)c->items.size()) {
      idx = (int)c->items.size();
    } else idx = (int)wp;
    c->items.insert(c->items.begin() + idx, it);
    if (m == CB_INSERTSTRING && c->cur >= idx) c->cur++;
    return idx;
  }
  case CB_DELETESTRING: {
    int i = (int)wp;
    if (i < 0 || i >= (int)c->items.size()) return CB_ERR;
    c->items.erase(c->items.begin() + i);
    if (c->cur == i) c->cur = -1;
    else if (c->cur > i) c->cur--;
    InvalidateRect(h, nullptr, TRUE);
    return (LRESULT)c->items.size();
  }
  case CB_RESETCONTENT:
    c->items.clear();
    c->cur = -1;
    InvalidateRect(h, nullptr, TRUE);
    return CB_OKAY;
  case CB_GETCOUNT:
    return (LRESULT)c->items.size();
  case CB_GETCURSEL:
    return c->cur;
  case CB_SETCURSEL: {
    int i = (int)(INT_PTR)wp;
    if (i < 0 || i >= (int)c->items.size()) { c->cur = -1; InvalidateRect(h, nullptr, TRUE); return CB_ERR; }
    c->cur = i;
    InvalidateRect(h, nullptr, TRUE);
    return i;
  }
  case CB_GETLBTEXT: {
    int i = (int)wp;
    if (i < 0 || i >= (int)c->items.size() || !lp) return CB_ERR;
    strcpy((char *)lp, c->items[i].text.c_str());
    return (LRESULT)c->items[i].text.size();
  }
  case CB_GETLBTEXTLEN: {
    int i = (int)wp;
    if (i < 0 || i >= (int)c->items.size()) return CB_ERR;
    return (LRESULT)c->items[i].text.size();
  }
  case CB_FINDSTRING: case CB_FINDSTRINGEXACT: case CB_SELECTSTRING: {
    ListState tmp;
    tmp.items = c->items;
    int i = lb_find(&tmp, (int)(INT_PTR)wp, (const char *)lp, m == CB_FINDSTRINGEXACT);
    if (m == CB_SELECTSTRING && i != CB_ERR) { c->cur = i; InvalidateRect(h, nullptr, TRUE); }
    return i;
  }
  case CB_GETITEMDATA: {
    int i = (int)wp;
    if (i < 0 || i >= (int)c->items.size()) return CB_ERR;
    return c->items[i].data;
  }
  case CB_SETITEMDATA: {
    int i = (int)wp;
    if (i < 0 || i >= (int)c->items.size()) return CB_ERR;
    c->items[i].data = lp;
    return CB_OKAY;
  }
  case CB_SHOWDROPDOWN:
    if (wp) cb_drop(w, c);
    else cb_close(w, c, false);
    return TRUE;
  case CB_GETDROPPEDSTATE:
    return c->dropped;
  case CB_GETITEMHEIGHT:
    return (int)(INT_PTR)wp == -1 ? cb_field_h(w, c) : cb_item_h(w, c);
  case CB_SETITEMHEIGHT:
    if ((int)(INT_PTR)wp == -1) { c->selH = (int)LOWORD(lp); cb_resize(w, c); }
    else c->itemH = (int)LOWORD(lp);
    return 0;
  case CB_GETDROPPEDCONTROLRECT: {
    RECT *r = (RECT *)lp;
    if (!r) return 0;
    GetWindowRect(h, r);
    r->bottom += c->dropH;
    return TRUE;
  }
  default:
    return DefWindowProc(h, m, wp, lp);
  }
}

/* =============================== SCROLLBAR =============================== */
LRESULT CALLBACK scrollbar_proc(HWND h, UINT m, WPARAM wp, LPARAM lp)
{
  Wnd *w = wnd(h);
  if (!w) return 0;
  bool vert = (w->style & SBS_VERT) != 0;
  switch (m) {
  case WM_PAINT: {
    Paint p(h);
    if (!p.ok) return 0;
    Target t = p.t;
    RECT r = {t.ox, t.oy, t.ox + p.W, t.oy + p.Hh};
    int pressed = w->nchit_pressed >= 200 ? w->nchit_pressed - 200 : 0;
    sb_draw(t, r, vert, w->sbH, w->enabled(), pressed);
    return 0;
  }
  case WM_ERASEBKGND:
    return 1;
  case WM_ENABLE:
    InvalidateRect(h, nullptr, TRUE);
    return 0;
  case WM_GETDLGCODE:
    return DLGC_WANTARROWS;
  case WM_LBUTTONDOWN: case WM_LBUTTONDBLCLK: {
    if ((w->style & WS_TABSTOP) && GetFocus() != h) SetFocus(h);
    POINT pt;
    pt.x = GET_X_LPARAM(lp);
    pt.y = GET_Y_LPARAM(lp);
    ClientToScreen(h, &pt);
    int part = sb_hit(h, true, vert, pt.x, pt.y);
    sb_track(h, true, vert, part);
    return 0;
  }
  case WM_KEYDOWN: {
    HWND parent = w->parent ? w->parent->hwnd : 0;
    UINT msg = vert ? WM_VSCROLL : WM_HSCROLL;
    WORD code;
    switch (wp) {
    case VK_UP: case VK_LEFT: code = SB_LINEUP; break;
    case VK_DOWN: case VK_RIGHT: code = SB_LINEDOWN; break;
    case VK_PRIOR: code = SB_PAGEUP; break;
    case VK_NEXT: code = SB_PAGEDOWN; break;
    case VK_HOME: code = SB_TOP; break;
    case VK_END: code = SB_BOTTOM; break;
    default: return 0;
    }
    if (parent) {
      send_msg(parent, msg, MAKEWPARAM(code, 0), (LPARAM)h);
      send_msg(parent, msg, MAKEWPARAM(SB_ENDSCROLL, 0), (LPARAM)h);
    }
    return 0;
  }
  case WM_SETFOCUS: case WM_KILLFOCUS:
    InvalidateRect(h, nullptr, TRUE);
    return 0;
  case WM_SETFONT:
    w->font = (HFONT)wp;
    return 0;
  case SBM_SETPOS: return SetScrollPos(h, SB_CTL, (int)wp, (BOOL)lp);
  case SBM_GETPOS: return GetScrollPos(h, SB_CTL);
  case SBM_SETRANGE: return SetScrollRange(h, SB_CTL, (int)wp, (int)lp, FALSE);
  case SBM_SETRANGEREDRAW: return SetScrollRange(h, SB_CTL, (int)wp, (int)lp, TRUE);
  case SBM_GETRANGE: GetScrollRange(h, SB_CTL, (LPINT)wp, (LPINT)lp); return 0;
  case SBM_SETSCROLLINFO: return SetScrollInfo(h, SB_CTL, (LPCSCROLLINFO)lp, (BOOL)wp);
  case SBM_GETSCROLLINFO: return GetScrollInfo(h, SB_CTL, (LPSCROLLINFO)lp);
  default:
    return DefWindowProc(h, m, wp, lp);
  }
}

void controls_register() {}

}  /* namespace w32 */
