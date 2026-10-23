/* ************************************************************************* */
/* ipc_child_main.cpp -- an original program as a child of the Godot app.    */
/*                                                                           */
/* NEW FILE, added 2026-10-12 for the Stellar Frontier Godot port.           */
/*                                                                           */
/* Built once per program (frontier, server, nexus) as a 32-bit executable.  */
/* It runs the original main() on its game thread (sf_host_start) and       */
/* connects the emulation's host interface (src/host/sf_host.h) to the       */
/* shared memory block the Godot application created (src/ipc/sf_ipc.h).     */
/*                                                                           */
/*   <program> --sf-ipc NAME --sf-parent PID [program arguments]             */
/* ************************************************************************* */
#include "sf_host.h"
#include "sf_ipc.h"
#include "sf_ipc_os.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <string>
#include <vector>

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

namespace {

sfipc::Shm g_shm;
sf_ipc_header *g_hd = nullptr;
uint8_t *g_in = nullptr, *g_out = nullptr;
uint32_t *g_pixels = nullptr;
std::mutex g_outMtx;
uint32_t g_parent = 0;
bool g_parentGone = false;

/* Write a message to the host, waiting while the ring is full. */
void out_msg(uint32_t type, const void *a, uint32_t na, const void *b = nullptr, uint32_t nb = 0)
{
  if (!g_hd || g_parentGone) return;
  std::lock_guard<std::mutex> lk(g_outMtx);
  for (int tries = 0; tries < 5000; tries++) {
    if (sf_ipc_write(g_out, g_hd->out_size, &g_hd->out_head, &g_hd->out_tail, type, a, na, b, nb))
      return;
    if (g_parent && !sfipc::parent_alive(g_parent)) { g_parentGone = true; return; }
    sfipc::sleep_ms(1);
  }
}

void out_text(uint32_t type, const char *s) { out_msg(type, s, (uint32_t)strlen(s)); }

void cb_log(const char *line) { out_text(SF_OUT_LOG, line); }
void cb_open_url(const char *url) { out_text(SF_OUT_OPEN_URL, url); }
int cb_run(const char *cmd) { out_text(SF_OUT_RUN, cmd); return 1; }
void cb_minimize(int m) { int32_t v = m; out_msg(SF_OUT_MINIMIZE, &v, 4); }

void handle_input()
{
  uint32_t type, len;
  int32_t a[8];
  while (sf_ipc_read(g_in, g_hd->in_size, &g_hd->in_head, &g_hd->in_tail, &type, &len, a, sizeof(a))) {
    switch (type) {
    case SF_IN_MOUSE_MOVE:   sf_host_mouse_move(a[0], a[1]); break;
    case SF_IN_MOUSE_BUTTON: sf_host_mouse_button(a[0], a[1], a[2], a[3]); break;
    case SF_IN_MOUSE_WHEEL:  sf_host_mouse_wheel(a[0], a[1], a[2]); break;
    case SF_IN_KEY:          sf_host_key(a[0], a[1], a[2], a[3], (unsigned)a[4]); break;
    case SF_IN_FOCUS:        sf_host_focus(a[0]); break;
    case SF_IN_CLOSE:        sf_host_request_close(); break;
    case SF_IN_RESTORE:      sf_host_restore(); break;
    }
  }
}

void pump_audio()
{
  sf_host_audio_cmd c;
  while (sf_host_audio_poll(&c)) {
    if (c.cmd == SF_AUD_CREATE) {
      const void *pcm = nullptr;
      int ch = 1, bits = 8, rate = 22050;
      uint32_t n = sf_host_sound_data(c.id, &pcm, &ch, &bits, &rate);
      int32_t h[4] = {c.id, ch, bits, rate};
      out_msg(SF_OUT_SOUND, h, sizeof(h), pcm, pcm ? n : 0);
    } else {
      int32_t h[5] = {c.cmd, c.id, c.loop, c.volume_mb, c.restart};
      out_msg(SF_OUT_AUDIO, h, sizeof(h), c.path, (uint32_t)strlen(c.path));
    }
  }
}

}  /* namespace */

