/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
#include "S_OS.h"
#include "S_Menu.h"
#include "S_Window.h"


int
S_Menu::Init() {
   menuBar = this;
   parent = NULL;
   hMenu = NULL;
   window = NULL;
   return 0;
   }

S_Menu::S_Menu() {
   Init();
   }


int
S_Menu::Create(char *name,int itype) {
   Destroy();
   S_Object::Create();
   type = itype;
   return 0;
   }


int
S_Menu::Release()
{
  if ( IsMenu(hMenu) )
    {
      DestroyMenu(hMenu);
      hMenu = NULL;
    }
  return 0;
}


int
S_Menu::Destroy()
{
  Release();
  Init();
  return 0;
}

S_Menu::~S_Menu() {
   Destroy();
   }

S_MenuBar::S_MenuBar() {
   Init();
   }


int
S_MenuBar::Init() {
   type = S_OBJECT_MENUBAR;  
   return 0;
   }

S_MenuBar::S_MenuBar(S_Window *iwindow) {
   Init();
   Create(iwindow);
   }


int
S_MenuBar::Create(S_Window *iwindow) {
   Destroy();
   S_Menu::Create("S_MenuBar",S_OBJECT_MENUBAR);
   window = iwindow;
#  ifdef OS2
      hMenu = WinCreateWindow(window->hwndFrame,WC_MENU,"",MS_ACTIONBAR,
                              0,0,0,0,window->hwndFrame,HWND_TOP,
                              FID_MENU,NULL,&defaultPres2);
      WinSendMsg(window->hwndFrame,WM_UPDATEFRAME,MPFROMSHORT(FCF_MENU),0);
#  endif
#  ifdef WIN32
      hMenu = CreateMenu();
      SetMenu(window->hwndFrame,hMenu);
#  endif
   hwnd = hwndFrame = hwndClient = (HWND)hMenu;
   window->menuBar = this;
   if ( window )
     {
       window->AddChild(this);
     }
   return 0;
   }


int
S_MenuBar::Destroy()
{
  Release();
  if ( window )
    {
      window->RemoveChild(this);
    }
  Init();
  return 0;
}

S_MenuBar::~S_MenuBar() {
   Destroy();
   }


int
S_SubMenu::Init() {
   type = S_OBJECT_SUBMENU;
   return 0;
   }

S_SubMenu::S_SubMenu() {
   Init();
   }

S_SubMenu::S_SubMenu(S_Menu *iparent,char *text) {
   Init();
   Create(iparent,text);
   }


int
S_SubMenu::Create(S_Menu *iparent,char *text) {
   Destroy();
   S_Menu::Create(text,S_OBJECT_SUBMENU);
   parent = iparent;
   if ( parent ) {
      menuBar = parent->menuBar;
      window = parent->window;
      CreateSubMenu(text);
      if ( window )
	{
	  window->AddChild(this);
	}
      }
   return 0;
   }

S_SubMenu::S_SubMenu(S_Window *iwindow,char *text) {
   Init();
   Create(iwindow,text);
   }



int
S_SubMenu::Create(S_Window *iwindow,char *text) {
   Destroy();
   S_Menu::Create(text,S_OBJECT_SUBMENU);
   window = iwindow;
   CreateSubMenu(text);
   if ( window )
     {
       window->AddChild(this);
     }
   return 0;
   }


int
S_SubMenu::CreateSubMenu(char *text) {
#  ifdef OS2
  if ( parent )
    {
      hMenu = WinCreateWindow(parent->hMenu,WC_MENU,text,0,0,0,0,0,
                              parent->hMenu,HWND_TOP,id,NULL,&defaultPres2);
      
      MENUITEM menuItem;
      menuItem.iPosition = MIT_END;
      menuItem.afStyle = MIS_SUBMENU|MIS_TEXT;
      menuItem.afAttribute = 0;
      menuItem.id = id;
      menuItem.hwndSubMenu = hMenu;
      menuItem.hItem = 0;
      WinSendMsg(parent->hMenu,MM_INSERTITEM,MPFROMP(&menuItem),MPFROMP(text));
    }
#  endif
#  ifdef WIN32
      hMenu = CreatePopupMenu();
      if ( parent )
         AppendMenu(parent->hMenu,MF_ENABLED|MF_STRING|MF_POPUP,
                    (UINT)hMenu,text);
      if ( window && window->hwndFrame )
         DrawMenuBar(window->hwndFrame);
#  endif
   hwnd = hwndFrame = hwndClient = (HWND)hMenu;
   return 0;
   }


