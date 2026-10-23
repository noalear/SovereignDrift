/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
#include "S_OS.h"
#include "S_ZArchive.h"
#include "S_SystemBitmap.h"
#include "S_Canvas.h"
#include "S_Root.h"

int _DDPrintErrorMsg(int);


int
S_SystemBitmap::Init() {
   rc = S_SYSTEMBITMAP_NOTLOADED;
   oldPalette = NULL;
   lpBuffer = NULL;
   hbm = NULL;
   ohbm = NULL;
   hdc = NULL;
   return 0;
   }

S_SystemBitmap::S_SystemBitmap() {
   Init();
   }

S_SystemBitmap::S_SystemBitmap(uchar *ifileBuffer,ulong ifileSize,
                               slong imaskFlags,
                               slong idefaultBlitFlags,slong inumFrames,
                               slong igamma,ulong itColor ) {
   Init();
   Create(ifileBuffer,ifileSize,imaskFlags,idefaultBlitFlags,
          inumFrames,igamma,itColor);
   }

S_SystemBitmap::S_SystemBitmap(char *ifilename,slong imaskFlags,
                               slong idefaultBlitFlags,slong inumFrames,
                               slong igamma,ulong itColor ) {
   Init();
   Create(ifilename,imaskFlags,idefaultBlitFlags,inumFrames,igamma,itColor);
   }


int
S_SystemBitmap::ZCreate(char *ifilename,slong imaskFlags,
			slong idefaultBlitFlags,slong inumFrames,
			slong igamma,ulong itColor ) {
  Destroy();
  S_ZArchive zArc;
  S_ZFile *zFile;
  if ( _root )
    {
      zArc.Open(_root->exeFilename);
      do
	{
	  zFile = zArc.ReadNextFile();
	  if ( zFile )
	    {
	      if ( stricmp(zFile->name,ifilename) == 0 )
		{
		  zFile->UnCompress();
		  Create(zFile->buffer,zFile->hdr.fileSize,imaskFlags,
			 idefaultBlitFlags,inumFrames,igamma,itColor);
		}
	      delete zFile;
	    }
	} while (zFile);
    }

  return 0;
}


int
S_SystemBitmap::Create(char *ifilename,slong imaskFlags,
                       slong idefaultBlitFlags,slong inumFrames,
                       slong igamma,ulong itColor ) {

   Destroy();
   S_BitmapData bmpData(ifilename);
   secretCode = bmpData.secretCode;
   checkSum = bmpData.checkSum;
   Create(&bmpData,imaskFlags,idefaultBlitFlags,inumFrames,igamma,itColor);
   return rc;
   }


int
S_SystemBitmap::Create(uchar *ifileBuffer,ulong ifileSize,slong imaskFlags,
                       slong idefaultBlitFlags,slong inumFrames,
                       slong igamma,ulong itColor ) {
   Destroy();
   S_BitmapData bmpData(ifileBuffer,ifileSize);
   secretCode = bmpData.secretCode;
   checkSum = bmpData.checkSum;
   Create(&bmpData,imaskFlags,idefaultBlitFlags,inumFrames,igamma,itColor);
   return rc;
   }

S_SystemBitmap::S_SystemBitmap(S_BitmapData *bmpData,slong imaskFlags,
                               slong idefaultBlitFlags,
                               slong inumFrames,slong igamma,ulong itColor ) {
   Init();
   Create(bmpData,imaskFlags,idefaultBlitFlags,inumFrames,igamma,itColor);
   }


