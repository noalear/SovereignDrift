#ifndef S_GWINDOW
#define S_GWINDOW

#include "S_Tree.h"
#include "S_Window.h"
#include "S_SystemBitmap.h"

class S_GWindow : public S_Window,public S_SystemBitmap {
public:
   int draggable;

   int dragging;
   int x0;
   int y0;
   int dx;
   int dy;

   S_Tree widgets;

   int Init();
   S_GWindow();
   int Create(S_Root *,S_GWindow *,char *,char *,int,int,int,int=-1,int=-1,
          ulong=S_WINDOW_BLANK);
   int Destroy();
   virtual ~S_GWindow();

   virtual int Draw(int=0);

   virtual int HandleCreateMsg();
   virtual int HandleKeyboardMsg();
   virtual int HandleMouseMsg();
   virtual int HandlePaintMsg();
   virtual int HandleCloseMsg();
   };

#endif
