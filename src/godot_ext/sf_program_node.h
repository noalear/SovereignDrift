/* ************************************************************************* */
/* sf_program_node.h -- Godot node that runs an original Stellar Frontier    */
/* program (a 32-bit child process) and exposes its desktop as a texture.    */
/*                                                                           */
/* NEW FILE, added 2026-10-12 for the Stellar Frontier Godot port.           */
/* ************************************************************************* */
#ifndef SF_PROGRAM_NODE_H
#define SF_PROGRAM_NODE_H

#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>

#include "sf_ipc_host.h"

namespace godot {

class SFProgram : public Node {
  GDCLASS(SFProgram, Node)

public:
  SFProgram();
  ~SFProgram();

  /* Start the program at exe_path with a width x height desktop. */
  bool start(const String &exe_path, const PackedStringArray &args, int width, int height);
  void stop();
  /* 0 starting, 1 running, 2 exited, 3 failed */
  int get_state();
  int get_exit_code() const;
  String get_error() const { return error_; }
  Ref<ImageTexture> get_texture() const { return texture_; }
  int get_width() const { return prog_.width(); }
  int get_height() const { return prog_.height(); }
  /* -1 hidden, 0 arrow, 1 I-beam, 2 wait, 3 size all, 4 hand */
  int get_cursor() const;

  /* Call once per frame: takes the newest picture, emits messages.
     Returns true when the texture changed. */
  bool poll();

  void mouse_move(int x, int y);
  void mouse_button(int x, int y, int button, bool down);
  void mouse_wheel(int x, int y, int delta);
  /* Translate a Godot key event into a Win32 key (virtual key from the
     logical key, scan code from the physical key, the typed character). */
  void key_event(const Ref<InputEventKey> &event);
  void key_raw(int vk, int scancode, bool extended, bool down, int ch);
  void set_focus(bool has_focus);
  void request_close();
  void restore();

  /* MIDI music through the Windows synthesizer (Windows only). */
  void set_music_enabled(bool on) { musicEnabled_ = on; }
  bool get_music_enabled() const { return musicEnabled_; }

protected:
  static void _bind_methods();

private:
  SfProgram prog_;
  Ref<Image> image_;
  Ref<ImageTexture> texture_;
  PackedByteArray rgba_;
  String error_;
  bool musicEnabled_ = true;
  bool exitedSent_ = false;
};

}  /* namespace godot */

#endif
