/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
#include "S_OS.h"
#include "S_TreeViewNode.h"
#include "S_TreeView.h"
#include "S_SystemIcon.h"


S_TreeViewNode::S_TreeViewNode()
{
  Init();
}


int
S_TreeViewNode::Init()
{
  tv = NULL;
  icon = NULL;
  puser = NULL;
  luser = 0;
# ifdef WIN32
  hItem = NULL;
  iconIndex = 0;
# endif
  return 0;
}


S_TreeViewNode::S_TreeViewNode(S_TreeView *itv,char *iname,
                               S_SystemIcon *iicon)
{
  Create(itv,iname,iicon);
}


int
S_TreeViewNode::Create(S_TreeView *itv,char *iname,S_SystemIcon *iicon)
{
  S_Tree::Create(iname);
  Init();
  icon = iicon;
  tv = itv;
  if ( tv )
    {
      tv->rootNode->AddChild(this);
      AddGUINode();
    }
  return 0;
}


S_TreeViewNode::S_TreeViewNode(S_TreeViewNode *iparent,char *iname,
                               S_SystemIcon *iicon)
{
  Create(iparent,iname,iicon);
}


int
S_TreeViewNode::Create(S_TreeViewNode *iparent,char *iname,
                       S_SystemIcon *iicon)
{
  S_Tree::Create(iname);
  Init();
  icon = iicon;
  if ( iparent )
    iparent->AddChild(this);
  return 0;
}


S_TreeViewNode::S_TreeViewNode(char *iname,S_SystemIcon *iicon)
{
  Create(iname,iicon);
}


int
S_TreeViewNode::Create(char *iname,S_SystemIcon *iicon)
{
  S_Tree::Create(iname);
  Init();
  icon = iicon;
  return 0;
}


int
S_TreeViewNode::AddChild(S_TreeViewNode *child)
{
  if ( child )
    {
      child->tv = tv;
      S_Tree::AddChild(child);
      if ( child->icon )
	{
	  child->SetIcon(child->icon);
	}
      child->AddGUINode();
    }
  return 0;
}


int
S_TreeViewNode::InsertBefore(S_TreeViewNode *sibling)
{
  if ( sibling )
    {
      tv = sibling->tv;
      S_Tree::InsertBefore(sibling);
      SetIcon(icon);
      AddGUINode();
    }
  return 0;
}


int
S_TreeViewNode::InsertAfter(S_TreeViewNode *sibling)
{
  if ( sibling )
    {
      tv = sibling->tv;
      S_Tree::InsertAfter(sibling);
      SetIcon(icon);
      AddGUINode();
    }
  return 0;
}


int
S_TreeViewNode::SetIcon(S_SystemIcon *iicon)
{
  icon = iicon;
  if ( tv && icon && icon->hIcon )
    {
      SetIcon(icon->hIcon);
    }
  return 0;
}


int
S_TreeViewNode::SetIcon(HICON ihIcon)
{
# ifdef WIN32
  hIcon = ihIcon;
  if ( tv && hIcon ) {
    if ( !iconIndex )
      {
	iconIndex = ImageList_AddIcon(tv->himl,hIcon); 
      }
    else
      {
	ImageList_ReplaceIcon(tv->himl,iconIndex,hIcon);
      }
  }
# endif
  return 0;
}


int
S_TreeViewNode::SetText(char *text)
{
# ifdef WIN32
  if ( tv && hItem )
    {
      TV_ITEM tvi;
      tvi.mask = TVIF_TEXT;
      tvi.hItem = hItem;
      tvi.pszText = text;
      SendMessage(tv->hwnd,TVM_SETITEM,0,(LPARAM)&tvi);
    }
# endif
  return 0;
}


