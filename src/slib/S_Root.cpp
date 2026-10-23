#define S_ROOTCPP

#include "S_OS.h"
#include "S_Root.h"
#include "S_Palette.h"
#include "S_SystemIcon.h"
#include "S_DirList.h"

S_Root::S_Root(int *pargc,char ***pargv,ulong iflags)
{
  hook = NULL;
  flags = iflags;
# ifdef OS2
  /* ***************************************** */
  /* *** Morph into a PM if app is console *** */
  /* ***************************************** */
  PPIB pib;
  PTIB tib;
  DosGetInfoBlocks(&tib,&pib);
  if ( pib->pib_ultype == 2 )
    pib->pib_ultype = 3;
  /* ***************************************** */
  
  hab = WinInitialize(0);
  hmq = WinCreateMsgQueue (hab,0);
  
  LONG alCaps[13];
  DEVOPENSTRUC dop = { 0L, "DISPLAY", NULL, 0L,0L,0L,0L,0L,0L };
  
  HDC hdc=DevOpenDC(hab,OD_MEMORY,"*",5L,(PDEVOPENDATA)&dop,NULLHANDLE);
  DevQueryCaps( hdc, CAPS_WIDTH, 13, alCaps );
  /*DevQueryCaps( hdc, CAPS_BACKGROUND_MIX_SUPPORT,1,
    &videoConfig->backMixCaps);*/
  DevCloseDC(hdc);
  
  cx = alCaps[0];
  cy = alCaps[1];
  numColors = alCaps[10];
  bitsPerPixel = alCaps[12];
  cxIcon = WinQuerySysValue (HWND_DESKTOP, SV_CXICON);
  cyIcon = WinQuerySysValue (HWND_DESKTOP, SV_CYICON);
  cyMenu = WinQuerySysValue (HWND_DESKTOP, SV_CYMENU);
  cyTitleBar = WinQuerySysValue(HWND_DESKTOP, SV_CYTITLEBAR);
  cxSizeBorder=WinQuerySysValue(HWND_DESKTOP,SV_CXSIZEBORDER);
  cySizeBorder=WinQuerySysValue(HWND_DESKTOP,SV_CYSIZEBORDER);
# endif
# ifdef WIN32
  hInst = (HINSTANCE)0x00400000;
  HDC hdc = CreateDC("DISPLAY",NULL,NULL,NULL);
  cx = GetDeviceCaps(hdc,HORZRES);
  cy = GetDeviceCaps(hdc,VERTRES);;
  bitsPerPixel = GetDeviceCaps(hdc,BITSPIXEL);
  numColors = 1<<bitsPerPixel;
  if ( bitsPerPixel > 24 )
    numColors = 1<<24;
  cxIcon = GetSystemMetrics (SM_CXICON);
  cyIcon = GetSystemMetrics (SM_CYICON);
  cyMenu = GetSystemMetrics (SM_CYMENU);
  cyTitleBar = GetSystemMetrics(SM_CYCAPTION);
  cxSizeBorder=GetSystemMetrics(SM_CXSIZEFRAME);
  cySizeBorder=GetSystemMetrics(SM_CYSIZEFRAME);
  DeleteDC(hdc);

  SystemParametersInfo(SPI_GETWORKAREA,0,&rcDesktop,0);
  
  /* ************************************************************* */
  /* *** Initialize DirectDraw by loading draw.dll and getting *** */
  /* *** a DirectDraw object pointer                           *** */
  /* ************************************************************* */
  lpDD = NULL;
  hmodDD = NULL;
  if ( flags & S_ROOT_USEDIRECTDRAW )
    {
      DDCAPS ddDriverCaps,ddHELCaps;
      FNDirectDrawCreate *pDirectDrawCreate;
      hmodDD = LoadLibrary("ddraw.dll");
      if ( hmodDD )
	{
	  pDirectDrawCreate = (FNDirectDrawCreate *)
	    GetProcAddress(hmodDD,"DirectDrawCreate");
	  if (  pDirectDrawCreate &&
                (*pDirectDrawCreate)(NULL,&lpDD,NULL) == DD_OK ) {
	    ddDriverCaps.dwSize = sizeof(DDCAPS);
	    ddHELCaps.dwSize = sizeof(DDCAPS);
	    lpDD->GetCaps(&ddDriverCaps,&ddHELCaps);
	  }
	}
    }
  
  /* **************************************************************** */
  /* *** Initialize DirectSound by loading dsound.dll and getting *** */
  /* *** a DirectSound object pointer                             *** */
  /* **************************************************************** */
  lpDs = NULL;
  hmodDS = NULL;
  if ( flags & S_ROOT_USEDIRECTSOUND )
    {
      FNDirectSoundCreate *pDirectSoundCreate;
      hmodDS = LoadLibrary("dsound.dll");
      if ( hmodDS )
	{
	  pDirectSoundCreate = (FNDirectSoundCreate *)
	    GetProcAddress(hmodDS,"DirectSoundCreate");
	  if ( pDirectSoundCreate )
	    (*pDirectSoundCreate)(NULL,&lpDs,NULL);
	  if ( lpDs )
	    {
	      /*
		HWND hwnd;
		char newTitle[1024];
		char oldTitle[1024];
		GetConsoleTitle(oldTitle,sizeof(oldTitle));
		sprintf(newTitle,"%d/%d",GetTickCount(),GetCurrentProcessId());
		SetConsoleTitle(newTitle);
		Sleep(20);
		hwnd = FindWindow(NULL,newTitle);
		SetConsoleTitle(oldTitle);
		printf("hwnd: 0x%x\n",hwnd);
		lpDs->SetCooperativeLevel(hwnd,DSSCL_NORMAL);
	      */
	    }
	}
    }
  
# endif
# ifdef GTK
  gtk_init(pargc,pargv);
# endif
  _root = this;
  palette = new S_Palette(NULL);
  
  //
  // Get the directory where the app was started.
  //
  exeDirectory[0] = 0;
# ifdef WIN32
  GetModuleFileName(NULL,exeDirectory,1023);
# endif
  int i = strlen(exeDirectory);
  if ( i > 0 )
    {
      do
	{
	  exeDirectory[i--] = 0;
	} while ( i > 0 && exeDirectory[i] != '\\' );
    }
  
  //
  // Get the name of the app.
  //
  exeFilename = NULL;
  if ( pargv )
    {
      int j;
      char *str = *(pargv)[0];
      if ( str[0] != '.' && str[1] != ':' ) {
	for ( j=strlen(str); j>0 && str[j]!=S_DIRLIST_SEPARATOR; j-- );
	if ( str[j] == S_DIRLIST_SEPARATOR )
	  j++;
      }
      else {
	j = 0;
      }
      exeFilename = (char *)S_malloc(strlen(str)+5);
      sprintf(exeFilename,"%s.exe",str+j);
      if ( !S_DirList_FileExists(exeFilename) )
	strcpy(exeFilename,str+j);
    }
  
}


