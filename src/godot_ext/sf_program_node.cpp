/* ************************************************************************* */
/* sf_program_node.cpp -- see sf_program_node.h.                             */
/*                                                                           */
/* NEW FILE, added 2026-10-12 for the Stellar Frontier Godot port.           */
/* ************************************************************************* */
#include "sf_program_node.h"
#include "sf_keymap.h"
#include "sf_music.h"
#include "sf_host.h"
#include <cstring>

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <string>
#include <vector>

using namespace godot;

namespace {
std::string g_binwin;      /* the program's working directory (music paths) */
}

SFProgram::SFProgram() {}

SFProgram::~SFProgram()
{
  prog_.stop();
  sfmusic::shutdown();
}

void SFProgram::_bind_methods()
{
  ClassDB::bind_method(D_METHOD("start", "exe_path", "args", "width", "height"), &SFProgram::start);
  ClassDB::bind_method(D_METHOD("stop"), &SFProgram::stop);
  ClassDB::bind_method(D_METHOD("get_state"), &SFProgram::get_state);
  ClassDB::bind_method(D_METHOD("get_exit_code"), &SFProgram::get_exit_code);
  ClassDB::bind_method(D_METHOD("get_error"), &SFProgram::get_error);
  ClassDB::bind_method(D_METHOD("get_texture"), &SFProgram::get_texture);
  ClassDB::bind_method(D_METHOD("get_width"), &SFProgram::get_width);
  ClassDB::bind_method(D_METHOD("get_height"), &SFProgram::get_height);
  ClassDB::bind_method(D_METHOD("get_cursor"), &SFProgram::get_cursor);
  ClassDB::bind_method(D_METHOD("poll"), &SFProgram::poll);
  ClassDB::bind_method(D_METHOD("mouse_move", "x", "y"), &SFProgram::mouse_move);
  ClassDB::bind_method(D_METHOD("mouse_button", "x", "y", "button", "down"), &SFProgram::mouse_button);
  ClassDB::bind_method(D_METHOD("mouse_wheel", "x", "y", "delta"), &SFProgram::mouse_wheel);
  ClassDB::bind_method(D_METHOD("key_event", "event"), &SFProgram::key_event);
  ClassDB::bind_method(D_METHOD("key_raw", "vk", "scancode", "extended", "down", "ch"), &SFProgram::key_raw);
  ClassDB::bind_method(D_METHOD("set_focus", "has_focus"), &SFProgram::set_focus);
  ClassDB::bind_method(D_METHOD("request_close"), &SFProgram::request_close);
  ClassDB::bind_method(D_METHOD("restore"), &SFProgram::restore);
  ClassDB::bind_method(D_METHOD("set_music_enabled", "on"), &SFProgram::set_music_enabled);
  ClassDB::bind_method(D_METHOD("get_music_enabled"), &SFProgram::get_music_enabled);
  ADD_PROPERTY(PropertyInfo(Variant::BOOL, "music_enabled"), "set_music_enabled", "get_music_enabled");

  ADD_SIGNAL(MethodInfo("log_line", PropertyInfo(Variant::STRING, "text")));
  ADD_SIGNAL(MethodInfo("open_url", PropertyInfo(Variant::STRING, "url")));
  ADD_SIGNAL(MethodInfo("run_requested", PropertyInfo(Variant::STRING, "command_line")));
  ADD_SIGNAL(MethodInfo("minimize_requested", PropertyInfo(Variant::BOOL, "minimize")));
  ADD_SIGNAL(MethodInfo("warp_requested", PropertyInfo(Variant::INT, "x"), PropertyInfo(Variant::INT, "y")));
  ADD_SIGNAL(MethodInfo("exited", PropertyInfo(Variant::INT, "code")));
  ADD_SIGNAL(MethodInfo("sound_created", PropertyInfo(Variant::INT, "id"),
                        PropertyInfo(Variant::PACKED_BYTE_ARRAY, "data"), PropertyInfo(Variant::INT, "bits"),
                        PropertyInfo(Variant::INT, "rate"), PropertyInfo(Variant::BOOL, "stereo")));
  ADD_SIGNAL(MethodInfo("audio_command", PropertyInfo(Variant::INT, "cmd"), PropertyInfo(Variant::INT, "id"),
                        PropertyInfo(Variant::BOOL, "loop"), PropertyInfo(Variant::INT, "volume_mb"),
                        PropertyInfo(Variant::BOOL, "restart"), PropertyInfo(Variant::STRING, "path")));

  BIND_CONSTANT(SF_AUD_PLAY);
  BIND_CONSTANT(SF_AUD_STOP);
  BIND_CONSTANT(SF_AUD_VOLUME);
  BIND_CONSTANT(SF_AUD_DESTROY);
  BIND_CONSTANT(SF_AUD_MUSIC_PLAY);
  BIND_CONSTANT(SF_AUD_MUSIC_STOP);
}

