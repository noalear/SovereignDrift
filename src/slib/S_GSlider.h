#ifndef S_GSLIDER
#define S_GSLIDER

#include "S_GWidget.h"

class S_GSlider : public S_GWidget {
public:
   int dragging;
   int x0;
   int y0;
   int dx;
   int dy;

   int SetRange(int,int);
   int value;
   int minValue;
   int maxValue;

   int SetValue(int);
   int SetPosition(int,int);

   int Draw(int=0);

   int SetDragBounds(int,int,int,int);
   int xLeft,xRight,yTop,yBottom;

   S_GSlider();
   int Create(S_GWindow *,int (*)(S_GSlider *),char *,int=0,int=0,int=-1,int=-1);
   int Destroy();
   ~S_GSlider();

   int HandleMouseMsg();
   };

#endif
