/* ************************************************************************* */
/* w32_kernel.cpp -- kernel32/msvcrt/shell/winsock parts of the Win32        */
/* emulation: handles, threads, synchronisation, time, files, processes.     */
/*                                                                           */
/* NEW FILE, added 2026-10-12 for the Stellar Frontier Godot port.           */
/* ************************************************************************* */
#include "w32_internal.h"
#include "sf_fs.h"
#include "sf_net.h"

#include <thread>
#include <chrono>
#include <unordered_map>
#include <filesystem>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <sys/stat.h>
#ifdef _WIN32
#  include <process.h>
#  include <sys/utime.h>
#else
#  include <unistd.h>
#  include <utime.h>
#  include <sys/mman.h>
#endif

namespace fs = std::filesystem;
using namespace w32;

/* ================================ handles ================================ */
namespace w32 {

static std::mutex g_hmtx;
static std::unordered_map<uintptr_t, Obj *> g_handles W32_EARLY;
static uintptr_t g_next_handle = 0x10000;

uintptr_t hnew(Obj *o, HType t)
{
  std::lock_guard<std::mutex> lk(g_hmtx);
  uintptr_t h = g_next_handle;
  g_next_handle += 4;           /* never reused: stale handles stay invalid */
  o->type = t;
  o->handle = h;
  g_handles[h] = o;
  return h;
}

Obj *hget(const void *h, HType t)
{
  uintptr_t v = (uintptr_t)h;
  if (v < 0x10000) return nullptr;
  std::lock_guard<std::mutex> lk(g_hmtx);
  auto it = g_handles.find(v);
  if (it == g_handles.end()) return nullptr;
  if (t != HT_NONE && it->second->type != t) return nullptr;
  return it->second;
}

Obj *hget_any(const void *h) { return hget(h, HT_NONE); }

void hdel(const void *h)
{
  std::lock_guard<std::mutex> lk(g_hmtx);
  g_handles.erase((uintptr_t)h);
}

}  /* namespace w32 */

/* =============================== last error ============================== */
static thread_local DWORD t_last_error = 0;
DWORD GetLastError(void) { return t_last_error; }
void  SetLastError(DWORD e) { t_last_error = e; }

/* ============================ waitable objects =========================== */
namespace {

std::mutex g_kmtx;
std::condition_variable g_kcv W32_EARLY;

struct Waitable : Obj {
  virtual bool signaled(DWORD tid) = 0;
  virtual void acquire(DWORD) {}
};

struct ThreadObj : Waitable {
  DWORD tid = 0;
  bool done = false;
  std::atomic<bool> kill{false};
  DWORD exitCode = STILL_ACTIVE;
  bool autoClose = false;
  bool signaled(DWORD) override { return done || kill; }
};

struct MutexObj : Waitable {
  DWORD owner = 0;
  int count = 0;
  bool signaled(DWORD tid) override { return owner == 0 || owner == tid; }
  void acquire(DWORD tid) override { owner = tid; count++; }
};

struct EventObj : Waitable {
  bool manual = true;
  bool state = false;
  bool signaled(DWORD) override { return state; }
  void acquire(DWORD) override { if (!manual) state = false; }
};

struct ProcessObj : Waitable {
  bool signaled(DWORD) override { return true; }
};

struct GlobalObj : Obj { std::vector<char> mem; };
struct FileObj : Obj { std::string path; };
struct FindObj : Obj { std::vector<std::string> names; size_t next = 0; std::string dir; };
struct ModuleObj : Obj { std::string name; };
struct TokenObj : Obj {};

std::atomic<DWORD> g_next_tid{1000};
thread_local DWORD t_tid = 0;
thread_local ThreadObj *t_self = nullptr;

struct ThreadExit {};

const auto g_t0 = std::chrono::steady_clock::now();

int64_t now_us()
{
  return std::chrono::duration_cast<std::chrono::microseconds>(
           std::chrono::steady_clock::now() - g_t0).count();
}

}  /* namespace */

namespace w32 {

DWORD cur_tid()
{
  if (!t_tid) t_tid = g_next_tid.fetch_add(4);
  return t_tid;
}

void check_terminated()
{
  if (t_self && t_self->kill.load())
    throw ThreadTerminated();
}

}  /* namespace w32 */

DWORD GetCurrentThreadId(void) { return cur_tid(); }
DWORD GetCurrentProcessId(void) { return 0x5F0; }
HANDLE GetCurrentThread(void) { return (HANDLE)(LONG_PTR)-2; }
HANDLE GetCurrentProcess(void) { return (HANDLE)(LONG_PTR)-1; }

/* ================================ threads ================================ */
bool w32::trace(char what)
{
  static const char *t = getenv("SF_W32_TRACE");
  return t && strchr(t, what);
}

void w32::fpu_msvc_default()
{
#if defined(__i386__)
  unsigned short cw = 0x027F;
  __asm__ volatile("fldcw %0" : : "m"(cw));
#endif
}

