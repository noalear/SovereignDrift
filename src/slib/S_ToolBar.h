#ifndef S_TOOLBAR
#define S_TOOLBAR

#include "S_Object.h"

#define S_TOOLBAR_NOMSG         0
#define S_TOOLBAR_BUTTONPRESSED 1

class S_SystemIcon;

class S_ToolBar : public S_Object
{
 public:
  
  int numButtons;
  int AddButton(S_SystemIcon *,char *,int=0);
  int AddSeparator(int=4);

  int SetButtonEnabled(int,int);

  int Flush();
  int CreateImageList();
  int DestroyImageList();
# ifdef WIN32
  HIMAGELIST himl;
  int AddButton(HICON,char *,int=0);
# endif
  int CheckButton(int,int);
  
  int Init();
  S_ToolBar();
  S_ToolBar(S_Object *,int (*)(S_ToolBar *) = NULL,
	    int=0,int=0,int=0,int=0,int=16);
  int Create(S_Object *,int (*)(S_ToolBar *) = NULL,
	 int=0,int=0,int=0,int=0,int=16);
  int Destroy();
  ~S_ToolBar();
};

#endif
