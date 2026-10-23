/* ************************************************************************* */
/* w32_gdi.cpp -- GDI emulation: device contexts, pens, brushes, bitmaps,    */
/* palettes, clipping, raster primitives, BitBlt/StretchBlt with ternary     */
/* raster operations, and device-independent bitmaps.                        */
/*                                                                           */
/* NEW FILE, added 2026-10-12 for the Stellar Frontier Godot port.           */
/*                                                                           */
/* Pixels are 32-bit 0x00RRGGBB; the emulated display is 32 bits per pixel   */
/* (true colour), so the program takes its 65536-colour bitmap path.          */
/* ************************************************************************* */
#include "w32_internal.h"
#include <cmath>
#include <cstring>

using namespace w32;

namespace w32 {

std::atomic<uint32_t> g_screen_serial{1};

void Surface::resize(int nw, int nh, uint32_t fill, bool keep)
{
  if (nw < 0) nw = 0;
  if (nh < 0) nh = 0;
  if (nw == w && nh == h) return;
  std::vector<uint32_t> n((size_t)nw * nh, fill);
  if (keep) {
    int cw = std::min(w, nw), ch = std::min(h, nh);
    for (int y = 0; y < ch; y++)
      memcpy(&n[(size_t)y * nw], &px[(size_t)y * w], cw * 4);
  }
  px.swap(n);
  w = nw;
  h = nh;
}

/* ============================== stock objects ============================ */
static HGDIOBJ g_stock[20];
static BitmapObj *g_default_bitmap;   /* 1x1 monochrome, selected into new memory DCs */

static HGDIOBJ make_stock_brush(int style, COLORREF c)
{
  BrushObj *b = new BrushObj();
  b->style = style;
  b->color = c;
  b->stock = true;
  return (HGDIOBJ)hnew(b, HT_BRUSH);
}

static HGDIOBJ make_stock_pen(int style, COLORREF c)
{
  PenObj *p = new PenObj();
  p->style = style;
  p->color = c;
  p->stock = true;
  return (HGDIOBJ)hnew(p, HT_PEN);
}

static HGDIOBJ make_stock_font(const SfFontData *fd)
{
  FontObj *f = new FontObj();
  memset(&f->lf, 0, sizeof(f->lf));
  f->fd = fd;
  f->stock = true;
  return (HGDIOBJ)hnew(f, HT_FONT);
}

void gdi_init()
{
  static std::once_flag once;
  std::call_once(once, []() {
    g_stock[WHITE_BRUSH]  = make_stock_brush(BS_SOLID, RGB(255,255,255));
    g_stock[LTGRAY_BRUSH] = make_stock_brush(BS_SOLID, RGB(192,192,192));
    g_stock[GRAY_BRUSH]   = make_stock_brush(BS_SOLID, RGB(128,128,128));
    g_stock[DKGRAY_BRUSH] = make_stock_brush(BS_SOLID, RGB(64,64,64));
    g_stock[BLACK_BRUSH]  = make_stock_brush(BS_SOLID, RGB(0,0,0));
    g_stock[NULL_BRUSH]   = make_stock_brush(BS_NULL, 0);
    g_stock[WHITE_PEN]    = make_stock_pen(PS_SOLID, RGB(255,255,255));
    g_stock[BLACK_PEN]    = make_stock_pen(PS_SOLID, RGB(0,0,0));
    g_stock[NULL_PEN]     = make_stock_pen(PS_NULL, 0);
    g_stock[SYSTEM_FONT]  = make_stock_font(font_default_gui_bold());
    g_stock[ANSI_VAR_FONT] = make_stock_font(font_default_gui());
    g_stock[DEFAULT_GUI_FONT] = make_stock_font(font_default_gui());
    g_stock[DEVICE_DEFAULT_FONT] = g_stock[SYSTEM_FONT];
    g_stock[SYSTEM_FIXED_FONT] = g_stock[SYSTEM_FONT];
    g_stock[ANSI_FIXED_FONT] = g_stock[SYSTEM_FONT];
    g_stock[OEM_FIXED_FONT] = g_stock[SYSTEM_FONT];
    PaletteObj *p = new PaletteObj();
    p->stock = true;
    g_stock[DEFAULT_PALETTE] = (HGDIOBJ)hnew(p, HT_PALETTE);
    g_default_bitmap = new BitmapObj();
    g_default_bitmap->surf = std::make_shared<Surface>();
    g_default_bitmap->surf->resize(1, 1, 0, false);
    g_default_bitmap->mono = true;
    g_default_bitmap->bpp = 1;
    g_default_bitmap->stock = true;
    hnew(g_default_bitmap, HT_BITMAP);
  });
}

}  /* namespace w32 */

HGDIOBJ GetStockObject(int i)
{
  gdi_init();
  if (i < 0 || i >= 20) return nullptr;
  return g_stock[i];
}

/* ============================= object creation =========================== */
HPEN CreatePen(int style, int width, COLORREF c)
{
  gdi_init();
  PenObj *p = new PenObj();
  p->style = style;
  p->width = width < 1 ? 1 : width;
  p->color = c;
  return (HPEN)hnew(p, HT_PEN);
}

HBRUSH CreateSolidBrush(COLORREF c)
{
  gdi_init();
  BrushObj *b = new BrushObj();
  b->style = BS_SOLID;
  b->color = c;
  return (HBRUSH)hnew(b, HT_BRUSH);
}

static BitmapObj *new_bitmap(int w, int h, bool mono)
{
  BitmapObj *b = new BitmapObj();
  b->surf = std::make_shared<Surface>();
  b->surf->resize(w, h, mono ? 0 : 0, false);
  b->mono = mono;
  b->bpp = mono ? 1 : 32;
  hnew(b, HT_BITMAP);
  return b;
}

namespace w32 {
DCObj *dc_from(HDC h) { return H<DCObj>(h, HT_DC); }
}

static bool dc_is_mono(DCObj *dc)
{
  return dc && dc->kind == DCObj::MEM && dc->bmp && dc->bmp->mono;
}

HBITMAP CreateCompatibleBitmap(HDC hdc, int w, int h)
{
  gdi_init();
  DCObj *dc = dc_from(hdc);
  if (w <= 0 || h <= 0) { w = 1; h = 1; }
  BitmapObj *b = new_bitmap(w, h, dc_is_mono(dc));
  return (HBITMAP)b->handle;
}

HBITMAP CreateBitmap(int w, int h, UINT planes, UINT bpp, const void *bits)
{
  gdi_init();
  bool mono = (planes * bpp) == 1;
  BitmapObj *b = new_bitmap(w, h, mono);
  if (bits) {
    const uint8_t *src = (const uint8_t *)bits;
    if (mono) {
      int stride = ((w + 15) / 16) * 2;
      for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++)
          b->surf->px[(size_t)y * w + x] =
            (src[y * stride + x / 8] & (0x80 >> (x & 7))) ? 0xFFFFFF : 0;
    } else if (bpp == 32) {
      memcpy(b->surf->px.data(), bits, (size_t)w * h * 4);
    }
  }
  return (HBITMAP)b->handle;
}

/* ========================== DIB decoding/encoding ======================== */
/* Decode a packed DIB (BITMAPINFO + bits) into XRGB rows (top-down).  For
   DIB_PAL_COLORS the colour table holds 16-bit indices into the logical
   palette selected in dc (identity grey ramp if none).  Returns false if
   the format is not supported. */
