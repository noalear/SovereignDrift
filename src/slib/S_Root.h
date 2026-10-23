#ifndef S_ROOT
#define S_ROOT

#define S_ROOT_DEFAULT        0
#define S_ROOT_USEDIRECTDRAW  1
#define S_ROOT_USEDIRECTSOUND 2

#ifdef OS2
typedef HAB HINSTANCE;
typedef QMSG MSG;
#endif
#ifdef WIN32
typedef HINSTANCE HAB;
typedef ulong HMQ;
typedef MSG QMSG;
#endif
#ifdef UNIX
typedef ulong MSG;
typedef ulong HAB;
typedef ulong HINSTANCE;
#endif

class S_Palette;
class S_SystemIcon;

class S_Root
{
 public:
  int cx,cy;
  int bitsPerPixel;
  int numColors;
  int cxSizeBorder,cySizeBorder;
  int cxIcon,cyIcon;
  int cyTitleBar,cyMenu;
  
  int SetHook( int (*)(MSG *) );
  int (*hook)(MSG *);
  
  char *exeFilename;
  char exeDirectory[1024];
  
  S_Palette *palette;
  
# ifdef OS2
  HAB hab;
  HMQ hmq;
# endif
# ifdef WIN32
  HINSTANCE hInst;
  HMODULE hmodDD;
  HMODULE hmodDS;
  
  LPDIRECTDRAW lpDD;
  typedef HRESULT (APIENTRY FNDirectDrawCreate)(GUID FAR *,
						LPDIRECTDRAW FAR *,
						IUnknown FAR *);
  typedef FNDirectDrawCreate *PFNDirectDrawCreate;
  
  LPDIRECTSOUND lpDs;
  typedef HRESULT (APIENTRY FNDirectSoundCreate)(GUID FAR *,
						 LPDIRECTSOUND FAR *,
						 IUnknown FAR *);
  typedef FNDirectSoundCreate *PFNDirectSoundCreate;

  RECT rcDesktop;
# endif
  
  ulong flags;
  
  S_Root(int * =NULL,char *** =NULL,ulong=S_ROOT_DEFAULT);
  ~S_Root();
  
  int Run();
  
  int ShutDown(int=FALSE);
  
  int SetMousePointer(S_SystemIcon *);
  int SetVideoMode(int,int,int);
  int ResetVideoMode();
};

#ifdef S_ROOTCPP
S_Root *_root;
#else
extern S_Root *_root;
#endif

#endif
