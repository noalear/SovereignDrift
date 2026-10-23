/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
#include "S_OS.h"
#include "S_Window.h"
#include "S_Root.h"
#include "S_ListBox.h"
#include "S_TreeView.h"
#include "S_TreeViewNode.h"
#include "S_SystemIcon.h"
#include "S_ScrollBar.h"
#include "S_StaticText.h"
#include "S_Menu.h"
#include "S_Font.h"

char *S_Window_ClassName = "S_Window";

int
S_Window::Init()
{
  selfDelete = FALSE;
  created = FALSE;
  closing = FALSE;
  dragging = NULL;
  tvDrag = NULL;
  menuBar = NULL;
  flFrameFlags = 0;
  puser = NULL;
  rootWindow = FALSE;
  root = NULL;
  hwndFrame = NULL;
  hwndClient = NULL;
  lshift = FALSE;
  rshift = FALSE;
# ifdef WIN32
  hIcon = NULL;
  lpBuffer = NULL;
  lpClipper = NULL;
  pCommandHook = NULL;
  pKeyboardHook = NULL;
  pGenericHook = NULL;
# endif
  return 0;
}


S_Window::S_Window()
{
  Init();
}


S_Window::S_Window( S_Root *iroot,
                    S_Object *iparent,
                    char *iwindowName,
                    int (*ipMsgHandler)(S_Window *),
                    int iconID,
                    int ix, int iy, int icx, int icy,
                    ulong iflFrameFlags,
                    void *ipuser)
{
  Init();
  Create( iroot,iparent,iwindowName,ipMsgHandler,iconID,
	  ix,iy,icx,icy,iflFrameFlags,ipuser );
}


int
S_Window::Create( S_Root *iroot,
                  S_Object *iparent,
                  char *iwindowName,
                  int (*ipMsgHandler)(S_Window *),
                  int iconID,
                  int ix, int iy, int icx, int icy,
                  ulong iflFrameFlags,
                  void *ipuser)
{
  
  Destroy();
  S_Object::Create(iparent,(int(*)(S_Object *))ipMsgHandler,
		   S_OBJECT_WINDOW,ix,iy,icx,icy);
  
  flFrameFlags = iflFrameFlags;
  puser = ipuser;
  
  HWND hwndParent;
  if ( parent )
    hwndParent = parent->hwnd;
  else
    hwndParent = HWND_DESKTOP;
  
  rootWindow = FALSE;
  if ( iroot )
    {
      root = iroot;
      rootWindow = TRUE;
    }
  else if ( parent )
    root = parent->root;
  
  selfDelete = !rootWindow;
  
  cxCanvas = cx;
  cyCanvas = cy;
  cxClient = cx;
  cyClient = cy;
  
  char *windowName;
  if ( iwindowName == NULL )
    {
      windowName = (char *) S_malloc(strlen("window")+10+1);
      sprintf(windowName,"window%d",id);
    }
  else {
    windowName = (char *) S_malloc( strlen(iwindowName)+1 );
    strcpy(windowName,iwindowName);
  }
  
# ifdef OS2
  if ( iconID != 0 )
    flFrameFlags |= FCF_ICON;
  WinRegisterClass(root->hab,S_Window_ClassName,(PFNWP)_S_WindowProc,
		   CS_SIZEREDRAW,4);
  hwndFrame = WinCreateStdWindow (HWND_DESKTOP,0,
				  &flFrameFlags, S_Window_ClassName, 
				  windowName, 0, 0, iconID,
				  &hwndClient);
  WinSetOwner(hwndFrame,hwndParent);
  if ( flFrameFlags == S_WINDOW_STATICCHILD )
    WinSetParent(hwndFrame,hwndParent,TRUE);
  hwnd = hwndClient;
  
  WinSetWindowULong(hwndClient,QWL_USER,(ULONG)this);
  WinSetWindowULong(hwndFrame,QWL_USER,(ULONG)this);
  WinSetWindowPos(hwndFrame,HWND_TOP,0,0,0,0,SWP_ZORDER);
# endif
# ifdef WIN32
  WNDCLASS WndClass;
  
  WndClass.style       = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
  WndClass.lpfnWndProc = (WNDPROC) _S_WindowProc;
  WndClass.cbClsExtra  = 0;
  WndClass.cbWndExtra  = 0;
  WndClass.hInstance   = root->hInst;
  WndClass.hIcon = 0;
  WndClass.hCursor     = LoadCursor(NULL, IDC_ARROW);
  WndClass.hbrBackground = NULL;
  WndClass.lpszMenuName = 0;
  if ( iconID != 0 )
    WndClass.lpszMenuName = MAKEINTRESOURCE(iconID);
  WndClass.lpszClassName  = S_Window_ClassName;
  
  RegisterClass (&WndClass);
  
  FreeResource(WndClass.hCursor);
  
  /*
    if ( flFrameFlags == S_WINDOW_STATICCHILD )
    flFrameFlags |= WS_CHILD;
  */
  hwnd = hwndFrame = hwndClient = 
    CreateWindow(S_Window_ClassName, windowName,
		 flFrameFlags,
		 x, y, cx, cy,
		 hwndParent, NULL, root->hInst, NULL);
  
  if ( iconID != 0 )
    {
      hIcon = LoadIcon(root->hInst,MAKEINTRESOURCE(iconID));
      if ( hIcon )
	SetClassLong(hwnd,GCL_HICON,(slong)hIcon);
    }
  
  if ( rootWindow && root->lpDD )
    {
      //root->lpDD->SetCooperativeLevel(hwnd,DDSCL_NORMAL);
      root->lpDD->SetCooperativeLevel(hwnd,
				      DDSCL_EXCLUSIVE|DDSCL_FULLSCREEN);
    }
  
  if ( rootWindow && root->lpDs )
    {
      root->lpDs->SetCooperativeLevel(hwnd,DSSCL_NORMAL);
    }
  
  SetWindowLong(hwndClient,GWL_USERDATA,(slong)this);
  
# endif
# ifdef GTK
  hwnd = hwndFrame = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  hwndClient = gtk_vbox_new(TRUE,0);
  gtk_container_add(GTK_CONTAINER(hwnd),hwndClient);
  gtk_widget_show(hwndClient);
  gtk_widget_show(hwnd);
  gtk_signal_connect(GTK_OBJECT (hwnd), "delete_event",
		     (GtkSignalFunc) gtk_exit, NULL);
#  endif
  
  if ( windowName )
    {
      S_free(windowName);
      windowName = NULL;
    }
  
  if ( cx != (int)S_WINDOW_DEFAULT && cy != (int)S_WINDOW_DEFAULT )
    SetSize(cx,cy);
  else
    SetSize(640,480);
  if ( !rootWindow && x != (int)S_WINDOW_DEFAULT && y != (int)S_WINDOW_DEFAULT )
    SetPosition(x,y);
  
  CreateSystemCanvas();
  
  if ( flFrameFlags == S_WINDOW_STATICCHILD && parent )
    parent->AddChild(this);
  
  HandleCreateMsg();
  created = TRUE;
  return 0;
}


int
S_Window::Delete()
{
  selfDelete = TRUE;
  Close();
  return 0;
}


