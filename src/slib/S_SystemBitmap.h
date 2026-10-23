/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
#ifndef S_SYSTEMBITMAP
#define S_SYSTEMBITMAP

#include "S_Bitmap.h"
#include "S_BitmapData.h"

#define S_SYSTEMBITMAP_NOTLOADED     1
#define S_SYSTEMBITMAP_OK            0
#define S_SYSTEMBITMAP_NODATA       -5
#define S_SYSTEMBITMAP_CREATEFAILED -7

#ifdef UNIX
   typedef void *HDC;
   typedef void *HBITMAP;
   typedef void *LPDIRECTDRAWSURFACE;
#endif

#ifdef WIN32
   HBITMAP _CreateWIN32Bitmap(BitmapInfo3Struct *,char *);
#endif
#ifdef OS2
   HBITMAP _CreateOS2Bitmap(BitmapInfo3Struct *,char *);
#endif

class S_Canvas;
class S_Palette;

class S_SystemBitmap : public S_Bitmap
{
 public:
  int rc;
  
  HDC hdc;
  HBITMAP ohbm;
  HBITMAP hbm;
  LPDIRECTDRAWSURFACE lpBuffer;
  
  int RestoreBuffer();
  void *Lock();
  int Unlock();
  
  int Blit(S_Canvas *,int,int=0,int=0,ulong=0);
  int Blit(S_Canvas *,int,int,int,int,int,int=0,int=0,ulong=0);
  int StretchBlit(S_Canvas *,int,int,int,int,int,ulong=0);
  int StretchBlit(S_Canvas *,int,int,int,int,int,int,int,int,int,ulong=0);
  
  int SystemBlit(S_Canvas *,int,int,int,int,int,int,ulong=ROP_SRCCOPY);
  int SystemBlit(S_Canvas *,int,int,int,int,int,int,int,int,ulong=ROP_SRCCOPY);
  
  int SystemBlit(S_SystemBitmap *,int,int,int,int,int,int,ulong=ROP_SRCCOPY);
  int SystemBlit(S_SystemBitmap *,int,int,int,int,int,int,int,int,ulong=ROP_SRCCOPY);
  
  S_Palette *SetPalette(S_Palette * =NULL);
  S_Palette *oldPalette;
  int ResetPalette();
  
  int Init();
  S_SystemBitmap();
  S_SystemBitmap( uchar *,ulong,slong=S_BITMAP_MASK_NONE,
		  slong=S_BITMAP_BLIT_OPAQUE,
		  slong=1,slong=100,ulong=0x000000 );
  S_SystemBitmap( char *,slong=S_BITMAP_MASK_NONE,
		  slong=S_BITMAP_BLIT_OPAQUE,
		  slong=1,slong=100,ulong=0x000000 );
  S_SystemBitmap( S_BitmapData *,slong=S_BITMAP_MASK_NONE,
		  slong=S_BITMAP_BLIT_OPAQUE,
		  slong=1,slong=100,ulong=0x000000 );
  int Create(uchar *,ulong,slong=S_BITMAP_MASK_NONE,
	 slong=S_BITMAP_BLIT_OPAQUE,
	 slong=1,slong=100,ulong=0x000000);
  int ZCreate(char *,slong=S_BITMAP_MASK_NONE,
          slong=S_BITMAP_BLIT_OPAQUE,
          slong=1,slong=100,ulong=0x000000);
  int Create(char *,slong=S_BITMAP_MASK_NONE,
	 slong=S_BITMAP_BLIT_OPAQUE,
	 slong=1,slong=100,ulong=0x000000);
  int Create(S_BitmapData *,slong=S_BITMAP_MASK_NONE,
	 slong=S_BITMAP_BLIT_OPAQUE,
	 slong=1,slong=100,ulong=0x000000);
  int Destroy();
  ~S_SystemBitmap();
};

#endif
