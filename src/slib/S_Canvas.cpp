/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
#include "S_OS.h"
#include "S_Canvas.h"
#include "S_Font.h"


int
S_Canvas::Init() {
   oldFont = NULL;
   font = NULL;
   oldPalette = NULL;
   lpBuffer = NULL;
   hdc = NULL;
   ResetClipRect();
   return 0;
   }

S_Canvas::S_Canvas() {
   Init();
   }


int
S_Canvas::Create() {
   Init();
   return 0;
   }


int
S_Canvas::Destroy() {
#  ifdef WIN32
     if ( oldFont ) {
        if ( hdc )
           SelectObject(hdc,oldFont);
        oldFont = NULL;
        }
#  endif
   return 0;
   }

S_Canvas::~S_Canvas() {
   Destroy();
   }

int
S_Canvas::SetFgColor(slong color)
{
# ifdef OS2
    GpiSetColor(hdc,color);
# endif
# ifdef WIN32
    SetTextColor(hdc,RGB_INVERT(color));
# endif
  return 0;
}

int
S_Canvas::SetBgColor(slong color)
{
# ifdef OS2
    if ( color == -1 )
      {
	GpiSetBackColor(hdc,CLR_DEFAULT);
      }
    else
      {
	GpiSetBackColor(hdc,color);
      }
# endif
# ifdef WIN32
    if ( color == -1 )
      {
	SetBkMode(hdc,TRANSPARENT);
      }
    else
      {
	SetBkMode(hdc,OPAQUE);
	SetBkColor(hdc,RGB_INVERT(color));
      }
# endif
  return 0;
}



int
S_Canvas::ResetClipRect() {
   clipRect.xLeft = 0;
   clipRect.xRight = cxCanvas;
   clipRect.yTop = 0;
   clipRect.yBottom = cyCanvas;
#  ifdef OS2
      HRGN hrgnOld;
      GpiSetClipRegion(hdc,NULL,&hrgnOld);
      if ( hrgnOld && hdc )
         GpiDestroyRegion(hdc,hrgnOld);
#  endif
#  ifdef WIN32
      if ( hdc )
         SelectClipRgn(hdc,NULL);
#  endif
   return 0;
   }


int
S_Canvas::DrawPieWedge(int x0,int y0,int rx,int ry,
                       int angle1,int angle2,ulong color) {
#  ifdef OS2
      POINTL ptl = {x0,cyCanvas-y0};
      ARCPARAMS arcp = {rx,ry,0,0};
      GpiSetArcParams(hdc,&arcp);
      GpiMove(hdc,&ptl);
      GpiSetColor(hdc,color);
      GpiPartialArc(hdc,&ptl,MAKEFIXED(1,0),MAKEFIXED(angle1,0),
                    MAKEFIXED(angle2-angle1,0));
      GpiLine(hdc,&ptl);
#  endif
#  ifdef WIN32
      HPEN hPen = CreatePen(PS_SOLID,1,RGB_INVERT(color));
      HPEN hOldPen = (HPEN)SelectObject(hdc,hPen);
      double theta1 = 2*3.1415926*angle1/360.0;
      double theta2 = 2*3.1415926*angle2/360.0;
      Arc(hdc,
          x0-rx,y0-ry,x0+rx,y0+ry,
          x0+(int)(rx*cos(theta1)),y0-(int)(ry*sin(theta1)),
          x0+(int)(rx*cos(theta2)),y0-(int)(ry*sin(theta2)));
      MoveToEx(hdc,x0+(int)(rx*cos(theta1)),y0-(int)(ry*sin(theta1)),NULL);
      LineTo(hdc,x0,y0);
      LineTo(hdc,x0+(int)(rx*cos(theta2)),y0-(int)(ry*sin(theta2)));
      SelectObject(hdc,hOldPen);
      DeleteObject(hPen);
#  endif
   return 0;
   }


