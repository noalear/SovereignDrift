/* ************************************************************************* */
/* w32_nc.cpp -- non-client area (frames, captions, menu bars, standard      */
/* scroll bars), DefWindowProc, modal move/size loops, scroll bar tracking   */
/* and the 3-D drawing helpers, in the Windows 2000 "Classic" style.         */
/*                                                                           */
/* NEW FILE, added 2026-10-12 for the Stellar Frontier Godot port.           */
/* ************************************************************************* */
#include "w32_internal.h"
#include <cstring>
#include <chrono>
#include <thread>

using namespace w32;

#define WM_SF_CLICKACTIVATE 0x0399   /* private: activate a window clicked by the user */

namespace w32 {

/* ============================ drawing helpers ============================ */
void draw_3d_rect(Target &t, int x0, int y0, int x1, int y1, uint32_t tl, uint32_t br)
{
  hline(t, x0, x1 - 1, y0, tl);
  fill_rect(t, x0, y0, x0 + 1, y1 - 1, tl);
  hline(t, x0, x1, y1 - 1, br);
  fill_rect(t, x1 - 1, y0, x1, y1 - 1, br);
}

void draw_edge(Target &t, RECT r, UINT edge, UINT flags)
{
  uint32_t lt1 = 0, rb1 = 0, lt2 = 0, rb2 = 0;
  bool outer = false, inner = false;
  if (edge & BDR_RAISEDOUTER) { lt1 = syscolor_px(COLOR_3DLIGHT); rb1 = syscolor_px(COLOR_3DDKSHADOW); outer = true; }
  if (edge & BDR_SUNKENOUTER) { lt1 = syscolor_px(COLOR_BTNSHADOW); rb1 = syscolor_px(COLOR_BTNHIGHLIGHT); outer = true; }
  if (edge & BDR_RAISEDINNER) { lt2 = syscolor_px(COLOR_BTNHIGHLIGHT); rb2 = syscolor_px(COLOR_BTNSHADOW); inner = true; }
  if (edge & BDR_SUNKENINNER) { lt2 = syscolor_px(COLOR_3DDKSHADOW); rb2 = syscolor_px(COLOR_3DLIGHT); inner = true; }
  if (flags & BF_MONO) { lt1 = rb1 = lt2 = rb2 = 0; }
  if (flags & BF_FLAT) { lt1 = rb1 = syscolor_px(COLOR_BTNSHADOW); lt2 = rb2 = syscolor_px(COLOR_BTNFACE); }
  int x0 = r.left, y0 = r.top, x1 = r.right, y1 = r.bottom;
  if (outer) {
    if (flags & BF_TOP) hline(t, x0, x1, y0, lt1);
    if (flags & BF_LEFT) fill_rect(t, x0, y0, x0 + 1, y1, lt1);
    if (flags & BF_BOTTOM) hline(t, x0, x1, y1 - 1, rb1);
    if (flags & BF_RIGHT) fill_rect(t, x1 - 1, y0, x1, y1, rb1);
    if ((flags & BF_BOTTOM) && (flags & BF_LEFT)) plot(t, x0, y1 - 1, rb1);
    if ((flags & BF_TOP) && (flags & BF_RIGHT)) plot(t, x1 - 1, y0, rb1);
    if (flags & BF_LEFT) x0++;
    if (flags & BF_TOP) y0++;
    if (flags & BF_RIGHT) x1--;
    if (flags & BF_BOTTOM) y1--;
  }
  if (inner) {
    if (flags & BF_TOP) hline(t, x0, x1, y0, lt2);
    if (flags & BF_LEFT) fill_rect(t, x0, y0, x0 + 1, y1, lt2);
    if (flags & BF_BOTTOM) hline(t, x0, x1, y1 - 1, rb2);
    if (flags & BF_RIGHT) fill_rect(t, x1 - 1, y0, x1, y1, rb2);
    if ((flags & BF_BOTTOM) && (flags & BF_LEFT)) plot(t, x0, y1 - 1, rb2);
    if ((flags & BF_TOP) && (flags & BF_RIGHT)) plot(t, x1 - 1, y0, rb2);
    if (flags & BF_LEFT) x0++;
    if (flags & BF_TOP) y0++;
    if (flags & BF_RIGHT) x1--;
    if (flags & BF_BOTTOM) y1--;
  }
  if (flags & BF_MIDDLE) fill_rect(t, x0, y0, x1, y1, syscolor_px(COLOR_BTNFACE));
}

/* dir: 0 up, 1 down, 2 left, 3 right */
void draw_arrow(Target &t, int cx, int cy, int dir, uint32_t c, int size)
{
  /* a classic 7x4 triangle (size 4 rows) */
  for (int i = 0; i < size; i++) {
    int half = size - 1 - i;
    switch (dir) {
    /* dir: 0 up, 1 down, 2 left, 3 right; row/column i = 0 is the widest */
    case 0: hline(t, cx - half, cx + half + 1, cy + size / 2 - i, c); break;
    case 1: hline(t, cx - half, cx + half + 1, cy - size / 2 + i, c); break;
    case 2: fill_rect(t, cx + size / 2 - i, cy - half, cx + size / 2 - i + 1, cy + half + 1, c); break;
    case 3: fill_rect(t, cx - size / 2 + i, cy - half, cx - size / 2 + i + 1, cy + half + 1, c); break;
    }
  }
}

void draw_check(Target &t, int x, int y, uint32_t c)
{
  static const char *g[7] = {
    "......#", ".....##", "#...###", "##.###.", "#####..", ".###...", "..#...." };
  for (int r = 0; r < 7; r++)
    for (int k = 0; k < 7; k++)
      if (g[r][k] == '#') plot(t, x + k, y + r, c);
}

/* ============================ frame metrics ============================== */
int nc_border(DWORD style, DWORD ex)
{
  if (style & WS_THICKFRAME) return M_CXFRAME;
  if ((style & WS_DLGFRAME) || (ex & WS_EX_DLGMODALFRAME)) return M_CXDLGFRAME;
  if (style & WS_BORDER) return M_CXBORDER;
  return 0;
}

static int caption_h(DWORD style, DWORD ex)
{
  if ((style & WS_CAPTION) != WS_CAPTION) return 0;
  return (ex & WS_EX_TOOLWINDOW) ? 16 : M_CYCAPTION;
}

static int client_edge(DWORD ex)
{
  if (ex & WS_EX_CLIENTEDGE) return 2;
  if (ex & WS_EX_STATICEDGE) return 1;
  return 0;
}

void nc_calc(Wnd *w, DWORD style, DWORD ex, bool hasMenu, int cx, int cy, RECT *c)
{
  int b = nc_border(style, ex);
  int top = b + caption_h(style, ex);
  if (hasMenu && w) top += menubar_height(w, cx - 2 * b);
  int e = client_edge(ex);
  c->left = b + e;
  c->top = top + e;
  c->right = cx - b - e;
  c->bottom = cy - b - e;
  if (style & WS_VSCROLL) c->right -= M_CXVSCROLL;
  if (style & WS_HSCROLL) c->bottom -= M_CYHSCROLL;
  if (c->right < c->left) c->right = c->left;
  if (c->bottom < c->top) c->bottom = c->top;
}

/* caption button rectangles in window coordinates */
static bool caption_buttons(Wnd *w, RECT *close, RECT *maxb, RECT *minb)
{
  if (caption_h(w->style, w->exstyle) == 0 || !(w->style & WS_SYSMENU)) return false;
  int W = w->rc.right - w->rc.left;
  int b = nc_border(w->style, w->exstyle);
  int bw = 16, bh = 14, y = b + 2;
  if (w->exstyle & WS_EX_TOOLWINDOW) { bw = 12; bh = 10; }
  *close = RECT{W - b - 2 - bw, y, W - b - 2, y + bh};
  if (w->style & (WS_MINIMIZEBOX | WS_MAXIMIZEBOX)) {
    *maxb = RECT{close->left - 2 - bw, y, close->left - 2, y + bh};
    *minb = RECT{maxb->left - bw, y, maxb->left, y + bh};
  } else {
    *maxb = *minb = RECT{0, 0, 0, 0};
  }
  return true;
}

/* ============================== scroll bars ============================== */
struct SbGeom {
  RECT r;          /* whole bar */
  bool vert;
  int arrow;       /* arrow button length */
  int t0, t1;      /* track extent (along the bar) */
  int thumb0, thumb1;
  bool hasThumb;
};

static SbGeom sb_geom(RECT r, bool vert, const ScrollData &sd, bool enabled)
{
  SbGeom g;
  g.r = r;
  g.vert = vert;
  int len = vert ? r.bottom - r.top : r.right - r.left;
  int thick = vert ? r.right - r.left : r.bottom - r.top;
  g.arrow = std::min(thick, len / 2);
  int start = vert ? r.top : r.left;
  g.t0 = start + g.arrow;
  g.t1 = start + len - g.arrow;
  int track = g.t1 - g.t0;
  int range = sd.max - sd.min;
  g.hasThumb = enabled && sd.enabled && range > 0 && track > 0;
  if (sd.page && (UINT)sd.page > (UINT)range) g.hasThumb = false;
  int thumb = thick;
  if (sd.page > 0) thumb = std::max(8, track * sd.page / (range + 1));
  if (thumb > track) g.hasThumb = false;
  if (g.hasThumb) {
    int posRange = range - std::max(sd.page - 1, 0);
    int pos = sd.pos - sd.min;
    if (pos < 0) pos = 0;
    if (pos > posRange) pos = posRange;
    int usable = track - thumb;
    int off = posRange > 0 ? (int)((int64_t)usable * pos / posRange) : 0;
    g.thumb0 = g.t0 + off;
    g.thumb1 = g.thumb0 + thumb;
  } else {
    g.thumb0 = g.thumb1 = 0;
  }
  return g;
}

/* parts: 1 arrow-up/left, 2 page-up, 3 thumb, 4 page-down, 5 arrow-down */
static int sb_part(const SbGeom &g, int x, int y)
{
  int v = g.vert ? y : x;
  int start = g.vert ? g.r.top : g.r.left;
  int end = g.vert ? g.r.bottom : g.r.right;
  if (x < g.r.left || x >= g.r.right || y < g.r.top || y >= g.r.bottom) return 0;
  if (v < start + g.arrow) return 1;
  if (v >= end - g.arrow) return 5;
  if (!g.hasThumb) return 0;
  if (v < g.thumb0) return 2;
  if (v >= g.thumb1) return 4;
  return 3;
}

static void draw_button_face(Target &t, RECT r, bool pressed)
{
  fill_rect(t, r.left, r.top, r.right, r.bottom, syscolor_px(COLOR_BTNFACE));
  if (pressed) {
    draw_3d_rect(t, r.left, r.top, r.right, r.bottom, syscolor_px(COLOR_BTNSHADOW), syscolor_px(COLOR_BTNSHADOW));
  } else {
    draw_edge(t, r, EDGE_RAISED, BF_RECT);
  }
}

void sb_draw(Target &t, RECT r, bool vert, const ScrollData &sd, bool enabled, int pressedPart)
{
  SbGeom g = sb_geom(r, vert, sd, enabled);
  bool active = enabled && sd.enabled && sd.max > sd.min;
  /* arrows */
  RECT a1, a2;
  if (vert) {
    a1 = RECT{r.left, r.top, r.right, r.top + g.arrow};
    a2 = RECT{r.left, r.bottom - g.arrow, r.right, r.bottom};
  } else {
    a1 = RECT{r.left, r.top, r.left + g.arrow, r.bottom};
    a2 = RECT{r.right - g.arrow, r.top, r.right, r.bottom};
  }
  for (int k = 0; k < 2; k++) {
    RECT a = k ? a2 : a1;
    bool p = pressedPart == (k ? 5 : 1);
    draw_button_face(t, a, p);
    int cx = (a.left + a.right) / 2 + (p ? 1 : 0), cy = (a.top + a.bottom) / 2 + (p ? 1 : 0);
    int dir = vert ? (k ? 1 : 0) : (k ? 3 : 2);
    if (active) draw_arrow(t, cx - (vert ? 0 : 0), cy, dir, syscolor_px(COLOR_BTNTEXT), 4);
    else {
      draw_arrow(t, cx + 1, cy + 1, dir, syscolor_px(COLOR_BTNHIGHLIGHT), 4);
      draw_arrow(t, cx, cy, dir, syscolor_px(COLOR_BTNSHADOW), 4);
    }
  }
  /* track: 50% dither of face and highlight */
  uint32_t c1 = syscolor_px(COLOR_BTNHIGHLIGHT), c2 = syscolor_px(COLOR_BTNFACE);
  int ts = g.t0, te = g.t1;
  for (int v = ts; v < te; v++) {
    for (int u = (vert ? r.left : r.top); u < (vert ? r.right : r.bottom); u++) {
      int x = vert ? u : v, y = vert ? v : u;
      bool pressedPage = (pressedPart == 2 && v < g.thumb0) || (pressedPart == 4 && v >= g.thumb1);
      uint32_t c = ((x + y) & 1) ? c1 : c2;
      if (pressedPage) c = ((x + y) & 1) ? 0 : syscolor_px(COLOR_3DDKSHADOW);
      plot(t, x, y, c);
    }
  }
  if (g.hasThumb) {
    RECT th = vert ? RECT{r.left, g.thumb0, r.right, g.thumb1} : RECT{g.thumb0, r.top, g.thumb1, r.bottom};
    draw_button_face(t, th, false);
  }
}

/* ============================ non-client paint =========================== */
static void draw_caption_button(Target &t, RECT r, int kind, bool pressed, bool enabled)
{
  draw_button_face(t, r, false);
  if (pressed) {
    draw_edge(t, r, EDGE_SUNKEN, BF_RECT);
    fill_rect(t, r.left + 2, r.top + 2, r.right - 2, r.bottom - 2, syscolor_px(COLOR_BTNFACE));
  }
  int ox = r.left + (pressed ? 1 : 0), oy = r.top + (pressed ? 1 : 0);
  uint32_t c = enabled ? 0 : syscolor_px(COLOR_BTNSHADOW);
  int bw = r.right - r.left;
  if (kind == 0) {   /* close: X */
    static const char *x8[7] = { "##....##", ".##..##.", "..####..", "...##...", "..####..", ".##..##.", "##....##" };
    int gx = ox + (bw - 8) / 2, gy = oy + 3;
    if (!enabled)
      for (int k = 0; k < 7; k++) for (int j = 0; j < 8; j++)
        if (x8[k][j] == '#') plot(t, gx + j + 1, gy + k + 1, syscolor_px(COLOR_BTNHIGHLIGHT));
    for (int k = 0; k < 7; k++) for (int j = 0; j < 8; j++)
      if (x8[k][j] == '#') plot(t, gx + j, gy + k, c);
  } else if (kind == 1) {   /* maximize */
    int gx = ox + (bw - 9) / 2, gy = oy + 2;
    fill_rect(t, gx, gy, gx + 9, gy + 2, c);
    fill_rect(t, gx, gy, gx + 1, gy + 9, c);
    fill_rect(t, gx + 8, gy, gx + 9, gy + 9, c);
    fill_rect(t, gx, gy + 8, gx + 9, gy + 9, c);
  } else if (kind == 3) {   /* restore */
    int gx = ox + (bw - 9) / 2, gy = oy + 2;
    fill_rect(t, gx + 3, gy, gx + 9, gy + 2, c);
    fill_rect(t, gx + 8, gy, gx + 9, gy + 6, c);
    fill_rect(t, gx + 3, gy, gx + 4, gy + 3, c);
    fill_rect(t, gx, gy + 3, gx + 6, gy + 5, c);
    fill_rect(t, gx, gy + 3, gx + 1, gy + 9, c);
    fill_rect(t, gx + 5, gy + 3, gx + 6, gy + 9, c);
    fill_rect(t, gx, gy + 8, gx + 6, gy + 9, c);
  } else {   /* minimize */
    int gx = ox + (bw - 6) / 2, gy = oy + 9;
    fill_rect(t, gx, gy, gx + 6, gy + 2, c);
  }
}

struct IconObjPub;   /* defined in w32_misc.cpp */
void icon_draw_small(Target &t, HICON ic, int x, int y);

void nc_paint(Wnd *w)
{
  if (!w->surf) return;
  Target t;
  t.hold = w->surf;
  t.s = w->surf.get();
  int W = t.s->w, Hh = t.s->h;
  if (W <= 0 || Hh <= 0) return;
  t.clips.push_back(RECT{0, 0, W, Hh});
  /* exclude the client area */
  RECT c = w->client;
  {
    std::vector<RECT> clips;
    RECT a = {0, 0, W, Hh};
    if (c.top > a.top) clips.push_back(RECT{0, 0, W, c.top});
    if (c.bottom < Hh) clips.push_back(RECT{0, c.bottom, W, Hh});
    if (c.left > 0) clips.push_back(RECT{0, c.top, c.left, c.bottom});
    if (c.right < W) clips.push_back(RECT{c.right, c.top, W, c.bottom});
    t.clips.swap(clips);
  }
  if (t.clips.empty()) return;
  int b = nc_border(w->style, w->exstyle);
  RECT full = {0, 0, W, Hh};
  if (w->style & WS_THICKFRAME) {
    draw_edge(t, full, EDGE_RAISED, BF_RECT);
    uint32_t fc = syscolor_px(w->active ? COLOR_ACTIVEBORDER : COLOR_INACTIVEBORDER);
    draw_3d_rect(t, 2, 2, W - 2, Hh - 2, fc, fc);
    draw_3d_rect(t, 3, 3, W - 3, Hh - 3, syscolor_px(COLOR_BTNFACE), syscolor_px(COLOR_BTNFACE));
  } else if (b == M_CXDLGFRAME) {
    draw_edge(t, full, EDGE_RAISED, BF_RECT);
    draw_3d_rect(t, 2, 2, W - 2, Hh - 2, syscolor_px(COLOR_BTNFACE), syscolor_px(COLOR_BTNFACE));
  } else if (b == 1) {
    draw_3d_rect(t, 0, 0, W, Hh, syscolor_px(COLOR_WINDOWFRAME), syscolor_px(COLOR_WINDOWFRAME));
  }
  int y = b;
  int ch = caption_h(w->style, w->exstyle);
  if (ch) {
    int cw = W - 2 * b;
    uint32_t c0 = syscolor_px(w->active ? COLOR_ACTIVECAPTION : COLOR_INACTIVECAPTION);
    uint32_t c1 = syscolor_px(w->active ? COLOR_GRADIENTACTIVECAPTION : COLOR_GRADIENTINACTIVECAPTION);
    for (int x = 0; x < cw; x++) {
      int k = cw > 1 ? x * 255 / (cw - 1) : 0;
      uint32_t r = (((c0 >> 16) & 255) * (255 - k) + ((c1 >> 16) & 255) * k) / 255;
      uint32_t g = (((c0 >> 8) & 255) * (255 - k) + ((c1 >> 8) & 255) * k) / 255;
      uint32_t bl = ((c0 & 255) * (255 - k) + (c1 & 255) * k) / 255;
      fill_rect(t, b + x, y, b + x + 1, y + ch - 1, (r << 16) | (g << 8) | bl);
    }
    hline(t, b, W - b, y + ch - 1, syscolor_px(COLOR_BTNFACE));
    int tx = b + 2;
    HICON icon = w->icon ? w->icon : w->cls->icon;
    if ((w->style & WS_SYSMENU) && icon && !(w->exstyle & WS_EX_TOOLWINDOW)) {
      icon_draw_small(t, icon, b + 2, y + 1);
      tx = b + 20;
    }
    RECT cl, mx, mn;
    int textRight = W - b - 2;
    if (caption_buttons(w, &cl, &mx, &mn)) {
      textRight = (mn.right ? mn.left : cl.left) - 2;
      draw_caption_button(t, cl, 0, w->nchit_pressed == HTCLOSE, true);
      if (mx.right) {
        draw_caption_button(t, mx, w->maximized ? 3 : 1, w->nchit_pressed == HTMAXBUTTON,
                            (w->style & WS_MAXIMIZEBOX) != 0);
        draw_caption_button(t, mn, 2, w->nchit_pressed == HTMINBUTTON,
                            (w->style & WS_MINIMIZEBOX) != 0);
      }
    }
    const SfFontData *fd = font_default_gui_bold();
    /* captions use the 8 pt bold UI font */
    LOGFONT lf;
    memset(&lf, 0, sizeof(lf));
    lf.lfHeight = 13;
    lf.lfWeight = FW_BOLD;
    strcpy(lf.lfFaceName, "Tahoma");
    fd = font_match(lf);
    Target tt = t;
    std::vector<RECT> cc;
    for (auto &r : t.clips) {
      RECT i, a = {tx, y, textRight, y + ch - 1};
      if (IntersectRect(&i, &r, &a)) cc.push_back(i);
    }
    tt.clips = cc;
    if (!tt.clips.empty())
      text_draw(tt, fd, tx, y + (ch - 1 - fd->height) / 2, w->text.c_str(), (int)w->text.size(),
                syscolor_px(w->active ? COLOR_CAPTIONTEXT : COLOR_INACTIVECAPTIONTEXT));
    y += ch;
  }
  if (w->menu && !w->is_child()) {
    int mh = menubar_height(w, W - 2 * b);
    menubar_paint(w, t, b, y, W - 2 * b);
    y += mh;
  }
  int e = client_edge(w->exstyle);
  if (e == 2) {
    RECT r = {c.left - 2, c.top - 2, c.right + 2 + ((w->style & WS_VSCROLL) ? M_CXVSCROLL : 0),
              c.bottom + 2 + ((w->style & WS_HSCROLL) ? M_CYHSCROLL : 0)};
    draw_edge(t, r, EDGE_SUNKEN, BF_RECT);
  } else if (e == 1) {
    RECT r = {c.left - 1, c.top - 1, c.right + 1 + ((w->style & WS_VSCROLL) ? M_CXVSCROLL : 0),
              c.bottom + 1 + ((w->style & WS_HSCROLL) ? M_CYHSCROLL : 0)};
    draw_edge(t, r, BDR_SUNKENOUTER, BF_RECT);
  }
  bool en = w->enabled();
  if (w->style & WS_VSCROLL)
    sb_draw(t, RECT{c.right, c.top, c.right + M_CXVSCROLL, c.bottom}, true, w->sbV, en,
            w->nchit_pressed >= 100 && w->nchit_pressed < 110 ? w->nchit_pressed - 100 : 0);
  if (w->style & WS_HSCROLL)
    sb_draw(t, RECT{c.left, c.bottom, c.right, c.bottom + M_CYHSCROLL}, false, w->sbH, en,
            w->nchit_pressed >= 110 ? w->nchit_pressed - 110 : 0);
  if ((w->style & WS_VSCROLL) && (w->style & WS_HSCROLL))
    fill_rect(t, c.right, c.bottom, c.right + M_CXVSCROLL, c.bottom + M_CYHSCROLL, syscolor_px(COLOR_BTNFACE));
  screen_dirty();
}

/* ============================== hit testing ============================== */
int nc_hittest(Wnd *w, int sx, int sy)
{
  int ox, oy;
  wnd_screen_origin(w, &ox, &oy);
  int x = sx - ox, y = sy - oy;
  int W = w->rc.right - w->rc.left, Hh = w->rc.bottom - w->rc.top;
  if (x < 0 || y < 0 || x >= W || y >= Hh) return HTNOWHERE;
  RECT c = w->client;
  if (x >= c.left && x < c.right && y >= c.top && y < c.bottom) return HTCLIENT;
  if ((w->style & WS_VSCROLL) && x >= c.right && x < c.right + M_CXVSCROLL && y >= c.top && y < c.bottom)
    return HTVSCROLL;
  if ((w->style & WS_HSCROLL) && y >= c.bottom && y < c.bottom + M_CYHSCROLL && x >= c.left && x < c.right)
    return HTHSCROLL;
  int b = nc_border(w->style, w->exstyle);
  if ((w->style & WS_THICKFRAME) && !w->maximized) {
    const int k = 16;
    bool l = x < b, r = x >= W - b, t = y < b, bt = y >= Hh - b;
    if (l || r || t || bt) {
      if ((t && x < k) || (l && y < k)) return HTTOPLEFT;
      if ((t && x >= W - k) || (r && y < k)) return HTTOPRIGHT;
      if ((bt && x < k) || (l && y >= Hh - k)) return HTBOTTOMLEFT;
      if ((bt && x >= W - k) || (r && y >= Hh - k)) return HTBOTTOMRIGHT;
      if (l) return HTLEFT;
      if (r) return HTRIGHT;
      if (t) return HTTOP;
      return HTBOTTOM;
    }
  } else if (x < b || y < b || x >= W - b || y >= Hh - b) {
    return HTBORDER;
  }
  int ch = caption_h(w->style, w->exstyle);
  if (ch && y >= b && y < b + ch) {
    RECT cl, mx, mn;
    if (caption_buttons(w, &cl, &mx, &mn)) {
      POINT p = {x, y};
      if (PtInRect(&cl, p)) return HTCLOSE;
      if (mx.right && PtInRect(&mx, p)) return HTMAXBUTTON;
      if (mn.right && PtInRect(&mn, p)) return HTMINBUTTON;
      if (x < b + 18) return HTSYSMENU;
    }
    return HTCAPTION;
  }
  if (w->menu && !w->is_child()) {
    int mh = menubar_height(w, W - 2 * b);
    if (y >= b + ch && y < b + ch + mh) return HTMENU;
  }
  return HTBORDER;
}

/* ============================= modal helpers ============================= */
/* Pump this thread's messages while a modal NC operation runs: mouse input
   is handed to fn (which returns false to stop); everything else is
   dispatched normally. */
static void modal_loop(HWND h, std::function<bool(const MSG &)> fn, int pollMs,
                       std::function<bool()> tick = nullptr)
{
  SetCapture(h);
  for (;;) {
    MSG m;
    if (PeekMessage(&m, 0, 0, 0, PM_REMOVE)) {
      if (m.message == WM_QUIT) { PostQuitMessage((int)m.wParam); break; }
      if ((m.message >= WM_MOUSEFIRST && m.message <= WM_MOUSELAST) ||
          (m.message >= WM_KEYFIRST && m.message <= WM_KEYLAST) ||
          m.message == WM_CANCELMODE) {
        if (!fn(m)) break;
        continue;
      }
      if (m.message >= WM_NCMOUSEMOVE && m.message <= WM_NCMBUTTONDBLCLK) {
        MSG mm = m;
        mm.message = m.message - WM_NCMOUSEMOVE + WM_MOUSEMOVE;
        if (!fn(mm)) break;
        continue;
      }
      DispatchMessage(&m);
      continue;
    }
    if (tick && !tick()) break;
    if (GetCapture() != h) break;
    if (!wnd(h)) break;
    std::this_thread::sleep_for(std::chrono::milliseconds(pollMs));
  }
  if (GetCapture() == h) ReleaseCapture();
}

static void move_size_loop(Wnd *w, int hit)
{
  HWND h = w->hwnd;
  POINT start;
  RECT r0;
  {
    std::lock_guard<std::recursive_mutex> lk(g_wm);
    start = g_mouse;
    r0 = w->rc;
  }
  send_msg(h, WM_ENTERSIZEMOVE, 0, 0);
  modal_loop(h, [&](const MSG &m) -> bool {
    if (m.message == WM_MOUSEMOVE || m.message == WM_LBUTTONUP) {
      POINT p;
      { std::lock_guard<std::recursive_mutex> lk(g_wm); p = g_mouse; }
      int dx = p.x - start.x, dy = p.y - start.y;
      RECT r = r0;
      if (hit == HTCAPTION) OffsetRect(&r, dx, dy);
      else {
        if (hit == HTLEFT || hit == HTTOPLEFT || hit == HTBOTTOMLEFT) r.left += dx;
        if (hit == HTRIGHT || hit == HTTOPRIGHT || hit == HTBOTTOMRIGHT) r.right += dx;
        if (hit == HTTOP || hit == HTTOPLEFT || hit == HTTOPRIGHT) r.top += dy;
        if (hit == HTBOTTOM || hit == HTBOTTOMLEFT || hit == HTBOTTOMRIGHT) r.bottom += dy;
        if (r.right - r.left < 112) { if (r.left != r0.left) r.left = r.right - 112; else r.right = r.left + 112; }
        if (r.bottom - r.top < 27) { if (r.top != r0.top) r.top = r.bottom - 27; else r.bottom = r.top + 27; }
      }
      if (wnd(h))
        SetWindowPos(h, 0, r.left, r.top, r.right - r.left, r.bottom - r.top,
                     SWP_NOZORDER | SWP_NOACTIVATE | (hit == HTCAPTION ? SWP_NOSIZE : 0));
      return m.message != WM_LBUTTONUP;
    }
    if (m.message == WM_KEYDOWN && m.wParam == VK_ESCAPE) {
      SetWindowPos(h, 0, r0.left, r0.top, r0.right - r0.left, r0.bottom - r0.top,
                   SWP_NOZORDER | SWP_NOACTIVATE);
      return false;
    }
    return m.message != WM_RBUTTONDOWN && m.message != WM_CANCELMODE;
  }, 5);
  if (wnd(h)) send_msg(h, WM_EXITSIZEMOVE, 0, 0);
}

static void caption_button_loop(Wnd *w, int hit)
{
  HWND h = w->hwnd;
  auto over = [&]() -> bool {
    std::lock_guard<std::recursive_mutex> lk(g_wm);
    return wnd(h) && nc_hittest(w, g_mouse.x, g_mouse.y) == hit;
  };
  auto setp = [&](bool p) {
    std::lock_guard<std::recursive_mutex> lk(g_wm);
    if (!wnd(h)) return;
    int nv = p ? hit : 0;
    if (w->nchit_pressed != nv) { w->nchit_pressed = nv; nc_paint(w); }
  };
  setp(true);
  bool released = false;
  modal_loop(h, [&](const MSG &m) -> bool {
    if (m.message == WM_MOUSEMOVE) { setp(over()); return true; }
    if (m.message == WM_LBUTTONUP) { released = over(); return false; }
    return m.message != WM_CANCELMODE;
  }, 5);
  setp(false);
  if (!released || !wnd(h)) return;
  WPARAM cmd = hit == HTCLOSE ? SC_CLOSE : hit == HTMINBUTTON ? SC_MINIMIZE :
               (w->maximized ? SC_RESTORE : SC_MAXIMIZE);
  send_msg(h, WM_SYSCOMMAND, cmd, MAKELPARAM(g_mouse.x, g_mouse.y));
}

/* ----------------------------- scroll tracking --------------------------- */
/* Shared by SCROLLBAR controls (ctl=true: notify the parent, lParam = bar)
   and standard window scroll bars (notify the window itself, lParam 0). */
void sb_track(HWND h, bool ctl, bool vert, int startPart)
{
  Wnd *w = wnd(h);
  if (!w) return;
  HWND target = ctl ? (w->parent ? w->parent->hwnd : 0) : h;
  UINT msgId = vert ? WM_VSCROLL : WM_HSCROLL;
  LPARAM lpBar = ctl ? (LPARAM)h : 0;
  auto geom = [&]() -> SbGeom {
    std::lock_guard<std::recursive_mutex> lk(g_wm);
    int ox, oy;
    RECT r;
    ScrollData &sd = ctl ? w->sbH : (vert ? w->sbV : w->sbH);
    if (ctl) {
      wnd_client_origin(w, &ox, &oy);
      r = RECT{ox, oy, ox + (w->client.right - w->client.left), oy + (w->client.bottom - w->client.top)};
    } else {
      wnd_screen_origin(w, &ox, &oy);
      RECT c = w->client;
      r = vert ? RECT{ox + c.right, oy + c.top, ox + c.right + M_CXVSCROLL, oy + c.bottom}
               : RECT{ox + c.left, oy + c.bottom, ox + c.right, oy + c.bottom + M_CYHSCROLL};
    }
    return sb_geom(r, vert, sd, w->enabled());
  };
  auto setPressed = [&](int part) {
    std::lock_guard<std::recursive_mutex> lk(g_wm);
    if (!wnd(h)) return;
    int v = part ? (ctl ? 200 + part : (vert ? 100 : 110) + part) : 0;
    if (w->nchit_pressed == v) return;
    w->nchit_pressed = v;
    if (ctl) wnd_invalidate(w, nullptr, false);
    else nc_paint(w);
  };
  auto send = [&](WORD code, int pos) {
    if (target) send_msg(target, msgId, MAKEWPARAM(code, pos), lpBar);
  };
  int part = startPart;
  if (part == 0) return;
  SbGeom g0 = geom();
  if (part == 3) {
    /* thumb drag */
    POINT start;
    { std::lock_guard<std::recursive_mutex> lk(g_wm); start = g_mouse; }
    int off0 = g0.thumb0;
    ScrollData sd0 = ctl ? w->sbH : (vert ? w->sbV : w->sbH);
    int lastPos = sd0.pos;
    setPressed(3);
    modal_loop(h, [&](const MSG &m) -> bool {
      if (m.message == WM_MOUSEMOVE || m.message == WM_LBUTTONUP) {
        POINT p;
        { std::lock_guard<std::recursive_mutex> lk(g_wm); p = g_mouse; }
        int d = vert ? p.y - start.y : p.x - start.x;
        int track = g0.t1 - g0.t0, thumb = g0.thumb1 - g0.thumb0;
        int usable = track - thumb;
        int range = sd0.max - sd0.min - std::max(sd0.page - 1, 0);
        int pos = sd0.min;
        if (usable > 0) pos = sd0.min + (int)((int64_t)(off0 + d - g0.t0) * range / usable);
        if (pos < sd0.min) pos = sd0.min;
        if (pos > sd0.min + range) pos = sd0.min + range;
        {
          std::lock_guard<std::recursive_mutex> lk(g_wm);
          ScrollData &sd = ctl ? w->sbH : (vert ? w->sbV : w->sbH);
          sd.track = pos;
        }
        if (m.message == WM_MOUSEMOVE) {
          if (pos != lastPos) { lastPos = pos; send(SB_THUMBTRACK, pos); }
          return true;
        }
        send(SB_THUMBPOSITION, pos);
        return false;
      }
      return m.message != WM_CANCELMODE;
    }, 5);
    setPressed(0);
    send(SB_ENDSCROLL, 0);
    return;
  }
  WORD code = part == 1 ? SB_LINEUP : part == 5 ? SB_LINEDOWN : part == 2 ? SB_PAGEUP : SB_PAGEDOWN;
  setPressed(part);
  send(code, 0);
  auto next = std::chrono::steady_clock::now() + std::chrono::milliseconds(400);
  bool down = true;
  modal_loop(h, [&](const MSG &m) -> bool {
    if (m.message == WM_LBUTTONUP) { down = false; return false; }
    if (m.message == WM_MOUSEMOVE) {
      POINT p;
      { std::lock_guard<std::recursive_mutex> lk(g_wm); p = g_mouse; }
      SbGeom g = geom();
      setPressed(sb_part(g, p.x, p.y) == part ? part : 0);
    }
    return m.message != WM_CANCELMODE;
  }, 5, [&]() -> bool {
    if (!down) return false;
    auto now = std::chrono::steady_clock::now();
    if (now >= next) {
      next = now + std::chrono::milliseconds(50);
      POINT p;
      { std::lock_guard<std::recursive_mutex> lk(g_wm); p = g_mouse; }
      SbGeom g = geom();
      if (sb_part(g, p.x, p.y) == part) send(code, 0);
    }
    return true;
  });
  setPressed(0);
  send(SB_ENDSCROLL, 0);
}

int sb_hit(HWND h, bool ctl, bool vert, int sx, int sy)
{
  std::lock_guard<std::recursive_mutex> lk(g_wm);
  Wnd *w = wnd(h);
  if (!w) return 0;
  int ox, oy;
  RECT r;
  ScrollData &sd = ctl ? w->sbH : (vert ? w->sbV : w->sbH);
  if (ctl) {
    wnd_client_origin(w, &ox, &oy);
    r = RECT{ox, oy, ox + (w->client.right - w->client.left), oy + (w->client.bottom - w->client.top)};
  } else {
    wnd_screen_origin(w, &ox, &oy);
    RECT c = w->client;
    r = vert ? RECT{ox + c.right, oy + c.top, ox + c.right + M_CXVSCROLL, oy + c.bottom}
             : RECT{ox + c.left, oy + c.bottom, ox + c.right, oy + c.bottom + M_CYHSCROLL};
  }
  return sb_part(sb_geom(r, vert, sd, w->enabled()), sx, sy);
}

}  /* namespace w32 */