int
S_SystemBitmap::Create(S_BitmapData *bmpData,slong imaskFlags,
                       slong idefaultBlitFlags,
                       slong inumFrames,slong igamma,ulong itColor ) {
   Destroy();
   S_Bitmap::Create(bmpData,imaskFlags,idefaultBlitFlags,inumFrames,
                    igamma,itColor);

   rc = bmpData->rc;
   if ( rc < 0 )
      return rc;

#  ifdef OS2
      ulong hab,hps;
      DEVOPENSTRUC dop = {0L,"DISPLAY",NULL,0,0,0,0,0,0};
      SIZEL sizl = {0,0};

      hab = WinInitialize (0) ;
      hdc = DevOpenDC(hab,OD_MEMORY,"*",5L,(PDEVOPENDATA)&dop,NULLHANDLE);
      hps = GpiCreatePS(hab,hdc,&sizl,PU_PELS | GPIT_MICRO | GPIA_ASSOC);
      hbm = GpiCreateBitmap( hps,(PBITMAPINFOHEADER2)pbmpInfo3,
                             CBM_INIT,pimageData,
                             (PBITMAPINFO2)pbmpInfo3);
      DevCloseDC (hdc);
      WinTerminate (hab);

      hdc = hps;
      ohbm = GpiSetBitmap(hdc,hbm);

      if ( !hbm )
         rc = S_SYSTEMBITMAP_CREATEFAILED;
#  endif
#  ifdef WIN32
      hdc = CreateDC("DISPLAY",NULL,NULL,NULL);
      SetPalette(NULL);
      hbm = CreateDIBitmap(hdc,(BITMAPINFOHEADER *)pbmpInfo3,CBM_INIT,
                           pimageData,(BITMAPINFO*)pbmpInfo3,DIB_RGB_COLORS);
      DeleteDC(hdc);
      hdc = CreateCompatibleDC(NULL);
      ohbm = (HBITMAP) SelectObject(hdc,(HGDIOBJ)hbm);

      if ( !hbm )
         rc = S_SYSTEMBITMAP_CREATEFAILED;

      if ( _root && _root->lpDD ) {
         DDSURFACEDESC ddsd;
         ddsd.dwSize = sizeof(ddsd);
         ddsd.dwHeight = pbmpInfo3->cy;
         ddsd.dwWidth = pbmpInfo3->cx;
         ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
         ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN|DDSCAPS_VIDEOMEMORY; 
         _root->lpDD->CreateSurface(&ddsd,&lpBuffer,NULL);

         RestoreBuffer();
         }
#  endif

   S_BitmapData::Destroy();

   return rc;
   }


int
S_SystemBitmap::Destroy() {
#     ifdef OS2
      if ( hdc ) {
         GpiSetBitmap(hdc,ohbm);
         GpiDestroyPS (hdc);
         }
      if ( hbm )
         GpiDeleteBitmap( hbm );
#     endif
#     ifdef WIN32
      if ( hdc ) {
         SelectObject(hdc,(HGDIOBJ)ohbm);
         ResetPalette();
         DeleteDC(hdc);
         }
      if ( hbm )
         DeleteObject( hbm );
      if ( lpBuffer )
         lpBuffer->Release();
#     endif
   Init();
   return 0;
   }


S_SystemBitmap::~S_SystemBitmap() {
   Destroy();
   }


int
S_SystemBitmap::RestoreBuffer() {
#  ifdef WIN32
   if ( lpBuffer ) {

      lpBuffer->Restore();
      lpBuffer->GetDC(&hdc);
      SetPalette(NULL);
      HDC bitmapDC = CreateCompatibleDC(hdc);

      HBITMAP oldBitmap = (HBITMAP)SelectObject(bitmapDC,(HGDIOBJ)hbm);
      BitBlt(hdc,0,0,cx,cy,bitmapDC,0,0,ROP_SRCCOPY);
      SelectObject(bitmapDC,(HGDIOBJ)oldBitmap);
      DeleteDC(bitmapDC);
      lpBuffer->ReleaseDC(hdc);

      DDCOLORKEY ddColorKey;
      ddColorKey.dwColorSpaceLowValue = RGB_INVERT(tColor);
      ddColorKey.dwColorSpaceHighValue = RGB_INVERT(tColor);
      lpBuffer->SetColorKey(DDCKEY_SRCBLT,&ddColorKey);
      }
#  endif
   return 0;
   }


