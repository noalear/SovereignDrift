#include "S_OS.h"
#include "S_GBmpListBox.h"

S_GBmpListBox::S_GBmpListBox() {
   selectedLine = -1;
   numLines = 0;
   scroll = 0;
   bmp = NULL;
   S_GWidget::Init();
   }


int
S_GBmpListBox::Create(S_GWindow *iparent,int (*ipMsgHandler)(S_GBmpListBox *),
                   char *filename,int ix,int iy,int icx,int icy) {
   S_GWidget::Create(iparent,(int (*)(S_GWidget *))ipMsgHandler,
                     filename,1,ix,iy,icx,icy);
   return rc;
   }


int
S_GBmpListBox::CreateScrollBar( char *sbFilename,
                                char *upFilename,
                                char *dnFilename,
                                char *slFilename ) {
   scrollBar.Create( parent,S_GBmpListBox_ScrollBarHandler,sbFilename,x+cx,y);
   scrollBar.SetSize( scrollBar.width,height);
   scrollBar.wparent = this;
   scrollBar.CreateUpButton(upFilename,S_GBmpListBox_UpButtonHandler);
   scrollBar.CreateDownButton(dnFilename,S_GBmpListBox_DownButtonHandler);
   scrollBar.CreateSlider(slFilename,S_GBmpListBox_SliderHandler);
   return 0;
   }


int
S_GBmpListBox::SetSliderHeight() {
   if ( numLines > 0 ) {
      int totalHeight = 0;
      for ( int i=0; i<numLines; i++ )
         totalHeight += bmp[i]->cy;
      int h = (scrollBar.height-2*scrollBar.width) * cy / totalHeight;
      if ( h > scrollBar.height-2*scrollBar.width )
         h = scrollBar.height-2*scrollBar.width;
      scrollBar.slider.height = h;
      scrollBar.slider.Draw();
      }
   return 0;
   }


int
S_GBmpListBox::Destroy() {
   S_GWidget::Destroy();
   if ( bmp ) {
      S_free(bmp);
      bmp = NULL;
      }
   return 0;
   }

S_GBmpListBox::~S_GBmpListBox() {
   Destroy();
   }


int
S_GBmpListBox::HandleMouseMsg() {
   mouseIsOver = Contains(parent->mx,parent->my);
   if ( numLines > 0 && mouseIsOver ) {
      if ( parent->button == 1 && !parent->keyup ) {
         if ( (int)selectedLine != (int)(scroll + (parent->my-y)/bmp[0]->cy) ){
            selectedLine = scroll + (parent->my-y)/bmp[0]->cy;
            if ( pMsgHandler )
               (*pMsgHandler)(this);
            Draw();
            }
         }
      return TRUE;
      }
   return FALSE;
   }


int
S_GBmpListBox::AddItem(S_SystemBitmap *ibmp) {
   if ( numLines == 0 )
      bmp = (S_SystemBitmap **)S_malloc(sizeof(S_SystemBitmap *));
   else
      bmp = (S_SystemBitmap **)
             S_realloc(bmp,sizeof(S_SystemBitmap *)*(numLines+1));
   bmp[numLines] = ibmp;
   numLines++;

   int maxval = 0;
   if ( (int)numLines > (int)(cy/bmp[0]->cy) )
      maxval = numLines - cy/bmp[0]->cy;
   scrollBar.slider.SetRange(0,maxval);

   return 0;
   }


int
S_GBmpListBox::SetLine(int index,S_SystemBitmap *ibmp) {
   if ( index >=0 && index < numLines ) {
      bmp[index] = ibmp;
      }
   Draw();
   return 0;
   }


int
S_GBmpListBox::Draw(int frame) {
   if ( parent && !parent->closing ) {
      SetSliderHeight();
      S_GWidget::Draw(frame);
      parent->SetClipRect(x,y,x+cx,y+cy);
      int x0 = x;
      int y0 = y;
      for ( int i=scroll; i<numLines; i++ ) {
         if ( i == selectedLine )
            bmp[i]->Blit(parent,1,x0,y0);
         else
            bmp[i]->Blit(parent,0,x0,y0);
         y0 += bmp[i]->cy;
         }
      parent->ResetClipRect();
      }
   return 0;
   }


int
S_GBmpListBox_UpButtonHandler(S_GButton *b) {
   S_GBmpListBox *c = (S_GBmpListBox *)b->wparent->wparent;
   if ( c ) {
      if ( c->scroll > 0 ) {
         c->scroll--;
         c->scrollBar.slider.SetValue(c->scroll);
         c->Draw();
         }
      }
   return 0;
   }


int
S_GBmpListBox_DownButtonHandler(S_GButton *b) {
   S_GBmpListBox *c = (S_GBmpListBox *)b->wparent->wparent;
   if ( c ) {
      c->scroll++;
      if ( c->scroll > c->scrollBar.slider.maxValue )
         c->scroll = c->scrollBar.slider.maxValue;
      c->scrollBar.slider.SetValue(c->scroll);
      c->Draw();
      }
   return 0;
   }


int
S_GBmpListBox_SliderHandler(S_GSlider *s) {
   S_GBmpListBox *c = (S_GBmpListBox *)s->wparent->wparent;
   if ( c->scroll != s->value ) {
      c->scroll = s->value;
      c->Draw();
      }
   return 0;
   }


int
S_GBmpListBox_ScrollBarHandler(S_GScrollBar *sb) {
   S_GBmpListBox *c = (S_GBmpListBox *)sb->wparent;
   switch (sb->msg) {
      case S_GSCROLLBAR_PAGEUP :
         c->scroll --;
         if ( c->scroll < 0 )
            c->scroll = 0;
         c->scrollBar.slider.SetValue(c->scroll);
         c->Draw();
         break;
      case S_GSCROLLBAR_PAGEDOWN :
         c->scroll ++;
         if ( c->scroll > c->scrollBar.slider.maxValue )
            c->scroll = c->scrollBar.slider.maxValue;
         c->scrollBar.slider.SetValue(c->scroll);
         c->Draw();
         break;
      }
   return 0;
   }
