/* ************************************************************************* */
/* sf_ipc.h -- shared-memory protocol between the Godot application (64-bit) */
/* and an original Stellar Frontier program running as a 32-bit process.     */
/*                                                                           */
/* NEW FILE, added 2026-10-12 for the Stellar Frontier Godot port.           */
/*                                                                           */
/* The original programs keep pointers in 32-bit integers, so they run as    */
/* 32-bit processes (see Makefile).  The host creates one shared memory      */
/* block per program and starts the program with                             */
/*     <program>.exe --sf-ipc <name> --sf-parent <pid> [program arguments]  */
/* The block holds:                                                          */
/*   - a header (sizes, state, cursor, frame handshake),                     */
/*   - the composed desktop image (screen_w * screen_h 0x00RRGGBB pixels),   */
/*   - an input ring (host -> program) and an output ring (program -> host)  */
/*     carrying small framed messages.                                       */
/* Every field has a fixed size so 32- and 64-bit code agree on the layout.  */
/*                                                                           */
/* Frame handshake: the program writes a new picture only when             */
/* frame_ack == frame_written, then increments frame_written.  The host      */
/* copies the picture and sets frame_ack = frame_written.  No tearing, no    */
/* waiting on either side.                                                   */
/* ************************************************************************* */
#ifndef SF_IPC_H
#define SF_IPC_H

#include <stdint.h>
#include <string.h>

#define SF_IPC_MAGIC    0x50494653u      /* "SFIP" */
#define SF_IPC_VERSION  1

#define SF_IPC_IN_RING   (256u * 1024u)
#define SF_IPC_OUT_RING  (8u * 1024u * 1024u)

enum {
  SF_IPC_STARTING = 0,
  SF_IPC_RUNNING  = 1,
  SF_IPC_EXITED   = 2,
  SF_IPC_FAILED   = 3
};

/* host -> program messages */
enum {
  SF_IN_MOUSE_MOVE   = 1,   /* x, y                          */
  SF_IN_MOUSE_BUTTON = 2,   /* x, y, button (1 L 2 R 3 M), down */
  SF_IN_MOUSE_WHEEL  = 3,   /* x, y, delta                   */
  SF_IN_KEY          = 4,   /* vk, scancode, extended, down, ch */
  SF_IN_FOCUS        = 5,   /* has_focus                     */
  SF_IN_CLOSE        = 6,   /* host window closing           */
  SF_IN_RESTORE      = 7    /* host window restored          */
};

/* program -> host messages */
enum {
  SF_OUT_LOG          = 101, /* text                                       */
  SF_OUT_OPEN_URL     = 102, /* text                                       */
  SF_OUT_RUN          = 103, /* text: command line the program asked to run */
  SF_OUT_MINIMIZE     = 104, /* i[0]: 1 minimise                           */
  SF_OUT_WARP         = 105, /* i[0], i[1]: desktop position               */
  SF_OUT_SOUND        = 110, /* i[0] id, i[1] channels, i[2] bits, i[3] rate,
                                followed by the PCM bytes (unsigned 8-bit or
                                signed 16-bit little endian, interleaved)  */
  SF_OUT_AUDIO        = 111, /* i[0] cmd (SF_AUD_*), i[1] id, i[2] loop,
                                i[3] volume_mb, i[4] restart, then text path */
  SF_OUT_EXITED       = 120  /* i[0] exit code                             */
};

typedef struct {
  uint32_t magic, version;
  uint32_t header_size, total_size;
  uint32_t screen_w, screen_h;          /* set by the host                  */
  uint32_t frame_offset;                /* pixels                           */
  uint32_t in_offset, in_size;          /* ring buffers (data areas)        */
  uint32_t out_offset, out_size;
  uint32_t emulate_nt;                  /* 1 XP, 0 Windows 98               */
  /* frame handshake */
  volatile uint32_t frame_written;
  volatile uint32_t frame_ack;
  /* program state */
  volatile int32_t  state;
  volatile int32_t  exit_code;
  volatile int32_t  cursor;             /* sf_host_cursor()                 */
  volatile uint32_t child_pid;
  /* rings: head = next write offset (producer), tail = next read (consumer) */
  volatile uint32_t in_head, in_tail;
  volatile uint32_t out_head, out_tail;
  uint32_t reserved[16];
} sf_ipc_header;

/* message framing: [u32 type][u32 length of payload][payload][pad to 4] */
#define SF_IPC_MSG_HDR 8u
static inline uint32_t sf_ipc_pad(uint32_t n) { return (n + 3u) & ~3u; }

static inline uint32_t sf_ipc_load(volatile uint32_t *p)
{ return __atomic_load_n(p, __ATOMIC_ACQUIRE); }
static inline void sf_ipc_store(volatile uint32_t *p, uint32_t v)
{ __atomic_store_n(p, v, __ATOMIC_RELEASE); }

