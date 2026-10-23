#ifndef S_NOTEBOOK
#define S_NOTEBOOK

#include "S_Object.h"

class S_SystemIcon;

class S_Notebook : public S_Object {
public:
#  ifdef WIN32
      HIMAGELIST himl;
#  endif

   int Init();
   S_Notebook();
   S_Notebook(S_Object *,int (*)(S_Notebook *) = NULL,int=0,int=0,int=0,int=0);
   int Create(S_Object *,int (*)(S_Notebook *) = NULL,int=0,int=0,int=0,int=0);
   int Destroy();
   ~S_Notebook();

   int AddItem(char *,S_SystemIcon * =NULL);
   int InsertItem(char *,int,S_SystemIcon * =NULL);
   int RemoveItem(int);
   int GetIndex();
   int SetIndex(int);
   int Flush();
   };

#endif
