#ifndef S_GSCROLLBAR
#define S_GSCROLLBAR

#define S_GSCROLLBAR_VERTICAL    0
#define S_GSCROLLBAR_HORIZONTAL  1

#define S_GSCROLLBAR_PAGEUP   1
#define S_GSCROLLBAR_PAGEDOWN 2

#include "S_GWidget.h"
#include "S_GWindow.h"
#include "S_GButton.h"
#include "S_GSlider.h"

class S_GScrollBar : public S_GWidget {
public:
   int scroll;
   int maxScroll;

   int CreateSlider(char *,int (*)(S_GSlider *));
   int CreateUpButton(char *,int (*)(S_GButton *));
   int CreateDownButton(char *,int (*)(S_GButton *));

   S_GButton upButton;
   S_GButton downButton;
   S_GSlider slider;

   int options;

   int Draw(int=0);

   S_GScrollBar();
   int Create(S_GWindow *,int (*)(S_GScrollBar *),char *,
          int=0,int=0,int=-1,int=-1,int=0);
   int Destroy();
   ~S_GScrollBar();

   int HandleMouseMsg();
   };

#endif