static bool decode_dib(const BITMAPINFO *bi, const void *bitsv, UINT usage,
                       DCObj *dc, std::vector<uint32_t> &out, int &w, int &h)
{
  const BITMAPINFOHEADER &bh = bi->bmiHeader;
  w = bh.biWidth;
  int ah = bh.biHeight;
  bool topdown = ah < 0;
  h = topdown ? -ah : ah;
  if (w <= 0 || h <= 0 || !bitsv) return false;
  int bpp = bh.biBitCount;
  const uint8_t *bits = (const uint8_t *)bitsv;
  out.assign((size_t)w * h, 0);

  /* colour table */
  uint32_t pal[256];
  int ncol = 0;
  if (bpp <= 8) {
    ncol = bh.biClrUsed ? (int)bh.biClrUsed : (1 << bpp);
    if (ncol > 256) ncol = 256;
    const uint8_t *ct = (const uint8_t *)bi + bh.biSize;
    for (int i = 0; i < 256; i++) pal[i] = 0;
    for (int i = 0; i < ncol; i++) {
      if (usage == DIB_PAL_COLORS) {
        WORD idx = ((const WORD *)ct)[i];
        PaletteObj *lp = dc ? dc->pal : nullptr;
        if (lp && idx < lp->entries.size()) {
          const PALETTEENTRY &e = lp->entries[idx];
          pal[i] = (e.peRed << 16) | (e.peGreen << 8) | e.peBlue;
        } else {
          pal[i] = (idx << 16) | (idx << 8) | idx;
        }
      } else {
        const RGBQUAD &q = ((const RGBQUAD *)ct)[i];
        pal[i] = (q.rgbRed << 16) | (q.rgbGreen << 8) | q.rgbBlue;
      }
    }
  }

  auto dst_row = [&](int fileRow) -> uint32_t * {
    int y = topdown ? fileRow : (h - 1 - fileRow);
    return &out[(size_t)y * w];
  };

  if (bh.biCompression == BI_RLE8 || bh.biCompression == BI_RLE4) {
    /* RLE rows are always bottom-up */
    bool rle4 = bh.biCompression == BI_RLE4;
    size_t n = bh.biSizeImage ? bh.biSizeImage : (size_t)-1;
    size_t i = 0;
    int x = 0, row = 0;
    while (i + 1 < n && row < h) {
      uint8_t a = bits[i++], b = bits[i++];
      if (a) {   /* encoded run */
        for (int k = 0; k < a; k++, x++) {
          if (x >= w || row >= h) continue;
          uint8_t idx = rle4 ? ((k & 1) ? (b & 15) : (b >> 4)) : b;
          out[(size_t)(h - 1 - row) * w + x] = pal[idx];
        }
      } else if (b == 0) { x = 0; row++; }
      else if (b == 1) break;
      else if (b == 2) { if (i + 1 >= n) break; x += bits[i++]; row += bits[i++]; }
      else {     /* absolute run of b pixels */
        int cnt = b;
        for (int k = 0; k < cnt; k++, x++) {
          uint8_t idx;
          if (rle4) idx = (k & 1) ? (bits[i + k / 2] & 15) : (bits[i + k / 2] >> 4);
          else idx = bits[i + k];
          if (x < w && row < h) out[(size_t)(h - 1 - row) * w + x] = pal[idx];
        }
        size_t used = rle4 ? (size_t)((cnt + 1) / 2) : (size_t)cnt;
        i += (used + 1) & ~(size_t)1;   /* word aligned */
      }
    }
    return true;
  }

  int stride = ((w * bpp + 31) / 32) * 4;
  uint32_t rmask = 0x7C00, gmask = 0x03E0, bmask = 0x001F;
  if (bh.biCompression == BI_BITFIELDS) {
    const DWORD *m = (const DWORD *)((const uint8_t *)bi + bh.biSize);
    rmask = m[0]; gmask = m[1]; bmask = m[2];
  } else if (bpp == 32) {
    rmask = 0xFF0000; gmask = 0xFF00; bmask = 0xFF;
  }
  auto chan = [](uint32_t v, uint32_t mask) -> uint32_t {
    if (!mask) return 0;
    int sh = 0;
    while (!(mask & 1)) { mask >>= 1; sh++; }
    uint32_t c = (v >> sh) & mask;
    int bits = 0;
    while (mask) { bits++; mask >>= 1; }
    if (bits >= 8) return c >> (bits - 8);
    /* expand like the display driver: shift up, replicate high bits */
    uint32_t r = c << (8 - bits);
    return r | (r >> bits);
  };
  for (int r = 0; r < h; r++) {
    const uint8_t *s = bits + (size_t)r * stride;
    uint32_t *d = dst_row(r);
    switch (bpp) {
    case 1:
      for (int x = 0; x < w; x++) d[x] = pal[(s[x >> 3] >> (7 - (x & 7))) & 1];
      break;
    case 4:
      for (int x = 0; x < w; x++) d[x] = pal[(x & 1) ? (s[x >> 1] & 15) : (s[x >> 1] >> 4)];
      break;
    case 8:
      for (int x = 0; x < w; x++) d[x] = pal[s[x]];
      break;
    case 16:
      for (int x = 0; x < w; x++) {
        uint32_t v = s[2 * x] | (s[2 * x + 1] << 8);
        d[x] = (chan(v, rmask) << 16) | (chan(v, gmask) << 8) | chan(v, bmask);
      }
      break;
    case 24:
      for (int x = 0; x < w; x++)
        d[x] = (s[3 * x + 2] << 16) | (s[3 * x + 1] << 8) | s[3 * x];
      break;
    case 32:
      for (int x = 0; x < w; x++) {
        uint32_t v = s[4 * x] | (s[4 * x + 1] << 8) | (s[4 * x + 2] << 16) | ((uint32_t)s[4 * x + 3] << 24);
        if (bh.biCompression == BI_BITFIELDS)
          d[x] = (chan(v, rmask) << 16) | (chan(v, gmask) << 8) | chan(v, bmask);
        else
          d[x] = v & 0xFFFFFF;
      }
      break;
    default:
      return false;
    }
  }
  return true;
}

namespace w32 {
bool dib_decode(const BITMAPINFO *bi, const void *bits, std::vector<uint32_t> &out, int &w, int &h)
{
  return decode_dib(bi, bits, DIB_RGB_COLORS, nullptr, out, w, h);
}
}

HBITMAP CreateDIBitmap(HDC hdc, const BITMAPINFOHEADER *bih, DWORD init,
                       const void *bits, const BITMAPINFO *bi, UINT usage)
{
  gdi_init();
  if (!bih) return nullptr;
  int w = bih->biWidth, h = bih->biHeight < 0 ? -bih->biHeight : bih->biHeight;
  DCObj *dc = dc_from(hdc);
  BitmapObj *b = new_bitmap(w > 0 ? w : 1, h > 0 ? h : 1, dc_is_mono(dc));
  if ((init & CBM_INIT) && bits && bi) {
    std::vector<uint32_t> px;
    int dw, dh;
    if (decode_dib(bi, bits, usage, dc, px, dw, dh)) {
      if (b->mono)
        for (auto &p : px) p = p ? 0xFFFFFF : 0;   /* approximation */
      b->surf->px.swap(px);
    }
  }
  return (HBITMAP)b->handle;
}

HBITMAP CreateDIBSection(HDC, const BITMAPINFO *, UINT, void **bits, HANDLE, DWORD)
{
  /* Not used by Stellar Frontier. */
  if (bits) *bits = nullptr;
  return nullptr;
}