/* ========================= scroll bar API (USER) ========================= */
static ScrollData *sb_data(Wnd *w, int bar)
{
  if (!w) return nullptr;
  if (bar == SB_CTL) return &w->sbH;
  if (bar == SB_VERT) return &w->sbV;
  if (bar == SB_HORZ) return &w->sbH;
  return nullptr;
}

static void sb_changed(Wnd *w, int bar, BOOL redraw)
{
  if (!redraw) return;
  if (bar == SB_CTL) wnd_invalidate(w, nullptr, false);
  else nc_paint(w);
}

int SetScrollPos(HWND h, int bar, int pos, BOOL redraw)
{
  std::lock_guard<std::recursive_mutex> lk(g_wm);
  Wnd *w = wnd(h);
  ScrollData *sd = sb_data(w, bar);
  if (!sd) return 0;
  int old = sd->pos;
  int maxPos = sd->max - std::max(sd->page - 1, 0);
  if (pos > maxPos) pos = maxPos;
  if (pos < sd->min) pos = sd->min;
  sd->pos = pos;
  sb_changed(w, bar, redraw);
  return old;
}

int GetScrollPos(HWND h, int bar)
{
  std::lock_guard<std::recursive_mutex> lk(g_wm);
  ScrollData *sd = sb_data(wnd(h), bar);
  return sd ? sd->pos : 0;
}

