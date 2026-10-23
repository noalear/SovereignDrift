#ifndef SF_FONT_INCLUDED
#define SF_FONT_INCLUDED

class SF_Sprite;

class SF_Font {
public:
   int cx;
   int cy;
   SF_Sprite *spr;

   int DrawString(S_Canvas *,char *,int,int,int);

   SF_Font(int,int,SF_Sprite *);
   ~SF_Font();
   };

#endif
