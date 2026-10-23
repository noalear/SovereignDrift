#include "S_OS.h"
#include "S_GSlider.h"
#include "S_GWindow.h"

S_GSlider::S_GSlider() {
   xLeft = 0;
   xRight = 0;
   yTop = 0;
   yBottom = 0;
   dragging = FALSE;
   x0 = y0 = dx = dy = 0;
   value = 0;
   minValue = 0;
   maxValue = 100;
   S_GWidget::Init();
   }


int
S_GSlider::Create(S_GWindow *iparent,int (*ipMsgHandler)(S_GSlider *),
                  char *filename,int ix,int iy,int icx,int icy) {
   S_GWidget::Create(iparent,(int (*)(S_GWidget *))ipMsgHandler,
                     filename,4,ix,iy,icx,icy);
   xLeft = x;
   xRight = x+width;
   yTop = y;
   yBottom = y + height;
   return 0;
   }


int
S_GSlider::Destroy() {
   S_GWidget::Destroy();
   return 0;
   }

S_GSlider::~S_GSlider() {
   Destroy();
   }


int
S_GSlider::SetDragBounds(int ixLeft,int iyTop,int ixRight,int iyBottom) {
   xLeft = ixLeft;
   xRight = ixRight;
   yTop = iyTop;
   yBottom = iyBottom;
   return 0;
   }


int
S_GSlider::Draw(int frame) {

   if ( parent && !parent->closing && height > 4 ) {

      int drawSliderDown = leftMouseButtonIsDown && mouseIsOver;

      /* *** Blit top 2 scan lines *** */
      Blit(parent,mouseIsOver+2*drawSliderDown,x,y,x+cx,y+2,0,cy-2);

      /* *** Blit the middle part *** */
      Blit(parent,mouseIsOver+2*drawSliderDown,
           x,y+2,x+cx,y+height-2,0,cy/2-height/2+2);

      /* *** Blit bottom 2 scan lines *** */
      Blit(parent,mouseIsOver+2*drawSliderDown,x,y+height-2,x+cx,y+height,0,0);
      }

   return 0;
   }


int
S_GSlider::SetRange(int iminValue,int imaxValue) {
   minValue = iminValue;
   maxValue = imaxValue;
   return 0;
   }


int
S_GSlider::SetValue(int ivalue) {
   value = ivalue;
   if ( value < minValue )
      value = minValue;
   if (  value > maxValue )
      value = maxValue;
   if ( maxValue != minValue )
      SetPosition(x,yTop+value*(yBottom-height-yTop)/(maxValue-minValue));
   return 0;
   }


int
S_GSlider::SetPosition(int ix,int iy) {
   x = ix;
   y = iy;
   if ( x < xLeft ) x = xLeft;
   if ( y < yTop ) y = yTop;
   if ( x+(int)width > xRight ) x = xRight - width;
   if ( y+(int)height > yBottom ) y = yBottom - height;
   value = 0;
   if ( (yBottom-height)-yTop != 0 )
      value = minValue + (maxValue*(y-yTop))/((yBottom-height)-yTop);
   wparent->Draw();
   Draw();
   return 0;
   }


int
S_GSlider::HandleMouseMsg() {
#  ifdef WIN32
      POINT pt;
      GetCursorPos(&pt);
#  endif
   switch ( parent->button ) {
      case 0 : {
         int oldMouseIsOver = mouseIsOver;
         mouseIsOver = Contains(parent->mx,parent->my);

#        ifdef WIN32
            if ( /*mouseIsOver &&*/ dragging ) {
               SetPosition(x0+pt.x-dx,y0+pt.y-dy);
               if ( pMsgHandler )
                  (*pMsgHandler)(this);
	       }
#        endif

         if ( oldMouseIsOver != mouseIsOver )
            Draw();

         return FALSE;
         }
      case 1 : {
         dragging = FALSE;
         if ( !parent->keyup && mouseIsOver ) {
            leftMouseButtonIsDown = TRUE;

#           ifdef WIN32
               dragging = TRUE;
               dx = pt.x;
               dy = pt.y;
               x0 = x;
               y0 = y;
#           endif

            Draw();
            return TRUE;
	    }
         if ( parent->keyup ) {
            if ( leftMouseButtonIsDown && mouseIsOver ) {
               leftMouseButtonIsDown = FALSE;
               Draw();
               if ( pMsgHandler )
                  (*pMsgHandler)(this);
	       }
            leftMouseButtonIsDown = FALSE;
            Draw();
	    }
	 }
      }

   return 0;
   }
