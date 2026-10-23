#ifndef SF_MAPCONSOLE_INCLUDED
#define SF_MAPCONSOLE_INCLUDED

class SF_Sprite;

class SF_MapConsole {
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

   int SetBmpBack(SF_Sprite *);
   SF_MapConsole(S_CanvasBuffer *,int,int);
   ~SF_MapConsole();

   int Draw();
   int SetVisible(int);
   int Contains(int,int);

   };

#endif
