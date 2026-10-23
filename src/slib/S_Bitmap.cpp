/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
#include "S_OS.h"
#include "S_Bitmap.h"
#include "S_Canvas.h"
#include "S_Palette.h"
#include "S_Root.h"

int
S_Bitmap::Init() {
   numFrames = 1;
   maskFlags = 0;
   defaultBlitFlags = 0;
   gamma = 100;
   tColor = 0;
   return 0;
   }

S_Bitmap::S_Bitmap() {
   Init();
   }

S_Bitmap::S_Bitmap(char *ifilename,
                   slong imaskFlags,slong idefaultBlitFlags,
                   slong inumFrames,slong igamma,ulong itColor ) {
   Init();
   Create(ifilename,imaskFlags,idefaultBlitFlags,inumFrames,igamma,itColor);
   }


int
S_Bitmap::Create(char *ifilename,
                 slong imaskFlags,slong idefaultBlitFlags,
                 slong inumFrames,slong igamma,ulong itColor ) {

   Destroy();
   S_BitmapData bmpData(ifilename);
   secretCode = bmpData.secretCode;
   checkSum = bmpData.checkSum;
   Create(&bmpData,imaskFlags,idefaultBlitFlags,inumFrames,igamma,itColor);
   return 0;
   }

S_Bitmap::S_Bitmap(S_BitmapData *bmpData,slong imaskFlags,
                   slong idefaultBlitFlags,
                   slong inumFrames,slong igamma,ulong itColor ) {
   Init();
   Create(bmpData,imaskFlags,idefaultBlitFlags,inumFrames,igamma,itColor);
   }


