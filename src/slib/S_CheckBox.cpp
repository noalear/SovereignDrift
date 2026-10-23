/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
#include "S_OS.h"
#include "S_CheckBox.h"
#include "S_Root.h"

S_CheckBox::S_CheckBox() {
   }

S_CheckBox::S_CheckBox(S_Object *iparent,int (*ipMsgHandler)(S_CheckBox *),
		       char *text,int ix,int iy,int icx,int icy) {
   Create(iparent,ipMsgHandler,text,ix,iy,icx,icy);
   }


int
S_CheckBox::Create(S_Object *iparent,int (*ipMsgHandler)(S_CheckBox *),
		   char *text,int ix,int iy,int icx,int icy) {
   S_Object::Create(iparent,(int(*)(S_Object *))ipMsgHandler,
                    S_OBJECT_CHECKBOX,ix,iy,icx,icy);
#  ifdef OS2
      hwnd = WinCreateWindow(parent->hwnd,WC_BUTTON,text,
                             BS_AUTOCHECKBOX | WS_TABSTOP | WS_VISIBLE,
                             x,parent->cyClient-cy-y,cx,cy,
                             parent->hwnd,HWND_TOP,id,NULL,
                             &defaultPres1);
      WinSetWindowULong(hwnd,QWL_USER,(ULONG)this);
#  endif
#  ifdef WIN32
      hwnd = CreateWindowEx(0,"BUTTON",text,
                            BS_AUTOCHECKBOX | WS_TABSTOP | 
                            WS_CHILD | WS_VISIBLE,
                            x,y,cx,cy,
                            parent->hwnd,(HMENU)id,root->hInst,NULL);
      SetWindowLong(hwnd,GWL_USERDATA,(slong)this);
      SetFont(defaultFont);
#  endif
#  ifdef GTK
      hwndFrame = gtk_alignment_new(0.0,0.0,0.0,0.0);
      gtk_container_add(GTK_CONTAINER(parent->hwndClient),hwndFrame);
      gtk_widget_show(hwndFrame);
      hwnd = gtk_check_button_new_with_label(text);
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
S_CheckBox::Destroy() {
   if ( parent ) {
      parent->RemoveChild(this);
      parent = NULL;
      }
   S_Object::Destroy();
   return 0;
   }

S_CheckBox::~S_CheckBox() {
   Destroy();
   }


int
S_CheckBox::SetCheck(int flag) {
#  ifdef OS2
      WinSendMsg(hwnd,BM_SETCHECK,(MPARAM)flag,0);
#  endif
#  ifdef WIN32
      SendMessage(hwnd,BM_SETCHECK,(WPARAM)flag,0);
#  endif
   return 0;
   }


int
S_CheckBox::IsChecked() {
#  ifdef OS2
      return (int)WinSendMsg(hwnd,BM_QUERYCHECK,(MPARAM)0,0);
#  endif
#  ifdef WIN32
      return SendMessage(hwnd,BM_GETCHECK,(WPARAM)0,0);
#  endif
#  ifdef UNIX
   return 0;
#  endif
   }