int
S_Canvas::DrawFilledPieWedge(int x0,int y0,int rx,int ry,
                             int angle1,int angle2,ulong color) {
#  ifdef OS2
      POINTL ptl = {x0,cyCanvas-y0};
      ARCPARAMS arcp = {rx,ry,0,0};
      GpiSetArcParams(hdc,&arcp);
      GpiMove(hdc,&ptl);
      GpiBeginPath(hdc,1);
      GpiSetColor(hdc,color);
      GpiPartialArc(hdc,&ptl,MAKEFIXED(1,0),MAKEFIXED(angle1,0),
                          MAKEFIXED(angle2-angle1,0));
      GpiLine(hdc,&ptl);
      GpiEndPath(hdc);
      GpiFillPath(hdc,1,0);
#  endif
#  ifdef WIN32
      HPEN hPen = CreatePen(PS_SOLID,1,RGB_INVERT(color));
      HPEN hOldPen = (HPEN)SelectObject(hdc,hPen);
      HBRUSH aBrush = CreateSolidBrush(RGB_INVERT(color));
      HBRUSH oldBrush = (HBRUSH) SelectObject(hdc,(HGDIOBJ)aBrush);
      double theta1 = 2*3.1415926*angle1/360.0;
      double theta2 = 2*3.1415926*angle2/360.0;
      Pie(hdc,
          x0-rx,y0-ry,x0+rx,y0+ry,
          x0+(int)(rx*cos(theta1)),y0-(int)(ry*sin(theta1)),
          x0+(int)(rx*cos(theta2)),y0-(int)(ry*sin(theta2)));
      SelectObject(hdc,(HGDIOBJ)oldBrush);
      DeleteObject((HGDIOBJ)aBrush);
      SelectObject(hdc,hOldPen);
      DeleteObject(hPen);
#  endif
   return 0;
   }


int
S_Canvas::SetClipRect(int x1,int y1,int x2,int y2) {
   clipRect.xLeft = x1;
   clipRect.xRight = x2;
   clipRect.yTop = y1;
   clipRect.yBottom = y2;
#  ifdef OS2
      RECTL rcl = {x1,cyCanvas-y2,x2,cyCanvas-y1};
      GpiIntersectClipRectangle(hdc,&rcl);
#  endif
#  ifdef WIN32
      IntersectClipRect(hdc,x1,y1,x2,y2);
#  endif
   return 0;
   }


int
S_Canvas::SetAntiClipRect(int x1,int y1,int x2,int y2) {
#  ifdef OS2
      RECTL rcl = {x1,cyCanvas-y2,x2,cyCanvas-y1};
      GpiExcludeClipRectangle(hdc,&rcl);
#  endif
#  ifdef WIN32
      ExcludeClipRect(hdc,x1,y1,x2,y2);
#  endif
   return 0;
   }


int
S_Canvas::DrawPoint(int x,int y,ulong color) {
#  ifdef OS2
      POINTL ptl;
#  endif
#  ifdef WIN32
      HDC hdcTmp = hdc;
      if ( lpBuffer )
         lpBuffer->GetDC(&hdcTmp);
      SetPixel(hdcTmp,xCanvas+x,yCanvas+y,RGB_INVERT(color));
      if ( lpBuffer )
         lpBuffer->ReleaseDC(hdcTmp);
#  endif
   return 0;
   }


int
S_Canvas::DrawLine(int x1,int y1, int x2,int y2,int width,ulong color) {
#  ifdef OS2
      POINTL ptl;
      GpiSetColor(hdc,color);
      ptl.x = x1;
      ptl.y = cyCanvas - y1 - 1;
      GpiMove(hdc,&ptl);
      ptl.x = x2;
      ptl.y = cyCanvas - y2 - 1;
      GpiLine(hdc,&ptl);
#  endif
#  ifdef WIN32
      HDC hdcTmp = hdc;
      if ( lpBuffer )
         lpBuffer->GetDC(&hdcTmp);
      HPEN hPen = CreatePen(PS_SOLID,width,RGB_INVERT(color));
      HPEN hOldPen = (HPEN)SelectObject(hdcTmp,hPen);
      MoveToEx(hdcTmp,xCanvas+x1,yCanvas+y1,NULL);
      LineTo(hdcTmp,xCanvas+x2,yCanvas+y2);
      SelectObject(hdcTmp,hOldPen);
      DeleteObject(hPen);
      if ( lpBuffer )
         lpBuffer->ReleaseDC(hdcTmp);
#  endif
   return 0;
   }


