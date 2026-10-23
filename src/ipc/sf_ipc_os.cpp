/* ************************************************************************* */
/* sf_ipc_os.cpp -- shared memory and process helpers (see sf_ipc_os.h).     */
/*                                                                           */
/* NEW FILE, added 2026-10-12 for the Stellar Frontier Godot port.           */
/* ************************************************************************* */
#include "sf_ipc_os.h"
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <thread>

#ifdef _WIN32
#  define WIN32_LEAN_AND_MEAN
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  include <windows.h>
#  include <mmsystem.h>
#else
#  include <fcntl.h>
#  include <signal.h>
#  include <spawn.h>
#  include <sys/mman.h>
#  include <sys/stat.h>
#  include <sys/wait.h>
#  include <unistd.h>
#  ifdef __linux__
#    include <sys/prctl.h>
#  endif
extern char **environ;
#endif

namespace sfipc {

static std::atomic<unsigned> g_counter{0};

#ifdef _WIN32

bool shm_create(Shm &s, uint32_t size)
{
  char name[96];
  snprintf(name, sizeof(name), "Local\\sfport_%lu_%u_%lu", (unsigned long)GetCurrentProcessId(),
           g_counter.fetch_add(1), (unsigned long)GetTickCount());
  HANDLE h = CreateFileMappingA(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, size, name);
  if (!h) return false;
  void *p = MapViewOfFile(h, FILE_MAP_ALL_ACCESS, 0, 0, size);
  if (!p) { CloseHandle(h); return false; }
  memset(p, 0, size);
  s.base = p; s.size = size; s.name = name; s.handle = (intptr_t)h; s.owner = true;
  return true;
}

bool shm_open_existing(Shm &s, const char *name, uint32_t size)
{
  HANDLE h = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, name);
  if (!h) return false;
  void *p = MapViewOfFile(h, FILE_MAP_ALL_ACCESS, 0, 0, size);
  if (!p) { CloseHandle(h); return false; }
  s.base = p; s.size = size; s.name = name; s.handle = (intptr_t)h; s.owner = false;
  return true;
}

void shm_close(Shm &s)
{
  if (s.base) UnmapViewOfFile(s.base);
  if (s.handle) CloseHandle((HANDLE)s.handle);
  s.base = nullptr; s.handle = 0;
}

static std::string quote_arg(const std::string &a)
{
  if (!a.empty() && a.find_first_of(" \t\"") == std::string::npos) return a;
  std::string r = "\"";
  int bs = 0;
  for (char c : a) {
    if (c == '\\') { bs++; continue; }
    if (c == '"') { r.append(bs * 2 + 1, '\\'); r += '"'; bs = 0; continue; }
    r.append(bs, '\\'); bs = 0; r += c;
  }
  r.append(bs * 2, '\\');
  return r + "\"";
}

bool proc_spawn(Proc &p, const std::string &exe, const std::vector<std::string> &args,
                const std::string &workdir, std::string *err)
{
  static HANDLE job = nullptr;
  if (!job) {
    job = CreateJobObjectA(nullptr, nullptr);
    if (job) {
      JOBOBJECT_EXTENDED_LIMIT_INFORMATION li;
      memset(&li, 0, sizeof(li));
      li.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
      SetInformationJobObject(job, JobObjectExtendedLimitInformation, &li, sizeof(li));
    }
  }
  std::string cmd = quote_arg(exe);
  for (auto &a : args) cmd += " " + quote_arg(a);
  STARTUPINFOA si;
  memset(&si, 0, sizeof(si));
  si.cb = sizeof(si);
  PROCESS_INFORMATION pi;
  memset(&pi, 0, sizeof(pi));
  std::vector<char> cl(cmd.begin(), cmd.end());
  cl.push_back(0);
  if (!CreateProcessA(exe.c_str(), cl.data(), nullptr, nullptr, FALSE,
                      CREATE_NO_WINDOW | CREATE_SUSPENDED, nullptr,
                      workdir.empty() ? nullptr : workdir.c_str(), &si, &pi)) {
    if (err) { char b[64]; snprintf(b, sizeof(b), "CreateProcess error %lu", GetLastError()); *err = b; }
    return false;
  }
  if (job) AssignProcessToJobObject(job, pi.hProcess);
  ResumeThread(pi.hThread);
  CloseHandle(pi.hThread);
  p.handle = (intptr_t)pi.hProcess;
  p.pid = pi.dwProcessId;
  return true;
}

bool proc_alive(Proc &p, int *code)
{
  if (!p.handle) return false;
  DWORD c = 0;
  if (!GetExitCodeProcess((HANDLE)p.handle, &c)) return false;
  if (c == STILL_ACTIVE) return true;
  if (code) *code = (int)c;
  return false;
}

