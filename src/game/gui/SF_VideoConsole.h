#ifndef SF_VIDEOCONSOLE_INCLUDED
#define SF_VIDEOCONSOLE_INCLUDED

#define SF_CONSOLELINE_TEXT       0
#define SF_CONSOLELINE_TEXTGAUGE1 1
#define SF_CONSOLELINE_TEXTGAUGE2 2

#define SF_CONSOLELINE_MAXLEN    64

class SF_GameEngine;
class SF_VideoConsole;

class SF_VideoConsoleLine {
public:
   int type;
   int x0,y0;
   int chx;

   char text[SF_CONSOLELINE_MAXLEN];
   int val1,val2;
   int update;
   ulong textColor;
   ulong val1Color;
   ulong val2Color;
   int maxTextWidth;

   SF_VideoConsoleLine();
   int Create(SF_VideoConsole *,int,int);
   ~SF_VideoConsoleLine();

   int Set(char *,ulong,int=-1);
   int Set(char *,ulong,int,ulong);
   int Set(char *,ulong,int,ulong,int,ulong);
   int Update();
   int Draw(S_CanvasBuffer *,int);

   SF_VideoConsole *parent;
   };


#define VIDEO_NAVIGATE   0
#define VIDEO_TARGET     1
#define VIDEO_STATUS     2
#define VIDEO_MESSAGE    3
#define VIDEO_PLAYERLIST 4
#define VIDEO_VIEW       5
#define VIDEO_RACELIST   6
#define VIDEO_SELECT     7
#define VIDEO_RANKLIST   8
#define VIDEO_PLANETLIST 9
#define VIDEO_HELP      10

class SF_Sprite;
class SF_SpriteList;

#define SF_VIDEOCONSOLE_NUMROWS 13
#define SF_VIDEOCONSOLE_MAXROWS 1024

class SF_VideoConsole {
public:

   ulong check;

   SF_Sprite *bmpBack;
   SF_Sprite *bmpButtons;
   S_CanvasBuffer *parentBuffer;
   S_CanvasBuffer *buffer;
   SF_SpriteList *iconArray;
   int x0,y0;
   int cx,cy;
   int mode;
   int scroll;
   int numHeaderLines;

   int icon;
   int iconX1,iconY1,iconX2,iconY2;
   int updateIcon;

   int visible;
   int redrawAll;

   int chx,chy,cyBuffer;
   SF_VideoConsoleLine lines[SF_VIDEOCONSOLE_MAXROWS];
   SF_GameEngine *ge;

   int SetBmpBack(SF_Sprite *);

   SF_VideoConsole(S_CanvasBuffer *,SF_GameEngine *,
                   int,int,int,int,int=1024);
   ~SF_VideoConsole();

   int Contains(int,int);
   int HandleMouseClick(int,int,int,int);

   S_Font *font;

   int SetVisible(int);
   int SetMode(int);
   int SetLine(int,char *,ulong,int=-1);
   int SetLine(int,char *,ulong,int,ulong);
   int SetLine(int,char *,ulong,int,ulong,int,ulong);
   int SetIcon(int=-1,int=0,int=0,int=0,int=0);
   int Clear();
   int Draw();
   int DrawIcon();
   int DrawBackGround();
   int DrawBackGround(int,int,int,int);
   int ScrollUp();
   int ScrollDown();
   int ScrollHome();
   int SetNumHeaderLines(int=0);
   };

#endif
