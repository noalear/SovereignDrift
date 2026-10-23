/* ************************************************************************* */
/* sf_ipc_host.h -- host side of the IPC protocol: start an original program */
/* as a child process and exchange frames, input and messages with it.       */
/*                                                                           */
/* NEW FILE, added 2026-10-12 for the Stellar Frontier Godot port.           */
/* Used by the Godot extension (godot/extension) and the ipctest tool.       */
/* ************************************************************************* */
#ifndef SF_IPC_HOST_H
#define SF_IPC_HOST_H

#include "sf_ipc.h"
#include "sf_ipc_os.h"
#include <cstdint>
#include <string>
#include <vector>

class SfProgram {
public:
  struct Msg {
    uint32_t type = 0;
    int32_t i[8] = {0};
    std::string text;               /* LOG, OPEN_URL, RUN, AUDIO path     */
    std::vector<uint8_t> data;      /* SOUND: PCM bytes                    */
  };

  ~SfProgram() { stop(); }

  /* Start exe (a 32-bit original program) with a w x h desktop. */
  bool start(const std::string &exe, const std::vector<std::string> &args, int w, int h,
             bool emulateNT, std::string *err);
  bool started() const { return hd_ != nullptr; }
  /* SF_IPC_STARTING / RUNNING / EXITED / FAILED; EXITED also when the
     process has gone away. */
  int state();
  int exit_code() const { return exitCode_; }
  int width() const { return w_; }
  int height() const { return h_; }

  /* A new desktop picture, or nullptr.  Call frame_done() after copying. */
  const uint32_t *new_frame();
  void frame_done();
  int cursor() const;

  void mouse_move(int x, int y);
  void mouse_button(int x, int y, int button, bool down);
  void mouse_wheel(int x, int y, int delta);
  void key(int vk, int scancode, bool extended, bool down, unsigned ch);
  void focus(bool has);
  void request_close();
  void restore();

  /* Next message from the program; false when there is none. */
  bool poll(Msg &m);

  void stop();

private:
  void send(uint32_t type, const int32_t *v, int n);

  sfipc::Shm shm_;
  sfipc::Proc proc_;
  sf_ipc_header *hd_ = nullptr;
  uint8_t *in_ = nullptr, *out_ = nullptr;
  const uint32_t *pixels_ = nullptr;
  int w_ = 0, h_ = 0;
  int exitCode_ = 0;
  bool gone_ = false;
  std::vector<uint8_t> buf_;
};

#endif