void proc_kill(Proc &p) { if (p.handle) TerminateProcess((HANDLE)p.handle, 1); }
void proc_close(Proc &p) { if (p.handle) CloseHandle((HANDLE)p.handle); p.handle = 0; }

uint32_t self_pid() { return GetCurrentProcessId(); }

bool parent_alive(uint32_t pid)
{
  static HANDLE h = nullptr;
  static uint32_t hp = 0;
  if (!h || hp != pid) {
    h = OpenProcess(SYNCHRONIZE, FALSE, pid);
    hp = pid;
    if (!h) return false;
  }
  return WaitForSingleObject(h, 0) == WAIT_TIMEOUT;
}

void die_with_parent() {}
void sleep_ms(int ms) { Sleep(ms); }
void precise_timers() { timeBeginPeriod(1); }

std::string exe_dir()
{
  char b[MAX_PATH];
  DWORD n = GetModuleFileNameA(nullptr, b, MAX_PATH);
  std::string s(b, n);
  size_t k = s.find_last_of("\\/");
  return k == std::string::npos ? std::string(".") : s.substr(0, k);
}

#else  /* POSIX */

bool shm_create(Shm &s, uint32_t size)
{
  char name[96];
  snprintf(name, sizeof(name), "/sfport_%d_%u_%ld", (int)getpid(), g_counter.fetch_add(1),
           (long)std::chrono::steady_clock::now().time_since_epoch().count() % 1000000);
  int fd = ::shm_open(name, O_CREAT | O_EXCL | O_RDWR, 0600);
  if (fd < 0) return false;
  if (ftruncate(fd, size) != 0) { close(fd); shm_unlink(name); return false; }
  void *p = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (p == MAP_FAILED) { close(fd); shm_unlink(name); return false; }
  s.base = p; s.size = size; s.name = name; s.handle = fd; s.owner = true;
  return true;
}

bool shm_open_existing(Shm &s, const char *name, uint32_t size)
{
  int fd = ::shm_open(name, O_RDWR, 0600);
  if (fd < 0) return false;
  void *p = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (p == MAP_FAILED) { close(fd); return false; }
  s.base = p; s.size = size; s.name = name; s.handle = fd; s.owner = false;
  return true;
}

void shm_close(Shm &s)
{
  if (s.base) munmap(s.base, s.size);
  if (s.handle > 0) close((int)s.handle);
  if (s.owner && !s.name.empty()) shm_unlink(s.name.c_str());
  s.base = nullptr; s.handle = 0;
}

bool proc_spawn(Proc &p, const std::string &exe, const std::vector<std::string> &args,
                const std::string &workdir, std::string *err)
{
  std::vector<std::string> a;
  a.push_back(exe);
  for (auto &x : args) a.push_back(x);
  std::vector<char *> av;
  for (auto &x : a) av.push_back(&x[0]);
  av.push_back(nullptr);
  pid_t pid = fork();
  if (pid < 0) { if (err) *err = "fork failed"; return false; }
  if (pid == 0) {
#ifdef __linux__
    prctl(PR_SET_PDEATHSIG, SIGKILL);
#endif
    if (!workdir.empty() && chdir(workdir.c_str()) != 0) _exit(126);
    execv(exe.c_str(), av.data());
    _exit(127);
  }
  p.handle = pid;
  p.pid = (uint32_t)pid;
  return true;
}

bool proc_alive(Proc &p, int *code)
{
  if (!p.handle) return false;
  int st = 0;
  pid_t r = waitpid((pid_t)p.handle, &st, WNOHANG);
  if (r == 0) return true;
  if (r == (pid_t)p.handle && code) *code = WIFEXITED(st) ? WEXITSTATUS(st) : 128 + WTERMSIG(st);
  p.handle = 0;
  return false;
}

void proc_kill(Proc &p) { if (p.handle) kill((pid_t)p.handle, SIGKILL); }
void proc_close(Proc &p) { p.handle = 0; }

uint32_t self_pid() { return (uint32_t)getpid(); }
bool parent_alive(uint32_t pid) { return getppid() == (pid_t)pid; }

void die_with_parent()
{
#ifdef __linux__
  prctl(PR_SET_PDEATHSIG, SIGKILL);
#endif
}

void sleep_ms(int ms) { std::this_thread::sleep_for(std::chrono::milliseconds(ms)); }
void precise_timers() {}

std::string exe_dir()
{
  char b[4096];
  ssize_t n = readlink("/proc/self/exe", b, sizeof(b) - 1);
  if (n <= 0) return ".";
  std::string s(b, (size_t)n);
  size_t k = s.find_last_of('/');
  return k == std::string::npos ? std::string(".") : s.substr(0, k);
}

#endif

}  /* namespace sfipc */
