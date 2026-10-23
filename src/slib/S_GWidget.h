#ifndef S_GWIDGET
#define S_GWIDGET

class S_GWindow;

#include "S_Font.h"
#include "S_Tree.h"
#include "S_GFont.h"
#include "S_Canvas.h"
#include "S_SystemBitmap.h"

class S_GWidget : public S_Tree,public S_SystemBitmap {
public:
   S_GWindow *parent;
   S_GWidget *wparent;

   int SetFont(S_Font * =NULL);
   S_Font *font;

   int SetGFont(S_GFont * =NULL);
   S_GFont *gfont;

   int msg;

   int x;
   int y;

   ulong color;
   int SetTextColor(ulong=RGB_GREEN);

   ulong gcolor;
   int SetGTextColor(ulong=0);

   virtual int SetPosition(int,int);
   virtual int SetSize(int,int);

   int height;
   int width;

   int mouseIsOver;
   int leftMouseButtonIsDown;
   int rightMouseButtonIsDown;
   int middleMouseButtonIsDown;

   int Contains(int,int);

   int toggle;
   int SetToggle(int=TRUE);

   int enabled;
   int SetEnabled(int=TRUE);

   int visible;
   int Hide();
   int Show();
   virtual int Draw(int=0);

   int Init();
   S_GWidget();
   int Create(S_GWindow *,int (*)(S_GWidget *),char *,int=1,
          int=0,int=0,int=-1,int=-1);
   int Destroy();
   virtual ~S_GWidget();

   int (*pMsgHandler)(void *);

   virtual int HandleCreateMsg();
   virtual int HandleKeyboardMsg();
   virtual int HandleMouseMsg();
   virtual int HandlePaintMsg();
   virtual int HandleCloseMsg();
   };

#endif