extern char *S_Window_ClassName;

S_Root::~S_Root()
{
  if ( exeFilename )
    {
      S_free(exeFilename);
      exeFilename = NULL;
    }
  
  if ( palette )
    {
      delete palette;
      palette = NULL;
    }
  
# ifdef OS2
  WinDestroyMsgQueue (hmq);
  WinTerminate (hab);
# endif
# ifdef WIN32
  if ( lpDD )
    {
      lpDD->Release();
      lpDD = NULL;
    }
  if ( lpDs )
    {
      lpDs->Release();
      lpDs = NULL;
    }
  if ( hmodDS )
    {
      FreeLibrary(hmodDS);
      hmodDS = NULL;
    }
  if ( hmodDD )
    {
      FreeLibrary(hmodDD);
      hmodDD = NULL;
    }
  UnregisterClass(S_Window_ClassName,hInst);
# endif
}


int
S_Root::SetMousePointer(S_SystemIcon *icon)
{
# ifdef WIN32
  SetCursor(icon->hIcon);
# endif
  return 0;
}

int _DDPrintErrorMsg(int);

int
S_Root::SetVideoMode(int icx,int icy,int ibitsPerPixel)
{
# ifdef WIN32
  if ( lpDD )
    {
      int rc = lpDD->SetDisplayMode(icx,icy,ibitsPerPixel);
      if ( rc == 0 )
	{
	  cx = icx;
	  cy = icy;
	  bitsPerPixel = ibitsPerPixel;
	}
      _DDPrintErrorMsg(rc);
    }
  else
    {
      DEVMODE dm;
      dm.dmSize = sizeof(dm);
      dm.dmBitsPerPel = ibitsPerPixel;
      dm.dmPelsWidth = icx;
      dm.dmPelsHeight = icy;
      dm.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
      if ( ChangeDisplaySettings(&dm,0) == DISP_CHANGE_SUCCESSFUL )
	{
	  bitsPerPixel = ibitsPerPixel;
	  cx = icx;
	  cy = icy;
	}
    }
#  endif
  return 0;
}


