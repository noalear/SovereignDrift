/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
#include "S_OS.h"
#include "S_ToolBar.h"
#include "S_Root.h"
#include "S_Window.h"
#include "S_SystemIcon.h"


int
S_ToolBar::Init()
{
  numButtons = 0;
# ifdef WIN32
    himl = NULL;
# endif

  return 0;
}


S_ToolBar::S_ToolBar()
{
  Init();
}


S_ToolBar::S_ToolBar(S_Object *iparent,
		     int (*ipMsgHandler)(S_ToolBar *),
		     int ix,int iy,int icx,int icy,int iiconSize)
{
  Init();
  Create(iparent,ipMsgHandler,ix,iy,icx,icy,iiconSize);
}


int
S_ToolBar::Create(S_Object *iparent,
		  int (*ipMsgHandler)(S_ToolBar *),
		  int ix,int iy,int icx,int icy,
		  int iconSize)
{
  
  Destroy();
  
  S_Object::Create(iparent,(int(*)(S_Object *))ipMsgHandler,
		   S_OBJECT_TOOLBAR,ix,iy,icx,icy);
# ifdef OS2
# endif
# ifdef WIN32
  InitCommonControls();
  hwnd = CreateWindowEx(WS_EX_CLIENTEDGE,TOOLBARCLASSNAME,"",
			WS_CHILD |
			WS_VISIBLE |
			WS_CLIPSIBLINGS |
			WS_TABSTOP |
			TBSTYLE_TOOLTIPS |
			CCS_ADJUSTABLE,
			x,y,cx,cy,
			parent->hwnd,(HMENU)id,root->hInst,NULL);
  SetWindowLong(hwnd,GWL_USERDATA,(slong)this);
  SetFont(defaultFont);

  SendMessage(hwnd, TB_BUTTONSTRUCTSIZE,(WPARAM) sizeof(TBBUTTON), 0); 
  //SendMessage(hwnd, TB_AUTOSIZE, 0, 0); 

  SendMessage(hwnd,TB_SETMAXTEXTROWS,0,0);

  himl = ImageList_Create(iconSize,iconSize,ILC_COLOR24,256,512);
  SendMessage(hwnd, TB_SETIMAGELIST, 0, (LPARAM)himl);

  SetSize(icx,icy);
  SetPosition(ix,iy);

# endif
# ifdef GTK
# endif
  hwndFrame = hwndClient = hwnd;
  parent->AddChild(this);

  return 0;
}


int
S_ToolBar::Destroy()
{
  DestroyImageList();

  S_Object::Destroy();
  
  Init();
  
  return 0;
}


S_ToolBar::~S_ToolBar()
{
  Destroy();
}


int
S_ToolBar::Flush()
{
  CreateImageList();
  return 0;
}



int
S_ToolBar::CreateImageList()
{
  DestroyImageList();
# ifdef WIN32
  himl = ImageList_Create(16,16,ILC_COLOR24,256,512);
  SendMessage(hwnd, TB_SETIMAGELIST, 0, (LPARAM)himl);
# endif
  return 0;
}


int
S_ToolBar::DestroyImageList()
{
# ifdef WIN32
  if ( himl )
    {
      ImageList_Destroy(himl);
      himl = NULL;
    }
# endif
  return 0;
}


int
S_ToolBar::CheckButton(int id,int state)
{
# ifdef WIN32
  SendMessage(hwnd,TB_CHECKBUTTON,(WPARAM)id,(LPARAM)MAKELONG(state,0));
# endif
  return 0;
}



#ifdef WIN32
int
S_ToolBar::AddButton(HICON hIcon,char *text,int style)
{

  SendMessage(hwnd,TB_SETMAXTEXTROWS,0,0);

  TBBUTTON tbb; 

  tbb.iBitmap = numButtons; 
  tbb.idCommand = numButtons; 
  tbb.fsState = TBSTATE_ENABLED; 
  tbb.fsStyle = TBSTYLE_BUTTON | style; 
  tbb.dwData = numButtons; 
  tbb.iString = (int)text; 

  ImageList_AddIcon(himl,hIcon);
  SendMessage(hwnd, TB_ADDBUTTONS, (WPARAM) 1, (LPARAM) (LPTBBUTTON) &tbb);

  numButtons++;

  return 0;
}
#endif


int
S_ToolBar::AddButton(S_SystemIcon *icon,char *text,int style)
{
# ifdef WIN32
  AddButton(icon->hIcon,text,style);
# endif
  return 0;
}


int
S_ToolBar::AddSeparator(int width)
{
# ifdef WIN32
  TBBUTTON tbb; 

  tbb.iBitmap = width; 
  tbb.idCommand = 0; 
  tbb.fsState = TBSTATE_ENABLED; 
  tbb.fsStyle = BTNS_SEP; 
  tbb.dwData = 0; 
  tbb.iString = -1; 

  SendMessage(hwnd, TB_ADDBUTTONS, (WPARAM) 1, (LPARAM) (LPTBBUTTON) &tbb);
# endif
  return 0;
}

int
S_ToolBar::SetButtonEnabled(int id,int enabled)
{
# ifdef WIN32
  int state = enabled ? TBSTATE_ENABLED : TBSTATE_INDETERMINATE;
  SendMessage(hwnd,TB_SETSTATE,id,MAKELONG(state,0));
# endif
  return 0;
}
