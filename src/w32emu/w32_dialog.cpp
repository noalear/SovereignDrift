/* ************************************************************************* */
/* w32_dialog.cpp -- the dialog manager's keyboard interface                */
/* (IsDialogMessage: Tab, Enter, Esc, arrows) and MessageBox.               */
/*                                                                           */
/* NEW FILE, added 2026-10-12 for the Stellar Frontier Godot port.           */
/* ************************************************************************* */
#include "w32_internal.h"
#include <cstring>

using namespace w32;

/* =========================== IsDialogMessage ============================ */
static bool tab_stop(HWND h)
{
  Wnd *w = wnd(h);
  return w && w->visible() && w->enabled() && (w->style & WS_TABSTOP);
}

static HWND next_tab(HWND dlg, HWND from, bool prev)
{
  std::vector<HWND> kids;
  {
    std::lock_guard<std::recursive_mutex> lk(g_wm);
    Wnd *d = wnd(dlg);
    if (!d) return 0;
    for (Wnd *c : d->children) kids.push_back(c->hwnd);
  }
  if (kids.empty()) return 0;
  int n = (int)kids.size();
  int i = -1;
  for (int k = 0; k < n; k++) if (kids[k] == from) i = k;
  for (int step = 1; step <= n; step++) {
    int j = prev ? ((i < 0 ? 0 : i) - step + 2 * n) % n : (i + step + n) % n;
    if (tab_stop(kids[j])) return kids[j];
  }
  return 0;
}

static HWND next_in_group(HWND dlg, HWND from, bool prev)
{
  std::vector<HWND> kids;
  {
    std::lock_guard<std::recursive_mutex> lk(g_wm);
    Wnd *d = wnd(dlg);
    if (!d) return 0;
    for (Wnd *c : d->children) kids.push_back(c->hwnd);
  }
  int n = (int)kids.size(), i = -1;
  for (int k = 0; k < n; k++) if (kids[k] == from) i = k;
  if (i < 0) return 0;
  /* group = from the previous WS_GROUP window up to the next one */
  int s = i;
  while (s > 0 && !(GetWindowLong(kids[s], GWL_STYLE) & WS_GROUP)) s--;
  int e = i + 1;
  while (e < n && !(GetWindowLong(kids[e], GWL_STYLE) & WS_GROUP)) e++;
  int len = e - s;
  for (int step = 1; step < len; step++) {
    int j = s + ((i - s) + (prev ? -step : step) + 2 * len) % len;
    Wnd *w = wnd(kids[j]);
    if (w && w->visible() && w->enabled() && !(SendMessage(kids[j], WM_GETDLGCODE, 0, 0) & DLGC_STATIC))
      return kids[j];
  }
  return 0;
}

static void set_dlg_focus(HWND h)
{
  if (!h) return;
  SetFocus(h);
  if (SendMessage(h, WM_GETDLGCODE, 0, 0) & DLGC_HASSETSEL)
    SendMessage(h, EM_SETSEL, 0, -1);
}

BOOL IsDialogMessage(HWND dlg, LPMSG msg)
{
  if (!msg || !wnd(dlg)) return FALSE;
  if (msg->hwnd != dlg && !IsChild(dlg, msg->hwnd)) return FALSE;
  if (msg->message == WM_KEYDOWN) {
    LRESULT code = SendMessage(msg->hwnd, WM_GETDLGCODE, msg->wParam, (LPARAM)msg);
    if (!(code & DLGC_WANTMESSAGE)) {
      switch (msg->wParam) {
      case VK_TAB:
        if (code & (DLGC_WANTTAB | DLGC_WANTALLKEYS)) break;
        set_dlg_focus(next_tab(dlg, GetFocus(), (GetKeyState(VK_SHIFT) & 0x8000) != 0));
        return TRUE;
      case VK_RETURN: {
        if (code & DLGC_WANTALLKEYS) break;
        HWND f = GetFocus();
        if (f && IsChild(dlg, f) && (SendMessage(f, WM_GETDLGCODE, 0, 0) & DLGC_DEFPUSHBUTTON)) {
          SendMessage(dlg, WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(f), BN_CLICKED), (LPARAM)f);
        } else {
          LRESULT d = SendMessage(dlg, DM_GETDEFID, 0, 0);
          UINT id = (HIWORD(d) == DC_HASDEFID) ? LOWORD(d) : IDOK;
          HWND b = GetDlgItem(dlg, id);
          if (!b || IsWindowEnabled(b))
            SendMessage(dlg, WM_COMMAND, MAKEWPARAM(id, BN_CLICKED), (LPARAM)b);
        }
        return TRUE;
      }
      case VK_ESCAPE:
        if (code & DLGC_WANTALLKEYS) break;
        SendMessage(dlg, WM_COMMAND, MAKEWPARAM(IDCANCEL, BN_CLICKED), (LPARAM)GetDlgItem(dlg, IDCANCEL));
        return TRUE;
      case VK_LEFT: case VK_UP: case VK_RIGHT: case VK_DOWN:
        if (code & (DLGC_WANTARROWS | DLGC_WANTALLKEYS)) break;
        if (HWND n = next_in_group(dlg, GetFocus(), msg->wParam == VK_LEFT || msg->wParam == VK_UP)) {
          set_dlg_focus(n);
          if (SendMessage(n, WM_GETDLGCODE, 0, 0) & DLGC_RADIOBUTTON) SendMessage(n, BM_CLICK, 0, 0);
        }
        return TRUE;
      default:
        break;
      }
    }
  } else if (msg->message == WM_CHAR) {
    LRESULT code = SendMessage(msg->hwnd, WM_GETDLGCODE, msg->wParam, (LPARAM)msg);
    if ((msg->wParam == '\t' && !(code & (DLGC_WANTTAB | DLGC_WANTALLKEYS))) ||
        ((msg->wParam == '\r' || msg->wParam == 27) && !(code & DLGC_WANTALLKEYS)))
      return TRUE;
  }
  TranslateMessage(msg);
  DispatchMessage(msg);
  return TRUE;
}

