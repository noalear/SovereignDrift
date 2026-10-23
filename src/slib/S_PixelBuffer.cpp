/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
#include "S_OS.h"
#include "S_PixelBuffer.h"
#include "S_BitmapData.h"
#include "S_Canvas.h"
#include "S_Root.h"
#include "S_Palette.h"

S_PixelBuffer::S_PixelBuffer(ulong icx,ulong icy,ulong ibits) {
   CreateBuffer(icx,icy,ibits);
   }

S_PixelBuffer::S_PixelBuffer(S_BitmapData *pbmp) {
   int i,j,k;

   CreateBuffer(pbmp->pbmpInfo3->cx,
                pbmp->pbmpInfo3->cy,
                pbmp->pbmpInfo3->cBitCount);
   *pbmi = *(pbmp->pbmpInfo3);

   /* ********************************************************************** */
   /* *** If the display mode is 8bit, then remap the palette to match   *** */
   /* *** closest colors in selected palette in _root->palette.          *** */
   /* ********************************************************************** */
   RgbStruct *p1 = _root->palette->entries;
   Rgb2Struct *p2 = pbmp->ppalData;
   if ( _root->bitsPerPixel == 8 ) {
      for ( j=0; j<(1<<pbmp->pbmpInfo3->cBitCount); j++ ) {
         ulong minRSqr = 0xFFFFFFFF;
         for ( k=0; k<_root->palette->numEntries; k++ ) {
            if ( minRSqr > 0 ) {
               ulong rSqr =
                  (p1[k].bRed  -p2[j].bRed  )*(p1[k].bRed  -p2[j].bRed  )+
                  (p1[k].bGreen-p2[j].bGreen)*(p1[k].bGreen-p2[j].bGreen)+
                  (p1[k].bBlue -p2[j].bBlue )*(p1[k].bBlue -p2[j].bBlue );
               if ( rSqr < minRSqr ) {
                  minRSqr = rSqr;
                  ppal[j] = k;
                  }
               }
            }
         }
      }
   else {
     int numColors = pbmp->pbmpInfo3->cclrUsed;
     if ( numColors == 0 )
       {
	 numColors = (1<<pbmp->pbmpInfo3->cBitCount);
       }
      for ( j=0; j<numColors; j++ )
         prgbpal[j] = p2[j];
      }

   if ( (pbmi->cBitCount == 8 && pbmi->ulCompression == 0) ||
         _root->bitsPerPixel != 8 ) {
      for ( i=0; i<(int)bufferSize; i++ )
         buffer[i] = pbmp->pimageData[i];
      }
   else if ( pbmi->cBitCount == 8 && pbmi->ulCompression == BCA_RLE8 ) {
      pbmi->ulCompression = 0;
      pbmi->cbImage = 0;
      int i = 0;
      int j = 0;
      int endOfBitmap = FALSE;
      while ( !endOfBitmap ) {
         uchar ch1 = pbmp->pimageData[i++];
         uchar ch2 = pbmp->pimageData[i++];
         if ( ch1 == 0 ) {
            switch( ch2 ) {
               case 0:
                  j += ((bufferSize/cy)-(j%(bufferSize/cy)))%(bufferSize/cy);
                  break;
               case 1:
                  endOfBitmap = TRUE;
                  break;
               case 2:
		 {
		   //int dx = pbmp->pimageData[i++];
		   //int dy = pbmp->pimageData[i++];
	          }
                  break;
               default: {
                  for ( int k=0; k<ch2; k++ )
                     buffer[j++] = pbmp->pimageData[i++];
                  i += (2 - (i%2)) % 2;
                  break;
	          }
               }
            }
         else {
            for ( int k=0; k<ch1; k++ )
               buffer[j++] = ch2;
	    }
         }
      }

}


