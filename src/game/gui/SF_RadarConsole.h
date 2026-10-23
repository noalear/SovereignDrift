#ifndef SF_RADARCONSOLE_INCLUDED
#define SF_RADARCONSOLE_INCLUDED

class SF_Sprite;

class SF_RadarConsole {
public:

   int x0;
   int y0;
   int visible;
   int redrawAll;

   int zoom;

   SF_Sprite *bmpBack;
   S_CanvasBuffer *parentBuffer;
   S_CanvasBuffer *buffer;
   S_Font *font;

   SF_RadarConsole(S_CanvasBuffer *,SF_Sprite *,int,int);
   ~SF_RadarConsole();

   int Draw();
   int SetVisible(int);
   int DrawBackGround(int,int,int,int);
   int DrawBackGround(ulong);

   };

#endif