/* ================================ MessageBox ============================== */
namespace {
struct BoxState {
  UINT type = 0;
  int result = 0;
  bool done = false;
};

const SfFontData *box_font()
{
  LOGFONT lf;
  memset(&lf, 0, sizeof(lf));
  lf.lfHeight = 13;
  strcpy(lf.lfFaceName, "Tahoma");
  return font_match(lf);
}
}  /* namespace */

namespace w32 {
LRESULT CALLBACK msgbox_proc(HWND h, UINT m, WPARAM wp, LPARAM lp)
{
  Wnd *w = wnd(h);
  BoxState *s = w ? (BoxState *)w->ctl : nullptr;
  switch (m) {
  case WM_COMMAND: {
    if (!s) return 0;
    UINT id = LOWORD(wp);
    UINT kind = s->type & MB_TYPEMASK;
    if (id == IDCANCEL) {
      if (kind == MB_OK) id = IDOK;
      else if (kind == MB_YESNO || kind == MB_ABORTRETRYIGNORE) return 0;   /* no cancel */
    }
    s->result = (int)id;
    s->done = true;
    return 0;
  }
  case WM_CLOSE:
    if (s) {
      UINT kind = s->type & MB_TYPEMASK;
      if (kind == MB_YESNO || kind == MB_ABORTRETRYIGNORE) return 0;
      s->result = kind == MB_OK ? IDOK : IDCANCEL;
      s->done = true;
    }
    return 0;
  default:
    return DefWindowProc(h, m, wp, lp);
  }
}
}  /* namespace w32 */

