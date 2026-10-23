#include "S_OS.h"
#include "S_GWidget.h"
#include "S_GWindow.h"
#include "S_ZArchive.h"
#include "S_Root.h"


int
S_GWidget::Init() {
   S_Tree::Init();
   S_SystemBitmap::Init();
   x = 0;
   y = 0;
   msg = 0;
   width = 0;
   height = 0;
   enabled = TRUE;
   visible = TRUE;
   toggle = FALSE;
   font = NULL;
   gfont = NULL;
   color = RGB_GREEN;
   gcolor = 0;
   parent = NULL;
   wparent = NULL;
   pMsgHandler = NULL;
   mouseIsOver = FALSE;
   leftMouseButtonIsDown = FALSE;
   rightMouseButtonIsDown = FALSE;
   middleMouseButtonIsDown = FALSE;
   return 0;
   }

S_GWidget::S_GWidget() {
   Init();
   }


int
S_GWidget::Create(S_GWindow *iparent,int (*ipMsgHandler)(S_GWidget *),
                  char *filename,int inumFrames,
                  int ix,int iy,int icx,int icy) {

   S_SystemBitmap::ZCreate(filename,S_BITMAP_MASK_NONE,
                           S_BITMAP_BLIT_OPAQUE,inumFrames,100,0);

   parent = iparent;
   x = ix;
   y = iy;
   width = cx;
   height = cy;
   if ( icx >= 0 )
      width = icx;
   if ( icy >= 0 )
      height = icy;
   pMsgHandler = (int(*)(void *))ipMsgHandler;

   if ( parent )
      parent->widgets.AddChild(this);
   return rc;
   }


int
S_GWidget::Destroy() {
   S_Tree::Destroy();
   S_SystemBitmap::Destroy();
   return 0;
   }

S_GWidget::~S_GWidget() {
   Destroy();
   }


int
S_GWidget::SetEnabled(int ienabled) {
   enabled = ienabled;
   return 0;
   }


int
S_GWidget::SetToggle(int itoggle) {
   toggle = itoggle;
   return 0;
   }


int
S_GWidget::Hide() {
   visible = FALSE;
   Draw();
   return 0;
   }


int
S_GWidget::Show() {
   visible = TRUE;
   Draw();
   return 0;
   }


int
S_GWidget::Draw(int frame) {
   if ( visible && parent && !parent->closing ) {
      SetFont();
      Blit(parent,frame,x,y,x+width,y+height,0,0);
      }
   return 0;
   }


int
S_GWidget::SetPosition(int ix,int iy) {
   x = ix;
   y = iy;
   Draw();
   return 0;
   }


int
S_GWidget::SetSize(int iwidth,int iheight) {
   width = iwidth;
   height = iheight;
   Draw();
   return 0;
   }


int
S_GWidget::Contains(int ix,int iy) {
   return ix >= x && ix < (int)(x+width) && iy >= y && iy < (int)(y+height);
   }


int
S_GWidget::HandleCreateMsg() {
   return 0;
   }


int
S_GWidget::HandleKeyboardMsg() {
   return 0;
   }


int
S_GWidget::HandleMouseMsg() {
   return 0;
   }


int
S_GWidget::HandlePaintMsg() {
   Draw();
   return 0;
   }


int
S_GWidget::HandleCloseMsg() {
   return 0;
   }


int
S_GWidget::SetTextColor(ulong icolor) {
   color = icolor;
   return 0;
   }


int
S_GWidget::SetFont(S_Font *ifont) {
   if ( ifont )
      font = ifont;
   if ( parent )
     parent->SetFont(font);
   return 0;
   }


int
S_GWidget::SetGFont(S_GFont *igfont) {
   if ( igfont )
      gfont = igfont;
   if ( parent ) {
      parent->lAveCharWidth = gfont->width;
      parent->lMaxBaselineExt = gfont->height;
      }
   return 0;
   }

