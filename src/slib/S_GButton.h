#ifndef S_GBUTTON
#define S_GBUTTON

#include "S_GWidget.h"

class S_GButton : public S_GWidget {
public:
   int Draw(int=0);

   S_GButton();
   int Create(S_GWindow *,int (*)(S_GButton *),char *,int=0,int=0,int=-1,int=-1);
   int Destroy();
   ~S_GButton();

   int HandleMouseMsg();
   };

#endif
