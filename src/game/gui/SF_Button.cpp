#include "sf.h"
#include "SF_Button.h"
#include "SF_PortCpp.h"
#include "SF_Engine.h"
#include "SF_GameEngine.h"
#include "SF_DisplayEngine.h"
#include "SF_Globals.h"
#include "SF_ClientWindow.h"
#include "SF_Client.h"
#include "SF_Externals.h"
#include "SF_MsgBuffer.h"
#include "SF_ChatConsole.h"

SF_Button::SF_Button(SF_DisplayEngine *ide,char *itext,
                     int ix1,int iy1,int ix2,int iy2,
                     ulong itextColor,ulong ilightColor,
                     ulong idarkColor,ulong ifillColor,
                     uchar ich0 ,uchar ich1 ,uchar ich2 ,uchar ich3,uchar ich4,
                     uchar imsg0,uchar imsg1,uchar imsg2,uchar imsg3,uchar imsg4) {
  visible = FALSE;
   mode = SF_BUTTON_UP;
   text = NULL;
   de = ide;
   x1 = ix1;
   y1 = iy1;
   x2 = ix2;
   y2 = iy2;
   textColor = itextColor;
   lightColor = ilightColor;
   darkColor = idarkColor;
   fillColor = ifillColor;
   SetProperties(itext,ich0,ich1,ich2,ich3,ich4,imsg0,imsg1,imsg2,imsg3,imsg4);
   }


int
SF_Button::SetProperties(char *itext,
                 uchar ich0 ,uchar ich1 ,uchar ich2 ,uchar ich3 ,uchar ich4,
                 uchar imsg0,uchar imsg1,uchar imsg2,uchar imsg3,uchar imsg4) {
   int redraw = FALSE;
   if ( text && itext && text[0] && itext[0] && strcmp(text,itext) )
      redraw = TRUE;
   if ( text ) {
      S_free(text);
      text = NULL;
      }
   if ( itext )
      text = S_strdup(itext);
   ch[0] = ich0;
   ch[1] = ich1;
   ch[2] = ich2;
   ch[3] = ich3;
   ch[4] = ich4;
   msg[0] = imsg0;
   msg[1] = imsg1;
   msg[2] = imsg2;
   msg[3] = imsg3;
   msg[4] = imsg4;
   if ( redraw )
      Draw();
   return 0;
   }


int
SF_Button::Contains(int x,int y) {
   return x>x1 && x<x2 && y>y1 && y<y2;
   }



int
SF_Button::HandleClick(int imode) {
   mode = imode;
   for ( int i=0; i<5; i++ )
      if ( msg[i] > 0 && ch[i] > 0 ) {
         if ( ch[i] == SF_KEY_LSHIFT || ch[i] == SF_KEY_RSHIFT )
            sf->shiftDown = (msg[i] == SF_MSG_KEYPRESSED_4);
         if ( ch[i] == SF_KEY_LCTRL || ch[i] == SF_KEY_RCTRL )
            sf->ctrlDown = (msg[i] == SF_MSG_KEYPRESSED_4);
         if ( !de->ge->FilterKeystroke(msg[i],ch[i]) ) {
            sf->engine->PushKeyboardMsg(msg[i],ch[i]);
	    }
         }
   return 0;
   }


int
SF_Button::Draw(int imode) {

  if ( !visible )
    {
      return 0;
    }

   if ( imode >= 0 )
      mode = imode;

   if ( de && de->bb ) {

      if ( fillColor != 0xFFFFFFFF )
         de->bb->DrawFilledRect(x1,y1,x2,y2,fillColor);
      else if ( de->ge->bgSprite )
         ((S_SystemBitmap *)de->ge->bgSprite)->
            SystemBlit(de->bb,x1,y1,x2,y2,x1,y1);
      if ( mode == SF_BUTTON_UP || mode == SF_BUTTON_FLASH) {
         de->bb->DrawLine(x1,y1,x2,y1,1,lightColor);
         de->bb->DrawLine(x1,y1,x1,y2,1,lightColor);
         de->bb->DrawLine(x1,y2,x2,y2,1,darkColor);
         de->bb->DrawLine(x2,y1,x2,y2,1,darkColor);
         }
      else {
         de->bb->DrawLine(x1,y1,x2,y1,1,darkColor);
         de->bb->DrawLine(x1,y1,x1,y2,1,darkColor);
         de->bb->DrawLine(x1,y2,x2,y2,1,lightColor);
         de->bb->DrawLine(x2,y1,x2,y2,1,lightColor);
         }
      if ( text ) {
         SizeStruct size = de->bb->GetStringSize(text);
         if ( mode == SF_BUTTON_UP )
            de->bb->DrawString( text,
                               (x1+x2)/2-size.cx/2+mode+1,
                               (y1+y2)/2+de->bb->lMaxAscender/2+mode-1,
                                textColor);
         else if ( mode == SF_BUTTON_DOWN )
            de->bb->DrawString( text,
                               (x1+x2)/2-size.cx/2+mode+1,
                               (y1+y2)/2+de->bb->lMaxAscender/2+mode-1,
                                RGB_YELLOW);
	 else
            de->bb->DrawString( text,
                               (x1+x2)/2-size.cx/2+mode+1,
                               (y1+y2)/2+de->bb->lMaxAscender/2+mode-1,
                                RGB_RED);
         }
      de->bb->SystemBlit(de->window,x1,y1-sf->dy1,x2+1,y2-sf->dy1+1,x1,y1);
      }
   return 0;
   }

SF_Button::~SF_Button() {
   if ( text ) {
      S_free(text);
      text = NULL;
      }
   }
