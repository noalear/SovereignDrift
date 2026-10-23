/* S_PortStubs.cpp -- NEW FILE, added 2026-10-12 for the Stellar Frontier
   Godot port.  S_Window's Win32 message handler refers to tree-view members
   for drag-and-drop between tree views.  Stellar Frontier has no tree views,
   so the tree-view classes are not built; these definitions satisfy the
   linker and are never called. */
#include "S_OS.h"
#include "S_TreeView.h"
#include "S_TreeViewNode.h"

S_TreeViewNode *S_TreeView::GetNodeUnder(int, int) { return NULL; }
int S_TreeViewNode::HiLite() { return 0; }
int S_TreeViewNode::SetAsDropTarget() { return 0; }
