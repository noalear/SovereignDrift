/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
#include "S_OS.h"
#include "S_Container.h"
#include "S_Root.h"

S_Container::S_Container() {
   }

S_Container::S_Container(S_Object *iparent,int (*ipMsgHandler)(S_Container *),
                     int ix,int iy,int icx,int icy,int ioptions) {
   Create(iparent,ipMsgHandler,ix,iy,icx,icy,ioptions);
   }


int
S_Container::Create(S_Object *iparent,int (*ipMsgHandler)(S_Container *),
                  int ix,int iy,int icx,int icy,int ioptions) {
   S_Object::Create(iparent,(int(*)(S_Object *))ipMsgHandler,
                    S_OBJECT_CONTAINER,ix,iy,icx,icy);
#  ifdef OS2
      hwnd = WinCreateWindow(parent->hwnd,WC_CONTAINER,NULL,
                             WS_VISIBLE|WS_TABSTOP|ioptions,
                             x,parent->cyClient-y-cy,cx,cy,
                             parent->hwnd,HWND_TOP,id,NULL,
                             &defaultPres2);
      WinSetWindowULong(hwnd,QWL_USER,(ULONG)this);
#  endif
#  ifdef WIN32
      InitCommonControls();
      hwnd = CreateWindowEx(WS_EX_CLIENTEDGE,WC_LISTVIEW,NULL,
                            WS_VISIBLE | WS_CHILD | WS_VSCROLL | WS_TABSTOP |
                            WS_HSCROLL | ioptions,
                            x,y,cx,cy,
                            parent->hwnd,(HMENU)id,root->hInst,NULL);
      SetWindowLong(hwnd,GWL_USERDATA,(slong)this);
      SetFont(defaultFont);
#  endif
#  ifdef GTK
      /*
      hwndFrame = gtk_alignment_new(0.0,0.0,0.0,0.0);
      gtk_container_add(GTK_CONTAINER(parent->hwndClient),hwndFrame);
      gtk_widget_show(hwndFrame);
      hwnd = gtk_list_new();
      gtk_container_add(GTK_CONTAINER(hwndFrame),hwnd);
      gtk_widget_show(hwnd);
      SetSize(cx,cy);
      SetPosition(x,y);
      */
#  endif

   hwndFrame = hwndClient = hwnd;
   parent->AddChild(this);
   return 0;
   }


int
S_Container::Destroy() {
   if ( parent ) {
      parent->RemoveChild(this);
      parent = NULL;
      }
   S_Object::Destroy();
   return 0;
   }

S_Container::~S_Container() {
   Destroy();
   }


int
S_Container::InsertColumn(int i,char *text,int width) {
#  ifdef WIN32
      LV_COLUMN lvc;
      lvc.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
      lvc.fmt = LVCFMT_LEFT;
      lvc.cx = width;
      lvc.pszText = text;
      lvc.cchTextMax = strlen(text)+1;
      lvc.iSubItem = i;
      SendMessage(hwnd,LVM_INSERTCOLUMN,i,(LPARAM)&lvc);
#  endif
   return 0;
   }


int
S_Container::InsertItem(int i,char *text0,char *text1,char *text2,
                        char *text3,char *text4) {
#  ifdef OS2
#  endif
#  ifdef WIN32
      LV_ITEM lvi;
      lvi.mask = LVIF_TEXT;
      lvi.iItem = i;
      if ( text0 ) {
         lvi.iSubItem = 0;
         lvi.pszText = text0;
         lvi.cchTextMax = strlen(text0)+1;
         lvi.iItem = SendMessage(hwnd,LVM_INSERTITEM,0,(LPARAM)&lvi);
         }
      if ( text1 ) {
         lvi.iSubItem = 1;
         lvi.pszText = text1;
         lvi.cchTextMax = strlen(text1)+1;
         SendMessage(hwnd,LVM_SETITEM,0,(LPARAM)&lvi);
         }
      if ( text2 ) {
         lvi.iSubItem = 2;
         lvi.pszText = text2;
         lvi.cchTextMax = strlen(text2)+1;
         SendMessage(hwnd,LVM_SETITEM,0,(LPARAM)&lvi);
         }
      if ( text3 ) {
         lvi.iSubItem = 3;
         lvi.pszText = text3;
         lvi.cchTextMax = strlen(text3)+1;
         SendMessage(hwnd,LVM_SETITEM,0,(LPARAM)&lvi);
         }
      if ( text4 ) {
         lvi.iSubItem = 4;
         lvi.pszText = text4;
         lvi.cchTextMax = strlen(text4)+1;
         SendMessage(hwnd,LVM_SETITEM,0,(LPARAM)&lvi);
         }
#  endif
#  ifdef GTK
#  endif
   return 0;
   }


int
S_Container::Flush() {
#  ifdef OS2
#  endif
#  ifdef WIN32
      SendMessage(hwnd,LVM_DELETEALLITEMS,0,0);
#  endif
#  ifdef GTK
#  endif
   return 0;
   }


int
S_Container::InsertItem(char *,int) {
#  ifdef OS2
#  endif
#  ifdef WIN32
#  endif
#  ifdef GTK
#  endif
   return 0;
   }


int
S_Container::RemoveItem(int) {
#  ifdef OS2
#  endif
#  ifdef WIN32
#  endif
#  ifdef GTK
#  endif
   return 0;
   }


int
S_Container::GetIndex() {
#  ifdef OS2
   return 0;
#  endif
#  ifdef WIN32
      return SendMessage(hwnd,LVM_GETNEXTITEM,(WPARAM)(-1),MAKELPARAM(LVNI_SELECTED,0));
#  endif
#  ifdef GTK
   return 0;
#  endif
   }


int
S_Container::SetIndex(int index) {
#  ifdef OS2
#  endif
#  ifdef WIN32
      LV_ITEM lvi;
      lvi.mask = LVIF_STATE;
      lvi.iItem = index;
      lvi.iSubItem = 0;
      lvi.state = LVIS_SELECTED | LVIS_FOCUSED;
      lvi.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
      SendMessage(hwnd,LVM_SETITEMSTATE,0,(LPARAM)&lvi);
#  endif
#  ifdef GTK
#  endif
   return 0;
   }

char *
S_Container::GetItem() {
#  ifdef GTK
#  endif
#  ifdef OS2
#  endif
#  ifdef WIN32
#  endif
#  ifdef OS2
#  endif
#  ifdef WIN32
#  endif
   return str;
   }

