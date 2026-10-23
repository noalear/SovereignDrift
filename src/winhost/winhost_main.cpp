/* ************************************************************************* */
/* winhost_main.cpp -- Windows preview host.                                 */
/*                                                                           */
/* NEW FILE, added 2026-10-12 for the Stellar Frontier Godot port.           */
/*                                                                           */
/* A small native Windows front end used until the Godot application is     */
/* ready, and kept afterwards as a test harness.  It plays the host's part  */
/* (see src/host/sf_host.h): it runs the original program on its game       */
/* thread, shows the program's virtual desktop in a window (scaled, Alt+Enter*/
/* toggles full screen), passes mouse and keyboard input through, mixes the */
/* program's DirectSound buffers with waveOut and plays its MIDI music with */
/* the Windows MIDI synthesizer.                                            */
/*                                                                           */
/* This file uses the real Win32 API; it must not include sfwin.h.          */
/*                                                                           */
/* Command line: frontier.exe [-screen WxH] [-window|-fullscreen]           */
/*               [program arguments, e.g. -log or a host name]              */
/* ************************************************************************* */
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <mmsystem.h>
#include <shellapi.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <condition_variable>
#include <deque>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include "sf_host.h"

int sf_program_main(int argc, char **argv);

#ifndef SF_EXE_NAME
#  define SF_EXE_NAME "frontier.exe"
#endif
#ifndef SF_ZAP_NAME
#  define SF_ZAP_NAME "frontier.zap"
#endif
#ifndef SF_ICON_NAME
#  define SF_ICON_NAME "frontier.ico"
#endif
#ifndef SF_TITLE
#  define SF_TITLE "Stellar Frontier"
#endif