int
S_Bitmap::Create(S_BitmapData *bmpData,slong imaskFlags,
                 slong idefaultBlitFlags,
                 slong inumFrames,slong igamma,ulong itColor ) {

   Destroy();

   maskFlags = imaskFlags;
   defaultBlitFlags = idefaultBlitFlags;
   numFrames = inumFrames;
   gamma = igamma;
   tColor = itColor;

   rc = bmpData->rc;
   if ( rc < 0 )
      return rc;

   ulong      i,j;
   ulong infoBytes;
   RgbStruct *rgbColorData = NULL;

   char              *rawImageData = NULL;
   char              *combinedImageData = NULL;
   ulong      numColors;

   if ( numFrames <= 0 ) {
      rc = S_BITMAP_LOAD_INVALIDNUMFRAMES;
      return rc;
      }

   infoBytes = bmpData->infoBytes;
   pbmpInfo3 = (BitmapInfo3Struct *)S_malloc(infoBytes);

   if ( !pbmpInfo3 ) {
      rc = S_BITMAP_MALLOCFAILED;
      return rc;
      }

   for ( i=0; i<infoBytes; i++ )
      ((char *)pbmpInfo3)[i] = ((char *)bmpData->pbmpInfo3)[i];

   ppalData = &pbmpInfo3->argbColor[0];

   imageBytes = bmpData->imageBytes;
   rawImageData = (char *) S_malloc( imageBytes );

   if ( !rawImageData ) {
      S_free(pbmpInfo3);
      rc = S_BITMAP_MALLOCFAILED;
      return rc;
      }

   for ( i=0; i<imageBytes; i++ )
      rawImageData[i] = bmpData->pimageData[i];

   numColors = 1<<pbmpInfo3->cBitCount;
   if ( pbmpInfo3->cBitCount == 24 ) {
      numColors = ( pbmpInfo3->cy * 
                     (((slong)(pbmpInfo3->cx*(
                        (float)pbmpInfo3->cBitCount/8.0)) + 3) / 4) * 4)/3;
      rgbColorData = (RgbStruct *) rawImageData;
      }

   RgbStruct transparent;
   transparent.bRed =   (char)((tColor & 0x00FF0000)>>16);
   transparent.bGreen = (char)((tColor & 0x0000FF00)>>8);
   transparent.bBlue =  (char)((tColor & 0x000000FF));

   /* *********************************************************** */
   /* *** If tColor=0 then use the first palette entry as the *** */
   /* *** transparent color                                   *** */
   /* *********************************************************** */
   if ( tColor == 0 ) {
      if ( pbmpInfo3->cBitCount != 24 ) {
         transparent.bRed = pbmpInfo3->argbColor[0].bRed;
         transparent.bGreen = pbmpInfo3->argbColor[0].bGreen;
         transparent.bBlue = pbmpInfo3->argbColor[0].bBlue;
         }
      else {
         transparent.bRed = rgbColorData[0].bRed;
         transparent.bGreen = rgbColorData[0].bGreen;
         transparent.bBlue = rgbColorData[0].bBlue;
         }
      }

   pbmpInfo3->cclrUsed = 0;
   cx = pbmpInfo3->cx;
   cy = pbmpInfo3->cy/numFrames;
   cBitCount = pbmpInfo3->cBitCount;

   whiteIndex = -1;
   blackIndex = -1;

   if ( maskFlags == S_BITMAP_MASK_TRANSPARENT ||
        maskFlags == S_BITMAP_MASK_TRANSLUCENT ) {

      /* ************************************************************ */
      /* *** Transparency mask will sit on top of bmp so cy->cy*2 *** */
      /* ************************************************************ */
      pbmpInfo3->cy *= 2;

      /* ********************************************************* */
      /* *** Make a buffer to hold both the image and the mask *** */
      /* ********************************************************* */
      combinedImageData = (char *) S_malloc(imageBytes*2);

      if ( !combinedImageData ) {
         S_free(pbmpInfo3);
         S_free(rawImageData);
         rc = S_BITMAP_MALLOCFAILED;
         return rc;
         }

      /* ******************************************************************* */
      /* *** Fill the buffer with two copies, stripping off the last two *** */
      /* *** bytes of the first copy for RLE encoded bmps                *** */
      /* ******************************************************************* */
      int strip = 0;
      if ( pbmpInfo3->ulCompression != 0 )
         strip = 2;
      for( i=0; i<imageBytes; i++ )
         combinedImageData[i] = rawImageData[i];
      for ( i=0; i<imageBytes; i++ )
        combinedImageData[imageBytes-strip+i]=rawImageData[i];

      if ( pbmpInfo3->ulCompression != 0 ) {
         pbmpInfo3->cbImage *= 2;
         pbmpInfo3->cbImage -= strip;
         }

      /* ************************************************************* */
      /* *** Check to see if palette has white (255,255,255) entry *** */
      /* *** and black (0,0,0) entry.  If so, record their values  *** */
      /* *** If not, add them, taking over existing entries if     *** */
      /* *** necessary.                                            *** */
      /* ************************************************************* */
      if ( pbmpInfo3->cBitCount != 24 ) {
         for ( i=0; i<numColors; i++ ) {

            int red = pbmpInfo3->argbColor[i].bRed;
            int green = pbmpInfo3->argbColor[i].bGreen;
            int blue = pbmpInfo3->argbColor[i].bBlue;

            if ( red == 255 && green == 255 && blue == 255 && whiteIndex < 0 )
               whiteIndex = i;

            if ( red == 0 && green == 0 && blue == 0 && blackIndex < 0 )
               blackIndex = i;
            }
         if ( whiteIndex < 0 ) {
            whiteIndex = numColors-1;
            pbmpInfo3->argbColor[whiteIndex].bRed = 255;
            pbmpInfo3->argbColor[whiteIndex].bGreen = 255;
            pbmpInfo3->argbColor[whiteIndex].bBlue = 255;
            }
         }
      }
   /* ********************************************************************** */


   /* *********************************************************** */
   /* **** Make translucent colors to be completely black    **** */
   /* *********************************************************** */
   if ( tColor > 0 && 
        maskFlags == S_BITMAP_MASK_TRANSLUCENT ||
       (defaultBlitFlags<<24)>>24 == S_BITMAP_BLIT_TRANSLUCENT) {

      /* ******************************************************************* */
      /* ************ Modify the palette to provide transparency *********** */
      /* ******************************************************************* */
      if ( pbmpInfo3->cBitCount != 24 ) {
         for ( i=0; i<numColors; i++ ) {
            if (pbmpInfo3->argbColor[i].bRed<=transparent.bRed &&
                pbmpInfo3->argbColor[i].bGreen<=transparent.bGreen&&
                pbmpInfo3->argbColor[i].bBlue<=transparent.bBlue ) {
               pbmpInfo3->argbColor[i].bRed = 0;
               pbmpInfo3->argbColor[i].bGreen = 0;
               pbmpInfo3->argbColor[i].bBlue = 0;
               }
            }
         }
      else if ( pbmpInfo3->cBitCount == 24 && 
                pbmpInfo3->ulCompression == 0 ) {
         for ( i=0; i<numColors; i++ ) {
            if (rgbColorData[i].bRed<=transparent.bRed &&
                rgbColorData[i].bGreen<=transparent.bGreen&&
                rgbColorData[i].bBlue<=transparent.bBlue ) {
               rgbColorData[i].bRed = 0;
               rgbColorData[i].bGreen = 0;
               rgbColorData[i].bBlue = 0;
               }
            }
         }
      transparent.bRed = 0;
      transparent.bGreen = 0;
      transparent.bBlue = 0;
      }
   /* *********************************************************** */

   if ( maskFlags == S_BITMAP_MASK_TRANSPARENT ||
        maskFlags == S_BITMAP_MASK_TRANSLUCENT ) {

      /* ***************************************************** */
      /* ***** Modify image data to provide transparency ***** */
      /* ***** For starters we'll just do RLE8 bitmaps   ***** */
      /* ***************************************************** */
      if ( pbmpInfo3->cBitCount==8 && pbmpInfo3->ulCompression==BCA_RLE8 && 
           pbmpInfo3->cbImage != 0 ) {
         int colorIndex,endOfBitmap = FALSE;
         uint numAbsolutePixels,numFillerPixels;
         int pixelCount = 0;
         i = (pbmpInfo3->cbImage-2)/2;

         pimageData = combinedImageData;
         while ( i<pbmpInfo3->cbImage && !endOfBitmap) {
            if ( pimageData[i++] == 0 ) {
               switch ( pimageData[i++] ) {
                  case 0 :
                     break;
                  case 1 :
                     endOfBitmap = TRUE;
                     break;
                  case 2 :
                     i++;
                     i++;
                     break;
                  default :
                     numAbsolutePixels = pimageData[i-1];
                     pixelCount += numAbsolutePixels;
                     for ( j=0; j<numAbsolutePixels &&
                           j+i<pbmpInfo3->cbImage; j++ ) {
                        colorIndex = pimageData[i];
                        if ( colorIndex >= 0 && colorIndex < (int)numColors &&
                             (pbmpInfo3->argbColor[colorIndex].bRed != 
                              transparent.bRed ||
                              pbmpInfo3->argbColor[colorIndex].bGreen != 
                              transparent.bGreen ||
                              pbmpInfo3->argbColor[colorIndex].bBlue != 
                              transparent.bBlue) ) {
                           pimageData[i++] = (char)blackIndex;
                           }
                        else {
                           pimageData[i++] = (char)whiteIndex;
                           }
                        }
                     numFillerPixels = (2 - (i%2))%2;
                     for ( j=0; j<numFillerPixels; j++ ) {
                        i++;
                        }
                     break;
                  }
               }
            else {
               pixelCount += pimageData[i-1];
               colorIndex = pimageData[i];
               if ( colorIndex >= 0 && colorIndex < (int)numColors &&
                    (pbmpInfo3->argbColor[colorIndex].bRed != 
                     transparent.bRed ||
                     pbmpInfo3->argbColor[colorIndex].bGreen != 
                     transparent.bGreen ||
                     pbmpInfo3->argbColor[colorIndex].bBlue != 
                     transparent.bBlue) ) {
                  pimageData[i++] = (char)blackIndex;
                  }
               else {
                  pimageData[i++] = (char)whiteIndex;
                  }
               }
	    }

         /* ******************************************************** */
         /* ***** Modify palette to provide transparency color ***** */
         /* ***** Transparent pixels will be black             ***** */
         /* ******************************************************** */
         if ( pbmpInfo3->cBitCount != 24 ) {
            for ( i=0; i<numColors; i++ ) {
               if (pbmpInfo3->argbColor[i].bRed==transparent.bRed &&
                   pbmpInfo3->argbColor[i].bGreen==transparent.bGreen&&
                   pbmpInfo3->argbColor[i].bBlue==transparent.bBlue ) {
                  pbmpInfo3->argbColor[i].bRed = 0;
                  pbmpInfo3->argbColor[i].bGreen = 0;
                  pbmpInfo3->argbColor[i].bBlue = 0;
                  }
               }
	    }
         else if ( pbmpInfo3->ulCompression == 0 ) {
            for ( i=0; i<numColors; i++ ) {
               if (rgbColorData[i].bRed==transparent.bRed &&
                   rgbColorData[i].bGreen==transparent.bGreen&&
                   rgbColorData[i].bBlue==transparent.bBlue ) {
                  rgbColorData[i].bRed = 0;
                  rgbColorData[i].bGreen = 0;
                  rgbColorData[i].bBlue = 0;
                  }
               }
	    }
         /* ***************************************************** */

         }


      /* ***************************************************** */
      /* ***** Modify image data to provide transparency ***** */
      /* ***** Now we do uncompressed 4 bit bitmaps      ***** */
      /* ***************************************************** */
      if ( pbmpInfo3->cBitCount==4 && pbmpInfo3->ulCompression == 0 ) {

         pimageData = combinedImageData;
         for ( i=imageBytes; i<2*imageBytes; i++ ) {

	    int hiColorIndex = (pimageData[i] & 0xF0) >> 4;
	    int hiMaskIndex;
            if ( hiColorIndex >= 0 && hiColorIndex < (int)numColors && 
                (pbmpInfo3->argbColor[hiColorIndex].bRed != 
                 transparent.bRed ||
                 pbmpInfo3->argbColor[hiColorIndex].bGreen != 
                 transparent.bGreen ||
                 pbmpInfo3->argbColor[hiColorIndex].bBlue != 
                 transparent.bBlue) ) {
               hiMaskIndex = blackIndex;
               }
            else {
               hiMaskIndex = whiteIndex;
               hiColorIndex = blackIndex;
               }

            int loColorIndex = pimageData[i] & 0x0F;
            int loMaskIndex;
            if ( hiColorIndex >= 0 && hiColorIndex < (int)numColors && 
                (pbmpInfo3->argbColor[loColorIndex].bRed != 
                 transparent.bRed ||
                 pbmpInfo3->argbColor[loColorIndex].bGreen != 
                 transparent.bGreen ||
                 pbmpInfo3->argbColor[loColorIndex].bBlue != 
                 transparent.bBlue) ) {
               loMaskIndex = blackIndex;
               }
            else {
               loMaskIndex = whiteIndex;
               loColorIndex = blackIndex;
               }

            pimageData[i] = (char)((hiMaskIndex<<4) + loMaskIndex);
            pimageData[i-imageBytes] = (char)((hiColorIndex<<4)+loColorIndex);
	    }

         }
      /* ***************************************************** */


      /* ***************************************************** */
      /* ***** Modify image data to provide transparency ***** */
      /* ***** Now we do uncompressed 8 bit bitmaps      ***** */
      /* ***************************************************** */
      if ( pbmpInfo3->cBitCount==8 && pbmpInfo3->ulCompression == 0 ) {

         pimageData = combinedImageData;
         for ( i=imageBytes; i<2*imageBytes; i++ ) {

	    int colorIndex = pimageData[i];
	    int maskIndex;
            if ( colorIndex >= 0 && colorIndex < (int)numColors &&
                (pbmpInfo3->argbColor[colorIndex].bRed != 
                 transparent.bRed ||
                 pbmpInfo3->argbColor[colorIndex].bGreen != 
                 transparent.bGreen ||
                 pbmpInfo3->argbColor[colorIndex].bBlue != 
                 transparent.bBlue) ) {
               maskIndex = blackIndex;
               }
            else {
               maskIndex = whiteIndex;
               colorIndex = blackIndex;
               }

            pimageData[i] = (char)maskIndex;
            pimageData[i-imageBytes] = (char)colorIndex;
	    }

         }
      /* ***************************************************** */


      }
   /* ********************************************************************** */

   if ( maskFlags == S_BITMAP_MASK_TRANSPARENT ||
        maskFlags == S_BITMAP_MASK_TRANSLUCENT )
      pimageData = combinedImageData;
   else
      pimageData = rawImageData;   

   cySrc = pbmpInfo3->cy;

   if ( maskFlags == S_BITMAP_MASK_TRANSPARENT ||
        maskFlags == S_BITMAP_MASK_TRANSLUCENT ) {
      pimageData = combinedImageData;
      if ( rawImageData )
         S_free(rawImageData);
      }
   else {
      pimageData = rawImageData;   
      if ( combinedImageData )
         S_free(combinedImageData);
      }

   /* ********************************************************************** */
   /* *********************** Handle the gamma factor ********************** */
   /* ********************************************************************** */
   if ( pbmpInfo3->cBitCount != 24 )
      for ( i=0; i<numColors; i++ ) {
         uchar r = pbmpInfo3->argbColor[i].bRed;
         uchar g = pbmpInfo3->argbColor[i].bGreen;
         uchar b = pbmpInfo3->argbColor[i].bBlue;
         if ( gamma > 0 && (r < 255 || g < 255 || b < 255) ) {
            pbmpInfo3->argbColor[i].bRed = (char)(r*((double)gamma)/100.0);
            pbmpInfo3->argbColor[i].bGreen = (char)(g*((double)gamma)/100.0);
            pbmpInfo3->argbColor[i].bBlue = (char)(b*((double)gamma)/100.0);
            }
         }
   /* ********************************************************************** */

   /* ***************************************************************** */
   /* *** For 8 bit display, remap the palette to be indices in the *** */
   /* *** Logical color table.                                      *** */
   /* ***************************************************************** */
   /*
   RgbStruct *p1 = _root->palette->entries;
   Rgb2Struct *p2 = bmpData->ppalData;
   if ( _root->bitsPerPixel == 8 && pbmpInfo3->cBitCount == 8 ) {
      for ( j=0; j<256; j++ ) {
         ulong minRSqr = 0xFFFFFFFF;
         for ( int k=0; k<_root->palette->numEntries; k++ ) {
            if ( minRSqr > 0 ) {
               ulong rSqr =
                  (p1[k].bRed  -p2[j].bRed  )*(p1[k].bRed  -p2[j].bRed  )+
                  (p1[k].bGreen-p2[j].bGreen)*(p1[k].bGreen-p2[j].bGreen)+
                  (p1[k].bBlue -p2[j].bBlue )*(p1[k].bBlue -p2[j].bBlue );
               if ( rSqr < minRSqr ) {
                  minRSqr = rSqr;
                  ((ushort *)&ppalData[0])[j] = k;
                  }
               }
            }
         }
      }
      */

   return 0;
   }
