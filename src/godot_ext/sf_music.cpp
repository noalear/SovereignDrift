/* sf_music.cpp -- NEW FILE (2026-10-12), Stellar Frontier Godot port.
   See sf_music.h. */
#include "sf_music.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <mmsystem.h>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>

namespace sfmusic {
namespace {

struct Cmd { int op; std::string path; bool restart; };   /* 1 play, 2 stop, 3 quit */
std::mutex g_mx;
std::condition_variable g_cv;
std::deque<Cmd> g_q;
std::thread g_thread;
bool g_started = false;

void worker()
{
  std::string openPath;
  bool isOpen = false;
  for (;;) {
    Cmd c;
    {
      std::unique_lock<std::mutex> lk(g_mx);
      g_cv.wait(lk, [] { return !g_q.empty(); });
      c = g_q.back();                       /* only the newest request matters */
      for (auto &q : g_q) if (q.op == 1 && q.restart && c.op == 1) c.restart = true;
      g_q.clear();
    }
    if (c.op == 3) break;
    if (c.op == 2) {
      if (isOpen) mciSendStringA("stop sfmusic", nullptr, 0, nullptr);
      continue;
    }
    bool restart = c.restart;
    if (!isOpen || _stricmp(c.path.c_str(), openPath.c_str()) != 0) {
      if (isOpen) mciSendStringA("close sfmusic", nullptr, 0, nullptr);
      std::string cmd = "open \"" + c.path + "\" type sequencer alias sfmusic";
      isOpen = mciSendStringA(cmd.c_str(), nullptr, 0, nullptr) == 0;
      openPath = c.path;
      restart = true;
    }
    if (isOpen) mciSendStringA(restart ? "play sfmusic from 0" : "play sfmusic", nullptr, 0, nullptr);
  }
  if (isOpen) {
    mciSendStringA("stop sfmusic", nullptr, 0, nullptr);
    mciSendStringA("close sfmusic", nullptr, 0, nullptr);
  }
}

void post(Cmd c)
{
  {
    std::lock_guard<std::mutex> lk(g_mx);
    if (!g_started) { g_thread = std::thread(worker); g_started = true; }
    g_q.push_back(std::move(c));
  }
  g_cv.notify_one();
}

}  /* namespace */

void play(const std::string &dir, const std::string &path, bool restart)
{
  /* "..\mid\x.mid" relative to the program's working directory */
  std::string p = path;
  bool absolute = p.size() > 2 && (p[1] == ':' || (p[0] == '\\' && p[1] == '\\'));
  std::string full = absolute ? p : dir + "\\" + p;
  char buf[MAX_PATH];
  if (GetFullPathNameA(full.c_str(), MAX_PATH, buf, nullptr)) full = buf;
  post(Cmd{1, full, restart});
}

void stop() { post(Cmd{2, "", false}); }

void shutdown()
{
  {
    std::lock_guard<std::mutex> lk(g_mx);
    if (!g_started) return;
  }
  post(Cmd{3, "", false});
  if (g_thread.joinable()) g_thread.join();
  g_started = false;
}

}  /* namespace sfmusic */

#else

namespace sfmusic {
void play(const std::string &, const std::string &, bool) {}
void stop() {}
void shutdown() {}
}

#endif