int
S_SystemBitmap::Blit( S_Canvas *canvas,int frame,
                      int x1targ, int y1targ,
                      ulong blitFlags ) {
   Blit(canvas,frame,x1targ,y1targ,x1targ+cx,y1targ+cy,0,0,blitFlags);
   return 0;
   }


int
S_SystemBitmap::Blit( S_Canvas *canvas,int frame,
                      int x1targ, int y1targ, int x2targ, int y2targ,
                      int x1src,  int y1src,  ulong blitFlags ) {

   if ( !canvas )
      return 0;

   if ( frame >= numFrames || frame < 0 )
      frame = 0;

   if ( x2targ == 0 )
      x2targ = x1targ + cx;
   if ( y2targ == 0 )
      y2targ = y1targ + cy;

   if ( blitFlags == S_BITMAP_BLIT_DEFAULT )
     blitFlags = defaultBlitFlags;

   if ( lpBuffer && blitFlags == S_BITMAP_BLIT_USEMASK &&
        (maskFlags == S_BITMAP_MASK_TRANSPARENT ||
         maskFlags == S_BITMAP_MASK_TRANSLUCENT) )
      blitFlags = S_BITMAP_BLIT_TRANSPARENT;

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

      case S_BITMAP_BLIT_TRANSPARENT :
         SystemBlit(canvas,x1targ,y1targ,x2targ,y2targ,
                    x1src,y1src+frame*cy,ROP_TRANSPARENT);
         break;

      }

   return(0);
   }


int
S_SystemBitmap::StretchBlit( S_Canvas *canvas,int frame,
                             int x1targ, int y1targ, int x2targ, int y2targ,
                             ulong blitFlags ) {
   StretchBlit(canvas,frame,x1targ,y1targ,x2targ,y2targ,
               0,0,cx,cy,blitFlags);
   return 0;
   }


int
S_SystemBitmap::StretchBlit( S_Canvas *canvas,int frame,
                             int x1targ, int y1targ, int x2targ, int y2targ,
                             int x1src,  int y1src,  int x2src,  int y2src,
                             ulong blitFlags ) {

   if ( !canvas )
      return 0;

   if ( frame >= numFrames || frame < 0 )
      frame = 0;

   if ( x2src == 0 )
      x2src = x1src + cx;
   if ( y2src == 0 )
      y2src = y1src + cy;

   if ( blitFlags == S_BITMAP_BLIT_DEFAULT )
      blitFlags = defaultBlitFlags;

   if ( lpBuffer && blitFlags == S_BITMAP_BLIT_USEMASK &&
        (maskFlags == S_BITMAP_MASK_TRANSPARENT ||
         maskFlags == S_BITMAP_MASK_TRANSLUCENT) )
      blitFlags = S_BITMAP_BLIT_TRANSPARENT;

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

      case S_BITMAP_BLIT_TRANSPARENT :
         SystemBlit(canvas,x1targ,y1targ,x2targ,y2targ,
                    x1src,y1src+frame*cy,
                    x2src,y2src+frame*cy,
                    ROP_TRANSPARENT);
         break;

      }

   return(0);
   }



