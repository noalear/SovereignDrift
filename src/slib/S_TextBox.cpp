/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
#include "S_OS.h"
#include "S_TextBox.h"
#include "S_Root.h"


S_TextBox::S_TextBox()
{
}



S_TextBox::S_TextBox(S_Object *iparent,int (*ipMsgHandler)(S_TextBox *),
                     char *text,int ix,int iy,int icx,int icy,ulong iflag)
{
  Create(iparent,ipMsgHandler,text,ix,iy,icx,icy,iflag);
}


int
S_TextBox::Create(S_Object *iparent,int (*ipMsgHandler)(S_TextBox *),
                  char *text,int ix,int iy,int icx,int icy,ulong iflag)
{
  S_Object::Create(iparent,(int(*)(S_Object *))ipMsgHandler,
		   S_OBJECT_TEXTBOX,ix,iy,icx,icy);
# ifdef OS2
  cx -= 4;
  cy -= 4;
  hwnd = WinCreateWindow(parent->hwnd,WC_ENTRYFIELD,text,
			 WS_VISIBLE | ES_AUTOSCROLL |
			 ES_MARGIN | WS_TABSTOP | iflag,
			 x+2,parent->cyClient-cy-y-2,cx-2,cy-2,
			 parent->hwnd,HWND_TOP,id,NULL,
			 &defaultPres2);
  WinSetWindowULong(hwnd,QWL_USER,(ULONG)this);
  WinSendMsg(hwnd,EM_SETTEXTLIMIT,(MPARAM)65535,(MPARAM)0);
# endif
# ifdef WIN32
  hwnd = CreateWindowEx(WS_EX_CLIENTEDGE,"EDIT",text,
			WS_VISIBLE | WS_CHILD | 
			WS_TABSTOP | ES_AUTOHSCROLL | iflag,
			x,y,cx,cy,
			parent->hwnd,(HMENU)id,root->hInst,NULL);
  SetWindowLong(hwnd,GWL_USERDATA,(slong)this);
  SendMessage(hwnd,EM_SETLIMITTEXT,(WPARAM)0,(LPARAM)0);
  SetFont(defaultFont);
# endif
# ifdef GTK
  hwndFrame = gtk_alignment_new(0.0,0.0,0.0,0.0);
  gtk_container_add(GTK_CONTAINER(parent->hwndClient),hwndFrame);
  gtk_widget_show(hwndFrame);
  hwnd = gtk_entry_new_with_max_length(1024);
  gtk_container_add(GTK_CONTAINER(hwndFrame),hwnd);
  gtk_widget_show(hwnd);
  SetSize(cx,cy);
  SetPosition(x,y);
# endif
  hwndFrame = hwndClient = hwnd;
  
  SetText(text);
  parent->AddChild(this);
  
  return 0;
}


#ifdef OS2
int
S_TextBox::SetSize(int icx,int icy)
{
  cx = icx - 4;
  cy = icy - 4;
  WinSetWindowPos(hwndFrame,hwndFrame,0,0,cx-2,cy-2,
		  SWP_SIZE|SWP_SHOW|SWP_ZORDER);
  return 0;
}


int
S_TextBox::SetPosition(int ix,int iy)
{
  x = ix;
  y = iy;
  WinSetWindowPos(hwndFrame,hwndFrame,x+2,parent->cyClient-y-cy-2,0,0,
		  SWP_MOVE|SWP_SHOW|SWP_ZORDER);
  return 0;
}
#endif


int
S_TextBox::Destroy()
{
  if ( parent ) 
    {
      parent->RemoveChild(this);
      parent = NULL;
    }
  S_Object::Destroy();
  return 0;
}


S_TextBox::~S_TextBox()
{
  Destroy();
}


char *
S_TextBox::GetString()
{
  int len = 0;
# ifdef OS2
  len = WinQueryWindowTextLength(hwnd);
# endif
# ifdef WIN32
  len = SendMessage(hwnd,WM_GETTEXTLENGTH,0,0);
# endif
  if ( str == NULL )
    str = (char *) S_malloc(len+1);
  else
    str = (char *) S_realloc(str,len+1);
# ifdef OS2
  WinQueryWindowText(hwnd,len+1,str);
# endif
# ifdef WIN32
  SendMessage(hwnd,WM_GETTEXT,len+1,(LPARAM)str);
# endif
  return str;
}


int
S_TextBox::SetReadOnly(int flag)
{
# ifdef OS2
  WinSendMsg(hwnd,MLM_SETREADONLY,(MPARAM)flag,(MPARAM)0);
# endif
# ifdef WIN32
  SendMessage(hwnd,EM_SETREADONLY,(WPARAM)flag,(LPARAM)0);
# endif
  return 0;
}


double 
S_TextBox::GetDouble()
{
  return atof(GetString());
}
