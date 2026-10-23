/* ************************************************************************* */
/* sf_ipc_host.cpp -- host side of the IPC protocol (see sf_ipc_host.h).     */
/*                                                                           */
/* NEW FILE, added 2026-10-12 for the Stellar Frontier Godot port.           */
/* ************************************************************************* */
#include "sf_ipc_host.h"
#include <cstring>

bool SfProgram::start(const std::string &exe, const std::vector<std::string> &args, int w, int h,
                      bool emulateNT, std::string *err)
{
  stop();
  uint32_t size = sf_ipc_total_size((uint32_t)w, (uint32_t)h);
  if (!sfipc::shm_create(shm_, size)) {
    if (err) *err = "cannot create shared memory";
    return false;
  }
  hd_ = (sf_ipc_header *)shm_.base;
  sf_ipc_init(hd_, (uint32_t)w, (uint32_t)h, emulateNT ? 1 : 0);
  uint8_t *b = (uint8_t *)shm_.base;
  pixels_ = (const uint32_t *)(b + hd_->frame_offset);
  in_ = b + hd_->in_offset;
  out_ = b + hd_->out_offset;
  w_ = w;
  h_ = h;
  gone_ = false;
  exitCode_ = 0;
  std::vector<std::string> a;
  a.push_back("--sf-ipc");
  a.push_back(shm_.name);
  a.push_back("--sf-parent");
  a.push_back(std::to_string(sfipc::self_pid()));
  for (auto &x : args) a.push_back(x);
  std::string dir = exe;
  size_t k = dir.find_last_of("\\/");
  dir = k == std::string::npos ? std::string(".") : dir.substr(0, k);
  if (!sfipc::proc_spawn(proc_, exe, a, dir, err)) {
    sfipc::shm_close(shm_);
    hd_ = nullptr;
    return false;
  }
  buf_.resize(SF_IPC_OUT_RING);
  return true;
}

int SfProgram::state()
{
  if (!hd_) return SF_IPC_FAILED;
  int s = (int)sf_ipc_load((volatile uint32_t *)&hd_->state);
  if (s == SF_IPC_EXITED || s == SF_IPC_FAILED) {
    exitCode_ = hd_->exit_code;
    return s;
  }
  int code = 0;
  if (!gone_ && !sfipc::proc_alive(proc_, &code)) {
    gone_ = true;
    exitCode_ = code;
  }
  return gone_ ? SF_IPC_EXITED : s;
}

const uint32_t *SfProgram::new_frame()
{
  if (!hd_) return nullptr;
  if (sf_ipc_load(&hd_->frame_written) == hd_->frame_ack) return nullptr;
  return pixels_;
}

void SfProgram::frame_done()
{
  if (hd_) sf_ipc_store(&hd_->frame_ack, sf_ipc_load(&hd_->frame_written));
}

int SfProgram::cursor() const { return hd_ ? hd_->cursor : 0; }

void SfProgram::send(uint32_t type, const int32_t *v, int n)
{
  if (!hd_) return;
  sf_ipc_write(in_, hd_->in_size, &hd_->in_head, &hd_->in_tail, type, v, (uint32_t)(n * 4), nullptr, 0);
}

void SfProgram::mouse_move(int x, int y) { int32_t v[2] = {x, y}; send(SF_IN_MOUSE_MOVE, v, 2); }
void SfProgram::mouse_button(int x, int y, int b, bool d) { int32_t v[4] = {x, y, b, d ? 1 : 0}; send(SF_IN_MOUSE_BUTTON, v, 4); }
void SfProgram::mouse_wheel(int x, int y, int delta) { int32_t v[3] = {x, y, delta}; send(SF_IN_MOUSE_WHEEL, v, 3); }
void SfProgram::key(int vk, int sc, bool ext, bool down, unsigned ch)
{
  int32_t v[5] = {vk, sc, ext ? 1 : 0, down ? 1 : 0, (int32_t)ch};
  send(SF_IN_KEY, v, 5);
}
void SfProgram::focus(bool has) { int32_t v[1] = {has ? 1 : 0}; send(SF_IN_FOCUS, v, 1); }
void SfProgram::request_close() { send(SF_IN_CLOSE, nullptr, 0); }
void SfProgram::restore() { send(SF_IN_RESTORE, nullptr, 0); }

bool SfProgram::poll(Msg &m)
{
  if (!hd_) return false;
  uint32_t type, len;
  if (!sf_ipc_read(out_, hd_->out_size, &hd_->out_head, &hd_->out_tail, &type, &len, buf_.data(),
                   (uint32_t)buf_.size()))
    return false;
  m = Msg();
  m.type = type;
  const uint8_t *p = buf_.data();
  switch (type) {
  case SF_OUT_LOG: case SF_OUT_OPEN_URL: case SF_OUT_RUN:
    m.text.assign((const char *)p, len);
    break;
  case SF_OUT_MINIMIZE: case SF_OUT_EXITED:
    if (len >= 4) memcpy(m.i, p, 4);
    break;
  case SF_OUT_WARP:
    if (len >= 8) memcpy(m.i, p, 8);
    break;
  case SF_OUT_SOUND:
    if (len >= 16) {
      memcpy(m.i, p, 16);
      m.data.assign(p + 16, p + len);
    }
    break;
  case SF_OUT_AUDIO:
    if (len >= 20) {
      memcpy(m.i, p, 20);
      m.text.assign((const char *)p + 20, len - 20);
    }
    break;
  }
  return true;
}

void SfProgram::stop()
{
  if (proc_.handle) {
    int code;
    if (sfipc::proc_alive(proc_, &code)) sfipc::proc_kill(proc_);
    sfipc::proc_close(proc_);
  }
  if (shm_.base) sfipc::shm_close(shm_);
  hd_ = nullptr;
}