int
S_SystemBitmap::SystemBlit(S_Canvas *target,
                     int x1targ,int y1targ,int x2targ,int y2targ,
                     int x1src,int y1src,ulong ropFlags ) {

   if ( !hbm || !target || (target && !target->hdc && !target->lpBuffer) ) {
      return rc;
      }

#  ifdef OS2
      POINTL aptl[3] = {x1targ,target->cyCanvas-y2targ,
                        x2targ/*-1*/,target->cyCanvas-y1targ/*-1*/,
                        x1src,y1src};
      GpiBitBlt(target->hdc,hdc,3,aptl,ropFlags,BBO_IGNORE);
#  endif
#  ifdef WIN32
      if ( target->lpBuffer && lpBuffer ) {
         RECT rclDest = {target->xCanvas+x1targ,
                         target->yCanvas+y1targ,
                         target->xCanvas+x2targ,
                         target->yCanvas+y2targ};
         RECT rclSrc  = {x1src,cySrc-y1src-(y2targ-y1targ),
                         x1src+(x2targ-x1targ),cySrc-y1src};

         if ( rclDest.right > target->cxCanvas ) {
            rclSrc.right -= rclDest.right - target->cxCanvas;
            rclDest.right -= rclDest.right - target->cxCanvas;
            }
         if ( rclDest.bottom > target->cyCanvas ) {
            rclSrc.bottom -= rclDest.bottom - target->cyCanvas;
            rclDest.bottom -= rclDest.bottom - target->cyCanvas;
            }
         if ( rclDest.top < 0 ) {
            rclSrc.top -= rclDest.top;
            rclDest.top -= rclDest.top;
            }
         if ( rclDest.left < 0 ) {
            rclSrc.left -= rclDest.left;
            rclDest.left -= rclDest.left;
            }

         int rc = 0;
         DDBLTFX ddBltFx;
         ddBltFx.dwSize = sizeof(DDBLTFX);
         if ( ropFlags == ROP_TRANSPARENT ) {
            rc = target->lpBuffer->Blt(&rclDest,lpBuffer,&rclSrc,
                                       DDBLT_KEYSRC|DDBLT_WAIT,&ddBltFx);
	    }
         else {
            ddBltFx.dwROP = ROP_SRCCOPY;
            rc = target->lpBuffer->Blt(&rclDest,lpBuffer,&rclSrc,
                                       DDBLT_ROP|DDBLT_WAIT,&ddBltFx);
	    }
         _DDPrintErrorMsg(rc);
         }
      else {
         BitBlt(target->hdc,x1targ,y1targ,x2targ-x1targ,y2targ-y1targ,
                hdc,x1src,cySrc-y1src-(y2targ-y1targ),ropFlags);
         }
#  endif
   return 0;
   }


int
S_SystemBitmap::SystemBlit(S_Canvas *target,
                     int x1targ,int y1targ,int x2targ,int y2targ,
                     int x1src,int y1src,int x2src,int y2src,
                     ulong ropFlags ) {
#  ifdef OS2
      POINTL aptl[4] = {x1targ  ,target->cyCanvas-y2targ,
                        x2targ/*-1*/,target->cyCanvas-y1targ/*-1*/,
                        x1src,y1src,x2src,y2src/*+1*/};
      GpiBitBlt(target->hdc,hdc,4,aptl,ropFlags,BBO_IGNORE);
#  endif
#  ifdef WIN32
      if ( target->lpBuffer && lpBuffer) {
         RECT rclDest = {target->xCanvas+x1targ,
                         target->yCanvas+y1targ,
                         target->xCanvas+x2targ,
                         target->yCanvas+y2targ};
         RECT rclSrc  = {x1src,cySrc-y2src,x2src,cySrc-y1src};
         if ( rclDest.right > target->cxCanvas ) {
            rclSrc.right -= rclDest.right - target->cxCanvas;
            rclDest.right -= rclDest.right - target->cxCanvas;
            }
         if ( rclDest.bottom > target->cyCanvas ) {
            rclSrc.bottom -= rclDest.bottom - target->cyCanvas;
            rclDest.bottom -= rclDest.bottom - target->cyCanvas;
            }
         if ( rclDest.top < 0 ) {
            rclSrc.top -= rclDest.top;
            rclDest.top -= rclDest.top;
            }
         if ( rclDest.left < 0 ) {
            rclSrc.left -= rclDest.left;
            rclDest.left -= rclDest.left;
            }

         int rc = 0;
         DDBLTFX ddBltFx;
         ddBltFx.dwSize = sizeof(DDBLTFX);
         if ( ropFlags == ROP_TRANSPARENT ) {
            rc = target->lpBuffer->Blt(&rclDest,lpBuffer,&rclSrc,
                                       DDBLT_KEYSRC|DDBLT_WAIT,&ddBltFx);
	    }
         else {
            ddBltFx.dwROP = ROP_SRCCOPY;
            rc = target->lpBuffer->Blt(&rclDest,lpBuffer,&rclSrc,
                                       DDBLT_ROP|DDBLT_WAIT,&ddBltFx);
            }
         _DDPrintErrorMsg(rc);
         }
      else {
         StretchBlt(target->hdc,x1targ,y1targ,x2targ-x1targ,y2targ-y1targ,
                    hdc,x1src,cySrc-y2src,
                    x2src-x1src,y2src-y1src,ropFlags);
         }
#  endif
   return 0;
   }


