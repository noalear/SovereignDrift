/* sf_zlib.cpp -- zlib entry points with the 1997 program's declarations.
   Added 2026-10-12 for the Stellar Frontier Godot port (new file).
   S_ZArchive.cpp declares compress()/uncompress() with 32-bit length
   pointers (Win32 'unsigned long'); zlib is built with Z_PREFIX and these
   wrappers adapt the lengths, so the result is right on 64-bit hosts too. */
#include <stdint.h>
#define Z_PREFIX
#include "zlib.h"
#undef compress
#undef uncompress

extern "C" int compress(unsigned char *dst, uint32_t *dstLen, const unsigned char *src, uint32_t srcLen)
{
  uLongf l = *dstLen;
  int r = z_compress(dst, &l, src, srcLen);
  *dstLen = (uint32_t)l;
  return r;
}

extern "C" int uncompress(unsigned char *dst, uint32_t *dstLen, const unsigned char *src, uint32_t srcLen)
{
  uLongf l = *dstLen;
  int r = z_uncompress(dst, &l, src, srcLen);
  *dstLen = (uint32_t)l;
  return r;
}
