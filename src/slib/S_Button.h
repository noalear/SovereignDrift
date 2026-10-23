#ifndef S_BUTTON
#define S_BUTTON

#include "S_Object.h"

#ifdef OS2
#  define S_BUTTON_DEFAULT BS_DEFAULT
#endif
#ifdef WIN32
#  define S_BUTTON_DEFAULT BS_DEFPUSHBUTTON
#endif

class S_Button : public S_Object
{
 public:
  int SetDefault();
  
  S_Button();
  S_Button(S_Object *,int (*)(S_Button *) = NULL,char* = NULL,
	   int=0,int=0,int=0,int=0,int=0);
  int Create(S_Object *,int (*)(S_Button *) = NULL,char* = NULL,
	     int=0,int=0,int=0,int=0,int=0);
  int Destroy();
  ~S_Button();
};

#endif