int
S_SystemBitmap::SystemBlit(S_SystemBitmap *target,
                     int x1targ,int y1targ,int x2targ,int y2targ,
                     int x1src,int y1src,ulong ropFlags ) {

   if ( !hbm || !target || (target && !target->hdc && !target->lpBuffer) ) {
      return rc;
      }

#  ifdef OS2
      POINTL aptl[3] = {x1targ,target->cySrc-y2targ,
                        x2targ,target->cySrc-y1targ,
                        x1src,y1src};
      GpiBitBlt(target->hdc,hdc,3,aptl,ropFlags,BBO_IGNORE);
#  endif
#  ifdef WIN32
      if ( target->lpBuffer && lpBuffer ) {
         RECT rclDest = {x1targ,
                         y1targ,
                         x2targ,
                         y2targ};
         RECT rclSrc  = {x1src,cySrc-y1src-(y2targ-y1targ),
                         x1src+(x2targ-x1targ),cySrc-y1src};

         if ( rclDest.right > (int)target->pbmpInfo3->cx ) {
            rclSrc.right -= rclDest.right - target->pbmpInfo3->cx;
            rclDest.right -= rclDest.right - target->pbmpInfo3->cx;
            }
         if ( rclDest.bottom > (int)target->cySrc ) {
            rclSrc.bottom -= rclDest.bottom - target->cySrc;
            rclDest.bottom -= rclDest.bottom - target->cySrc;
            }
         if ( rclDest.top < 0 ) {
            rclSrc.top -= rclDest.top;
            rclDest.top -= rclDest.top;
            }
         if ( rclDest.left < 0 ) {
            rclSrc.left -= rclDest.left;
            rclDest.left -= rclDest.left;
            }

         DDBLTFX ddBltFx;
         ddBltFx.dwSize = sizeof(DDBLTFX);
         if ( ropFlags == ROP_TRANSPARENT ) {
            target->lpBuffer->Blt(&rclDest,lpBuffer,&rclSrc,
                                  DDBLT_KEYSRC|DDBLT_WAIT,&ddBltFx);
	    }
         else {
            ddBltFx.dwROP = ROP_SRCCOPY;
            target->lpBuffer->Blt(&rclDest,lpBuffer,&rclSrc,
                                  DDBLT_ROP|DDBLT_WAIT,&ddBltFx);
	    }
         }
      else {
         BitBlt(target->hdc,x1targ,y1targ,x2targ-x1targ,y2targ-y1targ,
                hdc,x1src,cySrc-y1src-(y2targ-y1targ),ropFlags);
         }
#  endif
   return 0;
   }


