/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
#include "S_OS.h"
#include "S_Palette.h"
#include "S_Canvas.h"
#include "S_SystemBitmap.h"
#include "S_Root.h"

uchar _rawDefaultPaletteEntries[768] = {

  0,  0,  0,    128,  0,  0,      0,128,  0,    128,128,  0,
  0,  0,128,    128,  0,128,      0,128,128,    192,192,192,
162,141,104,    213,204,187,

192,192,192,     208,208,208,   224,224,224,    240,240,240,
165,165,165,     173,173,173,

 16, 16, 16,     32, 32, 32,
 48, 48, 48,     64, 64, 64,    80, 80, 80,     96, 96, 96,

128,128,128,     85,  0,255,    170,  0,255,    144,144,144,
  0, 36,255,     85, 36,255,    170, 36,255,    255, 36,255,
  0, 73,255,     85, 73,255,    170, 73,255,    255, 73,255,
  0,109,255,     85,109,255,    170,109,255,    255,109,255,
  0,146,255,     85,146,255,    170,146,255,    255,146,255,
  0,182,255,     85,182,255,    170,182,255,    255,182,255,
  0,219,255,     85,219,255,    170,219,255,    255,219,255,
160,160,160,     85,255,255,    170,255,255,    176,176,176,

112,112,112,     85,  0,  0,    170,  0,  0,    255,  0,  0,
  0, 36,  0,     85, 36,  0,    170, 36,  0,    255, 36,  0,
  0, 73,  0,     85, 73,  0,    170, 73,  0,    255, 73,  0,
  0,109,  0,     85,109,  0,    170,109,  0,    255,109,  0,
  0,146,  0,     85,146,  0,    170,146,  0,    255,146,  0,
  0,182,  0,     85,182,  0,    170,182,  0,    255,182,  0,
  0,219,  0,     85,219,  0,    170,219,  0,    255,219,  0,
  0,255,  0,     85,255,  0,    170,255,  0,    255,255,  0,

  0,  0, 43,     85,  0, 43,    170,  0, 43,    255,  0, 43,
  0, 36, 43,     85, 36, 43,    170, 36, 43,    255, 36, 43,
  0, 73, 43,     85, 73, 43,    170, 73, 43,    255, 73, 43,
  0,109, 43,     85,109, 43,    170,109, 43,    255,109, 43,
  0,146, 43,     85,146, 43,    170,146, 43,    255,146, 43,
  0,182, 43,     85,182, 43,    170,182, 43,    255,182, 43,
  0,219, 43,     85,219, 43,    170,219, 43,    255,219, 43,
  0,255, 43,     85,255, 43,    170,255, 43,    255,255, 43,

  0,  0, 85,     85,  0, 85,    170,  0, 85,    255,  0, 85,
  0, 36, 85,     85, 36, 85,    170, 36, 85,    255, 36, 85,
  0, 73, 85,     85, 73, 85,    170, 73, 85,    255, 73, 85,
  0,109, 85,     85,109, 85,    170,109, 85,    255,109, 85,
  0,146, 85,     85,146, 85,    170,146, 85,    255,146, 85,
  0,182, 85,     85,182, 85,    170,182, 85,    255,182, 85,
  0,219, 85,     85,219, 85,    170,219, 85,    255,219, 85,
  0,255, 85,     85,255, 85,    170,255, 85,    255,255, 85,

  0,  0,128,     85,  0,128,    170,  0,128,    255,  0,128,
  0, 36,128,     85, 36,128,    170, 36,128,    255, 36,128,
  0, 73,128,     85, 73,128,    170, 73,128,    255, 73,128,
  0,109,128,     85,109,128,    170,109,128,    255,109,128,
  0,146,128,     85,146,128,    170,146,128,    255,146,128,
  0,182,128,     85,182,128,    170,182,128,    255,182,128,
  0,219,128,     85,219,128,    170,219,128,    255,219,128,
  0,255,128,     85,255,128,    170,255,128,    255,255,128,

  0,  0,170,     85,  0,170,    170,  0,170,    255,  0,170,
  0, 36,170,     85, 36,170,    170, 36,170,    255, 36,170,
  0, 73,170,     85, 73,170,    170, 73,170,    255, 73,170,
  0,109,170,     85,109,170,    170,109,170,    255,109,170,
  0,146,170,     85,146,170,    170,146,170,    255,146,170,
  0,182,170,     85,182,170,    170,182,170,    255,182,170,
  0,219,170,     85,219,170,    170,219,170,    255,219,170,
  0,255,170,     85,255,170,    170,255,170,    255,255,170,

  0,  0,213,     85,  0,213,    170,  0,213,    255,  0,213,
  0, 36,213,     85, 36,213,    170, 36,213,    255, 36,213,
  0, 73,213,     85, 73,213,    170, 73,213,    255, 73,213,
  0,109,213,     85,109,213,    170,109,213,    255,109,213,
  0,146,213,     85,146,213,    170,146,213,    255,146,213,
  0,182,213,     85,182,213,    170,182,213,    255,182,213,
  0,219,213,     85,219,213,    170,219,213,    255,219,213,
  0,255,213,     85,255,213,    170,255,213,    255,255,213,

                                255,251,240,    160,160,164,
128,128,128,     255,  0,  0,     0,255,  0,    255,255,  0,
  0,  0,255,     255,  0,255,     0,255,255,    255,255,255
 };

RgbStruct *_defaultPaletteEntries = (RgbStruct *)_rawDefaultPaletteEntries;



S_Palette::S_Palette() {
   hPal = NULL;
   lpDDPal = NULL;
   numEntries = 0;
   entries = NULL;
   }

