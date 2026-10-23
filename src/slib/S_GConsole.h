#ifndef S_GCONSOLE
#define S_GCONSOLE

#include "S_GWidget.h"
#include "S_GWindow.h"
#include "S_GScrollBar.h"

class S_GConsole : public S_GWidget {
public:
   int scroll;
   int numLines;
   char **buffer;

   int xmargin;
   int ymargin;

   virtual int AddLine(char * ="");
   virtual int AppendLine(char * ="");
   virtual int AppendText(char *);

   int SetSliderHeight();
   int CreateScrollBar(char *,char *,char *,char *);
   S_GScrollBar scrollBar;

   int Flush();
   int SetLine(int=0,char * ="");

   int Draw(int=0);

   S_GConsole();
   int Create(S_GWindow *,int (*)(S_GConsole *),char *,int=0,int=0,int=-1,int=-1);
   int Destroy();
   ~S_GConsole();
   };


int S_GConsole_UpButtonHandler(S_GButton *);
int S_GConsole_DownButtonHandler(S_GButton *);
int S_GConsole_SliderHandler(S_GSlider *);
int S_GConsole_ScrollBarHandler(S_GScrollBar *);

#endif