int GetDIBits(HDC hdc, HBITMAP hbm, UINT start, UINT lines, LPVOID bits,
              LPBITMAPINFO bi, UINT usage)
{
  BitmapObj *b = H<BitmapObj>(hbm, HT_BITMAP);
  if (!b || !bi) return 0;
  BITMAPINFOHEADER &bh = bi->bmiHeader;
  Surface &s = *b->surf;
  if (!bits) {
    /* fill in the header */
    bh.biWidth = s.w;
    bh.biHeight = s.h;
    bh.biPlanes = 1;
    if (bh.biBitCount == 0) bh.biBitCount = (WORD)b->bpp;
    bh.biCompression = BI_RGB;
    bh.biSizeImage = ((s.w * bh.biBitCount + 31) / 32) * 4 * s.h;
    return s.h;
  }
  int bpp = bh.biBitCount;
  bool topdown = bh.biHeight < 0;
  int stride = ((s.w * bpp + 31) / 32) * 4;
  uint8_t *out = (uint8_t *)bits;
  int done = 0;
  for (UINT k = 0; k < lines; k++) {
    int fileRow = (int)(start + k);
    if (fileRow >= s.h) break;
    int y = topdown ? fileRow : s.h - 1 - fileRow;
    const uint32_t *src = &s.px[(size_t)y * s.w];
    uint8_t *d = out + (size_t)k * stride;
    for (int x = 0; x < s.w; x++) {
      uint32_t p = src[x];
      switch (bpp) {
      case 32: d[4*x] = p & 255; d[4*x+1] = (p >> 8) & 255; d[4*x+2] = (p >> 16) & 255; d[4*x+3] = 0; break;
      case 24: d[3*x] = p & 255; d[3*x+1] = (p >> 8) & 255; d[3*x+2] = (p >> 16) & 255; break;
      case 16: { uint16_t v = (uint16_t)((((p >> 19) & 31) << 10) | (((p >> 11) & 31) << 5) | ((p >> 3) & 31));
                 d[2*x] = v & 255; d[2*x+1] = v >> 8; break; }
      default: break;
      }
    }
    done++;
  }
  (void)hdc; (void)usage;
  return done;
}

int SetDIBits(HDC hdc, HBITMAP hbm, UINT start, UINT lines, const void *bits,
              const BITMAPINFO *bi, UINT usage)
{
  BitmapObj *b = H<BitmapObj>(hbm, HT_BITMAP);
  if (!b || !bi) return 0;
  std::vector<uint32_t> px;
  int w, h;
  if (!decode_dib(bi, bits, usage, dc_from(hdc), px, w, h)) return 0;
  Surface &s = *b->surf;
  int cw = std::min(w, s.w);
  for (int y = 0; y < std::min(h, s.h); y++)
    memcpy(&s.px[(size_t)y * s.w], &px[(size_t)y * w], cw * 4);
  (void)start;
  return (int)lines;
}

/* ================================ palettes =============================== */
HPALETTE CreatePalette(const LOGPALETTE *lp)
{
  gdi_init();
  PaletteObj *p = new PaletteObj();
  if (lp) p->entries.assign(lp->palPalEntry, lp->palPalEntry + lp->palNumEntries);
  return (HPALETTE)hnew(p, HT_PALETTE);
}

HPALETTE SelectPalette(HDC hdc, HPALETTE hp, BOOL)
{
  DCObj *dc = dc_from(hdc);
  PaletteObj *p = H<PaletteObj>(hp, HT_PALETTE);
  if (!dc || !p) return nullptr;
  PaletteObj *old = dc->pal;
  if (old) old->selected--;
  dc->pal = p;
  p->selected++;
  return old ? (HPALETTE)old->handle : (HPALETTE)GetStockObject(DEFAULT_PALETTE);
}

UINT RealizePalette(HDC) { return 0; }   /* true-colour display */

UINT GetPaletteEntries(HPALETTE hp, UINT start, UINT n, LPPALETTEENTRY out)
{
  PaletteObj *p = H<PaletteObj>(hp, HT_PALETTE);
  if (!p) return 0;
  if (!out) return (UINT)p->entries.size();
  UINT k = 0;
  for (; k < n && start + k < p->entries.size(); k++) out[k] = p->entries[start + k];
  return k;
}

UINT SetPaletteEntries(HPALETTE hp, UINT start, UINT n, const PALETTEENTRY *in)
{
  PaletteObj *p = H<PaletteObj>(hp, HT_PALETTE);
  if (!p || !in) return 0;
  if (p->entries.size() < start + n) p->entries.resize(start + n);
  for (UINT k = 0; k < n; k++) p->entries[start + k] = in[k];
  return n;
}

UINT GetSystemPaletteEntries(HDC, UINT, UINT, LPPALETTEENTRY) { return 0; }

/* ================================ regions ================================ */
HRGN CreateRectRgn(int l, int t, int r, int b)
{
  RgnObj *g = new RgnObj();
  if (r > l && b > t) g->rects.push_back(RECT{l, t, r, b});
  return (HRGN)hnew(g, HT_RGN);
}

/* =============================== select/delete =========================== */
HGDIOBJ SelectObject(HDC hdc, HGDIOBJ h)
{
  DCObj *dc = dc_from(hdc);
  Obj *o = hget_any(h);
  if (!dc || !o) return nullptr;
  switch (o->type) {
  case HT_PEN: {
    PenObj *old = dc->pen;
    dc->pen = static_cast<PenObj *>(o);
    dc->pen->selected++;
    if (old) old->selected--;
    return old ? (HGDIOBJ)old->handle : nullptr;
  }
  case HT_BRUSH: {
    BrushObj *old = dc->brush;
    dc->brush = static_cast<BrushObj *>(o);
    dc->brush->selected++;
    if (old) old->selected--;
    return old ? (HGDIOBJ)old->handle : nullptr;
  }
  case HT_FONT: {
    FontObj *old = dc->font;
    dc->font = static_cast<FontObj *>(o);
    dc->font->selected++;
    if (old) old->selected--;
    return old ? (HGDIOBJ)old->handle : nullptr;
  }
  case HT_BITMAP: {
    if (dc->kind != DCObj::MEM) return nullptr;
    BitmapObj *nb = static_cast<BitmapObj *>(o);
    if (nb->selected && nb != dc->bmp && !nb->stock) return nullptr;   /* one DC at a time */
    BitmapObj *old = dc->bmp;
    dc->bmp = nb;
    nb->selected++;
    if (old) old->selected--;
    return old ? (HGDIOBJ)old->handle : nullptr;
  }
  case HT_RGN: {
    RgnObj *r = static_cast<RgnObj *>(o);
    dc->hasClip = true;
    dc->clip = r->rects;
    return (HGDIOBJ)(uintptr_t)(r->rects.empty() ? NULLREGION : SIMPLEREGION);
  }
  default:
    return nullptr;
  }
}

BOOL DeleteObject(HGDIOBJ h)
{
  Obj *o = hget_any(h);
  if (!o) return FALSE;
  switch (o->type) {
  case HT_PEN: case HT_BRUSH: case HT_FONT: case HT_BITMAP: case HT_PALETTE: case HT_RGN: {
    GdiObj *g = static_cast<GdiObj *>(o);
    if (g->stock) return TRUE;
    if (g->selected > 0) return FALSE;       /* still selected into a DC */
    hdel(h);
    delete g;
    return TRUE;
  }
  default:
    return FALSE;
  }
}

int GetObject(HGDIOBJ h, int n, LPVOID out)
{
  Obj *o = hget_any(h);
  if (!o || !out) return 0;
  if (o->type == HT_BITMAP && n >= (int)sizeof(BITMAP)) {
    BitmapObj *b = static_cast<BitmapObj *>(o);
    BITMAP *bm = (BITMAP *)out;
    bm->bmType = 0;
    bm->bmWidth = b->surf->w;
    bm->bmHeight = b->surf->h;
    bm->bmPlanes = 1;
    bm->bmBitsPixel = (WORD)b->bpp;
    bm->bmWidthBytes = ((b->surf->w * b->bpp + 15) / 16) * 2;
    bm->bmBits = nullptr;
    return sizeof(BITMAP);
  }
  if (o->type == HT_FONT && n >= (int)sizeof(LOGFONT)) {
    *(LOGFONT *)out = static_cast<FontObj *>(o)->lf;
    return sizeof(LOGFONT);
  }
  return 0;
}

