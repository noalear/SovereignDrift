/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
#ifndef S_FONT
#  define S_FONT

#  ifdef WIN32
#     define S_FONT_ARIAL   "Arial"
#     define S_FONT_HELV    "Arial"
#     define S_FONT_COURIER "Courier"
#     define S_FONT_NORMAL  FW_NORMAL
#     define S_FONT_BOLD    FW_BOLD
#  endif
#  ifdef OS2
      typedef ulong HFONT;
#     define S_FONT_ARIAL "Helv"
#     define S_FONT_HELV  "Helv"
#     define S_FONT_COURIER "Courier"
#     define S_FONT_NORMAL  0
#     define S_FONT_BOLD    0
#  endif
#  ifdef UNIX
      typedef void *HFONT;
#     define S_FONT_ARIAL "Helv"
#     define S_FONT_HELV  "Helv"
#     define S_FONT_COURIER "Courier"
#     define S_FONT_NORMAL  0
#     define S_FONT_BOLD    0
#  endif

   class S_Font {
   public:
      HFONT hFont;
      slong id;

      char *faceName;
      int pointSize;
      int cx;
      int cy;
      int angle;

      int Init();
      S_Font();
      S_Font(char *,int,int=0,int=S_FONT_NORMAL);
      S_Font(char *,int,int,int,int);
      int Create(char *,int,int=0,int=S_FONT_NORMAL);
      int Create(char *,int,int,int,int);
      int Destroy();
      ~S_Font();
      };

#endif
