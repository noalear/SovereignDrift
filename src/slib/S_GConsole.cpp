#include "S_OS.h"
#include "S_GConsole.h"

S_GConsole::S_GConsole() {
   buffer = NULL;
   numLines = 0;
   scroll = 0;
   xmargin = 5;
   ymargin = 15;
   S_GWidget::Init();
   }


int
S_GConsole::Create(S_GWindow *iparent,int (*ipMsgHandler)(S_GConsole *),
                   char *filename,int ix,int iy,int icx,int icy) {
   S_GWidget::Create(iparent,(int (*)(S_GWidget *))ipMsgHandler,
                     filename,1,ix,iy,icx,icy);
   return rc;
   }


int
S_GConsole::CreateScrollBar( char *sbFilename,
                             char *upFilename,
                             char *dnFilename,
                             char *slFilename ) {
   scrollBar.Create( parent,S_GConsole_ScrollBarHandler,sbFilename,x+width,y);
   scrollBar.SetSize( scrollBar.width,height);
   scrollBar.wparent = this;
   scrollBar.CreateUpButton(upFilename,S_GConsole_UpButtonHandler);
   scrollBar.CreateDownButton(dnFilename,S_GConsole_DownButtonHandler);
   scrollBar.CreateSlider(slFilename,S_GConsole_SliderHandler);
   return 0;
   }


int
S_GConsole::SetSliderHeight() {
   if ( numLines > 0 && parent && parent->lMaxBaselineExt ) {
      int h = (scrollBar.height-2*scrollBar.width) * height /
              (numLines*parent->lMaxBaselineExt);
      if ( h > scrollBar.height-2*scrollBar.width )
	{
	  h = scrollBar.height-2*scrollBar.width;
	}
      if ( h < scrollBar.width )
	{
	  h = scrollBar.width;
	}
      scrollBar.slider.height = h;
      scrollBar.slider.Draw();
      }
   return 0;
   }


int
S_GConsole::Destroy() {
   S_GWidget::Destroy();
   if ( buffer ) {
      for ( int i=0; i<numLines; i++ ) {
         if ( buffer[i] ) {
            S_free(buffer[i]);
            buffer[i] = NULL;
            }
         }
      S_free(buffer);
      buffer = NULL;
      numLines = 0;
      }
   return 0;
   }

S_GConsole::~S_GConsole() {
   Destroy();
   }


int
S_GConsole::Flush() {
   for ( int i=0; i<numLines; i++ )
      if ( buffer[i] )
         buffer[i][0] = 0;
   return 0;
   }


int
S_GConsole::AddLine(char *line)
{
  if ( !line || !parent || (parent && parent->closing) )
    {
      return 0;
    }

  int maxWidth = cx-10-scrollBar.width;
  int maxChars = maxWidth/parent->lAveCharWidth;
  SizeStruct size;
  int i;
  int j = maxChars;
  int len;
  char *str = S_strdup(line);
  char *tstr = str;
  
   if ( str )
     {
       do
	 {
	   char *tmp = S_strdup(tstr);
	   if ( tmp )
	     {
	       len = strlen(tstr);
	       
	       if ( gfont )
		 {
		   size.cx = gfont->width*strlen(tmp);
		 }
	       else if ( parent )
		 {
		   size = parent->GetStringSize(tmp);
		 }
	       
	       if ( size.cx > maxWidth )
		 {
		   i = maxChars;
		   do
		     {
		       i--;
		       if ( i >= 0 )
			 {
			   if ( tmp[i] == ' ' )
			     {
			       tmp[i] = 0;
			     }
			   
			   if ( gfont )
			     {
			       size.cx = gfont->width*strlen(tmp);
			     }
			   else if ( parent )
			     {
			       size = parent->GetStringSize(tmp);
			     }
			 }
		     } while ( i>0 && (int)size.cx>maxWidth );

		   //
		   // Should not be necessary, but what the heck.
		   //
		   if ( i < 0 )
		     {
		       i = 0;
		     }
		   if ( i > maxChars )
		     {
		       i = maxChars;
		     }

		   j = maxChars;
		   if ( i > 0 && i < maxChars)
		     {
		       j = i;
		     }
		 }
	       
	       AppendLine(tmp);
	       
	       if ( gfont )
		 {
		   size.cx = gfont->width*strlen(tstr);
		 }
	       else if ( parent )
		 {
		   size = parent->GetStringSize(tstr);
		 }
	       
	       if ( size.cx > maxWidth )
		 {
		   tstr += j+1;
		   if ( tstr < str )
		     {
		       tstr = str;
		     }
		   if ( tstr > str + strlen(str) )
		     {
		       tstr = str + strlen(str);
		     }
		 }
	       
	       S_free(tmp);
	     }
	   
         } while ( size.cx > maxWidth );
       S_free(str);
     }
   
   int maxval = 0;
   if ( numLines > (int)(height/parent->lMaxBaselineExt+1) )
     {
       maxval = numLines - height/parent->lMaxBaselineExt + 1;
     }
   scrollBar.slider.SetRange(0,maxval);
   scroll = scrollBar.slider.maxValue;
   scrollBar.slider.SetValue(scrollBar.slider.maxValue);

   return 0;
   }