BOOL SetScrollRange(HWND h, int bar, int mn, int mx, BOOL redraw)
{
  std::lock_guard<std::recursive_mutex> lk(g_wm);
  Wnd *w = wnd(h);
  ScrollData *sd = sb_data(w, bar);
  if (!sd) return FALSE;
  sd->min = mn;
  sd->max = mx;
  if (sd->pos < mn) sd->pos = mn;
  if (sd->pos > mx) sd->pos = mx;
  sb_changed(w, bar, redraw);
  return TRUE;
}

BOOL GetScrollRange(HWND h, int bar, LPINT mn, LPINT mx)
{
  std::lock_guard<std::recursive_mutex> lk(g_wm);
  ScrollData *sd = sb_data(wnd(h), bar);
  if (!sd) return FALSE;
  if (mn) *mn = sd->min;
  if (mx) *mx = sd->max;
  return TRUE;
}

int SetScrollInfo(HWND h, int bar, LPCSCROLLINFO si, BOOL redraw)
{
  std::lock_guard<std::recursive_mutex> lk(g_wm);
  Wnd *w = wnd(h);
  ScrollData *sd = sb_data(w, bar);
  if (!sd || !si) return 0;
  if (si->fMask & SIF_RANGE) { sd->min = si->nMin; sd->max = si->nMax; }
  if (si->fMask & SIF_PAGE) sd->page = (int)si->nPage;
  if (si->fMask & SIF_POS) sd->pos = si->nPos;
  int maxPos = sd->max - std::max(sd->page - 1, 0);
  if (sd->pos > maxPos) sd->pos = maxPos;
  if (sd->pos < sd->min) sd->pos = sd->min;
  sb_changed(w, bar, redraw);
  return sd->pos;
}