int MessageBox(HWND owner, LPCSTR text, LPCSTR caption, UINT type)
{
  user_init();
  if (!text) text = "";
  if (!caption) caption = "Error";
  HWND top = owner;
  {
    std::lock_guard<std::recursive_mutex> lk(g_wm);
    Wnd *o = wnd(owner);
    while (o && o->parent) o = o->parent;
    top = o ? o->hwnd : 0;
  }
  const SfFontData *fd = box_font();
  /* text extent with word wrap at about 5/8 of the screen */
  HDC sdc = GetDC(0);
  HFONT f;
  {
    LOGFONT lf;
    memset(&lf, 0, sizeof(lf));
    lf.lfHeight = 13;
    strcpy(lf.lfFaceName, "Tahoma");
    f = CreateFontIndirect(&lf);
  }
  HGDIOBJ of = SelectObject(sdc, f);
  RECT tr = {0, 0, g_screen_w * 5 / 8, 0};
  DrawText(sdc, text, -1, &tr, DT_CALCRECT | DT_WORDBREAK | DT_NOPREFIX | DT_EXPANDTABS);
  SelectObject(sdc, of);
  ReleaseDC(0, sdc);

  UINT kind = type & MB_TYPEMASK;
  std::vector<std::pair<int, const char *>> buttons;
  switch (kind) {
  case MB_OKCANCEL: buttons = {{IDOK, "OK"}, {IDCANCEL, "Cancel"}}; break;
  case MB_YESNO: buttons = {{IDYES, "&Yes"}, {IDNO, "&No"}}; break;
  case MB_YESNOCANCEL: buttons = {{IDYES, "&Yes"}, {IDNO, "&No"}, {IDCANCEL, "Cancel"}}; break;
  case MB_RETRYCANCEL: buttons = {{IDRETRY, "&Retry"}, {IDCANCEL, "Cancel"}}; break;
  case MB_ABORTRETRYIGNORE: buttons = {{IDABORT, "&Abort"}, {IDRETRY, "&Retry"}, {IDIGNORE, "&Ignore"}}; break;
  default: buttons = {{IDOK, "OK"}}; break;
  }
  const int BW = 75, BH = 23, GAP = 6, M = 12;
  int nb = (int)buttons.size();
  int btnsW = nb * BW + (nb - 1) * GAP;
  int textW = tr.right - tr.left, textH = std::max((int)(tr.bottom - tr.top), fd->height);
  int capW = text_width(font_default_gui_bold(), caption, (int)strlen(caption)) + 40;
  int cw = std::max(std::max(textW + 2 * M + 6, btnsW + 2 * M), capW);
  int ch = M + textH + 18 + BH + M;
  DWORD style = WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_CLIPSIBLINGS;
  DWORD ex = WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE | WS_EX_TOPMOST;
  RECT wr = {0, 0, cw, ch};
  AdjustWindowRectEx(&wr, style, FALSE, ex);
  int ww = wr.right - wr.left, wh = wr.bottom - wr.top;
  RECT ref = {0, 0, g_screen_w, g_screen_h};
  if (top) GetWindowRect(top, &ref);
  int x = ref.left + ((ref.right - ref.left) - ww) / 2;
  int y = ref.top + ((ref.bottom - ref.top) - wh) / 2;
  x = std::max(0, std::min(x, g_screen_w - ww));
  y = std::max(0, std::min(y, g_screen_h - wh));

  bool disabledOwner = false;
  if (top && IsWindowEnabled(top)) { EnableWindow(top, FALSE); disabledOwner = true; }

  HWND box = CreateWindowEx(ex, "#32770", caption, style, x, y, ww, wh, top, 0, 0, nullptr);
  if (!box) {
    if (disabledOwner) EnableWindow(top, TRUE);
    DeleteObject(f);
    return 0;
  }
  BoxState state;
  state.type = type;
  wnd(box)->ctl = &state;
  wnd(box)->ctl_free = nullptr;
  HWND st = CreateWindowEx(0, "STATIC", text, WS_CHILD | WS_VISIBLE | SS_LEFT | SS_NOPREFIX,
                           M + 3, M, textW + 2, textH, box, (HMENU)(uintptr_t)0xFFFF, 0, nullptr);
  SendMessage(st, WM_SETFONT, (WPARAM)f, 0);
  int bx = (cw - btnsW) / 2, by = M + textH + 18;
  HWND first = 0;
  int defIdx = (type & MB_DEFMASK) == MB_DEFBUTTON2 ? 1 : (type & MB_DEFMASK) == MB_DEFBUTTON3 ? 2 : 0;
  for (int i = 0; i < nb; i++) {
    DWORD bs = WS_CHILD | WS_VISIBLE | WS_TABSTOP | (i == 0 ? WS_GROUP : 0) |
               (i == defIdx ? BS_DEFPUSHBUTTON : BS_PUSHBUTTON);
    HWND b = CreateWindowEx(0, "BUTTON", buttons[i].second, bs, bx + i * (BW + GAP), by, BW, BH, box,
                            (HMENU)(uintptr_t)buttons[i].first, 0, nullptr);
    SendMessage(b, WM_SETFONT, (WPARAM)f, 0);
    if (i == defIdx) first = b;
  }
  ShowWindow(box, SW_SHOW);
  SetFocus(first);

  bool quit = false;
  int quitCode = 0;
  while (!state.done && wnd(box)) {
    MSG msg;
    if (!GetMessage(&msg, 0, 0, 0)) { quit = true; quitCode = (int)msg.wParam; break; }
    if (!IsDialogMessage(box, &msg)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
  }
  if (disabledOwner && wnd(top)) EnableWindow(top, TRUE);
  if (Wnd *bw = wnd(box)) { bw->ctl = nullptr; DestroyWindow(box); }
  if (top && wnd(top)) SetActiveWindow(top);
  DeleteObject(f);
  if (quit) PostQuitMessage(quitCode);
  return state.result ? state.result : (kind == MB_OK ? IDOK : IDCANCEL);
}
