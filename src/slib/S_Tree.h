#ifndef S_TREE
#define S_TREE

class S_Tree
{
 public:
  char *name;
  int type;
  int level;
  int count;
  
  S_Tree *prev;
  S_Tree *next;
  
  int Remove();
  int InsertBefore(S_Tree *);
  int InsertAfter(S_Tree *);
  
  S_Tree *parent;
  S_Tree *child;

  S_Tree *GetChildNamed(char *);
  
  int DeleteChildren();
  int AddChild(S_Tree *);
  
  int Init();
  S_Tree(char * =NULL,int=0);
  int Create(char * =NULL,int=0);
  int Destroy();
  virtual ~S_Tree();
};

#endif