/* *********************** */


int
S_Bitmap::Blit( S_Canvas *canvas,int frame,
                int x1targ, int y1targ,
                ulong blitFlags ) {
   Blit(canvas,frame,x1targ,y1targ,x1targ+cx,y1targ+cy,0,0,blitFlags);
   return 0;
   }


int
S_Bitmap::Blit( S_Canvas *canvas,int frame,
                int x1targ, int y1targ, int x2targ, int y2targ,
                int x1src,  int y1src,  ulong blitFlags ) {


   if ( frame >= numFrames || frame < 0 )
      frame = 0;

   if ( x2targ == 0 )
      x2targ = x1targ + cx;
   if ( y2targ == 0 )
      y2targ = y1targ + cy;

   if ( blitFlags == S_BITMAP_BLIT_DEFAULT )
     blitFlags = defaultBlitFlags;

   switch (blitFlags) {

      case S_BITMAP_BLIT_OPAQUE :
         SystemBlit(canvas,x1targ,y1targ,x2targ,y2targ,
                    x1src,y1src+frame*cy,ROP_SRCCOPY);
         break;

      case S_BITMAP_BLIT_XOR :
         SystemBlit(canvas,x1targ,y1targ,x2targ,y2targ,
                    x1src,y1src+frame*cy,ROP_SRCINVERT);
         break;

      case S_BITMAP_BLIT_AND :
         SystemBlit(canvas,x1targ,y1targ,x2targ,y2targ,
                    x1src,y1src+frame*cy,ROP_SRCAND);
         break;

      case S_BITMAP_BLIT_MASKONLY :
         SystemBlit(canvas,x1targ,y1targ,x2targ,y2targ,
                    x1src,y1src+cy*(numFrames+frame),ROP_SRCAND);
         break;

      case S_BITMAP_BLIT_MASKONLYNOT :
         SystemBlit(canvas,x1targ,y1targ,x2targ,y2targ,
                    x1src,y1src+cy*(numFrames+frame),ROP_NOTSRCAND);
         break;

      case S_BITMAP_BLIT_USEMASK :
         SystemBlit(canvas,x1targ,y1targ,x2targ,y2targ,
                    x1src,y1src+cy*(numFrames+frame),ROP_SRCAND);
         SystemBlit(canvas,x1targ,y1targ,x2targ,y2targ,
                    x1src,y1src+frame*cy,ROP_SRCINVERT);
         break;

      }

   return(0);   }



