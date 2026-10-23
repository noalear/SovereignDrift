/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
#ifndef S_BITMAPDATA
#define S_BITMAPDATA

#define S_BITMAPDATA_NOTLOADED         1
#define S_BITMAPDATA_OK                0
#define S_BITMAPDATA_FILEDOESNOTEXIST -1
#define S_BITMAPDATA_INVALIDNUMBITS   -2
#define S_BITMAPDATA_INVALIDNUMFRAMES -3
#define S_BITMAPDATA_CANNOTOPENFILE   -4
#define S_BITMAPDATA_NODATA           -5
#define S_BITMAPDATA_MALLOCFAILED     -6

#ifdef WIN32
#  define ROP_SRCCOPY SRCCOPY
#endif
#ifdef UNIX
#  define ROP_SRCCOPY 0
#endif

typedef struct
{
  ulong cbFix;
  unsigned short cx;
  unsigned short cy;
  unsigned short cPlanes;
  unsigned short cBitCount;
  RgbStruct  argbColor[1];
} BitmapInfoStruct;

typedef struct
{
  ulong cbFix;
  unsigned short cx;
  unsigned short cy;
  unsigned short cPlanes;
  unsigned short cBitCount;
} BitmapInfoHeaderStruct;

typedef struct
{
  unsigned short dummy;
  unsigned short usType;
  ulong cbSize;
  short xHotspot;
  short yHotspot;
  ulong offBits;
  BitmapInfoHeaderStruct bmp;
} BitmapFileHeaderStruct;

typedef struct
{
  ulong cbFix;
  ulong cx;
  ulong cy;
  unsigned short cPlanes;
  unsigned short cBitCount;
  ulong ulCompression;
  
  ulong cbImage;
  ulong cxResolution;
  ulong cyResolution;
  ulong cclrUsed;
  ulong cclrImportant;
  unsigned short usUnits;
  unsigned short usReserved;
  unsigned short usRecording;
  unsigned short usRendering;
  ulong cSize1;
  ulong cSize2;
  ulong ulColorEncoding;
  ulong ulIdentifier;
  Rgb2Struct  argbColor[1];
} BitmapInfo2Struct;

typedef struct
{
  ulong cbFix;
  ulong cx;
  ulong cy;
  unsigned short cPlanes;
  unsigned short cBitCount;
  ulong ulCompression;
  
  ulong cbImage;
  ulong cxResolution;
  ulong cyResolution;
  ulong cclrUsed;
  ulong cclrImportant;
  unsigned short usUnits;
  unsigned short usReserved;
  unsigned short usRecording;
  unsigned short usRendering;
  ulong cSize1;
  ulong cSize2;
  ulong ulColorEncoding;
  ulong ulIdentifier;
} BitmapInfoHeader2Struct;

typedef struct
{
  ulong cbFix;
  ulong cx;
  ulong cy;
  unsigned short cPlanes;
  unsigned short cBitCount;
  ulong ulCompression;
  
  ulong cbImage;
  ulong cxResolution;
  ulong cyResolution;
  ulong cclrUsed;
  ulong cclrImportant;
} BitmapInfoHeader3Struct;

typedef struct
{
  unsigned short dummy;
  unsigned short usType;
  ulong cbSize;
  short xHotspot;
  short yHotspot;
  ulong offBits;
  BitmapInfoHeader2Struct bmp2;
} BitmapFileHeader2Struct;

class S_Canvas;

class S_BitmapData
{
 public:
  int rc;
  
  ulong secretCode;
  ulong checkSum;
  
  ulong imageBytes;
  char *pimageData;
  Rgb2Struct *ppalData;
  
  ulong infoBytes;
  BitmapInfo3Struct *pbmpInfo3;
  
  int SystemBlit(S_Canvas *,int,int,int,int,int,int,ulong=ROP_SRCCOPY);
  int SystemBlit(S_Canvas *,int,int,int,int,int,int,int,int,ulong=ROP_SRCCOPY);
  
  int SetPaletteData(int,int,int,int);
  int FillInPaletteData();
  int SetPixelData(int,int,ushort);
  int SetPixelData(int,int,uchar,uchar,uchar);
  int GetPixelData(int,int,uchar *,uchar *,uchar *);
  int GetPixelData(int,int);
  
  int Init();
  S_BitmapData();
  S_BitmapData(int,int,int);
  S_BitmapData( char * );
  S_BitmapData( uchar *,ulong );
  int Create(int,int,int);
  int Create(char *);
  int ZCreate(char *);
  int Create(uchar *,ulong);
  int Destroy();
  ~S_BitmapData();
  
  int GenerateCode(char *);
  int GenerateCode2(char *);
};

#endif
