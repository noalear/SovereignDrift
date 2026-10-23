#include "sf.h"
#include "SF_MapConsole.h"
#include "SF_Sprite.h"
#include "SF_Globals.h"
#include "SF_ClientWindow.h"
#include "SF_Client.h"
#include "SF_Externals.h"

SF_MapConsole::SF_MapConsole(S_CanvasBuffer *iparentBuffer,
                             int ix0, int iy0) {
   parentBuffer = iparentBuffer;
   bmpBack = NULL;

   x0 = ix0;
   y0 = iy0;

   zoom = 4;
   visible = TRUE;
   redrawAll = TRUE;

#  ifdef WIN32
      font = new S_Font("Small Fonts",7);
#  else
      font = new S_Font(S_FONT_ARIAL,8);
#  endif

   int cy = min((int)sf->cyMapConsole,parentBuffer->cyCanvas-y0-sf->cyChatMin);

   buffer = new S_CanvasBuffer(sf->cxMapConsole,cy,_root->bitsPerPixel);
   buffer->SetFont(font);
   parentBuffer->Embed(buffer,x0,y0);
   }


int
SF_MapConsole::SetBmpBack(SF_Sprite *ibmpBack) {
   if ( ibmpBack ) {
      bmpBack = ibmpBack;
      }
   return 0;
   }


int
SF_MapConsole::Draw() {
   if ( visible ) {
      buffer->ResetClipRect();
      if ( redrawAll ) {
         if ( bmpBack ) {
            bmpBack->RequestDownload();
            buffer->BlitInRects(bmpBack,S_CANVASBUFFER_BLIT_ALL);
            ((S_SystemBitmap *)bmpBack)->SystemBlit(buffer,
                                                    0,0,buffer->cxCanvas,10,
                                                    0,bmpBack->cy-10);
	    }
         buffer->Show(visible);
         redrawAll = FALSE;
         }
      else {
         if ( bmpBack ) {
            bmpBack->RequestDownload();
            buffer->BlitInRects(bmpBack,S_CANVASBUFFER_BLIT_RECTANGLES);
            ((S_SystemBitmap *)bmpBack)->SystemBlit(buffer,
                                                    0,0,buffer->cxCanvas,10,
                                                    0,bmpBack->cy-10);
	    }
         }
      }
   else {
      redrawAll = FALSE;
      }

   buffer->ResetRects();

   if ( visible ) {

      buffer->SetClipRect(10,10,buffer->cxCanvas-10,buffer->cyCanvas-10);

      int i;
      int dx = buffer->cxCanvas/zoom;
      int dy = buffer->cyCanvas/zoom;
      if ( dx > 1 && dy > 1 ) {
         for ( i=dx/2; i<buffer->cxCanvas; i +=dx )
            buffer->DrawLine(i,0,i,buffer->cyCanvas,1,0x008000);
         for ( i=dy/2; i<buffer->cyCanvas; i += dy )
            buffer->DrawLine(0,i,buffer->cxCanvas,i,1,0x008000);
	  }
      }

   if ( redrawAll ) {
      buffer->Show(visible);
      }

   return 0;
   }


int
SF_MapConsole::Contains(int x,int y) {
   return x>x0 && x<x0+buffer->cxCanvas && y>y0 && y<y0+buffer->cyCanvas;
   }


int
SF_MapConsole::SetVisible(int ivisible) {
   visible = ivisible;
   redrawAll = TRUE;
   buffer->Show(visible);
   return 0;
   }

SF_MapConsole::~SF_MapConsole() {
   if ( buffer ) {
      delete buffer;
      buffer = NULL;
      }
  if ( font ) {
     delete font;
     font = NULL;
     }
   }