int
S_Bitmap::StretchBlit( S_Canvas *canvas,int frame,
                       int x1targ, int y1targ, int x2targ, int y2targ,
                       ulong blitFlags ) {
   StretchBlit(canvas,frame,x1targ,y1targ,x2targ,y2targ,
               0,0,cx,cy,blitFlags);
   return 0;
   }


int
S_Bitmap::StretchBlit( S_Canvas *canvas,int frame,
                       int x1targ, int y1targ, int x2targ, int y2targ,
                       int x1src,  int y1src,  int x2src,  int y2src,
                       ulong blitFlags ) {

   if ( frame >= numFrames || frame < 0 )
      frame = 0;

   if ( x2src == 0 )
      x2src = x1src + cx;
   if ( y2src == 0 )
      y2src = y1src + cy;

   if ( blitFlags == S_BITMAP_BLIT_DEFAULT )
     blitFlags = defaultBlitFlags;

   switch (blitFlags) {

      case S_BITMAP_BLIT_OPAQUE :
         SystemBlit(canvas,x1targ,y1targ,x2targ,y2targ,
                    x1src,y1src+frame*cy,
                    x2src,y2src+frame*cy,ROP_SRCCOPY);
         break;

      case S_BITMAP_BLIT_XOR :
         SystemBlit(canvas,x1targ,y1targ,x2targ,y2targ,
                    x1src,y1src+frame*cy,
                    x2src,y2src+frame*cy,ROP_SRCINVERT);
         break;

      case S_BITMAP_BLIT_AND :
         SystemBlit(canvas,x1targ,y1targ,x2targ,y2targ,
                    x1src,y1src+frame*cy,
                    x2src,y2src+frame*cy,ROP_SRCAND);
         break;

      case S_BITMAP_BLIT_MASKONLY :
         SystemBlit(canvas,x1targ,y1targ,x2targ,y2targ,
                    x1src,y1src+cy*(numFrames+frame),
                    x2src,y2src+cy*(numFrames+frame),ROP_SRCAND);
         break;

      case S_BITMAP_BLIT_MASKONLYNOT :
         SystemBlit(canvas,x1targ,y1targ,x2targ,y2targ,
                    x1src,y1src+cy*(numFrames+frame),
                    x2src,y2src+cy*(numFrames+frame),ROP_NOTSRCAND);
         break;

      case S_BITMAP_BLIT_USEMASK :
         SystemBlit(canvas,x1targ,y1targ,x2targ,y2targ,
                    x1src,y1src+cy*(numFrames+frame),
                    x2src,y2src+cy*(numFrames+frame),ROP_SRCAND);
         SystemBlit(canvas,x1targ,y1targ,x2targ,y2targ,
                    x1src,y1src+frame*cy,
                    x2src,y2src+frame*cy,
                    ROP_SRCINVERT);
         break;

      }

   return(0);
   }


