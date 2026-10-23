#ifndef S_TEXTBOX
#define S_TEXTBOX

#include "S_Object.h"

#ifdef OS2
#  define S_TEXTBOX_UNREADABLE ES_UNREADABLE
#endif
#ifdef WIN32
#  define S_TEXTBOX_UNREADABLE ES_PASSWORD	
#endif
#ifdef GTK
#  define S_TEXTBOX_UNREADABLE 0
#endif

class S_TextBox : public S_Object
{
 public:
  int Create(S_Object *,int (*)(S_TextBox *) = NULL,char* = NULL,
	     int=0,int=0,int=0,int=0,ulong=0);
  int Destroy();
  
  S_TextBox();
  S_TextBox(S_Object *,int (*)(S_TextBox *) = NULL,char* = NULL,
	    int=0,int=0,int=0,int=0,ulong=0);
  ~S_TextBox();
  
  char *GetString();
  double GetDouble();
  int SetReadOnly(int);
  
# ifdef OS2
  int SetSize(int,int);
  int SetPosition(int,int);
# endif
};

#endif