S_Palette::S_Palette(RgbStruct *rgb,int inumEntries) {
   numEntries = inumEntries;
   if ( rgb == NULL ) {
      rgb = _defaultPaletteEntries;
      numEntries = 256;
      }
   entries = (RgbStruct *)S_malloc(numEntries*sizeof(RgbStruct));

#  ifdef OS2
      Rgb2Struct *pal = (Rgb2Struct *)S_malloc(numEntries*sizeof(Rgb2Struct));
      for ( int i=0; i<numEntries; i++ ) {
         pal[i].bRed = rgb[i].bRed;
         pal[i].bGreen = rgb[i].bGreen;
         pal[i].bBlue = rgb[i].bBlue;
         pal[i].fcOptions = 0;
         }
/*
      hPal = GpiCreatePalette(_root->hab,
                              LCOL_PURECOLOR|LCOL_OVERRIDE_DEFAULT_COLORS,
                              LCOLF_CONSECRGB,numEntries,(ulong *)pal);
*/
      S_free(pal);
#  endif

#  ifdef WIN32
      LPLOGPALETTE pLogPal = (LPLOGPALETTE) 
         S_malloc( sizeof(LOGPALETTE)+numEntries*sizeof(PALETTEENTRY) );
      pLogPal->palVersion = 0x300;
      pLogPal->palNumEntries = numEntries;

      for ( int i=0; i<numEntries; i++ ) {
         pLogPal->palPalEntry[i].peRed = rgb[i].bRed;
         pLogPal->palPalEntry[i].peGreen = rgb[i].bGreen;
         pLogPal->palPalEntry[i].peBlue = rgb[i].bBlue;
         pLogPal->palPalEntry[i].peFlags = 0;
         }

      lpDDPal = NULL;
      if ( _root && _root->lpDD )
         _root->lpDD->CreatePalette(DDPCAPS_8BIT,
                                    pLogPal->palPalEntry,
                                    &lpDDPal,NULL);

      hPal = CreatePalette(pLogPal);
      GetPaletteEntries(hPal,0,numEntries,&pLogPal->palPalEntry[0]);
      for (int i=0; i<numEntries; i++ ) { /* port: 'int' (old for-scope rule) */
         entries[i].bRed   = pLogPal->palPalEntry[i].peRed;
         entries[i].bGreen = pLogPal->palPalEntry[i].peGreen;
         entries[i].bBlue  = pLogPal->palPalEntry[i].peBlue;
         }

      S_free(pLogPal);
#  endif

   }

S_Palette::~S_Palette() {
   if ( entries ) {
      S_free(entries);
      entries = NULL;
      }
#  ifdef OS2
   if ( hPal ) {
      GpiDeletePalette(hPal);
      hPal = NULL;
      }
#  endif
#  ifdef WIN32
   if ( lpDDPal ) {
      lpDDPal->Release();
      lpDDPal = NULL;
      }
   if ( hPal ) {
      DeleteObject(hPal);
      hPal = NULL;
      }
#  endif
   }

HPAL 
_SelectPalette(HDC hdc,HPAL hPal) {
   if ( !hdc ) return 0;
#  ifdef OS2
      /*return GpiSelectPalette(hdc,NULL);*/
#  endif
#  ifdef WIN32
      return SelectPalette(hdc,hPal,FALSE);
#  endif
   return 0;
   }


int
_RealizePalette(HDC hdc) {
#  ifdef OS2
      /*WinRealizePalette(hwnd,hdc,NULL);*/
#  endif
#  ifdef WIN32
      RealizePalette(hdc);
#  endif
   return 0;
   }


S_Palette *
S_SystemBitmap::SetPalette(S_Palette *ipalette) {
   if ( cBitCount != 8 )
      return NULL;

   S_Palette *palette = ipalette;
   if ( palette == NULL && _root  )
      palette = _root->palette;

#  ifdef WIN32
      HRESULT rc;
      if ( lpBuffer )
         rc = lpBuffer->SetPalette(palette->lpDDPal);
#  endif
   if ( hdc ) {
      if ( !oldPalette )
         oldPalette = new S_Palette();
      if ( oldPalette->hPal )
         _SelectPalette(hdc,oldPalette->hPal);
      if ( palette )
         oldPalette->hPal = _SelectPalette(hdc,palette->hPal);
      _RealizePalette(hdc);
      }

   return palette;
   }


int
S_SystemBitmap::ResetPalette() {
   if ( oldPalette ) {
      if ( oldPalette->hPal )
         _SelectPalette(hdc,oldPalette->hPal);
      delete oldPalette;
      oldPalette = NULL;
      }
   return 0;
   }


int
S_Canvas::ResetPalette() {
   if ( oldPalette ) {
      if ( oldPalette->hPal )
         _SelectPalette(hdc,oldPalette->hPal);
      delete oldPalette;
      oldPalette = NULL;
      }
   return 0;
   }

S_Palette *
S_Canvas::SetPalette(S_Palette *ipalette) {
   if ( _root && _root->bitsPerPixel != 8 )
      return NULL;

   S_Palette *palette = ipalette;
   if ( palette == NULL && _root )
      palette = _root->palette;

#  ifdef WIN32
      HRESULT rc;
      if ( lpBuffer )
         rc = lpBuffer->SetPalette(palette->lpDDPal);
#  endif
   if ( hdc ) {
      if ( !oldPalette )
         oldPalette = new S_Palette();
      if ( oldPalette && oldPalette->hPal )
         _SelectPalette(hdc,oldPalette->hPal);
      if ( palette )
         oldPalette->hPal = _SelectPalette(hdc,palette->hPal);
      _RealizePalette(hdc);
      }

   return palette;
   }