int
S_TreeViewNode::AddGUINode()
{
  
  if ( !name )
    name = S_strdup("noname");
  
# ifdef WIN32
  TV_ITEM tvi;
  TV_INSERTSTRUCT tvins;
  
  tvi.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
  if ( iconIndex )
    {
      tvi.mask |= TVIF_IMAGE | TVIF_SELECTEDIMAGE;
    }
  else
    {
      tvi.state = INDEXTOSTATEIMAGEMASK(0);
      tvi.stateMask = TVIS_STATEIMAGEMASK;
    }
  tvi.pszText = name;
  tvi.cchTextMax = strlen(name);
  tvi.iImage = iconIndex;
  tvi.iSelectedImage = iconIndex;
  tvi.lParam = (slong)this;
  
  tvins.item = tvi;
  if ( this->prev )
    tvins.hInsertAfter = ((S_TreeViewNode *)this->prev)->hItem;
  else
    tvins.hInsertAfter = TVI_FIRST;
  
  if ( parent && ((S_TreeViewNode *)parent)->hItem )
    tvins.hParent = ((S_TreeViewNode *)parent)->hItem;
  else
    tvins.hParent = TVI_ROOT;
  
  if ( tv && IsWindow(tv->hwnd) )
    hItem = (HTREEITEM)SendMessage(tv->hwnd,TVM_INSERTITEM,
				   0,(LPARAM)&tvins);
# endif
  
  HiLite();
  
  return 0;
}


int
S_TreeViewNode::RemoveGUINode()
{
# ifdef WIN32
  if ( tv && hItem && IsWindow(tv->hwnd) ) {
    SendMessage(tv->hwnd,TVM_DELETEITEM,0,(LPARAM)hItem);
    hItem = NULL;
  }
# endif
  return 0;
}


int
S_TreeViewNode::DeleteChildren(int recursively)
{
  while ( child )
    {
      if ( recursively )
	{
	  ((S_TreeViewNode *)child)->DeleteChildren(recursively);
	}
      delete ((S_TreeViewNode *)child);
    }
  return 0;
}


int
S_TreeViewNode::Destroy()
{
  Remove();
  S_Tree::Destroy();
  return 0;
}


S_TreeViewNode::~S_TreeViewNode()
{
  Destroy();
}


int
S_TreeViewNode::Remove()
{
  RemoveGUINode();
  if ( tv && tv->dropTarget == this )
    {
      tv->dropTarget = NULL;
    }
  if ( tv && tv->dragObject == this )
    {
      tv->dragObject = NULL;
    }
  S_Tree::Remove();
  return 0;
}


int
S_TreeViewNode::RemoveChild(S_TreeViewNode *ichild)
{
  ((S_Tree *)ichild)->Remove();
  return 0;
}


int
S_TreeViewNode::SetAsDropTarget()
{
  if ( tv )
    tv->dropTarget = this;
  HiLite();
  Select();
  return 0;
}


int
S_TreeViewNode::Select()
{
# ifdef WIN32
  if ( tv && hItem )
    SendMessage(tv->hwnd,TVM_SELECTITEM,TVGN_CARET,(LPARAM)hItem);
# endif
  return 0;
}


int
S_TreeViewNode::HiLite()
{
# ifdef WIN32
  if ( tv && hItem )
    SendMessage(tv->hwnd,TVM_SELECTITEM,TVGN_DROPHILITE,(LPARAM)hItem);
# endif
  return 0;
}


int
S_TreeViewNode::Expand()
{
# ifdef WIN32
  if ( tv && hItem)
    SendMessage(tv->hwnd,TVM_EXPAND,TVE_EXPAND,(LPARAM)hItem);
# endif
  return 0;
}


int
S_TreeViewNode::Collapse() {
# ifdef WIN32
  if ( tv && hItem )
    SendMessage(tv->hwnd,TVM_EXPAND,TVE_COLLAPSE,(LPARAM)hItem);
# endif
  return 0;
}


int
S_TreeViewNode::ExpandAll()
{
# ifdef WIN32
  for ( S_TreeViewNode *tvn = (S_TreeViewNode *)child; 
	tvn!=NULL; 
	tvn=(S_TreeViewNode *)tvn->next )
    {
      tvn->ExpandAll();
    }
  if ( tv && hItem)
    SendMessage(tv->hwnd,TVM_EXPAND,TVE_EXPAND,(LPARAM)hItem);
# endif
  return 0;
}


int
S_TreeViewNode::CollapseAll() {
# ifdef WIN32
  for ( S_TreeViewNode *tvn = (S_TreeViewNode *)child; 
	tvn!=NULL; 
	tvn=(S_TreeViewNode *)tvn->next )
    {
      tvn->CollapseAll();
    }
  if ( tv && hItem )
    SendMessage(tv->hwnd,TVM_EXPAND,TVE_COLLAPSE,(LPARAM)hItem);
# endif
  return 0;
}

