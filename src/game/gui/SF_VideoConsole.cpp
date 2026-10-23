#include "sf.h"
#include "SF_PortCpp.h"
#include "SF_Externals.h"
#include "SF_VideoConsole.h"
#include "SF_GameEngine.h"
#include "SF_Sprite.h"
#include "SF_SpriteList.h"
#include "SF_Globals.h"
#include "SF_ClientWindow.h"
#include "SF_Client.h"
#include "SF_Engine.h"
#include "SF_Button.h"
#include "SF_DisplayEngine.h"

/* ************************************************************************* */
/* ************ A class for managing the video console window ************** */
/* ************************************************************************* */
SF_VideoConsoleLine::SF_VideoConsoleLine() {
   }

int
SF_VideoConsoleLine::Create(SF_VideoConsole *iparent,int ix0,int ichx) {
   x0 = ix0;
   chx = ichx;
   type = SF_CONSOLELINE_TEXT;
   val1 = 0;
   val2 = 0;
   update = TRUE;
   textColor = RGB_GREEN;
   val1Color = RGB_GREEN;
   val2Color = RGB_GREEN;
   parent = iparent;
   return 0;
   }


int
SF_VideoConsoleLine::Draw(S_CanvasBuffer *buffer,int y0) {
   SF_VideoConsole *console = (SF_VideoConsole *)parent;
   if ( !update )
      return 0;
   switch ( type ) {
      case SF_CONSOLELINE_TEXT :
         console->DrawBackGround(x0,
                                 y0-buffer->lMaxAscender,
                                 x0+maxTextWidth,
                                 y0+buffer->lMaxDescender);
         buffer->DrawString(text,x0,y0,textColor,
                            S_CANVASBUFFER_ADDRECTANGLE);
         break;
      case SF_CONSOLELINE_TEXTGAUGE1 : {
         int dx = buffer->cxCanvas/6;
         int dy = buffer->lMaxAscender;
         int x = buffer->cxCanvas/3 + 2;
         console->DrawBackGround(x0,
                                 y0-buffer->lMaxAscender,
                                 buffer->cxCanvas/2,
                                 y0+buffer->lMaxDescender);
         buffer->DrawString(text,x0,y0,textColor,
                            S_CANVASBUFFER_ADDRECTANGLE);
         buffer->DrawFilledRoundedRect(x,y0-dy+4,x+dx,y0+1,dy/2+1,RGB_BLACK,
                                       S_CANVASBUFFER_ADDRECTANGLE);
         buffer->DrawFilledRoundedRect(x,y0-dy+4,x+dx*val1/100,y0+1,dy/2+1,
                                       val1Color);
         break;
         }
      case SF_CONSOLELINE_TEXTGAUGE2 : {
         int dx = 60;
         int dy = buffer->lMaxAscender;
         int x = 180;
         console->DrawBackGround(x0,
                                 y0-buffer->lMaxAscender,
                                 buffer->cxCanvas,
                                 y0+buffer->lMaxDescender);
         buffer->DrawString(text,x0,y0,textColor,
                            S_CANVASBUFFER_ADDRECTANGLE);
         buffer->DrawFilledRoundedRect(x,y0-dy+4,x+dx,y0+1,dy/2+1,RGB_BLACK,
                                       S_CANVASBUFFER_ADDRECTANGLE);
         buffer->DrawFilledRoundedRect(x + dx*(100-val1)/100,y0-dy+4,
                                       x+dx,y0+1,dy/2+1,val1Color);
                                       
         x += dx+2;
         buffer->DrawFilledRoundedRect(x,y0-dy+4,x+dx,y0+1,dy/2+1,RGB_BLACK,
                                       S_CANVASBUFFER_ADDRECTANGLE);
         buffer->DrawFilledRoundedRect(x,y0-dy+4,x+dx*val2/100,y0+1,dy/2+1,
                                       val2Color);
         break;
         }
      }
   update = FALSE;
   return 0;
   }


SF_VideoConsoleLine::~SF_VideoConsoleLine() {
   }


int
SF_VideoConsoleLine::Set(char *itext,ulong itextColor,int imaxTextWidth) {
   if ( strcmp(text,itext) == 0 && itextColor == textColor )
      return 0;
   strncpy(text,itext,chx);
   text[chx] = 0;
   textColor = itextColor;
   maxTextWidth = imaxTextWidth;
   update = TRUE;
   type = SF_CONSOLELINE_TEXT;
   return 0;
   }


int
SF_VideoConsoleLine::Set(char *itext,ulong itextColor,
                         int ival1,ulong ival1Color) {
   if ( strcmp(text,itext) == 0 && itextColor == textColor &&
        ival1 == val1 && ival1Color == val1Color )
      return 0;
   strncpy(text,itext,chx);
   text[chx] = 0;
   textColor = itextColor;
   val1 = ival1;
   if ( val1 > 100 ) val1 = 100;
   val1Color = ival1Color;
   update = TRUE;
   type = SF_CONSOLELINE_TEXTGAUGE1;
   return 0;
   }


