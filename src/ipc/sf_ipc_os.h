/* ************************************************************************* */
/* sf_ipc_os.h -- shared memory and process helpers for the IPC protocol.    */
/*                                                                           */
/* NEW FILE, added 2026-10-12 for the Stellar Frontier Godot port.           */
/* Windows: named file mappings, CreateProcess, a kill-on-close job object.  */
/* POSIX:   shm_open/mmap, posix_spawn, PR_SET_PDEATHSIG on Linux.           */
/* ************************************************************************* */
#ifndef SF_IPC_OS_H
#define SF_IPC_OS_H

#include <stdint.h>
#include <string>
#include <vector>

namespace sfipc {

struct Shm {
  void *base = nullptr;
  uint32_t size = 0;
  std::string name;
  intptr_t handle = 0;      /* HANDLE on Windows, fd on POSIX */
  bool owner = false;
};

/* host: create a new block (zero-filled) with a unique name */
bool shm_create(Shm &s, uint32_t size);
/* program: open the block the host created */
bool shm_open_existing(Shm &s, const char *name, uint32_t size);
void shm_close(Shm &s);

struct Proc {
  intptr_t handle = 0;      /* HANDLE (Windows) or pid (POSIX) */
  uint32_t pid = 0;
};

/* host: start exe with args in workdir; the program dies with the host */
bool proc_spawn(Proc &p, const std::string &exe, const std::vector<std::string> &args,
                const std::string &workdir, std::string *err);
bool proc_alive(Proc &p, int *exit_code);
void proc_kill(Proc &p);
void proc_close(Proc &p);

/* program side */
uint32_t self_pid();
bool parent_alive(uint32_t pid);
void die_with_parent();
void sleep_ms(int ms);
void precise_timers();      /* 1 ms timer resolution on Windows */
std::string exe_dir();

}  /* namespace sfipc */

#endif
