#include "S_OS.h"
#include "S_GScrollBar.h"

S_GScrollBar::S_GScrollBar() {
   options = 0;
   S_GWidget::Init();
   }


int
S_GScrollBar::Create(S_GWindow *iparent,int (*ipMsgHandler)(S_GScrollBar *),
                  char *filename,int ix,int iy,int icx,int icy,
                  int ioptions) {
   S_GWidget::Create(iparent,(int (*)(S_GWidget *))ipMsgHandler,
                     filename,1,ix,iy,icx,icy);
   options = ioptions;
   return 0;
   }


int
S_GScrollBar::CreateSlider(char *ifilename,
                           int (*ipMsgHandler)(S_GSlider *)) {
   slider.Create(parent,ipMsgHandler,ifilename,x+1,y+cx);
   slider.wparent = this;
   slider.height = height - 2*cx;
   slider.SetDragBounds(x+1,y+cx,x+cx,y+height-cx);
   slider.SetValue(0);
   parent->Refresh();
   return 0;
   }


int
S_GScrollBar::CreateUpButton(char *ifilename,
                             int (*ipMsgHandler)(S_GButton *)) {
   upButton.Create(parent,ipMsgHandler,ifilename,x,y);
   upButton.wparent = this;
   return 0;
   }


int
S_GScrollBar::CreateDownButton(char *ifilename,
                               int (*ipMsgHandler)(S_GButton *)) {
   downButton.Create(parent,ipMsgHandler,ifilename,x,y+height-cx);
   downButton.wparent = this;
   return 0;
   }


int
S_GScrollBar::Destroy() {
   S_GWidget::Destroy();
   return 0;
   }

S_GScrollBar::~S_GScrollBar() {
   Destroy();
   }


int
S_GScrollBar::HandleMouseMsg() {
   switch ( parent->button ) {
      case 0 : {
         mouseIsOver = Contains(parent->mx,parent->my);
         return FALSE;
         }
      case 1 : {
         if ( !parent->keyup && mouseIsOver ) {
            leftMouseButtonIsDown = TRUE;

            msg = 0;
            if ( parent->my < slider.y && parent->my > slider.yTop )
               msg = S_GSCROLLBAR_PAGEUP;
            if ( parent->my > slider.y+slider.height && 
                 parent->my < slider.yBottom )
               msg = S_GSCROLLBAR_PAGEDOWN;
            if ( pMsgHandler )
               (*pMsgHandler)(this);

            return TRUE;
	    }
	 }
      }

   return 0;
   }


int
S_GScrollBar::Draw(int frame) {
   if ( parent && !parent->closing )
      Blit(parent,frame,x,y+width,x+width,y+height-width,0,width);
   return 0;
   }