int
SF_VideoConsoleLine::Set(char *itext,ulong itextColor,
                         int ival1,ulong ival1Color,
                         int ival2,ulong ival2Color) {
   if ( strcmp(text,itext) == 0 && itextColor == textColor &&
        ival1 == val1 && ival1Color == val1Color &&
        ival2 == val2 && ival2Color == val2Color )
      return 0;
   strncpy(text,itext,chx);
   text[chx] = 0;
   textColor = itextColor;
   val1 = ival1;
   if ( val1 > 100 ) val1 = 100;
   val1Color = ival1Color;
   val2 = ival2;
   if ( val2 > 100 ) val2 = 100;
   val2Color = ival2Color;
   update = TRUE;
   maxTextWidth = 0;
   type = SF_CONSOLELINE_TEXTGAUGE2;
   return 0;
   }

SF_VideoConsole::SF_VideoConsole(S_CanvasBuffer *iparentBuffer,
                                 SF_GameEngine *ige,
                                 int ix0,int iy0,int ichx,int ichy,
                                 int icyBuffer) {

   parentBuffer = iparentBuffer;
   bmpBack = NULL;
   check = 0xAAAAAAAA;

   ge = ige;
   x0 = ix0;
   y0 = iy0;
   cx = sf->cxVideoConsole;
   cy = sf->cyVideoConsole;
   chx = ichx;
   chy = ichy;
   cyBuffer = icyBuffer;
   redrawAll = TRUE;
   visible = TRUE;
   updateIcon = TRUE;
   bmpBack = NULL;

#  ifdef WIN32
      DWORD ver = GetVersion();
      if ( ver>>31 == 0 ) /* NT */
         font = new S_Font("Lucida Console",8,11,0,S_FONT_NORMAL);
      else /* Win95 */
         font = new S_Font("Courier New",8,15,0,S_FONT_NORMAL);
#  endif
#  ifdef OS2
      font = new S_Font(S_FONT_COURIER,8);
#  endif

   buffer = new S_CanvasBuffer(cx,cy,_root->bitsPerPixel);
   buffer->SetFont(font);
   parentBuffer->Embed(buffer,x0,y0);

   /*
   lines=(SF_VideoConsoleLine **)S_malloc(cyBuffer*sizeof(SF_VideoConsole *));
   */
   for ( int i=0; i<cyBuffer; i++ )
     {
      lines[i].Create(this,10,chx);
     }
   }


int
SF_VideoConsole::SetBmpBack(SF_Sprite *ibmpBack) {
   if ( ibmpBack ) {
      bmpBack = ibmpBack;
      cx = bmpBack->cx;
      cy = bmpBack->cy;
      }
   return 0;
   }


int
SF_VideoConsole::SetVisible(int ivisible) {
   visible = ivisible;
   redrawAll = TRUE;
   return 0;
   }

int ConsoleLineClicked(int);

extern short subMode;

int
SF_VideoConsole::SetMode(int imode) {
   mode = imode;
   redrawAll = TRUE;
   scroll = 0;
   numHeaderLines = 0;
   ConsoleLineClicked(0);
   Clear();
   return 0;
   }


int
SF_VideoConsole::SetNumHeaderLines(int inumHeaderLines) {
   numHeaderLines = inumHeaderLines;
   if ( scroll < numHeaderLines )
      scroll = numHeaderLines;
   return 0;
   }


int
SF_VideoConsole::ScrollUp() {
   scroll++;
   int lmax = max(cyBuffer - chy - numHeaderLines, 0);
   if ( scroll > lmax)
      scroll = lmax;
   redrawAll = TRUE;
   return 0;
   }


int
SF_VideoConsole::ScrollDown() {
   scroll--;
   if ( scroll < numHeaderLines )
      scroll = numHeaderLines;
   redrawAll = TRUE;
   return 0;
   }


int
SF_VideoConsole::ScrollHome() {
   scroll = numHeaderLines;
   redrawAll = TRUE;
   return 0;
   }


int
SF_VideoConsole::SetIcon(int iicon,int ix1,int iy1,int ix2,int iy2) {

   if ( iconX1 == ix1 && iconY1 == cy - iy2 && 
        iconX2 == ix2 && iconY2 == cy - iy1 &&
        icon == iicon )
      return 0;

   iconX1 = ix1;
   iconY1 = cy - iy2;
   iconX2 = ix2;
   iconY2 = cy - iy1;

   icon = iicon;
   updateIcon = TRUE;
   redrawAll = TRUE;

   return 0;
   }


