/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
#include "S_OS.h"
#include "S_BitmapData.h"
#include "S_Canvas.h"
#include "S_Root.h"
#include "S_ZArchive.h"

int
S_BitmapData::Init() {
   rc = S_BITMAPDATA_NOTLOADED;
   imageBytes = 0;
   pimageData = NULL;
   infoBytes = 0;
   pbmpInfo3 = NULL;
   return 0;
   }

S_BitmapData::S_BitmapData() {
   Init();
   }

S_BitmapData::S_BitmapData(int icx,int icy,int icBitCount) {
   Init();
   Create(icx,icy,icBitCount);
   }

int
S_BitmapData::Create(int icx,int icy,int icBitCount) {

   Destroy();

   imageBytes = icy * (((slong)(icx*((float)icBitCount/8.0)) + 3) / 4) * 4;
   pimageData = (char *)S_malloc(imageBytes);

   if ( !pimageData ) {
      rc = S_BITMAPDATA_MALLOCFAILED;
      return rc;
      }

   infoBytes = 40;
   if ( icBitCount != 24 )
      infoBytes += (1<<icBitCount)*sizeof(Rgb2Struct);
   pbmpInfo3 = (BitmapInfo3Struct *) S_malloc( infoBytes );
   if ( !pbmpInfo3 ) {
      S_free(pimageData);
      rc = S_BITMAPDATA_MALLOCFAILED;
      return rc;
      }

   for ( int i=0; i<(int)infoBytes; i++ )
      ((char *)pbmpInfo3)[i] = 0;

   pbmpInfo3->cbFix = 40;
   pbmpInfo3->cx = icx;
   pbmpInfo3->cy = icy;
   pbmpInfo3->cPlanes = 1;
   pbmpInfo3->cBitCount = (ushort)icBitCount;
   pbmpInfo3->ulCompression = 0;
   pbmpInfo3->cbImage = 0;
   pbmpInfo3->cxResolution = 0;
   pbmpInfo3->cyResolution = 0;
   pbmpInfo3->cclrUsed = 0;
   pbmpInfo3->cclrImportant = 0;

   ppalData = &pbmpInfo3->argbColor[0];

   return rc;
   }

int
S_BitmapData::FillInPaletteData() {
  if ( !ppalData || !pbmpInfo3 )
    {
      return 0;
    }

   int i = 0,j = 0;
   int numColors = 1<<pbmpInfo3->cBitCount;

   do {
      while ( i < numColors &&
	      (ppalData[i].bRed   != 0 ||
               ppalData[i].bGreen != 0 ||
               ppalData[i].bBlue  != 0) ) {
         i++;
         }
      i--;
      j = i+1;
      while ( j < numColors &&
	      ppalData[j].bRed == 0 && 
              ppalData[j].bGreen == 0 && 
              ppalData[j].bBlue == 0 ) {
         j++;
         }

      if ( j < numColors && j > i ) {
         for ( int k=i; k<j; k++ ) {
	   ppalData[k].bRed = (uchar)(ppalData[i].bRed + 
				      (k-i)*(ppalData[j].bRed - 
					     ppalData[i].bRed)/(j-i));
	   ppalData[k].bGreen = (uchar)(ppalData[i].bGreen + 
					(k-i)*(ppalData[j].bGreen - 
					       ppalData[i].bGreen)/(j-i));
	   ppalData[k].bBlue = (uchar)(ppalData[i].bBlue + 
				       (k-i)*(ppalData[j].bBlue - 
					      ppalData[i].bBlue)/(j-i));
	 }
      }
      
      i = j;

      } while ( i < numColors && j < numColors );

   return 0;
   }

