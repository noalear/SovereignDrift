/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
#ifndef S_TREEVIEWNODE
#define S_TREEVIEWNODE

#include "S_Tree.h"

#ifdef OS2
typedef HPOINTER HICON;
#endif
#ifdef UNIX
typedef void * HICON;
#endif

#define S_TREEVIEW_BEGINDRAG   1
#define S_TREEVIEW_ENDDRAG     2
#define S_TREEVIEW_SELCHANGED  3
#define S_TREEVIEW_MOUSECLICK  4
#define S_TREEVIEW_LMOUSECLICK 4
#define S_TREEVIEW_RMOUSECLICK 5

#define S_TREEVIEW_MOUSEDBLCLICK  6
#define S_TREEVIEW_LMOUSEDBLCLICK 6
#define S_TREEVIEW_RMOUSEDBLCLICK 7

class S_TreeView;
class S_SystemIcon;

class S_TreeViewNode : public S_Tree
{
 public:
  
  S_TreeView *tv;
  void *puser;
  slong  luser;
  
  int SetText(char *);
  int SetIcon(S_SystemIcon *);
  int SetIcon(HICON);
  S_SystemIcon *icon;
  HICON hIcon;  

# ifdef WIN32
  HTREEITEM hItem;
  int iconIndex;
# endif
  
  int AddGUINode();
  int RemoveGUINode();
  
  int DeleteChildren(int=FALSE);
  
  int Remove();
  int InsertBefore(S_TreeViewNode *);
  int InsertAfter(S_TreeViewNode *);
  
  int SetAsDropTarget();
  int Select();
  int HiLite();
  int Expand();
  int ExpandAll();
  int Collapse();
  int CollapseAll();
  
  int AddChild(S_TreeViewNode *);
  int RemoveChild(S_TreeViewNode *);
  
  S_TreeViewNode();
  int Init();
  S_TreeViewNode(char *,S_SystemIcon * =NULL);
  S_TreeViewNode(S_TreeViewNode *,char *,S_SystemIcon * =NULL);
  S_TreeViewNode(S_TreeView *,char *,S_SystemIcon * =NULL);
  int Create(char *,S_SystemIcon * =NULL);
  int Create(S_TreeViewNode *,char *,S_SystemIcon * =NULL);
  int Create(S_TreeView *,char *,S_SystemIcon * =NULL);
  int Destroy();
  ~S_TreeViewNode();
};

#endif
