/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
#include "S_OS.h"
#include "S_Notebook.h"
#include "S_SystemIcon.h"
#include "S_Root.h"

S_Notebook::S_Notebook() {
   Init();
   }


int
S_Notebook::Init() {
#  ifdef WIN32
      himl = NULL;
#  endif
   return 0;
   }

S_Notebook::S_Notebook(S_Object *iparent,int (*ipMsgHandler)(S_Notebook *),
		       int ix,int iy,int icx,int icy) {
   Create(iparent,ipMsgHandler,ix,iy,icx,icy);
   }


int
S_Notebook::Create(S_Object *iparent,int (*ipMsgHandler)(S_Notebook *),
		   int ix,int iy,int icx,int icy) {
   S_Object::Create(iparent,(int(*)(S_Object *))ipMsgHandler,
                    S_OBJECT_NOTEBOOK,ix,iy,icx,icy);
   Init();
#  ifdef OS2
      hwnd = WinCreateWindow(parent->hwnd,WC_NOTEBOOK,"",
                             WS_VISIBLE | 0x800,
                             x,parent->cyClient-cy-y,cx,cy,
                             parent->hwnd,HWND_TOP,id,NULL,
                             &defaultPres2);
      WinSetWindowULong(hwnd,QWL_USER,(ULONG)this);
#  endif
#  ifdef WIN32
      InitCommonControls();
      hwnd = CreateWindowEx(0,WC_TABCONTROL,"",
                            WS_CLIPSIBLINGS | WS_VISIBLE | WS_CHILD,
                            x,y,cx,cy,
                            parent->hwnd,(HMENU)id,root->hInst,NULL);
      SetWindowLong(hwnd,GWL_USERDATA,(slong)this);
      SetFont(defaultFont);
      himl = ImageList_Create(16,16,ILC_MASK|ILC_COLOR,256,512);
      SendMessage(hwnd,TCM_SETIMAGELIST,0,(LPARAM)himl); 
#  endif
#  ifdef GTK
#  endif
   hwndFrame = hwndClient = hwnd;
   parent->AddChild(this);
   return 0;
   }


int
S_Notebook::Destroy() {
#  ifdef WIN32
   if ( himl ) {
      ImageList_Destroy(himl);
      himl = NULL;
      }
#  endif
   if ( parent ) {
      parent->RemoveChild(this);
      parent = NULL;
      }
   S_Object::Destroy();
   return 0;
   }

S_Notebook::~S_Notebook() {
   Destroy();
   }


int
S_Notebook::AddItem(char *text,S_SystemIcon *iicon) {
#  ifdef OS2
      int index = (int) WinSendMsg(hwnd,BKM_INSERTPAGE,(MPARAM)NULL,
                                   MPFROM2SHORT(BKA_MAJOR,BKA_LAST));
      WinSendMsg(hwnd,BKM_SETTABTEXT,(MPARAM)index,(MPARAM)text);
#  endif
#  ifdef WIN32
      int index = SendMessage(hwnd,TCM_GETITEMCOUNT,0,0);
      InsertItem(text,index,iicon);
#  endif
#  ifdef GTK
#  endif
   return 0;
   }


int
S_Notebook::InsertItem(char *text,int index,S_SystemIcon *iicon) {
#  ifdef OS2
      WinSendMsg(hwnd,BKM_INSERTPAGE,(MPARAM)index,
                 MPFROM2SHORT(BKA_MAJOR,BKA_PREV));
      WinSendMsg(hwnd,BKM_SETTABTEXT,(MPARAM)index,(MPARAM)text);
#  endif
#  ifdef WIN32
      int iconIndex=-1;
      if ( iicon )
         iconIndex = ImageList_AddIcon(himl,iicon->hIcon);
      TC_ITEM tci;
      tci.mask = TCIF_TEXT;
      if ( iicon )
         tci.mask |= TCIF_IMAGE;
      tci.iImage = iconIndex;
      tci.pszText = text;
      SendMessage(hwnd,TCM_INSERTITEM,(WPARAM)index,(LPARAM)&tci);
#  endif
#  ifdef GTK
#  endif
   return 0;
   }


int
S_Notebook::RemoveItem(int index) {
#  ifdef OS2
      WinSendMsg(hwnd,BKM_DELETEPAGE,(MPARAM)index,(MPARAM)BKA_SINGLE);
#  endif
#  ifdef WIN32
      SendMessage(hwnd,TCM_DELETEITEM,(WPARAM)index,0);
#  endif
#  ifdef GTK
#  endif
   return 0;
   }


int
S_Notebook::GetIndex() {
#  ifdef OS2
      return (int)WinSendMsg(hwnd,LM_QUERYSELECTION,(MPARAM)0,(MPARAM)0);
#  endif
#  ifdef WIN32
      return SendMessage(hwnd,TCM_GETCURSEL,0,0);
#  endif
#  ifdef GTK
      return 0;
#  endif
   }


int
S_Notebook::SetIndex(int index) {
#  ifdef OS2
      return (int)WinSendMsg(hwnd,LM_SELECTITEM,(MPARAM)index,(MPARAM)TRUE);
#  endif
#  ifdef WIN32
      return SendMessage(hwnd,TCM_SETCURSEL,(WPARAM)index,0);
#  endif
#  ifdef GTK
      return 0;
#  endif
   }


int
S_Notebook::Flush() {
#  ifdef OS2
      WinSendMsg(hwnd,LM_DELETEALL,0,0);
#  endif
#  ifdef WIN32
      SendMessage(hwnd,TCM_DELETEALLITEMS,0,0);
#  endif
#  ifdef GTK
#  endif
   return 0;
   }

