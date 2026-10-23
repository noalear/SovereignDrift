/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
#include "S_OS.h"
#include "S_ScrollBar.h"
#include "S_Root.h"

S_ScrollBar::S_ScrollBar() {
   }

S_ScrollBar::S_ScrollBar(S_Object *iparent,int (*ipMsgHandler)(S_ScrollBar *),
                         int ix,int iy,int icx,int icy,int flag) {
   Create(iparent,ipMsgHandler,ix,iy,icx,icy,flag);
   }


int
S_ScrollBar::Create(S_Object *iparent,int (*ipMsgHandler)(S_ScrollBar *),
                    int ix,int iy,int icx,int icy,int flag) {
   S_Object::Create(iparent,(int(*)(S_Object *))ipMsgHandler,
                    S_OBJECT_SCROLLBAR,ix,iy,icx,icy);
#  ifdef OS2
      hwnd = WinCreateWindow(parent->hwnd,WC_SCROLLBAR,NULL,
                             SBS_AUTOTRACK | WS_VISIBLE | flag | WS_TABSTOP,
                             x,parent->cyClient-cy-y,cx,cy,
                             parent->hwnd,HWND_TOP,id,NULL,
                             /*&defaultPres2*/NULL);
      WinSetWindowULong(hwnd,QWL_USER,(ULONG)this);
#  endif
#  ifdef WIN32
      hwnd = CreateWindowEx(0,"SCROLLBAR",NULL,
                            flag | WS_TABSTOP | WS_CHILD | WS_VISIBLE,
                            x,y,cx,cy,
                            parent->hwnd,(HMENU)id,root->hInst,NULL);
      SetWindowLong(hwnd,GWL_USERDATA,(slong)this);
      SetFont(defaultFont);
#  endif
#  ifdef GTK
#  endif
   hwndFrame = hwndClient = hwnd;
   parent->AddChild(this);

   SetRange(0,100);
   return 0;
   }


int
S_ScrollBar::Destroy() {
   if ( parent ) {
      parent->RemoveChild(this);
      parent = NULL;
      }
   S_Object::Destroy();
   return 0;
   }

S_ScrollBar::~S_ScrollBar() {
   Destroy();
   }


int
S_ScrollBar::GetValue() {
#  ifdef OS2
      return (int)WinSendMsg(hwnd,SBM_QUERYPOS,(MPARAM)0,(MPARAM)0);
#  endif
#  ifdef WIN32
      return GetScrollPos(hwnd,SB_CTL);
#  endif
#  ifdef GTK
      return 0;
#  endif
   }


int
S_ScrollBar::SetValue(int value) {
#  ifdef OS2
      WinSendMsg(hwnd,SBM_SETPOS,(MPARAM)value,(MPARAM)0);
#  endif
#  ifdef WIN32
      SetScrollPos(hwnd,SB_CTL,value,TRUE);
#  endif
   return 0;
   }


int
S_ScrollBar::SetRange(int minRange,int maxRange) {
#  ifdef OS2
      WinSendMsg(hwnd,SBM_SETSCROLLBAR,0,
                 MPFROM2SHORT((short)minRange,(short)maxRange));
#  endif
#  ifdef WIN32
      SetScrollRange(hwnd,SB_CTL,minRange,maxRange,TRUE);
#  endif
   return 0;
   }


int
S_ScrollBar::SetBarSize(int pageSize) {
#  ifdef OS2
#  endif
#  ifdef WIN32
      SCROLLINFO si;
      si.cbSize = sizeof(si);
      si.fMask = SIF_PAGE;
      si.nPage = pageSize;
      SetScrollInfo(hwnd,SIF_PAGE,&si,TRUE);
#  endif
   return 0;
   }