static HANDLE start_thread(std::function<void()> body, bool autoClose, LPDWORD tidOut)
{
  ThreadObj *t = new ThreadObj();
  t->autoClose = autoClose;
  t->tid = g_next_tid.fetch_add(4);
  HANDLE h = (HANDLE)hnew(t, HT_THREAD);
  if (tidOut) *tidOut = t->tid;
  try {
    std::thread th([t, body]() {
      t_tid = t->tid;
      t_self = t;
      fpu_msvc_default();
      DWORD code = 0;
      try { body(); }
      catch (ThreadExit &) {}
      catch (ThreadTerminated &) { code = 0; }
      {
        std::lock_guard<std::mutex> lk(g_kmtx);
        if (!t->kill) t->exitCode = code;
        t->done = true;
        if (t->autoClose) hdel((void *)t->handle);   /* _endthread closes its handle */
      }
      g_kcv.notify_all();
      /* The ThreadObj is intentionally not freed: a waiter may still hold it. */
    });
    th.detach();
  } catch (...) {
    hdel(h);
    delete t;
    return nullptr;
  }
  return h;
}

uintptr_t _beginthread(void (*fn)(void *), unsigned, void *arg)
{
  HANDLE h = start_thread([fn, arg]() { fn(arg); }, true, nullptr);
  return h ? (uintptr_t)h : (uintptr_t)-1;
}

void _endthread(void) { throw ThreadExit(); }

HANDLE CreateThread(void *, size_t, LPTHREAD_START_ROUTINE fn, LPVOID arg, DWORD, LPDWORD tid)
{
  return start_thread([fn, arg]() { fn(arg); }, false, tid);
}

void ExitThread(DWORD) { throw ThreadExit(); }

BOOL TerminateThread(HANDLE h, DWORD code)
{
  /* Standard C++ cannot kill a thread.  The thread is marked killed: its
     handle becomes signalled at once, and the next time the thread enters
     the emulated API (Sleep, waits, messages...) it unwinds and exits. */
  ThreadObj *t = H<ThreadObj>(h, HT_THREAD);
  if (!t) { SetLastError(ERROR_INVALID_HANDLE); return FALSE; }
  {
    std::lock_guard<std::mutex> lk(g_kmtx);
    t->exitCode = code;
    t->kill = true;
  }
  g_kcv.notify_all();
  w32::wake_all();
  return TRUE;
}

DWORD SuspendThread(HANDLE) { return 0; }
DWORD ResumeThread(HANDLE) { return 0; }
BOOL  SetThreadPriority(HANDLE, int) { return TRUE; }
int   GetThreadPriority(HANDLE) { return THREAD_PRIORITY_NORMAL; }

/* ============================ synchronisation ============================ */
HANDLE CreateMutex(void *, BOOL initialOwner, LPCSTR)
{
  MutexObj *m = new MutexObj();
  if (initialOwner) m->acquire(cur_tid());
  return (HANDLE)hnew(m, HT_MUTEX);
}

BOOL ReleaseMutex(HANDLE h)
{
  MutexObj *m = H<MutexObj>(h, HT_MUTEX);
  if (!m) { SetLastError(ERROR_INVALID_HANDLE); return FALSE; }
  {
    std::lock_guard<std::mutex> lk(g_kmtx);
    if (m->owner != cur_tid()) { SetLastError(288 /*ERROR_NOT_OWNER*/); return FALSE; }
    if (--m->count == 0) m->owner = 0;
  }
  g_kcv.notify_all();
  return TRUE;
}

HANDLE CreateEvent(void *, BOOL manual, BOOL initial, LPCSTR)
{
  EventObj *e = new EventObj();
  e->manual = manual != 0;
  e->state = initial != 0;
  return (HANDLE)hnew(e, HT_EVENT);
}

static BOOL set_event_state(HANDLE h, int op)
{
  EventObj *e = H<EventObj>(h, HT_EVENT);
  if (!e) { SetLastError(ERROR_INVALID_HANDLE); return FALSE; }
  {
    std::lock_guard<std::mutex> lk(g_kmtx);
    if (op == 0) e->state = false;
    else e->state = true;
  }
  g_kcv.notify_all();
  if (op == 2) {   /* PulseEvent: release waiters, then reset */
    std::this_thread::yield();
    std::lock_guard<std::mutex> lk(g_kmtx);
    e->state = false;
  }
  return TRUE;
}
BOOL SetEvent(HANDLE h)   { return set_event_state(h, 1); }
BOOL ResetEvent(HANDLE h) { return set_event_state(h, 0); }
BOOL PulseEvent(HANDLE h) { return set_event_state(h, 2); }

static Waitable *as_waitable(HANDLE h)
{
  Obj *o = hget_any(h);
  if (!o) return nullptr;
  switch (o->type) {
  case HT_THREAD: case HT_MUTEX: case HT_EVENT: case HT_PROCESS:
    return static_cast<Waitable *>(o);
  default:
    return nullptr;
  }
}

