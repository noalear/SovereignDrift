#ifndef S_GBMPLISTBOX
#define S_GBMPLISTBOX

#include "S_SystemBitmap.h"
#include "S_GWidget.h"
#include "S_GWindow.h"
#include "S_GScrollBar.h"

class S_GBmpListBox : public S_GWidget {
public:
   int scroll;
   int numLines;
   int selectedLine;
   S_SystemBitmap **bmp;

   virtual int AddItem(S_SystemBitmap *);

   int SetSliderHeight();
   int CreateScrollBar(char *,char *,char *,char *);
   S_GScrollBar scrollBar;

   int SetLine(int=0,S_SystemBitmap * =NULL);

   int Draw(int=0);

   S_GBmpListBox();
   int Create(S_GWindow *,int (*)(S_GBmpListBox *),char *,
          int=0,int=0,int=-1,int=-1);
   int Destroy();
   ~S_GBmpListBox();

   int HandleMouseMsg();
   };

int S_GBmpListBox_UpButtonHandler(S_GButton *);
int S_GBmpListBox_DownButtonHandler(S_GButton *);
int S_GBmpListBox_SliderHandler(S_GSlider *);
int S_GBmpListBox_ScrollBarHandler(S_GScrollBar *);

#endif
