/* ************************************************************************* */
/* w32_internal.h -- private data structures of the Win32 emulation layer.   */
/*                                                                           */
/* NEW FILE, added 2026-10-12 for the Stellar Frontier Godot port.           */
/* ************************************************************************* */
#ifndef W32_INTERNAL_H
#define W32_INTERNAL_H

#include "sfwin.h"
#include "sf_host.h"

#undef min
#undef max
#undef rand
#undef srand
#undef clock
#undef printf
#undef fprintf
#undef sprintf
#undef snprintf
#undef vsprintf
#undef vsnprintf
#undef vfprintf
#undef sscanf
#undef fscanf
#undef fopen

#include <mutex>
#include <condition_variable>
#include <memory>
#include <vector>
#include <deque>
#include <string>
#include <atomic>
#include <functional>
#include <algorithm>

/* The program's own global objects (e.g. SF_Client.cpp's S_Event
   refreshEvent) call Win32 functions from their constructors, as they could
   on Windows where the system is always initialised first.  The emulation's
   own globals are therefore constructed before any other C++ global. */
#define W32_EARLY __attribute__((init_priority(101)))

namespace w32 {

/* ================================ handles ================================ */
enum HType : uint8_t {
  HT_NONE, HT_WND, HT_MENU, HT_DC, HT_PEN, HT_BRUSH, HT_FONT, HT_BITMAP,
  HT_PALETTE, HT_RGN, HT_ICON, HT_THREAD, HT_MUTEX, HT_EVENT, HT_PROCESS,
  HT_FILE, HT_FIND, HT_GLOBAL, HT_MODULE, HT_TOKEN
};

struct Obj {
  HType type = HT_NONE;
  uintptr_t handle = 0;
  virtual ~Obj() {}
};

uintptr_t hnew(Obj *o, HType t);
Obj      *hget(const void *h, HType t);   /* NULL if not a live handle of type t */
Obj      *hget_any(const void *h);
void      hdel(const void *h);
template <class T> inline T *H(const void *h, HType t) { return static_cast<T *>(hget(h, t)); }
#define HV(x) ((void *)(uintptr_t)(x))

/* ============================ threads (kernel) =========================== */
DWORD    cur_tid();
void     check_terminated();     /* throws if TerminateThread() was called on us */
struct ThreadTerminated {};

/* ================================ surfaces =============================== */
struct Surface {
  int w = 0, h = 0;
  std::vector<uint32_t> px;           /* 0x00RRGGBB, row-major, top-down */
  void resize(int nw, int nh, uint32_t fill, bool keep);
  uint32_t *row(int y) { return &px[(size_t)y * w]; }
};
typedef std::shared_ptr<Surface> SurfaceP;

extern std::atomic<uint32_t> g_screen_serial;   /* bumped when the screen may have changed */
inline void screen_dirty() { g_screen_serial.fetch_add(1, std::memory_order_relaxed); }

inline uint32_t cref_to_px(COLORREF c) {
  return ((c & 0xFF) << 16) | (c & 0xFF00) | ((c >> 16) & 0xFF);
}
inline COLORREF px_to_cref(uint32_t p) {
  return ((p >> 16) & 0xFF) | (p & 0xFF00) | ((p & 0xFF) << 16);
}

/* ================================== GDI ================================== */
struct GdiObj : Obj {
  int  selected = 0;
  bool stock = false;
};
struct PenObj : GdiObj { int style = PS_SOLID; int width = 1; COLORREF color = 0; };
struct BrushObj : GdiObj { int style = BS_SOLID; COLORREF color = 0; };
struct SfGlyph { int16_t adv, x, y, w, h; uint32_t off; };
struct SfFontData {
  const char *family;
  int req_height, req_width, weight;
  int height, ascent, descent, intlead, avgw, maxw, ppem, pitchfamily;
  const SfGlyph *glyphs;
  const unsigned char *bits;
};
struct FontObj : GdiObj { LOGFONT lf; const SfFontData *fd = nullptr; };
struct BitmapObj : GdiObj {
  SurfaceP surf;
  int bpp = 32;                 /* what GetObject/GetDIBits report */
  bool mono = false;            /* 1-bpp DDB: pixels are 0 or 0xFFFFFF */
};
struct PaletteObj : GdiObj { std::vector<PALETTEENTRY> entries; };
struct RgnObj : GdiObj { std::vector<RECT> rects; };

struct DCObj : Obj {
  enum Kind { MEM, WIN, SCREEN, INFO } kind = MEM;
  HWND hwnd = 0;
  bool client = true;           /* GetDC (client) vs GetWindowDC */
  bool paint = false;           /* BeginPaint DC: clipped to paintRect */
  RECT paintRect = {0,0,0,0};   /* client coordinates */
  BitmapObj  *bmp = nullptr;
  PenObj     *pen = nullptr;
  BrushObj   *brush = nullptr;
  FontObj    *font = nullptr;
  PaletteObj *pal = nullptr;
  COLORREF text = 0, bk = 0xFFFFFF;
  int bkMode = OPAQUE;
  UINT align = TA_TOP | TA_LEFT;
  int rop2 = R2_COPYPEN;
  int stretch = BLACKONWHITE;
  POINT cur = {0, 0};
  bool hasClip = false;
  std::vector<RECT> clip;       /* DC (logical) coordinates */
  std::vector<int> saved;       /* SaveDC stack (unused mostly) */
};

/* A resolved drawing target: surface + origin + clip rectangles. */
struct Target {
  Surface *s = nullptr;
  SurfaceP hold;
  int ox = 0, oy = 0;           /* DC coordinate (0,0) maps to surface (ox,oy) */
  std::vector<RECT> clips;      /* surface coordinates, non-overlapping */
  bool valid() const { return s && !clips.empty(); }
};
bool resolve_dc(DCObj *dc, Target &t);
DCObj *dc_from(HDC h);
bool   dib_decode(const BITMAPINFO *bi, const void *bits, std::vector<uint32_t> &out, int &w, int &h);
HDC    dc_new_window(HWND hwnd, bool client, bool paint, const RECT *paintRect);
HDC    dc_new_screen();
void   dc_free(HDC h);
void   icon_draw_small(Target &t, HICON ic, int x, int y);
extern std::atomic<int> g_cursor_shape;
void   gdi_init();
const SfFontData *font_match(const LOGFONT &lf);
const SfFontData *font_default_gui();   /* MS Sans Serif / Tahoma 8 substitute */
const SfFontData *font_default_gui_bold();
void   text_draw(Target &t, const SfFontData *fd, int x, int y, const char *s,
                 int n, uint32_t color);           /* x,y: top-left of the cell */
int    text_width(const SfFontData *fd, const char *s, int n);

/* Raster helpers, all in surface coordinates with clipping. */
void fill_rect(Target &t, int x0, int y0, int x1, int y1, uint32_t c, int rop2 = R2_COPYPEN);
void plot(Target &t, int x, int y, uint32_t c, int rop2 = R2_COPYPEN);
void hline(Target &t, int x0, int x1, int y, uint32_t c, int rop2 = R2_COPYPEN);
void line(Target &t, int x0, int y0, int x1, int y1, uint32_t c, int rop2, bool lastPixel);

/* ================================== USER ================================= */
struct WndClass {
  std::string name;
  UINT style = 0;
  WNDPROC proc = nullptr;
  HBRUSH bg = 0;
  HICON icon = 0;
  HCURSOR cursor = 0;
  bool builtin = false;
};

struct ScrollData {
  int  min = 0, max = 100, page = 0, pos = 0, track = 0;
  bool shown = false;           /* standard (WS_VSCROLL/WS_HSCROLL) bar visible */
  bool enabled = true;
};

struct Wnd : Obj {
  HWND hwnd = 0;
  WndClass *cls = nullptr;
  WNDPROC proc = nullptr;
  DWORD style = 0, exstyle = 0;
  Wnd *parent = nullptr;        /* WS_CHILD windows */
  Wnd *owner = nullptr;         /* owned top-level windows */
  std::vector<Wnd *> children;  /* z-order, [0] = top */
  RECT rc = {0,0,0,0};          /* window rect: screen (top-level) or parent client coords */
  RECT client = {0,0,0,0};      /* client rect relative to the window's top-left */
  std::string text;
  LONG_PTR user = 0;
  UINT_PTR id = 0;
  HMENU menu = 0;
  HFONT font = 0;
  DWORD tid = 0;
  bool destroying = false;
  bool minimized = false, maximized = false;
  bool restoreToMax = false;      /* minimised from the maximised state */
  RECT rcNormal = {0,0,0,0};
  bool hasUpdate = false, erase = false;
  RECT upd = {0,0,0,0};         /* client coordinates */
  bool active = false;          /* caption drawn active */
  SurfaceP surf;
  ScrollData sbV, sbH;          /* standard scroll bars and SCROLLBAR controls */
  void *ctl = nullptr;          /* control-class private data */
  void (*ctl_free)(void *) = nullptr;
  HICON icon = 0;
  int  nchit_pressed = 0;       /* caption button being pressed */
  int  menu_hot = -1;           /* highlighted menu bar item */
  bool sentSizeMove = false;
  bool visible() const { return (style & WS_VISIBLE) != 0; }
  bool enabled() const { return (style & WS_DISABLED) == 0; }
  bool is_child() const { return (style & WS_CHILD) != 0; }
};

extern std::recursive_mutex g_wm;   /* protects every USER structure */
extern std::condition_variable_any g_wm_cv;
extern std::vector<Wnd *> g_top;    /* top-level windows, [0] = top of the z-order */
extern int g_screen_w, g_screen_h;
extern HWND g_focus, g_active, g_capture;
extern POINT g_mouse;
extern uint8_t g_keys[256];         /* bit 7 = down, bit 0 = toggled */

Wnd *wnd(HWND h);                   /* live window or NULL (takes no lock) */
LRESULT call_proc(Wnd *w, UINT m, WPARAM wp, LPARAM lp);   /* same-thread dispatch */
void   wnd_invalidate(Wnd *w, const RECT *r, bool erase); /* lock held */
void   wnd_screen_origin(Wnd *w, int *x, int *y);         /* window top-left in screen */
void   wnd_client_origin(Wnd *w, int *x, int *y);         /* client top-left in screen */
bool   wnd_is_visible_chain(Wnd *w);
void   nc_paint(Wnd *w);                                  /* lock held */
void   nc_calc(Wnd *w, DWORD style, DWORD exstyle, bool hasMenu, int cx, int cy, RECT *client);
int    nc_hittest(Wnd *w, int sx, int sy);
int    nc_border(DWORD style, DWORD exstyle);       /* frame width, no client edge */
void   host_notify_minimized();
Wnd   *window_at(int sx, int sy, bool includeDisabledChildren);
void   user_init();

void   post_msg(Wnd *w, UINT m, WPARAM wp, LPARAM lp, DWORD extra = 0);
void   wake_all();
LRESULT send_msg(HWND h, UINT m, WPARAM wp, LPARAM lp);

/* Controls (w32_controls.cpp) */
void controls_register();
LRESULT CALLBACK button_proc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK static_proc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK edit_proc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK listbox_proc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK combobox_proc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK scrollbar_proc(HWND, UINT, WPARAM, LPARAM);

/* Menus (w32_menu.cpp) */
struct MenuItem {
  UINT type = MFT_STRING;
  UINT state = 0;
  UINT_PTR id = 0;
  HMENU sub = 0;
  std::string text;
  ULONG_PTR data = 0;
};
struct MenuObj : Obj {
  bool popup = false;
  std::vector<MenuItem> items;
};
void menubar_paint(Wnd *w, Target &t, int x, int y, int cx);   /* lock held */
int  menubar_height(Wnd *w, int cx);
int  menubar_item_at(Wnd *w, int sx, int sy, RECT *itemScreen);
void menu_track_bar(Wnd *w, int startItem, bool keyboard);
BOOL menu_track_popup(HMENU m, UINT flags, int x, int y, HWND owner);

/* Scroll bars (w32_nc.cpp), shared by standard bars and SCROLLBAR controls */
void sb_draw(Target &t, RECT r, bool vert, const ScrollData &sd, bool enabled, int pressedPart);
int  sb_hit(HWND h, bool ctl, bool vert, int sx, int sy);
void sb_track(HWND h, bool ctl, bool vert, int startPart);

/* Drawing helpers shared by NC painting, menus and controls. */
uint32_t syscolor_px(int idx);
void draw_edge(Target &t, RECT r, UINT edge, UINT flags);
void draw_3d_rect(Target &t, int x0, int y0, int x1, int y1, uint32_t tl, uint32_t br);
void draw_text_in(Target &t, const SfFontData *fd, const char *s, RECT r, UINT dtflags, uint32_t color);
void draw_check(Target &t, int x, int y, uint32_t c);
void draw_arrow(Target &t, int cx, int cy, int dir, uint32_t c, int size);

/* System metrics (Windows 2000/XP "Windows Classic" at 96 dpi). */
enum {
  M_CYCAPTION = 19, M_CXFRAME = 4, M_CXDLGFRAME = 3, M_CXBORDER = 1,
  M_CXEDGE = 2, M_CYMENU = 19, M_CXVSCROLL = 16, M_CYHSCROLL = 16,
  M_CXSIZE = 18, M_CYSIZE = 18, M_CXSMICON = 16
};

/* Host glue (w32_host.cpp) */
void host_log(const char *fmt, ...);
/* Put the x87 FPU in the state the Visual C++ runtime gives every thread:
   53-bit precision, round to nearest, all exceptions masked (0x027F). */
void fpu_msvc_default();
/* Debug tracing, enabled by the environment variable SF_W32_TRACE (a list of
   letters: a = activation/focus, m = messages sent, w = window creation). */
bool trace(char what);
std::string host_install_dir();
std::string host_exe_path();
const char *host_exe_name();
std::string host_zap_path();

/* Audio (w32_audio.cpp) */
HRESULT dsound_create(LPDIRECTSOUND *out);

}  /* namespace w32 */

#endif
