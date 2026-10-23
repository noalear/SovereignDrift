#ifndef S_GTEXTBOX
#define S_GTEXTBOX

#include "S_GWidget.h"
#include "S_GWindow.h"
#include "S_String.h"

class S_GTextBox : public S_GWidget {
public:
   int scroll;
   int cursor;

   S_String buffer;

   int xmargin;
   int ymargin;

   int SetText(char *);
   int Flush();

   int Draw(int=0);

   int Init();
   S_GTextBox();
   int Create(S_GWindow *,int (*)(S_GTextBox *),char *,int=0,int=0,int=-1,int=-1);
   int Destroy();
   ~S_GTextBox();

   int HandleKeyboardMsg();
   int HandleMouseMsg();
   };

#endif
