#include "S_OS.h"
#include "S_GWindow.h"
#include "S_ZArchive.h"
#include "S_Root.h"
#include "S_GWidget.h"


int
S_GWindow::Init() {
   draggable = TRUE;
   dragging = FALSE;
   x0 = y0 = dx = dy = 0;
   return 0;
   }

S_GWindow::S_GWindow() {
   Init();
   }


int
S_GWindow::Create(S_Root *iroot,S_GWindow *iparent,char *ititle,
                  char *bmpFilename,int inumFrames,
                  int ix,int iy,int icx,int icy,ulong iflFrameFlags) {

   S_SystemBitmap::ZCreate(bmpFilename,
                           S_BITMAP_MASK_NONE,S_BITMAP_BLIT_OPAQUE,
                           inumFrames,100,0);

   if ( icx < 0 )
     icx = S_SystemBitmap::cx + 2*_root->cxSizeBorder;
   if ( icy < 0 )
     icy = S_SystemBitmap::cy + 2*_root->cySizeBorder;
   S_Window::Create(iroot,iparent,ititle,NULL,1,ix,iy,icx,icy,
                    iflFrameFlags);

   return 0;
   }


int
S_GWindow::HandleCreateMsg() {
   Show();
   return 0;
   }


int
S_GWindow::HandleKeyboardMsg() {
   for ( S_Tree *w=widgets.child; w!=NULL; w=w->next )
     if ( ((S_GWidget *)w)->visible )
        ((S_GWidget *)w)->HandleKeyboardMsg();
   return 0;
   }


int
S_GWindow::HandleMouseMsg() {
   int mouseHandled = FALSE;

   if ( !dragging ) {
      for ( S_Tree *w=widgets.child; w!=NULL; w=w->next )
        if ( ((S_GWidget *)w)->visible && ((S_GWidget *)w)->HandleMouseMsg() )
            mouseHandled = TRUE;
      }

   if ( mouseHandled )
      return 0;

   if ( draggable ) {

      PointStruct pt = {0,0};
#     ifdef WIN32
         GetCursorPos((POINT *)&pt);
#     endif
#     ifdef OS2
         WinQueryPointerPos(HWND_DESKTOP,(POINTL *)&pt);
         pt.y = _root->cy - pt.y;
#     endif
      switch ( button ) {
         case 0 :
            if ( dragging )
               SetPosition(x0+pt.x-dx,y0+pt.y-dy);
            break;
         case 1 :
            dragging = FALSE;
            if ( !keyup ) {
               dragging = TRUE;
               dx = pt.x;
               dy = pt.y;
               x0 = x;
               y0 = y;
#              ifdef WIN32
                  SetCapture(hwnd);
#              endif
#              ifdef OS2
                  WinSetCapture(HWND_DESKTOP,hwnd);
#              endif
               }
            else {
#              ifdef WIN32
                  ReleaseCapture();
#              endif
#              ifdef OS2
                  WinSetCapture(HWND_DESKTOP,NULL);
#              endif
               }
            break;
         }
      }

   return 0;
   }


int
S_GWindow::HandlePaintMsg() {
   Draw();
   return 0;
   }


int
S_GWindow::HandleCloseMsg() {
   return 0;
   }


int
S_GWindow::Destroy() {
   Init();
   return 0;
   }

S_GWindow::~S_GWindow() {
   Destroy();
   }


int
S_GWindow::Draw(int frame) {
   if ( closing ) return 0;
   Blit(this,frame,0,0);
   for ( S_Tree *w=widgets.child; w!=NULL; w=w->next )
      ((S_GWidget *)w)->Draw();
   return 0;
   }
