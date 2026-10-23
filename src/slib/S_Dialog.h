#ifndef S_DIALOG
#define S_DIALOG

#include "S_Window.h"

class S_Dialog : public S_Window {
public:
   virtual int HandlePaintMsg();

   S_Dialog(S_Object *,char *,int (*)(S_Dialog *) = NULL,int = 0,
            int=S_WINDOW_DEFAULT,int=S_WINDOW_DEFAULT,
            int=S_WINDOW_DEFAULT,int=S_WINDOW_DEFAULT,
            ulong=S_WINDOW_DIALOG,void * =NULL);
   S_Dialog(S_Root *,char *,int (*)(S_Dialog *) = NULL,int = 0,
            int=S_WINDOW_DEFAULT,int=S_WINDOW_DEFAULT,
            int=S_WINDOW_DEFAULT,int=S_WINDOW_DEFAULT,
            ulong=S_WINDOW_DIALOG,void * =NULL);
   ~S_Dialog();
   };

#endif