int
S_Menu::AddSeparator(int pos) {
#  ifdef OS2
      MENUITEM menuItem;
      menuItem.iPosition = MIT_END;
      menuItem.afStyle = MIS_SEPARATOR;
      menuItem.afAttribute = 0;
      menuItem.id = -1;
      menuItem.hwndSubMenu = NULL;
      menuItem.hItem = 0;
      WinSendMsg(hMenu,MM_INSERTITEM,MPFROMP(&menuItem),0);
#  endif
#  ifdef WIN32
      if ( pos == S_MENU_APPEND )
	{
          AppendMenu(hMenu,MF_SEPARATOR,-1,NULL);
	}
      else
        {
	  MENUITEMINFO mii;
	  mii.cbSize = sizeof(mii);
	  mii.fMask = MIIM_TYPE;
	  mii.fType = MFT_SEPARATOR;
	  mii.fState = MFS_ENABLED;
	  mii.wID = 0;
	  mii.hSubMenu = NULL;
	  mii.hbmpChecked = NULL;
	  mii.hbmpUnchecked = NULL;
	  mii.dwItemData = 0;
	  mii.dwTypeData = 0;
	  mii.cch = 0;
	  InsertMenuItem(hMenu,pos,TRUE,&mii);
        }
      if ( window && window->hwndFrame )
	DrawMenuBar(window->hwndFrame);
#  endif
   return 0;
   }


int
S_SubMenu::PopupOnMouse() {
#  ifdef WIN32
      POINT pt;
      GetCursorPos(&pt);
      Popup(pt.x,pt.y);
#  endif
   return 0;
   }


int
S_SubMenu::Popup(int x,int y) {
#  ifdef OS2
/*      WinPopupMenu(hMenu,x,y);*/
#  endif
#  ifdef WIN32
  if ( window )
    {
      TrackPopupMenu(hMenu,TPM_LEFTALIGN,x,y,0,window->hwnd,NULL);
    }
#  endif
   return 0;
   }


int
S_SubMenu::Destroy() {
#  ifdef OS2
  if ( hMenu/* && (window && !window->closing)*/ ) {
     WinSendMsg(hMenu,WM_CLOSE,0,0);
     hMenu = NULL;
     }
#  endif
#  ifdef WIN32
   Release();
#  endif
  if ( window )
    {
      window->RemoveChild(this);
    }
   Init();
   return 0;
   }


S_SubMenu::~S_SubMenu() {
   Destroy();
   }

S_MenuItem::S_MenuItem() {
   Init();
   }


int
S_MenuItem::Init() {
   type = S_OBJECT_MENUITEM;
   return 0;
   }

S_MenuItem::S_MenuItem(S_Menu *iparent,char *text,
                       int (*ipMsgHandler)(S_MenuItem *)) {
   Init();
   Create(iparent,text,ipMsgHandler);
   }


int
S_MenuItem::Create(S_Menu *iparent,char *text,
                   int (*ipMsgHandler)(S_MenuItem *),
                   int pos) {
   Destroy();
   S_Menu::Create(text,S_OBJECT_MENUITEM);
   parent = iparent;
   if ( parent )
     {
       window = parent->window;
       menuBar = parent->menuBar;
     }
   pMsgHandler = (int(*)(void *))ipMsgHandler;
#  ifdef OS2
      MENUITEM menuItem;
      menuItem.iPosition = MIT_END;
      menuItem.afStyle = MIS_TEXT;
      menuItem.afAttribute = 0;
      menuItem.id = id;
      menuItem.hwndSubMenu = NULL;
      menuItem.hItem = 0;
      WinSendMsg(parent->hMenu,MM_INSERTITEM,MPFROMP(&menuItem),MPFROMP(text));
#  endif
#  ifdef WIN32
      if ( parent)
	{
	  if ( pos == S_MENU_APPEND )
	    AppendMenu(parent->hMenu,MF_ENABLED|MF_STRING,id,text);
	  else {
	    MENUITEMINFO mii;
	    mii.cbSize = sizeof(mii);
	    mii.fMask = MIIM_TYPE | MIIM_STATE | MIIM_ID;
	    mii.fType = MFT_STRING;
	    mii.fState = MFS_ENABLED;
	    mii.wID = id;
	    mii.hSubMenu = NULL;
	    mii.hbmpChecked = NULL;
	    mii.hbmpUnchecked = NULL;
	    mii.dwItemData = (slong)this;
	    mii.dwTypeData = text;
	    mii.cch = strlen(text);
	    //mii.hbmpItem = NULL;
	    InsertMenuItem(parent->hMenu,pos,TRUE,&mii);
	  }
	}
      if ( window && window->hwndFrame )
	{
	  DrawMenuBar(window->hwndFrame);
	}
#  endif
      if ( window )
	{
	  window->AddChild(this);
	}
   return 0;
   }