DWORD WaitForMultipleObjects(DWORD n, const HANDLE *hs, BOOL all, DWORD ms)
{
  check_terminated();
  std::vector<Waitable *> ws;
  for (DWORD i = 0; i < n; i++) {
    Waitable *w = as_waitable(hs[i]);
    if (!w) { SetLastError(ERROR_INVALID_HANDLE); return WAIT_FAILED; }
    ws.push_back(w);
  }
  DWORD tid = cur_tid();
  auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(ms);
  std::unique_lock<std::mutex> lk(g_kmtx);
  for (;;) {
    if (all) {
      bool ok = true;
      for (auto *w : ws) ok = ok && w->signaled(tid);
      if (ok) { for (auto *w : ws) w->acquire(tid); return WAIT_OBJECT_0; }
    } else {
      for (DWORD i = 0; i < n; i++)
        if (ws[i]->signaled(tid)) { ws[i]->acquire(tid); return WAIT_OBJECT_0 + i; }
    }
    if (t_self && t_self->kill) { lk.unlock(); check_terminated(); }
    if (ms == 0) return WAIT_TIMEOUT;
    if (ms == INFINITE)
      g_kcv.wait_for(lk, std::chrono::milliseconds(50));
    else if (g_kcv.wait_until(lk, deadline) == std::cv_status::timeout) {
      /* one last look */
      for (DWORD i = 0; i < n; i++)
        if (!all && ws[i]->signaled(tid)) { ws[i]->acquire(tid); return WAIT_OBJECT_0 + i; }
      if (all) {
        bool ok = true;
        for (auto *w : ws) ok = ok && w->signaled(tid);
        if (ok) { for (auto *w : ws) w->acquire(tid); return WAIT_OBJECT_0; }
      }
      return WAIT_TIMEOUT;
    }
  }
}

DWORD WaitForSingleObject(HANDLE h, DWORD ms)
{
  if (!as_waitable(h)) { SetLastError(ERROR_INVALID_HANDLE); return WAIT_FAILED; }
  return WaitForMultipleObjects(1, &h, FALSE, ms);
}

BOOL CloseHandle(HANDLE h)
{
  Obj *o = hget_any(h);
  if (!o) { SetLastError(ERROR_INVALID_HANDLE); return FALSE; }
  switch (o->type) {
  case HT_THREAD: case HT_MUTEX: case HT_EVENT: case HT_PROCESS:
    hdel(h);        /* waitables are not freed: another thread may be waiting */
    return TRUE;
  case HT_FILE: case HT_FIND: case HT_TOKEN: case HT_GLOBAL:
    hdel(h);
    delete o;
    return TRUE;
  default:
    SetLastError(ERROR_INVALID_HANDLE);
    return FALSE;
  }
}

/* ================================== time ================================= */
void Sleep(DWORD ms)
{
  check_terminated();
  if (ms == INFINITE) ms = 0x7FFFFFFF;
  if (ms == 0) { std::this_thread::yield(); return; }
  /* Sleep in slices so TerminateThread() is noticed. */
  int64_t end = now_us() + (int64_t)ms * 1000;
  for (;;) {
    int64_t left = end - now_us();
    if (left <= 0) break;
    std::this_thread::sleep_for(std::chrono::microseconds(std::min<int64_t>(left, 100000)));
    check_terminated();
  }
}

DWORD GetTickCount(void) { return (DWORD)(now_us() / 1000 + 3600000); }
DWORD timeGetTime(void)  { return (DWORD)(now_us() / 1000 + 3600000); }

/* The ACPI power-management timer rate used by most Windows 2000/XP PCs. */
static const int64_t QPF = 3579545;

BOOL QueryPerformanceFrequency(LARGE_INTEGER *f)
{
  f->QuadPart = QPF;
  return TRUE;
}

BOOL QueryPerformanceCounter(LARGE_INTEGER *c)
{
  int64_t us = now_us() + 3600000000LL;
  /* us * QPF / 1e6 without 64-bit overflow (no __int128 on 32-bit) */
  int64_t sec = us / 1000000, rem = us % 1000000;
  c->QuadPart = sec * QPF + rem * QPF / 1000000;
  return TRUE;
}

/* ============================= MSVC C runtime ============================ */
static thread_local uint32_t t_holdrand = 1;   /* per thread, as with /MT */

/* MSVC string copies: front to back, so overlapping copies to a lower
   address behave as they did on Windows; memcpy is memmove. */
extern "C" char *sf_msvc_strcpy(char *d, const char *s)
{
  char *r = d;
  while ((*d++ = *s++) != 0) {}
  return r;
}

extern "C" char *sf_msvc_strcat(char *d, const char *s)
{
  char *r = d;
  while (*d) d++;
  while ((*d++ = *s++) != 0) {}
  return r;
}

extern "C" char *sf_msvc_strncpy(char *d, const char *s, size_t n)
{
  char *r = d;
  while (n && (*d++ = *s++) != 0) n--;
  if (n) while (--n) *d++ = 0;
  return r;
}

