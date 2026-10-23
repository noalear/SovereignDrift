#ifndef  S_COMBOBOX
#define S_COMBOBOX

#include "S_Object.h"

class S_ComboBox : public S_Object {
public:
   S_ComboBox();
   S_ComboBox(S_Object *,int (*)(S_ComboBox *) = NULL,
              int=0,int=0,int=0,int=0);
   int Create(S_Object *,int (*)(S_ComboBox *) = NULL,int=0,int=0,int=0,int=0);
   int Destroy();
   virtual ~S_ComboBox();

   int AddItem(char *);
   int InsertItem(char *,int);
   int RemoveItem(int);
   int GetIndex();
   int SetIndex(int);
   int SetItem(char *);
   int Flush();

   char *GetItem();
   };

#endif