/* ================================== DCs ================================== */
static DCObj *new_dc(DCObj::Kind k)
{
  gdi_init();
  DCObj *dc = new DCObj();
  dc->kind = k;
  dc->pen = H<PenObj>(GetStockObject(BLACK_PEN), HT_PEN);
  dc->brush = H<BrushObj>(GetStockObject(WHITE_BRUSH), HT_BRUSH);
  dc->font = H<FontObj>(GetStockObject(SYSTEM_FONT), HT_FONT);
  dc->pal = H<PaletteObj>(GetStockObject(DEFAULT_PALETTE), HT_PALETTE);
  dc->pen->selected++;
  dc->brush->selected++;
  dc->font->selected++;
  dc->pal->selected++;
  if (k == DCObj::MEM) {
    dc->bmp = g_default_bitmap;
    dc->bmp->selected++;
  }
  hnew(dc, HT_DC);
  return dc;
}

static void free_dc(DCObj *dc)
{
  if (dc->pen) dc->pen->selected--;
  if (dc->brush) dc->brush->selected--;
  if (dc->font) dc->font->selected--;
  if (dc->pal) dc->pal->selected--;
  if (dc->bmp) dc->bmp->selected--;
  hdel((void *)dc->handle);
  delete dc;
}

namespace w32 {
HDC dc_new_window(HWND hwnd, bool client, bool paint, const RECT *paintRect)
{
  DCObj *dc = new_dc(DCObj::WIN);
  dc->hwnd = hwnd;
  dc->client = client;
  dc->paint = paint;
  if (paintRect) dc->paintRect = *paintRect;
  return (HDC)dc->handle;
}

HDC dc_new_screen() { return (HDC)new_dc(DCObj::SCREEN)->handle; }

void dc_free(HDC h)
{
  DCObj *dc = dc_from(h);
  if (dc) free_dc(dc);
}
}  /* namespace w32 */

HDC CreateDC(LPCSTR, LPCSTR, LPCSTR, const void *) { return dc_new_screen(); }

HDC CreateCompatibleDC(HDC) { return (HDC)new_dc(DCObj::MEM)->handle; }

BOOL DeleteDC(HDC h)
{
  DCObj *dc = dc_from(h);
  if (!dc) return FALSE;
  free_dc(dc);
  return TRUE;
}

int SaveDC(HDC) { return 1; }
BOOL RestoreDC(HDC, int) { return TRUE; }

int GetDeviceCaps(HDC, int idx)
{
  switch (idx) {
  case HORZRES:    return g_screen_w;
  case VERTRES:    return g_screen_h;
  case BITSPIXEL:  return 32;
  case PLANES:     return 1;
  case NUMCOLORS:  return -1;
  case LOGPIXELSX: return 96;
  case LOGPIXELSY: return 96;
  case SIZEPALETTE: return 0;
  case NUMRESERVED: return 20;
  case COLORRES:   return 24;
  case TECHNOLOGY: return DT_RASDISPLAY;
  case RASTERCAPS: return 0x0001 | 0x0008 | 0x0080 | 0x0200 | 0x0800;
  default:         return 0;
  }
}

int SetStretchBltMode(HDC h, int m) { DCObj *dc = dc_from(h); if (!dc) return 0; int o = dc->stretch; dc->stretch = m; return o; }
int SetBkMode(HDC h, int m) { DCObj *dc = dc_from(h); if (!dc) return 0; int o = dc->bkMode; dc->bkMode = m; return o; }
int GetBkMode(HDC h) { DCObj *dc = dc_from(h); return dc ? dc->bkMode : 0; }
COLORREF SetBkColor(HDC h, COLORREF c) { DCObj *dc = dc_from(h); if (!dc) return CLR_INVALID; COLORREF o = dc->bk; dc->bk = c; return o; }
COLORREF GetBkColor(HDC h) { DCObj *dc = dc_from(h); return dc ? dc->bk : 0; }
COLORREF SetTextColor(HDC h, COLORREF c) { DCObj *dc = dc_from(h); if (!dc) return CLR_INVALID; COLORREF o = dc->text; dc->text = c; return o; }
COLORREF GetTextColor(HDC h) { DCObj *dc = dc_from(h); return dc ? dc->text : 0; }
UINT SetTextAlign(HDC h, UINT a) { DCObj *dc = dc_from(h); if (!dc) return 0; UINT o = dc->align; dc->align = a; return o; }
UINT GetTextAlign(HDC h) { DCObj *dc = dc_from(h); return dc ? dc->align : 0; }
int SetROP2(HDC h, int r) { DCObj *dc = dc_from(h); if (!dc) return 0; int o = dc->rop2; dc->rop2 = r; return o; }
BOOL GdiFlush(void) { return TRUE; }

/* ================================ clipping =============================== */
static void rect_subtract(const RECT &a, const RECT &b, std::vector<RECT> &out)
{
  /* a minus b, as up to four rectangles */
  RECT i = { std::max(a.left, b.left), std::max(a.top, b.top),
             std::min(a.right, b.right), std::min(a.bottom, b.bottom) };
  if (i.left >= i.right || i.top >= i.bottom) { out.push_back(a); return; }
  if (a.top < i.top)       out.push_back(RECT{a.left, a.top, a.right, i.top});
  if (i.bottom < a.bottom) out.push_back(RECT{a.left, i.bottom, a.right, a.bottom});
  if (a.left < i.left)     out.push_back(RECT{a.left, i.top, i.left, i.bottom});
  if (i.right < a.right)   out.push_back(RECT{i.right, i.top, a.right, i.bottom});
}

static RECT dc_extent(DCObj *dc)
{
  Target t;
  if (resolve_dc(dc, t) || t.s) {
    /* full extent in DC coordinates, ignoring the DC clip region */
    return RECT{-t.ox, -t.oy, t.s->w - t.ox, t.s->h - t.oy};
  }
  return RECT{-100000, -100000, 100000, 100000};
}

int SelectClipRgn(HDC h, HRGN rgn)
{
  DCObj *dc = dc_from(h);
  if (!dc) return ERROR;
  if (!rgn) { dc->hasClip = false; dc->clip.clear(); return SIMPLEREGION; }
  RgnObj *r = H<RgnObj>(rgn, HT_RGN);
  if (!r) return ERROR;
  dc->hasClip = true;
  dc->clip = r->rects;
  return r->rects.empty() ? NULLREGION : (r->rects.size() == 1 ? SIMPLEREGION : COMPLEXREGION);
}

int IntersectClipRect(HDC h, int l, int t, int r, int b)
{
  DCObj *dc = dc_from(h);
  if (!dc) return ERROR;
  if (!dc->hasClip) {
    dc->hasClip = true;
    dc->clip.assign(1, dc_extent(dc));
  }
  std::vector<RECT> out;
  for (auto &c : dc->clip) {
    RECT i = { std::max(c.left, (LONG)l), std::max(c.top, (LONG)t),
               std::min(c.right, (LONG)r), std::min(c.bottom, (LONG)b) };
    if (i.left < i.right && i.top < i.bottom) out.push_back(i);
  }
  dc->clip.swap(out);
  return dc->clip.empty() ? NULLREGION : (dc->clip.size() == 1 ? SIMPLEREGION : COMPLEXREGION);
}

int ExcludeClipRect(HDC h, int l, int t, int r, int b)
{
  DCObj *dc = dc_from(h);
  if (!dc) return ERROR;
  if (!dc->hasClip) {
    dc->hasClip = true;
    dc->clip.assign(1, dc_extent(dc));
  }
  std::vector<RECT> out;
  RECT ex = {l, t, r, b};
  for (auto &c : dc->clip) rect_subtract(c, ex, out);
  dc->clip.swap(out);
  return dc->clip.empty() ? NULLREGION : (dc->clip.size() == 1 ? SIMPLEREGION : COMPLEXREGION);
}

int GetClipBox(HDC h, LPRECT r)
{
  DCObj *dc = dc_from(h);
  if (!dc || !r) return ERROR;
  Target t;
  if (!resolve_dc(dc, t)) { SetRectEmpty(r); return NULLREGION; }
  RECT u = t.clips[0];
  for (auto &c : t.clips) {
    u.left = std::min(u.left, c.left); u.top = std::min(u.top, c.top);
    u.right = std::max(u.right, c.right); u.bottom = std::max(u.bottom, c.bottom);
  }
  r->left = u.left - t.ox; r->right = u.right - t.ox;
  r->top = u.top - t.oy; r->bottom = u.bottom - t.oy;
  return SIMPLEREGION;
}

