#include "sf.h"
#include "SF_Font.h"

SF_Font::SF_Font(int icx,int icy,SF_Sprite *ispr) {
   cx = icx;
   cy = icy;
   spr = ispr;
   }

int
SF_Font::DrawString(S_Canvas *canvas,char *str,int x0,int y0,int frame) {
   int x = x0;
   int y = y0;
   for ( int i=0; i<(int)strlen(str); i++ ) {
     int ch = str[i]-32;
     x = x0 + i*cx;
     if ( ch >= 0 && ch < 96 && x > 0 )
        ((S_SystemBitmap *)spr)->Blit(canvas,frame,x,y-cy,x+cx+1,y,ch*cx,0);
      }
   return 0;
   }

SF_Font::~SF_Font() {
   }