extern "C" char *sf_msvc_strncat(char *d, const char *s, size_t n)
{
  char *r = d;
  while (*d) d++;
  while (n-- && (*d = *s++) != 0) d++;
  *d = 0;
  return r;
}

extern "C" void *sf_msvc_memcpy(void *d, const void *s, size_t n) { return memmove(d, s, n); }

extern "C" int sf_msvc_rand(void)
{
  t_holdrand = t_holdrand * 214013u + 2531011u;
  return (int)((t_holdrand >> 16) & 0x7fff);
}

extern "C" void sf_msvc_srand(unsigned int s) { t_holdrand = s; }

extern "C" clock_t sf_msvc_clock(void) { return (clock_t)(now_us() / 1000); }

#if defined(__LP64__) || defined(_LP64)
/* Drop the 'l' length modifier from integer conversions: every 'long' of
   the 1997 program is 32 bits.  "%ll" and "%lf"/"%le"/"%lg" are kept. */
static const char *fmt32(const char *f, std::string &buf)
{
  if (!f || !strchr(f, 'l')) return f;
  buf.clear();
  for (const char *p = f; *p; p++) {
    buf.push_back(*p);
    if (*p != '%') continue;
    p++;
    if (*p == '%') { buf.push_back('%'); continue; }
    /* flags, width, precision */
    while (*p && strchr("-+ #0'*.0123456789", *p)) buf.push_back(*p++);
    if (*p == 'l' && p[1] != 'l' && p[1] && strchr("diouxXn", p[1])) {
      p++;                     /* skip the single 'l' */
    }
    if (!*p) break;
    buf.push_back(*p);
  }
  return buf.c_str();
}

extern "C" int sf_vfprintf(FILE *f, const char *fmt, va_list ap)
{ std::string b; return vfprintf(f, fmt32(fmt, b), ap); }
extern "C" int sf_vsprintf(char *s, const char *fmt, va_list ap)
{ std::string b; return vsprintf(s, fmt32(fmt, b), ap); }
extern "C" int sf_vsnprintf(char *s, size_t n, const char *fmt, va_list ap)
{ std::string b; return vsnprintf(s, n, fmt32(fmt, b), ap); }
extern "C" int sf_printf(const char *fmt, ...)
{ va_list ap; va_start(ap, fmt); int r = sf_vfprintf(stdout, fmt, ap); va_end(ap); return r; }
extern "C" int sf_fprintf(FILE *f, const char *fmt, ...)
{ va_list ap; va_start(ap, fmt); int r = sf_vfprintf(f, fmt, ap); va_end(ap); return r; }
extern "C" int sf_sprintf(char *s, const char *fmt, ...)
{ va_list ap; va_start(ap, fmt); int r = sf_vsprintf(s, fmt, ap); va_end(ap); return r; }
extern "C" int sf_snprintf(char *s, size_t n, const char *fmt, ...)
{ va_list ap; va_start(ap, fmt); int r = sf_vsnprintf(s, n, fmt, ap); va_end(ap); return r; }
extern "C" int sf_sscanf(const char *s, const char *fmt, ...)
{ std::string b; va_list ap; va_start(ap, fmt); int r = vsscanf(s, fmt32(fmt, b), ap); va_end(ap); return r; }
extern "C" int sf_fscanf(FILE *f, const char *fmt, ...)
{ std::string b; va_list ap; va_start(ap, fmt); int r = vfscanf(f, fmt32(fmt, b), ap); va_end(ap); return r; }
#endif

/* ================================= files ================================= */
/* The program sees its install as C:\Stellar Frontier\... */
static const char *FAKE_ROOT = "C:\\Stellar Frontier";

extern "C" const char *sf_hostpath(const char *path, char *buf, int buflen)
{
  if (!path) return path;
  std::string p(path);
  /* "nul" device */
  {
    std::string l = p;
    for (auto &c : l) c = (char)tolower((unsigned char)c);
    if (l == "nul" || l == "nul:") {
#ifdef _WIN32
      snprintf(buf, buflen, "nul");
#else
      snprintf(buf, buflen, "/dev/null");
#endif
      return buf;
    }
    /* the program's own executable holds its zap archive */
    std::string base = l.substr(l.find_last_of("\\/") == std::string::npos ? 0 : l.find_last_of("\\/") + 1);
    std::string exe = w32::host_exe_name();
    for (auto &c : exe) c = (char)tolower((unsigned char)c);
    if (!exe.empty() && base == exe) {
      std::string z = w32::host_zap_path();
      snprintf(buf, buflen, "%s", z.c_str());
      return buf;
    }
    /* absolute paths under the fake install root */
    std::string fr = FAKE_ROOT;
    for (auto &c : fr) c = (char)tolower((unsigned char)c);
    if (l.compare(0, fr.size(), fr) == 0)
      p = w32::host_install_dir() + p.substr(fr.size());
  }
#ifdef _WIN32
  snprintf(buf, buflen, "%s", p.c_str());
  return buf;
#else
  for (auto &c : p) if (c == '\\') c = '/';
  return sf_fs_resolve_case(p.c_str(), buf, buflen);
#endif
}

