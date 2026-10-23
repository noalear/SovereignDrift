#ifndef S_TREEVIEW
#define S_TREEVIEW

#include "S_Object.h"

class S_TreeViewNode;

class S_TreeView : public S_Object
{
 public:
  
  int Flush();
  int CreateImageList();
  int DestroyImageList();
# ifdef WIN32
  HIMAGELIST himl;
# endif
  
  int Init();
  S_TreeView();
  S_TreeView(S_Object *,int (*)(S_TreeView *) = NULL,
	     int=0,int=0,int=0,int=0);
  int Create(S_Object *,int (*)(S_TreeView *) = NULL,
	 int=0,int=0,int=0,int=0);
  int Destroy();
  ~S_TreeView();
  
  S_TreeViewNode *rootNode;
  
  S_TreeViewNode *dragObject;
  S_TreeViewNode *dropTarget;
  S_TreeViewNode *GetNodeUnder(int,int);
  int XwrtRoot();
  int YwrtRoot();
};

#endif
