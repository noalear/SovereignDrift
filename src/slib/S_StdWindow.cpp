#include "S_OS.h"
#include "S_StdWindow.h"

S_StdWindow::S_StdWindow( S_Root *iroot,char *iwindowName,
                          int (*ipMsgHandler)(S_StdWindow *),
                          int iconID, int ix, int iy, int icx, int icy,
                          ulong iflFrameFlags,void *ipuser ) :
   S_Window( iroot,NULL,iwindowName,
             (int (*)(S_Window *))ipMsgHandler,
             iconID,ix,iy,icx,icy,iflFrameFlags,ipuser) {
   created = TRUE;
   }

S_StdWindow::S_StdWindow( S_Object *iparent,char *iwindowName,
                          int (*ipMsgHandler)(S_StdWindow *),
                          int iconID, int ix, int iy, int icx, int icy,
                          ulong iflFrameFlags,void *ipuser ) :
   S_Window( NULL,iparent,iwindowName,
             (int (*)(S_Window *))ipMsgHandler,
             iconID,ix,iy,icx,icy,iflFrameFlags,ipuser) {
   created = TRUE;
   }


S_StdWindow::~S_StdWindow() {
   }