BOOL GetScrollInfo(HWND h, int bar, LPSCROLLINFO si)
{
  std::lock_guard<std::recursive_mutex> lk(g_wm);
  ScrollData *sd = sb_data(wnd(h), bar);
  if (!sd || !si) return FALSE;
  if (si->fMask & SIF_RANGE) { si->nMin = sd->min; si->nMax = sd->max; }
  if (si->fMask & SIF_PAGE) si->nPage = sd->page;
  if (si->fMask & SIF_POS) si->nPos = sd->pos;
  if (si->fMask & SIF_TRACKPOS) si->nTrackPos = sd->track;
  return TRUE;
}

BOOL ShowScrollBar(HWND h, int bar, BOOL show)
{
  Wnd *w = wnd(h);
  if (!w) return FALSE;
  DWORD bits = bar == SB_VERT ? WS_VSCROLL : bar == SB_HORZ ? WS_HSCROLL : bar == SB_BOTH ? (WS_VSCROLL | WS_HSCROLL) : 0;
  if (!bits) return FALSE;
  DWORD ns = show ? (w->style | bits) : (w->style & ~bits);
  if (ns == w->style) return TRUE;
  {
    std::lock_guard<std::recursive_mutex> lk(g_wm);
    w->style = ns;
  }
  SetWindowPos(h, 0, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
  return TRUE;
}

BOOL EnableScrollBar(HWND h, UINT bar, UINT arrows)
{
  std::lock_guard<std::recursive_mutex> lk(g_wm);
  Wnd *w = wnd(h);
  ScrollData *sd = sb_data(w, (int)bar);
  if (!sd) return FALSE;
  sd->enabled = arrows != ESB_DISABLE_BOTH;
  sb_changed(w, (int)bar, TRUE);
  return TRUE;
}

/* ============================= DefWindowProc ============================= */
static Wnd *top_of(Wnd *w) { while (w && w->parent) w = w->parent; return w; }

LRESULT DefWindowProc(HWND h, UINT m, WPARAM wp, LPARAM lp)
{
  Wnd *w = wnd(h);
  if (!w) return 0;
  switch (m) {
  case WM_NCCREATE: {
    CREATESTRUCT *cs = (CREATESTRUCT *)lp;
    if (cs && cs->lpszName && !IS_INTRESOURCE(cs->lpszName)) {
      std::lock_guard<std::recursive_mutex> lk(g_wm);
      w->text = cs->lpszName;
    }
    return TRUE;
  }
  case WM_NCCALCSIZE:
    return 0;
  case WM_NCHITTEST: {
    std::lock_guard<std::recursive_mutex> lk(g_wm);
    return nc_hittest(w, GET_X_LPARAM(lp), GET_Y_LPARAM(lp));
  }
  case WM_NCPAINT: {
    std::lock_guard<std::recursive_mutex> lk(g_wm);
    nc_paint(w);
    return 0;
  }
  case WM_NCACTIVATE: {
    std::lock_guard<std::recursive_mutex> lk(g_wm);
    w->active = wp != 0;
    nc_paint(w);
    return TRUE;
  }
  case WM_ACTIVATE:
    if (LOWORD(wp) != WA_INACTIVE && !HIWORD(wp)) SetFocus(h);
    return 0;
  case WM_MOUSEACTIVATE:
    if (w->parent) {
      LRESULT r = send_msg(w->parent->hwnd, WM_MOUSEACTIVATE, wp, lp);
      if (r) return r;
    }
    return MA_ACTIVATE;
  case WM_SF_CLICKACTIVATE: {
    Wnd *t = top_of(w);
    if (t && t->enabled()) {
      LRESULT r = send_msg(h, WM_MOUSEACTIVATE, (WPARAM)t->hwnd, MAKELPARAM(LOWORD(lp), HIWORD(lp)));
      if (r == MA_ACTIVATE || r == MA_ACTIVATEANDEAT) {
        if (g_active != t->hwnd) SetActiveWindow(t->hwnd);
      }
    }
    return 0;
  }
  case WM_SETCURSOR:
    return FALSE;
  case WM_ERASEBKGND: {
    HBRUSH br = w->cls->bg;
    if (!br) return 0;
    RECT r;
    GetClientRect(h, &r);
    FillRect((HDC)wp, &r, br);
    return 1;
  }
  case WM_PAINT: {
    PAINTSTRUCT ps;
    BeginPaint(h, &ps);
    EndPaint(h, &ps);
    return 0;
  }
  case WM_CLOSE:
    DestroyWindow(h);
    return 0;
  case WM_SYSCOMMAND:
    switch (wp & 0xFFF0) {
    case SC_CLOSE:    send_msg(h, WM_CLOSE, 0, 0); return 0;
    case SC_MINIMIZE: ShowWindow(h, SW_MINIMIZE); return 0;
    case SC_MAXIMIZE: ShowWindow(h, SW_MAXIMIZE); return 0;
    case SC_RESTORE:  ShowWindow(h, SW_RESTORE); return 0;
    case SC_MOUSEMENU: {
      int item;
      {
        std::lock_guard<std::recursive_mutex> lk(g_wm);
        item = menubar_item_at(w, GET_X_LPARAM(lp), GET_Y_LPARAM(lp), nullptr);
      }
      menu_track_bar(w, item, false);
      return 0;
    }
    case SC_KEYMENU:
      if (w->menu) menu_track_bar(w, 0, true);
      return 0;
    default:
      return 0;
    }
  case WM_NCLBUTTONDOWN:
    if (!w->is_child() && g_active != h && w->enabled() &&
        wp != HTCLOSE && wp != HTMINBUTTON && wp != HTMAXBUTTON)
      SetActiveWindow(h);
    switch (wp) {
    case HTCAPTION:
      if (!w->maximized) move_size_loop(w, HTCAPTION);
      return 0;
    case HTLEFT: case HTRIGHT: case HTTOP: case HTBOTTOM:
    case HTTOPLEFT: case HTTOPRIGHT: case HTBOTTOMLEFT: case HTBOTTOMRIGHT:
      move_size_loop(w, (int)wp);
      return 0;
    case HTCLOSE: case HTMINBUTTON: case HTMAXBUTTON:
      caption_button_loop(w, (int)wp);
      return 0;
    case HTMENU:
      send_msg(h, WM_SYSCOMMAND, SC_MOUSEMENU, lp);
      return 0;
    case HTVSCROLL: case HTHSCROLL: {
      bool vert = wp == HTVSCROLL;
      int part = sb_hit(h, false, vert, GET_X_LPARAM(lp), GET_Y_LPARAM(lp));
      sb_track(h, false, vert, part);
      return 0;
    }
    default:
      return 0;
    }
  case WM_NCLBUTTONDBLCLK:
    if (wp == HTCAPTION && (w->style & WS_MAXIMIZEBOX))
      send_msg(h, WM_SYSCOMMAND, w->maximized ? SC_RESTORE : SC_MAXIMIZE, lp);
    else if (wp == HTSYSMENU && (w->style & WS_SYSMENU))
      send_msg(h, WM_SYSCOMMAND, SC_CLOSE, lp);
    else if (wp == HTVSCROLL || wp == HTHSCROLL)
      return DefWindowProc(h, WM_NCLBUTTONDOWN, wp, lp);
    return 0;
  case WM_GETTEXT: {
    std::lock_guard<std::recursive_mutex> lk(g_wm);
    if (!lp || wp == 0) return 0;
    size_t n = std::min(w->text.size(), (size_t)wp - 1);
    memcpy((char *)lp, w->text.data(), n);
    ((char *)lp)[n] = 0;
    return (LRESULT)n;
  }
  case WM_GETTEXTLENGTH: {
    std::lock_guard<std::recursive_mutex> lk(g_wm);
    return (LRESULT)w->text.size();
  }
  case WM_SETTEXT: {
    std::lock_guard<std::recursive_mutex> lk(g_wm);
    w->text = lp ? (const char *)lp : "";
    nc_paint(w);
    return TRUE;
  }
  case WM_WINDOWPOSCHANGED: {
    WINDOWPOS *p = (WINDOWPOS *)lp;
    if (!p) return 0;
    if (!(p->flags & SWP_NOMOVE)) {
      int x = w->rc.left + w->client.left, y = w->rc.top + w->client.top;
      send_msg(h, WM_MOVE, 0, MAKELPARAM(x, y));
    }
    if (!(p->flags & SWP_NOSIZE) || (p->flags & SWP_FRAMECHANGED)) {
      if (!wnd(h)) return 0;
      RECT c;
      GetClientRect(h, &c);
      WPARAM k = w->maximized ? SIZE_MAXIMIZED : w->minimized ? SIZE_MINIMIZED : SIZE_RESTORED;
      send_msg(h, WM_SIZE, k, MAKELPARAM(c.right, c.bottom));
    }
    return 0;
  }
  case WM_CTLCOLORMSGBOX: case WM_CTLCOLOREDIT: case WM_CTLCOLORLISTBOX:
  case WM_CTLCOLORBTN: case WM_CTLCOLORDLG: case WM_CTLCOLORSCROLLBAR:
  case WM_CTLCOLORSTATIC: {
    HDC dc = (HDC)wp;
    if (m == WM_CTLCOLOREDIT || m == WM_CTLCOLORLISTBOX) {
      SetTextColor(dc, GetSysColor(COLOR_WINDOWTEXT));
      SetBkColor(dc, GetSysColor(COLOR_WINDOW));
      return (LRESULT)GetSysColorBrush(COLOR_WINDOW);
    }
    if (m == WM_CTLCOLORSCROLLBAR) return (LRESULT)GetSysColorBrush(COLOR_SCROLLBAR);
    SetTextColor(dc, GetSysColor(COLOR_WINDOWTEXT));
    SetBkColor(dc, GetSysColor(COLOR_3DFACE));
    return (LRESULT)GetSysColorBrush(COLOR_3DFACE);
  }
  case WM_SYSKEYDOWN:
    if (wp == VK_F4 && (lp & (1 << 29))) {
      Wnd *t = top_of(w);
      if (t) PostMessage(t->hwnd, WM_SYSCOMMAND, SC_CLOSE, 0);
    }
    return 0;
  case WM_MOUSEWHEEL:
    if (w->parent) return send_msg(w->parent->hwnd, m, wp, lp);
    return 0;
  case WM_CANCELMODE:
    if (GetCapture() == h) ReleaseCapture();
    return 0;
  case WM_QUERYENDSESSION:
    return TRUE;
  case WM_GETDLGCODE:
    return 0;
  case WM_GETFONT:
    return 0;
  default:
    return 0;
  }
}
