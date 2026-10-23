#ifndef S_DIRECTORYTREE
#define S_DIRECTORYTREE

#include "S_Tree.h"

class S_DirectoryTree : public S_Tree {
public:
   char *rootDir;

   S_DirectoryTree(char *);
   ~S_DirectoryTree();
   };

#endif