int
S_Canvas::DrawFilledRect(int x1,int y1,int x2,int y2,ulong color) {
#  ifdef OS2
      RECTL rcl;
      rcl.xLeft = x1;
      rcl.xRight = x2 + 1;
      rcl.yTop = cyCanvas - y1;
      rcl.yBottom = cyCanvas - y2 - 1;
      WinFillRect(hdc,&rcl,color);
#  endif
#  ifdef WIN32
      HDC hdcTmp = hdc;
      if ( lpBuffer )
         lpBuffer->GetDC(&hdcTmp);

      RECT rcl;
      rcl.left = xCanvas+x1;
      rcl.right = xCanvas+x2 + 1;
      rcl.top = yCanvas+y1;
      rcl.bottom = yCanvas+y2 + 1;
      HBRUSH hBr = CreateSolidBrush(RGB_INVERT(color));
      FillRect(hdcTmp, &rcl, hBr);
      DeleteObject(hBr);
      if ( lpBuffer )
         lpBuffer->ReleaseDC(hdcTmp);
#  endif
   return 0;
   }


int
S_Canvas::DrawCircle( int x, int y, int rad, ulong color ) {
#  ifdef OS2
      GpiSetColor(hdc,color);
      POINTL ptl = {x,cyCanvas - y - 1};
      ARCPARAMS arcp = {rad,rad,0,0};
      GpiSetArcParams(hdc,&arcp);
      GpiMove(hdc,&ptl);
      GpiFullArc(hdc,DRO_OUTLINE,MAKEFIXED(1,0));
#  endif
#  ifdef WIN32
      HDC hdcTmp = hdc;
      if ( lpBuffer )
         lpBuffer->GetDC(&hdcTmp);
      HPEN hPen = CreatePen(PS_SOLID,1,RGB_INVERT(color));
      HPEN hOldPen = (HPEN)SelectObject(hdcTmp,hPen);
      Arc(hdcTmp,x-rad,y-rad,x+rad,y+rad,x-rad,y,x-rad,y);
      SelectObject(hdcTmp,hOldPen);
      DeleteObject(hPen);
      if ( lpBuffer )
         lpBuffer->ReleaseDC(hdcTmp);
#  endif
   return 0;
   }


int
S_Canvas::DrawFilledCircle( int x, int y, int rad, ulong color ) {
#  ifdef OS2
      GpiSetColor(hdc,color);
      POINTL ptl = {x,cyCanvas - y - 1};
      ARCPARAMS arcp = {rad,rad,0,0};
      GpiSetArcParams(hdc,&arcp);
      GpiMove(hdc,&ptl);
      GpiFullArc(hdc,DRO_FILL,MAKEFIXED(1,0));
#  endif
#  ifdef WIN32
      HDC hdcTmp = hdc;
      if ( lpBuffer )
         lpBuffer->GetDC(&hdcTmp);
      HPEN hPen = CreatePen(PS_SOLID,1,RGB_INVERT(color));
      HPEN hOldPen = (HPEN)SelectObject(hdcTmp,hPen);
      HBRUSH aBrush = CreateSolidBrush(RGB_INVERT(color));
      HBRUSH oldBrush = (HBRUSH) SelectObject(hdcTmp,(HGDIOBJ)aBrush);
      Ellipse(hdcTmp,x-rad,y-rad,x+rad+1,y+rad+1); /*Not work with DD*/
      /*RoundRect(hdcTmp,xCanvas+x-rad,yCanvas+y-rad,
                         xCanvas+x+rad+1,yCanvas+y+rad+1,2*rad,2*rad);*/
      SelectObject(hdcTmp,(HGDIOBJ)oldBrush);
      DeleteObject((HGDIOBJ)aBrush);
      SelectObject(hdcTmp,hOldPen);
      DeleteObject(hPen);
      if ( lpBuffer )
         lpBuffer->ReleaseDC(hdcTmp);
#  endif
   return 0;
   }



