#ifndef S_WBWINDOW
#define S_WBWINDOW

#include "S_Window.h"

class S_WBWindow : public S_Window
{
 public:
  
  int Init();
  S_WBWindow();
  int Create(S_Root *,S_Object *,char *,int (*)(S_Window *) = NULL,int = 0,
	     int=S_WINDOW_DEFAULT,int=S_WINDOW_DEFAULT,
	     int=S_WINDOW_DEFAULT,int=S_WINDOW_DEFAULT,
	     ulong=S_WINDOW_STANDARD,void * =NULL);
  int Destroy();
  ~S_WBWindow();
};

#endif
