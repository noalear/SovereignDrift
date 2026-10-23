/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
#ifndef S_BITMAP
#define S_BITMAP

#include "S_BitmapData.h"

#define ROP_NOTSRCAND   0x00220000L
#define ROP_TRANSPARENT 0xFFFFFFFFL

#define S_BITMAP_OK                     0
#define S_BITMAP_LOAD_FILEDOESNOTEXIST -1
#define S_BITMAP_LOAD_INVALIDNUMBITS   -2
#define S_BITMAP_LOAD_INVALIDNUMFRAMES -3
#define S_BITMAP_LOAD_CANNOTOPENFILE   -4
#define S_BITMAP_NODATA                -5
#define S_BITMAP_MALLOCFAILED          -6

#define S_BITMAP_MASK_OPAQUE      0
#define S_BITMAP_MASK_NONE        0
#define S_BITMAP_MASK_TRANSPARENT 1
#define S_BITMAP_MASK_TRANSLUCENT 2

#define S_BITMAP_BLIT_DEFAULT          0
#define S_BITMAP_BLIT_USEMASK          1
#define S_BITMAP_BLIT_TRANSPARENT      2
#define S_BITMAP_BLIT_TRANSLUCENT      3
#define S_BITMAP_BLIT_XOR              4
#define S_BITMAP_BLIT_AND              5
#define S_BITMAP_BLIT_OPAQUE           6
#define S_BITMAP_BLIT_MASKONLY         7
#define S_BITMAP_BLIT_MASKONLYNOT      8

class S_Canvas;

class S_Bitmap : public S_BitmapData
{
 public:
  int rc;
  
  ulong cx,cy;
  unsigned short cBitCount;
  slong maskFlags;
  slong numFrames;
  slong defaultBlitFlags;
  ulong cySrc;
  ulong tColor;
  slong gamma;
  
  ulong secretCode;
  ulong checkSum;
  
  int whiteIndex;
  int blackIndex;
  
  int Blit(S_Canvas *,int,int=0,int=0,ulong=0);
  int Blit(S_Canvas *,int,int,int,int,int,int=0,int=0,ulong=0);
  int StretchBlit(S_Canvas *,int,int,int,int,int,ulong=0);
  int StretchBlit(S_Canvas *,int,int,int,int,int,int,int,int,int,ulong=0);
  
  int SystemBlit(S_Canvas *,int,int,int,int,int,int,ulong);
  int SystemBlit(S_Canvas *,int,int,int,int,int,int,int,int,ulong);
  
  int Init();
  S_Bitmap();
  S_Bitmap( char *,slong=S_BITMAP_MASK_NONE,slong=S_BITMAP_BLIT_OPAQUE,
	    slong=1,slong=100,ulong=0x000000 );
  S_Bitmap( S_BitmapData *,slong=S_BITMAP_MASK_NONE,slong=S_BITMAP_BLIT_OPAQUE,
	    slong=1,slong=100,ulong=0x000000 );
  int Create( char *,slong=S_BITMAP_MASK_NONE,slong=S_BITMAP_BLIT_OPAQUE,
	      slong=1,slong=100,ulong=0x000000 );
  int Create(S_BitmapData *,slong=S_BITMAP_MASK_NONE,slong=S_BITMAP_BLIT_OPAQUE,
	     slong=1,slong=100,ulong=0x000000 );
  int Destroy();
  ~S_Bitmap();
};

#endif