namespace {

/* ================================ audio ================================== */
const int MIX_RATE = 44100;
const int MIX_FRAMES = 512;      /* per waveOut buffer (~11.6 ms)            */
const int MIX_BUFFERS = 4;

struct Sound {
  std::vector<int16_t> pcm;      /* interleaved stereo                       */
  int rate = 22050;
  size_t frames() const { return pcm.size() / 2; }
};

struct Voice {
  std::shared_ptr<Sound> snd;
  bool playing = false, loop = false;
  double pos = 0;                /* in source frames                         */
  float gain = 1.0f;
};

std::mutex g_amx;
std::map<int, Voice> g_voices;
HWAVEOUT g_wo = nullptr;
HANDLE g_woEvent = nullptr;
WAVEHDR g_hdr[MIX_BUFFERS];
std::vector<int16_t> g_buf[MIX_BUFFERS];
std::vector<float> g_acc;
volatile bool g_audioRun = true;

float mb_to_gain(int mb)
{
  if (mb <= -10000) return 0.0f;
  return (float)std::pow(10.0, mb / 2000.0);
}

void mix(int16_t *out, int frames)
{
  g_acc.assign((size_t)frames * 2, 0.0f);
  {
    std::lock_guard<std::mutex> lk(g_amx);
    for (auto &kv : g_voices) {
      Voice &v = kv.second;
      if (!v.playing || !v.snd || v.snd->frames() == 0) continue;
      const Sound &s = *v.snd;
      double step = (double)s.rate / MIX_RATE;
      size_t n = s.frames();
      for (int i = 0; i < frames; i++) {
        if (v.pos >= (double)n) {
          if (v.loop) v.pos = std::fmod(v.pos, (double)n);
          else { v.playing = false; v.pos = 0; break; }
        }
        size_t i0 = (size_t)v.pos;
        size_t i1 = i0 + 1 < n ? i0 + 1 : (v.loop ? 0 : i0);
        float f = (float)(v.pos - (double)i0);
        float l = s.pcm[2 * i0] * (1 - f) + s.pcm[2 * i1] * f;
        float r = s.pcm[2 * i0 + 1] * (1 - f) + s.pcm[2 * i1 + 1] * f;
        g_acc[2 * i] += l * v.gain;
        g_acc[2 * i + 1] += r * v.gain;
        v.pos += step;
      }
    }
  }
  for (int i = 0; i < frames * 2; i++) {
    float x = g_acc[i];
    out[i] = (int16_t)(x > 32767.f ? 32767 : x < -32768.f ? -32768 : (int)x);
  }
}

DWORD WINAPI audio_thread(LPVOID)
{
  while (g_audioRun) {
    WaitForSingleObject(g_woEvent, 50);
    for (int b = 0; b < MIX_BUFFERS; b++) {
      if (!(g_hdr[b].dwFlags & WHDR_DONE)) continue;
      mix(g_buf[b].data(), MIX_FRAMES);
      g_hdr[b].dwFlags &= ~WHDR_DONE;
      waveOutWrite(g_wo, &g_hdr[b], sizeof(WAVEHDR));
    }
  }
  return 0;
}

void audio_init()
{
  WAVEFORMATEX wf = {};
  wf.wFormatTag = WAVE_FORMAT_PCM;
  wf.nChannels = 2;
  wf.nSamplesPerSec = MIX_RATE;
  wf.wBitsPerSample = 16;
  wf.nBlockAlign = 4;
  wf.nAvgBytesPerSec = MIX_RATE * 4;
  g_woEvent = CreateEventA(nullptr, FALSE, FALSE, nullptr);
  if (waveOutOpen(&g_wo, WAVE_MAPPER, &wf, (DWORD_PTR)g_woEvent, 0, CALLBACK_EVENT) != MMSYSERR_NOERROR) {
    g_wo = nullptr;
    return;
  }
  for (int b = 0; b < MIX_BUFFERS; b++) {
    g_buf[b].assign(MIX_FRAMES * 2, 0);
    memset(&g_hdr[b], 0, sizeof(WAVEHDR));
    g_hdr[b].lpData = (LPSTR)g_buf[b].data();
    g_hdr[b].dwBufferLength = MIX_FRAMES * 4;
    waveOutPrepareHeader(g_wo, &g_hdr[b], sizeof(WAVEHDR));
    g_hdr[b].dwFlags |= WHDR_DONE;
  }
  CreateThread(nullptr, 0, audio_thread, nullptr, 0, nullptr);
  SetEvent(g_woEvent);
}

std::shared_ptr<Sound> fetch_sound(int id)
{
  const void *p = nullptr;
  int ch = 1, bits = 8, rate = 22050;
  uint32_t n = sf_host_sound_data(id, &p, &ch, &bits, &rate);
  auto s = std::make_shared<Sound>();
  s->rate = rate > 0 ? rate : 22050;
  if (!p || n == 0 || ch < 1) return s;
  int bps = bits == 16 ? 2 : 1;
  size_t frames = n / (size_t)(bps * ch);
  s->pcm.resize(frames * 2);
  const uint8_t *b = (const uint8_t *)p;
  for (size_t i = 0; i < frames; i++) {
    int16_t smp[2];
    for (int c = 0; c < 2; c++) {
      int src = c < ch ? c : 0;
      size_t o = (i * ch + src) * bps;
      smp[c] = bps == 2 ? (int16_t)(b[o] | (b[o + 1] << 8)) : (int16_t)((b[o] - 128) << 8);
    }
    s->pcm[2 * i] = smp[0];
    s->pcm[2 * i + 1] = smp[1];
  }
  return s;
}

/* MIDI music through MCI (the Windows GS synthesizer).  Opening a MIDI
   file can block for up to a second while Windows starts its synthesizer,
   so all MCI calls run on their own thread; the window never waits. */
struct MusicCmd { int op; std::string path; bool restart; };   /* op: 1 play, 2 stop, 3 quit */
std::mutex g_mmx;
std::condition_variable g_mcv;
std::deque<MusicCmd> g_mq;
HANDLE g_musicThread = nullptr;

void music_play_now(std::string &openPath, bool &isOpen, const std::string &path, bool restart)
{
  char full[MAX_PATH];
  if (!GetFullPathNameA(path.c_str(), MAX_PATH, full, nullptr)) return;
  if (!isOpen || _stricmp(full, openPath.c_str()) != 0) {
    if (isOpen) mciSendStringA("close sfmusic", nullptr, 0, nullptr);
    std::string cmd = std::string("open \"") + full + "\" type sequencer alias sfmusic";
    isOpen = mciSendStringA(cmd.c_str(), nullptr, 0, nullptr) == 0;
    openPath = full;
    restart = true;
  }
  if (!isOpen) return;
  mciSendStringA(restart ? "play sfmusic from 0" : "play sfmusic", nullptr, 0, nullptr);
}

DWORD WINAPI music_thread(LPVOID)
{
  std::string openPath;
  bool isOpen = false;
  for (;;) {
    MusicCmd c;
    {
      std::unique_lock<std::mutex> lk(g_mmx);
      g_mcv.wait(lk, [] { return !g_mq.empty(); });
      /* only the newest request matters (a stop followed by a play is a play) */
      c = g_mq.back();
      bool restart = false;
      for (auto &q : g_mq) if (q.op == 1 && q.restart) restart = true;
      if (c.op == 1 && restart) c.restart = true;
      g_mq.clear();
    }
    if (c.op == 3) break;
    if (c.op == 1) music_play_now(openPath, isOpen, c.path, c.restart);
    else if (c.op == 2 && isOpen) mciSendStringA("stop sfmusic", nullptr, 0, nullptr);
  }
  if (isOpen) {
    mciSendStringA("stop sfmusic", nullptr, 0, nullptr);
    mciSendStringA("close sfmusic", nullptr, 0, nullptr);
  }
  return 0;
}

void music_post(int op, const char *path = "", bool restart = false)
{
  {
    std::lock_guard<std::mutex> lk(g_mmx);
    g_mq.push_back(MusicCmd{op, path ? path : "", restart});
  }
  g_mcv.notify_one();
}

void pump_audio()
{
  sf_host_audio_cmd c;
  while (sf_host_audio_poll(&c)) {
    switch (c.cmd) {
    case SF_AUD_CREATE: {
      auto s = fetch_sound(c.id);
      std::lock_guard<std::mutex> lk(g_amx);
      g_voices[c.id].snd = s;
      break;
    }
    case SF_AUD_DESTROY: {
      std::lock_guard<std::mutex> lk(g_amx);
      g_voices.erase(c.id);
      break;
    }
    case SF_AUD_PLAY: {
      std::lock_guard<std::mutex> lk(g_amx);
      Voice &v = g_voices[c.id];
      if (!v.snd) {
        /* not announced yet (should not happen): fetch now */
        g_amx.unlock();
        auto s = fetch_sound(c.id);
        g_amx.lock();
        v.snd = s;
      }
      if (c.restart) v.pos = 0;
      v.loop = c.loop != 0;
      v.gain = mb_to_gain(c.volume_mb);
      v.playing = true;
      break;
    }
    case SF_AUD_STOP: {
      std::lock_guard<std::mutex> lk(g_amx);
      auto it = g_voices.find(c.id);
      if (it != g_voices.end()) it->second.playing = false;
      break;
    }
    case SF_AUD_VOLUME: {
      std::lock_guard<std::mutex> lk(g_amx);
      auto it = g_voices.find(c.id);
      if (it != g_voices.end()) it->second.gain = mb_to_gain(c.volume_mb);
      break;
    }
    case SF_AUD_MUSIC_PLAY: music_post(1, c.path, c.restart != 0); break;
    case SF_AUD_MUSIC_STOP: music_post(2); break;
    }
  }
}

/* ================================ window ================================= */
HWND g_hwnd = nullptr;
int g_sw = 1024, g_sh = 768;              /* virtual desktop size            */
std::vector<uint32_t> g_frame;
unsigned g_serial = 0;
bool g_fullscreen = false;
WINDOWPLACEMENT g_wpPrev = {sizeof(WINDOWPLACEMENT)};
RECT g_dest = {0, 0, 0, 0};               /* where the desktop is drawn      */
int g_buttons = 0;
bool g_programDone = false;
std::string g_installDir;

void update_dest()
{
  RECT rc;
  GetClientRect(g_hwnd, &rc);
  int cw = rc.right, ch = rc.bottom;
  if (cw <= 0 || ch <= 0) { g_dest = {0, 0, 0, 0}; return; }
  double s = std::min((double)cw / g_sw, (double)ch / g_sh);
  /* prefer whole-number scales when they fill most of the window */
  double si = std::floor(s);
  if (si >= 1 && si / s > 0.9) s = si;
  int w = (int)(g_sw * s + 0.5), h = (int)(g_sh * s + 0.5);
  g_dest.left = (cw - w) / 2;
  g_dest.top = (ch - h) / 2;
  g_dest.right = g_dest.left + w;
  g_dest.bottom = g_dest.top + h;
}

void to_desktop(int cx, int cy, int *x, int *y)
{
  int w = g_dest.right - g_dest.left, h = g_dest.bottom - g_dest.top;
  if (w <= 0 || h <= 0) { *x = cx; *y = cy; return; }
  long long dx = (long long)(cx - g_dest.left) * g_sw / w;
  long long dy = (long long)(cy - g_dest.top) * g_sh / h;
  *x = (int)(dx < 0 ? 0 : dx >= g_sw ? g_sw - 1 : dx);
  *y = (int)(dy < 0 ? 0 : dy >= g_sh ? g_sh - 1 : dy);
}

void set_fullscreen(bool on)
{
  DWORD style = GetWindowLongA(g_hwnd, GWL_STYLE);
  if (on && !g_fullscreen) {
    MONITORINFO mi = {sizeof(mi)};
    if (GetWindowPlacement(g_hwnd, &g_wpPrev) &&
        GetMonitorInfoA(MonitorFromWindow(g_hwnd, MONITOR_DEFAULTTOPRIMARY), &mi)) {
      SetWindowLongA(g_hwnd, GWL_STYLE, style & ~WS_OVERLAPPEDWINDOW);
      SetWindowPos(g_hwnd, HWND_TOP, mi.rcMonitor.left, mi.rcMonitor.top,
                   mi.rcMonitor.right - mi.rcMonitor.left, mi.rcMonitor.bottom - mi.rcMonitor.top,
                   SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
      g_fullscreen = true;
    }
  } else if (!on && g_fullscreen) {
    SetWindowLongA(g_hwnd, GWL_STYLE, style | WS_OVERLAPPEDWINDOW);
    SetWindowPlacement(g_hwnd, &g_wpPrev);
    SetWindowPos(g_hwnd, nullptr, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    g_fullscreen = false;
  }
  update_dest();
  InvalidateRect(g_hwnd, nullptr, TRUE);
}

void paint(HDC dc)
{
  RECT rc;
  GetClientRect(g_hwnd, &rc);
  HBRUSH black = (HBRUSH)GetStockObject(BLACK_BRUSH);
  /* borders around the picture */
  RECT r;
  r = {0, 0, rc.right, g_dest.top};                       FillRect(dc, &r, black);
  r = {0, g_dest.bottom, rc.right, rc.bottom};            FillRect(dc, &r, black);
  r = {0, g_dest.top, g_dest.left, g_dest.bottom};        FillRect(dc, &r, black);
  r = {g_dest.right, g_dest.top, rc.right, g_dest.bottom}; FillRect(dc, &r, black);
  BITMAPINFO bi = {};
  bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  bi.bmiHeader.biWidth = g_sw;
  bi.bmiHeader.biHeight = -g_sh;          /* top-down */
  bi.bmiHeader.biPlanes = 1;
  bi.bmiHeader.biBitCount = 32;
  bi.bmiHeader.biCompression = BI_RGB;
  int w = g_dest.right - g_dest.left, h = g_dest.bottom - g_dest.top;
  bool whole = (w % g_sw) == 0 && (h % g_sh) == 0;
  SetStretchBltMode(dc, whole ? COLORONCOLOR : HALFTONE);
  SetBrushOrgEx(dc, 0, 0, nullptr);
  StretchDIBits(dc, g_dest.left, g_dest.top, w, h, 0, 0, g_sw, g_sh, g_frame.data(), &bi,
                DIB_RGB_COLORS, SRCCOPY);
}

void apply_cursor()
{
  static HCURSOR cur[5] = {};
  static const LPCSTR ids[5] = {IDC_ARROW, IDC_IBEAM, IDC_WAIT, IDC_SIZEALL, IDC_HAND};
  int s = sf_host_cursor();
  if (s < 0) { SetCursor(nullptr); return; }
  if (s > 4) s = 0;
  if (!cur[s]) cur[s] = LoadCursorA(nullptr, ids[s]);
  SetCursor(cur[s]);
}

unsigned key_char(WPARAM vk, LPARAM lp)
{
  BYTE ks[256];
  if (!GetKeyboardState(ks)) return 0;
  WCHAR wb[4];
  UINT sc = (lp >> 16) & 0xFF;
  int n = ToUnicode((UINT)vk, sc, ks, wb, 4, 0);
  if (n != 1) return 0;
  char ab[4];
  BOOL used = FALSE;
  int m = WideCharToMultiByte(1252, 0, wb, 1, ab, sizeof(ab), nullptr, &used);
  if (m != 1 || used) return 0;
  return (unsigned char)ab[0];
}

LRESULT CALLBACK wndproc(HWND h, UINT m, WPARAM wp, LPARAM lp)
{
  switch (m) {
  case WM_SIZE:
    update_dest();
    if (wp == SIZE_RESTORED || wp == SIZE_MAXIMIZED) sf_host_restore();
    InvalidateRect(h, nullptr, FALSE);
    return 0;
  case WM_ERASEBKGND:
    return 1;
  case WM_PAINT: {
    PAINTSTRUCT ps;
    HDC dc = BeginPaint(h, &ps);
    paint(dc);
    EndPaint(h, &ps);
    return 0;
  }
  case WM_SETCURSOR:
    if (LOWORD(lp) == HTCLIENT) { apply_cursor(); return TRUE; }
    break;
  case WM_MOUSEMOVE: {
    int x, y;
    to_desktop((short)LOWORD(lp), (short)HIWORD(lp), &x, &y);
    sf_host_mouse_move(x, y);
    return 0;
  }
  case WM_LBUTTONDOWN: case WM_RBUTTONDOWN: case WM_MBUTTONDOWN:
  case WM_LBUTTONDBLCLK: case WM_RBUTTONDBLCLK: case WM_MBUTTONDBLCLK:
  case WM_LBUTTONUP: case WM_RBUTTONUP: case WM_MBUTTONUP: {
    int b = (m == WM_LBUTTONDOWN || m == WM_LBUTTONUP || m == WM_LBUTTONDBLCLK) ? 1 :
            (m == WM_RBUTTONDOWN || m == WM_RBUTTONUP || m == WM_RBUTTONDBLCLK) ? 2 : 3;
    bool down = !(m == WM_LBUTTONUP || m == WM_RBUTTONUP || m == WM_MBUTTONUP);
    int x, y;
    to_desktop((short)LOWORD(lp), (short)HIWORD(lp), &x, &y);
    if (down) { if (!g_buttons) SetCapture(h); g_buttons |= 1 << b; }
    else { g_buttons &= ~(1 << b); if (!g_buttons) ReleaseCapture(); }
    sf_host_mouse_button(x, y, b, down ? 1 : 0);
    return 0;
  }
  case WM_MOUSEWHEEL: {
    POINT p = {(short)LOWORD(lp), (short)HIWORD(lp)};
    ScreenToClient(h, &p);
    int x, y;
    to_desktop(p.x, p.y, &x, &y);
    sf_host_mouse_wheel(x, y, (short)HIWORD(wp));
    return 0;
  }
  case WM_KEYDOWN: case WM_SYSKEYDOWN: case WM_KEYUP: case WM_SYSKEYUP: {
    bool down = (m == WM_KEYDOWN || m == WM_SYSKEYDOWN);
    bool alt = (lp & (1 << 29)) != 0;
    if (down && alt && wp == VK_RETURN) {          /* Alt+Enter: full screen */
      if (!(lp & (1 << 30))) set_fullscreen(!g_fullscreen);
      return 0;
    }
    int sc = (lp >> 16) & 0xFF;
    int ext = (lp >> 24) & 1;
    unsigned ch = down ? key_char(wp, lp) : 0;
    sf_host_key((int)wp, sc, ext, down ? 1 : 0, ch);
    return 0;
  }
  case WM_CHAR: case WM_SYSCHAR: case WM_DEADCHAR: case WM_SYSDEADCHAR:
    return 0;
  case WM_SETFOCUS:
    sf_host_focus(1);
    return 0;
  case WM_KILLFOCUS:
    sf_host_focus(0);
    if (g_buttons) { g_buttons = 0; ReleaseCapture(); }
    return 0;
  case WM_CLOSE:
    if (g_programDone) DestroyWindow(h);
    else sf_host_request_close();
    return 0;
  case WM_DESTROY:
    PostQuitMessage(0);
    return 0;
  }
  return DefWindowProcA(h, m, wp, lp);
}

/* ============================== callbacks ================================ */
FILE *g_log = nullptr;

void cb_log(const char *line)
{
  if (g_log) { fprintf(g_log, "%s\n", line); fflush(g_log); }
}
void cb_open_url(const char *url) { ShellExecuteA(nullptr, "open", url, nullptr, nullptr, SW_SHOWNORMAL); }
int cb_create_process(const char *cmd)
{
  cb_log((std::string("program asked to run: ") + cmd).c_str());
  return 0;
}
void cb_minimize(int m) { if (m && g_hwnd) PostMessageA(g_hwnd, WM_SYSCOMMAND, SC_MINIMIZE, 0); }

std::string exe_dir()
{
  char p[MAX_PATH];
  DWORD n = GetModuleFileNameA(nullptr, p, MAX_PATH);
  std::string s(p, n);
  size_t k = s.find_last_of("\\/");
  return k == std::string::npos ? std::string(".") : s.substr(0, k);
}

}  /* namespace */

int WINAPI WinMain(HINSTANCE inst, HINSTANCE, LPSTR, int show)
{
  SetProcessDPIAware();
  g_installDir = exe_dir();
  std::string logPath = g_installDir + "\\sfhost.log";
  g_log = fopen(logPath.c_str(), "w");

  /* arguments: host options first, the rest go to the program */
  int startFull = 0;
  std::vector<std::string> args;
  args.push_back(std::string(SF_EXE_NAME).substr(0, std::string(SF_EXE_NAME).size() - 4));
  for (int i = 1; i < __argc; i++) {
    std::string a = __argv[i];
    if (a == "-screen" && i + 1 < __argc) { sscanf(__argv[++i], "%dx%d", &g_sw, &g_sh); continue; }
    if (a == "-fullscreen") { startFull = 1; continue; }
    if (a == "-window") { startFull = -1; continue; }
    args.push_back(a);
  }
  if (g_sw < 640) g_sw = 640;
  if (g_sh < 480) g_sh = 480;
  g_frame.assign((size_t)g_sw * g_sh, 0);

  sf_host_callbacks cb = {cb_log, cb_open_url, cb_create_process, cb_minimize};
  sf_host_set_callbacks(&cb);

  WNDCLASSA wc = {};
  wc.style = CS_DBLCLKS;
  wc.lpfnWndProc = wndproc;
  wc.hInstance = inst;
  std::string icon = g_installDir + "\\icons\\" SF_ICON_NAME;
  wc.hIcon = (HICON)LoadImageA(nullptr, icon.c_str(), IMAGE_ICON, 0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE);
  wc.hCursor = nullptr;
  wc.hbrBackground = nullptr;
  wc.lpszClassName = "SFPreviewHost";
  RegisterClassA(&wc);

  /* window size: the largest whole-number scale that fits the work area */
  RECT work;
  SystemParametersInfoA(SPI_GETWORKAREA, 0, &work, 0);
  int scale = 1;
  for (int s = 2; s <= 4; s++) {
    RECT r = {0, 0, g_sw * s, g_sh * s};
    AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, FALSE);
    if (r.right - r.left <= work.right - work.left && r.bottom - r.top <= work.bottom - work.top) scale = s;
  }
  RECT r = {0, 0, g_sw * scale, g_sh * scale};
  AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, FALSE);
  int ww = r.right - r.left, wh = r.bottom - r.top;
  int wx = work.left + std::max(0L, ((work.right - work.left) - ww) / 2);
  int wy = work.top + std::max(0L, ((work.bottom - work.top) - wh) / 2);
  bool fits = ww <= work.right - work.left && wh <= work.bottom - work.top;
  g_hwnd = CreateWindowExA(0, wc.lpszClassName, SF_TITLE, WS_OVERLAPPEDWINDOW, wx, wy, ww, wh,
                           nullptr, nullptr, inst, nullptr);
  if (!g_hwnd) return 1;
  update_dest();
  ShowWindow(g_hwnd, show);
  if (startFull > 0 || (startFull == 0 && !fits)) set_fullscreen(true);

