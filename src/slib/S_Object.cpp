#include "S_OS.h"
#include "S_Object.h"
#include "S_Font.h"
#include "S_Root.h"
#include "S_StaticText.h"
#include "S_Window.h"

int _objectIndex = 0;

int
S_Object::Init()
{
  parent = NULL;
  pMsgHandler = NULL;
  
  hwnd = NULL;
  hwndFrame = NULL;
  hwndClient = NULL;
  
  type = S_OBJECT_GENERIC;
  
  x = 0;
  y = 0;
  cx = 0;
  cy = 0;
  puser = NULL;
  
  str = NULL;
  
  firstChild = NULL;
  lastChild = NULL;
  
  next = NULL;
  prev = NULL;
  
  label = NULL;
  
  id = _objectIndex++;
  
  defaultFont = NULL;
  font = NULL;
  
# ifdef WIN32
  textColor = 0x00000000;
  backGroundRGB = backGroundColor = RGB_INVERT(GetSysColor(COLOR_BTNFACE));
  hBackGroundBrush = NULL;
# endif
  
  return 0;
}


S_Object::S_Object()
{
  Init();
}


S_Object::S_Object(S_Object *iparent,int (*ipMsgHandler)(S_Object *),
                   int itype,int ix,int iy,int icx,int icy)
{
  Init();
  Create(iparent,ipMsgHandler,itype,ix,iy,icx,icy);
}


int
S_Object::Create(S_Object *iparent,int (*ipMsgHandler)(S_Object *),
                 int itype,int ix,int iy,int icx,int icy)
{
  Destroy();
  parent = iparent;
  pMsgHandler = (int(*)(void *))ipMsgHandler;
  type = itype;
  x = ix;
  y = iy;
  cx = icx;
  cy = icy;
  if ( parent )
    root = parent->root;
  
  defaultFont = new S_Font(S_FONT_HELV,8);
  font = defaultFont;
# ifdef OS2
  backGroundColor = CLR_PALEGRAY;
  backGroundRGB = 0x00CCCCCC;
  textColor = CLR_BLACK;
# endif
  
  switch ( type )
    {
    case S_OBJECT_LISTBOX :
    case S_OBJECT_COMBOBOX :
    case S_OBJECT_TEXTBOX :
    case S_OBJECT_TEXTEDIT :
    case S_OBJECT_TREEVIEW :
    case S_OBJECT_CONTAINER :
#     ifdef WIN32
      backGroundRGB = backGroundColor = RGB_INVERT(RGB_WHITE);
#     endif
#     ifdef OS2
      backGroundRGB = RGB_WHITE;
      backGroundColor = CLR_WHITE;
#     endif
      break;
    }
  
# ifdef OS2
  defaultPres1.fcparam.id = PP_FOREGROUNDCOLORINDEX;
  defaultPres1.fcparam.cb = sizeof(defaultPres1.fcparam.ulColor);
  defaultPres1.fcparam.ulColor = textColor;
  
  defaultPres1.bcparam.id = PP_BACKGROUNDCOLORINDEX;
  defaultPres1.bcparam.cb = sizeof(defaultPres1.fcparam.ulColor);
  defaultPres1.bcparam.ulColor = backGroundColor;
  
  defaultPres1.fntparam.id = PP_FONTNAMESIZE;
  defaultPres1.fntparam.cb = 20;
  sprintf(defaultPres1.fntparam.szFontNameSize,"%d.%s",
	  defaultFont->pointSize,defaultFont->faceName);
  
  defaultPres1.cb = sizeof(defaultPres1.fcparam) +
    sizeof(defaultPres1.bcparam) +
    sizeof(defaultPres1.fntparam);
  
  defaultPres2.fcparam.id = PP_FOREGROUNDCOLORINDEX;
  defaultPres2.fcparam.cb = sizeof(defaultPres2.fcparam.ulColor);
  defaultPres2.fcparam.ulColor = textColor;
  
  defaultPres2.fntparam.id = PP_FONTNAMESIZE;
  defaultPres2.fntparam.cb = 20;
  sprintf(defaultPres2.fntparam.szFontNameSize,"%d.%s",
	  defaultFont->pointSize,defaultFont->faceName);
  
  defaultPres2.cb = sizeof(defaultPres2.fcparam) +
    sizeof(defaultPres2.fntparam);
# endif
# ifdef WIN32
  hBackGroundBrush = CreateSolidBrush(RGB_INVERT(backGroundColor));
# endif
  
  if ( parent && 
       (type == S_OBJECT_STATICTEXT || type == S_OBJECT_RADIOBUTTON ||
        type == S_OBJECT_CHECKBOX) )
    SetBackGroundColor(parent->backGroundColor);
  
  return 0;
}


