/* ************************************************************************* */
/* w32_audio.cpp -- DirectSound (static secondary buffers) and the MCI       */
/* sequencer string commands used for MIDI music.  Playback itself is done   */
/* by the host; this layer keeps the state the program queries (playing,     */
/* looping, positions) on the emulated side so that it is deterministic.     */
/*                                                                           */
/* NEW FILE, added 2026-10-12 for the Stellar Frontier Godot port.           */
/* ************************************************************************* */
#include "w32_internal.h"
#include "sf_files.h"
#include <cstring>
#include <cstdio>
#include <chrono>
#include <map>

using namespace w32;

/* ============================== host queue =============================== */
namespace {

std::mutex g_amtx;
std::deque<sf_host_audio_cmd> g_aq W32_EARLY;
struct SoundData { std::vector<uint8_t> pcm; int ch = 1, bits = 8, rate = 11025; };
std::map<int, SoundData> g_sounds W32_EARLY;
int g_next_sound = 1;

void push(const sf_host_audio_cmd &c)
{
  std::lock_guard<std::mutex> lk(g_amtx);
  if (g_aq.size() > 4096) g_aq.pop_front();
  g_aq.push_back(c);
}

int64_t now_ms()
{
  using namespace std::chrono;
  return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

/* ============================== DirectSound ============================== */
struct DSBuf : IDirectSoundBuffer {
  ULONG refs = 1;
  int id = 0;
  std::vector<uint8_t> data;
  WAVEFORMATEX fmt;
  LONG volume = 0;
  LONG pan = 0;
  DWORD freq = 0;
  bool playing = false, looping = false;
  DWORD pos = 0;                /* position when stopped / at start of play */
  int64_t startMs = 0;
  bool announced = false;

  uint32_t bytes_per_sec() const { return fmt.nAvgBytesPerSec ? fmt.nAvgBytesPerSec : 11025; }
  DWORD cur_pos() {
    if (!playing || data.empty()) return pos;
    int64_t el = now_ms() - startMs;
    uint64_t p = pos + (uint64_t)el * bytes_per_sec() / 1000;
    p -= p % std::max<uint32_t>(1, fmt.nBlockAlign);
    if (looping) return (DWORD)(p % data.size());
    if (p >= data.size()) { playing = false; pos = 0; return 0; }
    return (DWORD)p;
  }
  void announce() {
    if (announced) return;
    announced = true;
    {
      std::lock_guard<std::mutex> lk(g_amtx);
      SoundData &sd = g_sounds[id];
      sd.pcm = data;
      sd.ch = fmt.nChannels;
      sd.bits = fmt.wBitsPerSample;
      sd.rate = (int)fmt.nSamplesPerSec;
    }
    sf_host_audio_cmd c;
    memset(&c, 0, sizeof(c));
    c.cmd = SF_AUD_CREATE;
    c.id = id;
    push(c);
  }

  HRESULT QueryInterface(REFIID, LPVOID *p) override { if (p) *p = nullptr; return E_NOINTERFACE; }
  ULONG AddRef() override { return ++refs; }
  ULONG Release() override {
    ULONG r = --refs;
    if (r == 0) {
      sf_host_audio_cmd c;
      memset(&c, 0, sizeof(c));
      c.cmd = SF_AUD_DESTROY;
      c.id = id;
      push(c);
      {
        std::lock_guard<std::mutex> lk(g_amtx);
        g_sounds.erase(id);
      }
      delete this;
    }
    return r;
  }
  HRESULT GetCaps(void *) override { return DS_OK; }
  HRESULT GetCurrentPosition(LPDWORD play, LPDWORD write) override {
    DWORD p = cur_pos();
    if (play) *play = p;
    if (write) *write = data.empty() ? 0 : (DWORD)((p + bytes_per_sec() / 100) % data.size());
    return DS_OK;
  }
  HRESULT GetFormat(LPWAVEFORMATEX f, DWORD n, LPDWORD w) override {
    if (f && n >= sizeof(WAVEFORMATEX)) *f = fmt;
    if (w) *w = sizeof(WAVEFORMATEX);
    return DS_OK;
  }
  HRESULT GetVolume(LPLONG v) override { if (v) *v = volume; return DS_OK; }
  HRESULT GetPan(LPLONG p) override { if (p) *p = pan; return DS_OK; }
  HRESULT GetFrequency(LPDWORD f) override { if (f) *f = freq ? freq : fmt.nSamplesPerSec; return DS_OK; }
  HRESULT GetStatus(LPDWORD s) override {
    cur_pos();
    if (s) *s = (playing ? DSBSTATUS_PLAYING : 0) | (playing && looping ? DSBSTATUS_LOOPING : 0);
    return DS_OK;
  }
  HRESULT Initialize(void *, LPCDSBUFFERDESC) override { return DSERR_ALLOCATED; }
  HRESULT Lock(DWORD off, DWORD n, LPVOID *p1, LPDWORD n1, LPVOID *p2, LPDWORD n2, DWORD flags) override {
    if (data.empty()) return DSERR_INVALIDPARAM;
    if (flags & DSBLOCK_ENTIREBUFFER) { off = 0; n = (DWORD)data.size(); }
    off %= data.size();
    DWORD first = std::min<DWORD>(n, (DWORD)data.size() - off);
    if (p1) *p1 = data.data() + off;
    if (n1) *n1 = first;
    if (p2) *p2 = n > first ? data.data() : nullptr;
    if (n2) *n2 = n > first ? std::min<DWORD>(n - first, off) : 0;
    return DS_OK;
  }
  HRESULT Unlock(LPVOID, DWORD, LPVOID, DWORD) override {
    announce();
    return DS_OK;
  }
  HRESULT Play(DWORD, DWORD, DWORD flags) override {
    announce();
    bool restart = !playing || (flags & DSBPLAY_LOOPING) != (DWORD)(looping ? DSBPLAY_LOOPING : 0);
    if (playing && !restart) return DS_OK;    /* already playing: nothing changes */
    DWORD from = cur_pos();
    playing = true;
    looping = (flags & DSBPLAY_LOOPING) != 0;
    pos = from;
    startMs = now_ms();
    sf_host_audio_cmd c;
    memset(&c, 0, sizeof(c));
    c.cmd = SF_AUD_PLAY;
    c.id = id;
    c.loop = looping;
    c.volume_mb = volume;
    c.restart = from == 0;
    push(c);
    return DS_OK;
  }
  HRESULT SetCurrentPosition(DWORD p) override {
    if (!data.empty()) p %= data.size();
    bool wasPlaying = playing && (cur_pos(), playing);
    pos = p;
    startMs = now_ms();
    if (wasPlaying) {
      sf_host_audio_cmd c;
      memset(&c, 0, sizeof(c));
      c.cmd = SF_AUD_PLAY;
      c.id = id;
      c.loop = looping;
      c.volume_mb = volume;
      c.restart = 1;
      push(c);
    }
    return DS_OK;
  }
  HRESULT SetFormat(const WAVEFORMATEX *) override { return DSERR_INVALIDCALL; }
  HRESULT SetVolume(LONG v) override {
    if (v > 0) v = 0;
    if (v < DSBVOLUME_MIN) v = DSBVOLUME_MIN;
    volume = v;
    sf_host_audio_cmd c;
    memset(&c, 0, sizeof(c));
    c.cmd = SF_AUD_VOLUME;
    c.id = id;
    c.volume_mb = v;
    push(c);
    return DS_OK;
  }
  HRESULT SetPan(LONG p) override { pan = p; return DS_OK; }
  HRESULT SetFrequency(DWORD f) override { freq = f; return DS_OK; }
  HRESULT Stop() override {
    pos = cur_pos();
    if (playing) {
      playing = false;
      sf_host_audio_cmd c;
      memset(&c, 0, sizeof(c));
      c.cmd = SF_AUD_STOP;
      c.id = id;
      push(c);
    }
    return DS_OK;
  }
  HRESULT Restore() override { return DS_OK; }
};

struct DSound : IDirectSound {
  ULONG refs = 1;
  HRESULT QueryInterface(REFIID, LPVOID *p) override { if (p) *p = nullptr; return E_NOINTERFACE; }
  ULONG AddRef() override { return ++refs; }
  ULONG Release() override { ULONG r = --refs; if (!r) delete this; return r; }
  HRESULT CreateSoundBuffer(LPCDSBUFFERDESC d, LPDIRECTSOUNDBUFFER *out, LPUNKNOWN) override {
    if (!d || !out) return DSERR_INVALIDPARAM;
    if (d->dwFlags & DSBCAPS_PRIMARYBUFFER) { *out = nullptr; return DSERR_INVALIDPARAM; }
    if (!d->lpwfxFormat || d->dwBufferBytes == 0) return DSERR_INVALIDPARAM;
    DSBuf *b = new DSBuf();
    memset(&b->fmt, 0, sizeof(b->fmt));
    memcpy(&b->fmt, d->lpwfxFormat, sizeof(WAVEFORMAT) + sizeof(WORD));   /* PCMWAVEFORMAT part */
    b->fmt.cbSize = 0;
    b->data.assign(d->dwBufferBytes, b->fmt.wBitsPerSample == 8 ? 0x80 : 0);
    {
      std::lock_guard<std::mutex> lk(g_amtx);
      b->id = g_next_sound++;
    }
    *out = b;
    return DS_OK;
  }
  HRESULT GetCaps(LPDSCAPS) override { return DS_OK; }
  HRESULT DuplicateSoundBuffer(LPDIRECTSOUNDBUFFER, LPDIRECTSOUNDBUFFER *) override { return DSERR_INVALIDCALL; }
  HRESULT SetCooperativeLevel(HWND, DWORD) override { return DS_OK; }
  HRESULT Compact() override { return DS_OK; }
  HRESULT GetSpeakerConfig(LPDWORD c) override { if (c) *c = 4; return DS_OK; }
  HRESULT SetSpeakerConfig(DWORD) override { return DS_OK; }
  HRESULT Initialize(LPCGUID) override { return DS_OK; }
};

}  /* namespace */

namespace w32 {
HRESULT dsound_create(LPDIRECTSOUND *out)
{
  if (!out) return DSERR_INVALIDPARAM;
  *out = new DSound();
  return DS_OK;
}
}  /* namespace w32 */

extern "C" int sf_host_audio_poll(sf_host_audio_cmd *out)
{
  std::lock_guard<std::mutex> lk(g_amtx);
  if (g_aq.empty() || !out) return 0;
  *out = g_aq.front();
  g_aq.pop_front();
  return 1;
}

extern "C" uint32_t sf_host_sound_data(int id, const void **pcm, int *channels, int *bits, int *rate)
{
  std::lock_guard<std::mutex> lk(g_amtx);
  auto it = g_sounds.find(id);
  if (it == g_sounds.end()) return 0;
  if (pcm) *pcm = it->second.pcm.data();
  if (channels) *channels = it->second.ch;
  if (bits) *bits = it->second.bits;
  if (rate) *rate = it->second.rate;
  return (uint32_t)it->second.pcm.size();
}

/* =============================== MCI (MIDI) ============================== */
namespace {

/* Length of a standard MIDI file in milliseconds (tempo map aware). */
uint32_t midi_length_ms(const std::string &path)
{
  char hb[1024];
  FILE *f = fopen(sf_hostpath(path.c_str(), hb, sizeof(hb)), "rb");
  if (!f) return 0;
  std::vector<uint8_t> d;
  fseek(f, 0, SEEK_END);
  long n = ftell(f);
  fseek(f, 0, SEEK_SET);
  if (n > 0) { d.resize(n); if (fread(d.data(), 1, n, f) != (size_t)n) d.clear(); }
  fclose(f);
  if (d.size() < 14 || memcmp(d.data(), "MThd", 4)) return 0;
  auto be16 = [&](size_t p) { return (d[p] << 8) | d[p + 1]; };
  auto be32 = [&](size_t p) { return ((uint32_t)d[p] << 24) | (d[p + 1] << 16) | (d[p + 2] << 8) | d[p + 3]; };
  int ntr = be16(10), div = be16(12);
  std::vector<std::pair<uint64_t, uint32_t>> tempos;   /* tick, us per quarter */
  uint64_t maxTick = 0;
  size_t p = 8 + be32(4);
  for (int t = 0; t < ntr && p + 8 <= d.size(); t++) {
    if (memcmp(&d[p], "MTrk", 4)) break;
    size_t len = be32(p + 4), q = p + 8, end = std::min(d.size(), p + 8 + len);
    uint64_t tick = 0;
    uint8_t running = 0;
    while (q < end) {
      uint32_t delta = 0;
      while (q < end) { uint8_t b = d[q++]; delta = (delta << 7) | (b & 0x7F); if (!(b & 0x80)) break; }
      tick += delta;
      if (q >= end) break;
      uint8_t st = d[q];
      if (st & 0x80) q++;
      else st = running;
      if (st == 0xFF) {
        if (q >= end) break;
        uint8_t type = d[q++];
        uint32_t l = 0;
        while (q < end) { uint8_t b = d[q++]; l = (l << 7) | (b & 0x7F); if (!(b & 0x80)) break; }
        if (type == 0x51 && l == 3 && q + 3 <= end)
          tempos.push_back({tick, (uint32_t)((d[q] << 16) | (d[q + 1] << 8) | d[q + 2])});
        q += l;
        if (type == 0x2F) break;
      } else if (st == 0xF0 || st == 0xF7) {
        uint32_t l = 0;
        while (q < end) { uint8_t b = d[q++]; l = (l << 7) | (b & 0x7F); if (!(b & 0x80)) break; }
        q += l;
      } else {
        running = st;
        int hi = st & 0xF0;
        q += (hi == 0xC0 || hi == 0xD0) ? 1 : 2;
      }
    }
    if (tick > maxTick) maxTick = tick;
    p = p + 8 + len;
  }
  if (div & 0x8000) {   /* SMPTE */
    int fps = -(int8_t)(div >> 8), tpf = div & 0xFF;
    return (uint32_t)(maxTick * 1000 / std::max(1, fps * tpf));
  }
  std::sort(tempos.begin(), tempos.end());
  double us = 0;
  uint64_t last = 0;
  uint32_t tempo = 500000;
  for (auto &tc : tempos) {
    if (tc.first > maxTick) break;
    us += (double)(tc.first - last) * tempo / div;
    last = tc.first;
    tempo = tc.second;
  }
  us += (double)(maxTick - last) * tempo / div;
  return (uint32_t)(us / 1000.0);
}

struct MciDev {
  std::string file;
  uint32_t length = 0;
  bool playing = false;
  int64_t startMs = 0;
  uint32_t pausedAt = 0;
};
std::map<std::string, MciDev> g_mci W32_EARLY;
std::mutex g_mcimtx;

std::vector<std::string> words(const char *s)
{
  std::vector<std::string> w;
  std::string cur;
  bool quote = false;
  for (; *s; s++) {
    if (*s == '"') { quote = !quote; continue; }
    if (!quote && isspace((unsigned char)*s)) { if (!cur.empty()) { w.push_back(cur); cur.clear(); } continue; }
    cur.push_back(*s);
  }
  if (!cur.empty()) w.push_back(cur);
  return w;
}

std::string lower(std::string s) { for (auto &c : s) c = (char)tolower((unsigned char)c); return s; }

void ret(LPSTR buf, UINT n, const std::string &s) { if (buf && n) snprintf(buf, n, "%s", s.c_str()); }

}  /* namespace */

MCIERROR mciSendString(LPCSTR cmd, LPSTR retBuf, UINT retLen, HWND)
{
  if (retBuf && retLen) retBuf[0] = 0;
  if (!cmd) return 0x101 + 0x100;
  std::vector<std::string> w = words(cmd);
  if (w.size() < 2) return 0x100 + 0x11;   /* MCIERR_MISSING_COMMAND_STRING */
  std::string verb = lower(w[0]);
  std::lock_guard<std::mutex> lk(g_mcimtx);
  if (verb == "open") {
    std::string alias = w[1];
    for (size_t i = 2; i + 1 < w.size(); i++) if (lower(w[i]) == "alias") alias = w[i + 1];
    MciDev d;
    d.file = w[1];
    d.length = midi_length_ms(d.file);
    if (d.length == 0) return 0x100 + 19;   /* MCIERR_FILE_NOT_FOUND-ish */
    g_mci[lower(alias)] = d;
    return 0;
  }
  auto it = g_mci.find(lower(w[1]));
  if (it == g_mci.end()) return 0x100 + 7;  /* MCIERR_INVALID_DEVICE_NAME */
  MciDev &d = it->second;
  auto position = [&]() -> uint32_t {
    if (!d.playing) return d.pausedAt;
    int64_t p = now_ms() - d.startMs + d.pausedAt;
    if (p >= (int64_t)d.length) { return d.length; }
    return (uint32_t)p;
  };
  if (verb == "play") {
    if (!d.playing) {
      d.playing = true;
      d.startMs = now_ms();
      sf_host_audio_cmd c;
      memset(&c, 0, sizeof(c));
      c.cmd = SF_AUD_MUSIC_PLAY;
      snprintf(c.path, sizeof(c.path), "%s", d.file.c_str());
      c.restart = d.pausedAt == 0;
      push(c);
    }
    return 0;
  }
  if (verb == "stop" || verb == "pause") {
    if (d.playing) {
      d.pausedAt = verb == "pause" ? position() : 0;
      d.playing = false;
      sf_host_audio_cmd c;
      memset(&c, 0, sizeof(c));
      c.cmd = SF_AUD_MUSIC_STOP;
      push(c);
    }
    return 0;
  }
  if (verb == "close") {
    if (d.playing) {
      sf_host_audio_cmd c;
      memset(&c, 0, sizeof(c));
      c.cmd = SF_AUD_MUSIC_STOP;
      push(c);
    }
    g_mci.erase(it);
    return 0;
  }
  if (verb == "status" && w.size() >= 3) {
    std::string what = lower(w[2]);
    if (what == "length") { ret(retBuf, retLen, std::to_string(d.length)); return 0; }
    if (what == "position") { ret(retBuf, retLen, std::to_string(position())); return 0; }
    if (what == "mode") { ret(retBuf, retLen, d.playing && position() < d.length ? "playing" : "stopped"); return 0; }
    return 0x100 + 0x20;
  }
  if (verb == "set" || verb == "seek") return 0;
  return 0x100 + 0x11;
}

MCIERROR mciSendCommand(MCIDEVICEID, UINT, DWORD_PTR, DWORD_PTR)
{
  /* The program only uses the MCI command interface when DirectSound is
     missing; DirectSound is always present here. */
  return 0x100 + 0x11;
}