/* Free space in a ring (one byte kept empty to tell full from empty). */
static inline uint32_t sf_ipc_ring_free(uint32_t head, uint32_t tail, uint32_t size)
{ return (tail + size - head - 1u) % size; }

static inline void sf_ipc_ring_put(uint8_t *ring, uint32_t size, uint32_t *pos,
                                   const void *src, uint32_t n)
{
  const uint8_t *s = (const uint8_t *)src;
  uint32_t first = size - *pos;
  if (first > n) first = n;
  memcpy(ring + *pos, s, first);
  if (n > first) memcpy(ring, s + first, n - first);
  *pos = (*pos + n) % size;
}

static inline void sf_ipc_ring_get(const uint8_t *ring, uint32_t size, uint32_t *pos,
                                   void *dst, uint32_t n)
{
  uint8_t *d = (uint8_t *)dst;
  uint32_t first = size - *pos;
  if (first > n) first = n;
  if (d) memcpy(d, ring + *pos, first);
  if (d && n > first) memcpy(d + first, ring, n - first);
  *pos = (*pos + n) % size;
}

/* Write one message (header + up to two payload parts).  Returns 0 when the
   ring has no room (the caller retries). Single producer per ring. */
static inline int sf_ipc_write(uint8_t *ring, uint32_t size, volatile uint32_t *head,
                               volatile uint32_t *tail, uint32_t type,
                               const void *a, uint32_t na, const void *b, uint32_t nb)
{
  uint32_t len = na + nb;
  uint32_t need = SF_IPC_MSG_HDR + sf_ipc_pad(len);
  uint32_t h = *head, t = sf_ipc_load(tail);
  if (need >= size || sf_ipc_ring_free(h, t, size) < need) return 0;
  uint32_t hdr[2] = {type, len};
  uint32_t pos = h;
  sf_ipc_ring_put(ring, size, &pos, hdr, SF_IPC_MSG_HDR);
  if (na) sf_ipc_ring_put(ring, size, &pos, a, na);
  if (nb) sf_ipc_ring_put(ring, size, &pos, b, nb);
  uint32_t padn = sf_ipc_pad(len) - len;
  if (padn) { uint32_t z = 0; sf_ipc_ring_put(ring, size, &pos, &z, padn); }
  sf_ipc_store(head, pos);
  return 1;
}

/* Read one message.  Returns 0 if the ring is empty, else 1 with *type and
   *len set and min(len, cap) payload bytes copied to buf (the rest is
   skipped).  Single consumer per ring. */
static inline int sf_ipc_read(const uint8_t *ring, uint32_t size, volatile uint32_t *head,
                              volatile uint32_t *tail, uint32_t *type, uint32_t *len,
                              void *buf, uint32_t cap)
{
  uint32_t t = *tail, h = sf_ipc_load(head);
  if (t == h) return 0;
  uint32_t hdr[2];
  uint32_t pos = t;
  sf_ipc_ring_get(ring, size, &pos, hdr, SF_IPC_MSG_HDR);
  *type = hdr[0];
  *len = hdr[1];
  uint32_t take = hdr[1] < cap ? hdr[1] : cap;
  sf_ipc_ring_get(ring, size, &pos, buf, take);
  sf_ipc_ring_get(ring, size, &pos, 0, sf_ipc_pad(hdr[1]) - take);
  sf_ipc_store(tail, pos);
  return 1;
}

static inline uint32_t sf_ipc_total_size(uint32_t w, uint32_t h)
{
  return (uint32_t)((sizeof(sf_ipc_header) + 63u) & ~63u) + w * h * 4u +
         SF_IPC_IN_RING + SF_IPC_OUT_RING;
}

/* Fill in the header of a freshly created block (host side). */
static inline void sf_ipc_init(sf_ipc_header *hd, uint32_t w, uint32_t h, uint32_t nt)
{
  memset(hd, 0, sizeof(*hd));
  hd->magic = SF_IPC_MAGIC;
  hd->version = SF_IPC_VERSION;
  hd->header_size = (uint32_t)((sizeof(sf_ipc_header) + 63u) & ~63u);
  hd->screen_w = w;
  hd->screen_h = h;
  hd->frame_offset = hd->header_size;
  hd->in_offset = hd->frame_offset + w * h * 4u;
  hd->in_size = SF_IPC_IN_RING;
  hd->out_offset = hd->in_offset + SF_IPC_IN_RING;
  hd->out_size = SF_IPC_OUT_RING;
  hd->total_size = hd->out_offset + SF_IPC_OUT_RING;
  hd->emulate_nt = nt;
  hd->cursor = 0;
  hd->state = SF_IPC_STARTING;
}

#endif /* SF_IPC_H */
