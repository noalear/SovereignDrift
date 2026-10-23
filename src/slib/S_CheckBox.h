#ifndef S_CHECKBOX
#define S_CHECKBOX

#include "S_Object.h"

class S_CheckBox : public S_Object {
public:
   int SetCheck(int);
   int IsChecked();

   S_CheckBox();
   S_CheckBox(S_Object *,int (*)(S_CheckBox *) = NULL,char* = NULL,
             int=0,int=0,int=0,int=0);
   int Create(S_Object *,int (*)(S_CheckBox *) = NULL,char* = NULL,
          int=0,int=0,int=0,int=0);
   int Destroy();
   ~S_CheckBox();
   };

#endif