int
S_Window::Destroy()
{
  if ( !selfDelete )
    Close();
  if ( parent )
    {
      parent->RemoveChild(this);
      parent = NULL;
    }
# ifdef WIN32
  if ( hIcon )
    {
      FreeResource(hIcon);
      hIcon = NULL;
    }
  if ( S_Canvas::font )
    {
      SelectObject(hdc,oldFont);
    }
#  endif
  Init();
  return 0;
}


S_Window::~S_Window()
{
  Destroy();
}


int
S_Window::CreateSystemCanvas()
{
  if ( !hwnd ) return 0;
  
  DestroySystemCanvas();
  
  cxCanvas = cxClient;
  cyCanvas = cyClient;
  
  xCanvas = 0;
  yCanvas = 0;
  
# ifdef OS2
  hdc = WinGetPS(hwndClient);
  GpiCreateLogColorTable(hdc,0,LCOLF_RGB,0,0,0);
# endif
# ifdef WIN32
  /* *** Setup DirectDraw *** */
  if ( root->lpDD )
    {
      int rc;
      
      /* ***** Go Ahead and create a primary surface buffer ***** */
      DDSURFACEDESC ddsd;
      ddsd.dwSize = sizeof(ddsd);
      ddsd.dwFlags = DDSD_CAPS;
      ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
      root->lpDD->CreateSurface(&ddsd,&lpBuffer,NULL);
      
      /* *** Now create a DirectDrawClipper object *** */
      if ( lpBuffer )
	{
	root->lpDD->CreateClipper(0,&lpClipper,NULL);
	if ( lpClipper )
	  rc = lpClipper->SetHWnd(0,hwnd);
	if ( rc == DD_OK && lpBuffer && lpClipper )
	  rc = lpBuffer->SetClipper(lpClipper);
	lpClipper->Release();
	lpClipper = NULL;
	if ( rc != DD_OK ) {
	  lpBuffer->Release();
	  lpBuffer = NULL;
	}
	}
    }
  
  if ( lpBuffer )
    {
      RECT rcl;
      GetClientRect(hwnd,&rcl);
      ClientToScreen(hwnd,(LPPOINT)&rcl);
      ClientToScreen(hwnd,(LPPOINT)&rcl+1);
      xCanvas = rcl.left;
      yCanvas = rcl.top;
    }
  
  hdc = NULL;
  if ( !lpBuffer ) 
    {
      hdc = GetDC(hwndClient);
      SetBkMode(hdc,TRANSPARENT);
      SetTextAlign(hdc,TA_BASELINE);
      SetStretchBltMode(hdc,STRETCH_DELETESCANS);
    }
  
# endif
  
  palette = SetPalette(NULL);
  
  return 0;
}


/* *** Windows 95 patch *** */
int
S_Window::RefreshSystemCanvas() {
  if ( !hwnd ) return 0;
  DefreshSystemCanvas();
# ifdef WIN32
  if ( _OsType() == S_OS_WIN95 )
    {
      hdc = GetDC(hwndClient);
      SetBkMode(hdc,TRANSPARENT);
      SetTextAlign(hdc,TA_BASELINE);
      SetStretchBltMode(hdc,STRETCH_DELETESCANS);
      SetFont(S_Canvas::font);
    }
# endif
  return 0;
}


int
S_Window::DefreshSystemCanvas() {
  if ( _OsType() == S_OS_WIN95 )
    DestroySystemCanvas();
  return 0;
}


int
S_Window::DestroySystemCanvas()
{
  ResetPalette();
# ifdef OS2
# endif
# ifdef WIN32
  if( lpClipper )
    {
      lpClipper->Release();
      lpClipper = NULL;
    }
  if ( lpBuffer )
    {
      lpBuffer->Release();
      lpBuffer= NULL;
    }
  if ( hdc )
    {
      ReleaseDC(hwnd,hdc);
      hdc = NULL;
    }
# endif
  return 0;
}


int
S_Window::FullScreen()
{
  if ( !hwnd ) return 0;
# ifdef OS2
  WinSetWindowPos( hwndFrame,NULLHANDLE,
		   -root->cxSizeBorder,-root->cySizeBorder,
		   root->cx + 2*root->cxSizeBorder,
		   root->cy + 2*root->cySizeBorder,
		   SWP_MOVE | SWP_SIZE | SWP_ACTIVATE | SWP_SHOW );
# endif
# ifdef WIN32
  SetWindowPos( hwndFrame,NULL,
		-root->cxSizeBorder,
		-root->cySizeBorder,
		root->rcDesktop.right + root->cxSizeBorder,
		root->rcDesktop.bottom + root->cySizeBorder,
		0);
# endif
  return 0;

}


int
S_Window::Minimize()
{
# ifdef OS2
  WinSetWindowPos( hwndFrame,NULLHANDLE,x,y,cx,cy,SWP_MINIMIZE);
# endif
# ifdef WIN32
  WINDOWPLACEMENT wp;
  wp.length = sizeof(wp);
  wp.flags = 0;
  wp.showCmd = SW_MINIMIZE;
  wp.ptMinPosition.x = x;
  wp.ptMinPosition.y = y;
  wp.ptMaxPosition.x = x;
  wp.ptMaxPosition.y = y;
  wp.rcNormalPosition.top = y;
  wp.rcNormalPosition.bottom = y+cy;
  wp.rcNormalPosition.left = x;
  wp.rcNormalPosition.right = x+cx;
  SetWindowPlacement( hwndFrame,&wp);
# endif
  return 0;
}


int
S_Window::Maximize()
{
  if ( !hwnd ) return 0;
# ifdef OS2
  WinSetWindowPos( hwndFrame,NULLHANDLE,
		   root->rcDesktop.left-root->cxSizeBorder,
		   root->rcDesktop.top-root->cySizeBorder,
		   root->rcDesktop.right + root->cxSizeBorder,
		   root->rcDesktop.bottom + root->cySizeBorder,
		   SWP_MOVE | SWP_SIZE | SWP_ACTIVATE | SWP_SHOW );
# endif
# ifdef WIN32
  WINDOWPLACEMENT wp;
  wp.length = sizeof(wp);
  wp.flags = WPF_RESTORETOMAXIMIZED;
  wp.showCmd = SW_SHOWMAXIMIZED;
  wp.ptMinPosition.x = 0;
  wp.ptMinPosition.y = 0;
  wp.ptMaxPosition.x = -root->cxSizeBorder;
  wp.ptMaxPosition.y = -root->cySizeBorder;
  wp.rcNormalPosition.top = 0;
  wp.rcNormalPosition.bottom = root->rcDesktop.bottom - root->rcDesktop.top;
  wp.rcNormalPosition.left = 0;
  wp.rcNormalPosition.right = root->rcDesktop.right;
  SetWindowPlacement( hwndFrame,&wp);
# endif
  return 0;
}