/* Text-mode reading as the Microsoft C runtime does it: CR LF -> LF, and
   Ctrl-Z ends the file.  POSIX hosts get a translated in-memory copy. */
extern "C" int sf_text_copy(const char *hostpath, char *buf, int buflen)
{
#ifdef _WIN32
  (void)hostpath; (void)buf; (void)buflen;
  return -1;                    /* the C runtime translates by itself */
#else
  FILE *f = fopen(hostpath, "rb");
  if (!f) return -1;
  std::string d;
  char tmp[65536];
  size_t n;
  while ((n = fread(tmp, 1, sizeof(tmp), f)) > 0) d.append(tmp, n);
  fclose(f);
  if (d.find('\r') == std::string::npos && d.find('\x1a') == std::string::npos) return -1;
  std::string o;
  o.reserve(d.size());
  for (size_t i = 0; i < d.size(); i++) {
    char c = d[i];
    if (c == '\x1a') break;
    if (c == '\r' && i + 1 < d.size() && d[i + 1] == '\n') continue;
    o += c;
  }
  int fd = memfd_create("sf_text", MFD_CLOEXEC);
  if (fd < 0) return -1;
  size_t off = 0;
  while (off < o.size()) {
    ssize_t w = ::write(fd, o.data() + off, o.size() - off);
    if (w <= 0) { ::close(fd); return -1; }
    off += (size_t)w;
  }
  lseek(fd, 0, SEEK_SET);
  snprintf(buf, buflen, "/proc/self/fd/%d", fd);
  return fd;
#endif
}

extern "C" void sf_text_done(int fd)
{
#ifndef _WIN32
  if (fd >= 0) ::close(fd);
#endif
}

extern "C" FILE *sf_fopen(const char *path, const char *mode)
{
  char b[1024];
  const char *hp = sf_hostpath(path, b, sizeof(b));
#ifndef _WIN32
  if (mode && mode[0] == 'r' && !strchr(mode, 'b') && !strchr(mode, '+')) {
    char t[64];
    int fd = sf_text_copy(hp, t, sizeof(t));
    if (fd >= 0) {
      FILE *f = fopen(t, mode);
      sf_text_done(fd);
      return f;
    }
  }
#endif
  return fopen(hp, mode);
}

extern "C" const char *sf_host_path(const char *winpath, char *buf, int buflen)
{
  return sf_hostpath(winpath, buf, buflen);
}

static std::string hp(LPCSTR p)
{
  char b[1024];
  return std::string(sf_hostpath(p, b, sizeof(b)));
}

static void fill_find(FindObj *f, LPWIN32_FIND_DATA d)
{
  const std::string &full = f->names[f->next++];
  memset(d, 0, sizeof(*d));
  std::string name = full.substr(full.find_last_of('/') == std::string::npos ? 0 : full.find_last_of('/') + 1);
  snprintf(d->cFileName, sizeof(d->cFileName), "%s", name.c_str());
  std::error_code ec;
  fs::path fp(full);
  if (fs::is_directory(fp, ec)) d->dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
  else {
    d->dwFileAttributes = FILE_ATTRIBUTE_ARCHIVE;
    uintmax_t sz = fs::file_size(fp, ec);
    if (!ec) { d->nFileSizeLow = (DWORD)sz; d->nFileSizeHigh = (DWORD)(sz >> 32); }
  }
}

HANDLE FindFirstFile(LPCSTR pattern, LPWIN32_FIND_DATA d)
{
  std::string pat = hp(pattern);
  /* hp() resolved the directory part; glob the last component */
  char **list = nullptr;
  int n = sf_fs_glob(pat.c_str(), SF_FS_FILE | SF_FS_DIR, &list);
  FindObj *f = new FindObj();
  /* Win32 also lists "." and ".." for "*" style masks in subdirectories;
     the program filters them, and the slib code skips them, so they are
     reported as on NTFS for "*" and "*.*". */
  std::string last = pat.substr(pat.find_last_of('/') == std::string::npos ? 0 : pat.find_last_of('/') + 1);
  std::string dir = pat.substr(0, pat.size() - last.size());
  if (last == "*" || last == "*.*") {
    f->names.push_back(dir + ".");
    f->names.push_back(dir + "..");
  }
  for (int i = 0; i < n; i++) f->names.push_back(list[i]);
  sf_fs_glob_free(list, n);
  if (f->names.empty()) {
    delete f;
    SetLastError(ERROR_FILE_NOT_FOUND);
    return INVALID_HANDLE_VALUE;
  }
  HANDLE h = (HANDLE)hnew(f, HT_FIND);
  fill_find(f, d);
  return h;
}

BOOL FindNextFile(HANDLE h, LPWIN32_FIND_DATA d)
{
  FindObj *f = H<FindObj>(h, HT_FIND);
  if (!f) { SetLastError(ERROR_INVALID_HANDLE); return FALSE; }
  if (f->next >= f->names.size()) { SetLastError(ERROR_NO_MORE_FILES); return FALSE; }
  fill_find(f, d);
  return TRUE;
}