int
S_PixelBuffer::CreateBuffer(int icx,int icy,int ibits) {
   cx = icx;
   cy = icy;
   bits = ibits;
   bufferSize = cy * (((slong)(cx*((float)bits/8.0)) + 3) / 4) * 4;
   buffer = (char *)S_malloc(bufferSize);

   infoBytes = 40;
   if ( bits != 24 )
      infoBytes += (1<<bits)*sizeof(ushort);
   pbmi = (BitmapInfo3Struct *) S_malloc( infoBytes );

   int i;
   for ( i=0; i<(int)infoBytes; i++ )
      ((char *)pbmi)[i] = 0;

   pbmi->cbFix = 40;
   pbmi->cx = cx;
   pbmi->cy = cy;
   pbmi->cPlanes = 1;
   pbmi->cBitCount = (ushort)bits;
   pbmi->ulCompression = 0;
   pbmi->cbImage = 0;
   pbmi->cxResolution = 0;
   pbmi->cyResolution = 0;
   pbmi->cclrUsed = 0;
   pbmi->cclrImportant = 0;

   ppal = (ushort *)&pbmi->argbColor[0];
   prgbpal = (Rgb2Struct *)&pbmi->argbColor[0];
   for ( i=0; i<(1<<bits); i++ )
      ppal[i] = i;

   return 0;
   }


int
S_PixelBuffer::DestroyBuffer()
{
  if ( pbmi )
    {
      S_free(pbmi);
      pbmi = NULL;
    }
  if ( buffer )
    {
      S_free(buffer);
      buffer = NULL;
    }
  return 0;
}

S_PixelBuffer::~S_PixelBuffer() {
   DestroyBuffer();
   }


int
S_PixelBuffer::SystemBlit(S_Canvas *target,
                          int x1targ,int y1targ,int x2targ,int y2targ,
                          int x1src,int y1src ) {

   if ( !target || (target && !target->hdc) || !buffer ) {
      return 0;
      }

#  ifdef OS2
      POINTL aptl[4] = {x1targ,target->cyCanvas-y2targ,
                        x2targ-1,target->cyCanvas-y1targ-1,
                        x1src,y1src,
                        x1src+(x2targ-x1targ),y1src+(y2targ-y1targ)};
      GpiDrawBits(target->hdc,buffer,(PBITMAPINFO2)pbmi,
                  4,aptl,ROP_SRCCOPY,BBO_IGNORE);
#  endif
#  ifdef WIN32
      ulong pal = DIB_RGB_COLORS;
      if ( _root->bitsPerPixel == 8 )
         pal = DIB_PAL_COLORS;
      StretchDIBits(target->hdc,x1targ,y1targ,x2targ-x1targ,y2targ-y1targ,
                    x1src,y1src,x2targ-x1targ,y2targ-y1targ,buffer,
                    (BITMAPINFO *)pbmi,pal,ROP_SRCCOPY);
#  endif
   return 0;
   }


int
S_PixelBuffer::SystemBlit(S_Canvas *target,
                          int x1targ,int y1targ,int x2targ,int y2targ,
                          int x1src,int y1src,int x2src,int y2src ) {

   if ( !target || (target && !target->hdc) || !buffer ) {
      return 0;
      }

#  ifdef OS2
      POINTL aptl[4] = {x1targ  ,target->cyCanvas-y2targ,
                        x2targ-1,target->cyCanvas-y1targ-1,
                        x1src,y1src,x2src,y2src+1};
      GpiDrawBits(target->hdc,buffer,(PBITMAPINFO2)pbmi,
                  4,aptl,ROP_SRCCOPY,BBO_IGNORE);
#  endif
#  ifdef WIN32
      ulong pal = DIB_RGB_COLORS;
      if ( _root->bitsPerPixel == 8 )
         pal = DIB_PAL_COLORS;
      StretchDIBits(target->hdc,x1targ,y1targ,x2targ-x1targ,y2targ-y1targ,
                    x1src,y1src,x2src-x1src,y2src-y1src,buffer,
                    (BITMAPINFO *)pbmi,pal,ROP_SRCCOPY);
#  endif
   return 0;
   }
