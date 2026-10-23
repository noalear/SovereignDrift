#include "S_OS.h"
#include "S_GListBox.h"


int
S_GListBox::Init() {
   mouseOverLine = -1;
   selectedLine = -1;
   S_GConsole::Init();
   return 0;
   }

S_GListBox::S_GListBox() {
   Init();
   }


int
S_GListBox::Create(S_GWindow *iparent,int (*ipMsgHandler)(S_GListBox *),
                   char *filename,int ix,int iy,int icx,int icy) {
   S_GConsole::Create(iparent,(int (*)(S_GConsole *))ipMsgHandler,
                      filename,ix,iy,icx,icy);
   return 0;
   }


int
S_GListBox::AddLine(char *line) {
   if ( !line || !parent ) return 0;
   AppendLine(line);
   int maxval = 0;
   if ( numLines > (int)(cy/parent->lMaxBaselineExt) )
      maxval = numLines - cy/parent->lMaxBaselineExt;
   scrollBar.slider.SetRange(0,maxval);
   return 0;
   }

S_GListBox::~S_GListBox() {
   Destroy();
   }


int
S_GListBox::Draw(int frame) {
   if ( parent && !parent->closing ) {
      SetSliderHeight();
      S_GWidget::Draw(frame);
      parent->SetClipRect(x+2,y+2,x+width-2,y+height-2);
      for ( int i=scroll; i<numLines; i++ ) {
         ulong tcolor = color;
         if ( i == selectedLine )
            tcolor = highlightColor;
         ulong gcolor = 0;
         if ( i == selectedLine )
            gcolor = 1;
         if ( gfont )
            gfont->DrawString(parent,
                              buffer[i],
                              x+xmargin,
                              y+ymargin+(i-scroll)*parent->lMaxBaselineExt,
                              gcolor);
         else
            parent->DrawString(buffer[i],
                               x+xmargin,
                               y+ymargin+(i-scroll)*parent->lMaxBaselineExt,
                               tcolor);
         }
      parent->ResetClipRect();
      }
   return 0;
   }


int
S_GListBox::HandleMouseMsg() {
   mouseIsOver = Contains(parent->mx,parent->my);
   if ( mouseIsOver ) {
      mouseOverLine = scroll + 
                     (parent->my-y-ymargin+parent->lMaxBaselineExt)/
                      parent->lMaxBaselineExt;
      if ( parent->button == 1 && !parent->keyup ) {
         selectedLine = mouseOverLine;
         Draw();
         }
      if ( pMsgHandler )
         (*pMsgHandler)(this);
      return TRUE;
      }
   return FALSE;
   }
