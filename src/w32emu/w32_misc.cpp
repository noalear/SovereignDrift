/* ************************************************************************* */
/* w32_misc.cpp -- icons and cursors (LoadIcon from the program's .ico,      */
/* CreateIcon/CreateIconIndirect, DrawIcon/DrawIconEx, LoadCursor,           */
/* SetCursor, ShowCursor).                                                   */
/*                                                                           */
/* NEW FILE, added 2026-10-12 for the Stellar Frontier Godot port.           */
/* ************************************************************************* */
#include "w32_internal.h"
#include <cstring>
#include <cstdio>

using namespace w32;

namespace {
struct IconObj : Obj {
  int w = 0, h = 0;
  std::vector<uint32_t> color;
  std::vector<uint8_t> mask;     /* 1 = transparent (AND mask bit set) */
  int cursorShape = -1;          /* >= 0 for system cursors */
};

std::string g_icon_file W32_EARLY;

HICON make_icon(IconObj *o) { return (HICON)hnew(o, HT_ICON); }
IconObj *icon_of(HICON h) { return H<IconObj>(h, HT_ICON); }

/* Parse a Windows .ico file and return the best image (32x32 preferred). */
IconObj *load_ico(const std::string &path)
{
  FILE *f = fopen(path.c_str(), "rb");
  if (!f) return nullptr;
  std::vector<uint8_t> d;
  fseek(f, 0, SEEK_END);
  long n = ftell(f);
  fseek(f, 0, SEEK_SET);
  if (n > 0) { d.resize(n); if (fread(d.data(), 1, n, f) != (size_t)n) d.clear(); }
  fclose(f);
  if (d.size() < 6) return nullptr;
  int count = d[4] | (d[5] << 8);
  int best = -1, bestScore = -1;
  for (int i = 0; i < count; i++) {
    size_t e = 6 + 16 * i;
    if (e + 16 > d.size()) break;
    int w = d[e] ? d[e] : 256;
    int bpp = d[e + 6] | (d[e + 7] << 8);
    int score = (w == 32 ? 1000 : 0) + bpp;
    if (score > bestScore) { bestScore = score; best = i; }
  }
  if (best < 0) return nullptr;
  size_t e = 6 + 16 * best;
  uint32_t off = d[e + 12] | (d[e + 13] << 8) | (d[e + 14] << 16) | ((uint32_t)d[e + 15] << 24);
  if (off + sizeof(BITMAPINFOHEADER) > d.size()) return nullptr;
  BITMAPINFOHEADER bh;
  memcpy(&bh, &d[off], sizeof(bh));
  int w = bh.biWidth, h = bh.biHeight / 2;
  if (w <= 0 || h <= 0) return nullptr;
  int ncol = bh.biBitCount <= 8 ? (bh.biClrUsed ? (int)bh.biClrUsed : (1 << bh.biBitCount)) : 0;
  std::vector<uint8_t> hdr(bh.biSize + ncol * 4);
  memcpy(hdr.data(), &d[off], hdr.size());
  ((BITMAPINFOHEADER *)hdr.data())->biHeight = h;
  const uint8_t *xorBits = &d[off + bh.biSize + ncol * 4];
  int xorStride = ((w * bh.biBitCount + 31) / 32) * 4;
  const uint8_t *andBits = xorBits + xorStride * h;
  int andStride = ((w + 31) / 32) * 4;
  if ((size_t)(andBits - d.data()) + andStride * h > d.size()) return nullptr;
  IconObj *o = new IconObj();
  int dw, dh;
  if (!dib_decode((const BITMAPINFO *)hdr.data(), xorBits, o->color, dw, dh)) { delete o; return nullptr; }
  o->w = w;
  o->h = h;
  o->mask.assign((size_t)w * h, 0);
  for (int y = 0; y < h; y++) {
    const uint8_t *row = andBits + (size_t)(h - 1 - y) * andStride;
    for (int x = 0; x < w; x++)
      o->mask[(size_t)y * w + x] = (row[x >> 3] >> (7 - (x & 7))) & 1;
  }
  return o;
}

void draw_icon_scaled(Target &t, IconObj *o, int x, int y, int cx, int cy)
{
  if (!o || o->w <= 0) return;
  if (cx <= 0) cx = o->w;
  if (cy <= 0) cy = o->h;
  for (int yy = 0; yy < cy; yy++) {
    int sy = yy * o->h / cy;
    for (int xx = 0; xx < cx; xx++) {
      int sx = xx * o->w / cx;
      size_t i = (size_t)sy * o->w + sx;
      if (o->mask[i]) {
        /* AND=1: XOR the colour onto the destination (usually black: transparent) */
        if (o->color[i]) plot(t, x + xx, y + yy, o->color[i], R2_XORPEN);
        continue;
      }
      plot(t, x + xx, y + yy, o->color[i]);
    }
  }
}
}  /* namespace */

namespace w32 {

std::atomic<int> g_cursor_shape{0};

void icon_set_file(const char *path) { g_icon_file = path ? path : ""; }

void icon_draw_small(Target &t, HICON ic, int x, int y)
{
  IconObj *o = icon_of(ic);
  if (o) draw_icon_scaled(t, o, x, y, 16, 16);
}

}  /* namespace w32 */