/* ============================ target resolution ========================== */
namespace w32 {
bool wnd_target(HWND h, bool client, bool paint, const RECT &paintRect, Target &t);

bool resolve_dc(DCObj *dc, Target &t)
{
  t = Target();
  RECT base;
  switch (dc->kind) {
  case DCObj::MEM:
    if (!dc->bmp || !dc->bmp->surf) return false;
    t.hold = dc->bmp->surf;
    t.s = t.hold.get();
    base = RECT{0, 0, t.s->w, t.s->h};
    t.clips.push_back(base);
    break;
  case DCObj::WIN:
    if (!wnd_target(dc->hwnd, dc->client, dc->paint, dc->paintRect, t)) return false;
    break;
  default:
    return false;   /* screen/information DCs: drawing is discarded */
  }
  if (dc->hasClip) {
    std::vector<RECT> out;
    for (auto &c : dc->clip) {
      RECT cs = { c.left + t.ox, c.top + t.oy, c.right + t.ox, c.bottom + t.oy };
      for (auto &b : t.clips) {
        RECT i = { std::max(cs.left, b.left), std::max(cs.top, b.top),
                   std::min(cs.right, b.right), std::min(cs.bottom, b.bottom) };
        if (i.left < i.right && i.top < i.bottom) out.push_back(i);
      }
    }
    t.clips.swap(out);
  }
  return !t.clips.empty();
}

/* ============================ raster primitives ========================== */
static inline uint32_t rop2_apply(int rop2, uint32_t d, uint32_t p)
{
  switch (rop2) {
  case R2_COPYPEN:     return p;
  case R2_BLACK:       return 0;
  case R2_WHITE:       return 0xFFFFFF;
  case R2_NOT:         return ~d & 0xFFFFFF;
  case R2_XORPEN:      return d ^ p;
  case R2_MASKPEN:     return d & p;
  case R2_MERGEPEN:    return d | p;
  case R2_NOP:         return d;
  case R2_NOTCOPYPEN:  return ~p & 0xFFFFFF;
  case R2_NOTXORPEN:   return ~(d ^ p) & 0xFFFFFF;
  case R2_MASKNOTPEN:  return d & ~p & 0xFFFFFF;
  case R2_MERGENOTPEN: return (d | ~p) & 0xFFFFFF;
  case R2_MASKPENNOT:  return p & ~d & 0xFFFFFF;
  case R2_MERGEPENNOT: return (p | ~d) & 0xFFFFFF;
  case R2_NOTMASKPEN:  return ~(d & p) & 0xFFFFFF;
  case R2_NOTMERGEPEN: return ~(d | p) & 0xFFFFFF;
  default:             return p;
  }
}

void hline(Target &t, int x0, int x1, int y, uint32_t c, int rop2)
{
  /* [x0, x1) */
  if (x0 > x1) std::swap(x0, x1);
  for (auto &r : t.clips) {
    if (y < r.top || y >= r.bottom) continue;
    int a = std::max(x0, (int)r.left), b = std::min(x1, (int)r.right);
    if (a >= b) continue;
    uint32_t *p = t.s->row(y) + a;
    if (rop2 == R2_COPYPEN) { for (int x = a; x < b; x++) *p++ = c; }
    else { for (int x = a; x < b; x++, p++) *p = rop2_apply(rop2, *p, c); }
  }
}

void plot(Target &t, int x, int y, uint32_t c, int rop2)
{
  for (auto &r : t.clips) {
    if (x >= r.left && x < r.right && y >= r.top && y < r.bottom) {
      uint32_t *p = t.s->row(y) + x;
      *p = rop2 == R2_COPYPEN ? c : rop2_apply(rop2, *p, c);
      return;
    }
  }
}

void fill_rect(Target &t, int x0, int y0, int x1, int y1, uint32_t c, int rop2)
{
  if (x0 > x1) std::swap(x0, x1);
  if (y0 > y1) std::swap(y0, y1);
  for (auto &r : t.clips) {
    int a = std::max(x0, (int)r.left), b = std::min(x1, (int)r.right);
    int top = std::max(y0, (int)r.top), bot = std::min(y1, (int)r.bottom);
    if (a >= b || top >= bot) continue;
    for (int y = top; y < bot; y++) {
      uint32_t *p = t.s->row(y) + a;
      if (rop2 == R2_COPYPEN) { for (int x = a; x < b; x++) *p++ = c; }
      else { for (int x = a; x < b; x++, p++) *p = rop2_apply(rop2, *p, c); }
    }
  }
}

void line(Target &t, int x0, int y0, int x1, int y1, uint32_t c, int rop2, bool last)
{
  /* Bresenham, GDI convention: start pixel drawn, end pixel excluded. */
  int dx = std::abs(x1 - x0), dy = std::abs(y1 - y0);
  int sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1;
  if (dx == 0 && dy == 0) { if (last) plot(t, x0, y0, c, rop2); return; }
  if (dx >= dy) {
    int err = 2 * dy - dx;
    int y = y0;
    for (int x = x0; x != x1; x += sx) {
      plot(t, x, y, c, rop2);
      if (err > 0 || (err == 0 && sx < 0)) { y += sy; err -= 2 * dx; }
      err += 2 * dy;
    }
  } else {
    int err = 2 * dx - dy;
    int x = x0;
    for (int y = y0; y != y1; y += sy) {
      plot(t, x, y, c, rop2);
      if (err > 0 || (err == 0 && sy < 0)) { x += sx; err -= 2 * dy; }
      err += 2 * dx;
    }
  }
  if (last) plot(t, x1, y1, c, rop2);
}

}  /* namespace w32 */

/* Draw with the DC's pen (cosmetic or geometric width). */
static void pen_line(Target &t, DCObj *dc, int x0, int y0, int x1, int y1)
{
  PenObj *p = dc->pen;
  if (!p || p->style == PS_NULL) return;
  uint32_t c = cref_to_px(p->color);
  if (p->width <= 1) {
    line(t, x0 + t.ox, y0 + t.oy, x1 + t.ox, y1 + t.oy, c, dc->rop2, false);
    return;
  }
  /* geometric pen: round caps and joins -> stamp discs along the line */
  int r = p->width / 2;
  int dx = std::abs(x1 - x0), dy = std::abs(y1 - y0);
  int n = std::max(dx, dy);
  for (int i = 0; i <= n; i++) {
    int x = x0 + (n ? (x1 - x0) * i / n : 0) + t.ox;
    int y = y0 + (n ? (y1 - y0) * i / n : 0) + t.oy;
    for (int yy = -r; yy <= r; yy++) {
      int span = (int)std::floor(std::sqrt((double)(r * r - yy * yy)) + 0.5);
      if (p->width % 2 == 0) hline(t, x - span, x + span, y + yy, c, dc->rop2);
      else hline(t, x - span, x + span + 1, y + yy, c, dc->rop2);
    }
  }
}

BOOL MoveToEx(HDC h, int x, int y, LPPOINT old)
{
  DCObj *dc = dc_from(h);
  if (!dc) return FALSE;
  if (old) *old = dc->cur;
  dc->cur.x = x;
  dc->cur.y = y;
  return TRUE;
}

BOOL LineTo(HDC h, int x, int y)
{
  DCObj *dc = dc_from(h);
  if (!dc) return FALSE;
  Target t;
  if (resolve_dc(dc, t)) {
    pen_line(t, dc, dc->cur.x, dc->cur.y, x, y);
    screen_dirty();
  }
  dc->cur.x = x;
  dc->cur.y = y;
  return TRUE;
}