int
S_Object::Destroy()
{
# ifdef WIN32
  if ( hBackGroundBrush )
    {
      DeleteObject(hBackGroundBrush);
      hBackGroundBrush = NULL;
    }
# endif
  if ( label )
    {
      delete label;
      label = NULL;
    }
  if ( defaultFont )
    {
      SendMessage(hwnd,WM_SETFONT,(WPARAM)NULL,MAKELPARAM(TRUE,0));
      delete defaultFont;
      defaultFont = NULL;
    }
  if ( str )
    {
      S_free(str);
      str = NULL;
    }
  Init();
  return 0;
}


int
S_Object::Release()
{
  if ( defaultFont )
    {
      delete defaultFont;
      defaultFont = NULL;
    }
  return 0;
}


S_Object::~S_Object()
{
  Destroy();
}


int
S_Object::AddChild(S_Object *newChild)
{
  if ( !newChild )
    return 0;
  if ( firstChild == NULL )
    {
      newChild->prev = NULL;
      newChild->next = NULL;
      firstChild = newChild;
      lastChild = newChild;
    }
  else
    {
      lastChild->next = newChild;
      newChild->prev = lastChild;
      lastChild = newChild;
      lastChild->next = NULL;
    }
  return 0;
}


int
S_Object::RemoveChild(S_Object *child)
{
  if ( !child )
    return 0;
  if ( child == firstChild )
    {
      firstChild = firstChild->next;
      child->prev = NULL;
      child->next = NULL;
      child->parent = NULL;
      return 0;
    }
  if ( lastChild && child == lastChild )
    lastChild = lastChild->prev;
  if ( child->prev )
    child->prev->next = child->next;
  if ( child->next )
    child->next->prev = child->prev;
  return 0;
}


