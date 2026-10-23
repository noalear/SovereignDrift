/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
#include "S_OS.h"
#include "S_GroupBox.h"
#include "S_Root.h"


int
S_GroupBox::Init() {
   S_Object::Init();
   return 0;
   }

S_GroupBox::S_GroupBox() {
   Init();
   }

S_GroupBox::S_GroupBox(S_Object *iparent,char *text,
                       int ix,int iy,int icx,int icy) {
   Init();
   Create(iparent,text,ix,iy,icx,icy);
   }


int
S_GroupBox::Create(S_Object *iparent,char *text,
                   int ix,int iy,int icx,int icy) {
   S_Object::Create(iparent,NULL,S_OBJECT_GROUPBOX,ix,iy,icx,icy);
   parent = iparent;
   root = parent->root;
   x = ix;
   y = iy;
   cx = icx;
   cy = icy;
#  ifdef OS2
      hwnd = WinCreateWindow(parent->hwnd,WC_STATIC,text,
                             WS_VISIBLE|SS_GROUPBOX|WS_GROUP,
                             x,parent->cyClient-cy-y,cx,cy,
                             parent->hwnd,HWND_TOP,0,NULL,
                             &defaultPres1);
      WinSetWindowULong(hwnd,QWL_USER,(ULONG)this);
#  endif
#  ifdef WIN32
      hwnd = CreateWindowEx(WS_EX_TRANSPARENT,"BUTTON",text,
                            BS_GROUPBOX | WS_VISIBLE | WS_CHILD | WS_GROUP,
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
S_GroupBox::Destroy() {
  S_Object::Destroy();
   if ( parent )
      parent->RemoveChild(this);
   return 0;
   }

S_GroupBox::~S_GroupBox() {
   Destroy();
   }