BOOL Polyline(HDC h, const POINT *pts, int n)
{
  DCObj *dc = dc_from(h);
  if (!dc || n < 2) return FALSE;
  Target t;
  if (resolve_dc(dc, t)) {
    for (int i = 1; i < n; i++) pen_line(t, dc, pts[i-1].x, pts[i-1].y, pts[i].x, pts[i].y);
    screen_dirty();
  }
  return TRUE;
}

COLORREF SetPixel(HDC h, int x, int y, COLORREF c)
{
  DCObj *dc = dc_from(h);
  if (!dc) return (COLORREF)-1;
  Target t;
  if (resolve_dc(dc, t)) {
    plot(t, x + t.ox, y + t.oy, cref_to_px(c));
    screen_dirty();
  }
  return c;
}

COLORREF GetPixel(HDC h, int x, int y)
{
  DCObj *dc = dc_from(h);
  Target t;
  if (!dc || !resolve_dc(dc, t)) return (COLORREF)-1;
  int sx = x + t.ox, sy = y + t.oy;
  if (sx < 0 || sy < 0 || sx >= t.s->w || sy >= t.s->h) return (COLORREF)-1;
  return px_to_cref(t.s->px[(size_t)sy * t.s->w + sx]);
}

int FillRect(HDC h, const RECT *r, HBRUSH hb)
{
  DCObj *dc = dc_from(h);
  if (!dc || !r) return 0;
  BrushObj *b = nullptr;
  if ((uintptr_t)hb <= 31) {          /* (HBRUSH)(COLOR_xxx + 1) */
    uint32_t c = syscolor_px((int)(uintptr_t)hb - 1);
    Target t;
    if (resolve_dc(dc, t)) {
      fill_rect(t, r->left + t.ox, r->top + t.oy, r->right + t.ox, r->bottom + t.oy, c);
      screen_dirty();
    }
    return 1;
  }
  b = H<BrushObj>(hb, HT_BRUSH);
  if (!b) return 0;
  if (b->style == BS_NULL) return 1;
  Target t;
  if (resolve_dc(dc, t)) {
    fill_rect(t, r->left + t.ox, r->top + t.oy, r->right + t.ox, r->bottom + t.oy,
              cref_to_px(b->color));
    screen_dirty();
  }
  return 1;
}

int FrameRect(HDC h, const RECT *r, HBRUSH hb)
{
  DCObj *dc = dc_from(h);
  BrushObj *b = H<BrushObj>(hb, HT_BRUSH);
  if (!dc || !r || !b) return 0;
  Target t;
  if (resolve_dc(dc, t)) {
    uint32_t c = cref_to_px(b->color);
    int l = r->left + t.ox, tp = r->top + t.oy, rr = r->right + t.ox, bb = r->bottom + t.oy;
    fill_rect(t, l, tp, rr, tp + 1, c);
    fill_rect(t, l, bb - 1, rr, bb, c);
    fill_rect(t, l, tp, l + 1, bb, c);
    fill_rect(t, rr - 1, tp, rr, bb, c);
    screen_dirty();
  }
  return 1;
}

BOOL InvertRect(HDC h, const RECT *r)
{
  DCObj *dc = dc_from(h);
  Target t;
  if (!dc || !r || !resolve_dc(dc, t)) return FALSE;
  fill_rect(t, r->left + t.ox, r->top + t.oy, r->right + t.ox, r->bottom + t.oy, 0, R2_NOT);
  screen_dirty();
  return TRUE;
}

BOOL DrawFocusRect(HDC h, const RECT *r)
{
  DCObj *dc = dc_from(h);
  Target t;
  if (!dc || !r || !resolve_dc(dc, t)) return FALSE;
  int l = r->left + t.ox, tp = r->top + t.oy, rr = r->right + t.ox - 1, bb = r->bottom + t.oy - 1;
  for (int x = l; x <= rr; x++) {
    if (((x - l) & 1) == 0) { plot(t, x, tp, 0, R2_NOT); plot(t, x, bb, 0, R2_NOT); }
  }
  for (int y = tp + 1; y < bb; y++) {
    if (((y - tp) & 1) == 0) { plot(t, l, y, 0, R2_NOT); plot(t, rr, y, 0, R2_NOT); }
  }
  screen_dirty();
  return TRUE;
}

/* ------------------------- ellipses, arcs, pies -------------------------- */
/* Pixel classification against the ellipse inscribed in [l,r) x [t,b). */
namespace {
struct EllipseGeom {
  double cx, cy, a, b;
  EllipseGeom(int l, int t, int r, int bt) {
    cx = (l + r - 1) / 2.0; cy = (t + bt - 1) / 2.0;
    a = (r - l) / 2.0; b = (bt - t) / 2.0;
  }
  bool inside(int x, int y) const {
    if (a <= 0 || b <= 0) return false;
    double dx = (x - cx) / a, dy = (y - cy) / b;
    return dx * dx + dy * dy <= 1.0 + 1e-9;
  }
  /* scanline extent [x0,x1] inside the ellipse, false if none */
  bool span(int y, int &x0, int &x1) const {
    if (a <= 0 || b <= 0) return false;
    double dy = (y - cy) / b;
    double q = 1.0 - dy * dy;
    if (q < -1e-9) return false;
    if (q < 0) q = 0;
    double hw = a * std::sqrt(q);
    x0 = (int)std::ceil(cx - hw - 1e-9);
    x1 = (int)std::floor(cx + hw + 1e-9);
    return x0 <= x1;
  }
};

double angle_of(double dx, double dy) { return std::atan2(-dy, dx); }   /* y grows down */

bool in_sweep(double ang, double a0, double a1)
{
  /* counter-clockwise from a0 to a1 (GDI default arc direction) */
  const double two = 2 * M_PI;
  double s = std::fmod(ang - a0 + 4 * two, two);
  double e = std::fmod(a1 - a0 + 4 * two, two);
  if (e < 1e-12) e = two;   /* start == end: full ellipse */
  return s <= e + 1e-12;
}
}  /* namespace */

static void ellipse_outline(Target &t, DCObj *dc, const EllipseGeom &g, int l, int tp, int r, int b,
                            bool sweep, double a0, double a1)
{
  PenObj *p = dc->pen;
  if (!p || p->style == PS_NULL) return;
  uint32_t c = cref_to_px(p->color);
  int w = std::max(1, p->width);
  for (int y = tp; y < b; y++) {
    int x0, x1;
    if (!g.span(y, x0, x1)) continue;
    for (int x = x0; x <= x1; x++) {
      /* boundary pixel: some 4-neighbour (within w) is outside */
      bool edge = false;
      for (int k = 1; k <= w && !edge; k++)
        edge = !g.inside(x - k, y) || !g.inside(x + k, y) || !g.inside(x, y - k) || !g.inside(x, y + k);
      if (!edge) continue;
      if (sweep && !in_sweep(angle_of(x - g.cx, y - g.cy), a0, a1)) continue;
      plot(t, x + t.ox, y + t.oy, c, dc->rop2);
    }
  }
  (void)l; (void)r;
}

static void ellipse_fill(Target &t, DCObj *dc, const EllipseGeom &g, int tp, int b,
                         bool sweep, double a0, double a1)
{
  BrushObj *br = dc->brush;
  if (!br || br->style == BS_NULL) return;
  uint32_t c = cref_to_px(br->color);
  for (int y = tp; y < b; y++) {
    int x0, x1;
    if (!g.span(y, x0, x1)) continue;
    if (!sweep) { hline(t, x0 + t.ox, x1 + 1 + t.ox, y + t.oy, c, dc->rop2); continue; }
    for (int x = x0; x <= x1; x++)
      if (in_sweep(angle_of(x - g.cx, y - g.cy), a0, a1))
        plot(t, x + t.ox, y + t.oy, c, dc->rop2);
  }
}

