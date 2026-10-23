#include "S_OS.h"
#include "S_GTextBox.h"


int
S_GTextBox::Init() {
   scroll = 0;
   cursor = 0;
   xmargin = 5;
   ymargin = 12;
   return 0;
   }


S_GTextBox::S_GTextBox() {
   Init();
   }


int
S_GTextBox::Create(S_GWindow *iparent,int (*ipMsgHandler)(S_GTextBox *),
                   char *filename,int ix,int iy,int icx,int icy) {
   Destroy();
   S_GWidget::Create(iparent,(int (*)(S_GWidget *))ipMsgHandler,
                     filename,2,ix,iy,icx,icy);
   return rc;
   }


int
S_GTextBox::Destroy() {
   Init();
   return 0;
   }

S_GTextBox::~S_GTextBox() {
   Destroy();
   }


int
S_GTextBox::SetText(char *text) {
  if ( text )
     buffer.Create(text);
   return 0;
   }


int
S_GTextBox::Flush() {
	if ( buffer.str )
	{
       buffer.str[0] = 0;
	}

   return 0;
   }


uchar S_keymap[64] = {
  0,  0,'1','2','3','4','5','6','7','8','9','0','-','=',  0,  0,
'q','w','e','r','t','y','u','i','o','p','[',']',  0,  0,'a','s',
'd','f','g','h','j','k','l',';','\'',  0,  0,  0,'z','x','c','v',
'b','n','m',',','.','/',  0,'\\',  0,' ',  0,  0,  0,  0,  0,  0 };

uchar S_skeymap[64] = {
  0,  0,'!','@','#','$','%','^','&','*','(',')','_','+',  0,  0,
'Q','W','E','R','T','Y','U','I','O','P','{','}',  0,  0,'A','S',
'D','F','G','H','J','K','L',':','"',  0,  0,  0,'Z','X','C','V',
'B','N','M','<','>','?',  0,'|',  0,' ',  0,  0,  0,  0,  0,  0 };


int
S_GTextBox::HandleKeyboardMsg() {
   if ( mouseIsOver && !parent->keyup ) {
      if ( parent->scancode < 64 ) {
         if ( parent->scancode == S_KEY_BACKSPACE && strlen(buffer.str) > 0 ) {
            buffer.str[strlen(buffer.str)-1] = 0;
            Draw();
            }
         if ( S_keymap[parent->scancode] ) {
	   if ( !buffer.str ) {
               buffer.str = (char *)S_malloc(2);
               buffer.str[0] = 0;
	       }
            else 
               buffer.str = (char *)S_realloc(buffer.str,strlen(buffer.str)+2);
            if ( parent->lshift || parent->rshift )
               sprintf(buffer.str,"%s%c",
                       buffer.str,S_skeymap[parent->scancode]);
            else
               sprintf(buffer.str,"%s%c",
                       buffer.str,S_keymap[parent->scancode]);
            Draw();
            }
         }
      }

   if ( mouseIsOver && pMsgHandler )
      (*pMsgHandler)(this);

   return 0;
   }


int
S_GTextBox::HandleMouseMsg() {
   switch ( parent->button ) {
      case 0 : {
         int oldMouseIsOver = mouseIsOver;
         mouseIsOver = Contains(parent->mx,parent->my) && enabled;
         if ( oldMouseIsOver != mouseIsOver ) {
            Draw();
	    }
         return FALSE;
         }
      }

   return 0;
   }


int
S_GTextBox::Draw(int frame) {
   if ( parent && !parent->closing ) {
      S_GWidget::Draw(mouseIsOver);
      Blit(parent,mouseIsOver,x+width-2,y,x+width,y+height,cx-2,0);
      parent->SetClipRect(x+2,y+2,x+width-2,y+height-2);
      if ( gfont )
         gfont->DrawString(parent,
                           buffer.str,
                           x+xmargin,
                           y+ymargin,
                           gcolor);
      else
         parent->DrawString(buffer.str,
                            x+xmargin,
                            y+ymargin,
                            color);
      parent->ResetClipRect();
      }
   return 0;
   }

