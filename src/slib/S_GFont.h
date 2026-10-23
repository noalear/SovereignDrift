#ifndef SF_GFONT_INCLUDED
#define SF_GFONT_INCLUDED

#include "S_SystemBitmap.h"

class S_GFont : public S_SystemBitmap {
public:
   int height;
   int width;
   int DrawString(S_Canvas *,char *,int,int,int);

   int Init();
   S_GFont();
   int Create(char *,int,int,int);
   int ZCreate(char *,int,int,int);
   int Destroy();
   ~S_GFont();
   };

#endif