int
S_Canvas::DrawTriangle(int x0,int y0,int x1,int y1,int x2,int y2,ulong color) {
#  ifdef OS2
      GpiSetColor(hdc,color);
      POINTL ptl = {x0,cyCanvas - y0 - 1};
      GpiMove(hdc,&ptl);
      ptl.x = x1; ptl.y = cyCanvas - y1 - 1;
      GpiLine(hdc,&ptl);
      ptl.x = x2; ptl.y = cyCanvas - y2 - 1;
      GpiLine(hdc,&ptl);
      ptl.x = x0; ptl.y = cyCanvas - y0 - 1;
      GpiLine(hdc,&ptl);
#  endif
#  ifdef WIN32
      HDC hdcTmp = hdc;
      if ( lpBuffer )
         lpBuffer->GetDC(&hdcTmp);
      HPEN hPen = CreatePen(PS_SOLID,1,RGB_INVERT(color));
      HPEN hOldPen = (HPEN)SelectObject(hdcTmp,hPen);
      MoveToEx(hdcTmp,xCanvas+x0,yCanvas+y0,NULL);
      LineTo(hdcTmp,xCanvas+x1,yCanvas+y1);
      LineTo(hdcTmp,xCanvas+x2,yCanvas+y2);
      LineTo(hdcTmp,xCanvas+x0,yCanvas+y0);
      SelectObject(hdcTmp,hOldPen);
      DeleteObject(hPen);
      if ( lpBuffer )
         lpBuffer->ReleaseDC(hdcTmp);
#  endif
   return 0;
   }


int
S_Canvas::DrawRect(int x1,int y1,int x2,int y2,ulong color) {
#  ifdef OS2
      GpiSetColor(hdc,color);
      POINTL ptl;
      ptl.x = x1;  ptl.y = cyCanvas - y1 - 1;
      GpiMove(hdc,&ptl);
      ptl.x = x2;
      GpiLine(hdc,&ptl);
      ptl.y = cyCanvas - y2 - 1;
      GpiLine(hdc,&ptl);
      ptl.x = x1;
      GpiLine(hdc,&ptl);
      ptl.y = cyCanvas - y1 - 1;
      GpiLine(hdc,&ptl);
#  endif
#  ifdef WIN32
      HDC hdcTmp = hdc;
      if ( lpBuffer )
         lpBuffer->GetDC(&hdcTmp);
      HPEN hPen = CreatePen(PS_SOLID,1,RGB_INVERT(color));
      HPEN hOldPen = (HPEN)SelectObject(hdcTmp,hPen);
      MoveToEx(hdcTmp,xCanvas+x1,yCanvas+y1,NULL);
      LineTo(hdcTmp,xCanvas+x2,yCanvas+y1);
      LineTo(hdcTmp,xCanvas+x2,yCanvas+y2);
      LineTo(hdcTmp,xCanvas+x1,yCanvas+y2);
      LineTo(hdcTmp,xCanvas+x1,yCanvas+y1);
      SelectObject(hdcTmp,hOldPen);
      DeleteObject(hPen);
      if ( lpBuffer )
         lpBuffer->ReleaseDC(hdcTmp);
#  endif
   return 0;
   }


