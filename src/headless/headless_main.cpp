/* ************************************************************************* */
/* headless_main.cpp -- runs the original program without Godot, for tests. */
/*                                                                           */
/* NEW FILE, added 2026-10-12 for the Stellar Frontier Godot port.           */
/*                                                                           */
/* It plays the host's part: starts the program on its game thread, feeds   */
/* scripted mouse/keyboard input, saves screenshots (BMP) and logs sound     */
/* requests.                                                                 */
/*                                                                           */
/* Usage: sf [options] [-- program arguments]                                */
/*   -install DIR      game install directory (holds binwin, dat, bmp...)   */
/*   -exe NAME         executable name the program believes it has          */
/*   -zap FILE         its appended bitmap archive                          */
/*   -icon FILE        .ico for the main window                             */
/*   -screen WxH       virtual desktop size (default 1024x768)              */
/*   -run MS           stop after MS milliseconds (default 10000)           */
/*   -shot MS:FILE     save a screenshot at MS milliseconds                 */
/*   -input FILE       input script: "<ms> move x y", "<ms> click x y [b]", */
/*                     "<ms> down x y b", "<ms> up x y b",                  */
/*                     "<ms> key vk sc ext down ch", "<ms> type text",      */
/*                     "<ms> close"                                         */
/*   -nt 0|1           report Windows 98 (0) or XP (1, default)             */
/*   -gaps MS          report pauses longer than MS between screen updates  */
/* ************************************************************************* */
#include "sf_host.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <algorithm>

int sf_program_main(int argc, char **argv);

