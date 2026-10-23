#include "S_OS.h"
#include "S_Canvas.h"
#include "S_SystemIcon.h"
#include "S_SystemBitmap.h"
#include "S_Bitmap.h"
#include "S_Root.h"
#include "S_ZArchive.h"


int
S_SystemIcon::Init() {
   hIcon = NULL;
   tColor = 0;
   maskFlags = S_BITMAP_MASK_TRANSPARENT;
   defaultBlitFlags = S_BITMAP_BLIT_USEMASK;
   numFrames = 1;
   gamma = 100;
   return 0;
   }

S_SystemIcon::S_SystemIcon() {
   Init();
   }

S_SystemIcon::S_SystemIcon(char *ifilename,ulong itColor ) {
   Init();
   Create(ifilename,itColor);
   }


int
S_SystemIcon::Create(char *ifilename,ulong itColor ) {
   Destroy();
   S_BitmapData bmpData(ifilename);
   if ( bmpData.rc == S_BITMAPDATA_OK )
      Create(&bmpData,itColor);
   return 0;
   }


int
S_SystemIcon::ZCreate(char *ifilename,ulong itColor ) {
   Destroy();

   S_ZArchive zArc;
   S_ZFile *zFile;
   zArc.Open(_root->exeFilename);
   do {
      zFile = zArc.ReadNextFile();
      if ( zFile ) {
         if ( stricmp(zFile->name,ifilename) == 0 ) {
            zFile->UnCompress();
            S_BitmapData bmpData;
            bmpData.Create(zFile->buffer,zFile->hdr.fileSize);
            if ( bmpData.rc == S_BITMAPDATA_OK )
               Create(&bmpData,itColor);
	    }
         delete zFile;
         }
      } while (zFile);

   return 0;
   }

S_SystemIcon::S_SystemIcon(S_BitmapData *bmpData,ulong itColor ) {
   Init();
   Create(bmpData,itColor);
   }