int
S_Canvas::DrawFilledRoundedRect(int x1,int y1,int x2,int y2,
                                int rad,ulong color){

#  if defined(OS2) || defined(WIN32)
   int dx = x2 - x1;
   int dy = y2 - y1;
#  endif

#  ifdef OS2
      if ( dx > dy ) {
         POINTL ptl = {x1,cyCanvas-y2-1};
         GpiMove(hdc,&ptl);
         ptl.x = x2;
         ptl.y = cyCanvas - y1 - 1;
         GpiSetColor(hdc,color);
         GpiBox(hdc,DRO_OUTLINEFILL,&ptl,rad,rad);
         }
#  endif
#  ifdef WIN32
      if ( dx > dy ) {
         HDC hdcTmp = hdc;
         if ( lpBuffer )
            lpBuffer->GetDC(&hdcTmp);
         HPEN hPen = CreatePen(PS_SOLID,1,RGB_INVERT(color));
         HPEN hOldPen = (HPEN)SelectObject(hdcTmp,hPen);
         HBRUSH aBrush = CreateSolidBrush(RGB_INVERT(color));
         HBRUSH oldBrush = (HBRUSH) SelectObject(hdcTmp,(HGDIOBJ)aBrush);
         RoundRect(hdcTmp,xCanvas+x1,yCanvas+y1,
                   xCanvas+x2+1,yCanvas+y2+1,rad,rad);
	 SelectObject(hdcTmp,(HGDIOBJ)oldBrush);
         DeleteObject(aBrush);
         SelectObject(hdcTmp,hOldPen);
         DeleteObject(hPen);
         if ( lpBuffer )
            lpBuffer->ReleaseDC(hdcTmp);
	 }
#  endif
   return 0;
   }


int
S_Canvas::DrawRoundedRect(int,int,int,int,int,ulong) {
   return 0;
   }


int
S_Canvas::SetFont( S_Font *ifont ) {
#  ifdef OS2
      if ( font && ifont )
         int rc = GpiDeleteSetId(hdc,font->id);
#  endif
   if ( ifont )
      font = ifont;
#  ifdef OS2
      if ( font ) {
         FATTRS fat;
         fat.usRecordLength = sizeof(FATTRS);
         fat.fsSelection = 0;
         fat.lMatch = 0;
         strcpy(fat.szFacename,font->faceName);
         fat.idRegistry = 0;
         fat.usCodePage = 0;
         fat.lMaxBaselineExt = 0;
         fat.lAveCharWidth = 0;
         fat.fsType = 0;
         fat.fsFontUse = FATTR_FONTUSE_NOMIX | FATTR_FONTUSE_OUTLINE | 
                         FATTR_FONTUSE_TRANSFORMABLE;
         int rc = GpiCreateLogFont(hdc,NULL,font->id,&fat);

         SIZEF fs;
         GRADIENTL gtl = {0,0};
         rc = GpiSetCharSet(hdc,font->id);
         rc = GpiSetCharMode(hdc,CM_MODE3);
         fs.cx = MAKEFIXED(font->cx,0);
         fs.cy = MAKEFIXED(font->cy,0);
         rc = GpiSetCharBox(hdc,&fs);
         rc = GpiSetCharAngle(hdc,&gtl);

         FONTMETRICS fm;
         GpiQueryFontMetrics (hdc, sizeof fm, &fm) ;
         lAveCharWidth = fm.lAveCharWidth;
         lMaxBaselineExt = fm.lMaxBaselineExt;
         lMaxDescender = fm.lMaxDescender;
         lMaxAscender = fm.lMaxAscender;
         }
#  endif

#  ifdef WIN32
      if ( oldFont )
         SelectObject(hdc,oldFont);
      if ( font ) {
         oldFont = (HFONT)SelectObject(hdc,font->hFont);

         TEXTMETRIC tm;
         GetTextMetrics(hdc,&tm);
         lAveCharWidth = tm.tmAveCharWidth;
         lMaxBaselineExt = tm.tmHeight;
         lMaxDescender = tm.tmDescent;
         lMaxAscender = tm.tmAscent;
         }
#  endif
   return 0;
   }


SizeStruct 
S_Canvas::GetStringSize(char *str) {
   SizeStruct size = {0,0};
#  ifdef OS2
      POINTL aptl[TXTBOX_COUNT];
      GpiQueryTextBox(hdc,strlen(str),str,TXTBOX_COUNT,(POINTL *)&aptl);
      size.cx = aptl[TXTBOX_BOTTOMRIGHT].x - aptl[TXTBOX_BOTTOMLEFT].x;
      size.cy = aptl[TXTBOX_TOPRIGHT].y    - aptl[TXTBOX_BOTTOMRIGHT].y;
#  endif
#  ifdef WIN32
      SIZE sz;
      GetTextExtentPoint32(hdc,str,strlen(str),&sz);
      size.cx = sz.cx;
      size.cy = sz.cy;
#  endif
   return size;
   }