bool SFProgram::start(const String &exe_path, const PackedStringArray &args, int width, int height)
{
  std::string exe = exe_path.utf8().get_data();
  std::vector<std::string> a;
  for (int i = 0; i < args.size(); i++) a.push_back(args[i].utf8().get_data());
  std::string err;
  exitedSent_ = false;
  if (!prog_.start(exe, a, width, height, true, &err)) {
    error_ = String(err.c_str());
    return false;
  }
  size_t k = exe.find_last_of("\\/");
  std::string dir = k == std::string::npos ? std::string(".") : exe.substr(0, k);
#ifdef _WIN32
  g_binwin = dir + "\\binwin";
#else
  g_binwin = dir + "/binwin";
#endif
  image_ = Image::create_empty(width, height, false, Image::FORMAT_RGBA8);
  image_->fill(Color(0, 0, 0, 1));
  texture_ = ImageTexture::create_from_image(image_);
  rgba_.resize((int64_t)width * height * 4);
  return true;
}

void SFProgram::stop()
{
  prog_.stop();
  sfmusic::stop();
}

int SFProgram::get_state() { return prog_.state(); }
int SFProgram::get_exit_code() const { return prog_.exit_code(); }
int SFProgram::get_cursor() const { return prog_.cursor(); }

bool SFProgram::poll()
{
  bool changed = false;
  if (const uint32_t *px = prog_.new_frame()) {
    /* 0x00RRGGBB -> RGBA bytes */
    uint8_t *d = rgba_.ptrw();
    size_t n = (size_t)prog_.width() * prog_.height();
    for (size_t i = 0; i < n; i++) {
      uint32_t p = px[i];
      d[0] = (uint8_t)(p >> 16);
      d[1] = (uint8_t)(p >> 8);
      d[2] = (uint8_t)p;
      d[3] = 255;
      d += 4;
    }
    prog_.frame_done();
    image_->set_data(prog_.width(), prog_.height(), false, Image::FORMAT_RGBA8, rgba_);
    texture_->update(image_);
    changed = true;
  }

  SfProgram::Msg m;
  int budget = 2000;
  while (budget-- > 0 && prog_.poll(m)) {
    switch (m.type) {
    case SF_OUT_LOG:      emit_signal("log_line", String::utf8(m.text.c_str())); break;
    case SF_OUT_OPEN_URL: emit_signal("open_url", String::utf8(m.text.c_str())); break;
    case SF_OUT_RUN:      emit_signal("run_requested", String::utf8(m.text.c_str())); break;
    case SF_OUT_MINIMIZE: emit_signal("minimize_requested", m.i[0] != 0); break;
    case SF_OUT_WARP:     emit_signal("warp_requested", m.i[0], m.i[1]); break;
    case SF_OUT_EXITED:
      if (!exitedSent_) { exitedSent_ = true; emit_signal("exited", m.i[0]); }
      break;
    case SF_OUT_SOUND: {
      int ch = m.i[1], bits = m.i[2];
      PackedByteArray data;
      data.resize((int64_t)m.data.size());
      uint8_t *d = data.ptrw();
      if (bits == 8)
        for (size_t i = 0; i < m.data.size(); i++) d[i] = (uint8_t)(m.data[i] ^ 0x80);   /* to signed */
      else
        memcpy(d, m.data.data(), m.data.size());
      emit_signal("sound_created", m.i[0], data, bits, m.i[3], ch == 2);
      break;
    }
    case SF_OUT_AUDIO: {
      int cmd = m.i[0];
      if (cmd == SF_AUD_MUSIC_PLAY && musicEnabled_) sfmusic::play(g_binwin, m.text, m.i[4] != 0);
      else if (cmd == SF_AUD_MUSIC_STOP) sfmusic::stop();
      emit_signal("audio_command", cmd, m.i[1], m.i[2] != 0, m.i[3], m.i[4] != 0, String::utf8(m.text.c_str()));
      break;
    }
    }
  }
  if (!exitedSent_) {
    int st = prog_.state();
    if (st == SF_IPC_EXITED || st == SF_IPC_FAILED) {
      exitedSent_ = true;
      emit_signal("exited", prog_.exit_code());
    }
  }
  return changed;
}

void SFProgram::mouse_move(int x, int y) { prog_.mouse_move(x, y); }
void SFProgram::mouse_button(int x, int y, int b, bool down) { prog_.mouse_button(x, y, b, down); }
void SFProgram::mouse_wheel(int x, int y, int delta) { prog_.mouse_wheel(x, y, delta); }

void SFProgram::key_event(const Ref<InputEventKey> &ev)
{
  if (ev.is_null()) return;
  int vk, sc;
  bool ext;
  if (!sf_key_to_win32(ev->get_keycode(), ev->get_physical_keycode(), ev->get_location(), &vk, &sc, &ext))
    return;
  bool down = ev->is_pressed();
  int ch = down ? sf_key_char(ev->get_keycode(), (int)ev->get_unicode()) : 0;
  prog_.key(vk, sc, ext, down, (unsigned)ch);
}

void SFProgram::key_raw(int vk, int sc, bool ext, bool down, int ch) { prog_.key(vk, sc, ext, down, (unsigned)ch); }
void SFProgram::set_focus(bool f) { prog_.focus(f); }
void SFProgram::request_close() { prog_.request_close(); }
void SFProgram::restore() { prog_.restore(); }
