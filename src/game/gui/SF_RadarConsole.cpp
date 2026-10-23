#include "sf.h"
#include "SF_RadarConsole.h"
#include "SF_Sprite.h"

SF_RadarConsole::SF_RadarConsole(S_CanvasBuffer *iparentBuffer,SF_Sprite *ibmpBack,
                                 int ix0, int iy0) {
   parentBuffer = iparentBuffer;
   bmpBack = ibmpBack;
   x0 = ix0;
   y0 = iy0;

   zoom = 4;
   visible = TRUE;
   redrawAll = TRUE;

   font = new S_Font(S_FONT_ARIAL,8);

   buffer = new S_CanvasBuffer(bmpBack->cx,bmpBack->cy,_root->bitsPerPixel);
   buffer->SetFont(font);
   parentBuffer->Embed(buffer,x0,y0);
   }


int
SF_RadarConsole::Draw() {
   if ( visible ) {

      if ( redrawAll ) {
         redrawAll = FALSE;
         buffer->Show(visible);
         DrawBackGround(S_CANVASBUFFER_BLIT_ALL);
         }
       else
         DrawBackGround(S_CANVASBUFFER_BLIT_RECTANGLES);
       }

   buffer->ResetRects();

   if ( visible ) {
      if ( zoom > 1 && zoom < 17 )
         buffer->DrawRect(bmpBack->cx/2 - bmpBack->cx/zoom/2,
                          bmpBack->cy/2 - bmpBack->cy/zoom/2,
                          bmpBack->cx/2 + bmpBack->cx/zoom/2,
                          bmpBack->cy/2 + bmpBack->cy/zoom/2,
                          RGB_DARKGREEN);


      char tmp[8];
      sprintf(tmp,"%dx",zoom);
      int x = 34*buffer->cxCanvas/40 - (strlen(tmp)+1)/2*buffer->lAveCharWidth;
      int y = 9*buffer->cyCanvas/10 + buffer->lMaxBaselineExt/2;
      buffer->DrawString(tmp,x,y,RGB_GREEN,S_CANVASBUFFER_ADDRECTANGLE); 

      }

   if ( redrawAll ) {
      buffer->Show(visible);
      }

   return 0;
   }


int
SF_RadarConsole::SetVisible(int ivisible) {
   visible = ivisible;
   redrawAll = TRUE;
   buffer->Show(visible);
   return 0;
   }


int
SF_RadarConsole::DrawBackGround(int x1,int y1,int x2,int y2) {
   ((S_SystemBitmap *)bmpBack)->Blit(buffer,0,x1,y1,x2,y2,
                                     x1,bmpBack->cy-y2);
   buffer->AddRect(x1,y1,x2,y2);
   return 0;
   }


int
SF_RadarConsole::DrawBackGround(ulong flag) {
   buffer->BlitInRects(bmpBack,flag);
   return 0;
   }

SF_RadarConsole::~SF_RadarConsole() {
  if ( font ) {
     delete font;
     font = NULL;
     }
   if ( buffer ) {
      delete buffer;
      buffer = NULL;
      }
   }