int
S_Bitmap::Destroy() {
   Init();
   return 0;
   }

S_Bitmap::~S_Bitmap() {
   Destroy();
   }
/* *********************** */


int
S_Bitmap::SystemBlit(S_Canvas *target,
                     int x1targ,int y1targ,int x2targ,int y2targ,
                     int x1src,int y1src,ulong ropFlags ) {

   if ( !target || (target && !target->hdc) || !pimageData || !pbmpInfo3 ) {
      rc = S_BITMAP_NODATA;
      return rc;
      }

#  ifdef OS2
      POINTL aptl[4] = {x1targ,target->cyCanvas-y2targ,
                        x2targ-1,target->cyCanvas-y1targ-1,
                        x1src,y1src,
                        x1src+(x2targ-x1targ),y1src+(y2targ-y1targ)};
      GpiDrawBits(target->hdc,pimageData,(PBITMAPINFO2)pbmpInfo3,
                  4,aptl,ropFlags,BBO_IGNORE);
#  endif
#  ifdef WIN32
      /*
      SetDIBitsToDevice(target->hdc,x1targ,y1targ,x2targ-x1targ,y2targ-y1targ,
                    x1src,y1src,y1src,cy,pimageData,
                    (BITMAPINFO *)pbmpInfo3,DIB_RGB_COLORS);
		    */
      StretchDIBits(target->hdc,x1targ,y1targ,x2targ-x1targ,y2targ-y1targ,
                    x1src,y1src,x2targ-x1targ,y2targ-y1targ,pimageData,
                    (BITMAPINFO *)pbmpInfo3,DIB_RGB_COLORS,ropFlags);
#  endif
   return 0;
   }


int
S_Bitmap::SystemBlit(S_Canvas *target,
                     int x1targ,int y1targ,int x2targ,int y2targ,
                     int x1src,int y1src,int x2src,int y2src,
                     ulong ropFlags ) {

   if ( !target || (target && !target->hdc) || !pimageData || !pbmpInfo3 ) {
      rc = S_BITMAP_NODATA;
      return rc;
      }

#  ifdef OS2
      POINTL aptl[4] = {x1targ  ,target->cyCanvas-y2targ,
                        x2targ-1,target->cyCanvas-y1targ-1,
                        x1src,y1src,x2src,y2src+1};
      GpiDrawBits(target->hdc,pimageData,(PBITMAPINFO2)pbmpInfo3,
                  4,aptl,ropFlags,BBO_IGNORE);
#  endif
#  ifdef WIN32
      StretchDIBits(target->hdc,x1targ,y1targ,x2targ-x1targ,y2targ-y1targ,
                    x1src,y1src,x2src-x1src,y2src-y1src,pimageData,
                    (BITMAPINFO *)pbmpInfo3,DIB_RGB_COLORS,ropFlags);
#  endif
   return 0;
   }

