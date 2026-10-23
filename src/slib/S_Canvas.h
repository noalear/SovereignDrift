/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
#ifndef S_CANVAS
#define S_CANVAS

#ifdef OS2
   typedef HPS HDC;
   typedef ulong HFONT;
#  define SRCCOPY ROP_SRCCOPY
#endif
#ifdef WIN32
   typedef HDC HPS;
#  define ROP_SRCCOPY SRCCOPY
#endif
#ifdef UNIX
   typedef void *HDC;
#  define ROP_SRCCOPY 0
typedef void *HFONT;
#endif

class S_Font;
class S_Palette;

class S_Canvas
{
 public:
  HDC hdc;
  int xCanvas,yCanvas;
  int cxCanvas,cyCanvas;
  
  void *Lock();
  int Unlock();
  int Flip();
  
  S_Palette *oldPalette;
  S_Palette *palette;
  S_Palette *SetPalette(S_Palette * =NULL);
  int ResetPalette();
  
  RectStruct clipRect;
  int ResetClipRect();
  int SetClipRect(int,int,int,int);
  int SetAntiClipRect(int,int,int,int);
  
  int DrawPieWedge(int,int,int,int,int,int,ulong);
  int DrawFilledPieWedge(int,int,int,int,int,int,ulong);
  int DrawRect(int,int,int,int,ulong);
  int DrawRoundedRect(int,int,int,int,int,ulong);
  int DrawFilledRect(int,int,int,int,ulong);
  int DrawFilledRoundedRect(int,int,int,int,int,ulong);
  int DrawLine(int,int,int,int,int,ulong);
  int DrawPoint(int,int,ulong);
  SizeStruct DrawString(char *,int,int,ulong);
  SizeStruct GetStringSize(char *);
  int DrawCircle(int,int,int,ulong);
  int DrawFilledCircle(int,int,int,ulong);
  int DrawTriangle(int,int,int,int,int,int,ulong);
  
  int SetFgColor(slong);
  int SetBgColor(slong);
  
  int SystemBlit(S_Canvas *,int,int,int,int,int,int,ulong=ROP_SRCCOPY);
  int SystemStretchBlit(S_Canvas *,int,int,int,int,int,int,int,int,
		    ulong=ROP_SRCCOPY);
  
  int Capture(char *,int=0,int=0,int=0,int=0);
  
  /*int SetSystemPalette();
    int RestorePalette();*/
  
  S_Font *font;
  HFONT oldFont;
  int SetFont(S_Font * =NULL);
  
  ulong lAveCharWidth;
  ulong lMaxBaselineExt;
  ulong lMaxDescender;
  ulong lMaxAscender;
  
  int Init();
  S_Canvas();
  int Create();
  int Destroy();
  ~S_Canvas();

#  ifndef WIN32
      void *lpBuffer;
#  endif
#  ifdef WIN32
      LPDIRECTDRAWSURFACE lpBuffer;
      LPDIRECTDRAWCLIPPER lpClipper;
      DDSURFACEDESC ddsd;
#  endif
   };

#if defined(WIN32) || defined(UNIX)
#  define RGB_WHITE       0x00FFFFFFL
#  define RGB_BLACK       0x00000000L
#  define RGB_BLUE        0x000000FFL
#  define RGB_RED         0x00FF0000L
#  define RGB_PINK        0x00FF00FFL
#  define RGB_GREEN       0x0000FF00L
#  define RGB_CYAN        0x0000FFFFL
#  define RGB_YELLOW      0x00FFFF00L
#endif
#define RGB_PURPLE      0x00800080L
#define RGB_DARKGREEN   0x00008000L
#define RGB_PALEGREEN   0x0000CC00L
#define RGB_DARKRED     0x00800000L
#define RGB_PALERED     0x00CC0000L
#define RGB_LIGHTRED    0x00FF5555L
#define RGB_DARKBLUE    0x00000080L
#define RGB_PALEBLUE    0x000000CCL
#define RGB_DARKCYAN    0x00008080L
#define RGB_PALECYAN    0x0000CCCCL
#define RGB_DARKPINK    0x0008000FL
#define RGB_PALEPINK    0x00CC00CCL
#define RGB_BROWN       0x00808000L
#define RGB_PALEYELLOW  0x00C0C000L
#define RGB_DARKGRAY    0x00808080L
#define RGB_PALEGRAY    0x00CCCCCCL
#define RGB_ORANGE      0x00FF8000L
#define RGB_PALEORANGE  0x00FFCC40L
#define RGB_LIGHTBLUE   0x00A0A0FFL
#define RGB_BRIGHTBLUE  0x00CCCCFFL

#if defined(WIN32) || defined(UNIX)
#  define CLR_WHITE     RGB_WHITE
#  define CLR_BLACK     RGB_BLACK
#  define CLR_BLUE      RGB_BLUE
#  define CLR_RED       RGB_RED
#  define CLR_PINK      RGB_PINK
#  define CLR_GREEN     RGB_GREEN
#  define CLR_CYAN      RGB_CYAN
#  define CLR_YELLOW    RGB_YELLOW
#  define CLR_DARKGREEN RGB_DARKGREEN
#  define CLR_DARKRED   RGB_DARKRED
#  define CLR_DARKBLUE  RGB_DARKBLUE
#  define CLR_DARKCYAN  RGB_DARKCYAN
#  define CLR_DARKPINK  RGB_DARKPINK
#  define CLR_BROWN     RGB_BROWN
#  define CLR_DARKGRAY  RGB_DARKGRAY
#  define CLR_PALEGRAY  RGB_PALEGRAY
#endif

#endif
