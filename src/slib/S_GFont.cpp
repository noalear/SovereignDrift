#include "S_OS.h"
#include "S_GFont.h"


int
S_GFont::Init() {
   height = 0;
   width = 0;
   return 0;
   }

S_GFont::S_GFont() {
   Init();
   }


int
S_GFont::ZCreate(char *filename,int iwidth,int iheight,int inumFrames) {
   S_SystemBitmap::ZCreate(filename,S_BITMAP_MASK_TRANSPARENT,
                           S_BITMAP_BLIT_USEMASK,inumFrames,100,0);
   width = iwidth;
   height = iheight;
   return 0;
   }


int
S_GFont::Create(char *filename,int iwidth,int iheight,int inumFrames) {
   S_SystemBitmap::Create(filename,S_BITMAP_MASK_TRANSPARENT,
                          S_BITMAP_BLIT_USEMASK,inumFrames,100,0);
   width = iwidth;
   height = iheight;
   return 0;
   }


int
S_GFont::DrawString(S_Canvas *canvas,char *str,int x0,int y0,int frame) {
   if ( canvas && str ) {
      int x = x0;
      int y = y0;
      for ( int i=0; i<(int)strlen(str); i++ ) {
        int ch = str[i]-32;
        x = x0 + i*width;
        if ( ch >= 0 && ch < 96 && x > 0 )
           S_SystemBitmap::Blit(canvas,frame,x,y-height,x+width,y,ch*width,0);
         }
      }
   return 0;
   }


int
S_GFont::Destroy() {
   S_SystemBitmap::Destroy();
   return 0;
   }

S_GFont::~S_GFont() {
   Destroy();
   }
