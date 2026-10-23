#include "S_OS.h"
#include "S_GButton.h"
#include "S_GWindow.h"

S_GButton::S_GButton() {
   S_GWidget::Init();
   }


int
S_GButton::Create(S_GWindow *iparent,int (*ipMsgHandler)(S_GButton *),
                  char *filename,int ix,int iy,int icx,int icy) {
   S_GWidget::Create(iparent,(int (*)(S_GWidget *))ipMsgHandler,
                     filename,4,ix,iy,icx,icy);
   return 0;
   }


int
S_GButton::Destroy() {
   S_GWidget::Destroy();
   return 0;
   }

S_GButton::~S_GButton() {
   Destroy();
   }


int
S_GButton::Draw(int frame) {
   int drawButtonDown = (leftMouseButtonIsDown && mouseIsOver && enabled) ||
                         toggle;
   S_GWidget::Draw(mouseIsOver+2*drawButtonDown);
   return 0;
   }


int
S_GButton::HandleMouseMsg() {
   int oldMouseIsOver = mouseIsOver;
   mouseIsOver = Contains(parent->mx,parent->my) && enabled;
   switch ( parent->button ) {
      case 0 :
         if ( oldMouseIsOver != mouseIsOver )
            Draw();
         return FALSE;

      case 1 :
         if ( !parent->keyup && mouseIsOver || parent->repeat ) {
            leftMouseButtonIsDown = TRUE;
            Draw();
            return TRUE;
	    }
         if ( parent->keyup && !parent->repeat ) {
            if ( leftMouseButtonIsDown && mouseIsOver ) {
               leftMouseButtonIsDown = FALSE;
               if ( pMsgHandler )
                  (*pMsgHandler)(this);
               Draw();
               mouseIsOver = FALSE;
               return TRUE;
	       }
            mouseIsOver = FALSE;
            leftMouseButtonIsDown = FALSE;
	    }
         Draw();
	 return FALSE;

      }
         

   return 0;
   }
