#include "sf.h"
#include "SF_ClientWindow.h"
#include "SF_Globals.h"
#include "SF_Client.h"
#include "SF_Externals.h"


int
SF_ClientWindow::Init() {
   S_GWindow::Init();
   draggable = FALSE;
   mode = SF_CLIENTWINDOW_NOMODE;
   return 0;
   }

SF_ClientWindow::SF_ClientWindow() {
   Init();
   }


int
SF_ClientWindow::Create(S_Root *iroot) {
   char windowTitle[48];
   sprintf(windowTitle,
           "Stellar Frontier v%d.%d.%d (Preview # 3)",
           SF_MAJOR_VERSION,SF_MINOR_VERSION,SF_MINOR_MINOR_VERSION);
   S_GWindow::Create(iroot,NULL,windowTitle,NULL,1,0,0,iroot->cx,iroot->cy);
   return 0;
   }


int
SF_ClientWindow::Destroy() {
   S_GWindow::Destroy();
   return 0;
   }

SF_ClientWindow::~SF_ClientWindow() {
   Destroy();
   }


int
SF_ClientWindow::HandleCreateMsg() {

   sf->backBuffer=new S_CanvasBuffer(_root->cx,_root->cy,_root->bitsPerPixel);

   numColors = _root->numColors;
   if ( numColors == 0 || numColors > 65536 )
      numColors = 65536;

   SetLogoMode();

   Show();
   return 0;
   }


int
SF_ClientWindow::HandlePaintMsg() {
   switch(mode) {
      case SF_CLIENTWINDOW_LOGOMODE :
         sf->backBuffer->Blit(this,0,0,S_CANVASBUFFER_BLIT_ALL);
         break;
      case SF_CLIENTWINDOW_MENUMODE :
         break;
      case SF_CLIENTWINDOW_GAMEMODE :
         break;
      default :
         DrawFilledRect(0,0,cxCanvas,cyCanvas,0);
      }
   return 0;
   }


int
SF_ClientWindow::HandleCloseMsg() {
   SetNoMode();
   return 0;
   }


int
SF_ClientWindow::SetMenuMode() {

   char filename[128];

   int cx = 800;
   if ( _root->cx >= 1024 )
      cx = 1024;

   if ( bmpMenu.rc ) {
      sprintf(filename,"%s%d\\sf-menu1-%d.bmp",SF_BmpFileDir,numColors,cx);
      _OsFilename(filename);
      bmpMenu.Create(filename);
      }

   if ( bmpMenuMouseOver.rc ) {
      sprintf(filename,"%s%d\\sf-menu2-%d.bmp",SF_BmpFileDir,numColors,cx);
      _OsFilename(filename);
      bmpMenuMouseOver.Create(filename);
      }

   return 0;
   }


int
SF_ClientWindow::SetLogoMode() {

   char filename[128];

   sprintf(filename,"%s%d\\sf-sds",SF_BmpFileDir,numColors);
   _OsFilename(filename);

   bmpStardockLogo.Create(filename);

   sf->backBuffer->DrawFilledRect(0,0,cxCanvas,cyCanvas,0);
   bmpStardockLogo.Blit(sf->backBuffer,0,
                     (sf->backBuffer->cxCanvas-bmpStardockLogo.cx)/2,
                     (sf->backBuffer->cyCanvas-bmpStardockLogo.cy)/2);

   mode = SF_CLIENTWINDOW_LOGOMODE;

   return 0;
   }


int
SF_ClientWindow::SetNoMode() {
   mode = SF_CLIENTWINDOW_NOMODE;
   bmpStardockLogo.Destroy();
   sf->backBuffer->DrawFilledRect(0,0,cxCanvas,cyCanvas,0);
   return 0;
   }