int
S_Root::ResetVideoMode()
{
# ifdef WIN32
  int rc = 0;
  if ( lpDD )
    rc = lpDD->RestoreDisplayMode();
  _DDPrintErrorMsg(rc);
# endif
  return 0;
}


int
S_Root::Run()
{
# ifdef OS2
  QMSG msg;
  while ( WinGetMsg (hab, &msg, 0, 0, 0) )
    WinDispatchMsg (hab, &msg);
  return 0;
# endif
# ifdef WIN32
  MSG msg;
  while ( GetMessage(&msg, NULL, 0, 0) > 0 )
    {
      HWND parent = GetParent(msg.hwnd);
      if ( !parent || (parent && !IsDialogMessage(parent,&msg)) )
	{
	  TranslateMessage(&msg);
	  DispatchMessage(&msg);
	}
      if ( hook )
	(*hook)(&msg);
    }
  return(msg.wParam);
# endif
# ifdef GTK
  gtk_main();
  return 0;
# endif
}


int 
S_Root::SetHook( int (*ihook)(MSG *) )
{
  hook = ihook;
  return 0;
}


#define SYSFUNC   0xD5
#define REBOOT    0xAB
#define REBOOTDEV "\\DEV\\DOS$"

int
S_Root::ShutDown(int reboot)
{
# ifdef OS2
  if ( reboot )
    {
      int rc;
      HFILE  hREBOOT;
      ULONG  ulAction;
      rc = DosOpen(REBOOTDEV,&hREBOOT,&ulAction,0L,FILE_NORMAL,FILE_OPEN,
		   OPEN_SHARE_DENYNONE|OPEN_ACCESS_READWRITE,0L);
      if ( !rc )
	{
	  DosDevIOCtl(hREBOOT,SYSFUNC,REBOOT,NULL,0L,NULL,NULL,0L,NULL);
	  DosClose(hREBOOT);
	}
    }
  else
    {
      WinCancelShutdown(_root->hmq,TRUE);
      WinShutdownSystem(_root->hab,_root->hmq);
    }
# endif
# ifdef WIN32
  HANDLE hToken;
  TOKEN_PRIVILEGES tkp;
  OpenProcessToken(GetCurrentProcess(),
		   TOKEN_ADJUST_PRIVILEGES|TOKEN_QUERY,&hToken);
  LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME,&tkp.Privileges[0].Luid);
  tkp.PrivilegeCount = 1;
  tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
  AdjustTokenPrivileges(hToken, FALSE, &tkp, 0,(PTOKEN_PRIVILEGES)NULL,0);
  if ( reboot )
    ExitWindowsEx(EWX_REBOOT | EWX_FORCE, 0);
  else
    ExitWindowsEx(EWX_SHUTDOWN | EWX_FORCE, 0);
# endif
  return 0;
}

