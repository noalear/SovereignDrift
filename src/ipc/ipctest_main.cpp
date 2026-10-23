/* ************************************************************************* */
/* ipctest_main.cpp -- test host for the IPC protocol (64-bit).              */
/*                                                                           */
/* NEW FILE, added 2026-10-12 for the Stellar Frontier Godot port.           */
/* Starts a 32-bit program the way the Godot application does, feeds it a   */
/* scripted input file and saves screenshots.  Same script format as the    */
/* headless runner.                                                          */
/*   ipctest -exe PATH [-screen WxH] [-run MS] [-shot MS:FILE]...            */
/*           [-input FILE] [-- program arguments]                            */
/* ************************************************************************* */
#include "sf_ipc_host.h"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <thread>
#include <vector>

namespace {

struct Ev { int ms; std::string line; };
struct Shot { int ms; std::string file; };

bool write_bmp(const std::string &path, const uint32_t *px, int w, int h)
{
  FILE *f = fopen(path.c_str(), "wb");
  if (!f) return false;
  int stride = (w * 3 + 3) & ~3;
  uint8_t hdr[54] = {'B', 'M'};
  auto put32 = [&](int o, uint32_t v) { for (int i = 0; i < 4; i++) hdr[o + i] = (v >> (8 * i)) & 255; };
  put32(2, 54 + stride * h); put32(10, 54); put32(14, 40); put32(18, w); put32(22, h);
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

void type_char(SfProgram &p, char c)
{
  int vk = 0, sc = 0;
  bool shift = false;
  if (c >= 'a' && c <= 'z') vk = c - 32;
  else if (c >= 'A' && c <= 'Z') { vk = c; shift = true; }
  else if (c >= '0' && c <= '9') vk = c;
  else if (c == ' ') vk = 0x20;
  else if (c == '\n') vk = 0x0D;
  static const char *qw = "qwertyuiop", *as = "asdfghjkl", *zx = "zxcvbnm";
  char lc = (char)tolower((unsigned char)c);
  if (const char *q = strchr(qw, lc)) sc = 16 + (int)(q - qw);
  else if (const char *q = strchr(as, lc)) sc = 30 + (int)(q - as);
  else if (const char *q = strchr(zx, lc)) sc = 44 + (int)(q - zx);
  else if (c >= '1' && c <= '9') sc = 2 + (c - '1');
  else if (c == '0') sc = 11;
  else if (c == ' ') sc = 57;
  else if (c == '\n') sc = 28;
  if (shift) p.key(0x10, 42, false, true, 0);
  p.key(vk, sc, false, true, c == '\n' ? '\r' : (unsigned char)c);
  p.key(vk, sc, false, false, 0);
  if (shift) p.key(0x10, 42, false, false, 0);
}

void run_event(SfProgram &p, const std::string &line)
{
  char cmd[32] = {0};
  int a = 0, b = 0, c = 0, d = 0, e = 0;
  if (sscanf(line.c_str(), "%31s", cmd) != 1) return;
  const char *rest = line.c_str() + line.find(cmd) + strlen(cmd);
  if (!strcmp(cmd, "move") && sscanf(rest, "%d %d", &a, &b) == 2) p.mouse_move(a, b);
  else if (!strcmp(cmd, "click")) {
    if (sscanf(rest, "%d %d %d", &a, &b, &c) < 3) c = 1;
    p.mouse_move(a, b);
    p.mouse_button(a, b, c, true);
    std::this_thread::sleep_for(std::chrono::milliseconds(60));
    p.mouse_button(a, b, c, false);
  } else if (!strcmp(cmd, "key") && sscanf(rest, "%d %d %d %d %d", &a, &b, &c, &d, &e) >= 4)
    p.key(a, b, c != 0, d != 0, (unsigned)e);
  else if (!strcmp(cmd, "type")) {
    const char *t = rest;
    while (*t == ' ') t++;
    for (; *t; t++) { type_char(p, *t); std::this_thread::sleep_for(std::chrono::milliseconds(20)); }
  } else if (!strcmp(cmd, "close")) p.request_close();
}

}  /* namespace */

int main(int argc, char **argv)
{
  std::string exe, input;
  int w = 1024, h = 768, runMs = 10000;
  std::vector<Shot> shots;
  std::vector<std::string> args;
  for (int i = 1; i < argc; i++) {
    std::string a = argv[i];
    auto next = [&]() -> std::string { return i + 1 < argc ? argv[++i] : ""; };
    if (a == "-exe") exe = next();
    else if (a == "-screen") sscanf(next().c_str(), "%dx%d", &w, &h);
    else if (a == "-run") runMs = atoi(next().c_str());
    else if (a == "-input") input = next();
    else if (a == "-shot") {
      std::string s = next();
      size_t c = s.find(':');
      if (c != std::string::npos) shots.push_back(Shot{atoi(s.substr(0, c).c_str()), s.substr(c + 1)});
    } else if (a == "--") { for (i++; i < argc; i++) args.push_back(argv[i]); }
  }
  std::vector<Ev> events;
  if (!input.empty()) {
    FILE *f = fopen(input.c_str(), "r");
    char line[1024];
    while (f && fgets(line, sizeof(line), f)) {
      int ms;
      char rest[1000];
      if (line[0] != '#' && sscanf(line, "%d %999[^\n]", &ms, rest) == 2) events.push_back(Ev{ms, rest});
    }
    if (f) fclose(f);
    std::stable_sort(events.begin(), events.end(), [](const Ev &x, const Ev &y) { return x.ms < y.ms; });
  }
  std::sort(shots.begin(), shots.end(), [](const Shot &x, const Shot &y) { return x.ms < y.ms; });

  SfProgram p;
  std::string err;
  if (!p.start(exe, args, w, h, true, &err)) { fprintf(stderr, "start failed: %s\n", err.c_str()); return 1; }
  std::vector<uint32_t> frame((size_t)w * h, 0);
  auto t0 = std::chrono::steady_clock::now();
  size_t ei = 0, si = 0;
  int frames = 0, sounds = 0, lastFrameMs = -1;
  for (;;) {
    int ms = (int)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t0).count();
    while (ei < events.size() && events[ei].ms <= ms) run_event(p, events[ei++].line);
    if (const uint32_t *px = p.new_frame()) {
      memcpy(frame.data(), px, frame.size() * 4);
      p.frame_done();
      frames++;
      if (lastFrameMs >= 0 && ms - lastFrameMs > 1500) fprintf(stderr, "[ipctest] %d ms: %d ms without a new frame\n", ms, ms - lastFrameMs);
      lastFrameMs = ms;
    }
    while (si < shots.size() && shots[si].ms <= ms) {
      if (write_bmp(shots[si].file, frame.data(), w, h)) fprintf(stderr, "[ipctest] %d ms: wrote %s\n", ms, shots[si].file.c_str());
      si++;
    }
    SfProgram::Msg m;
    while (p.poll(m)) {
      switch (m.type) {
      case SF_OUT_LOG: fprintf(stderr, "[log] %s\n", m.text.c_str()); break;
      case SF_OUT_SOUND:
        if (sounds++ < 4) fprintf(stderr, "[ipctest] sound id=%d ch=%d bits=%d rate=%d bytes=%zu\n", m.i[0], m.i[1], m.i[2], m.i[3], m.data.size());
        break;
      case SF_OUT_AUDIO:
        if (m.i[0] == 6) fprintf(stderr, "[ipctest] %d ms: music %s\n", ms, m.text.c_str());
        break;
      case SF_OUT_EXITED: fprintf(stderr, "[ipctest] program exited with %d\n", m.i[0]); break;
      case SF_OUT_RUN: fprintf(stderr, "[ipctest] run: %s\n", m.text.c_str()); break;
      }
    }
    if (ms >= runMs) break;
    int st = p.state();
    if ((st == SF_IPC_EXITED || st == SF_IPC_FAILED) && ei >= events.size()) break;
    std::this_thread::sleep_for(std::chrono::milliseconds(8));
  }
  fprintf(stderr, "[ipctest] %d frames, %d sounds, state %d\n", frames, sounds, p.state());
  p.stop();
  return 0;
}