char *
S_Object::GetText() {
  if ( !hwnd ) return 0;
  
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
S_Object::SetBackGroundColor(ulong color)
{
# ifdef OS2
# endif
# ifdef WIN32
  backGroundRGB = color;
  backGroundColor = color;
  if ( hBackGroundBrush )
    {
      DeleteObject(hBackGroundBrush);
      hBackGroundBrush = NULL;
    }
  hBackGroundBrush = CreateSolidBrush(RGB_INVERT(backGroundColor));
# endif
  return 0;
}


int
S_Object::SetTextColor(ulong color)
{
# ifdef WIN32
  textColor = color;
# endif
  return 0;
}


int
S_Object::SetText(char *text)
{
  if ( !hwnd ) return 0;
# ifdef OS2
  WinSetWindowText(hwndFrame,text);
# endif
# ifdef WIN32
  SendMessage(hwnd,WM_SETTEXT,0,(LPARAM)text);
# endif
  return 0;
}


#define S_FONT_NAMESIZECHARS 20

int
S_Object::SetFont(char *faceName,int pointSize)
{
  if ( !hwnd ) return 0;
# ifdef OS2
  char fontNameSize[S_FONT_NAMESIZECHARS];
  sprintf(fontNameSize,"%d.%s",pointSize,faceName);
  WinSetPresParam(hwnd,PP_FONTNAMESIZE,
		  S_FONT_NAMESIZECHARS,(PVOID)fontNameSize);
# endif
# ifdef WIN32
  delete defaultFont;
  defaultFont = new S_Font(faceName,pointSize);
  font = defaultFont;
  SendMessage(hwnd,WM_SETFONT,(WPARAM)font->hFont,MAKELPARAM(TRUE,0));
# endif
  return 0;
}


int
S_Object::SetFont(S_Font *ifont)
{
  if ( !hwnd ) return 0;
# ifdef OS2
  char fontNameSize[S_FONT_NAMESIZECHARS];
  sprintf(fontNameSize,"%d.%s",ifont->pointSize,ifont->faceName);
  WinSetPresParam(hwnd,PP_FONTNAMESIZE,
		  S_FONT_NAMESIZECHARS,(PVOID)fontNameSize);
# endif
# ifdef WIN32
  font = ifont;
  SendMessage(hwnd,WM_SETFONT,(WPARAM)font->hFont,MAKELPARAM(TRUE,0));
# endif
  return 0;
}


int
S_Object::SetEnabled(int flag)
{
  if ( !hwnd ) return 0;
# ifdef OS2
  WinEnableWindow(hwnd,flag);
# endif
# ifdef WIN32
  EnableWindow(hwnd,flag);
# endif
  return 0;
}


int
S_Object::IsEnabled()
{
  if ( !hwnd ) return 0;
# ifdef OS2
  return WinIsWindowEnabled(hwnd);
# endif
# ifdef WIN32
  return IsWindowEnabled(hwnd);
# endif
# ifdef GTK
  return 0;
# endif
}


int
S_Object::Close(int post)
{
  if ( !hwnd ) return 0;
  if ( post )
    {
#     ifdef OS2
      WinPostMsg(hwnd,WM_CLOSE,0,0);
#     endif
#     ifdef WIN32
      PostMessage(hwnd,WM_CLOSE,0,0);
#     endif
    }
  else
    {
#     ifdef OS2
      WinSendMsg(hwnd,WM_CLOSE,0,0);
#     endif
#     ifdef WIN32
      SendMessage(hwnd,WM_CLOSE,0,0);
#     endif
    }
  if ( type != S_OBJECT_WINDOW )
    hwnd = NULL;
  return 0;
}


int
S_Object::Refresh()
{
  if ( !hwnd ) return 0;
# ifdef OS2
  WinInvalidateRect(hwnd,NULL,TRUE);
# endif
# ifdef WIN32
  InvalidateRect(hwnd,NULL,TRUE);
# endif
  return 0;
}


int
S_Object::SetSize(int icx,int icy)
{
  if ( !hwnd ) return 0;
  cx = icx;
  cy = icy;
  cxClient = cx;
  cyClient = cy;
# ifdef OS2
  if ( parent && !WinIsChild(hwnd,HWND_DESKTOP)  )
    WinSetWindowPos(hwndFrame,hwndFrame,
		    x,parent->cyClient-y-cy,cx,cy,
		    SWP_SIZE|SWP_ZORDER|SWP_MOVE);
  else if ( root )
    WinSetWindowPos(hwndFrame,hwndFrame,x,root->cy-y-cy,cx,cy,
		    SWP_SIZE|SWP_ZORDER|SWP_MOVE);
# endif
# ifdef WIN32
  SetWindowPos(hwnd,NULL,0,0,cx,cy,SWP_NOZORDER|SWP_NOMOVE);
# endif
# ifdef GTK
  gtk_widget_set_usize(hwnd,cx,cy);
# endif
  if ( label )
    label->SetSize(cx,label->cy);
  return 0;
}


int
S_Object::SetPosition(int ix,int iy)
{
  if ( !hwnd ) return 0;
  x = ix;
  y = iy;
# ifdef OS2
  if ( parent && WinIsChild(hwnd,HWND_DESKTOP) )
    WinSetWindowPos(hwndFrame,hwndFrame,
		    x,parent->cyClient-y-cy,0,0,
		    SWP_MOVE|SWP_ZORDER);
  else if ( root )
    WinSetWindowPos(hwndFrame,hwndFrame,x,root->cy-y-cy,0,0,
		    SWP_MOVE|SWP_ZORDER);
# endif
# ifdef WIN32
  SetWindowPos(hwnd,NULL,x,y,0,0,SWP_NOZORDER|SWP_NOSIZE);
# endif
# ifdef GTK
  gtk_widget_set_uposition(hwnd,x,y);
# endif
  if ( label )
    label->SetPosition(x,y-label->cy);
  return 0;
}


int
S_Object::HasFocus()
{
  if ( !hwnd ) return 0;
# ifdef OS2
  return WinQueryFocus(HWND_DESKTOP) == hwnd;
# endif
# ifdef WIN32
  return GetFocus() == hwnd;
# endif
# ifdef GTK
  return 0;
# endif
}


S_Object *S_Object::QueryFocus()
{
  S_Object *focusObject = NULL;
# ifdef OS2
  HWND hwndFocus = WinQueryFocus(HWND_DESKTOP);
  if ( hwndFocus )
    focusObject = (S_Object *)WinQueryWindowULong(hwndFocus,QWL_USER);
# endif
# ifdef WIN32
  HWND hwndFocus = GetFocus();
  if ( hwndFocus )
    focusObject = (S_Object *)GetWindowLong(hwndFocus,GWL_USERDATA);
# endif
  return focusObject;
}


int
S_Object::Focus()
{
  if ( !hwnd ) return 0;
# ifdef OS2
  WinSetFocus(HWND_DESKTOP,hwnd);
# endif
# ifdef WIN32
  SetFocus(hwnd);
# endif
  return 0;
}


int
S_Object::IsVisible()
{
  if ( !hwnd ) return 0;
# ifdef OS2
  return WinIsWindowVisible(hwnd);
# endif
# ifdef WIN32
  return IsWindowVisible(hwnd);
# endif
# ifdef GTK
  return 0;
# endif
}


int
S_Object::Show()
{
  if ( !hwnd ) return 0;
# ifdef OS2
  if ( parent && WinIsChild(hwnd,HWND_DESKTOP) )
    WinSetWindowPos(hwndFrame,HWND_TOP,x,parent->cyClient-y-cy,cx,cy,
		    SWP_MOVE | SWP_SIZE | SWP_ACTIVATE | SWP_SHOW |
		    SWP_ZORDER );
  else if ( root )
    WinSetWindowPos(hwndFrame,HWND_TOP,x,root->cy-y-cy,cx,cy,
		    SWP_MOVE | SWP_SIZE | SWP_ACTIVATE | SWP_SHOW |
		    SWP_ZORDER );
  /*WinShowWindow(hwndFrame,TRUE);*/
# endif
# ifdef WIN32
  ShowWindow(hwndFrame,SW_SHOW);
# endif
  if ( label )
    label->Show();
  if ( firstChild && firstChild->IsTabStop() )
    firstChild->Focus();
  return 0;
}


S_Object *
S_Object::GetDefaultButton()
{
  S_Object *def = NULL;
  for ( def=firstChild; def!=NULL && !def->IsDefaultButton(); def=def->next );
  return def;
}


S_Object *
S_Object::GetNextTabStop(S_Object *currentFocus)
{
  
  S_Object *nextObject = currentFocus;
  S_Object *nextTabStop = NULL;
  do
    {
      nextObject = nextObject->next;
      if ( !nextObject )
	nextObject = firstChild;
      if ( nextObject->IsTabStop() )
	nextTabStop = nextObject;
    } while ( !nextTabStop && nextObject != currentFocus );
  
  return nextTabStop;
}


int
S_Object::IsDefaultButton()
{
  if ( !hwnd ) return 0;
# ifdef OS2
  return type == S_OBJECT_BUTTON &&
    WinQueryWindowULong(hwnd,QWL_STYLE) & BS_DEFAULT;
# endif
# ifdef WIN32
  return type == S_OBJECT_BUTTON &&
    GetWindowLong(hwnd,GWL_STYLE) & BS_DEFPUSHBUTTON;
# endif
# ifdef GTK
  return 0;
# endif
}


int
S_Object::IsTabStop()
{
  if ( !hwnd ) return 0;
  if ( !IsEnabled() ) return FALSE;
# ifdef OS2
  return WinQueryWindowULong(hwnd,QWL_STYLE) & WS_TABSTOP &&
    type != S_OBJECT_COMBOBOX && type != S_OBJECT_WINDOW &&
    type != S_OBJECT_STATICTEXT;
# endif
# ifdef WIN32
  return GetWindowLong(hwnd,GWL_STYLE) & WS_TABSTOP;
# endif
# ifdef GTK
  return 0;
# endif
}


int
S_Object::Hide()
{
  if ( !hwnd ) return 0;
# ifdef OS2
  WinShowWindow(hwndFrame,FALSE);
# endif
# ifdef WIN32
  ShowWindow(hwndFrame,SW_HIDE);
# endif
  if ( label )
    label->Hide();
  return 0;
}


int
S_Object::AddLabel(char *itext,int icx,int icy)
{
  label = new S_StaticText(this->parent,itext,x,y-icy,icx,icy);
  return 0;
}


int
S_Object::GetNumChildren()
{
  if ( !firstChild )
    return 0;
  int nc = 0;
  S_Object *tmp = firstChild;
  do
    {
      nc++;
      tmp=tmp->next;
    } while ( tmp ); 
  return nc;
}


S_Object *
S_Object::GetObjectContaining(int ix,int iy,int otype,int flag)
{

# ifdef WIN32
  HWND top = GetTopWindow(hwnd);
  while ( top )
    {
      S_Object *tmp = (S_Object *)GetWindowLong(top,GWL_USERDATA);
      S_Object *obj = NULL;
      if ( tmp )
	{
	  if ( flag )
	    obj = tmp->GetObjectContaining(ix,iy,otype,FALSE);
	  else
	    obj = tmp->GetObjectContaining(ix-x,iy-y,otype,FALSE);
	  if ( obj && obj->IsVisible() )
	    return obj;
	}
      top = GetNextWindow(top,GW_HWNDNEXT);
    }
# endif
  /*
  for ( S_Object *tmp=firstChild; tmp!=NULL; tmp=tmp->next ) {
    S_Object *obj = NULL;
    if ( flag )
      obj = tmp->GetObjectContaining(ix,iy,otype,FALSE);
    else
      obj = tmp->GetObjectContaining(ix-x,iy-y,otype,FALSE);
    if ( obj && obj->IsVisible() )
      return obj;
  }
  */
  
  if ( !flag && type==otype && x < ix && ix < x+cx && y < iy && iy < y+cy )
    {
      return (S_Object *)this;
    }
  
  return NULL;
}


int
S_Object::ReleaseChildren()
{
  for ( S_Object *tmp=firstChild; tmp!=NULL; tmp=tmp->next )
    {
      tmp->ReleaseChildren();
    }

  Release();
  
  return 0;
}


int
S_Object::Profile(int indent)
{
  for ( int i=0; i<indent; i++ )
    printf(" ");
  printf("%d %s\n",type,GetText());
  for ( S_Object *tmp=firstChild; tmp!=NULL; tmp=tmp->next )
    tmp->Profile(indent+1);
  return 0;
}