BOOL FindClose(HANDLE h)
{
  FindObj *f = H<FindObj>(h, HT_FIND);
  if (!f) return FALSE;
  hdel(h);
  delete f;
  return TRUE;
}

HANDLE CreateFile(LPCSTR name, DWORD, DWORD, void *, DWORD disp, DWORD, HANDLE)
{
  std::string p = hp(name);
  std::error_code ec;
  if (disp == OPEN_EXISTING && !fs::exists(p, ec)) {
    SetLastError(ERROR_FILE_NOT_FOUND);
    return INVALID_HANDLE_VALUE;
  }
  if (disp == CREATE_ALWAYS || disp == CREATE_NEW || disp == OPEN_ALWAYS) {
    FILE *f = fopen(p.c_str(), disp == OPEN_ALWAYS ? "ab" : "wb");
    if (f) fclose(f);
  }
  FileObj *o = new FileObj();
  o->path = p;
  return (HANDLE)hnew(o, HT_FILE);
}

static void unix_to_ft(time_t t, FILETIME *ft)
{
  uint64_t v = (uint64_t)((int64_t)t * 10000000LL + 116444736000000000LL);
  ft->dwLowDateTime = (DWORD)v;
  ft->dwHighDateTime = (DWORD)(v >> 32);
}

static time_t ft_to_unix(const FILETIME *ft)
{
  uint64_t v = ((uint64_t)ft->dwHighDateTime << 32) | ft->dwLowDateTime;
  return (time_t)(((int64_t)v - 116444736000000000LL) / 10000000LL);
}

BOOL GetFileTime(HANDLE h, FILETIME *c, FILETIME *a, FILETIME *w)
{
  FileObj *f = H<FileObj>(h, HT_FILE);
  if (!f) { SetLastError(ERROR_INVALID_HANDLE); return FALSE; }
  struct stat st;
  if (stat(f->path.c_str(), &st) != 0) return FALSE;
  if (c) unix_to_ft(st.st_mtime, c);   /* creation time is not portable */
  if (a) unix_to_ft(st.st_atime, a);
  if (w) unix_to_ft(st.st_mtime, w);
  return TRUE;
}

BOOL SetFileTime(HANDLE h, const FILETIME *c, const FILETIME *a, const FILETIME *w)
{
  FileObj *f = H<FileObj>(h, HT_FILE);
  if (!f) { SetLastError(ERROR_INVALID_HANDLE); return FALSE; }
  struct stat st;
  if (stat(f->path.c_str(), &st) != 0) return FALSE;
#ifdef _WIN32
  struct _utimbuf ub;
#else
  struct utimbuf ub;
#endif
  ub.actime = a ? ft_to_unix(a) : st.st_atime;
  ub.modtime = w ? ft_to_unix(w) : (c ? ft_to_unix(c) : st.st_mtime);
#ifdef _WIN32
  return _utime(f->path.c_str(), &ub) == 0;
#else
  return utime(f->path.c_str(), &ub) == 0;
#endif
}

BOOL SetFileAttributes(LPCSTR name, DWORD attr)
{
  std::error_code ec;
  std::string p = hp(name);
  if (!fs::exists(p, ec)) { SetLastError(ERROR_FILE_NOT_FOUND); return FALSE; }
  fs::perms pm = fs::status(p, ec).permissions();
  if (attr & FILE_ATTRIBUTE_READONLY)
    pm &= ~(fs::perms::owner_write | fs::perms::group_write | fs::perms::others_write);
  else
    pm |= fs::perms::owner_write;
  fs::permissions(p, pm, ec);
  return !ec;
}

DWORD GetFileAttributes(LPCSTR name)
{
  std::error_code ec;
  std::string p = hp(name);
  if (!fs::exists(p, ec)) return 0xFFFFFFFF;
  return fs::is_directory(p, ec) ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_ARCHIVE;
}

BOOL CopyFile(LPCSTR from, LPCSTR to, BOOL failIfExists)
{
  std::error_code ec;
  fs::copy_file(hp(from), hp(to),
                failIfExists ? fs::copy_options::none : fs::copy_options::overwrite_existing, ec);
  return !ec;
}

BOOL DeleteFile(LPCSTR name)
{
  std::error_code ec;
  std::string p = hp(name);
  if (fs::is_directory(p, ec)) return FALSE;
  return fs::remove(p, ec) && !ec;
}

BOOL MoveFile(LPCSTR from, LPCSTR to)
{
  std::error_code ec;
  fs::rename(hp(from), hp(to), ec);
  return !ec;
}

BOOL CreateDirectory(LPCSTR name, void *)
{
  std::error_code ec;
  std::string p = hp(name);
  if (fs::exists(p, ec)) { SetLastError(ERROR_ALREADY_EXISTS); return FALSE; }
  return fs::create_directory(p, ec) && !ec;
}

