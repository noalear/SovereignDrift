#ifndef SF_CLIENTWINDOW
#define SF_CLIENTWINDOW

#define SF_CLIENTWINDOW_NOMODE   0
#define SF_CLIENTWINDOW_LOGOMODE 1
#define SF_CLIENTWINDOW_MENUMODE 2
#define SF_CLIENTWINDOW_GAMEMODE 3

class SF_ClientWindow : public S_GWindow {
public:

   int mode;
   int SetNoMode();
   int SetLogoMode();
   int SetMenuMode();

   int numColors;
   S_SystemBitmap bmpStardockLogo;
   S_SystemBitmap bmpMenu;
   S_SystemBitmap bmpMenuMouseOver;

   int Init();
   SF_ClientWindow();
   int Create(S_Root *);
   int Destroy();
   ~SF_ClientWindow();

   int HandleCreateMsg();
   int HandlePaintMsg();
   int HandleCloseMsg();
   };

#endif
