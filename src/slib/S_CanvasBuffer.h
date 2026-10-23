/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
#ifndef S_CANVASBUFFER
#define S_CANVASBUFFER

#include "S_Canvas.h"

#define S_CANVASBUFFER_OK              0
#define S_CANVASBUFFER_BLIT_RECTANGLES 1
#define S_CANVASBUFFER_BLIT_ALL        2
#define S_CANVASBUFFER_TIMER_LOWRES    4
#define S_CANVASBUFFER_TIMER_HIRES     8
#define S_CANVASBUFFER_NORECTANGLE    16
#define S_CANVASBUFFER_ADDRECTANGLE   32
#define S_CANVASBUFFER_RECT_FILL      64
#define S_CANVASBUFFER_RECT_OUTLINE  128
#define S_CANVASBUFFER_RECT_ROUNDED  256

#ifdef UNIX
   typedef void *HBITMAP;
#endif

class S_Bitmap;
class S_SystemBitmap;

class S_CanvasBuffer : public S_Canvas {
public:

   HBITMAP hbm;
   HBITMAP ohbm;
   ulong visible;
   unsigned short numBits;

   RectStruct position;

   RectStruct *blitRect;
   unsigned short numBlitRects;
   RectStruct *oldRect;
   unsigned short numOldRects;
   RectStruct *rect;
   unsigned short numRects;
   unsigned short maxRects;

   RectStruct *embeddedBufferRect;
   unsigned short numEmbeddedBuffers;
   unsigned short maxEmbeddedBuffers;
   S_CanvasBuffer **embeddedBuffer;
   S_CanvasBuffer *parentBuffer;

   int Init();
   S_CanvasBuffer();
   S_CanvasBuffer(int,int,int);
   int Create(int,int,int);
   int Destroy();
   ~S_CanvasBuffer();

   int Embed(S_CanvasBuffer *,int,int);

   int Blit(S_Canvas *,int,int,ulong);

   int FillInRects(ulong,ulong);
   int BlitInRects(S_Bitmap *,ulong);
   int BlitInRects(S_SystemBitmap *,ulong);
   int BlitInRects(S_Canvas *,ulong);

   int AddRect(int,int,int,int);
   int ResetRects(int=TRUE);

   int DrawPieWedge(int,int,int,int,int,int,ulong,
                ulong=S_CANVASBUFFER_NORECTANGLE);
   int DrawFilledPieWedge(int,int,int,int,int,int,ulong,
                      ulong=S_CANVASBUFFER_NORECTANGLE);
   int DrawRect(int,int,int,int,ulong,ulong=S_CANVASBUFFER_NORECTANGLE);
   int DrawFilledRect(int,int,int,int,ulong,ulong=S_CANVASBUFFER_NORECTANGLE);
   int DrawRoundedRect(int,int,int,int,int,ulong,ulong=S_CANVASBUFFER_NORECTANGLE);
   int DrawFilledRoundedRect(int,int,int,int,int,ulong,
                         ulong=S_CANVASBUFFER_NORECTANGLE);
   int DrawLine(int,int,int,int,int,ulong,ulong=S_CANVASBUFFER_NORECTANGLE);
   SizeStruct DrawString(char *,int,int,ulong,
                         ulong=S_CANVASBUFFER_NORECTANGLE);
   int DrawCircle(int,int,int,ulong,ulong=S_CANVASBUFFER_NORECTANGLE);
   int DrawFilledCircle(int,int,int,ulong,ulong=S_CANVASBUFFER_NORECTANGLE);
   int DrawTriangle(int,int,int,int,int,int,ulong,
                ulong=S_CANVASBUFFER_NORECTANGLE);

   int Show(int);
   
   };

#endif
