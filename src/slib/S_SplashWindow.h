#ifndef S_SPLASHWINDOW
#define S_SPLASHWINDOW

#include "S_Window.h"

class S_SystemBitmap;

class S_SplashWindow : public S_Window {
public:
   S_SystemBitmap *bmp;

   S_SplashWindow( S_Root *,char *,S_SystemBitmap *);
   S_SplashWindow( S_Object *,char *,S_SystemBitmap *);
   ~S_SplashWindow();
   };

#endif

