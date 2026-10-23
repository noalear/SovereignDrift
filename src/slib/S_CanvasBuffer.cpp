/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
#include "S_OS.h"
#include "S_Root.h"
#include "S_CanvasBuffer.h"
#include "S_SystemBitmap.h"
#include "S_Bitmap.h"

int
S_CanvasBuffer::Init() {
   oldFont = NULL;
   font = NULL;
   lpBuffer = NULL;
   hdc = NULL;
   hbm = NULL;
   ohbm = NULL;
   parentBuffer = NULL;
   cxCanvas = 0;
   cyCanvas = 0;
   xCanvas = 0;
   yCanvas = 0;
   numBits = 0;
   visible = TRUE;
   lpBuffer = NULL;
   rect = NULL;
   oldRect = NULL;
   blitRect = NULL;
   numRects = maxRects = numOldRects = numBlitRects = 0;
   parentBuffer = NULL;
   embeddedBuffer = NULL;
   numEmbeddedBuffers = maxEmbeddedBuffers = 0;
   return 0; 
  }

S_CanvasBuffer::S_CanvasBuffer() {
   Init();
   }

S_CanvasBuffer::S_CanvasBuffer(int icx,int icy,int inumBits) {
   Init();
   Create(icx,icy,inumBits);
   }

int
S_CanvasBuffer::Create(int icx,int icy,int inumBits) {
   Destroy();

   cxCanvas = icx;
   cyCanvas = icy;
   numBits = (ushort)inumBits;

   ResetClipRect();

#  ifdef OS2
      DEVOPENSTRUC dop = { 0L, "*", NULL, 0L,0L,0L,0L,0L,0L };
      HAB hab = WinInitialize (0) ;
      HPS hdcScr = DevOpenDC(hab,OD_MEMORY,"*",5L,(PDEVOPENDATA)&dop,NULL);
      SIZEL sizl = {0,0};
      hdc = GpiCreatePS(hab,hdcScr,&sizl,
                        PU_PELS|GPIF_DEFAULT|GPIT_MICRO|GPIA_ASSOC);
      BITMAPINFOHEADER2 bmp;
      bmp.cbFix = sizeof(bmp);
      bmp.cx = cxCanvas;
      bmp.cy = cyCanvas;
      bmp.cPlanes = 1;
      bmp.cBitCount = numBits;
      hbm = GpiCreateBitmap(hdc,(PBITMAPINFOHEADER2)&bmp,0L,0L,NULL);
      GpiSetBitmap( hdc,hbm );
      GpiCreateLogColorTable(hdc,0,LCOLF_RGB,0,0,0);
      DevCloseDC(hdcScr);
      WinTerminate(hab);
      palette = SetPalette(NULL);
#  endif
#  ifdef WIN32
      lpClipper = NULL;
      if ( _root->lpDD ) {
          DDSURFACEDESC ddsd;
          ddsd.dwSize = sizeof(ddsd);
          ddsd.dwHeight = cyCanvas;
          ddsd.dwWidth = cxCanvas;
          ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
          ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN|DDSCAPS_VIDEOMEMORY;
          _root->lpDD->CreateSurface(&ddsd,&lpBuffer,NULL);
          palette = SetPalette(NULL);
          }
      hbm = NULL;
      hdc = NULL;
      if ( !lpBuffer ) {
	 HDC hdcScr = GetDC(NULL);
         hdc = CreateCompatibleDC(hdcScr);
         palette = SetPalette(NULL);
         hbm = CreateCompatibleBitmap(hdcScr,cxCanvas,cyCanvas);
         ohbm = (HBITMAP)SelectObject(hdc,hbm);
         ReleaseDC(NULL,hdcScr);
         SetBkMode(hdc,TRANSPARENT);
         SetTextAlign(hdc,TA_BASELINE);
         SetStretchBltMode(hdc,STRETCH_DELETESCANS);
         }
#  endif

   return 0;
   }


