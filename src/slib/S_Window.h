/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
#ifndef S_WINDOW
#define S_WINDOW

#include "S_Canvas.h"
#include "S_Object.h"
#include "S_TreeView.h"
#include "S_ToolBar.h"

#ifdef WIN32
# define S_WINDOW_STATICCHILD   WS_CHILD
# define S_WINDOW_SPLASH   ( WS_POPUP | WS_THICKFRAME | WS_BORDER )
# define S_WINDOW_DEFAULT CW_USEDEFAULT
# define S_WINDOW_STANDARD ( WS_OVERLAPPEDWINDOW )
# define S_WINDOW_DIALOG ( WS_OVERLAPPED )
# define S_WINDOW_BLANK ( WS_POPUP | WS_SYSMENU )
# define S_WINDOW_BORDERED (WS_POPUP | WS_DLGFRAME | WS_SYSMENU )
# define S_WINDOW_MINIMIZEBUTTON (WS_MINIMIZEBOX|WS_SYSMENU)
# define S_WINDOW_MAXIMIZEBUTTON (WS_MAXIMIZEBOX|WS_SYSMENU)
# define S_WINDOW_SYSTEMMENU (WS_SYSMENU)
# define UWM_SYSTEMTRAY (WM_USER+1)
#endif
#ifdef OS2
# define S_WINDOW_STATICCHILD (FCF_NOBYTEALIGN)
# define S_WINDOW_SPLASH    (FCF_SIZEBORDER|FCF_NOMOVEWITHOWNER|FCF_NOBYTEALIGN) 
# define S_WINDOW_DEFAULT   0xFFFFFFFF
# define S_WINDOW_STANDARD (FCF_TITLEBAR|FCF_SIZEBORDER|FCF_MINMAX|FCF_TASKLIST|FCF_SYSMENU|FCF_SHELLPOSITION|FCF_NOMOVEWITHOWNER|FCF_NOBYTEALIGN)
# define S_WINDOW_DIALOG (FCF_TITLEBAR|FCF_SHELLPOSITION|FCF_NOMOVEWITHOWNER|FCF_NOBYTEALIGN|FCF_DLGBORDER)
# define S_WINDOW_BLANK (FCF_SHELLPOSITION|FCF_NOMOVEWITHOWNER|FCF_NOBYTEALIGN|FCF_TASKLIST)
# define S_WINDOW_BORDERED (FCF_SHELLPOSITION|FCF_NOMOVEWITHOWNER|FCF_NOBYTEALIGN|FCF_DLGBORDER|FCF_TASKLIST)
# define S_WINDOW_MINIMIZEBUTTON 0
# define S_WINDOW_MAXIMIZEBUTTON 0
# define S_WINDOW_SYSTEMMENU     0
#endif
#ifdef XWIN
# define HWND_DESKTOP 0
# define S_WINDOW_DEFAULT  0xFFFFFFFF
# define S_WINDOW_STANDARD 0
# define S_WINDOW_DIALOG 0
# define S_WINDOW_SPLASH 0
# define S_WINDOW_BLANK 0
# define S_WINDOW_BORDERED 0
# define S_WINDOW_STATICCHILD 0
# define S_WINDOW_MINIMIZEBUTTON 0
# define S_WINDOW_MAXIMIZEBUTTON 0
# define S_WINDOW_SYSTEMMENU     0
#endif

class S_MenuBar;
class S_TreeViewNode;
class S_SystemIcon;
class S_Font;
class S_Root;

class S_Window : public S_Canvas, public S_Object
{
 public:
  
  S_MenuBar *menuBar;
  
  /* ************************* */
  /* *** Basic window data *** */
  /* ************************* */
  int rootWindow;
  int created;
  int closing;
  int selfDelete;
  
# define S_WINDOW_NULL             0
# define S_WINDOW_CREATE           1
# define S_WINDOW_SIZE             2
# define S_WINDOW_MOVE             3
# define S_WINDOW_PAINT            4
# define S_WINDOW_CLOSE            5
# define S_WINDOW_DESTROY          6
# define S_WINDOW_KEYBOARD         7
# define S_WINDOW_MOUSE            8
# define S_WINDOW_DROP             9
# define S_WINDOW_FOCUS           10
# define S_WINDOW_SYSTEMTRAYMOUSE 11
# define S_WINDOW_EXITMENU        12
# define S_WINDOW_ENTERMENU       13
  
  int Delete();
  
  virtual int HandleCreateMsg();
  virtual int HandleSizeMsg();
  virtual int HandleMoveMsg();
  virtual int HandleFocusMsg();
  virtual int HandlePaintMsg();
  virtual int HandleCloseMsg();
  virtual int HandleDestroyMsg();
  virtual int HandleMouseMsg();
  virtual int HandleKeyboardMsg();
  virtual int HandleDropMsg();
  virtual int HandleSystemTrayMouseMsg();
  virtual int HandleExitMenuMsg();
  virtual int HandleEnterMenuMsg();
  
# ifdef WIN32
  int (*pCommandHook)(HWND,UINT,WPARAM,LPARAM);
  int (*pKeyboardHook)(HWND,UINT,WPARAM,LPARAM);
  int (*pGenericHook)(HWND,UINT,WPARAM,LPARAM);
# endif
  
