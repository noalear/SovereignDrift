/* ************************************************************************* */
/* sf_host.h -- bridge between the original program and its host (Godot or  */
/* the headless test runner).  Plain C API, no Win32 or Godot types.         */
/*                                                                           */
/* NEW FILE, added 2026-10-12 for the Stellar Frontier Godot port.           */
/*                                                                           */
/* Model: the original Win32 program runs unchanged on its own "game main"   */
/* thread on top of the port's Win32 emulation (src/w32emu).  The emulation  */
/* owns a virtual desktop of screen_w x screen_h pixels on which all of the  */
/* program's windows live.  The host:                                        */
/*   - configures and starts the program,                                    */
/*   - fetches the composed desktop image whenever it changes,               */
/*   - forwards mouse and keyboard input in desktop coordinates,             */
/*   - plays the sounds and music the program requests.                      */
/* All functions are thread-safe unless noted.                               */
/* ************************************************************************* */
#ifndef SF_HOST_H
#define SF_HOST_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------ lifecycle -------------------------------- */
typedef int (*sf_host_main_fn)(int argc, char **argv);

typedef struct {
  int  screen_w, screen_h;      /* virtual desktop size, e.g. 1024 x 768      */
  const char *install_dir;      /* directory holding binwin/, dat/, bmp/ ... */
  const char *exe_name;         /* e.g. "frontier.exe", "server.exe"         */
  const char *zap_archive;      /* file with the program's appended bitmaps  */
  const char *icon_file;        /* .ico used for the main window             */
  int  emulate_nt;              /* 1: report Windows XP, 0: Windows 98       */
} sf_host_config;

typedef struct {
  void (*log)(const char *line);
  void (*open_url)(const char *url);
  /* Return nonzero if the host started the program. */
  int  (*create_process)(const char *cmdline);
  /* The program asked to minimise / restore its last visible window. */
  void (*minimize)(int minimized);
} sf_host_callbacks;

void sf_host_set_callbacks(const sf_host_callbacks *cb);
/* Configure the emulation and run main() on a new thread with the
   program's install dir + "binwin" as working directory (as the original
   shortcuts did).  Returns 0 on success. */
int  sf_host_start(const sf_host_config *cfg, sf_host_main_fn fn,
                   int argc, char **argv);
int  sf_host_running(void);
int  sf_host_exit_code(void);
/* The host window is being closed: send WM_CLOSE to the program's
   top-level windows (as Windows does at shutdown). */
void sf_host_request_close(void);

/* ------------------------------- display --------------------------------- */
/* Compose the desktop into dst (screen_w*screen_h pixels, 0x00RRGGBB) if it
   changed since *serial.  Returns 1 and updates *serial when pixels were
   written. */
int  sf_host_frame(uint32_t *dst, unsigned *serial);
void sf_host_screen_size(int *w, int *h);

/* -------------------------------- input ---------------------------------- */
void sf_host_mouse_move(int x, int y);
/* button: 1 left, 2 right, 3 middle */
void sf_host_mouse_button(int x, int y, int button, int down);
void sf_host_mouse_wheel(int x, int y, int delta);
/* Win32 virtual-key code, set-1 scan code, extended-key flag, the
   character the key produces in the user's layout (0 if none). */
void sf_host_key(int vk, int scancode, int extended, int down, unsigned ch);
void sf_host_focus(int has_focus);
/* Pointer warps requested by the program (SetCursorPos). Returns 1 if a
   warp is pending and fills x,y (desktop coordinates). */
int  sf_host_take_warp(int *x, int *y);
/* -1 = hidden, 0 = arrow, 1 = I-beam, 2 = wait, 3 = size all, 4 = hand */
int  sf_host_cursor(void);
/* The host window was restored after the minimize callback: restore the
   program's minimised top-level windows (as clicking its task-bar button). */
void sf_host_restore(void);

/* -------------------------------- audio ---------------------------------- */
#define SF_AUD_CREATE      1   /* id: sound created; fetch PCM with sf_host_sound_data */
#define SF_AUD_DESTROY     2
#define SF_AUD_PLAY        3   /* id, loop, volume_mb (hundredths of dB, <= 0) */
#define SF_AUD_STOP        4
#define SF_AUD_VOLUME      5   /* id, volume_mb */
#define SF_AUD_MUSIC_PLAY  6   /* path (MIDI file, Windows path relative to cwd) */
#define SF_AUD_MUSIC_STOP  7
typedef struct {
  int  cmd;
  int  id;
  int  loop;
  int  volume_mb;
  int  restart;               /* PLAY: start from the beginning */
  char path[512];
} sf_host_audio_cmd;
int      sf_host_audio_poll(sf_host_audio_cmd *out);
uint32_t sf_host_sound_data(int id, const void **pcm, int *channels,
                            int *bits, int *rate);

/* ------------------------------- helpers --------------------------------- */
void sf_host_log(const char *fmt, ...);
/* Host path for a path written by the program ("..\dat\x.dat"). */
const char *sf_host_path(const char *winpath, char *buf, int buflen);

#ifdef __cplusplus
}
#endif

#endif /* SF_HOST_H */