int
S_CanvasBuffer::Destroy() {
   if ( rect ) {
      S_free( rect );
      rect = NULL;
      }
   if ( oldRect ) {
      S_free( oldRect );
      oldRect = NULL;
      }
   if ( blitRect ) {
      S_free( blitRect );
      blitRect = NULL;
      }
   if ( embeddedBuffer ) {
      S_free( embeddedBuffer );
      embeddedBuffer = NULL;
      }

#  ifdef OS2
      ResetPalette();
      if ( hbm ) {
         GpiDeleteBitmap(hbm);
         hbm = NULL;
         }
      if ( hdc ) {
         GpiDestroyPS(hdc);
         hdc = NULL;
         }
#  endif
#  ifdef WIN32
      ResetPalette();
      if ( lpBuffer ) {
         lpBuffer->Release();
         lpBuffer = NULL;
         }
      if ( hdc ) {
         SelectObject(hdc,oldFont);
         SelectObject(hdc,ohbm);
         DeleteDC(hdc);
         hdc = NULL;
         }
      if ( hbm ) {
         DeleteObject(hbm);
         hbm = NULL;
         }
#  endif
   S_Canvas::Destroy();

   Init();
   return 0;
   }

S_CanvasBuffer::~S_CanvasBuffer() {
   Destroy();
   }


int
S_CanvasBuffer::FillInRects( ulong color,ulong flags ) {
   int i;
   switch ( flags ) {
      case S_CANVASBUFFER_BLIT_ALL :
         DrawFilledRect(0,0,cxCanvas,cyCanvas,color,
                        S_CANVASBUFFER_NORECTANGLE);
         break;
      case S_CANVASBUFFER_BLIT_RECTANGLES :
         for ( i=0; i<numRects; i++ )
            DrawFilledRect(rect[i].xLeft,rect[i].yTop,
                           rect[i].xRight,rect[i].yBottom,color,
                           S_CANVASBUFFER_NORECTANGLE);
         break;
      }
   return 0;
   }


int
S_CanvasBuffer::BlitInRects(S_SystemBitmap *bmp, ulong flags ) {

   int i;
   switch ( flags ) {

      case S_CANVASBUFFER_BLIT_ALL :
         bmp->Blit(this,0,0,0,cxCanvas,cyCanvas,0,0,S_BITMAP_BLIT_OPAQUE);

         /* ********************************************* */
         /* **** Make sure embedded bmps stay on top **** */
         /* ********************************************* */
         for ( i=0; i<numEmbeddedBuffers; i++ )
            if ( embeddedBuffer[i]->visible )
               embeddedBuffer[i]->
                  SystemBlit(this,
                             embeddedBuffer[i]->position.xLeft,
                             embeddedBuffer[i]->position.yTop,
                             embeddedBuffer[i]->position.xRight+1,
                             embeddedBuffer[i]->position.yTop + 1,
                             0,0);
         break;

      case S_CANVASBUFFER_BLIT_RECTANGLES :
         for ( i=0; i<numRects; i++ )
            bmp->Blit(this,0,rect[i].xLeft,rect[i].yTop,
                      rect[i].xRight,rect[i].yBottom,
                      rect[i].xLeft,cyCanvas-rect[i].yBottom,
                      S_BITMAP_BLIT_OPAQUE);
         break;

      }

   return 0;
   }

int
S_CanvasBuffer::BlitInRects(S_Bitmap *bmp, ulong flags ) {

   int i;
   switch ( flags ) {

      case S_CANVASBUFFER_BLIT_ALL :
         bmp->Blit(this,0,0,0,cxCanvas,cyCanvas,0,0,S_BITMAP_BLIT_OPAQUE);

         /* ********************************************* */
         /* **** Make sure embedded bmps stay on top **** */
         /* ********************************************* */
         for ( i=0; i<numEmbeddedBuffers; i++ )
            if ( embeddedBuffer[i]->visible )
               embeddedBuffer[i]->
                  SystemBlit(this,
                             embeddedBuffer[i]->position.xLeft,
                             embeddedBuffer[i]->position.yTop,
                             embeddedBuffer[i]->position.xRight+1,
                             embeddedBuffer[i]->position.yTop + 1,
                             0,0);
         break;

      case S_CANVASBUFFER_BLIT_RECTANGLES :
         for ( i=0; i<numRects; i++ )
            bmp->Blit(this,0,rect[i].xLeft,rect[i].yTop,
                      rect[i].xRight,rect[i].yBottom,
                      rect[i].xLeft,cyCanvas-rect[i].yBottom,
                      S_BITMAP_BLIT_OPAQUE);
         break;

      }

   return 0;
   }


