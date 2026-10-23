#include "S_OS.h"
#include "S_SplashWindow.h"
#include "S_SystemBitmap.h"
#include "S_Root.h"

int MsgHandler(S_SplashWindow *);

S_SplashWindow::S_SplashWindow(S_Root *iroot,char *iwindowName,
                               S_SystemBitmap *ibmp):
   S_Window( iroot,NULL,iwindowName,
             (int (*)(S_Window *))MsgHandler,0,
             iroot->cx/2 - iroot->cxSizeBorder - ibmp->cx/2,
             iroot->cy/2 - iroot->cySizeBorder - ibmp->cy/2,
             ibmp->cx + 2*iroot->cxSizeBorder,
             ibmp->cy + 2*iroot->cxSizeBorder,S_WINDOW_SPLASH) {
   bmp = ibmp;
   created = TRUE;
   Show();
   }

S_SplashWindow::S_SplashWindow(S_Object *iparent,char *iwindowName,
                               S_SystemBitmap *ibmp):
   S_Window( NULL,iparent,iwindowName,
             (int (*)(S_Window *))MsgHandler,0,
             iparent->root->cx/2 - iparent->root->cxSizeBorder - ibmp->cx/2,
             iparent->root->cy/2 - iparent->root->cySizeBorder - ibmp->cy/2,
             ibmp->cx + 2*iparent->root->cxSizeBorder,
             ibmp->cy + 2*iparent->root->cxSizeBorder,S_WINDOW_SPLASH) {
   bmp = ibmp;
   created = TRUE;
   Show();
   }

S_SplashWindow::~S_SplashWindow() {
   }


int
MsgHandler(S_SplashWindow *window) {
   switch( window->msg ) {
      case S_WINDOW_PAINT :
         window->bmp->Blit(window,0);
         break;
      }
   return 0;
   }