int
S_BitmapData::SetPixelData(int i, int j, ushort val) {
   if ( pbmpInfo3 && pimageData &&
        i<(int)pbmpInfo3->cx && j<(int)pbmpInfo3->cy ) {
      int cx=(((slong)(pbmpInfo3->cx*((float)pbmpInfo3->cBitCount/8.0))+3)/4)*4;
      int index = i*pbmpInfo3->cBitCount/8 + j*cx;
      int offset = i*pbmpInfo3->cBitCount % 8;
      ushort *pus = (ushort *)&pimageData[index];
      uchar ich,ch;
      switch(pbmpInfo3->cBitCount) {
         case 4: ich = (uchar)((val & 0x0F) << (4-offset));
                 ch = (uchar)(pimageData[index] & (0x0F<<offset));
                 pimageData[index] = ch | ich;
                 return 0;
         case 8: pimageData[index] = (char)val; return 0;
         case 16: *pus = (ushort)val; return 0;
         }
      }
   return -1;
   }

int
S_BitmapData::GetPixelData(int i, int j) {
   if ( pbmpInfo3 && pimageData &&
        i<(int)pbmpInfo3->cx && j<(int)pbmpInfo3->cy ) {
      int cx=(((slong)(pbmpInfo3->cx*((float)pbmpInfo3->cBitCount/8.0))+3)/4)*4;
      int index = i*pbmpInfo3->cBitCount/8 + j*cx;
      int offset = i*pbmpInfo3->cBitCount % 8;
      ushort *pus = (ushort *)&pimageData[index];
      switch(pbmpInfo3->cBitCount) {
         case 4 : return (pimageData[index] & (0xF0>>offset)) >> (4-offset);
         case 8 : return pimageData[index];
         case 16: return *pus;
         }
      }
   return -1;
   }


int
S_BitmapData::GetPixelData(int i, int j,uchar *red,uchar *green,uchar *blue) {
   if ( pbmpInfo3 && pimageData &&
        i<(int)pbmpInfo3->cx && j<(int)pbmpInfo3->cy ) {
      int cx=(((slong)(pbmpInfo3->cx*((float)pbmpInfo3->cBitCount/8.0))+3)/4)*4;
      int index = i*pbmpInfo3->cBitCount/8 + j*cx;
      switch(pbmpInfo3->cBitCount) {
         case 8 : {
            uchar ch = pimageData[index++];
            *red = ppalData[ch].bRed;
            *green = ppalData[ch].bGreen;
            *blue = ppalData[ch].bBlue;
            return 0;
	    }
         case 24:
            *red = pimageData[index++];
            *green = pimageData[index++];
            *blue = pimageData[index++];
            return 0;
         }
      }
   return -1;
   }


int
S_BitmapData::SetPixelData(int i, int j, uchar red, uchar green, uchar blue) {
   if ( pbmpInfo3 && pimageData &&
        i<(int)pbmpInfo3->cx && j<(int)pbmpInfo3->cy ) {
      int cx=(((slong)(pbmpInfo3->cx*((float)pbmpInfo3->cBitCount/8.0))+3)/4)*4;
      int index = i*pbmpInfo3->cBitCount/8 + j*cx;
      switch(pbmpInfo3->cBitCount) {
         case 24:
            pimageData[index++] = red;
            pimageData[index++] = green;
            pimageData[index++] = blue;
            return 0;
         }
      }
   return -1;
   }


int
S_BitmapData::SetPaletteData(int index,int red,int green,int blue) {
   if ( pbmpInfo3 && index < (1<<pbmpInfo3->cBitCount) ) {
      pbmpInfo3->argbColor[index].bRed = (uchar)red;
      pbmpInfo3->argbColor[index].bGreen = (uchar)green;
      pbmpInfo3->argbColor[index].bBlue = (uchar)blue;
      pbmpInfo3->argbColor[index].fcOptions = 0;
      return 0;
      }
   return -1;
   }

S_BitmapData::S_BitmapData(char *ifilename ) {
   Init();
   Create(ifilename);
   }

S_BitmapData::S_BitmapData(uchar *ifileBuffer,ulong ifileSize ) {
   Init();
   Create(ifileBuffer,ifileSize);
   }

