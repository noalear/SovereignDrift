#include "S_OS.h"
#include "S_GProgressBar.h"

S_GProgressBar::S_GProgressBar() {
   percent = 0;
   margin = 2;
   S_GWidget::Init();
   }


int
S_GProgressBar::Create(S_GWindow *iparent,
                       int (*ipMsgHandler)(S_GProgressBar *),
                       char *filename,int ix,int iy,int icx,int icy) {
   S_GWidget::Create(iparent,(int (*)(S_GWidget *))ipMsgHandler,
                     filename,3,ix,iy,icx,icy);
   return rc;
   }


int
S_GProgressBar::Destroy() {
   S_GWidget::Destroy();
   return 0;
   }

S_GProgressBar::~S_GProgressBar() {
   Destroy();
   }


int
S_GProgressBar::Draw(int ipercent) {

   if ( parent && !parent->closing ) {

      if ( ipercent >= 0 && ipercent <= 100 )
         percent = ipercent;

      if ( percent <= 0 ) {
         Blit(parent,0,x,y,x+margin,y+height,0,0);
         Blit(parent,0,x+margin,y,x+width-margin,y+height,
              cx/2-width/2+margin,0);
         Blit(parent,0,x+width-margin,y,x+width,y+height,cx-margin,0);
         }
      else {
         int dx = (width*percent)/100;
         if ( width == (int)cx ) {
            Blit(parent,1,x+dx,y,x+cx,y+cy,dx,0);
            Blit(parent,2,x,y,x+dx,y+cy,0,0);
            }
         else {
            Blit(parent,2,x,y,x+margin,y+height,0,0);
            Blit(parent,2,x+margin,y,x+dx,y+cy,cx/2-width/2,0);
            Blit(parent,1,x+dx,y,x+width-margin,y+cy,cx/2-width/2+dx-margin,0);
            Blit(parent,1,x+width-margin,y,x+width,y+height,cx-margin,0);
            }
         }
      }
   return 0;
   }



