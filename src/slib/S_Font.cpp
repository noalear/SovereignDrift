#include "S_OS.h"
#include "S_Font.h"

static int _lcid = 0;


int
S_Font::Init()
{
  hFont = NULL;
  faceName = NULL;
  pointSize = 0;
  id = 0;
  cx = 0;
  cy = 0;
  angle = 0;
  return 0;
}


S_Font::S_Font()
{
  Init();
}


S_Font::S_Font(char *ifaceName,int ipointSize,int iangle,int iweight)
{
  Init();
  pointSize = ipointSize;
  Create(ifaceName,(pointSize*5)/8,(pointSize*14)/8,iangle,iweight);
  pointSize = ipointSize;
}



int
S_Font::Create(char *ifaceName,int ipointSize,int iangle,int iweight)
{
  Destroy();
  pointSize = ipointSize;
  Create(ifaceName,(pointSize*5)/8,(pointSize*14)/8,iangle,iweight);
  pointSize = ipointSize;
  return 0;
}


S_Font::S_Font(char *ifaceName,int icx,int icy,int iangle,int iweight)
{
  Init();
  Create(ifaceName,icx,icy,iangle,iweight);
}


int
S_Font::Create(char *ifaceName,int icx,int icy,int iangle,int iweight)
{
  Destroy();  
  id = _lcid++;
  faceName = (char *)S_malloc(strlen(ifaceName)+1);
  strcpy(faceName,ifaceName);
  cx = icx;
  cy = icy;
  angle = iangle;
# ifdef OS2
  cx *= 18;
  cx /= 8;
  cy *= 12;
  cy /= 14;
# endif
# ifdef WIN32
  hFont = CreateFont(cy,cx,angle*10,
		     0,iweight,0,0,0,ANSI_CHARSET,
		     OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,
		     DEFAULT_QUALITY,DEFAULT_PITCH | FF_SWISS,
		     faceName);
# endif
  return 0;
}


int
S_Font::Destroy()
{
# ifdef WIN32
  if ( hFont )
    {
      DeleteObject(hFont);
    }
# endif
  if ( faceName )
    {
      S_free(faceName);
    }
  
  Init();

  return 0;
}


S_Font::~S_Font()
{
  Destroy();
}
