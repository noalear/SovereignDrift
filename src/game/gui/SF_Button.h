#ifndef SF_BUTTON_INCLUDED
#define SF_BUTTON_INCLUDED

#define SF_BUTTON_UP    0
#define SF_BUTTON_DOWN  1
#define SF_BUTTON_FLASH 2

class SF_DisplayEngine;

class SF_Button
{
 public:
  int visible;
  int mode;
  SF_DisplayEngine *de;
  char *text;
  ulong textColor;
  ulong lightColor;
  ulong darkColor;
  ulong fillColor;
  int x1,y1,x2,y2;

  uchar msg[5];
  uchar ch[5];

  int Contains(int,int);
  int HandleClick(int=SF_BUTTON_DOWN);
  int Draw(int=-1);

  int SetProperties(char *,
		uchar,uchar,uchar,uchar,uchar,
		uchar,uchar,uchar,uchar,uchar);

  SF_Button(SF_DisplayEngine *,char *,int,int,int,int,
	    ulong=RGB_GREEN,ulong=RGB_DARKGRAY,
	    ulong=RGB_BLACK,ulong=0xFFFFFFFF,
	    uchar=0,uchar=0,uchar=0,uchar=0,uchar=0,
	    uchar=0,uchar=0,uchar=0,uchar=0,uchar=0);
  ~SF_Button();
};

#endif
