#ifndef S_SCROLLBAR
#define S_SCROLLBAR

#include "S_Object.h"

#if defined(OS2) || defined(WIN32)
#  define S_SCROLLBAR_VERTICAL   SBS_VERT
#  define S_SCROLLBAR_HORIZONTAL SBS_HORZ
#endif
#ifdef UNIX
#  define S_SCROLLBAR_VERTICAL   0
#  define S_SCROLLBAR_HORIZONTAL 1
#endif

#define S_SCROLLBAR_ENDSCROLL     2
#define S_SCROLLBAR_LINELEFT      3
#define S_SCROLLBAR_LINEUP        3
#define S_SCROLLBAR_LINERIGHT     4
#define S_SCROLLBAR_LINEDOWN      4
#define S_SCROLLBAR_PAGELEFT      5
#define S_SCROLLBAR_PAGEUP        5
#define S_SCROLLBAR_PAGERIGHT     6
#define S_SCROLLBAR_PAGEDOWN      6
#define S_SCROLLBAR_THUMBPOSITION 7
#define S_SCROLLBAR_THUMBTRACK    8


class S_ScrollBar : public S_Object {
public:
   S_ScrollBar();
   S_ScrollBar(S_Object *,int (*)(S_ScrollBar *) = NULL,
             int=0,int=0,int=0,int=0,int=S_SCROLLBAR_VERTICAL);
   int Create(S_Object *,int (*)(S_ScrollBar *) = NULL,
          int=0,int=0,int=0,int=0,int=S_SCROLLBAR_VERTICAL);
   int Destroy();
   ~S_ScrollBar();

   int GetValue();
   int SetValue(int);
   int SetBarSize(int);
   int SetRange(int,int);
   };

#endif
