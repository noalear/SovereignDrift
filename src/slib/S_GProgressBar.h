#ifndef S_GPROGRESSBAR
#define S_GPROGRESSBAR

#include "S_GWidget.h"
#include "S_GWindow.h"

class S_GProgressBar : public S_GWidget {
public:
   int percent;
   int margin;

   int Draw(int=-1);

   S_GProgressBar();
   int Create(S_GWindow *,int (*)(S_GProgressBar *),char *,
          int=0,int=0,int=-1,int=-1);
   int Destroy();
   ~S_GProgressBar();
   };

#endif
