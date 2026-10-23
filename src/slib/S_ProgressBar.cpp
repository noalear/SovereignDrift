/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
#include "S_OS.h"
#include "S_ProgressBar.h"
#include "S_Root.h"

S_ProgressBar::S_ProgressBar() {
   }


int
S_ProgressBar::Create(S_Object *iparent,
                      int ix,int iy,int icx,int icy) {
   S_Object::Create(iparent,NULL,
                    S_OBJECT_PROGRESSBAR,ix,iy,icx,icy);
#  ifdef OS2
#  endif
#  ifdef WIN32
      InitCommonControls();
      hwnd = CreateWindowEx(0,PROGRESS_CLASS,NULL,
                            WS_CHILD | WS_VISIBLE,
                            x,y,cx,cy,
                            parent->hwnd,(HMENU)id,root->hInst,NULL);
      SetWindowLong(hwnd,GWL_USERDATA,(slong)this);
#  endif
#  ifdef GTK
#  endif
   hwndFrame = hwndClient = hwnd;
   parent->AddChild(this);

   SetRange(0,100);
   return 0;
   }


int
S_ProgressBar::Destroy() {
   if ( parent ) {
      parent->RemoveChild(this);
      parent = NULL;
      }
   S_Object::Destroy();
   return 0;
   }

S_ProgressBar::~S_ProgressBar() {
   Destroy();
   }


int
S_ProgressBar::SetValue(int value) {
#  ifdef OS2
#  endif
#  ifdef WIN32
      SendMessage(hwnd,PBM_SETPOS,value,0);
#  endif
   return 0;
   }


int
S_ProgressBar::SetRange(int minRange,int maxRange) {
#  ifdef OS2
#  endif
#  ifdef WIN32
      SendMessage(hwnd,PBM_SETRANGE,0,MAKELPARAM(minRange,maxRange));
#  endif
   return 0;
   }