int
S_CanvasBuffer::AddRect(int x1,int y1,int x2,int y2 ) {

   if ( x1 >= clipRect.xRight  || x2 <= clipRect.xLeft ||
        y1 >= clipRect.yBottom || y2 <= clipRect.yTop )
      return 0;

   /* ************************************************************ */
   /* **** Add update rectangle getting more memory if needed **** */
   /* ************************************************************ */
   if ( maxRects == 0 ) {
      maxRects++;
      rect = (RectStruct *) S_malloc( sizeof(RectStruct)*maxRects );
      oldRect = (RectStruct *) S_malloc( sizeof(RectStruct)*maxRects );
      blitRect = (RectStruct *) S_malloc( sizeof(RectStruct)*maxRects );
      }
   else if ( maxRects <= numRects ) {
      maxRects++;
      rect = (RectStruct *) S_realloc(rect,sizeof(RectStruct)*maxRects);
      oldRect = (RectStruct *) S_realloc(oldRect,sizeof(RectStruct)*maxRects);
      blitRect = (RectStruct *) S_realloc(blitRect,sizeof(RectStruct)*maxRects);
      }

   /*DrawRect(x1,y1,x2-1,y2-1,0xFFFFFF);*/
   /* ************************************************************ */

   /* **** Check limits **** */
   if ( x1 < clipRect.xLeft   ) x1 = clipRect.xLeft;
   if ( y1 < clipRect.yTop    ) y1 = clipRect.yTop;
   if ( x2 > clipRect.xRight  ) x2 = clipRect.xRight;
   if ( y2 > clipRect.yBottom ) y2 = clipRect.yBottom;

   /* **** Add new update rectangle **** */
   rect[numRects].xLeft = x1;
   rect[numRects].xRight = x2;
   rect[numRects].yTop = y1;
   rect[numRects].yBottom = y2;

   blitRect[numRects] = rect[numRects];

   numRects++;
   numBlitRects = numRects;

   return(0);
   }


int
S_CanvasBuffer::Show( int flag ) {
   visible = flag;
   if ( parentBuffer )
      parentBuffer->AddRect(position.xLeft,position.yTop,
                            position.xRight,position.yBottom);
   return 0;
   }


int
S_CanvasBuffer::Embed(S_CanvasBuffer *child,int x,int y ) {

   /* *********************************************************** */
   /* **** Add embedded buffer getting more memory if needed **** */
   /* *********************************************************** */
   if ( maxEmbeddedBuffers == 0 ) {
      maxEmbeddedBuffers++;
      embeddedBuffer = (S_CanvasBuffer **) 
                        S_malloc( sizeof(S_CanvasBuffer *)*maxEmbeddedBuffers );
      }
   else if ( maxEmbeddedBuffers <= numEmbeddedBuffers ) {
      maxEmbeddedBuffers++;
      embeddedBuffer = (S_CanvasBuffer **) 
          S_realloc( embeddedBuffer,sizeof(S_CanvasBuffer *)*maxEmbeddedBuffers);
       }
   /* ************************************************************ */

   embeddedBuffer[numEmbeddedBuffers] = child;
   child->position.xLeft = x;
   child->position.yTop = y;
   child->position.xRight = x + child->cxCanvas;
   child->position.yBottom = y + child->cyCanvas;
   child->parentBuffer = this;
   child->Blit(this,x,y,S_CANVASBUFFER_BLIT_ALL);

   numEmbeddedBuffers++;

   return(0);
   }