SizeStruct 
S_Canvas::DrawString(char *str,int x,int y,ulong color) {
   SizeStruct size = {0,0};
   if ( str ) {
#  ifdef OS2
      POINTL ptl = {x,cyCanvas-y-1},aptl[TXTBOX_COUNT];
      int rc = GpiSetColor(hdc,color);
      rc = GpiCharStringAt(hdc,&ptl,strlen(str),str);
      rc = GpiQueryTextBox(hdc,strlen(str),str,TXTBOX_COUNT,(POINTL *)&aptl);
      size.cx = aptl[TXTBOX_BOTTOMRIGHT].x - aptl[TXTBOX_BOTTOMLEFT].x;
      size.cy = aptl[TXTBOX_TOPRIGHT].y    - aptl[TXTBOX_BOTTOMRIGHT].y;
#  endif
#  ifdef WIN32
      HDC hdcTmp = hdc;
      HFONT oldFont = NULL;
      if ( lpBuffer ) {
         lpBuffer->GetDC(&hdcTmp);
         SetBkMode(hdcTmp,TRANSPARENT);
         SetTextAlign(hdcTmp,TA_BASELINE);
         if ( font )
            oldFont = (HFONT)SelectObject(hdcTmp,font->hFont);
         }
      SIZE sz;
      SetTextColor(hdcTmp,RGB_INVERT(color));
      TextOut(hdcTmp,xCanvas+x,yCanvas+y,str,strlen(str));
      GetTextExtentPoint32(hdcTmp,str,strlen(str),&sz);
      size.cx = sz.cx;
      size.cy = sz.cy;
      if ( lpBuffer ) {
         if ( oldFont )
            SelectObject(hdcTmp,oldFont);
         lpBuffer->ReleaseDC(hdcTmp);
         }
#  endif
   }
   return size;
   }

int _DDPrintErrorMsg(int);



int
S_Canvas::SystemBlit(S_Canvas *target,
                     int x1targ,int y1targ,int x2targ,int y2targ,
                     int x1src,int y1src, ulong rop ) {

#  ifdef OS2
      POINTL aptl[3] = {x1targ,target->cyCanvas-y2targ,
                        x2targ,target->cyCanvas-y1targ,
                        x1src,cyCanvas-y1src-(y2targ-y1targ)};
      if ( target->hdc && hdc )
         GpiBitBlt(target->hdc,hdc,3,aptl,rop,BBO_IGNORE);
#  endif
#  ifdef WIN32
      if ( target->lpBuffer && lpBuffer ) {
         RECT rclDest = {target->xCanvas+x1targ,
			 target->yCanvas+y1targ,
			 target->xCanvas+x2targ,
			 target->yCanvas+y2targ};
         RECT rclSrc  = {x1src,y1src,
                         x1src+(x2targ-x1targ),
                         y1src+(y2targ-y1targ)};

         DDBLTFX ddBltFx;
         ddBltFx.dwSize = sizeof(DDBLTFX);
         ddBltFx.dwROP = rop;
         int rc = target->lpBuffer->Blt(&rclDest,lpBuffer,&rclSrc,
                                        DDBLT_ROP|DDBLT_WAIT,&ddBltFx);
         _DDPrintErrorMsg(rc);
         }
      else {
         if ( target->hdc && hdc )
            BitBlt(target->hdc,x1targ,y1targ,x2targ-x1targ,y2targ-y1targ,
                           hdc,x1src,y1src,rop);
         }
#  endif
   return 0;
   }


