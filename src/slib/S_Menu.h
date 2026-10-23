#ifndef S_MENU
#define S_MENU

#include "S_Object.h"

#define S_MENU_APPEND -1

#ifdef OS2
   typedef HWND HMENU;
#endif
#ifdef UNIX
   typedef void *HMENU;
#endif

class S_Window;

class S_Menu : public S_Object {
public:
   S_Window *window;
   S_Menu *parent;
   S_Menu *menuBar;
   HMENU hMenu;

   int Release();

   int Init();
   S_Menu();
   int Create(char *,int);
   int Destroy();
   virtual ~S_Menu();

   int AddSeparator(int=S_MENU_APPEND);
   };


class S_MenuBar : public S_Menu {
public:
   int Init();
   S_MenuBar();
   S_MenuBar(S_Window *);
   int Create(S_Window *);
   int Destroy();
   virtual ~S_MenuBar();
   };


class S_SubMenu : public S_Menu {
public:
   int PopupOnMouse();
   int Popup(int,int);

   int CreateSubMenu(char *);

   int Init();
   S_SubMenu();
   S_SubMenu(S_Window *,char * =NULL);
   S_SubMenu(S_Menu *,char * =NULL);
   int Create(S_Window *,char * =NULL);
   int Create(S_Menu * =NULL,char * =NULL);
   int Destroy();
   virtual ~S_SubMenu();
   };


class S_MenuItem : public S_Menu {
public:
   int Init();
   S_MenuItem();
   S_MenuItem(S_Menu *,char *text,int (*)(S_MenuItem *)=NULL);
   int Create(S_Menu *,char *text,int (*)(S_MenuItem *)=NULL,int=S_MENU_APPEND);
   int Destroy();
   virtual ~S_MenuItem();

   char *GetText();
   int SetText(char *);

   int SetCheck(int);
   int IsChecked();

   int SetEnabled(int);
   int IsEnabled();
   };

#endif