int
S_MenuItem::Destroy() {
   if ( str ) {
      S_free(str);
      str = NULL;
      }
#  ifdef OS2
      if ( parent && parent->hMenu && (window && !window->closing) ) {
         WinSendMsg(parent->hMenu,MM_DELETEITEM,MPFROM2SHORT(id,FALSE),0);
         parent->hMenu = NULL;
         }
#  endif
#  ifdef WIN32
      if ( parent && parent->hMenu && (window && !window->closing) ) {
         RemoveMenu(parent->hMenu,id,MF_BYCOMMAND);
         if ( window && window->hwndFrame )
            DrawMenuBar(window->hwndFrame);
         }
#  endif
   if ( window ) {
      window->RemoveChild(this);
      }
   Init();
   return 0;
   }

S_MenuItem::~S_MenuItem() {
   Destroy();
   }


int
S_MenuItem::SetCheck(int state) {
#  ifdef OS2
      if ( parent )
         WinSendMsg(parent->hMenu,MM_SETITEMATTR,
                    MPFROM2SHORT(id,FALSE),
                    MPFROM2SHORT(MIA_CHECKED,state ? MIA_CHECKED : 0));
#  endif
#  ifdef WIN32
      if ( parent )
	{
	  if ( state )
	    CheckMenuItem(parent->hMenu,id,MF_BYCOMMAND | MF_CHECKED);
	  else
	    CheckMenuItem(parent->hMenu,id,MF_BYCOMMAND | MF_UNCHECKED);
	}
#  endif
   return 0;
   }


int
S_MenuItem::SetEnabled(int state) {
#  ifdef OS2
      if ( parent )
         WinSendMsg(parent->hMenu,MM_SETITEMATTR,
                    MPFROM2SHORT(id,FALSE),
                    MPFROM2SHORT(MIA_DISABLED,state ? 0 : MIA_DISABLED));
#  endif
#  ifdef WIN32
      if ( parent )
	{
	  if ( state )
	    EnableMenuItem(parent->hMenu,id,MF_BYCOMMAND | MF_ENABLED);
	  else
	    EnableMenuItem(parent->hMenu,id,MF_BYCOMMAND | MF_GRAYED);
	}
#  endif
   return 0;
   }


int
S_MenuItem::IsChecked() {
#  ifdef OS2
      if ( parent )
         return (int) WinSendMsg(parent->hMenu,MM_QUERYITEMATTR,
                                 MPFROM2SHORT(id,FALSE),
                                 MPFROMSHORT(MIA_CHECKED));
#  endif
#  ifdef WIN32
      if ( parent )
         return GetMenuState(parent->hMenu,id,MF_BYCOMMAND) & MF_CHECKED;
#  endif
   return FALSE;
   }


int
S_MenuItem::IsEnabled() {
#  ifdef OS2
      if ( parent )
         return (int) !WinSendMsg(parent->hMenu,MM_QUERYITEMATTR,
                                 MPFROM2SHORT(id,FALSE),
                                 MPFROMSHORT(MIA_DISABLED));
#  endif
#  ifdef WIN32
      if ( parent )
         return GetMenuState(parent->hMenu,id,MF_BYCOMMAND) & MF_ENABLED;
#  endif
   return FALSE;
   }

char *
S_MenuItem::GetText() {
#  ifdef OS2
#  endif
#  ifdef WIN32
      if ( parent ) {
         int len = GetMenuString(parent->hMenu,id,NULL,0,MF_BYCOMMAND);
         if ( str )
            S_free(str);
         str = (char *)S_malloc(len+1);
         GetMenuString(parent->hMenu,id,str,len+1,MF_BYCOMMAND);
         }
#  endif
   return str;
   }


int
S_MenuItem::SetText(char *text) {
#  ifdef OS2
#  endif
#  ifdef WIN32
      if ( parent ) {
         MENUITEMINFO mii;
         mii.cbSize = sizeof(mii);
         mii.fMask = MIIM_TYPE;
         mii.fType = MFT_STRING;
         mii.fState = 0;
         mii.wID = 0;
         mii.hSubMenu = NULL;
         mii.hbmpChecked = NULL;
         mii.hbmpUnchecked = NULL;
         mii.dwItemData = 0;
         mii.dwTypeData = text;
         mii.cch = sizeof(text);
         SetMenuItemInfo(parent->hMenu,id,FALSE,&mii);
         }
#  endif
   return 0;
   }