HICON LoadIcon(HINSTANCE inst, LPCSTR name)
{
  static HICON appIcon = 0;
  static HICON stock[8];
  if (inst && IS_INTRESOURCE(name) && (uintptr_t)name == 1) {
    if (!appIcon && !g_icon_file.empty()) {
      IconObj *o = load_ico(g_icon_file);
      if (o) appIcon = make_icon(o);
    }
    return appIcon;
  }
  if (!inst && IS_INTRESOURCE(name)) {
    int k = (int)((uintptr_t)name - 32512) & 7;
    if (!stock[k]) {
      IconObj *o = new IconObj();
      o->w = o->h = 32;
      o->color.assign(32 * 32, 0);
      o->mask.assign(32 * 32, 1);
      stock[k] = make_icon(o);
    }
    return stock[k];
  }
  return 0;
}

HICON CreateIcon(HINSTANCE, int w, int h, BYTE planes, BYTE bpp, const BYTE *andBits, const BYTE *xorBits)
{
  if (w <= 0 || h <= 0) return 0;
  IconObj *o = new IconObj();
  o->w = w;
  o->h = h;
  o->color.assign((size_t)w * h, 0);
  o->mask.assign((size_t)w * h, 0);
  int andStride = ((w + 15) / 16) * 2;
  for (int y = 0; y < h; y++)
    for (int x = 0; x < w; x++) {
      if (andBits) o->mask[(size_t)y * w + x] = (andBits[y * andStride + x / 8] >> (7 - (x & 7))) & 1;
      if (xorBits) {
        if (planes * bpp == 1) {
          o->color[(size_t)y * w + x] = ((xorBits[y * andStride + x / 8] >> (7 - (x & 7))) & 1) ? 0xFFFFFF : 0;
        } else if (planes * bpp == 32) {
          const BYTE *p = xorBits + ((size_t)y * w + x) * 4;
          o->color[(size_t)y * w + x] = (p[2] << 16) | (p[1] << 8) | p[0];
        } else if (planes * bpp == 24) {
          int stride = ((w * 24 + 15) / 16) * 2;
          const BYTE *p = xorBits + (size_t)y * stride + x * 3;
          o->color[(size_t)y * w + x] = (p[2] << 16) | (p[1] << 8) | p[0];
        }
      }
    }
  return make_icon(o);
}

HICON CreateIconIndirect(PICONINFO ii)
{
  if (!ii) return 0;
  BitmapObj *mask = H<BitmapObj>(ii->hbmMask, HT_BITMAP);
  BitmapObj *col = H<BitmapObj>(ii->hbmColor, HT_BITMAP);
  if (!mask) return 0;
  IconObj *o = new IconObj();
  o->w = mask->surf->w;
  o->h = col ? col->surf->h : mask->surf->h / 2;
  o->color.assign((size_t)o->w * o->h, 0);
  o->mask.assign((size_t)o->w * o->h, 0);
  for (int y = 0; y < o->h; y++)
    for (int x = 0; x < o->w; x++) {
      o->mask[(size_t)y * o->w + x] = mask->surf->px[(size_t)y * o->w + x] ? 1 : 0;
      if (col && x < col->surf->w) o->color[(size_t)y * o->w + x] = col->surf->px[(size_t)y * col->surf->w + x];
      else if (!col) o->color[(size_t)y * o->w + x] = mask->surf->px[(size_t)(y + o->h) * o->w + x];
    }
  if (!ii->fIcon) o->cursorShape = 0;
  return make_icon(o);
}

BOOL DestroyIcon(HICON h)
{
  IconObj *o = icon_of(h);
  if (!o) return FALSE;
  hdel(h);
  delete o;
  return TRUE;
}

BOOL DrawIconEx(HDC hdc, int x, int y, HICON h, int cx, int cy, UINT, HBRUSH, UINT)
{
  DCObj *dc = dc_from(hdc);
  IconObj *o = icon_of(h);
  if (!dc || !o) return FALSE;
  Target t;
  if (!resolve_dc(dc, t)) return TRUE;
  draw_icon_scaled(t, o, x + t.ox, y + t.oy, cx, cy);
  screen_dirty();
  return TRUE;
}

BOOL DrawIcon(HDC hdc, int x, int y, HICON h) { return DrawIconEx(hdc, x, y, h, 0, 0, 0, 0, DI_NORMAL); }

/* -------------------------------- cursors -------------------------------- */
HCURSOR LoadCursor(HINSTANCE inst, LPCSTR name)
{
  static HCURSOR cur[16];
  if (inst || !IS_INTRESOURCE(name)) return 0;
  int shape = 0;
  switch ((uintptr_t)name) {
  case 32513: shape = 1; break;   /* IDC_IBEAM */
  case 32514: shape = 2; break;   /* IDC_WAIT */
  case 32646: shape = 3; break;   /* IDC_SIZEALL */
  case 32649: shape = 4; break;   /* IDC_HAND */
  default: shape = 0; break;
  }
  if (!cur[shape]) {
    IconObj *o = new IconObj();
    o->cursorShape = shape;
    cur[shape] = make_icon(o);
  }
  return cur[shape];
}

static int g_show_count = 0;     /* ShowCursor display counter */
static int g_set_shape = 0;      /* shape from SetCursor, -1 for none */

static void publish_cursor()
{
  g_cursor_shape = g_show_count < 0 ? -1 : g_set_shape;
}

HCURSOR SetCursor(HCURSOR h)
{
  static HCURSOR current = 0;
  HCURSOR old = current;
  current = h;
  IconObj *o = icon_of(h);
  /* SetCursor(NULL) removes the cursor from the screen */
  g_set_shape = !h ? -1 : (o && o->cursorShape >= 0 ? o->cursorShape : 0);
  publish_cursor();
  return old;
}

int ShowCursor(BOOL show)
{
  int r = (g_show_count += show ? 1 : -1);
  publish_cursor();
  return r;
}