int
S_CanvasBuffer::DrawCircle(int x,int y,int rad,ulong color,ulong flag) {
   S_Canvas::DrawCircle(x,y,rad,color);
   if ( !(flag & S_CANVASBUFFER_NORECTANGLE) )
      AddRect( x-rad-1,y-rad-1,x+rad+1,y+rad+1 );
   return 0;
   }


int
S_CanvasBuffer::DrawFilledCircle(int x,int y,int rad,ulong color,ulong flag) {
   S_Canvas::DrawFilledCircle(x,y,rad,color);
   if ( !(flag & S_CANVASBUFFER_NORECTANGLE) )
      AddRect( x-rad-1,y-rad-1,x+rad+1,y+rad+1 );
   return 0;
   }


int
S_CanvasBuffer::DrawTriangle( int x0,int y0,int x1,int y1,int x2,int y2,
                              ulong color, ulong flag ) {
   S_Canvas::DrawTriangle(x0,y0,x1,y1,x2,y2,color);
   if ( !(flag & S_CANVASBUFFER_NORECTANGLE) ) {
      int minX = x0;
      int minY = y0;
      int maxX = x0;
      int maxY = y0;

      if ( x1 > maxX ) maxX = x1;
      if ( x2 > maxX ) maxX = x2;
      if ( y1 > maxY ) maxY = y1;
      if ( y2 > maxY ) maxY = y2;

      if ( x1 < minX ) minX = x1;
      if ( x2 < minX ) minX = x2;
      if ( y1 < minY ) minY = y1;
      if ( y2 < minY ) minY = y2;

      AddRect( minX-1,minY-1,maxX+1,maxY+1 );
      }

   return 0;
   }


int
S_CanvasBuffer::DrawFilledRect( int x1,int y1,int x2,int y2,
                                ulong color,ulong flag ) {
   S_Canvas::DrawFilledRect(x1,y1,x2,y2,color);
   if ( !(flag & S_CANVASBUFFER_NORECTANGLE) )
      AddRect( x1,y1,x2+1,y2+1 );
   return 0;
   }


int
S_CanvasBuffer::DrawRect( int x1,int y1,int x2,int y2,
                          ulong color,ulong flag ) {
   S_Canvas::DrawRect(x1,y1,x2,y2,color);
   if ( !(flag & S_CANVASBUFFER_NORECTANGLE) )
      AddRect( x1,y1,x2+1,y2+1 );
   return 0;
   }


int
S_CanvasBuffer::DrawFilledPieWedge(int x0,int y0,int rx,int ry,
                                   int angle1,int angle2,ulong color,
                                   ulong flag ) {
   S_Canvas::DrawFilledPieWedge(x0,y0,rx,ry,angle1,angle2,color);
   if ( !(flag & S_CANVASBUFFER_NORECTANGLE) )
      AddRect( x0-rx,y0-ry,x0+rx,y0+ry );
   return 0;
   }


int
S_CanvasBuffer::DrawPieWedge( int x0,int y0,int rx,int ry,
                              int angle1,int angle2,ulong color,
                              ulong flag ) {
   S_Canvas::DrawPieWedge(x0,y0,rx,ry,angle1,angle2,color);
   if ( !(flag & S_CANVASBUFFER_NORECTANGLE) )
      AddRect( x0-rx,y0-ry,x0+rx,y0+ry );
   return 0;
   }


