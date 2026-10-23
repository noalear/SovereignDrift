#ifndef S_RADIOBUTTON
#define S_RADIOBUTTON

#include "S_Object.h"

class S_RadioButton : public S_Object {
public:
   int SetCheck(int);
   int IsChecked();

   S_RadioButton();
   S_RadioButton(S_Object *,int (*)(S_RadioButton *) = NULL,char* = NULL,
                 int=0,int=0,int=0,int=0);
   int Create(S_Object *,int (*)(S_RadioButton *) = NULL,char* = NULL,
          int=0,int=0,int=0,int=0);
   int Destroy();
   ~S_RadioButton();
   };

#endif