int
S_Canvas::SystemStretchBlit(S_Canvas *target,
                            int x1targ,int y1targ,int x2targ,int y2targ,
                            int x1src,int y1src,int x2src,int y2src,
                            ulong rop ) {

#  ifdef OS2
      POINTL aptl[4] = {x1targ,target->cyCanvas-y2targ,
                        x2targ,target->cyCanvas-y1targ,
                        x1src,cyCanvas-y2src,x2src,cyCanvas-y1src};
      if ( target->hdc && hdc )
         GpiBitBlt(target->hdc,hdc,4,aptl,rop,BBO_IGNORE);
#  endif
#  ifdef WIN32
      if ( target->lpBuffer && lpBuffer ) {
         RECT rclDest = {target->xCanvas+x1targ,
                         target->yCanvas+y1targ,
                         target->xCanvas+x2targ,
                         target->yCanvas+y2targ};
         RECT rclSrc  = {x1src,y1src,x2src,y2src};
         DDBLTFX ddBltFx;
         ddBltFx.dwSize = sizeof(DDBLTFX);
         ddBltFx.dwROP = rop;
         target->lpBuffer->Blt(&rclDest,lpBuffer,&rclSrc,
                               DDBLT_ROP|DDBLT_WAIT,&ddBltFx);
         }
      else {
         if ( target->hdc && hdc )
            StretchBlt(target->hdc,x1targ,y1targ,x2targ-x1targ,y2targ-y1targ,
                               hdc,x1src,y1src,x2src-x1src,y2src-y1src,rop);
         }
#  endif
   return 0;
   }

#ifdef WIN32
_DDPrintErrorMsg(int rc) {
   switch(rc) {
      case DDERR_GENERIC :
         printf("DDERR_GENERIC\n");
         break;
      case DDERR_INVALIDCLIPLIST :
         printf("DDERR_INVALIDCLIPLIST\n");
         break;
      case DDERR_INVALIDRECT :
         printf("DDERR_INVALIDRECT\n");
         break;
      case DDERR_NOALPHAHW :
         printf("DDERR_NOALPHAHW\n");
         break;
      case DDERR_NOBLTHW :
         printf("DDERR_NOBLTHW\n");
         break;
      case DDERR_NOCLIPLIST :
         printf("DDERR_NOCLIPLIST\n");
         break;
      case DDERR_NODDROPSHW :
         printf("DDERR_NODDROPSHW\n");
         break;
      case DDERR_NOMIRRORHW :
         printf("DDERR_NOMIRRORHW\n");
         break;
      case DDERR_NORASTEROPHW :
         printf("DDERR_NORASTEROPHW\n");
         break;
      case DDERR_NOROTATIONHW :
         printf("DDERR_NOROTATIONHW\n");
         break;
      case DDERR_NOSTRETCHHW :
         printf("DDERR_NOSTRETCHHW\n");
         break;
      case DDERR_NOZBUFFERHW :
         printf("DDERR_NOZBUFFERHW\n");
         break;
      case DDERR_IMPLICITLYCREATED :
         printf("DDERR_IMPLICITLYCREATED\n");
         break;
      case DDERR_INCOMPATIBLEPRIMARY :
         printf("DDERR_INCOMPATIBLEPRIMARY\n");
         break;
      case DDERR_NOEXCLUSIVEMODE :
         printf("DDERR_NOEXCLUSIVEMODE\n");
         break;
      case DDERR_UNSUPPORTED :
         printf("DDERR_UNSUPPORTED\n");
         break;
      case DDERR_UNSUPPORTEDMODE :
         printf("DDERR_UNSUPPORTEDMODE\n");
         break;
      case DDERR_INVALIDMODE :
         printf("DDERR_INVALIDMODE\n");
         break;
      case DDERR_LOCKEDSURFACES :
         printf("DDERR_LOCKEDSURFACES\n");
         break;
      case DDERR_WRONGMODE :
         printf("DDERR_WRONGMODE\n");
         break;
      case DDERR_INVALIDOBJECT :
         printf("DDERR_INVALIDOBJECT\n");
         break;
      case DDERR_INVALIDPARAMS :
         printf("DDERR_INVALIDPARAMS\n");
         break;
      case DDERR_OUTOFMEMORY :
         printf("DDERR_OUTOFMEMORY\n");
         break;
      case DDERR_SURFACEBUSY :
         printf("DDERR_SURFACEBUSY\n");
         break;
      case DDERR_SURFACELOST :
         printf("DDERR_SURFACELOST\n");
         break;
      case DDERR_WASSTILLDRAWING :
         printf("DDERR_WASSTILLDRAWING\n");
         break;
      }
   return 0;
   }