int
S_CanvasBuffer::DrawLine( int x1,int y1,int x2,int y2,
                          int width,ulong color,ulong flag ) {

   S_Canvas::DrawLine(x1,y1,x2,y2,width,color);
   if ( !(flag & S_CANVASBUFFER_NORECTANGLE) ) {
      int xx1 = min(x1 - width,x2 - width) - 1;
      int yy1 = min(y1 - width,y2 - width) - 1;
      int xx2 = max(x1 + width,x2 + width) + 1;
      int yy2 = max(y1 + width,y2 + width) + 1;
      int dx = xx2 - xx1;
      int dy = yy2 - yy1;
      int cMax = 40;

      if ( dx < cMax || dy < cMax ) {
         AddRect(xx1,yy1,xx2,yy2);
         }
      else {
         if ( dx > dy ) {
            if ( (x1 < x2 && y1 < y2) || (x1 > x2 && y1 > y2) )
               for ( int i=xx1; i<xx2; i+=cMax )
                 AddRect(i        - 2,yy1 + ((i        - xx1)*dy)/dx - 2,
                         i + cMax + 2,yy1 + ((i + cMax - xx1)*dy)/dx + 2);
            else 
               for ( int i=xx1; i<xx2; i+=cMax )
                 AddRect(i        - 2,yy2 - ((i + cMax - xx1)*dy)/dx - 2,
                         i + cMax + 2,yy2 - ((i        - xx1)*dy)/dx + 2);
            }
         else {
            if ( (x1 < x2 && y1 < y2) || (x1 > x2 && y1 > y2) )
               for ( int j=yy1; j<yy2; j+=cMax )
                  AddRect(xx1 + ((j        - yy1)*dx)/dy - 2,j        - 2,
                          xx1 + ((j + cMax - yy1)*dx)/dy + 2,j + cMax + 2);
            else
               for ( int j=yy1; j<yy2; j+=cMax )
                  AddRect(xx2 - ((j + cMax - yy1)*dx)/dy - 2,j        - 2,
                          xx2 - ((j        - yy1)*dx)/dy + 2,j + cMax + 2);
            }
	 }
      }
   return 0;
   }


int
S_CanvasBuffer::DrawFilledRoundedRect( int x1,int y1,int x2,int y2,
                                       int rad,ulong color,ulong flag ) {
   S_Canvas::DrawFilledRoundedRect(x1,y1,x2,y2,rad,color);
   if ( !(flag & S_CANVASBUFFER_NORECTANGLE) )
      AddRect( x1,y1,x2+1,y2+1 );
   return 0;
   }


int
S_CanvasBuffer::DrawRoundedRect( int x1,int y1,int x2,int y2,
                                 int rad,ulong color,ulong flag ) {
   S_Canvas::DrawRoundedRect(x1,y1,x2,y2,rad,color);
   if ( !(flag & S_CANVASBUFFER_NORECTANGLE) )
      AddRect( x1,y1,x2+1,y2+1 );
   return 0;
   }



SizeStruct S_CanvasBuffer::DrawString(char *str,int x,int y,ulong color,ulong flag) {
   SizeStruct size = S_Canvas::DrawString(str,x,y,color);
   if ( !(flag & S_CANVASBUFFER_NORECTANGLE) && size.cx>0 && size.cy>0 )
      AddRect( x-1,y-size.cy,x+size.cx+2,y+lMaxDescender+1 );
   return size;
   }


int
S_CanvasBuffer::ResetRects(int flag) {
   int i;

   numOldRects = numRects;
   numRects = numBlitRects = 0;
   for ( i=0; i<numOldRects; i++ )
      oldRect[i] = rect[i];

   if ( flag )
      for ( i=0; i<numEmbeddedBuffers; i++ )
         embeddedBuffer[i]->ResetRects();

   return(0);
   }