int
S_GConsole::AppendLine(char *text) {
   if ( numLines == 0 )
      buffer = (char **)S_malloc(sizeof(char *));
   else
      buffer = (char **)S_realloc(buffer,sizeof(char *)*(numLines+1));
   buffer[numLines] = NULL;
   numLines++;
   AppendText(text);
   return 0;
   }


int
S_GConsole::SetLine(int index,char *text) {
   if ( index >=0 && index < numLines ) {
      if ( buffer[index] ) {
         S_free(buffer[index]);
         buffer[index] = S_strdup(text);
         }
      }
   else {
      AddLine(text);
      }
   return 0;
   }


int
S_GConsole::AppendText(char *text) {
   int index = numLines - 1;
   if ( text && index >= 0 ) {
      if ( buffer[index] == NULL ) {
         buffer[index] = S_strdup(text);
         }
      else {
         int len1 = strlen(buffer[index]);
         int len2 = strlen(text);
         buffer[index] = (char *)S_realloc(buffer[index],len1+len2+1);
         if ( buffer[index] )
            sprintf(buffer[index],"%s%s",buffer[index],text);
         }
      }
   return 0;
   }


int
S_GConsole::Draw(int frame) {
   if ( parent && !parent->closing ) {
      SetSliderHeight();
      S_GWidget::Draw(frame);
      parent->SetClipRect(x+2,y+2,x+width-2,y+height-2);
      for ( int i=scroll; i<numLines; i++ ) {
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
                               color);
         }
      parent->ResetClipRect();
      }
   return 0;
   }


int
S_GConsole_UpButtonHandler(S_GButton *b) {
   S_GConsole *c = (S_GConsole *)b->wparent->wparent;
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
S_GConsole_DownButtonHandler(S_GButton *b) {
   S_GConsole *c = (S_GConsole *)b->wparent->wparent;
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
S_GConsole_SliderHandler(S_GSlider *s) {
   S_GConsole *c = (S_GConsole *)s->wparent->wparent;
   if ( c->scroll != s->value ) {
      c->scroll = s->value;
      c->Draw();
      }
   return 0;
   }


int
S_GConsole_ScrollBarHandler(S_GScrollBar *sb) {
   S_GConsole *c = (S_GConsole *)sb->wparent;
   switch (sb->msg) {
      case S_GSCROLLBAR_PAGEUP :
         c->scroll -= c->height/c->parent->lMaxBaselineExt;
         if ( c->scroll < 0 )
            c->scroll = 0;
         c->scrollBar.slider.SetValue(c->scroll);
         c->Draw();
         break;
      case S_GSCROLLBAR_PAGEDOWN :
         c->scroll += c->height/c->parent->lMaxBaselineExt;
         if ( c->scroll > c->scrollBar.slider.maxValue )
            c->scroll = c->scrollBar.slider.maxValue;
         c->scrollBar.slider.SetValue(c->scroll);
         c->Draw();
         break;
      }
   return 0;
   }
