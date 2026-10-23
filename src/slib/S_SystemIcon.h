#ifndef S_SYSTEMICON
#define S_SYSTEMICON

#include "S_Bitmap.h"

#define S_SYSTEMICON_OK                 0
#define S_SYSTEMICON_NODATA            -5
#define S_SYSTEMICON_CREATEFAILED      -7
#define S_SYSTEMICON_UNSUPPORTEDBMP    -8

#ifdef UNIX
   typedef void *HICON;
#endif
#ifdef OS2
   typedef HPOINTER HICON;
#endif

class S_BitmapData;

class S_SystemIcon : public S_Bitmap {
public:
   int rc;

   HICON hIcon;

   int Draw(S_Canvas *,int,int,int=0,int=0);

   int Init();
   S_SystemIcon();
   S_SystemIcon( char *,ulong=0x000000 );
   S_SystemIcon( S_BitmapData *,ulong=0x000000 );
   int Create(char *,ulong=0x00000000);
   int Create(S_BitmapData *,ulong=0x00000000);
   int ZCreate(char *,ulong=0x00000000);
   int Destroy();
   ~S_SystemIcon();
   };

#endif