  RectStruct paintRect;
  char lshift;
  char rshift;
  char keyup;
  char scancode;
  char repeat;
  char button;
  short mx,my;
  
# ifdef WIN32
  HICON hIcon;
# endif
  
  S_TreeViewNode *dragging;
  S_SystemIcon *dragIcon;
  S_TreeView *tvDrag;
  
  /* ***************************** */
  /* *** Contructor/Destructor *** */
  /* ***************************** */
  int Init();
  S_Window();
  S_Window( S_Root *,S_Object *,char *,int (*)(S_Window *) = NULL,int = 0,
	    int=S_WINDOW_DEFAULT,int=S_WINDOW_DEFAULT,
	    int=S_WINDOW_DEFAULT,int=S_WINDOW_DEFAULT,
	    ulong=S_WINDOW_STANDARD,void * =NULL );
  int Create(S_Root *,S_Object *,char *,int (*)(S_Window *) = NULL,int = 0,
	     int=S_WINDOW_DEFAULT,int=S_WINDOW_DEFAULT,
	     int=S_WINDOW_DEFAULT,int=S_WINDOW_DEFAULT,
	     ulong=S_WINDOW_STANDARD,void * =NULL);
  int Destroy();
  virtual ~S_Window();
  
  ulong flFrameFlags;
  int ShowTitleBar(int);
  int MenuBarIsVisible();
  
  int CreateSystemCanvas();
  int RefreshSystemCanvas();
  int DefreshSystemCanvas();
  int DestroySystemCanvas();
  
  int DisableCloseButton();
  
  int CreateSystemTrayIcon();
  int DestroySystemTrayIcon();
  
  int SetFont(S_Font *);
  int SetSize(int,int);
  
  int Maximize();
  int Minimize();
  int FullScreen();
  
  int Run();
};

#ifdef WIN32
LRESULT CALLBACK _S_WindowProc(HWND,UINT,WPARAM,LPARAM);
/* port 2026-10-12: 'int' added; pre-standard C++ allowed it to be implied */
int _CallHwndMsgHandler( HWND,ulong=0,ulong=0,ulong=0 );
int _CallIDMsgHandler(S_Window *,int,ulong=0,ulong=0,ulong=0);
#endif
#ifdef OS2
MRESULT EXPENTRY _S_WindowProc(HWND,ULONG,MPARAM,MPARAM);
_CallHwndMsgHandler( HWND,ulong=0,ulong=0,ulong=0 );
_CallIDMsgHandler(S_Window *,int,ulong=0,ulong=0,ulong=0);
#endif

#define S_KEY_ESC 1

#define S_KEY_NUM_1 79
#define S_KEY_NUM_2 80
#define S_KEY_NUM_3 81
#define S_KEY_NUM_4 75
#define S_KEY_NUM_5 76
#define S_KEY_NUM_6 77
#define S_KEY_NUM_7 71
#define S_KEY_NUM_8 72
#define S_KEY_NUM_9 73

#define S_KEY_1 2
#define S_KEY_2 3
#define S_KEY_3 4
#define S_KEY_4 5
#define S_KEY_5 6
#define S_KEY_6 7
#define S_KEY_7 8
#define S_KEY_8 9
#define S_KEY_9 10
#define S_KEY_0 11

#define S_KEY_TAB 15

#define S_KEY_Q 16
#define S_KEY_W 17
#define S_KEY_E 18
#define S_KEY_R 19
#define S_KEY_T 20
#define S_KEY_Y 21
#define S_KEY_U 22
#define S_KEY_I 23
#define S_KEY_O 24
#define S_KEY_P 25

#define S_KEY_ENTER     28
#define S_KEY_NUM_ENTER 90 
#define S_KEY_A 30
#define S_KEY_S 31
#define S_KEY_D 32
#define S_KEY_F 33
#define S_KEY_G 34
#define S_KEY_H 35
#define S_KEY_J 36
#define S_KEY_K 37
#define S_KEY_L 38

#define S_KEY_Z 44
#define S_KEY_X 45
#define S_KEY_C 46
#define S_KEY_V 47
#define S_KEY_B 48
#define S_KEY_N 49
#define S_KEY_M 50
#define S_KEY_LT 51
#define S_KEY_GT 52
#define S_KEY_QUESTION 53

#define S_KEY_SPACE 57

#define S_KEY_F1  59
#define S_KEY_F2  60
#define S_KEY_F3  61
#define S_KEY_F4  62
#define S_KEY_F5  63
#define S_KEY_F6  64
#define S_KEY_F7  65
#define S_KEY_F8  66
#define S_KEY_F9  67
#define S_KEY_F10 68
#define S_KEY_F11 87
#define S_KEY_F12 88

#define S_KEY_MINUS     12
#define S_KEY_PLUS      13
#define S_KEY_BACKSPACE 14
#define S_KEY_NUM_MINUS 74
#define S_KEY_HOME      71
#define S_KEY_NUM_PLUS  78

#define S_KEY_BACKSLASH 43
#define S_KEY_LSHIFT    42 
#define S_KEY_RSHIFT    54

#define S_KEY_LCTRL     29

#ifdef UNIX
# define S_KEY_RCTRL     91
#endif
#ifdef OS2
# define S_KEY_RCTRL     91
#endif
#ifdef WIN32
# define S_KEY_RCTRL     29
#endif

#endif
