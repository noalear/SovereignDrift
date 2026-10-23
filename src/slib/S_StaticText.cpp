/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
#include "S_OS.h"
#include "S_StaticText.h"
#include "S_Root.h"

S_StaticText::S_StaticText() {
   }

S_StaticText::S_StaticText(S_Object *iparent,char *text,
                           int ix,int iy,int icx,int icy) {
   Create(iparent,text,ix,iy,icx,icy);
   }


int
S_StaticText::Create(S_Object *iparent,char *text,
                     int ix,int iy,int icx,int icy) {
   S_Object::Create(iparent,NULL,S_OBJECT_STATICTEXT,ix,iy,icx,icy);
#  ifdef OS2
      hwnd = WinCreateWindow(parent->hwnd,WC_STATIC,text,
                             WS_VISIBLE|SS_TEXT|DT_TOP|DT_LEFT|DT_WORDBREAK,
                             x,parent->cyClient-cy-y,cx,cy,
                             parent->hwnd,HWND_TOP,0,NULL,
                             &defaultPres1);
      WinSetWindowULong(hwnd,QWL_USER,(ULONG)this);
#  endif
#  ifdef WIN32
      hwnd = CreateWindowEx(WS_EX_TRANSPARENT,"STATIC",text,
                            WS_VISIBLE | WS_CHILD,
                            x,y,cx,cy,
                            parent->hwnd,NULL,root->hInst,NULL);
      SetWindowLong(hwnd,GWL_USERDATA,(slong)this);
      SetFont(defaultFont);
#  endif
#  ifdef GTK
#  endif
   hwndFrame = hwndClient = hwnd;
   parent->AddChild(this);
   return 0;
   }


int
S_StaticText::Destroy() {
   if ( parent ) {
      parent->RemoveChild(this);
      parent = NULL;
      }
   S_Object::Destroy();
   return 0;
   }

S_StaticText::~S_StaticText() {
   Destroy();
   }

