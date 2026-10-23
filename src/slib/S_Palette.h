#ifndef S_PALETTE
#define S_PALETTE

#ifdef UNIX
   typedef void *HPAL;
   typedef void *LPDIRECTDRAWPALETTE;
#endif

class S_Palette {
public:
   HPAL hPal;
   LPDIRECTDRAWPALETTE lpDDPal;

   int numEntries;
   RgbStruct *entries;

   S_Palette();
   S_Palette(RgbStruct *,int=256);
   ~S_Palette();
   };

#endif

