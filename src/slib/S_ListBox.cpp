/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
#include "S_OS.h"
#include "S_ListBox.h"
#include "S_Root.h"

S_ListBox::S_ListBox() {
   }

S_ListBox::S_ListBox(S_Object *iparent,int (*ipMsgHandler)(S_ListBox *),
                     int ix,int iy,int icx,int icy,int ioptions) {
   Create(iparent,ipMsgHandler,ix,iy,icx,icy,ioptions);
   }


int
S_ListBox::Create(S_Object *iparent,int (*ipMsgHandler)(S_ListBox *),
                  int ix,int iy,int icx,int icy,int ioptions) {
   S_Object::Create(iparent,(int(*)(S_Object *))ipMsgHandler,
                    S_OBJECT_LISTBOX,ix,iy,icx,icy);
#  ifdef OS2
      hwnd = WinCreateWindow(parent->hwnd,WC_LISTBOX,NULL,
                             WS_VISIBLE|LS_NOADJUSTPOS|WS_TABSTOP|ioptions,
                             x,parent->cyClient-y-cy,cx,cy,
                             parent->hwnd,HWND_TOP,id,NULL,
                             &defaultPres2);
      WinSetWindowULong(hwnd,QWL_USER,(ULONG)this);
#  endif
#  ifdef WIN32
      hwnd = CreateWindowEx(WS_EX_CLIENTEDGE,"LISTBOX",NULL,
                            WS_VISIBLE | WS_CHILD | WS_VSCROLL | WS_TABSTOP |
                            WS_HSCROLL | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT |
                            ioptions,
                            x,y,cx,cy,
                            parent->hwnd,(HMENU)id,root->hInst,NULL);
      SetWindowLong(hwnd,GWL_USERDATA,(slong)this);
      SetFont(defaultFont);
#  endif
#  ifdef GTK
      hwndFrame = gtk_alignment_new(0.0,0.0,0.0,0.0);
      gtk_container_add(GTK_CONTAINER(parent->hwndClient),hwndFrame);
      gtk_widget_show(hwndFrame);
      hwnd = gtk_list_new();
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
S_ListBox::Destroy() {
   if ( parent ) {
      parent->RemoveChild(this);
      parent = NULL;
      }
   S_Object::Destroy();
   return 0;
   }

S_ListBox::~S_ListBox() {
   Destroy();
   }


int
S_ListBox::AddItem(char *text) {
#  ifdef OS2
      return (int)
             WinSendMsg(hwnd,LM_INSERTITEM,MPFROMSHORT(LIT_END),MPFROMP(text));
#  endif
#  ifdef WIN32
      return SendMessage(hwnd,LB_ADDSTRING,0,(LPARAM)text);
#  endif
#  ifdef GTK
   return 0;
#  endif
   }


int
S_ListBox::InsertItem(char *text,int index) {
#  ifdef OS2
      WinSendMsg(hwnd,LM_INSERTITEM,MPFROMSHORT(index),MPFROMP(text));
#  endif
#  ifdef WIN32
      SendMessage(hwnd,LB_INSERTSTRING,(WPARAM)index,(LPARAM)text);
#  endif
#  ifdef GTK
#  endif
   return 0;
   }


int
S_ListBox::RemoveItem(int index) {
#  ifdef OS2
      WinSendMsg(hwnd,LM_DELETEITEM,MPFROMSHORT(index),MPFROMSHORT(0));
#  endif
#  ifdef WIN32
      SendMessage(hwnd,LB_DELETESTRING,(WPARAM)index,0);
#  endif
#  ifdef GTK
#  endif
   return 0;
   }


int
S_ListBox::GetIndex() {
#  ifdef OS2
      return (int)WinSendMsg(hwnd,LM_QUERYSELECTION,(MPARAM)0,(MPARAM)0);
#  endif
#  ifdef WIN32
      return SendMessage(hwnd,LB_GETCURSEL,0,0);
#  endif
#  ifdef GTK
      return 0;
#  endif
   }


int
S_ListBox::SetIndex(int index) {
#  ifdef OS2
      return (int)WinSendMsg(hwnd,LM_SELECTITEM,(MPARAM)index,(MPARAM)TRUE);
#  endif
#  ifdef WIN32
      return SendMessage(hwnd,LB_SETCURSEL,(WPARAM)index,0);
#  endif
#  ifdef GTK
      return 0;
#  endif
   }


char *
S_ListBox::GetItem() {
   int len = 0;
#  ifdef OS2
      int index = (int)WinSendMsg(hwnd,LM_QUERYSELECTION,(MPARAM)0,(MPARAM)0);
      len = (int)WinSendMsg(hwnd,LM_QUERYITEMTEXTLENGTH,
                              (MPARAM)index,(MPARAM)0);
#  endif
#  ifdef WIN32
      int index = SendMessage(hwnd,LB_GETCURSEL,0,0);
      len = SendMessage(hwnd,LB_GETTEXTLEN,(WPARAM)index,0);
#  endif
      if ( str == NULL )
         str = (char *) S_malloc(len+1);
      else
         str = (char *) S_realloc(str,len+1);
#  ifdef OS2
      WinSendMsg(hwnd,LM_QUERYITEMTEXT,MPFROM2SHORT(index,len+1),(MPARAM)str);
#  endif
#  ifdef WIN32
      SendMessage(hwnd,LB_GETTEXT,index,(LPARAM)str);
#  endif
#  ifdef GTK
#  endif
   return str;
   }


int
S_ListBox::GetNumItems() {
#  ifdef OS2
      return (int)WinSendMsg(hwnd,LM_QUERYITEMCOUNT,0,0);
#  endif
#  ifdef WIN32
      return SendMessage(hwnd,LB_GETCOUNT,0,0);
#  endif
#  ifdef GTK
      return 0;
#  endif
   }


int
S_ListBox::Flush() {
#  ifdef OS2
      WinSendMsg(hwnd,LM_DELETEALL,0,0);
#  endif
#  ifdef WIN32
      SendMessage(hwnd,LB_RESETCONTENT,0,0);
#  endif
#  ifdef GTK
#  endif
   return 0;
   }

