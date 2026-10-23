/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
#include "S_OS.h"
#include "S_RadioButton.h"
#include "S_Root.h"

S_RadioButton::S_RadioButton() {
   }

S_RadioButton::S_RadioButton(S_Object *iparent,
                             int (*ipMsgHandler)(S_RadioButton *),
                             char *text,int ix,int iy,int icx,int icy ) {
   Create(iparent,ipMsgHandler,text,ix,iy,icx,icy);
   }


int
S_RadioButton::Create(S_Object *iparent,
                      int (*ipMsgHandler)(S_RadioButton *),
                      char *text,int ix,int iy,int icx,int icy ) {
   S_Object::Create(iparent,(int(*)(S_Object *))ipMsgHandler,
                    S_OBJECT_RADIOBUTTON,ix,iy,icx,icy);
#  ifdef OS2
      ulong flags = BS_RADIOBUTTON |WS_TABSTOP | WS_VISIBLE;
      hwnd = WinCreateWindow(parent->hwnd,WC_BUTTON,text,flags,
                             x,parent->cyClient-cy-y,cx,cy,
                             parent->hwnd,HWND_TOP,id,NULL,
                             &defaultPres1);
      WinSetWindowULong(hwnd,QWL_USER,(ULONG)this);
#  endif
#  ifdef WIN32
      ulong flags = BS_RADIOBUTTON | WS_TABSTOP | WS_VISIBLE | WS_CHILD;
      hwnd = CreateWindowEx(0,"BUTTON",text,flags,
                            x,y,cx,cy,
                            parent->hwnd,(HMENU)id,root->hInst,NULL);
      SetWindowLong(hwnd,GWL_USERDATA,(slong)this);
      SetFont(defaultFont);
#  endif
#  ifdef GTK
      hwndFrame = gtk_alignment_new(0.0,0.0,0.0,0.0);
      gtk_container_add(GTK_CONTAINER(parent->hwndClient),hwndFrame);
      gtk_widget_show(hwndFrame);
      hwnd = gtk_radio_button_new_with_label(NULL,text);
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
S_RadioButton::Destroy() {
   if ( parent ) {
      parent->RemoveChild(this);
      parent = NULL;
      }
   S_Object::Destroy();
   return 0;
   }

S_RadioButton::~S_RadioButton() {
   Destroy();
   }


int
S_RadioButton::SetCheck(int flag) {
#  ifdef OS2
      WinSendMsg(hwnd,BM_SETCHECK,(MPARAM)flag,0);
#  endif
#  ifdef WIN32
      SendMessage(hwnd,BM_SETCHECK,(WPARAM)flag,0);
#  endif
   return 0;
   }


int
S_RadioButton::IsChecked() {
#  ifdef OS2
      return (int)WinSendMsg(hwnd,BM_QUERYCHECK,(MPARAM)0,0);
#  endif
#  ifdef WIN32
      return SendMessage(hwnd,BM_GETCHECK,(WPARAM)0,0);
#  endif
   return 0;
   }