BOOL Ellipse(HDC h, int l, int tp, int r, int b)
{
  DCObj *dc = dc_from(h);
  Target t;
  if (!dc || !resolve_dc(dc, t)) return dc != nullptr;
  if (l > r) std::swap(l, r);
  if (tp > b) std::swap(tp, b);
  EllipseGeom g(l, tp, r, b);
  ellipse_fill(t, dc, g, tp, b, false, 0, 0);
  ellipse_outline(t, dc, g, l, tp, r, b, false, 0, 0);
  screen_dirty();
  return TRUE;
}

BOOL Arc(HDC h, int l, int tp, int r, int b, int xs, int ys, int xe, int ye)
{
  DCObj *dc = dc_from(h);
  Target t;
  if (!dc || !resolve_dc(dc, t)) return dc != nullptr;
  if (l > r) std::swap(l, r);
  if (tp > b) std::swap(tp, b);
  EllipseGeom g(l, tp, r, b);
  double a0 = angle_of(xs - g.cx, ys - g.cy), a1 = angle_of(xe - g.cx, ye - g.cy);
  ellipse_outline(t, dc, g, l, tp, r, b, true, a0, a1);
  screen_dirty();
  return TRUE;
}

BOOL Pie(HDC h, int l, int tp, int r, int b, int xs, int ys, int xe, int ye)
{
  DCObj *dc = dc_from(h);
  Target t;
  if (!dc || !resolve_dc(dc, t)) return dc != nullptr;
  if (l > r) std::swap(l, r);
  if (tp > b) std::swap(tp, b);
  EllipseGeom g(l, tp, r, b);
  double a0 = angle_of(xs - g.cx, ys - g.cy), a1 = angle_of(xe - g.cx, ye - g.cy);
  ellipse_fill(t, dc, g, tp, b, true, a0, a1);
  ellipse_outline(t, dc, g, l, tp, r, b, true, a0, a1);
  /* the two radii */
  int ccx = (int)std::lround(g.cx), ccy = (int)std::lround(g.cy);
  int sx = (int)std::lround(g.cx + g.a * std::cos(a0)), sy = (int)std::lround(g.cy - g.b * std::sin(a0));
  int ex = (int)std::lround(g.cx + g.a * std::cos(a1)), ey = (int)std::lround(g.cy - g.b * std::sin(a1));
  pen_line(t, dc, ccx, ccy, sx, sy);
  pen_line(t, dc, ccx, ccy, ex, ey);
  screen_dirty();
  return TRUE;
}

BOOL Chord(HDC h, int l, int tp, int r, int b, int xs, int ys, int xe, int ye)
{
  return Pie(h, l, tp, r, b, xs, ys, xe, ye);
}

BOOL Rectangle(HDC h, int l, int tp, int r, int b)
{
  DCObj *dc = dc_from(h);
  Target t;
  if (!dc || !resolve_dc(dc, t)) return dc != nullptr;
  if (l > r) std::swap(l, r);
  if (tp > b) std::swap(tp, b);
  if (dc->brush && dc->brush->style != BS_NULL)
    fill_rect(t, l + t.ox, tp + t.oy, r + t.ox, b + t.oy, cref_to_px(dc->brush->color), dc->rop2);
  if (dc->pen && dc->pen->style != PS_NULL) {
    uint32_t c = cref_to_px(dc->pen->color);
    int w = std::max(1, dc->pen->width);
    fill_rect(t, l + t.ox, tp + t.oy, r + t.ox, tp + w + t.oy, c, dc->rop2);
    fill_rect(t, l + t.ox, b - w + t.oy, r + t.ox, b + t.oy, c, dc->rop2);
    fill_rect(t, l + t.ox, tp + w + t.oy, l + w + t.ox, b - w + t.oy, c, dc->rop2);
    fill_rect(t, r - w + t.ox, tp + w + t.oy, r + t.ox, b - w + t.oy, c, dc->rop2);
  }
  screen_dirty();
  return TRUE;
}

BOOL RoundRect(HDC h, int l, int tp, int r, int b, int ew, int eh)
{
  DCObj *dc = dc_from(h);
  Target t;
  if (!dc || !resolve_dc(dc, t)) return dc != nullptr;
  if (l > r) std::swap(l, r);
  if (tp > b) std::swap(tp, b);
  ew = std::min(ew, r - l);
  eh = std::min(eh, b - tp);
  if (ew < 2 || eh < 2) { screen_dirty(); return Rectangle(h, l, tp, r, b); }
  /* corner ellipses of size ew x eh */
  EllipseGeom tl(l, tp, l + ew, tp + eh), tr(r - ew, tp, r, tp + eh);
  EllipseGeom bl(l, b - eh, l + ew, b), brr(r - ew, b - eh, r, b);
  auto inside = [&](int x, int y) -> bool {
    if (x < l || x >= r || y < tp || y >= b) return false;
    if (x < l + ew / 2 && y < tp + eh / 2) return tl.inside(x, y);
    if (x >= r - ew / 2 && y < tp + eh / 2) return tr.inside(x, y);
    if (x < l + ew / 2 && y >= b - eh / 2) return bl.inside(x, y);
    if (x >= r - ew / 2 && y >= b - eh / 2) return brr.inside(x, y);
    return true;
  };
  bool fill = dc->brush && dc->brush->style != BS_NULL;
  bool pen = dc->pen && dc->pen->style != PS_NULL;
  uint32_t fc = fill ? cref_to_px(dc->brush->color) : 0;
  uint32_t pc = pen ? cref_to_px(dc->pen->color) : 0;
  for (int y = tp; y < b; y++)
    for (int x = l; x < r; x++) {
      if (!inside(x, y)) continue;
      bool edge = !inside(x - 1, y) || !inside(x + 1, y) || !inside(x, y - 1) || !inside(x, y + 1);
      if (edge && pen) plot(t, x + t.ox, y + t.oy, pc, dc->rop2);
      else if (fill) plot(t, x + t.ox, y + t.oy, fc, dc->rop2);
    }
  screen_dirty();
  return TRUE;
}

BOOL Polygon(HDC h, const POINT *pts, int n)
{
  DCObj *dc = dc_from(h);
  Target t;
  if (!dc || n < 2 || !resolve_dc(dc, t)) return dc != nullptr;
  if (dc->brush && dc->brush->style != BS_NULL && n >= 3) {
    /* even-odd scanline fill (ALTERNATE, the default fill mode) */
    int ymin = pts[0].y, ymax = pts[0].y;
    for (int i = 1; i < n; i++) { ymin = std::min(ymin, (int)pts[i].y); ymax = std::max(ymax, (int)pts[i].y); }
    uint32_t c = cref_to_px(dc->brush->color);
    for (int y = ymin; y < ymax; y++) {
      std::vector<double> xs;
      double yc = y + 0.5;
      for (int i = 0; i < n; i++) {
        const POINT &a = pts[i], &b = pts[(i + 1) % n];
        if ((a.y <= yc && b.y > yc) || (b.y <= yc && a.y > yc))
          xs.push_back(a.x + (yc - a.y) * (double)(b.x - a.x) / (double)(b.y - a.y));
      }
      std::sort(xs.begin(), xs.end());
      for (size_t k = 0; k + 1 < xs.size(); k += 2)
        hline(t, (int)std::ceil(xs[k] - 0.5) + t.ox, (int)std::ceil(xs[k + 1] - 0.5) + t.ox, y + t.oy, c, dc->rop2);
    }
  }
  for (int i = 0; i < n; i++)
    pen_line(t, dc, pts[i].x, pts[i].y, pts[(i + 1) % n].x, pts[(i + 1) % n].y);
  screen_dirty();
  return TRUE;
}

