#ifndef S_STDWINDOW
#define S_STDWINDOW

#include "S_Window.h"

class S_StdWindow : public S_Window {
public:
   S_StdWindow( S_Root *,char *,int (*)(S_StdWindow *)=NULL,int=0,
                int=S_WINDOW_DEFAULT,int=S_WINDOW_DEFAULT,
                int=S_WINDOW_DEFAULT,int=S_WINDOW_DEFAULT,
                ulong=S_WINDOW_STANDARD,void * =NULL );
   S_StdWindow( S_Object *,char *,int (*)(S_StdWindow *)=NULL,int=0,
                int=S_WINDOW_DEFAULT,int=S_WINDOW_DEFAULT,
                int=S_WINDOW_DEFAULT,int=S_WINDOW_DEFAULT,
                ulong=S_WINDOW_STANDARD,void * =NULL );
   ~S_StdWindow();
   };

#endif
