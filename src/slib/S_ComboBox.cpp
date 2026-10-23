/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
#include "S_OS.h"
#include "S_ComboBox.h"
#include "S_Root.h"


S_ComboBox::S_ComboBox() {
   }


S_ComboBox::S_ComboBox(S_Object *iparent,int (*ipMsgHandler)(S_ComboBox *),
		       int ix,int iy,int icx,int icy) {
   Create(iparent,ipMsgHandler,ix,iy,icx,icy);
   }


int
S_ComboBox::Create(S_Object *iparent,int (*ipMsgHandler)(S_ComboBox *),
		   int ix,int iy,int icx,int icy) {
   S_Object::Create(iparent,(int(*)(S_Object *))ipMsgHandler,
                    S_OBJECT_COMBOBOX,ix,iy,icx,icy);
#  ifdef OS2
      hwnd = WinCreateWindow(parent->hwnd,WC_COMBOBOX,NULL,
                             WS_VISIBLE|WS_TABSTOP|CBS_DROPDOWNLIST,
                             x,parent->cyClient-y-cy,cx,cy,
                             parent->hwnd,HWND_TOP,id,NULL,
                             &defaultPres2);
      WinSetWindowULong(hwnd,QWL_USER,(ULONG)this);
#  endif
#  ifdef WIN32
      hwnd = CreateWindowEx(WS_EX_CLIENTEDGE,"COMBOBOX",NULL,
                            WS_VISIBLE | WS_CHILD | WS_VSCROLL | WS_TABSTOP | 
                            CBS_DROPDOWNLIST | CBS_AUTOHSCROLL,
                            x,y,cx,cy,
                            parent->hwnd,(HMENU)id,root->hInst,NULL);
      SetWindowLong(hwnd,GWL_USERDATA,(slong)this);
      SetFont(defaultFont);
#  endif
#  ifdef GTK
      hwndFrame = gtk_alignment_new(0.0,0.0,0.0,0.0);
      gtk_container_add(GTK_CONTAINER(parent->hwndClient),hwndFrame);
      gtk_widget_show(hwndFrame);
      hwnd = gtk_combo_new();
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
S_ComboBox::Destroy() {
   if ( parent ) {
      parent->RemoveChild(this);
      parent = NULL;
      }
   S_Object::Destroy();
   return 0;
   }

S_ComboBox::~S_ComboBox() {
   Destroy();
   }


int
S_ComboBox::AddItem(char *text) {
#  ifdef OS2
      WinSendMsg(hwnd,LM_INSERTITEM,MPFROMSHORT(LIT_END),MPFROMP(text));
#  endif
#  ifdef WIN32
      SendMessage(hwnd,CB_ADDSTRING,0,(LPARAM)text);
#  endif
   return 0;
   }


int
S_ComboBox::InsertItem(char *text,int index) {
#  ifdef OS2
      WinSendMsg(hwnd,LM_INSERTITEM,MPFROMSHORT(index),MPFROMP(text));
#  endif
#  ifdef WIN32
      SendMessage(hwnd,CB_INSERTSTRING,(WPARAM)index,(LPARAM)text);
#  endif
   return 0;
   }


int
S_ComboBox::RemoveItem(int index) {
#  ifdef OS2
      WinSendMsg(hwnd,LM_DELETEITEM,MPFROMSHORT(index),MPFROMSHORT(0));
#  endif
#  ifdef WIN32
      SendMessage(hwnd,CB_DELETESTRING,(WPARAM)index,0);
#  endif
   return 0;
   }


int
S_ComboBox::GetIndex() {
#  ifdef OS2
      return (int)WinSendMsg(hwnd,LM_QUERYSELECTION,(MPARAM)0,(MPARAM)0);
#  endif
#  ifdef WIN32
      return SendMessage(hwnd,CB_GETCURSEL,0,0);
#  endif
#  ifdef UNIX
      return 0;
#  endif
   }


int
S_ComboBox::SetIndex(int index) {
#  ifdef OS2
      return (int)WinSendMsg(hwnd,LM_SELECTITEM,(MPARAM)index,(MPARAM)TRUE);
#  endif
#  ifdef WIN32
      return SendMessage(hwnd,CB_SETCURSEL,(WPARAM)index,0);
#  endif
#  ifdef UNIX
      return 0;
#  endif
   }


int
S_ComboBox::SetItem(char *text) {
#  ifdef OS2
      int index = (int)WinSendMsg(hwnd,LM_SEARCHSTRING,(MPARAM)0,(MPARAM)text);
      return (int)WinSendMsg(hwnd,LM_SELECTITEM,(MPARAM)index,(MPARAM)TRUE);
#  endif
#  ifdef WIN32
      int index = SendMessage(hwnd,CB_FINDSTRINGEXACT,(WPARAM)0,(LPARAM)text);
      return SendMessage(hwnd,CB_SETCURSEL,(WPARAM)index,0);
#  endif
#  ifdef UNIX
      return 0;
#  endif
   }


char *
S_ComboBox::GetItem() {
   int len = 0;
   int index = 0;
#  ifdef OS2
      index = (int)WinSendMsg(hwnd,LM_QUERYSELECTION,(MPARAM)0,(MPARAM)0);
      len = (int)WinSendMsg(hwnd,LM_QUERYITEMTEXTLENGTH,
                            (MPARAM)index,(MPARAM)0);
#  endif
#  ifdef WIN32
      index = SendMessage(hwnd,CB_GETCURSEL,0,0);
      len = SendMessage(hwnd,CB_GETLBTEXTLEN,(WPARAM)index,0);
#  endif
      if ( index < 0 )
         return NULL;
      if ( str == NULL )
         str = (char *) S_malloc(len+1);
      else
         str = (char *) S_realloc(str,len+1);
#  ifdef OS2
      WinSendMsg(hwnd,LM_QUERYITEMTEXT,MPFROM2SHORT(index,len+1),(MPARAM)str);
#  endif
#  ifdef WIN32
      SendMessage(hwnd,CB_GETLBTEXT,index,(LPARAM)str);
#  endif
   return str;
   }


int
S_ComboBox::Flush() {
#  ifdef OS2
      return (int)WinSendMsg(hwnd,LM_DELETEALL,0,0);
#  endif
#  ifdef WIN32
      return SendMessage(hwnd,CB_RESETCONTENT,0,0);
#  endif
#  ifdef UNIX
      return 0;
#  endif
   }