int
SF_VideoConsole::Clear() {
   int i,j;
   for ( j=0; j<cyBuffer; j++ ) {
      for ( i=0; i<chx; i++ )
         lines[j].text[i] = ' ';
      lines[j].type = SF_CONSOLELINE_TEXT;
      lines[j].maxTextWidth = 0;
      }
   SetIcon();
   return 0;
   }


int
SF_VideoConsole::HandleMouseClick(int button,int x,int y,int keyup) {
   if ( button == 1 && !keyup ) {
      ConsoleLineClicked((int)((y - y0 - 18)/14.25));
      redrawAll = TRUE;
      }
   return 0;
   }


int
SF_VideoConsole::Contains(int x,int y) {
   return x>x0 && x<x0+buffer->cxCanvas && y>y0 && y<y0+buffer->cyCanvas;
   }


int
SF_VideoConsole::DrawBackGround(int x1,int y1,int x2,int y2) {
   if ( bmpBack ) {
      bmpBack->RequestDownload();
      ((S_SystemBitmap *)bmpBack)->Blit(buffer,0,x1,y1,x2,y2,
                                        x1,bmpBack->cy-y2);
      buffer->AddRect(x1,y1,x2,y2);
      }
   return 0;
   }


int
SF_VideoConsole::DrawBackGround() {
   if ( bmpBack ) {
      bmpBack->RequestDownload();
      buffer->BlitInRects(bmpBack,S_CANVASBUFFER_BLIT_ALL);
      /*bmpButtons->Blit(buffer,mode,0,buffer->cyCanvas-bmpButtons->cy);*/
      }
   return 0;
   }


int
SF_VideoConsole::DrawIcon() {
   if ( !updateIcon && !redrawAll )
      return 0;
   if ( buffer && icon >= 0) {
      SF_Sprite *spr = ge->sl->Sprite(icon);
      if ( spr ) {
         int x0 = iconX1;
         int y0 = iconY1;
         int cx0 = iconX2 - iconX1; 
         int cy0 = iconY2 - iconY1;
         x0 = cx/2 + 40;
         y0 = 20;
         cx0 = 96;
         cy0 = 96;
         if ( spr->schematic3D && !spr->schematic3D->rc )
           spr->schematic3D->Blit(buffer,0,x0,y0,x0+cx0,y0+cy0);
         else
           spr->StretchBlit(buffer,0,x0,y0,cx0,cy0);
         }
      }
   if ( icon < 0 )
      DrawBackGround(iconX1,iconY1,iconX2,iconY2);
   updateIcon = FALSE;
   return 0;
   }


int
SF_VideoConsole::SetLine(int i,char *text,ulong color,int maxTextWidth) {
   if ( maxTextWidth<0 )
      maxTextWidth = cx;
   if ( i>=0 && i<cyBuffer && lines )
      lines[i].Set(text,color,maxTextWidth);
   return 0;
   }


int
SF_VideoConsole::SetLine(int i,char *text,ulong textColor,
                         int val1,ulong val1Color) {
   if ( i>=0 && i<cyBuffer && lines )
      lines[i].Set(text,textColor,val1,val1Color);
   return 0;
   }


int
SF_VideoConsole::SetLine(int i,char *text,ulong textColor,
                         int val1,ulong val1Color,
                         int val2,ulong val2Color) {
   if ( i>=0 && i<cyBuffer && lines )
      lines[i].Set(text,textColor,
                    val1,val1Color,val2,val2Color);
   return 0;
   }


int
SF_VideoConsole::Draw() {
   if ( visible && redrawAll )
      DrawBackGround();

   int i;

   /* *** Draw the header lines *** */
   for ( i=0; i<numHeaderLines; i++ )
      if ( lines ) {
         if ( redrawAll )
            lines[i].update = TRUE;
         lines[i].Draw(buffer,18 + (int)14.25*i);
         }

   /* *** Draw the scrollable lines *** */
   for ( i=scroll; i<min(scroll+chy-numHeaderLines,cyBuffer); i++ )
      if ( lines ) {
         if ( redrawAll )
            lines[i].update = TRUE;
         lines[i].Draw(buffer,18 + (int)14.25*(i-scroll+numHeaderLines));
         }

   DrawIcon();
   if ( redrawAll ) {
      buffer->ResetRects();
      buffer->Show(visible);
      }
   redrawAll = FALSE;
   return 0;
   }

SF_VideoConsole::~SF_VideoConsole() {
  /*
  if ( lines ) {
     for ( int i=0; i<cyBuffer; i++ )
        if ( lines[i] ) {
           delete lines[i];
           lines[i] = NULL;
	   }
     S_free( lines );
     lines = NULL;
     }
  */
   if ( buffer ) {
      delete buffer;
      buffer = NULL;
      }
  if ( font ) {
     delete font;
     font = NULL;
     }
   }
/* ************************************************************************* */