/* ============================== bit blitting ============================= */
/* Generic ternary raster operation on 24-bit pixels. */
static inline uint32_t rop3(uint8_t idx, uint32_t d, uint32_t s, uint32_t p)
{
  switch (idx) {
  case 0xCC: return s;
  case 0x88: return d & s;
  case 0x66: return d ^ s;
  case 0xEE: return d | s;
  case 0x22: return d & ~s & 0xFFFFFF;
  case 0x33: return ~s & 0xFFFFFF;
  case 0x44: return s & ~d & 0xFFFFFF;
  case 0x11: return ~(d | s) & 0xFFFFFF;
  case 0xBB: return (d | ~s) & 0xFFFFFF;
  case 0x00: return 0;
  case 0xFF: return 0xFFFFFF;
  case 0xF0: return p;
  case 0x5A: return d ^ p;
  case 0x55: return ~d & 0xFFFFFF;
  case 0xAA: return d;
  case 0xC0: return p & s;
  default: {
    uint32_t r = 0;
    for (int m = 0; m < 8; m++) {
      if (!((idx >> m) & 1)) continue;
      uint32_t pp = (m & 4) ? p : ~p;
      uint32_t ss = (m & 2) ? s : ~s;
      uint32_t dd = (m & 1) ? d : ~d;
      r |= pp & ss & dd;
    }
    return r & 0xFFFFFF;
  }
  }
}

static inline bool rop_uses_src(uint8_t idx) { return ((idx >> 2) & 0x33) != (idx & 0x33); }

/* Core blit: dst rect (dx,dy,dw,dh) in surface coordinates of t, source
   pixels fetched by srcpx(sx,sy) over (sx,sy,sw,sh); nearest-neighbour
   stretching like COLORONCOLOR. Negative extents mirror. */
template <class F>
static void blit_core(Target &t, int dx, int dy, int dw, int dh,
                      int sx, int sy, int sw, int sh, uint8_t idx, uint32_t pat,
                      int srcw, int srch, F srcpx)
{
  bool mirrorX = (dw < 0) != (sw < 0);
  bool mirrorY = (dh < 0) != (sh < 0);
  if (dw < 0) { dx += dw + 1; dw = -dw; }
  if (dh < 0) { dy += dh + 1; dh = -dh; }
  if (sw < 0) { sx += sw + 1; sw = -sw; }
  if (sh < 0) { sy += sh + 1; sh = -sh; }
  if (dw == 0 || dh == 0) return;
  bool src = rop_uses_src(idx);
  bool straight = (dw == sw && dh == sh && !mirrorX && !mirrorY);
  for (auto &c : t.clips) {
    int x0 = std::max(dx, (int)c.left), x1 = std::min(dx + dw, (int)c.right);
    int y0 = std::max(dy, (int)c.top), y1 = std::min(dy + dh, (int)c.bottom);
    if (x0 >= x1 || y0 >= y1) continue;
    for (int y = y0; y < y1; y++) {
      int ry = y - dy;
      int syy = straight ? sy + ry : sy + (int)((int64_t)(mirrorY ? dh - 1 - ry : ry) * sh / dh);
      uint32_t *d = t.s->row(y);
      if (src && (syy < 0 || syy >= srch)) continue;   /* outside the source: unchanged */
      for (int x = x0; x < x1; x++) {
        int rx = x - dx;
        uint32_t s = 0;
        if (src) {
          int sxx = straight ? sx + rx : sx + (int)((int64_t)(mirrorX ? dw - 1 - rx : rx) * sw / dw);
          if (sxx < 0 || sxx >= srcw) continue;
          s = srcpx(sxx, syy);
        }
        d[x] = idx == 0xCC ? s : rop3(idx, d[x], s, pat);
      }
    }
  }
}

static uint32_t dc_pattern(DCObj *dc)
{
  return (dc && dc->brush && dc->brush->style != BS_NULL) ? cref_to_px(dc->brush->color) : 0;
}

static BOOL do_blt(HDC hd, int x, int y, int w, int h, HDC hs, int sx, int sy, int sw, int sh, DWORD rop)
{
  DCObj *dd = dc_from(hd);
  if (!dd) return FALSE;
  uint8_t idx = (uint8_t)((rop >> 16) & 0xFF);
  Target td;
  if (!resolve_dc(dd, td)) return TRUE;   /* invisible destination */
  uint32_t pat = dc_pattern(dd);
  if (!rop_uses_src(idx)) {
    blit_core(td, x + td.ox, y + td.oy, w, h, 0, 0, w, h, idx, pat, 1, 1,
              [](int, int) -> uint32_t { return 0; });
    screen_dirty();
    return TRUE;
  }
  DCObj *ds = dc_from(hs);
  if (!ds) return FALSE;
  Target tsrc;
  if (ds->kind == DCObj::MEM) {
    if (!ds->bmp || !ds->bmp->surf) return FALSE;
    tsrc.hold = ds->bmp->surf;
    tsrc.s = tsrc.hold.get();
  } else if (!resolve_dc(ds, tsrc)) {
    return TRUE;
  }
  Surface *S = tsrc.s;
  int sox = tsrc.ox, soy = tsrc.oy;
  /* monochrome source into colour destination: 0 -> text colour, 1 -> bk */
  bool monoSrc = ds->kind == DCObj::MEM && ds->bmp && ds->bmp->mono && !dc_is_mono(dd);
  uint32_t fg = cref_to_px(dd->text), bg = cref_to_px(dd->bk);
  bool monoDst = dc_is_mono(dd) && !(ds->bmp && ds->bmp->mono);
  uint32_t srcBk = cref_to_px(ds->bk);
  if (S == td.s) {
    /* same surface: copy the source first to handle overlap */
    std::shared_ptr<Surface> copy = std::make_shared<Surface>(*S);
    tsrc.hold = copy;
    S = copy.get();
  }
  auto fetch = [&](int px, int py) -> uint32_t {
    uint32_t v = S->px[(size_t)py * S->w + px];
    if (monoSrc) return v ? bg : fg;
    if (monoDst) return v == srcBk ? 0xFFFFFF : 0;
    return v;
  };
  blit_core(td, x + td.ox, y + td.oy, w, h, sx + sox, sy + soy, sw, sh, idx, pat,
            S->w, S->h, fetch);
  screen_dirty();
  return TRUE;
}

BOOL BitBlt(HDC hd, int x, int y, int w, int h, HDC hs, int sx, int sy, DWORD rop)
{
  return do_blt(hd, x, y, w, h, hs, sx, sy, w, h, rop);
}

BOOL StretchBlt(HDC hd, int x, int y, int w, int h, HDC hs, int sx, int sy, int sw, int sh, DWORD rop)
{
  return do_blt(hd, x, y, w, h, hs, sx, sy, sw, sh, rop);
}

BOOL PatBlt(HDC hd, int x, int y, int w, int h, DWORD rop)
{
  return do_blt(hd, x, y, w, h, nullptr, 0, 0, w, h, rop);
}

int StretchDIBits(HDC hd, int x, int y, int w, int h, int sx, int sy, int sw, int sh,
                  const void *bits, const BITMAPINFO *bi, UINT usage, DWORD rop)
{
  DCObj *dd = dc_from(hd);
  if (!dd || !bi) return 0;
  Target td;
  if (!resolve_dc(dd, td)) return h;
  std::vector<uint32_t> px;
  int bw, bh;
  if (!decode_dib(bi, bits, usage, dd, px, bw, bh)) return 0;
  /* DIB source coordinates have their origin at the lower-left corner for
     bottom-up DIBs: source rows sy..sy+sh counted from the bottom. */
  int top = (bi->bmiHeader.biHeight > 0) ? (bh - sy - sh) : sy;
  uint8_t idx = (uint8_t)((rop >> 16) & 0xFF);
  blit_core(td, x + td.ox, y + td.oy, w, h, sx, top, sw, sh, idx, dc_pattern(dd), bw, bh,
            [&](int px_, int py_) -> uint32_t { return px[(size_t)py_ * bw + px_]; });
  screen_dirty();
  return h;
}

int SetDIBitsToDevice(HDC hd, int x, int y, DWORD w, DWORD h, int sx, int sy, UINT,
                      UINT, const void *bits, const BITMAPINFO *bi, UINT usage)
{
  return StretchDIBits(hd, x, y, (int)w, (int)h, sx, sy, (int)w, (int)h, bits, bi, usage, SRCCOPY);
}