int
S_SystemBitmap::SystemBlit(S_SystemBitmap *target,
                     int x1targ,int y1targ,int x2targ,int y2targ,
                     int x1src,int y1src,int x2src,int y2src,
                     ulong ropFlags ) {
#  ifdef OS2
      POINTL aptl[4] = {x1targ,target->cySrc-y2targ,
                        x2targ,target->cySrc-y1targ,
                        x1src,y1src,x2src,y2src};
      GpiBitBlt(target->hdc,hdc,4,aptl,ropFlags,BBO_IGNORE);
#  endif
#  ifdef WIN32
      if ( target->lpBuffer && lpBuffer) {
         RECT rclDest = {x1targ,
                         y1targ,
                         x2targ,
                         y2targ};
         RECT rclSrc  = {x1src,cySrc-y2src,x2src,cySrc-y1src};
         if ( rclDest.right > (int)target->pbmpInfo3->cx ) {
            rclSrc.right -= rclDest.right - target->pbmpInfo3->cx;
            rclDest.right -= rclDest.right - target->pbmpInfo3->cx;
            }
         if ( rclDest.bottom > (int)target->cySrc ) {
            rclSrc.bottom -= rclDest.bottom - target->cySrc;
            rclDest.bottom -= rclDest.bottom - target->cySrc;
            }
         if ( rclDest.top < 0 ) {
            rclSrc.top -= rclDest.top;
            rclDest.top -= rclDest.top;
            }
         if ( rclDest.left < 0 ) {
            rclSrc.left -= rclDest.left;
            rclDest.left -= rclDest.left;
            }
         DDBLTFX ddBltFx;
         ddBltFx.dwSize = sizeof(DDBLTFX);
         if ( ropFlags == ROP_TRANSPARENT ) {
            target->lpBuffer->Blt(&rclDest,lpBuffer,&rclSrc,
                                  DDBLT_KEYSRC|DDBLT_WAIT,&ddBltFx);
	    }
         else {
            ddBltFx.dwROP = ROP_SRCCOPY;
            target->lpBuffer->Blt(&rclDest,lpBuffer,&rclSrc,
                                  DDBLT_ROP|DDBLT_WAIT,&ddBltFx);
            }
         }
      else {
         StretchBlt(target->hdc,x1targ,y1targ,x2targ-x1targ,y2targ-y1targ,
                    hdc,x1src,cySrc-y2src,
                    x2src-x1src,y2src-y1src,ropFlags);
         }
#  endif
   return 0;
   }

#ifdef OS2
HBITMAP 
_CreateOS2Bitmap(BitmapInfo3Struct *pbmpInfo3,char *pimageData) {
   ulong hab,hps,hdc;
   DEVOPENSTRUC dop = {0L,"DISPLAY",NULL,0,0,0,0,0,0};
   SIZEL sizl = {0,0};

   hab = WinInitialize (0) ;
   hdc = DevOpenDC(hab,OD_MEMORY,"*",5L,(PDEVOPENDATA)&dop,NULLHANDLE);
   hps = GpiCreatePS(hab,hdc,&sizl,PU_PELS | GPIT_MICRO | GPIA_ASSOC);
   HBITMAP hbm = GpiCreateBitmap( hps,(PBITMAPINFOHEADER2)pbmpInfo3,
                                  CBM_INIT,pimageData,
                                 (PBITMAPINFO2)pbmpInfo3);
   GpiDestroyPS (hps);
   DevCloseDC (hdc);
   WinTerminate (hab);
   return hbm;
   }
#endif

#ifdef WIN32
HBITMAP 
_CreateWIN32Bitmap(BitmapInfo3Struct *pbmpInfo3,char *pimageData) {
   HDC hdc = CreateDC("DISPLAY",NULL,NULL,NULL);
   HBITMAP hbm = CreateDIBitmap(hdc,(BITMAPINFOHEADER*)pbmpInfo3,CBM_INIT,
                                pimageData,(BITMAPINFO*)pbmpInfo3,
                                DIB_RGB_COLORS);
   DeleteDC(hdc);
   return hbm;
   }
#endif


void *
S_SystemBitmap::Lock() {
   void *ptr = NULL;
#  ifdef WIN32
   DDSURFACEDESC ddsd;
   ZeroMemory(&ddsd,sizeof(ddsd));
   ddsd.dwSize = sizeof(ddsd);
   if ( lpBuffer ) {
      int rc = lpBuffer->Lock(NULL,&ddsd,DDLOCK_WAIT,NULL);
      if ( rc == DDERR_SURFACELOST ) {
         rc = lpBuffer->Restore();
         _DDPrintErrorMsg(rc);
         rc = lpBuffer->Lock(NULL,&ddsd,DDLOCK_WAIT,NULL);
         }
      _DDPrintErrorMsg(rc);
      ptr = ddsd.lpSurface;
      }
#  endif
   return ptr;
   }

int
S_SystemBitmap::Unlock() {
#  ifdef WIN32
      lpBuffer->Unlock(NULL);
#  endif 
   return 0;
   }
