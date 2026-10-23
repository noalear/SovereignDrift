/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
#include "S_OS.h"
#include "S_Button.h"
#include "S_Root.h"

S_Button::S_Button() {
   }

S_Button::S_Button(S_Object *iparent,int (*ipMsgHandler)(S_Button *),
                     char *text,int ix,int iy,int icx,int icy,
                     int flag) {
   Create(iparent,ipMsgHandler,text,ix,iy,icx,icy,flag);
   }

int
S_Button::Create(S_Object *iparent,int (*ipMsgHandler)(S_Button *),
                   char *text,int ix,int iy,int icx,int icy,
                   int flag) {
   S_Object::Create(iparent,(int(*)(S_Object *))ipMsgHandler,
                    S_OBJECT_BUTTON,ix,iy,icx,icy);
#  ifdef OS2
      hwnd = WinCreateWindow(parent->hwnd,WC_BUTTON,text,
                             BS_PUSHBUTTON | flag |
                             WS_TABSTOP | WS_VISIBLE,
                             x,parent->cyClient-cy-y,cx,cy,
                             parent->hwnd,HWND_TOP,id,NULL,
                             &defaultPres2);
      WinSetWindowULong(hwnd,QWL_USER,(ULONG)this);
#  endif
#  ifdef WIN32
      hwnd = CreateWindowEx(0,"BUTTON",text,
                            flag | WS_TABSTOP | WS_CHILD | WS_VISIBLE,
                            x,y,cx,cy,
                            parent->hwnd,(HMENU)id,root->hInst,NULL);
      SetWindowLong(hwnd,GWL_USERDATA,(slong)this);
      SetFont(defaultFont);
#  endif
#  ifdef GTK
      hwndFrame = gtk_alignment_new(0.0,0.0,0.0,0.0);
      gtk_container_add(GTK_CONTAINER(parent->hwndClient),hwndFrame);
      gtk_widget_show(hwndFrame);
      hwnd = gtk_button_new_with_label(text);
      gtk_container_add(GTK_CONTAINER(hwndFrame),hwnd);
      gtk_widget_show(hwnd);
      SetSize(cx,cy);
      SetPosition(x,y);
#  endif
   hwndFrame = hwndClient = hwnd;
   parent->AddChild(this);
   return 0;
   }

int
S_Button::Destroy() {
  if ( parent ) {
      parent->RemoveChild(this);
      parent = NULL;
      }
   S_Object::Destroy();
   return 0;
   }

S_Button::~S_Button() { 
   Destroy();
   }


int
S_Button::SetDefault() {
#  ifdef OS2
      ulong style =  WinQueryWindowULong(hwnd,QWL_STYLE);
      WinSetWindowULong(hwnd,QWL_STYLE,style | BS_DEFAULT);
#  endif
#  ifdef WIN32
      ulong style = GetWindowLong(hwnd,GWL_STYLE);
      SetWindowLong(hwnd,GWL_STYLE,style | BS_DEFPUSHBUTTON);
#  endif
   return 0;
   }