  audio_init();
  g_musicThread = CreateThread(nullptr, 0, music_thread, nullptr, 0, nullptr);

  std::string zap = g_installDir + "\\zap\\" SF_ZAP_NAME;
  sf_host_config cfg = {};
  cfg.screen_w = g_sw;
  cfg.screen_h = g_sh;
  cfg.install_dir = g_installDir.c_str();
  cfg.exe_name = SF_EXE_NAME;
  cfg.zap_archive = zap.c_str();
  cfg.icon_file = icon.c_str();
  cfg.emulate_nt = 1;
  std::vector<char *> av;
  for (auto &s : args) av.push_back(&s[0]);
  if (sf_host_start(&cfg, sf_program_main, (int)av.size(), av.data()) != 0) {
    MessageBoxA(g_hwnd, "The game could not be started.", SF_TITLE, MB_ICONERROR);
    return 1;
  }

  timeBeginPeriod(1);
  MSG msg;
  bool quit = false;
  DWORD lastIter = timeGetTime();
  while (!quit) {
    {
      /* note in sfhost.log when the window stopped updating for a while */
      DWORD now = timeGetTime();
      if (now - lastIter > 250) {
        char b[96];
        snprintf(b, sizeof(b), "window loop stalled for %lu ms", (unsigned long)(now - lastIter));
        cb_log(b);
      }
      lastIter = now;
    }
    while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE)) {
      if (msg.message == WM_QUIT) { quit = true; break; }
      TranslateMessage(&msg);
      DispatchMessageA(&msg);
    }
    if (quit) break;
    if (sf_host_frame(g_frame.data(), &g_serial)) {
      HDC dc = GetDC(g_hwnd);
      paint(dc);
      ReleaseDC(g_hwnd, dc);
    }
    pump_audio();
    int x, y;
    while (sf_host_take_warp(&x, &y)) {
      POINT p = {g_dest.left + (x * (g_dest.right - g_dest.left) + g_sw / 2) / g_sw,
                 g_dest.top + (y * (g_dest.bottom - g_dest.top) + g_sh / 2) / g_sh};
      ClientToScreen(g_hwnd, &p);
      SetCursorPos(p.x, p.y);
    }
    {
      /* keep the pointer shape in step with the program */
      static int lastCursor = -2;
      int c = sf_host_cursor();
      if (c != lastCursor) {
        lastCursor = c;
        POINT p;
        GetCursorPos(&p);
        if (WindowFromPoint(p) == g_hwnd) {
          ScreenToClient(g_hwnd, &p);
          RECT rc;
          GetClientRect(g_hwnd, &rc);
          if (PtInRect(&rc, p)) apply_cursor();
        }
      }
    }
    if (!g_programDone && !sf_host_running()) {
      g_programDone = true;
      cb_log("program finished");
      DestroyWindow(g_hwnd);
    }
    MsgWaitForMultipleObjects(0, nullptr, FALSE, 8, QS_ALLINPUT);
  }
  timeEndPeriod(1);
  g_audioRun = false;
  music_post(3);
  if (g_musicThread) WaitForSingleObject(g_musicThread, 2000);
  if (g_log) fclose(g_log);
  /* the program's other threads may still be running */
  ExitProcess(0);
}