int
S_Window::SetSize(int icx,int icy)
{
  if ( !hwnd ) return 0;
  S_Object::SetSize(icx,icy);
# ifdef OS2
  RECTL rcl;
  WinQueryWindowRect(hwndClient,&rcl);
  cxClient = rcl.xRight - rcl.xLeft;
  cyClient = rcl.yTop - rcl.yBottom;
  cxCanvas = cxClient;
  cyCanvas = cyClient;
# endif
# ifdef WIN32
  RECT rcl;
  GetClientRect(hwndClient,&rcl);
  cxClient = rcl.right - rcl.left;
  cyClient = rcl.bottom - rcl.top;
  cxCanvas = cxClient;
  cyCanvas = cyClient;
# endif
  return 0;
}


int
S_Window::DisableCloseButton()
{
# ifdef WIn32
  HMENU hMenu = GetSystemMenu(hwnd,FALSE);
  if ( hMenu ) {
    EnableMenuItem(hMenu,SC_CLOSE,MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
    /*RemoveMenu(hMenu,SC_CLOSE,MF_BYCOMMAND);*/
  }
# endif
  return 0;
}


int
S_Window::CreateSystemTrayIcon()
{
# ifdef WIN32
  NOTIFYICONDATA nid;
  nid.cbSize = sizeof(nid);
  nid.hWnd = hwnd;
  nid.uID = id;
  nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
  nid.uCallbackMessage = UWM_SYSTEMTRAY;
  nid.hIcon = hIcon;
  strcpy(nid.szTip,GetText());
  Shell_NotifyIcon(NIM_ADD,&nid);
# endif
  return 0;
}


int
S_Window::DestroySystemTrayIcon()
{
# ifdef WIN32
  NOTIFYICONDATA nid;
  nid.cbSize = sizeof(nid);
  nid.hWnd = hwnd;
  nid.uID = id;
  nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
  nid.uCallbackMessage = UWM_SYSTEMTRAY;
  nid.hIcon = NULL;
  strcpy(nid.szTip,"");
  Shell_NotifyIcon(NIM_DELETE,&nid);
# endif
  return 0;
}


int
S_Window::HandleCreateMsg()
{
  msg = S_WINDOW_CREATE;
  if ( pMsgHandler )
    (*pMsgHandler)((void *)this);
  return 0;
}


int
S_Window::HandleSizeMsg()
{
  msg = S_WINDOW_SIZE;
  if ( pMsgHandler )
    (*pMsgHandler)((void *)this);
  return 0;
}


int
S_Window::HandleFocusMsg()
{
  msg = S_WINDOW_FOCUS;
  if ( pMsgHandler )
    (*pMsgHandler)((void *)this);
  return 0;
}


int
S_Window::HandleKeyboardMsg()
{
  msg = S_WINDOW_KEYBOARD;
  if ( pMsgHandler )
    (*pMsgHandler)((void *)this);
  return 0;
}


int
S_Window::HandleMouseMsg()
{
  msg = S_WINDOW_MOUSE;
  if ( pMsgHandler )
    (*pMsgHandler)((void *)this);
  return 0;
}


int
S_Window::HandleSystemTrayMouseMsg()
{
  msg = S_WINDOW_SYSTEMTRAYMOUSE;
  if ( pMsgHandler )
    (*pMsgHandler)((void *)this);
  return 0;
}


int
S_Window::HandleExitMenuMsg()
{
  msg = S_WINDOW_EXITMENU;
  if ( pMsgHandler )
    (*pMsgHandler)((void *)this);
  return 0;
}


int
S_Window::HandleEnterMenuMsg()
{
  msg = S_WINDOW_ENTERMENU;
  if ( pMsgHandler )
    (*pMsgHandler)((void *)this);
  return 0;
}


int
S_Window::HandleMoveMsg()
{
  msg = S_WINDOW_MOVE;
  if ( pMsgHandler )
    (*pMsgHandler)((void *)this);
  return 0;
}


int
S_Window::HandlePaintMsg()
{
  msg = S_WINDOW_PAINT;
  if ( pMsgHandler )
    (*pMsgHandler)((void *)this);
  return 0;
}


int
S_Window::HandleCloseMsg()
{
  msg = S_WINDOW_CLOSE;
  if ( pMsgHandler )
    (*pMsgHandler)((void *)this);
  return 0;
}


int
S_Window::HandleDropMsg()
{
  msg = S_WINDOW_DROP;
  if ( pMsgHandler )
    (*pMsgHandler)((void *)this);
  return 0;
}


int
S_Window::HandleDestroyMsg()
{
  msg = S_WINDOW_DESTROY;
  if ( pMsgHandler )
    (*pMsgHandler)((void *)this);
  return 0;
}


int
_HandleTreeView(S_Object *control,ulong msg,ulong mp1,ulong mp2)
{
# ifdef WIN32
  S_TreeView *tv = (S_TreeView *)control;
  switch(msg)
    {
    case NM_RCLICK :
      {
	POINT pt1;
	GetCursorPos(&pt1);
	POINT pt2 = {tv->x,tv->y};
	ClientToScreen(tv->parent->hwnd,&pt2);
	tv->dropTarget = tv->GetNodeUnder(pt1.x - pt2.x,pt1.y - pt2.y);
	if ( tv->dropTarget )
	  {
	    tv->dropTarget->SetAsDropTarget();
	  }
	tv->msg = S_TREEVIEW_RMOUSECLICK;
	break;
      }
    case NM_CLICK :
      {
	POINT pt1;
	GetCursorPos(&pt1);
	POINT pt2 = {tv->x,tv->y};
	ClientToScreen(tv->parent->hwnd,&pt2);
	S_TreeViewNode *tvn = tv->GetNodeUnder(pt1.x - pt2.x,pt1.y - pt2.y);
	if ( tvn )
	  {
	    tvn->HiLite();
	  }
	tv->msg = S_TREEVIEW_LMOUSECLICK;
	break;
      }
    case NM_DBLCLK :
      if ( tv->dropTarget )
	tv->dropTarget->HiLite();
      tv->msg = S_TREEVIEW_LMOUSEDBLCLICK;
      break;
    case TVN_SELCHANGED :
      tv->dragObject = (S_TreeViewNode *) 
	((NM_TREEVIEW *)mp1)->itemOld.lParam;
      tv->dropTarget = (S_TreeViewNode *)
	((NM_TREEVIEW *)mp1)->itemNew.lParam;
      tv->dropTarget->HiLite();
      tv->msg = S_TREEVIEW_SELCHANGED;
      break;
    case TVN_BEGINDRAG :
      tv->dragObject = (S_TreeViewNode *) 
	((NM_TREEVIEW *)mp1)->itemNew.lParam;
      tv->dropTarget = NULL;

      S_Object *obj = control;
      S_Window *rootWindow = NULL;
      do 
	{
	  obj = obj->parent;
	  if (obj->type==S_OBJECT_WINDOW && ((S_Window *)obj)->rootWindow)
	    rootWindow = (S_Window *)obj;
	} while ( rootWindow == NULL );
      
      SetCapture(rootWindow->hwnd);
      rootWindow->dragging = tv->dragObject;
      if ( rootWindow->dragging->icon )
	SetCursor(rootWindow->dragging->icon->hIcon);
      if ( rootWindow->dragging->hIcon )
	SetCursor(rootWindow->dragging->hIcon);
      tv->msg = S_TREEVIEW_BEGINDRAG;
      
      break;
    }
# endif
  return 0;
}


int
_HandleToolBar(S_Object *control,ulong msg,ulong mp1,ulong mp2)
{
# ifdef WIN32
  control->msg = S_TOOLBAR_NOMSG;
  if ( msg == 0 )
    control->msg = S_TOOLBAR_BUTTONPRESSED;
# endif
  return 0;
}



int
_HandleListBox(S_Object *control,ulong msg,ulong mp1,ulong mp2)
{
# ifdef WIN32
  control->msg = 0;
  if ( msg == LBN_DBLCLK )
    control->msg = S_LISTBOX_DOUBLECLICK;
# endif
  return 0;
}

int
_HandleComboBox(S_Object *control,ulong msg,ulong mp1,ulong mp2)
{
# ifdef WIN32
# endif
  return 0;
}

#if defined(OS2) || defined(WIN32)
int
_CallHwndMsgHandler( HWND hwndControl,ulong msg,ulong mp1,ulong mp2 )
{
  if ( hwndControl )
    {
      S_Object *control;
#     ifdef WIN32
      control = (S_Object *)GetWindowLong(hwndControl,GWL_USERDATA);
#     endif
#     ifdef OS2
      control = (S_Object *)WinQueryWindowULong(hwndControl,QWL_USER);
#     endif
      if ( control && control->pMsgHandler )
	{
	  control->msg = msg;
	  control->mp1 = mp1;
	  control->mp2 = mp2;
	  if ( control->type == S_OBJECT_TREEVIEW )
            _HandleTreeView(control,msg,mp1,mp2);
	  if ( control->type == S_OBJECT_TOOLBAR )
            _HandleToolBar(control,msg,mp1,mp2);
	  if ( control->type == S_OBJECT_LISTBOX )
            _HandleListBox(control,msg,mp1,mp2);
	  if ( control->type == S_OBJECT_COMBOBOX )
            _HandleComboBox(control,msg,mp1,mp2);
	  (*(control->pMsgHandler))((void *)control);
	  return 0;
	}
    }
  return -1;
}


int
_CallIDMsgHandler( S_Window *window,int id,
		   ulong msg,ulong mp1,ulong mp2 )
{
  if ( window )
    {
      S_Object *child;
      for ( child = window->firstChild;
            child != NULL && child->id != id; 
            child = child->next );
      if ( child && child->pMsgHandler )
	{
	  child->msg = msg;
	  child->msg = mp1;
	  child->msg = mp2;
	  if ( child->type == S_OBJECT_TOOLBAR )
            _HandleToolBar(child,msg,mp1,mp2);
	  if ( child->type == S_OBJECT_TREEVIEW )
            _HandleTreeView(child,msg,mp1,mp2);
	  if ( child->type == S_OBJECT_TOOLBAR )
            _HandleToolBar(child,msg,mp1,mp2);
	  (*(child->pMsgHandler))((void *)child);
	  return 0;
	}
    }
  return -1;
}
#endif

#ifdef WIN32
LRESULT CALLBACK 
_S_WindowProc( HWND hwnd,UINT msg,
	       WPARAM wParam,LPARAM lParam )
{

  S_Window *window = (S_Window *)GetWindowLong(hwnd,GWL_USERDATA);
  
  if ( window && window->hwnd == hwnd && window->pGenericHook )
    {
      (*window->pGenericHook)(hwnd,msg,wParam,lParam);
    }
  
  switch(msg)
    {
      
    case WM_CREATE :
      if ( window )
	window->HandleCreateMsg();
      return 0;
      
    case WM_SETFOCUS :
      if ( window && window->created )
	window->HandleFocusMsg();
      return 0;
      
      
    case UWM_SYSTEMTRAY :
      if ( window )
	{
	  switch(lParam)
	    {
	    case WM_MOUSEMOVE :
	      window->button = 0;
	      window->keyup = FALSE;
	      window->repeat = FALSE;
	      break;
	    case WM_RBUTTONUP :
	      window->button = 2;
	      window->keyup = TRUE;
	      window->repeat = FALSE;
	      break;
	    case WM_RBUTTONDOWN :
	      window->button = 2;
	      window->keyup = FALSE;
	      window->repeat = FALSE;
	      break;
	    case WM_RBUTTONDBLCLK :
	      window->button = 2;
	      window->keyup = FALSE;
	      window->repeat = TRUE;
	      break;
	    case WM_LBUTTONUP :
	      window->button = 1;
	      window->keyup = TRUE;
	      window->repeat = FALSE;
	      break;
	    case WM_LBUTTONDOWN :
	      window->button = 1;
	      window->keyup = FALSE;
	      window->repeat = FALSE;
	      break;
	    case WM_LBUTTONDBLCLK :
	      window->button = 1;
	      window->keyup = FALSE;
	      window->repeat = TRUE;
	      break;
	    case WM_MBUTTONUP :
	      window->button = 3;
	      window->keyup = TRUE;
	      window->repeat = FALSE;
	      break;
	    case WM_MBUTTONDOWN :
	      window->button = 3;
	      window->keyup = FALSE;
	      window->repeat = FALSE;
	      break;
	    case WM_MBUTTONDBLCLK :
	      window->button = 3;
	      window->keyup = FALSE;
	      window->repeat = TRUE;
	      break;
	    }
	  if ( window->created )
	    window->HandleSystemTrayMouseMsg();
	}
      return 0;
      
    case WM_EXITMENULOOP :
      if ( window->created )
	window->HandleExitMenuMsg();
      return 0;
      
    case WM_ENTERMENULOOP :
      if ( window->created )
	window->HandleEnterMenuMsg();
      return 0;
      
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    case WM_KEYDOWN:
    case WM_KEYUP:
      if ( window )
	{
	  if ( window->pKeyboardHook )
	    {
	      (*window->pKeyboardHook)(hwnd,msg,wParam,lParam);
	      return 0;
	    }
	  window->keyup    = (char)(lParam   >> 31);
	  window->scancode = (char)(lParam   >> 16);
	  window->repeat   = ((char)((lParam >> 30) & 1)) && !window->keyup;
	  if ( window->scancode == S_KEY_LSHIFT )
	    window->lshift = !window->keyup;
	  if ( window->scancode == S_KEY_RSHIFT )
	    window->rshift = !window->keyup;
	  if ( window->created ) {
	    window->HandleKeyboardMsg();
	  }
	}
      return 0;
      
    case WM_MOUSEMOVE :
      if ( window )
	{
	  window->button = 0;
	  window->keyup = FALSE;
	  window->repeat = FALSE;
	  window->mx = LOWORD(lParam);
	  window->my = HIWORD(lParam);
	  if ( window->created )
	    {
	      window->HandleMouseMsg();
	      if ( window->dragging )
		{
                  if ( window->dragging->icon )
		    SetCursor(window->dragging->icon->hIcon);
                  S_TreeView *tv = NULL;
                  tv = (S_TreeView *) 
		    window->GetObjectContaining(window->mx,
						window->my,
						S_OBJECT_TREEVIEW);
		  if ( !tv )
		    {
		      tv = window->tvDrag;
		    }
                  if ( tv )
		    {

		      POINT pt1;
		      GetCursorPos(&pt1);
		      POINT pt2 = {tv->x,tv->y};
		      ClientToScreen(tv->parent->hwnd,&pt2);
		      tv->dropTarget = tv->GetNodeUnder(pt1.x - pt2.x,pt1.y - pt2.y);
		      if ( tv->dropTarget )
                        tv->dropTarget->HiLite();
		    }
		  
		}
	    }
	}
      return 0;
      
    case WM_LBUTTONDOWN :
      if ( window )
	{
	  window->button = 1;
	  window->keyup = FALSE;
	  window->repeat = FALSE;
	  window->mx = LOWORD(lParam);
	  window->my = HIWORD(lParam);
	  SetWindowPos(window->hwndFrame,HWND_TOP,0,0,0,0,
		       SWP_NOMOVE|SWP_NOSIZE|SWP_SHOWWINDOW);
	  if ( window->created )
	    window->HandleMouseMsg();
	}
      return 0;
      
    case WM_LBUTTONUP :
      if ( window )
	{
	  window->button = 1;
	  window->keyup = TRUE;
	  window->repeat = FALSE;
	  window->mx = LOWORD(lParam);
	  window->my = HIWORD(lParam);
	  if ( window->created )
	    {
	      window->HandleMouseMsg();
	      if ( window->dragging )
		{
		  S_TreeView *tv = (S_TreeView *) 
		    window->GetObjectContaining(window->mx,
						window->my,
						S_OBJECT_TREEVIEW);
		  if ( !tv )
		    {
		      tv = window->tvDrag;
		    }
                  if ( tv )
		    {
		      tv->msg = S_TREEVIEW_ENDDRAG;
		      tv->dragObject = window->dragging;
		      (*(tv->pMsgHandler))((void *)tv);
		    }
		  
                  S_Window *w = (S_Window *)
		    window->GetObjectContaining(window->mx,
						window->my,
						S_OBJECT_WINDOW);
                  if ( w )
		    {
		      POINT pt = { window->mx,window->my };
		      ClientToScreen(window->hwnd,&pt);
		      ScreenToClient(w->hwnd,&pt);
		      w->mx = (short)pt.x;
		      w->my = (short)pt.y;
		      w->dragging = window->dragging;
		      w->HandleDropMsg();
		      w->dragging = NULL;
		    }
		  
                  window->dragging = NULL;
                  SetCursor(LoadCursor(NULL,IDC_ARROW));
                  ReleaseCapture();
		}
	    }
	}
      return 0;
      
    case WM_LBUTTONDBLCLK :
      if ( window )
	{
	  window->button = 1;
	  window->keyup = FALSE;
	  window->repeat = TRUE;
	  window->mx = LOWORD(lParam);
	  window->my = HIWORD(lParam);
	  if ( window->created )
	    window->HandleMouseMsg();
	}
      return 0;
      
    case WM_RBUTTONDOWN :
      if ( window )
	{
	  window->button = 2;
	  window->keyup = FALSE;
	  window->repeat = FALSE;
	  window->mx = LOWORD(lParam);
	  window->my = HIWORD(lParam);
	  if ( window->created )
	    window->HandleMouseMsg();
	}
      return 0;
      
    case WM_RBUTTONUP :
      if ( window )
	{
	  window->button = 2;
	  window->keyup = TRUE;
	  window->repeat = FALSE;
	  window->mx = LOWORD(lParam);
	  window->my = HIWORD(lParam);
	  if ( window->created )
	    window->HandleMouseMsg();
	}
      return 0;
      
    case WM_RBUTTONDBLCLK :
      if ( window )
	{
	  window->button = 2;
	  window->keyup = FALSE;
	  window->repeat = TRUE;
	  window->mx = LOWORD(lParam);
	  window->my = HIWORD(lParam);
	  if ( window->created )
	    window->HandleMouseMsg();
	}
      return 0;
      
    case WM_MBUTTONDOWN :
      if ( window )
	{
	  window->button = 3;
	  window->keyup = FALSE;
	  window->repeat = FALSE;
	  window->mx = LOWORD(lParam);
	  window->my = HIWORD(lParam);
	  if ( window->created )
	    window->HandleMouseMsg();
	}
      return 0;
      
    case WM_MBUTTONUP :
      if ( window )
	{
	  window->button = 3;
	  window->keyup = TRUE;
	  window->repeat = FALSE;
	  window->mx = LOWORD(lParam);
	  window->my = HIWORD(lParam);
	  if ( window->created )
	    window->HandleMouseMsg();
	}
      return 0;
      
    case WM_MBUTTONDBLCLK :
      if ( window )
	{
	  window->button = 3;
	  window->keyup = FALSE;
	  window->repeat = TRUE;
	  window->mx = LOWORD(lParam);
	  window->my = HIWORD(lParam);
	  if ( window->created )
	    window->HandleMouseMsg();
	}
      return 0;
      
    case WM_HSCROLL :
    case WM_VSCROLL :
      {
	int msg = 0;
	switch(LOWORD(wParam))
	  {
	  case SB_ENDSCROLL     : msg = S_SCROLLBAR_ENDSCROLL;     break;
	  case SB_LINELEFT      : msg = S_SCROLLBAR_LINELEFT;      break;
	  case SB_LINERIGHT     : msg = S_SCROLLBAR_LINERIGHT;     break;
	  case SB_PAGELEFT      : msg = S_SCROLLBAR_PAGELEFT;      break;
	  case SB_PAGERIGHT     : msg = S_SCROLLBAR_PAGERIGHT;     break;
	  case SB_THUMBPOSITION    :
	    msg = S_SCROLLBAR_THUMBPOSITION;
	    SetScrollPos((HWND)lParam,SB_CTL,HIWORD(wParam),TRUE);
	    break;
	  case SB_THUMBTRACK :
	    msg = S_SCROLLBAR_THUMBTRACK;
	    SetScrollPos((HWND)lParam,SB_CTL,HIWORD(wParam),TRUE);
	    break;
	  }
	_CallHwndMsgHandler( (HWND)lParam,msg,HIWORD(wParam) );
	return 0;
      }
      
    case WM_NOTIFY :
      if (_CallHwndMsgHandler( ((NMHDR *)lParam)->hwndFrom,
			       ((NMHDR *)lParam)->code,
			       lParam,
			       wParam) )
	{
	  _CallIDMsgHandler( window,
			     ((NMHDR *)lParam)->idFrom,
			     ((NMHDR *)lParam)->code,
			     lParam,
			     wParam);
	}
      return 0;
      
    case WM_COMMAND :
      if ( window && window->pCommandHook )
	{
	  return (*window->pCommandHook)(hwnd,msg,wParam,lParam);
	}
      if ( _CallHwndMsgHandler( (HWND)lParam,HIWORD(wParam),LOWORD(wParam) ) )
	{
	  _CallIDMsgHandler( window,LOWORD(wParam),HIWORD(wParam),LOWORD(wParam) );
	}
      return 0;
      
    case WM_MOVE:
      if ( window )
	{
	  RECT rcl;
	  if ( window->flFrameFlags != S_WINDOW_STATICCHILD )
	    {
	      GetWindowRect(hwnd,&rcl);
	      window->x = rcl.left;
	      window->y = rcl.top;
	      window->cx = rcl.right - rcl.left;
	      window->cy = rcl.bottom - rcl.top;
	    }
	  if ( window->lpBuffer )
	    {
	      GetClientRect(hwnd,&rcl);
	      ClientToScreen(hwnd,(LPPOINT)&rcl);
	      ClientToScreen(hwnd,(LPPOINT)&rcl+1);
	      window->xCanvas = rcl.left;
	      window->yCanvas = rcl.top;
	      if ( window->created )
		window->HandleMoveMsg();
	    }
	}
      return 0;
      
    case WM_SIZE:
      if ( window )
	{
	  RECT rcl;
	  if ( window->flFrameFlags != S_WINDOW_STATICCHILD )
	    {
	      GetWindowRect(hwnd,&rcl);
	      window->x = rcl.left;
	      window->y = rcl.top;
	      window->cx = rcl.right - rcl.left;
	      window->cy = rcl.bottom - rcl.top;
	    }
	  else
	    {
	      window->cx = LOWORD(lParam);
	      window->cy = HIWORD(lParam);
	    }
	  GetClientRect(hwnd,&rcl);
	  window->cxClient = rcl.right - rcl.left;
	  window->cyClient = rcl.bottom - rcl.top;
	  window->cxCanvas = window->cxClient;
	  window->cyCanvas = window->cyClient;
	  if ( window->created )
	    window->HandleSizeMsg();
	}
      return 0;
      
    case WM_CTLCOLORBTN :
    case WM_CTLCOLOREDIT :
    case WM_CTLCOLORLISTBOX :
    case WM_CTLCOLORMSGBOX :
    case WM_CTLCOLORSCROLLBAR :
    case WM_CTLCOLORSTATIC :
      {
	
	S_Object *obj = (S_Object *)GetWindowLong((HWND)lParam,GWL_USERDATA);
	
	//
	// Patch for multiline TEXT control scrolling bug
	//
	if ( !obj || (obj && obj->type == S_OBJECT_TEXTEDIT) )
	  return DefWindowProc(hwnd,msg,wParam,lParam);
	
	SetTextColor((HDC)wParam,obj->textColor);
	SetBkColor((HDC)wParam,obj->backGroundColor);
	SetBkMode((HDC)wParam,TRANSPARENT);
	
	return (int)obj->hBackGroundBrush;
      }
      
    case WM_PAINT :
      {
	if ( window )
	  {
	    HDC oldHdc;
            //HFONT oFont = NULL;
            PAINTSTRUCT ps;
	    
            if ( !window->lpBuffer )
	      {
		//
		// Select back old font, brush, pen, etc. before Release()
		//
		SelectObject(window->hdc,window->oldFont);

		ReleaseDC(window->hwnd,window->hdc);
		window->hdc = BeginPaint(hwnd,&ps);
		window->paintRect.xLeft = ps.rcPaint.left;
		window->paintRect.xRight = ps.rcPaint.right;
		window->paintRect.yTop = ps.rcPaint.top;
		window->paintRect.yBottom = ps.rcPaint.bottom;
		SetBkMode(window->hdc,TRANSPARENT);
		SetTextAlign(window->hdc,TA_BASELINE);
		SetStretchBltMode(window->hdc,STRETCH_DELETESCANS);
		window->SetPalette(window->palette);
		if ( ((S_Canvas *)window)->font )
                  window->oldFont =
		    (HFONT)SelectObject(window->hdc,
					((S_Canvas*)window)->font->hFont);
	      }
	    
            if ( window->created )
	      window->HandlePaintMsg();
	    
            if ( !window->lpBuffer )
	      {
		if ( window->oldFont )
                  SelectObject(window->hdc,window->oldFont);
		window->ResetPalette();
		EndPaint(hwnd,&ps);
		window->hdc = GetDC(window->hwnd);
		SetBkMode(window->hdc,TRANSPARENT);
		SetTextAlign(window->hdc,TA_BASELINE);
		SetStretchBltMode(window->hdc,STRETCH_DELETESCANS);
		window->SetPalette(window->palette);
		if ( ((S_Canvas *)window)->font )
                  window->oldFont = 
		    (HFONT)SelectObject(window->hdc,
					((S_Canvas*)window)->font->hFont);
	      }
	    
            if ( window->lpBuffer )
	      {
		oldHdc = window->hdc;
		window->hdc = BeginPaint(hwnd,&ps);
		EndPaint(hwnd,&ps);
		window->hdc = oldHdc;
	      }
	    
	    
	  }
	return 0;
      }
      
    case WM_CLOSE :
      
      /* *** This is a patch *** */
      if (  window && window->parent && 
	    (window->parent->type == S_OBJECT_WINDOW &&
	     ((S_Window *)window->parent)->rootWindow) )
	SetFocus(window->parent->hwnd);
      
      if ( window && window->created )
	{
	  if ( ((S_Canvas *)window)->font )
	    {
	      SelectObject(window->hdc,window->oldFont);
	    }
	  window->closing = TRUE;
	  window->HandleCloseMsg();
	}
      
      if ( window->closing )
	{
	  DestroyWindow(hwnd);
	}
      return 0;
      
    case WM_DESTROY :
      
      if ( window )
	{
	  if ( window->created )
	    {
	      if ( ((S_Canvas *)window)->font )
		{
		  SelectObject(window->hdc,window->oldFont);
		}
	      if ( !window->selfDelete )
		{
		  window->ReleaseChildren();
		}
	      window->HandleDestroyMsg();
	    }

	  if ( window->hdc )
	    {
	      SelectObject(window->hdc,window->oldFont);
	      ReleaseDC(window->hwndClient,window->hdc);
	      window->hdc = NULL;
	    }	  

	  window->DestroySystemCanvas();
	  
	  window->hwnd = window->hwndClient = window->hwndFrame = NULL;
	  
	  if ( window->rootWindow )
	    PostQuitMessage(0);
	  
	  if ( window->selfDelete )
	    {
	      delete window;
	    }
	}
      
      return 0;
    }
  
  return DefWindowProc(hwnd,msg,wParam,lParam);
}
#endif

#ifdef OS2
MRESULT EXPENTRY 
_S_WindowProc( HWND hwnd,ULONG msg,
	       MPARAM mp1,MPARAM mp2 )
{
  
  S_Window *window = (S_Window *)WinQueryWindowULong(hwnd,QWL_USER);
  
  switch(msg)
    {
      
    case WM_CREATE :
      if ( window )
	window->HandleCreateMsg();
      return 0;
      
    case WM_FOCUSCHANGE :
      WinDefWindowProc(hwnd,msg,mp1,mp2);
      if ( window && window->created )
	window->HandleFocusMsg();
      return 0;
      
      
    case WM_CHAR:
      if ( window )
	{
	  window->repeat   = (CHARMSG(&msg)->fs & KC_PREVDOWN) != 0;
	  window->keyup    = (CHARMSG(&msg)->fs & KC_KEYUP) != 0;
	  window->scancode = (char) CHARMSG(&msg)->scancode;
	  
	  if ( window->scancode == S_KEY_LSHIFT )
	    window->lshift = !window->keyup;
	  if ( window->scancode == S_KEY_RSHIFT )
	    window->rshift = !window->keyup;
	  
	  /* *** Handle default button behaviour *** */
	  if ( window->firstChild && !window->repeat &&
	       (window->scancode==28 || window->scancode == 90) )
	    {
	      S_Object *defaultButton = window->GetDefaultButton();
	      if ( defaultButton )
		WinPostMsg(defaultButton->hwnd,BM_CLICK,
			   (MPARAM)window->keyup,0);
	    }
	  
	  /* *** Handle TAB by selecting next TABSTOP control *** */
	  if (window->firstChild && window->scancode==15 && !window->keyup)
	    {
	      S_Object *currentFocus = window->QueryFocus();
	      if ( !currentFocus || currentFocus == (S_Object *)window )
		currentFocus = window->firstChild;
	      if ( currentFocus )
		{
		  S_Object *nextTabStop = window->GetNextTabStop(currentFocus);
		  if ( nextTabStop )
		    nextTabStop->Focus();
		}
	    }
	  
	  if ( window->created )
	    window->HandleKeyboardMsg();
	}
      return 0;
      
    case WM_MOUSEMOVE :
      if ( window )
	{
	  window->button = 0;
	  window->keyup = FALSE;
	  window->repeat = FALSE;
	  window->mx = MOUSEMSG(&msg)->x;
	  window->my = window->cyClient - MOUSEMSG(&msg)->y;
	  if ( window->created )
	    window->HandleMouseMsg();
	}
      return 0;
      
    case WM_BUTTON1DOWN :
      if ( window )
	{
	  window->button = 1;
	  window->keyup = FALSE;
	  window->repeat = FALSE;
	  window->mx = MOUSEMSG(&msg)->x;
	  window->my = window->cyClient - MOUSEMSG(&msg)->y;
	  WinSetWindowPos(window->hwndFrame,HWND_TOP,0,0,0,0,SWP_ZORDER);
	  if ( window->created )
	    window->HandleMouseMsg();
	}
      return 0;
      
    case WM_BUTTON1UP :
      if ( window )
	{
	  window->button = 1;
	  window->keyup = TRUE;
	  window->repeat = FALSE;
	  window->mx = MOUSEMSG(&msg)->x;
	  window->my = window->cyClient - MOUSEMSG(&msg)->y;
	  if ( window->created )
	    window->HandleMouseMsg();
	}
      return 0;
      
    case WM_BUTTON1DBLCLK :
      if ( window )
	{
	  window->button = 1;
	  window->keyup = FALSE;
	  window->repeat = TRUE;
	  window->mx = MOUSEMSG(&msg)->x;
	  window->my = window->cyClient - MOUSEMSG(&msg)->y;
	  if ( window->created )
	    window->HandleMouseMsg();
	}
      return 0;
      
    case WM_BUTTON2DOWN :
      if ( window )
	{
	  window->button = 2;
	  window->keyup = FALSE;
	  window->repeat = FALSE;
	  window->mx = MOUSEMSG(&msg)->x;
	  window->my = window->cyClient - MOUSEMSG(&msg)->y;
	  if ( window->created )
	    window->HandleMouseMsg();
	}
      return 0;
      
    case WM_BUTTON2UP :
      if ( window )
	{
	  window->button = 2;
	  window->keyup = TRUE;
	  window->repeat = FALSE;
	  window->mx = MOUSEMSG(&msg)->x;
	  window->my = window->cyClient - MOUSEMSG(&msg)->y;
	  if ( window->created )
	    window->HandleMouseMsg();
	}
      return 0;
      
    case WM_BUTTON2DBLCLK :
      if ( window )
	{
	  window->button = 2;
	  window->keyup = FALSE;
	  window->repeat = TRUE;
	  window->mx = MOUSEMSG(&msg)->x;
	  window->my = window->cyClient - MOUSEMSG(&msg)->y;
	  if ( window->created )
	    window->HandleMouseMsg();
	}
      return 0;
      
    case WM_BUTTON3DOWN :
      if ( window )
	{
	  window->button = 3;
	  window->keyup = FALSE;
	  window->repeat = FALSE;
	  window->mx = MOUSEMSG(&msg)->x;
	  window->my = window->cyClient - MOUSEMSG(&msg)->y;
	  if ( window->created )
	    window->HandleMouseMsg();
	}
      return 0;
      
    case WM_BUTTON3UP :
      if ( window )
	{
	  window->button = 3;
	  window->keyup = TRUE;
	  window->repeat = FALSE;
	  window->mx = MOUSEMSG(&msg)->x;
	  window->my = window->cyClient - MOUSEMSG(&msg)->y;
	  if ( window->created )
	    window->HandleMouseMsg();
	}
      return 0;
      
    case WM_BUTTON3DBLCLK :
      if ( window )
	{
	  window->button = 3;
	  window->keyup = FALSE;
	  window->repeat = TRUE;
	  window->mx = MOUSEMSG(&msg)->x;
	  window->my = window->cyClient - MOUSEMSG(&msg)->y;
	  if ( window->created )
	    window->HandleMouseMsg();
	}
      return 0;
      
    case WM_HSCROLL :
    case WM_VSCROLL :
      {
	int msg = 0;
	switch(SHORT2FROMMP(mp2))
	  {
	  case SB_ENDSCROLL      : msg = S_SCROLLBAR_ENDSCROLL;     break;
	  case SB_LINELEFT       : msg = S_SCROLLBAR_LINELEFT;      break;
	  case SB_LINERIGHT      : msg = S_SCROLLBAR_LINERIGHT;     break;
	  case SB_PAGELEFT       : msg = S_SCROLLBAR_PAGELEFT;      break;
	  case SB_PAGERIGHT      : msg = S_SCROLLBAR_PAGERIGHT;     break;
	  case SB_SLIDERTRACK    : msg = S_SCROLLBAR_THUMBTRACK;    break;
	  case SB_SLIDERPOSITION : msg = S_SCROLLBAR_THUMBPOSITION; break;
	  }
	_CallIDMsgHandler(window,(int)mp1,msg,SHORT2FROMMP(mp2));
	return 0;
      }
      
    case WM_CONTROL :
    case WM_COMMAND :
      if (_CallHwndMsgHandler(WinWindowFromID(hwnd,COMMANDMSG(&msg)->cmd),
			      SHORT2FROMMP(mp1)))
	_CallIDMsgHandler(window,COMMANDMSG(&msg)->cmd,SHORT2FROMMP(mp1));
      return 0;
      
    case WM_MOVE:
      /*
	if ( window ) {
	RECTL rcl;
	WinQueryWindowRect(window->hwndFrame,&rcl);
	window->x = rcl.xLeft;
	window->y = rcl.yTop;
	if ( window->created )
	window->HandleMoveMsg();
	}
      */
      return 0;
      
    case WM_SIZE:
      if ( window ) {
	/*
	  window->cx = SHORT1FROMMP(mp2);
	  window->cy = SHORT2FROMMP(mp2);
	*/
	
	RECTL rcl;
	WinQueryWindowRect(window->hwndFrame,&rcl);
	/*
	  window->x = rcl.xLeft;
	  window->y = rcl.yTop;
	*/
	window->cx = rcl.xRight - rcl.xLeft;
	window->cy = rcl.yTop - rcl.yBottom;
	
	WinQueryWindowRect(hwnd,&rcl);
	window->cxClient = rcl.xRight - rcl.xLeft;
	window->cyClient = rcl.yTop - rcl.yBottom;
	
	window->cxCanvas = window->cxClient;
	window->cyCanvas = window->cyClient;
	
	if ( window->created )
	  window->HandleSizeMsg();
      }
      return 0;
      
    case WM_PAINT :
      if ( window )
	{
	  ulong nc;
	  HPS oldHdc;
	  oldHdc = window->hdc;
	  window->hdc = WinBeginPaint(hwnd,NULL,0);
	  WinQueryUpdateRect(window->hwnd,(RECTL *)&window->paintRect);
	  GpiCreateLogColorTable(window->hdc,0,LCOLF_RGB,0,0,0);
	  window->SetPalette(window->palette);
	  WinRealizePalette(hwnd,window->hdc,&nc);
	  ((S_Canvas *)window)->SetFont();
	  if ( window->created )
	    window->HandlePaintMsg();
	  window->ResetPalette();
	  WinRealizePalette(hwnd,window->hdc,&nc);
	  WinEndPaint(window->hdc);
	  window->hdc = oldHdc;
	}
      return 0;
      
    case WM_CLOSE :
      if ( window && window->created )
	{
	  window->closing = TRUE;
	  window->HandleCloseMsg();
	}
      if ( window->closing )
	{
	  
	  if ( window->hdc )
	    {
	      ulong nc;
	      WinRealizePalette(window->hwnd,window->hdc,&nc);
	      WinReleasePS(window->hdc);
	      window->hdc = NULL;
	    }
	  
	  WinDestroyWindow(WinQueryWindow(hwnd,QW_PARENT));
	}
      return 0;
      
    case WM_DESTROY :
      if ( window )
	{
	  if ( window->created )
	    window->HandleDestroyMsg();

	  window->DestroySystemCanvas();
	  
	  window->hwnd = window->hwndClient = window->hwndFrame = NULL;
	  
	  if ( window->rootWindow )
	    WinPostMsg(NULL,WM_QUIT,0,0);
	  
	  if ( window->selfDelete )
	    {
	      delete window;
	    }
	}
      return 0;
    }
  
  return WinDefWindowProc(hwnd,msg,mp1,mp2);
}
#endif


int
S_Window::Run()
{
  if ( root )
    {
      rootWindow = TRUE;
      root->Run();
    }
  return 0;
}


int
S_Window::SetFont(S_Font *ifont)
{
  S_Canvas::SetFont(ifont);
  return 0;
}


int
S_Window::MenuBarIsVisible()
{
# ifdef OS2
  HWND hwndTitleBar = WinWindowFromID(hwndFrame,FID_MENU);
  return(WinIsWindowVisible(hwndTitleBar));
# endif
# ifdef WIN32
  return(GetMenu(hwnd) != NULL);
# endif
# ifdef GTK
  return 0;
# endif
}


int
S_Window::ShowTitleBar( int show )
{
  
# ifdef OS2
  HWND hwndTitleBar = WinWindowFromID(hwndFrame,FID_TITLEBAR);
  HWND hwndSysMenu = WinWindowFromID(hwndFrame,FID_SYSMENU);
  HWND hwndMinMax = WinWindowFromID(hwndFrame,FID_MINMAX); 
  HWND hwndMenu = WinWindowFromID(hwndFrame,FID_MENU);
# endif
  
  if ( !show )
    {
      
#     ifdef OS2
      if ( !WinIsWindowVisible(hwndTitleBar) )
	return FALSE;
      WinShowWindow(hwndMenu,FALSE);
      WinShowWindow(hwndTitleBar,FALSE);
      WinShowWindow(hwndMinMax,FALSE);
      WinShowWindow(hwndSysMenu,FALSE);
      
      SWP swp;
      WinQueryWindowPos( hwndFrame,&swp );
      WinSetWindowPos( hwnd,HWND_TOP,root->cxSizeBorder,root->cySizeBorder,
		       swp.cx-2*root->cxSizeBorder,
		       swp.cy-2*root->cySizeBorder,
		       SWP_SIZE | SWP_ZORDER | SWP_NOREDRAW );
      /*
	rcl.xLeft = 0;
	rcl.yBottom = swp.cy-2*cySizeBorder-cyTitleBar-cyMenu;
	rcl.xRight = swp.cx-2*cxSizeBorder;
	rcl.yTop = swp.cy-2*cySizeBorder;
	WinInvalidateRect(hwnd,(RECTL *)&rcl,FALSE);
      */
      /*         WinSetWindowPos( hwnd,HWND_TOP,0,0,0,0,SWP_ZORDER );*/
      WinInvalidateRect(hwnd,NULL,TRUE);
#     endif
#     ifdef WIN32
      if ( GetMenu(hwnd) == NULL )
	return FALSE;
      SetWindowLong(hwnd,GWL_STYLE,WS_POPUP|WS_THICKFRAME|WS_BORDER);
      SetMenu(hwnd,NULL);
      SetWindowPos( hwnd,HWND_TOP,0,0,0,0,
		    SWP_SHOWWINDOW|SWP_NOMOVE|SWP_NOSIZE );
      InvalidateRect(hwnd,NULL,FALSE);
#     endif
    }
  
  if ( show )
    {
#     ifdef OS2
      if ( WinIsWindowVisible(hwndTitleBar) )
	return TRUE;
      
      SWP swp;
      WinQueryWindowPos( hwndFrame,&swp );
      WinSetWindowPos(hwnd,HWND_BOTTOM,
		      root->cxSizeBorder,root->cySizeBorder,
		      swp.cx-2*root->cxSizeBorder,
		      swp.cy-2*root->cySizeBorder -
		      root->cyMenu-root->cyTitleBar,
		      SWP_SIZE | SWP_ZORDER | SWP_NOREDRAW );
      
      WinShowWindow(hwndMenu,TRUE);
      WinShowWindow(hwndTitleBar,TRUE);
      WinShowWindow(hwndMinMax,TRUE);
      WinShowWindow(hwndSysMenu,TRUE);
      
      /*WinSetWindowPos( hwnd,HWND_TOP,0,0,0,0,SWP_ZORDER );
	WinInvalidateRect(hwnd,NULL,FALSE);*/
#     endif
#     ifdef WIN32
      if ( menuBar ) {
	if ( GetMenu(hwnd) != NULL )
	  return TRUE;
	SetWindowLong(hwnd,GWL_STYLE,flFrameFlags);
	SetMenu(hwnd,menuBar->hMenu);
	SetWindowPos( hwnd,HWND_TOP,0,0,0,0,
		      SWP_SHOWWINDOW|SWP_NOMOVE|SWP_NOSIZE );
	InvalidateRect(hwnd,NULL,FALSE);
      }
#     endif
    }
  
  return 0;
}