BOOL RemoveDirectory(LPCSTR name)
{
  std::error_code ec;
  std::string p = hp(name);
  if (!fs::is_directory(p, ec)) return FALSE;
  return fs::remove(p, ec) && !ec;
}

DWORD GetCurrentDirectory(DWORD n, LPSTR buf)
{
  std::error_code ec;
  std::string cwd = fs::current_path(ec).string();
  std::string inst = w32::host_install_dir();
  std::string out;
  if (!inst.empty() && cwd.compare(0, inst.size(), inst) == 0)
    out = std::string(FAKE_ROOT) + cwd.substr(inst.size());
  else
    out = cwd;
  for (auto &c : out) if (c == '/') c = '\\';
  if (buf && n > out.size()) strcpy(buf, out.c_str());
  return (DWORD)out.size();
}

BOOL SetCurrentDirectory(LPCSTR dir)
{
  std::error_code ec;
  fs::current_path(hp(dir), ec);
  return !ec;
}

/* ============================ system and process ========================= */
static sf_host_callbacks g_cb;

namespace w32 {
sf_host_callbacks &host_callbacks() { return g_cb; }
}

extern "C" void sf_host_set_callbacks(const sf_host_callbacks *cb)
{
  if (cb) g_cb = *cb;
}

static int g_emulate_nt = 1;
extern "C" void sf_w32_set_nt(int nt) { g_emulate_nt = nt; }

DWORD GetVersion(void)
{
  if (g_emulate_nt)
    return 0x0A280105;          /* Windows XP, build 2600 */
  return 0xC0000A04;            /* Windows 98 */
}

BOOL GetVersionEx(LPOSVERSIONINFO v)
{
  if (!v) return FALSE;
  if (g_emulate_nt) {
    v->dwMajorVersion = 5; v->dwMinorVersion = 1; v->dwBuildNumber = 2600;
    v->dwPlatformId = VER_PLATFORM_WIN32_NT;
    snprintf(v->szCSDVersion, sizeof(v->szCSDVersion), "Service Pack 1");
  } else {
    v->dwMajorVersion = 4; v->dwMinorVersion = 10; v->dwBuildNumber = 0x040A08AE;
    v->dwPlatformId = VER_PLATFORM_WIN32_WINDOWS;
    snprintf(v->szCSDVersion, sizeof(v->szCSDVersion), " A ");
  }
  return TRUE;
}

void GlobalMemoryStatus(LPMEMORYSTATUS ms)
{
  /* A comfortable 2002 machine: 512 MB RAM, 1 GB page file. */
  ms->dwMemoryLoad = 30;
  ms->dwTotalPhys = 512u << 20;
  ms->dwAvailPhys = 300u << 20;
  ms->dwTotalPageFile = 1024u << 20;
  ms->dwAvailPageFile = 800u << 20;
  ms->dwTotalVirtual = 2047u << 20;
  ms->dwAvailVirtual = 1900u << 20;
}

void GetSystemInfo(LPSYSTEM_INFO si)
{
  memset(si, 0, sizeof(*si));
  si->dwPageSize = 4096;
  si->dwNumberOfProcessors = 1;   /* the game targets single-CPU machines */
  si->dwProcessorType = 586;
  si->dwAllocationGranularity = 65536;
  si->wProcessorLevel = 6;
}

static UINT copy_out(const char *s, LPSTR buf, UINT n)
{
  UINT len = (UINT)strlen(s);
  if (buf && n > len) strcpy(buf, s);
  return len;
}

UINT GetSystemDirectory(LPSTR buf, UINT n)
{
  return copy_out(g_emulate_nt ? "C:\\WINDOWS\\system32" : "C:\\WINDOWS\\SYSTEM", buf, n);
}

UINT GetWindowsDirectory(LPSTR buf, UINT n) { return copy_out("C:\\WINDOWS", buf, n); }

DWORD GetModuleFileName(HMODULE, LPSTR buf, DWORD n)
{
  std::string p = std::string(FAKE_ROOT) + "\\binwin\\" + w32::host_exe_name();
  return copy_out(p.c_str(), buf, n);
}

BOOL SetEnvironmentVariable(LPCSTR name, LPCSTR value)
{
#ifdef _WIN32
  return _putenv_s(name, value ? value : "") == 0;
#else
  return value ? setenv(name, value, 1) == 0 : unsetenv(name) == 0;
#endif
}

void OutputDebugString(LPCSTR s) { if (s) w32::host_log("%s", s); }

HMODULE LoadLibrary(LPCSTR name)
{
  if (!name) return nullptr;
  std::string l(name);
  for (auto &c : l) c = (char)tolower((unsigned char)c);
  /* DirectSound is present; DirectDraw is reported missing so the program
     uses its GDI drawing path (it only asks for it with S_ROOT_USEDIRECTDRAW). */
  if (l == "dsound.dll") {
    ModuleObj *m = new ModuleObj();
    m->name = l;
    return (HMODULE)hnew(m, HT_MODULE);
  }
  SetLastError(126 /*ERROR_MOD_NOT_FOUND*/);
  return nullptr;
}