int
S_BitmapData::ZCreate(char *ifilename) {
   Destroy();
   S_ZArchive zArc;
   S_ZFile *zFile;
   zArc.Open(_root->exeFilename);
   do {
      zFile = zArc.ReadNextFile();
      if ( zFile ) {
         if ( stricmp(zFile->name,ifilename) == 0 ) {
            zFile->UnCompress();
            Create(zFile->buffer,zFile->hdr.fileSize);
	    }
         delete zFile;
         }
      } while (zFile);

   return 0;
   }


int
S_BitmapData::Create(char *ifilename ) {

   Destroy();

   FILE      *fptr;
   ulong      fileSize;

   rc = 0;

   /* ********************************************************************** */
   /* **** These will contain the temporary buffers which must be freed **** */
   /* ********************************************************************** */
   uchar *tmpFileBuffer = NULL;

   rc = S_BITMAPDATA_MALLOCFAILED;

   char *fileName = NULL;
   if ( _OsFileExists(ifilename) )
      fileName = S_strdup(ifilename);
   else {
      fileName = (char *)S_malloc(strlen(ifilename)+strlen(".bmp")+1);
      if ( fileName )
         sprintf(fileName,"%s.bmp",ifilename);
      }

   if ( fileName ) {
      /* ************************************************************* */
      /* **** Get the size of the file and allocate enough memory **** */
      /* **** to hold it.                                         **** */
      /* ************************************************************* */

      rc = S_BITMAPDATA_FILEDOESNOTEXIST;
      fptr = fopen( fileName,"rb");
      if ( fptr ) {
         slong pos1,pos2;
         pos1 = ftell(fptr);
         fseek(fptr,0,SEEK_END);
         pos2 = ftell(fptr);
         fileSize = pos2 - pos1;
         fclose(fptr);

         rc = S_BITMAPDATA_MALLOCFAILED;
         tmpFileBuffer = (uchar *) S_malloc( fileSize*sizeof(char) );
         if ( tmpFileBuffer ) {

            /* **************************** */
            /* **** Read the file data **** */
            /* **************************** */
            rc = S_BITMAPDATA_FILEDOESNOTEXIST;
            fptr = fopen(fileName,"rb");
            if ( fptr ) {
               fread(tmpFileBuffer,fileSize,1,fptr);
               fclose(fptr);
               Create(tmpFileBuffer,fileSize);
               secretCode = *((slong *)&tmpFileBuffer[fileSize-4]);
               checkSum = 0;
               for ( ulong i=0; i<fileSize; i++ )
		 {
                   checkSum += tmpFileBuffer[i];
		 }
	       }
            S_free(tmpFileBuffer);
	    }
         }
      S_free(fileName);
      }

   return rc;
   }


