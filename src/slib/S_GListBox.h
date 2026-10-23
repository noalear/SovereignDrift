#ifndef S_GLISTBOX
#define S_GLISTBOX

#include "S_GConsole.h"

class S_GListBox : public S_GConsole {
public:
   ulong highlightColor;

   int mouseOverLine;
   int selectedLine;
   int AddLine(char * ="");

   int Init();
   S_GListBox();
   int Create(S_GWindow *,int (*)(S_GListBox *),char *,int=0,int=0,int=-1,int=-1);
   ~S_GListBox();

   int Draw(int=0);

   int HandleMouseMsg();
   };

#endif