BOOL FreeLibrary(HMODULE h)
{
  ModuleObj *m = H<ModuleObj>(h, HT_MODULE);
  if (!m) return FALSE;
  hdel(h);
  delete m;
  return TRUE;
}

FARPROC GetProcAddress(HMODULE h, LPCSTR name)
{
  ModuleObj *m = H<ModuleObj>(h, HT_MODULE);
  if (!m || !name) return nullptr;
  if (m->name == "dsound.dll" && strcmp(name, "DirectSoundCreate") == 0)
    return (FARPROC)(void *)&DirectSoundCreate;
  return nullptr;
}

HRESULT WINAPI DirectSoundCreate(LPCGUID, LPDIRECTSOUND *out, LPUNKNOWN)
{
  return w32::dsound_create(out);
}

HGLOBAL GlobalAlloc(UINT flags, size_t n)
{
  GlobalObj *g = new GlobalObj();
  g->mem.assign(n ? n : 1, 0);
  (void)flags;
  return (HGLOBAL)hnew(g, HT_GLOBAL);
}

HGLOBAL GlobalFree(HGLOBAL h)
{
  GlobalObj *g = H<GlobalObj>(h, HT_GLOBAL);
  if (!g) return h;
  hdel(h);
  delete g;
  return nullptr;
}

LPVOID GlobalLock(HGLOBAL h)
{
  GlobalObj *g = H<GlobalObj>(h, HT_GLOBAL);
  return g ? g->mem.data() : nullptr;
}

BOOL GlobalUnlock(HGLOBAL) { return TRUE; }
BOOL FreeResource(HGLOBAL) { return TRUE; }

BOOL OpenProcessToken(HANDLE, DWORD, PHANDLE out)
{
  *out = (HANDLE)hnew(new TokenObj(), HT_TOKEN);
  return TRUE;
}
BOOL LookupPrivilegeValue(LPCSTR, LPCSTR, PLUID l) { l->LowPart = 19; l->HighPart = 0; return TRUE; }
BOOL AdjustTokenPrivileges(HANDLE, BOOL, PTOKEN_PRIVILEGES, DWORD, PTOKEN_PRIVILEGES, LPDWORD) { return TRUE; }
BOOL ExitWindowsEx(UINT, DWORD) { return FALSE; }   /* never shut the user's machine down */

BOOL CreateProcess(LPCSTR app, LPSTR cmd, void *, void *, BOOL, DWORD, LPVOID,
                   LPCSTR, LPSTARTUPINFO, LPPROCESS_INFORMATION pi)
{
  std::string c;
  if (app) c = app;
  if (cmd) { if (!c.empty()) c += " "; c += cmd; }
  int ok = 0;
  if (g_cb.create_process) ok = g_cb.create_process(c.c_str());
  w32::host_log("CreateProcess(\"%s\") -> %s", c.c_str(), ok ? "started by host" : "not available");
  if (!ok) { SetLastError(ERROR_FILE_NOT_FOUND); return FALSE; }
  if (pi) {
    pi->hProcess = (HANDLE)hnew(new ProcessObj(), HT_PROCESS);
    pi->hThread = (HANDLE)hnew(new ProcessObj(), HT_PROCESS);
    pi->dwProcessId = 0x700;
    pi->dwThreadId = 0x704;
  }
  return TRUE;
}

BOOL TerminateProcess(HANDLE, UINT) { return TRUE; }
BOOL GetExitCodeProcess(HANDLE, LPDWORD code) { if (code) *code = 0; return TRUE; }
BOOL SetPriorityClass(HANDLE, DWORD) { return TRUE; }

HINSTANCE ShellExecute(HWND, LPCSTR, LPCSTR file, LPCSTR, LPCSTR, INT)
{
  if (file && g_cb.open_url) g_cb.open_url(file);
  return (HINSTANCE)(uintptr_t)42;   /* > 32 means success */
}

BOOL Shell_NotifyIcon(DWORD, PNOTIFYICONDATA) { return TRUE; }
void SHChangeNotify(LONG, UINT, LPCVOID, LPCVOID) {}

/* ================================ winsock ================================ */
int WSAStartup(WORD ver, LPWSADATA d)
{
  if (d) {
    memset(d, 0, sizeof(*d));
    d->wVersion = ver;
    d->wHighVersion = MAKEWORD(2, 2);
    snprintf(d->szDescription, sizeof(d->szDescription), "WinSock 2.0");
    snprintf(d->szSystemStatus, sizeof(d->szSystemStatus), "Running");
    d->iMaxSockets = 32767;
    d->iMaxUdpDg = 65467;
  }
  return sf_net_init() == 0 ? 0 : 10091 /*WSASYSNOTREADY*/;
}

int WSACleanup(void) { return 0; }

int WSAGetLastError(void)
{
  if (sf_would_block()) return WSAEWOULDBLOCK;
  return sf_errno() ? WSABASEERR + sf_errno() : 0;
}