int
S_BitmapData::Create(uchar *ifileBuffer,ulong fileSize) {

   Destroy();

   int i;

   RgbStruct  *rgbPaletteData = NULL;
   Rgb2Struct *rgb2PaletteData = NULL;

   pimageData = NULL;
   pbmpInfo3 = NULL;
   char *fileImageData = NULL;

   rc = S_BITMAPDATA_OK;

   uchar *fileBuffer = (uchar *)S_malloc(fileSize+2);
   for ( i=0; i<(int)fileSize; i++ )
     fileBuffer[i+2] = ifileBuffer[i];

   BitmapFileHeader2Struct *pbmpFileHeader2;
   BitmapInfoHeader2Struct *pbmpInfoHeader2;

   pbmpFileHeader2 = (BitmapFileHeader2Struct *) fileBuffer;
   pbmpInfoHeader2=(BitmapInfoHeader2Struct *) &(pbmpFileHeader2->bmp2);

   /* **************************************************************** */
   /* **** If cbFix == 12, the file uses the old OS/2 1.x headers **** */
   /* **************************************************************** */
   BitmapFileHeaderStruct *pbmpFileHeader;
   BitmapInfoHeaderStruct *pbmpInfoHeader;
   BitmapInfoStruct *pbmpInfo;

   ulong cbFix = 0;
   ulong cx = 0;
   ulong cy = 0;
   ushort cPlanes = 0;
   ushort cBitCount = 0;
   ulong ulCompression = 0;
   ulong cbImage = 0;
   ulong cxResolution = 0;
   ulong cyResolution = 0;
   ulong cclrUsed = 0;
   ulong cclrImportant = 0;

   cbFix = pbmpInfoHeader2->cbFix;
   if (  cbFix == 12 ) {
      pbmpFileHeader = (BitmapFileHeaderStruct *) fileBuffer;
      pbmpInfoHeader = (BitmapInfoHeaderStruct *) &(pbmpFileHeader->bmp);
      pbmpInfo = (BitmapInfoStruct *) pbmpInfoHeader;

      cx = pbmpInfoHeader->cx;
      cy = pbmpInfoHeader->cy;
      cPlanes = pbmpInfoHeader->cPlanes;
      cBitCount = pbmpInfoHeader->cBitCount;

      rgbPaletteData = (RgbStruct *) ((char *)pbmpInfoHeader+cbFix);
      fileImageData = (char *) pbmpFileHeader + pbmpFileHeader->offBits + 2;
      imageBytes = cy * (((slong)(cx*((float)cBitCount/8.0)) + 3) / 4) * 4;
      infoBytes = 40;
      if ( cBitCount != 24 )
         infoBytes += (1<<cBitCount)*sizeof(RgbStruct);
      }
   else {
      cx = pbmpInfoHeader2->cx;
      cy = pbmpInfoHeader2->cy;
      cPlanes = pbmpInfoHeader2->cPlanes;
      cBitCount = pbmpInfoHeader2->cBitCount;

      ulCompression = pbmpInfoHeader2->ulCompression;
      cbImage = pbmpInfoHeader2->cbImage;
      cxResolution = pbmpInfoHeader2->cxResolution;
      cyResolution = pbmpInfoHeader2->cyResolution;
      cclrUsed = pbmpInfoHeader2->cclrUsed;
      cclrImportant = pbmpInfoHeader2->cclrImportant;

      rgb2PaletteData = (Rgb2Struct *) ((char *)pbmpInfoHeader2 + cbFix);
      fileImageData = (char *)pbmpFileHeader2 + pbmpFileHeader2->offBits + 2;
      if ( pbmpInfoHeader2->cbFix >= 24 && pbmpInfoHeader2->cbImage > 0 )
         imageBytes = pbmpInfoHeader2->cbImage;
      else
         imageBytes = cy * (((slong)(cx*((float)cBitCount/8.0)) + 3) / 4) * 4;
      infoBytes = 40;
      if ( cBitCount != 24 )
         infoBytes += (1<<cBitCount)*sizeof(Rgb2Struct);
      }

   if( (cBitCount!=1) && (cBitCount!=4)&&(cBitCount!=8)&&(cBitCount!=24)) {
      if ( fileBuffer != NULL )
         S_free(fileBuffer);
      rc = S_BITMAPDATA_INVALIDNUMBITS;
      return rc;
      }

   pimageData = (char *)S_malloc(imageBytes);

   if ( !pimageData ) {
      S_free(fileBuffer);
      rc = S_BITMAPDATA_MALLOCFAILED;
      return rc;
      }

   for ( i=0; i<(int)imageBytes; i++ )
      pimageData[i] = fileImageData[i];

   /* ********************************************************************** */
   /* ********* Create a bitmap header for the bmp to be created *********** */
   /* ********************************************************************** */
   pbmpInfo3 = (BitmapInfo3Struct *) S_malloc( infoBytes );

   if ( !pbmpInfo3 ) {
      S_free(pimageData);
      S_free(fileBuffer);
      rc = S_BITMAPDATA_MALLOCFAILED;
      return rc;
      }

   pbmpInfo3->cbFix = 40;
   pbmpInfo3->cx = cx;
   pbmpInfo3->cy = cy;
   pbmpInfo3->cPlanes = cPlanes;
   pbmpInfo3->cBitCount = cBitCount;
   pbmpInfo3->ulCompression = ulCompression;
   pbmpInfo3->cbImage = cbImage;
   pbmpInfo3->cxResolution = cxResolution;
   pbmpInfo3->cyResolution = cyResolution;
   pbmpInfo3->cclrUsed = cclrUsed;
   pbmpInfo3->cclrImportant = cclrImportant;
   switch ( cbFix ) {
      case 12: 
      case 16:
         pbmpInfo3->ulCompression = 0;
      case 20:
         pbmpInfo3->cbImage = 0;
      case 24:
         pbmpInfo3->cxResolution = 0;
      case 28:
         pbmpInfo3->cyResolution = 0;
      case 32:
         pbmpInfo3->cclrUsed = 0;
      case 36:
         pbmpInfo3->cclrImportant = 0;
         break;
      }
   /* ********************************************************************** */

   /* ********************************************************************** */
   /* ************* Copy the palette info to the new structure ************* */
   /* ********************************************************************** */
   if ( cBitCount != 24 ) {
      for ( i=0; i<1<<cBitCount; i++ ) {
         if ( cbFix == 12 ) {
            pbmpInfo3->argbColor[i].bRed = rgbPaletteData[i].bRed;
            pbmpInfo3->argbColor[i].bGreen = rgbPaletteData[i].bGreen ;
            pbmpInfo3->argbColor[i].bBlue = rgbPaletteData[i].bBlue;
            pbmpInfo3->argbColor[i].fcOptions = 0;
            }
         else {
            pbmpInfo3->argbColor[i].bRed = rgb2PaletteData[i].bRed;
            pbmpInfo3->argbColor[i].bGreen = rgb2PaletteData[i].bGreen;
            pbmpInfo3->argbColor[i].bBlue = rgb2PaletteData[i].bBlue;
            pbmpInfo3->argbColor[i].fcOptions = rgb2PaletteData[i].fcOptions;
            }
         }
      }
   /* ********************************************************************** */

   ppalData = &pbmpInfo3->argbColor[0];

   if ( fileBuffer ) {
      S_free(fileBuffer);
      fileBuffer = NULL;
      }

   rc = S_BITMAPDATA_OK;
   return rc;
   }

