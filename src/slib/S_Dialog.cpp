#include "S_OS.h"
#include "S_Dialog.h"

S_Dialog::S_Dialog( S_Root *iroot,char *iwindowName,
                    int (*ipMsgHandler)(S_Dialog *),
                    int iconID, int ix, int iy, int icx, int icy,
                    ulong iflFrameFlags,void *ipuser ) :
   S_Window( iroot,NULL,iwindowName,
             (int (*)(S_Window *))ipMsgHandler,
             iconID,ix,iy,icx,icy,iflFrameFlags,ipuser) {
   created = TRUE;
   }

S_Dialog::S_Dialog( S_Object *iparent,char *iwindowName,
                    int (*ipMsgHandler)(S_Dialog *),
                    int iconID, int ix, int iy, int icx, int icy,
                    ulong iflFrameFlags,void *ipuser ) :
   S_Window( NULL,iparent,iwindowName,
             (int (*)(S_Window *))ipMsgHandler,
             iconID,ix,iy,icx,icy,iflFrameFlags,ipuser) {
   created = TRUE;
   }


int
S_Dialog::HandlePaintMsg(void) {
   DrawFilledRect(0,0,cxClient,cyClient,backGroundRGB);
   msg = S_WINDOW_PAINT;
   if ( pMsgHandler )
      (*pMsgHandler)((void *)this);
   return 0;
   }

S_Dialog::~S_Dialog() {
   }