namespace {

struct Shot { int ms; std::string file; };
struct Ev { int ms; std::string line; };

bool write_bmp(const std::string &path, const uint32_t *px, int w, int h)
{
  FILE *f = fopen(path.c_str(), "wb");
  if (!f) return false;
  int stride = (w * 3 + 3) & ~3;
  uint32_t size = 54 + stride * h;
  uint8_t hdr[54] = {'B', 'M'};
  auto put32 = [&](int o, uint32_t v) { for (int i = 0; i < 4; i++) hdr[o + i] = (v >> (8 * i)) & 255; };
  put32(2, size); put32(10, 54); put32(14, 40); put32(18, w); put32(22, h);
  hdr[26] = 1; hdr[28] = 24; put32(34, stride * h);
  fwrite(hdr, 1, 54, f);
  std::vector<uint8_t> row(stride, 0);
  for (int y = h - 1; y >= 0; y--) {
    for (int x = 0; x < w; x++) {
      uint32_t p = px[(size_t)y * w + x];
      row[3 * x] = p & 255; row[3 * x + 1] = (p >> 8) & 255; row[3 * x + 2] = (p >> 16) & 255;
    }
    fwrite(row.data(), 1, stride, f);
  }
  fclose(f);
  return true;
}

/* a few virtual-key / scan-code pairs for the "type" command */
void type_char(char c)
{
  int vk = 0, sc = 0;
  bool shift = false;
  static const char *row1 = "1234567890";
  if (c >= 'a' && c <= 'z') vk = c - 32;
  else if (c >= 'A' && c <= 'Z') { vk = c; shift = true; }
  else if (strchr(row1, c)) vk = c;
  else if (c == ' ') vk = 0x20;
  else if (c == '\n') vk = 0x0D;
  else if (c == '.') vk = 0xBE;
  else if (c == '-') vk = 0xBD;
  static const char *qw = "qwertyuiop";
  static const char *as = "asdfghjkl";
  static const char *zx = "zxcvbnm";
  char lc = (char)tolower((unsigned char)c);
  if (const char *p = strchr(qw, lc)) sc = 16 + (int)(p - qw);
  else if (const char *p = strchr(as, lc)) sc = 30 + (int)(p - as);
  else if (const char *p = strchr(zx, lc)) sc = 44 + (int)(p - zx);
  else if (c >= '1' && c <= '9') sc = 2 + (c - '1');
  else if (c == '0') sc = 11;
  else if (c == ' ') sc = 57;
  else if (c == '\n') sc = 28;
  else if (c == '.') sc = 52;
  else if (c == '-') sc = 12;
  if (shift) sf_host_key(0x10, 42, 0, 1, 0);
  sf_host_key(vk, sc, 0, 1, c == '\n' ? '\r' : (unsigned char)c);
  sf_host_key(vk, sc, 0, 0, 0);
  if (shift) sf_host_key(0x10, 42, 0, 0, 0);
}

void run_event(const std::string &line)
{
  char cmd[32] = {0};
  int a = 0, b = 0, c = 0, d = 0, e = 0;
  if (sscanf(line.c_str(), "%31s", cmd) != 1) return;
  const char *rest = line.c_str() + line.find(cmd) + strlen(cmd);
  if (!strcmp(cmd, "move") && sscanf(rest, "%d %d", &a, &b) == 2) sf_host_mouse_move(a, b);
  else if (!strcmp(cmd, "click")) {
    int n = sscanf(rest, "%d %d %d", &a, &b, &c);
    if (n < 3) c = 1;
    sf_host_mouse_move(a, b);
    sf_host_mouse_button(a, b, c, 1);
    std::this_thread::sleep_for(std::chrono::milliseconds(60));
    sf_host_mouse_button(a, b, c, 0);
  } else if (!strcmp(cmd, "down") && sscanf(rest, "%d %d %d", &a, &b, &c) == 3) sf_host_mouse_button(a, b, c, 1);
  else if (!strcmp(cmd, "up") && sscanf(rest, "%d %d %d", &a, &b, &c) == 3) sf_host_mouse_button(a, b, c, 0);
  else if (!strcmp(cmd, "key") && sscanf(rest, "%d %d %d %d %d", &a, &b, &c, &d, &e) >= 4) sf_host_key(a, b, c, d, (unsigned)e);
  else if (!strcmp(cmd, "type")) {
    const char *t = rest;
    while (*t == ' ') t++;
    for (; *t; t++) {
      if (*t == '\\' && t[1] == 'n') { type_char('\n'); t++; continue; }
      type_char(*t);
      std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
  } else if (!strcmp(cmd, "close")) sf_host_request_close();
  else fprintf(stderr, "[headless] unknown input: %s\n", line.c_str());
}

void log_line(const char *s) { fprintf(stderr, "[w32] %s\n", s); }
void open_url(const char *u) { fprintf(stderr, "[headless] open URL: %s\n", u); }
int create_process(const char *c) { fprintf(stderr, "[headless] run: %s\n", c); return 0; }
void minimize(int m) { fprintf(stderr, "[headless] minimize(%d)\n", m); }

}  /* namespace */

int main(int argc, char **argv)
{
  std::string install = ".", exe = "frontier.exe", zap, icon, input;
  int sw = 1024, shh = 768, runMs = 10000, nt = 1, gapMs = 0;
  std::vector<Shot> shots;
  std::vector<std::string> progArgs;
  progArgs.push_back(argv[0]);
  for (int i = 1; i < argc; i++) {
    std::string a = argv[i];
    auto next = [&]() -> std::string { return i + 1 < argc ? argv[++i] : ""; };
    if (a == "-install") install = next();
    else if (a == "-exe") exe = next();
    else if (a == "-zap") zap = next();
    else if (a == "-icon") icon = next();
    else if (a == "-screen") sscanf(next().c_str(), "%dx%d", &sw, &shh);
    else if (a == "-run") runMs = atoi(next().c_str());
    else if (a == "-input") input = next();
    else if (a == "-nt") nt = atoi(next().c_str());
    else if (a == "-gaps") gapMs = atoi(next().c_str());
    else if (a == "-shot") {
      std::string s = next();
      size_t c = s.find(':');
      if (c != std::string::npos) shots.push_back(Shot{atoi(s.substr(0, c).c_str()), s.substr(c + 1)});
    } else if (a == "--") {
      for (i++; i < argc; i++) progArgs.push_back(argv[i]);
    } else {
      fprintf(stderr, "unknown option %s\n", a.c_str());
      return 2;
    }
  }
  /* argv[0] of the program: its executable name without the extension */
  std::string base = exe;
  if (base.size() > 4 && (base.substr(base.size() - 4) == ".exe" || base.substr(base.size() - 4) == ".EXE"))
    base = base.substr(0, base.size() - 4);
  progArgs[0] = base;

  std::vector<Ev> events;
  if (!input.empty()) {
    FILE *f = fopen(input.c_str(), "r");
    if (!f) { fprintf(stderr, "cannot read %s\n", input.c_str()); return 2; }
    char line[1024];
    while (fgets(line, sizeof(line), f)) {
      int ms;
      char rest[1000];
      if (line[0] == '#') continue;
      if (sscanf(line, "%d %999[^\n]", &ms, rest) == 2) events.push_back(Ev{ms, rest});
    }
    fclose(f);
    std::stable_sort(events.begin(), events.end(), [](const Ev &x, const Ev &y) { return x.ms < y.ms; });
  }
  std::sort(shots.begin(), shots.end(), [](const Shot &x, const Shot &y) { return x.ms < y.ms; });

  sf_host_callbacks cb = {log_line, open_url, create_process, minimize};
  sf_host_set_callbacks(&cb);
  sf_host_config cfg;
  memset(&cfg, 0, sizeof(cfg));
  cfg.screen_w = sw;
  cfg.screen_h = shh;
  cfg.install_dir = install.c_str();
  cfg.exe_name = exe.c_str();
  cfg.zap_archive = zap.c_str();
  cfg.icon_file = icon.c_str();
  cfg.emulate_nt = nt;
  std::vector<char *> av;
  for (auto &s : progArgs) av.push_back(&s[0]);
  if (sf_host_start(&cfg, sf_program_main, (int)av.size(), av.data()) != 0) {
    fprintf(stderr, "start failed\n");
    return 1;
  }
  std::vector<uint32_t> frame((size_t)sw * shh, 0);
  unsigned serial = 0;
  auto t0 = std::chrono::steady_clock::now();
  size_t ei = 0, si = 0;
  int frames = 0;
  for (;;) {
    int ms = (int)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t0).count();
    while (ei < events.size() && events[ei].ms <= ms) run_event(events[ei++].line);
    if (sf_host_frame(frame.data(), &serial)) {
      static int lastFrameMs = -1;
      if (gapMs > 0 && lastFrameMs >= 0 && ms - lastFrameMs > gapMs)
        fprintf(stderr, "[headless] %d ms: no screen update for %d ms\n", ms, ms - lastFrameMs);
      lastFrameMs = ms;
      frames++;
    }
    while (si < shots.size() && shots[si].ms <= ms) {
      sf_host_frame(frame.data(), &serial);
      if (write_bmp(shots[si].file, frame.data(), sw, shh))
        fprintf(stderr, "[headless] %d ms: wrote %s\n", ms, shots[si].file.c_str());
      si++;
    }
    sf_host_audio_cmd ac;
    while (sf_host_audio_poll(&ac)) {
      static const char *names[] = {"?", "create", "destroy", "play", "stop", "volume", "music", "music-stop"};
      if (ac.cmd == 3 || ac.cmd >= 6)
        fprintf(stderr, "[audio] %d ms: %s id=%d loop=%d vol=%d %s\n", ms, names[ac.cmd < 8 ? ac.cmd : 0],
                ac.id, ac.loop, ac.volume_mb, ac.path);
    }
    int wx, wy;
    while (sf_host_take_warp(&wx, &wy)) fprintf(stderr, "[headless] pointer warp to %d,%d\n", wx, wy);
    if (ms >= runMs) break;
    if (!sf_host_running() && ei >= events.size() && si >= shots.size()) break;
    std::this_thread::sleep_for(std::chrono::milliseconds(16));
  }
  fprintf(stderr, "[headless] %d frames composed, program %s (exit code %d)\n", frames,
          sf_host_running() ? "still running" : "finished", sf_host_exit_code());
  fflush(stderr);
  _Exit(0);   /* the program's threads may still be running */
}