int
S_BitmapData::Destroy() {
   if ( pimageData )
      S_free( pimageData );
   if ( pbmpInfo3 )
      S_free(pbmpInfo3);
   Init();
   return 0;
   }

S_BitmapData::~S_BitmapData() {
   Destroy();
   }


int
S_BitmapData::SystemBlit(S_Canvas *target,
                         int x1targ,int y1targ,int x2targ,int y2targ,
                         int x1src,int y1src,ulong ropFlags ) {

   if ( !target || (target && !target->hdc) || !pimageData || !pbmpInfo3 ) {
      rc = S_BITMAPDATA_NODATA;
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
                        x1src,y1src,y1src,pbmpInfo3->cy,pimageData,
                       (BITMAPINFO *)pbmpInfo3,DIB_RGB_COLORS);
		       */
      StretchDIBits(target->hdc,x1targ,y1targ,x2targ-x1targ,y2targ-y1targ,
                    x1src,y1src,x2targ-x1targ,y2targ-y1targ,pimageData,
                    (BITMAPINFO *)pbmpInfo3,DIB_RGB_COLORS,ropFlags);
#  endif
   return 0;
   }


int
S_BitmapData::SystemBlit(S_Canvas *target,
                     int x1targ,int y1targ,int x2targ,int y2targ,
                     int x1src,int y1src,int x2src,int y2src,
                     ulong ropFlags ) {

   if ( !target || (target && !target->hdc) || !pimageData || !pbmpInfo3 ) {
      rc = S_BITMAPDATA_NODATA;
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
