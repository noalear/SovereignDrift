#ifndef S_PIXELBUFFER_INCLUDED
#define S_PIXELBUFFER_INCLUDED

#define S_PIXELBUFFER_8BIT    8
#define S_PIXELBUFFER_16BIT  16
#define S_PIXELBUFFER_24BIT  24
#define S_PIXELBUFFER_32BIT  32

class S_BitmapData;
class S_Canvas;

class S_PixelBuffer {
public:
   ulong cx;
   ulong cy;
   ulong bits;
   ulong infoBytes;
   ulong bufferSize;
   char *buffer;

   BitmapInfo3Struct *pbmi;
   ushort *ppal;
   Rgb2Struct *prgbpal;

   int SystemBlit(S_Canvas *,int,int,int,int,int,int);
   int SystemBlit(S_Canvas *,int,int,int,int,int,int,int,int);

   int CreateBuffer(int,int,int);
   int DestroyBuffer();

   S_PixelBuffer(char *);
   S_PixelBuffer(S_BitmapData *);
   S_PixelBuffer(ulong,ulong,ulong=S_PIXELBUFFER_8BIT);
   ~S_PixelBuffer();
   };


#endif
