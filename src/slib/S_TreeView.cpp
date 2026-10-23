/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
#include "S_OS.h"
#include "S_TreeView.h"
#include "S_Root.h"
#include "S_TreeViewNode.h"
#include "S_Window.h"


int
S_TreeView::Init()
{
  rootNode = NULL;
  dragObject = NULL;
  dropTarget = NULL;

# ifdef WIN32
    himl = NULL;
# endif

  return 0;
}


S_TreeView::S_TreeView()
{
  Init();
}


S_TreeView::S_TreeView(S_Object *iparent,
                       int (*ipMsgHandler)(S_TreeView *),
                       int ix,int iy,int icx,int icy)
{
  Init();
  Create(iparent,ipMsgHandler,ix,iy,icx,icy);
}


int
S_TreeView::Create(S_Object *iparent,
                   int (*ipMsgHandler)(S_TreeView *),
                   int ix,int iy,int icx,int icy)
{
  
  Destroy();
  
  S_Object::Create(iparent,(int(*)(S_Object *))ipMsgHandler,
		   S_OBJECT_TREEVIEW,ix,iy,icx,icy);
# ifdef OS2
  hwnd = WinCreateWindow(parent->hwnd,WC_CONTAINER,"",
			 WS_VISIBLE|WS_TABSTOP,
			 x,parent->cyClient-cy-y,cx,cy,
			 parent->hwnd,HWND_TOP,id,NULL,
			 &defaultPres2);
  WinSetWindowULong(hwnd,QWL_USER,(ULONG)this);
# endif
# ifdef WIN32
  InitCommonControls();
  hwnd = CreateWindowEx(WS_EX_CLIENTEDGE,WC_TREEVIEW,"",
			WS_CLIPSIBLINGS | WS_VISIBLE | WS_CHILD |
			TVS_HASLINES | TVS_HASBUTTONS | TVS_LINESATROOT |
			TVS_SHOWSELALWAYS | WS_TABSTOP,
			x,y,cx,cy,
			parent->hwnd,(HMENU)id,root->hInst,NULL);
  SetWindowLong(hwnd,GWL_USERDATA,(slong)this);
  SetFont(defaultFont);
  himl = ImageList_Create(16,16,ILC_MASK,256,512);
  TreeView_SetImageList(hwnd, himl, TVSIL_NORMAL); 
# endif
# ifdef GTK
# endif
  hwndFrame = hwndClient = hwnd;
  parent->AddChild(this);
  
  rootNode = new S_TreeViewNode((S_TreeView *)NULL,"Tree Root");
  rootNode->tv = this;
  
  return 0;
}


int
S_TreeView::Destroy()
{
  DestroyImageList();
  if ( parent )
    {
      parent->RemoveChild(this);
      parent = NULL;
    }
  if ( rootNode )
    {
      delete rootNode;
      rootNode = NULL;
    }
  S_Object::Destroy();
  
  Init();
  
  return 0;
}


S_TreeView::~S_TreeView()
{
  Destroy();
}


int
S_TreeView::Flush()
{
  if ( rootNode )
    {
      rootNode->DeleteChildren(TRUE);
      CreateImageList();
    }
  
  return 0;
}



int
S_TreeView::CreateImageList()
{
  DestroyImageList();
# ifdef WIN32
  himl = ImageList_Create(16,16,ILC_MASK,256,512);
  TreeView_SetImageList(hwnd, himl, TVSIL_NORMAL); 
# endif
  return 0;
}


int
S_TreeView::DestroyImageList()
{
# ifdef WIN32
  if ( himl )
    {
      ImageList_Destroy(himl);
      himl = NULL;
    }
# endif
  return 0;
}


S_TreeViewNode *
S_TreeView::GetNodeUnder(int x,int y) {
  S_TreeViewNode *node = NULL;
# ifdef WIN32
  TV_HITTESTINFO hti;
  hti.pt.x = x;
  hti.pt.y = y;
  HTREEITEM hItem;
  hItem = (HTREEITEM) SendMessage(hwnd,TVM_HITTEST,0,(LPARAM)&hti);
  if ( hItem ) 
    {
      TV_ITEM tvi;
      tvi.mask = TVIF_PARAM;
      tvi.hItem = hItem;
      SendMessage(hwnd,TVM_GETITEM,0,(LPARAM)&tvi);
      node = (S_TreeViewNode *)tvi.lParam;
    }
# endif
  return node;
}


int
S_TreeView::XwrtRoot()
{
  S_Object *obj = this;
  S_Window *rootWindow = NULL;
  int xr = x;
  do 
    {
      obj = obj->parent;
      if ( obj->type == S_OBJECT_WINDOW && ((S_Window *)obj)->rootWindow )
	rootWindow = (S_Window *)obj;
      if ( !rootWindow )
	xr += obj->x;
    } while ( rootWindow == NULL );
  return xr;
}


int
S_TreeView::YwrtRoot()
{
  S_Object *obj = this;
  S_Window *rootWindow = NULL;
  int yr = y;
  do
    {
      obj = obj->parent;
      if ( obj->type == S_OBJECT_WINDOW && ((S_Window *)obj)->rootWindow )
	rootWindow = (S_Window *)obj;
      if ( !rootWindow )
	yr += obj->y;
    } while ( rootWindow == NULL );
  return yr;
}