int
S_CanvasBuffer::Blit(S_Canvas *target,int x,int y,ulong flags){

   if ( !target )
      return 0;

   int i,j;
   int x1,y1,x2,y2,x3,y3;
   int oldx1,oldx2,oldy1,oldy2;
   switch (flags) {

      case S_CANVASBUFFER_BLIT_ALL :

         /* ********************************************* */
         /* **** Make sure embedded bmps stay on top **** */
         /* ********************************************* */
         for ( i=0; i<numEmbeddedBuffers; i++ )
            if ( embeddedBuffer[i]->visible )
               embeddedBuffer[i]->Blit(this,
                  embeddedBuffer[i]->position.xLeft,
                  embeddedBuffer[i]->position.yTop,
                  S_CANVASBUFFER_BLIT_ALL);

         SystemBlit(target,x,y,x+cxCanvas,y+cyCanvas,0,0);
         break;

      case S_CANVASBUFFER_BLIT_RECTANGLES : {
         slong mergedWithNew,mergedWithOld;
         slong totalPixelsCombined,totalPixelsSeparate;

         /* ************************************************************** */
         /* **** Clean up rectangles.  Any old rectangles that share  **** */
         /* **** pixels with a blit rectangle should be merged with   **** */
         /* **** blit rectangle.  Likewise for new rectangles.  This  **** */
         /* **** leaves oldRects with fewer regions to clean up since **** */
         /* **** blitRects will take care of it.                      **** */
         /* ************************************************************** */

         /* *** Merge blit rects that share space with old rects *** */
         for ( i=0; i<numBlitRects; i++ ) {
            mergedWithOld = FALSE;
            for ( j=0; j<numOldRects; j++ ) {
               if ( !mergedWithOld ) {
                  x1 = blitRect[i].xLeft;
                  x2 = blitRect[i].xRight;
                  y1 = blitRect[i].yTop;
                  y2 = blitRect[i].yBottom;

                  oldx1 = oldRect[j].xLeft;
                  oldx2 = oldRect[j].xRight;
                  oldy1 = oldRect[j].yTop;
                  oldy2 = oldRect[j].yBottom;

                  totalPixelsCombined = (max(x2,oldx2) - min(x1,oldx1)) *
                                        (max(y2,oldy2) - min(y1,oldy1));
                  totalPixelsSeparate = (x2-x1)*(y2-y1) + 
                                        (oldx2-oldx1)*(oldy2-oldy1);

                  if ( totalPixelsCombined < totalPixelsSeparate ) {
                     blitRect[i].xLeft = min(x1,oldx1);
                     blitRect[i].xRight = max(x2,oldx2);
                     blitRect[i].yTop = min(y1,oldy1);
                     blitRect[i].yBottom = max(y2,oldy2);

                     oldRect[j].xLeft = 0;
                     oldRect[j].xRight = -1;
                     oldRect[j].yBottom = 0;
                     oldRect[j].yTop = -1;

                     mergedWithOld = TRUE;
                     }
                  }
               }
            }

         /* *** Merge blit rects that share space with each other *** */
         for (i=0; i<numBlitRects; i++ ) {
            mergedWithNew = FALSE;
            for ( j=0; j<i; j++ ) {
               if ( !mergedWithNew ) {
                  x1 = blitRect[i].xLeft;
                  x2 = blitRect[i].xRight;
                  y1 = blitRect[i].yTop;
                  y2 = blitRect[i].yBottom;

                  oldx1 = blitRect[j].xLeft;
                  oldx2 = blitRect[j].xRight;
                  oldy1 = blitRect[j].yTop;
                  oldy2 = blitRect[j].yBottom;

                  totalPixelsCombined = (max(x2,oldx2) - min(x1,oldx1)) *
                                        (max(y2,oldy2) - min(y1,oldy1));
                  totalPixelsSeparate = (x2-x1)*(y2-y1) + 
                                        (oldx2-oldx1)*(oldy2-oldy1);

                  if ( totalPixelsCombined < totalPixelsSeparate ) {
                     blitRect[j].xLeft = min(x1,oldx1);
                     blitRect[j].xRight = max(x2,oldx2);
                     blitRect[j].yTop = min(y1,oldy1);
                     blitRect[j].yBottom = max(y2,oldy2);

                     blitRect[i].xLeft = 0;
                     blitRect[i].xRight = -1;
                     blitRect[i].yBottom = 0;
                     blitRect[i].yTop = -1;

                     mergedWithNew = TRUE;
                     }
                  }
               }
            }

         /* *************************************************************** */
         /* **** Make sure embedded bmps stay on top.  Don't erase any **** */
         /* **** embedded buffer regions.                              **** */
         /* *************************************************************** */
         for ( i=0; i<numEmbeddedBuffers; i++ ) {
            for ( j=0; j<numOldRects; j++ ) {

               if (embeddedBuffer[i]->position.xLeft   <= oldRect[j].xRight &&
                   embeddedBuffer[i]->position.xRight  >= oldRect[j].xLeft &&
                   embeddedBuffer[i]->position.yBottom >= oldRect[j].yTop &&
                   embeddedBuffer[i]->position.yTop    <= oldRect[j].yBottom ){

                  if ( embeddedBuffer[i]->visible ) {
                     x1 = max(embeddedBuffer[i]->position.xLeft,
                              oldRect[j].xLeft);
                     y1 = max(embeddedBuffer[i]->position.yTop,
                              oldRect[j].yTop);
                     x2 = min(embeddedBuffer[i]->position.xRight,
                              oldRect[j].xRight);
                     y2 = min(embeddedBuffer[i]->position.yBottom,
                              oldRect[j].yBottom);
                     x3 = x1 - embeddedBuffer[i]->position.xLeft;
                     y3 = y1 - embeddedBuffer[i]->position.yTop;

                     embeddedBuffer[i]->SystemBlit(this,x1,y1,x2,y2,x3,y3);
                     }
                  }
               }
            }

         for ( i=0; i<numEmbeddedBuffers; i++ ) {
            for ( j=0; j<numBlitRects; j++ ) {

               if ( embeddedBuffer[i]->position.xLeft <= blitRect[j].xRight &&
                    embeddedBuffer[i]->position.xRight >= blitRect[j].xLeft &&
                    embeddedBuffer[i]->position.yBottom >= blitRect[j].yTop &&
                    embeddedBuffer[i]->position.yTop <= blitRect[j].yBottom ) {

                  if ( embeddedBuffer[i]->visible ) {
                     x1 = max(embeddedBuffer[i]->position.xLeft,
                              blitRect[j].xLeft);
                     y1 = max(embeddedBuffer[i]->position.yTop,
                              blitRect[j].yTop);
                     x2 = min(embeddedBuffer[i]->position.xRight,
                              blitRect[j].xRight);
                     y2 = min(embeddedBuffer[i]->position.yBottom,
                              blitRect[j].yBottom);
                     x3 = x1 - embeddedBuffer[i]->position.xLeft;
                     y3 = y1 - embeddedBuffer[i]->position.yTop;

                     embeddedBuffer[i]->SystemBlit(this,x1,y1,x2,y2,x3,y3);
                     }
                  }
               }
            }

         for (i=0; i<numOldRects; i++ ) {
            x1 = x + oldRect[i].xLeft;
            y1 = y + oldRect[i].yTop;
            x2 = x + oldRect[i].xRight;
            y2 = y + oldRect[i].yBottom;
            x3 = oldRect[i].xLeft;
            y3 = oldRect[i].yTop;
            if ( x2 > x1 && y2 > y1 ) {
               SystemBlit(target,x1,y1,x2,y2,x3,y3);
	       }
            }

         for (i=0; i<numBlitRects; i++ ) {
            x1 = x + blitRect[i].xLeft;
            y1 = y + blitRect[i].yTop;
            x2 = x + blitRect[i].xRight;
            y2 = y + blitRect[i].yBottom;
            x3 = blitRect[i].xLeft;
            y3 = blitRect[i].yTop;
            if ( x2 > x1 && y2 > y1 ) {
               SystemBlit(target,x1,y1,x2,y2,x3,y3);
               }
            }

         for ( i=0; i<numEmbeddedBuffers; i++ )
            if ( embeddedBuffer[i]->visible )
               embeddedBuffer[i]->Blit(target,
                                       x+embeddedBuffer[i]->position.xLeft,
                                       y+embeddedBuffer[i]->position.yTop,
                                       S_CANVASBUFFER_BLIT_RECTANGLES);
         break;
         }
      }

   return(0);
   }