int
S_SystemIcon::Create(S_BitmapData *bmpData,ulong itColor ) {
   Destroy();

   if ( !bmpData && (bmpData && !bmpData->pbmpInfo3) )
      return S_SYSTEMICON_NODATA;

   if ( bmpData->pbmpInfo3->ulCompression != 0 ) {
      rc = S_SYSTEMICON_UNSUPPORTEDBMP;
      return rc;
      }

   S_Bitmap::Create(bmpData,maskFlags,defaultBlitFlags,numFrames,gamma,itColor);

   rc = bmpData->rc;
   if ( rc < 0 )
      return rc;

#  ifdef OS2
      ulong hab,hdc,hps,hbm;
      DEVOPENSTRUC dop = {0L,"DISPLAY",NULL,0,0,0,0,0,0};
      SIZEL sizl = {0,0};

      hab = WinInitialize (0) ;
      hdc = DevOpenDC(hab,OD_MEMORY,"*",5L,(PDEVOPENDATA)&dop,NULLHANDLE);
      hps = GpiCreatePS(hab,hdc,&sizl,PU_PELS | GPIT_MICRO | GPIA_ASSOC);
      hbm = GpiCreateBitmap( hps,(PBITMAPINFOHEADER2)pbmpInfo3,
                             CBM_INIT,pimageData,
                             (PBITMAPINFO2)pbmpInfo3);
      GpiDestroyPS (hps);
      DevCloseDC (hdc);
      WinTerminate (hab);

      if ( !hbm )
         rc = S_SYSTEMBITMAP_CREATEFAILED;

      hIcon = WinCreatePointer(HWND_DESKTOP,hbm,FALSE,0,0);
      if ( !hIcon )
         rc = S_SYSTEMICON_CREATEFAILED;

      GpiDeleteBitmap(hbm);
#  endif
#  ifdef WIN32

      int i,j;

      /* ****************************************** */
      /* *** Patch Win95 CreateIconIndirect bug *** */
      /* ****************************************** */
      if ( _OsType() == S_OS_WIN95 ) {
         int imageBits = imageBytes*8;
         int imagePixels = imageBits/pbmpInfo3->cBitCount;
         int monoBytes = imageBytes/pbmpInfo3->cBitCount;
         int monoBits = monoBytes*8;
         int monoPixels = imagePixels;
         char *monoMask = pimageData + imageBytes;
         for ( j=0; j<(int)pbmpInfo3->cy/4; j++ )
            for ( i=0; i<(int)pbmpInfo3->cx*pbmpInfo3->cBitCount/8; i++ ) {
               int dx = pbmpInfo3->cx*pbmpInfo3->cBitCount/8;
               int k = i + j*dx;
               int l = i + (pbmpInfo3->cy/2-j-1)*dx;
               char ch = pimageData[k];
               pimageData[k] = pimageData[l];
               pimageData[l] = ch;
               ch = monoMask[k];
               monoMask[k] = monoMask[l];
               monoMask[l] = ch;
	       }

         for ( int i=0; i<imageBits; i+=pbmpInfo3->cBitCount ) {
            ulong byte = i/8;
            ulong bit = i%8;
            ulong pixel = i/pbmpInfo3->cBitCount;
            ulong pixelVal;
            switch(pbmpInfo3->cBitCount) {
               case 1 :
                  pixelVal = (monoMask[byte] & (1<<bit))>>bit;
                  break;
               case 4 :
		  pixelVal = (monoMask[byte] & (0xF0>>bit));
                  break;
               case 8 :
                  pixelVal = monoMask[byte];
                  break;
               case 16 :
                  pixelVal = *((ushort *)&monoMask[byte]);
                  break;
               case 24 :
                  pixelVal =  monoMask[byte] + 
                             (monoMask[byte+1]<<8) + 
                             (monoMask[byte+2]<<16);
                  break;
               case 32 :
                  pixelVal = *((ulong *)&monoMask[byte]);
                  break;
	       }
            if ( pixelVal )
	      monoMask[pixel/8] |= 128>>(pixel%8);
            else
	      monoMask[pixel/8] &= ~(128>>(pixel%8));
	    }

         /*
         for ( i=0; i<32; i++ )
            for ( j=0; j<4; j++ )
               monoMask[j + i*4] = 0x00;
         for ( i=32; i<64; i++ )
            for ( j=0; j<4; j++ )
               monoMask[j + i*4] = 0x00;
	       */

         hIcon = CreateIcon(_root->hInst,pbmpInfo3->cx,pbmpInfo3->cy/2,
                            (BYTE)pbmpInfo3->cPlanes,
                            (BYTE)pbmpInfo3->cBitCount,
                            (const BYTE *)monoMask,
                            (const BYTE *)pimageData);
         }
      else {

         pbmpInfo3->cy /= 2;
         HDC hdc = CreateDC("DISPLAY",NULL,NULL,NULL);
         HBITMAP hbmColor = CreateDIBitmap(hdc,
                                          (BITMAPINFOHEADER*)pbmpInfo3,
                                           CBM_INIT,
                                           pimageData,
                                          (BITMAPINFO*)pbmpInfo3,
                                           DIB_RGB_COLORS);
         HBITMAP hbmMask = CreateDIBitmap(hdc,
                                          (BITMAPINFOHEADER *)pbmpInfo3,
                                          CBM_INIT,
                                          pimageData + imageBytes,
                                         (BITMAPINFO*)pbmpInfo3,
                                          DIB_RGB_COLORS);
         DeleteDC(hdc);

         if ( !hbmMask || !hbmColor )
            rc = S_SYSTEMBITMAP_CREATEFAILED;

         ICONINFO ii;
         ii.fIcon = TRUE;
         ii.xHotspot = 0;
         ii.yHotspot = 0;
         ii.hbmMask = hbmMask;
         ii.hbmColor = hbmColor;
         hIcon = CreateIconIndirect(&ii);

         DeleteObject(hbmMask);
         DeleteObject(hbmColor);
         }

      if ( !hIcon )
         rc = S_SYSTEMICON_CREATEFAILED;
#  endif

   S_BitmapData::Destroy();

   return 0;
   }


int
S_SystemIcon::Draw(S_Canvas *canvas,int x,int y,int cx,int cy) {
#  ifdef WIN32 
      if ( canvas && canvas->hdc && hIcon )
         if ( !cx || !cy )
            DrawIcon(canvas->hdc,x,y,hIcon);
         else
            DrawIconEx(canvas->hdc,x,y,hIcon,cx,cy,0,NULL,DI_NORMAL);
#  endif
#  ifdef OS2
      if ( canvas && canvas->hdc && hIcon )
         WinDrawPointer(canvas->hdc,x,canvas->cyCanvas-cy-y,hIcon,DP_NORMAL);
#  endif
   return 0;
   }


int
S_SystemIcon::Destroy() {
#     ifdef OS2
      if ( hIcon )
         WinDestroyPointer( hIcon );
#     endif
#     ifdef WIN32
      if ( hIcon )
	{
	  DestroyIcon( hIcon );
	}
#     endif
   Init();
   return 0;
   }


S_SystemIcon::~S_SystemIcon() {
   Destroy();
   }