#endif

void *
S_Canvas::Lock() {
   void *ptr = NULL;
#  ifdef WIN32
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
      }
   ptr = ddsd.lpSurface;
#  endif
   return ptr;
   }


int
S_Canvas::Unlock() {
#  ifdef WIN32
      lpBuffer->Unlock(NULL);
#  endif 
   return 0;
   }


int
S_Canvas::Flip() {
#  ifdef WIN32
#  endif
   return 0;
   }


int
S_Canvas::Capture(char *filename,int xLeft,int yTop,int cxCap,int cyCap)
{

  // This function only works for 24-bit (truecolor) display mode

  if ( cxCap == 0 )
    {
      cxCap = cxCanvas;
    }  

  if ( cyCap == 0 )
    {
      cyCap = cyCanvas;
    }  

  //cxCap = cxCanvas;
  //cyCap = cyCanvas;

  int cxCapWord = cxCap*3;
  if ( cxCapWord%4 > 0 )
    {
      cxCapWord += 4 - (cxCap*3 % 4);
    }

# ifdef WIN32

  // prepare DCs, bitmaps,..
  HDC hmemDC = CreateCompatibleDC(hdc);   
  HBITMAP hmemBM = CreateCompatibleBitmap(hdc, cxCap, cyCap);
  SelectObject(hmemDC, hmemBM);
  
  // copy screen to memory DC
  BitBlt(hmemDC, 0, 0, cxCap, cyCap, hdc, xLeft, yTop, SRCCOPY);
  
  // allocate and lock memory for the bitmap data
  HGLOBAL hpxldata = GlobalAlloc(GMEM_FIXED, cxCapWord * cyCap);
  void FAR* lpvpxldata = GlobalLock(hpxldata);
  
  // fill .bmp - structures
  BITMAPINFO bmInfo;
  bmInfo.bmiHeader.biSize = 40;
  bmInfo.bmiHeader.biWidth = cxCap;
  bmInfo.bmiHeader.biHeight = cyCap;
  bmInfo.bmiHeader.biPlanes = 1;
  bmInfo.bmiHeader.biBitCount = 24;
  bmInfo.bmiHeader.biCompression = 0;
  bmInfo.bmiHeader.biSizeImage = 0;
  bmInfo.bmiHeader.biXPelsPerMeter = 0;
  bmInfo.bmiHeader.biYPelsPerMeter = 0;
  bmInfo.bmiHeader.biClrUsed = 0;
  bmInfo.bmiHeader.biClrImportant = 0;
  
  BITMAPFILEHEADER bmFileHeader;
  bmFileHeader.bfType = 19778;
  bmFileHeader.bfSize = (cxCapWord * cyCap ) + 40 + 14;
  bmFileHeader.bfReserved1 = 0;
  bmFileHeader.bfReserved2 = 0;
  bmFileHeader.bfOffBits = 54;
  
  // copy bitmap data into global memory
  GetDIBits(hmemDC, hmemBM, 0, cyCap, lpvpxldata, &bmInfo, DIB_RGB_COLORS);
  
  // open file and write data
  int bmfile = open(filename, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY);
  write(bmfile, &bmFileHeader, 14);
  write(bmfile, &bmInfo, 40);
  write(bmfile, lpvpxldata, cxCapWord * cyCap );
  
  // clean up
  close(bmfile);
  GlobalUnlock(hpxldata);
  GlobalFree(hpxldata);
  DeleteObject(hmemBM);
  DeleteDC(hmemDC);

  _OsSetFileReadOnly(filename,FALSE);

# endif
  
  return 0;
}