int main(int argc, char **argv)
{
  std::string ipc;
  std::vector<std::string> progArgs;
  std::string base = SF_EXE_NAME;
  progArgs.push_back(base.substr(0, base.size() - 4));
  for (int i = 1; i < argc; i++) {
    std::string a = argv[i];
    if (a == "--sf-ipc" && i + 1 < argc) { ipc = argv[++i]; continue; }
    if (a == "--sf-parent" && i + 1 < argc) { g_parent = (uint32_t)strtoul(argv[++i], nullptr, 10); continue; }
    progArgs.push_back(a);
  }
  if (ipc.empty()) {
    fprintf(stderr, "%s is part of Stellar Frontier; start the game with StellarFrontier.exe\n", SF_EXE_NAME);
    return 2;
  }
  sfipc::die_with_parent();
  sfipc::precise_timers();

  /* map the header to learn the block size, then the whole block */
  sfipc::Shm probe;
  if (!sfipc::shm_open_existing(probe, ipc.c_str(), sizeof(sf_ipc_header))) {
    fprintf(stderr, "cannot open shared memory %s\n", ipc.c_str());
    return 3;
  }
  sf_ipc_header ph = *(sf_ipc_header *)probe.base;
  sfipc::shm_close(probe);
  if (ph.magic != SF_IPC_MAGIC || ph.version != SF_IPC_VERSION) {
    fprintf(stderr, "shared memory version mismatch\n");
    return 4;
  }
  if (!sfipc::shm_open_existing(g_shm, ipc.c_str(), ph.total_size)) return 3;
  g_hd = (sf_ipc_header *)g_shm.base;
  uint8_t *b = (uint8_t *)g_shm.base;
  g_pixels = (uint32_t *)(b + g_hd->frame_offset);
  g_in = b + g_hd->in_offset;
  g_out = b + g_hd->out_offset;
  g_hd->child_pid = sfipc::self_pid();

  sf_host_callbacks cb = {cb_log, cb_open_url, cb_run, cb_minimize};
  sf_host_set_callbacks(&cb);

  std::string dir = sfipc::exe_dir();
#ifdef _WIN32
  const char *sep = "\\";
#else
  const char *sep = "/";
#endif
  std::string zap = dir + sep + "zap" + sep + SF_ZAP_NAME;
  std::string icon = dir + sep + "icons" + sep + SF_ICON_NAME;
  sf_host_config cfg;
  memset(&cfg, 0, sizeof(cfg));
  cfg.screen_w = (int)g_hd->screen_w;
  cfg.screen_h = (int)g_hd->screen_h;
  cfg.install_dir = dir.c_str();
  cfg.exe_name = SF_EXE_NAME;
  cfg.zap_archive = zap.c_str();
  cfg.icon_file = icon.c_str();
  cfg.emulate_nt = (int)g_hd->emulate_nt;
  std::vector<char *> av;
  for (auto &s : progArgs) av.push_back(&s[0]);
  if (sf_host_start(&cfg, sf_program_main, (int)av.size(), av.data()) != 0) {
    g_hd->state = SF_IPC_FAILED;
    return 5;
  }
  sf_ipc_store((volatile uint32_t *)&g_hd->state, SF_IPC_RUNNING);

  unsigned serial = 0;
  int idle = 0;
  for (;;) {
    handle_input();
    if (sf_ipc_load(&g_hd->frame_ack) == g_hd->frame_written) {
      if (sf_host_frame(g_pixels, &serial))
        sf_ipc_store(&g_hd->frame_written, g_hd->frame_written + 1);
    }
    g_hd->cursor = sf_host_cursor();
    pump_audio();
    int wx, wy;
    while (sf_host_take_warp(&wx, &wy)) { int32_t v[2] = {wx, wy}; out_msg(SF_OUT_WARP, v, 8); }
    if (!sf_host_running()) {
      int32_t code = sf_host_exit_code();
      pump_audio();
      out_msg(SF_OUT_EXITED, &code, 4);
      g_hd->exit_code = code;
      sf_ipc_store((volatile uint32_t *)&g_hd->state, SF_IPC_EXITED);
      break;
    }
    if (g_parentGone || (g_parent && (++idle % 500) == 0 && !sfipc::parent_alive(g_parent))) break;
    sfipc::sleep_ms(1);
  }
  fflush(stdout);
  /* the program's other threads may still be running */
  _Exit(0);
}
