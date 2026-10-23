/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
#include "sf.h"
#include "SF_PortCpp.h"
#include "SF_Externals.h"
#include "SF_Globals.h"
#include "SF_ClientWindow.h"
#include "SF_Client.h"
#include "SF_ChatConsole.h"
#include "SF_Sprite.h"
#include "SF_Engine.h"
#include "SF_GameEngine.h"
#include "SF_NetEngine.h"
#include "SF_DisplayEngine.h"
#include "SF_Button.h"
#include "SF_Slot.h"
#include "SF_Profile.h"
#include "SF_MsgBuffer.h"
#include "SF_Player.h"

SF_ChatChannel::SF_ChatChannel()
{
Init();
}

int
SF_ChatChannel::Init()
{
   scrollBegin = 0;
   maxScrollBegin = 0;
   maxDisplayLines = 0;
   numLines = 0;
   rgbColor = NULL;
   color = NULL;
   text = NULL;
   console = NULL;
   return 0;
}


int
SF_ChatChannel::Destroy()
{
  if ( text )
    {
      for ( int i=0; i<numLines; i++ )
	{
	  if ( text[i] )
	    {
	      S_free(text[i]);
	      text[i] = NULL;
            }
	}
      S_free(text);
      text = NULL;
    }
  if ( color )
    {
      for ( int i=0; i<numLines; i++ )
	{
	  if ( color[i] )
	    {
	      S_free(color[i]);
	      color[i] = NULL;
	    }
	}
      S_free(color);
      color = NULL;
    }
  if ( rgbColor )
    {
      S_free(rgbColor);
      rgbColor = NULL;
    }
  Init();
  return 0;
}


SF_ChatChannel::~SF_ChatChannel()
{
  Destroy();
}


SF_ChatConsole::SF_ChatConsole(S_CanvasBuffer *iparentBuffer,
                               int ix0, int iy0) {
   parentBuffer = iparentBuffer;
   bmpBack = NULL;
   bmpScroll = NULL;
   x0 = ix0;
   y0 = iy0;

   zoom = 4;
   visible = TRUE;
   redrawAll = TRUE;
   devmode = FALSE;

   font = new S_Font(S_FONT_HELV,8);

   textStart = 0;
   textEntry[0] = 0;

   buffer = new S_CanvasBuffer(sf->cxVideoConsole,
                               parentBuffer->cyCanvas-y0-sf->margin,
                               _root->bitsPerPixel);
   buffer->SetFont(font);
   parentBuffer->Embed(buffer,x0,y0);

   dragging = FALSE;
   beginDragY = 0;
   beginDragYScroll = 0;

   for ( int i=0; i<SF_CHATCONSOLE_NUMCHANNELS; i++ )
     {
       channels[i].maxDisplayLines = 
	 (buffer->cyCanvas-20)/buffer->lMaxBaselineExt - 2;
       channels[i].console = this;
     }

   insertText = FALSE;
   runIRC = FALSE;
   channel = &channels[SF_CHANNEL_BROADCAST];

   }



int
SF_ChatConsole::ConnectIRC()
{
  if ( !runIRC )
    {
      runIRC = TRUE;
      ircThread.Start(SF_ChatConsole_IRCThread,(void *)this);
    }
  return 0;
}


int
SF_ChatConsole::DisconnectIRC()
{
  runIRC = FALSE;
  ircThread.WaitFor();
  return 0;
}


int
SF_ChatConsole::SetBmpBack(SF_Sprite *ibmpBack) {
   if ( ibmpBack ) {
      bmpBack = ibmpBack;
      }
   return 0;
   }


int
SF_ChatConsole::SetBmpScroll(SF_Sprite *ibmpScroll) {
   if ( ibmpScroll ) {
      bmpScroll = ibmpScroll;
      }
   return 0;
   }


int
SF_ChatConsole::HandleIRCCommands(char *line)
{

  int rc = FALSE;

  if ( line )
    {

      //
      // Filter out IRC commands.
      //
      if ( line[0] == '/' )
	{

	  S_TokenArray tokens(line);

	  if ( tokens.numLines > 0 )
	    {

	      char *cmd = &tokens.lines[0][1];

	      if ( !strcmp(cmd,"me") )
		{
		  rc = FALSE;
		  char *tmp = S_strdup(line+3);
		  if ( tmp )
		    {
		      sprintf(line,"%cACTION%s%c",1,tmp,1);
		      S_free(tmp);
		    }
		}
	      else 
		{
		  rc = TRUE;
		  sprintf(line,"Unknown command %s",cmd);
		  channel->AppendLine(line,RGB_RED);
		}

	    }

	  return rc;
	}
    }
  return rc;
}



int
SF_ChatChannel::AppendLine(char *line,ulong rgbColor,ushort *icolor)
{
  if ( !console )
    {
      return 0;
    }

  if ( !line || !console->buffer->cxCanvas )
    {
      return 0;
    }

  //ushort *color = icolor;

  //
  //
  //
  int maxWidth = console->buffer->cxCanvas-30;
  int maxChars = maxWidth/console->buffer->lAveCharWidth;
  SizeStruct size;
  int i = 0;
  int j = 0;
  int len = 0;
  char str[2048];
  strncpy(str,line,2047);
  char *tstr = str;
  ushort *tcolor = icolor;
  do
    {
      char tmp[2048];
      strncpy(tmp,tstr,2047);
      len = strlen(tstr);
      size = console->buffer->GetStringSize(tmp);
      if ( size.cx > maxWidth )
	{
	  i = maxChars;
	  do {
	    i--;
	    if ( i >= 0 )
	      {
		if ( tmp[i] == ' ' )
		  {
		    tmp[i] = 0;
		  }
		size = console->buffer->GetStringSize(tmp);
	      }
	  } while ( i>0 && (int)size.cx > maxWidth );
	  
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
	  if ( i > 0 && i < maxChars )
	    {
	      j = i;
	    }
	}

      AddLine(tmp,rgbColor,tcolor);
      
      size = console->buffer->GetStringSize(tstr);
      
      if ( size.cx > maxWidth )
	{
	  if ( tcolor )
	    {
	      tcolor += j+1;
	    }
	  tstr += j+1;
	  if ( tcolor && tcolor < icolor )
	    {
	      tcolor = icolor;
	    }
	  if ( tstr < str )
	    {
	      tstr = str;
	    }
	  if ( tcolor && tcolor > icolor + strlen(str) )
	    {
	      tcolor = icolor + strlen(str);
	    }
	  if ( tstr > str + strlen(str) )
	    {
	      tstr = str + strlen(str);
	    }
	}
    } while ( size.cx > maxWidth );

  if ( this != console->channel )
    {
      if ( sf && sf->engine && sf->engine->displayEngine )
	{
	  int index = 12 + (this - console->channels);
	  sf->engine->displayEngine->button[index]->mode = SF_BUTTON_FLASH;
	  sf->engine->displayEngine->redrawButtons = TRUE;
	}
    }

  return 0;
}



int
SF_ChatChannel::AddLine(char *line,ulong irgbColor,ushort *icolor)
 {

   if ( !color ) {
      color = (ushort **)S_malloc(sizeof(ushort *)*(numLines+1));
      }
   else {
      color = (ushort **)S_realloc(color,sizeof(ushort *)*(numLines+1));
      }

   if ( !rgbColor ) {
      rgbColor = (ulong *)S_malloc(sizeof(slong)*(numLines+1));
      }
   else {
      rgbColor = (ulong *)S_realloc(rgbColor,sizeof(slong)*(numLines+1));
      }

   if ( !text ) {
      text  = (char **)S_malloc(sizeof(char *)*(numLines+1));
      }
   else {
      text  = (char **)S_realloc(text,sizeof(char *)*(numLines+1));
      }

   if ( !color || !text || !line || !rgbColor )
     {
       return 0;
     }

   FilterString(line);
   text[numLines] = S_strdup(line);

   int len = strlen(line);

   color[numLines] = NULL;
   if ( icolor )
     {
       color[numLines] = (ushort *)malloc(sizeof(ushort)*len);
       for ( int i=0; i<len; i++ )
	 {
	   color[numLines][i] = icolor[i];
	 }
     }

   rgbColor[numLines] = irgbColor;

   numLines++;

   console->redrawAll = TRUE;
   return 0;
   }



int
SF_ChatConsole::DrawIndicatorRect() {
   if ( sf->tabToggle || Contains(sf->mainWindow->mx,sf->mainWindow->my) ) {
      buffer->DrawRect(10,10,buffer->cxCanvas-11,buffer->cyCanvas-11,
                       RGB_GREEN);
      parentBuffer->DrawRect(x0+10,y0+10,
                             x0+buffer->cxCanvas-11,
                             y0+buffer->cyCanvas-11,
                             RGB_GREEN);
      sf->mainWindow->DrawRect(x0+10,y0+10-sf->dy1,
                               x0+buffer->cxCanvas-11,
                               y0+buffer->cyCanvas-11-sf->dy1,
                               RGB_GREEN);
      }
   else {
      buffer->DrawRect(10,10,buffer->cxCanvas-11,buffer->cyCanvas-11,
                       RGB_BLACK);
      parentBuffer->DrawRect(x0+10,y0+10,
                             x0+buffer->cxCanvas-11,
                             y0+buffer->cyCanvas-11,
                             RGB_BLACK);
      sf->mainWindow->DrawRect(x0+10,y0+10-sf->dy1,
                               x0+buffer->cxCanvas-11,
                               y0+buffer->cyCanvas-11-sf->dy1,
                               RGB_BLACK);
      }
   return 0;
   }



int
SF_ChatConsole::Draw() {

  if ( buffer->cxCanvas == 0 )
    {
      return 0;
    }

   if ( visible ) {
      if ( redrawAll ) {
	 buffer->ResetClipRect();
         if ( bmpBack ) {
            bmpBack->RequestDownload();
            buffer->BlitInRects(bmpBack,S_CANVASBUFFER_BLIT_ALL);
            ((S_SystemBitmap *)bmpBack)->SystemBlit(buffer,
                                                    0,0,buffer->cxCanvas,10,
                                                    0,bmpBack->cy-10);
	    }

         buffer->SetClipRect(10,10,buffer->cxCanvas-10,buffer->cyCanvas-10);

         int x = 15;
         int y = 10;
         for ( int i = max(0,channel->numLines-channel->maxDisplayLines-
			   channel->scrollBegin); 
	       i<channel->numLines-channel->scrollBegin; 
	       i++ )
	   {
	     y += buffer->lMaxBaselineExt;
	     DrawLine(channel->text[i],
		      x,
		      y,
		      channel->rgbColor[i],
		      channel->color[i]);
	   }

         textStart = 0;
         int maxWidth = buffer->cxCanvas-45;
         while ( buffer->GetStringSize(textEntry+textStart).cx > maxWidth )
         textStart += 15;

         x = 15;
         y = buffer->cyCanvas - 15;
         char tmp[1024];
         sprintf(tmp,"%s_",textEntry+textStart);
         buffer->DrawString(tmp,x,y,RGB_WHITE,
                            S_CANVASBUFFER_ADDRECTANGLE);

         /* *********************** */
         /* *** Draw Scroll bar *** */
         /* *********************** */
         if ( bmpScroll ) {

            /* *** Scroll Thumb *** */
            slong height = buffer->cyCanvas-60-16;
            double fraction = 0.0;
            if ( channel->numLines-channel->maxDisplayLines != 0 )
               fraction = (double)channel->scrollBegin /
		 (channel->numLines-channel->maxDisplayLines);
            if ( fraction < 0.0 )
               fraction = 0.0;

            xUpButton = buffer->cxCanvas-30;
            yUpButton = 30-16;
            xDownButton = buffer->cxCanvas-30;
            yDownButton = buffer->cyCanvas-30;
            xThumbButton = buffer->cxCanvas-30;
            yThumbButton = buffer->cyCanvas-30-16-(int)(height*fraction);

            /* *** Arrow Buttons *** */
            bmpScroll->Blit(buffer,0,xDownButton,yDownButton);
            bmpScroll->Blit(buffer,2,xUpButton,yUpButton);
            bmpScroll->Blit(buffer,1,xThumbButton,yThumbButton);

	    }
         /* *********************** */


         buffer->Show(visible);
         redrawAll = FALSE;
         }

      }
   else {
      redrawAll = FALSE;
      }

   DrawIndicatorRect();

   buffer->ResetRects();

   if ( redrawAll ) {
      buffer->Show(visible);
      }

   return 0;
   }


int 
SF_ChatConsole::ProcessColorCode(char *itext,
				 int &fgIndex,
				 int &bgIndex, 
				 slong &bgColor)
{

  int ret = 0;
  char * text = itext;

  if ( text[0] != 3 )
    {
      return ret;
    }

  ret++;
  text++;

  int i = 0;
  char snum[128];
  for ( i=0; i<128; i++ )
    {
      snum[i] = 0;
    }

  i = 0;
  while ( *text >= 48 && *text <= 57 )
    {
      snum[i++] = *text;
      ret++;
      text++;
    }

  fgIndex = -1;

  if ( snum[0] == 0 )
    {
      // Blank color command
      return ret;
    }

  fgIndex = atol(snum);

  if ( *text != ',' )
    {
      // Forground color only
      return ret;
    }

  text++;
  ret++;

  for ( i=0; i<128; i++ )
    {
      snum[i] = 0;
    }

  i = 0;
  while ( *text >= 48 && *text <= 57 )
    {
      snum[i++] = *text;
      ret++;
      text++;
    }

  bgIndex = -1;

  if ( snum[0] == 0 )
    {
      // Blank bg color command
      return ret;
    }

  bgIndex = atol(snum);
  
  return ret;
}




char *
SF_ChatConsole::DrawLine(char *itext,int ix,int iy,ulong irgbColor,
			 ushort *icolor)
{

  int x = ix;
  int y = iy;
  slong rgbColor = irgbColor;
  char *text = itext;  
  ushort *color = icolor;

  if ( !color )
    {
      buffer->DrawString(text,x,y,rgbColor,S_CANVASBUFFER_ADDRECTANGLE);
      return itext;
    }

  slong fgColor = irgbColor;
  slong bgColor = -1;

  int fgIndex = -1;
  int bgIndex = -1;

  int endOfString = FALSE;

  buffer->SetBgColor(-1);

  do
    {

      fgIndex = (schar)(*color & 0x00FF);
      bgIndex = (schar)((*color & 0xFF00) >> 8);
      
      switch ( fgIndex % 16 )
	{
	case 0  : fgColor = RGB_WHITE;      break;
	case 1  : fgColor = RGB_BLACK;      break;
	case 2  : fgColor = RGB_DARKBLUE;   break;
	case 3  : fgColor = RGB_DARKGREEN;  break;
	case 4  : fgColor = RGB_RED;        break;
	case 5  : fgColor = RGB_DARKRED;    break;
	case 6  : fgColor = RGB_PURPLE;     break;
	case 7  : fgColor = RGB_ORANGE;     break;
	case 8  : fgColor = RGB_YELLOW;     break;
	case 9  : fgColor = RGB_GREEN;      break;
	case 10 : fgColor = RGB_DARKCYAN;   break;
	case 11 : fgColor = RGB_CYAN;       break;
	case 12 : fgColor = RGB_BLUE;       break;
	case 13 : fgColor = RGB_PINK;       break;
	case 14 : fgColor = RGB_DARKGRAY;   break;
	case 15 : fgColor = RGB_PALEGRAY;   break;
	case 16 : fgColor = RGB_WHITE;      break;
	default : fgColor = irgbColor;
	}
      
      bgColor = -1;
      if ( bgIndex >= 0 )
	{
	  switch ( bgIndex % 16 )
	    {
	    case 0  : bgColor = RGB_WHITE;      break;
	    case 1  : bgColor = RGB_BLACK;      break;
	    case 2  : bgColor = RGB_DARKBLUE;   break;
	    case 3  : bgColor = RGB_DARKGREEN;  break;
	    case 4  : bgColor = RGB_RED;        break;
	    case 5  : bgColor = RGB_DARKRED;    break;
	    case 6  : bgColor = RGB_PURPLE;     break;
	    case 7  : bgColor = RGB_ORANGE;     break;
	    case 8  : bgColor = RGB_YELLOW;     break;
	    case 9  : bgColor = RGB_GREEN;      break;
	    case 10 : bgColor = RGB_DARKCYAN;   break;
	    case 11 : bgColor = RGB_CYAN;       break;
	    case 12 : bgColor = RGB_BLUE;       break;
	    case 13 : bgColor = RGB_PINK;       break;
	    case 14 : bgColor = RGB_DARKGRAY;   break;
	    case 15 : bgColor = RGB_PALEGRAY;   break;
	    case 16 : bgColor = RGB_WHITE;      break;
	    }
	}

      char *nextText = text;
      ushort *nextColor = color;
      int found = FALSE;
      do
	{
	  if( *nextText == 0 )
	    {
	      found = TRUE;
	    }
	  if ( !found )
	    {
	      int nextFgIndex = (schar)(*nextColor & 0x00FF);
	      int nextBgIndex = (schar)((*nextColor & 0xFF00) >> 8);
	      found = 
		nextFgIndex != fgIndex || 
		nextBgIndex != bgIndex;
	    }
	  if ( !found )
	    {
	      nextText++;
	      nextColor++;
	    }
	} while ( !found );
      
      buffer->SetBgColor(bgColor);
      
      char ch = *nextText;
      *nextText = 0;
            
      SizeStruct size;
      size = buffer->DrawString(text,x,y,fgColor,S_CANVASBUFFER_ADDRECTANGLE);

      *nextText = ch;
      
      x += size.cx;
      
      text = nextText;
      color = nextColor;
      
      endOfString = *nextText == '\0';
      
    } while ( !endOfString );

  buffer->SetBgColor(-1);
  

  return itext;
}




uchar keymap[64] = {
  0,  0,'1','2','3','4','5','6','7','8','9','0','-','=',  0,  0,
'q','w','e','r','t','y','u','i','o','p','[',']',  0,  0,'a','s',
'd','f','g','h','j','k','l',';','\'',  0,  0,'\\','z','x','c','v',
'b','n','m',',','.','/',  0,  0,   0,' ',  0,   0,  0,  0,  0,  0 };

uchar skeymap[64] = {
  0,  0,'!','@','#','$','%','^','&','*','(',')','_','+',  0,  0,
'Q','W','E','R','T','Y','U','I','O','P','{','}',  0,  0,'A','S',
'D','F','G','H','J','K','L',':','"',  0,  0, '|','Z','X','C','V',
'B','N','M','<','>','?',  0,  0,  0,' ',  0,   0,  0,  0,  0,  0 };

uchar ckeymap[64] = {
  0,  0,'!','@','#','$','%','^','&','*','(',')','_','+',  0,  0,
'Q','W','E', 22,'T','Y', 31,  9, 15,'P','{','}',  0,  0,'A','S',
'D','F','G','H','J',  3,'L',':','"',  0,  0, '|','Z','X','C','V',
  2,'N','M','<','>','?',  0,  0,  0,' ',  0,   0,  0,  0,  0,  0 };


int
SF_ChatConsole::KeyPressed(uchar scancode,uchar shift,int keyup,uchar ctrl)
{
  if ( scancode == SF_KEY_ENTER )
    {
      if ( !keyup )
	{
	  if ( !HandleDevCommands(textEntry) )
	    {
	      SendChatMsg(textEntry);
	    }
	  if ( sf->tabToggle &&
	       sf->engine->displayEngine->button[14]->mode == SF_BUTTON_UP )
	    {
	      sf->tabToggle = FALSE;
	    }
	}
      return TRUE;
    }
  if ( scancode == SF_KEY_BACKSPACE )
    {
      if ( !keyup )
	{
	  if ( strlen(textEntry) > 0 )
	    {
	      textEntry[strlen(textEntry)-1] = 0;
	    }
	  redrawAll = TRUE;
	}
      return TRUE;
    }
  if ( scancode < 64 )
    {
      if ( shift )
	{
	  if ( !keyup )
	    {
	      sprintf(textEntry,"%s%c",textEntry,skeymap[scancode]);
	      redrawAll = TRUE;
	    }
	  return skeymap[scancode] != 0;
	}
      else if ( ctrl )
	{
	  if ( !keyup )
	    {
	      sprintf(textEntry,"%s%c",textEntry,ckeymap[scancode]);
	      redrawAll = TRUE;
	    }
	  return ckeymap[scancode] != 0;
	}
      else {
	if ( !keyup )
	  {
	    sprintf(textEntry,"%s%c",textEntry,keymap[scancode]);
	    redrawAll = TRUE;
	  }
	return keymap[scancode] != 0;
      }
    }
  return FALSE;
}


int
SF_ChatConsole::Contains(int x,int y) {
   return x>x0 && x<x0+buffer->cxCanvas && y>y0 && y<y0+buffer->cyCanvas;
   }


int
SF_ChatConsole::UpButtonContains(int x,int y) {
   return (x-x0)>xUpButton && (x-x0)<xUpButton+16 && 
          (y-y0)>yUpButton && (y-y0)<yUpButton+16;
   }


int
SF_ChatConsole::DownButtonContains(int x,int y) {
   return (x-x0)>xDownButton && (x-x0)<xDownButton+16 && 
          (y-y0)>yDownButton && (y-y0)<yDownButton+16;
   }


int
SF_ChatConsole::ThumbButtonContains(int x,int y) {
   return (x-x0)>xThumbButton && (x-x0)<xThumbButton+16 && 
          (y-y0)>yThumbButton && (y-y0)<yThumbButton+16;
   }



int
SF_ChatConsole::HandleMouseClick(int button,int x,int y,int keyup) {

   channel->maxScrollBegin = channel->numLines - channel->maxDisplayLines;
   if ( channel->maxScrollBegin < 0 )
      channel->maxScrollBegin = 0;

   if ( button == 1 && !keyup ) {
      if ( UpButtonContains(x,y) ) {
         channel->scrollBegin++;
         if ( channel->scrollBegin > channel->maxScrollBegin )
            channel->scrollBegin = channel->maxScrollBegin;
         }
      if ( DownButtonContains(x,y) ) {
         channel->scrollBegin--;
         if ( channel->scrollBegin < 0 )
            channel->scrollBegin = 0;
         }
      if ( ThumbButtonContains(x,y) ) {
         dragging = TRUE;
         beginDragY = y;
         beginDragYScroll = yThumbButton;
         }
      redrawAll = TRUE;
      }

   if ( button == 1 && keyup && dragging ) {
      dragging = FALSE;
      }

   if ( button == 0 && dragging ) {
      yThumbButton = (y - beginDragY) + beginDragYScroll;

      /* *** Scroll Thumb *** */
      slong height = buffer->cyCanvas-60-16;
      double fraction = 0.0;
      fraction = (double)(yDownButton - yThumbButton)/height;

      channel->scrollBegin = 
	(int)(fraction*(channel->numLines-channel->maxDisplayLines));
      if ( channel->scrollBegin < 0 )
         channel->scrollBegin = 0;

      if ( channel->scrollBegin > channel->maxScrollBegin )
         channel->scrollBegin = channel->maxScrollBegin;

      redrawAll = TRUE;
      }

   return 0;
   }



int
SF_ChatConsole::SendChatMsg(char *msg) {
   SF_Client *c = (SF_Client *) sf;
   SF_Engine *e = c->engine;
 
   if ( !e )
     {
       return 0;
     }
   
   SF_NetEngine *ne = e->netEngine;
   SF_MsgBuffer *wb = NULL;
   if ( ne )
     {
       wb = &ne->slot->writeBuffer;
     }
   
   SF_GameEngine *ge = c->engine->gameEngine;
   SF_MsgBuffer *gb = NULL;
   if ( ge )
     {
       gb = ge->msgBuffer;
     }
   
   if ( c->profile->selectedPlayer < 0 )
     {
       return 0;
     }
   
   char *name=c->profile->players[c->profile->selectedPlayer]->playerName.str;
   
   //
   // Strip out newline type characters.
   //
   for ( int i=0; i<(int)strlen(msg); i++ )
     {
       if ( msg[i] == '\n' || msg[i] == '\r' )
	 {
	   msg[i] = 0;
	 }
     }

     //
     // Send IRC msg if IRC button is selected and msg exists.
     //
     if ( msg[0] > 0 &&
	  sf->engine->displayEngine->button[14]->mode == SF_BUTTON_DOWN )
       {
	 if ( !HandleIRCCommands(msg) )
	   {
	     ircClient.SendMsg("#sf",msg);
	     ushort color = ircClient.ParsePrivMsg(ircInsertMsg.text,msg,name);
	     ircClient.SetMsgColor(&ircInsertMsg,color);
	     ircClient.ParseFormats(&ircInsertMsg);

	     //
	     // Signal IRC Thread to insert msg
	     //
	     insertText = TRUE;
	     runIRC = FALSE;
	   }
	 msg[0] = 0;
	 return 0;
       }

   if ( msg[0] > 0 ) {

      char buf[1024];
      sprintf(buf,"%s> %s",name,msg);

      if ( c->engine->state == SF_CLIENT_CONNECTED && wb ) {
         int mode = SF_MSG_CHAT_TOALL;
         int to = 0;
         if ( sf->engine->displayEngine->button[13]->mode == SF_BUTTON_DOWN ) {
            mode = SF_MSG_CHAT_TORACE;
            to = game->ship[ge->localSlot].raceID;
	    }
         wb->mutex.Request();
	 mstate = 60;
         wb->PushMsg((uchar)SF_MSG_PLAYERCHAT_N,
                     (ushort)ge->localSlot,
                     (uchar)mode,
                     (uchar)to,
                      buf,
                     (int)S_CLIENT_TCP);
	 mstate = 0;
         wb->mutex.Release();
	 SndCueWav(game->soundEvent[push_button1_event],
		   game->soundEvent[push_button1_event]);
	 msg[0] = 0;
         }
      else if ( gb ) {
         gb->mutex.Request();
	 mstate = 61;
         gb->PushMsg((uchar)SF_MSG_PLAYERCHAT_N,
                     (ushort)ge->localSlot,
                     (uchar)0,
                     (uchar)0,
                      buf,
                     (int)S_CLIENT_TCP);
	 mstate = 0;
         gb->mutex.Release();
	 SndCueWav(game->soundEvent[push_button1_event],
		   game->soundEvent[push_button1_event]);
	 msg[0] = 0;
         }
      }
   return 0;
   }



int
SF_ChatConsole::SetVisible(int ivisible) {
   visible = ivisible;
   redrawAll = TRUE;
   buffer->Show(visible);
   return 0;
   }

SF_ChatConsole::~SF_ChatConsole()
{
  if ( buffer ) {
    delete buffer;
    buffer = NULL;
  }
  if ( font )
    {
      delete font;
      font = NULL;
    }
}


int
SF_ChatConsole_IRCThread(void *iarg)
{
  SF_ChatConsole *console = (SF_ChatConsole *)iarg;
  S_IRCClient *client = &console->ircClient;

  do 
    {

      int disconnected = FALSE;
      int startOutput = FALSE;
      console->channel->
	AppendLine("Connecting to irc.stardock.com on port 667 . . .",
		   RGB_WHITE);
      if ( client->Connect("irc.stardock.com",667) == S_CLIENT_OK )
	{
	  int iplayer = sf->profile->selectedPlayer;
	  SF_Player *player = sf->profile->players[iplayer];
	  char *name = player->playerName.str;
	  char *password = player->password.str;
	  
	  client->SendPass(password);
	  client->SendNick(name);
	  client->SendUser(name);
	  
	  int motdRcvd = FALSE;
	  do 
	    {
	      S_IRCMsgStruct msg;
	      msg.text[0] = 0;
	      client->RecvNextLine(&msg,&console->runIRC);

	      SF_ChatChannel *channel = NULL;
	      channel = &console->channels[SF_CHANNEL_IRC];

	      if ( console->insertText )
		{
		  console->insertText = FALSE;
		  console->runIRC = TRUE;
		  channel->AppendLine(console->ircInsertMsg.text,
				      RGB_WHITE,
				      console->ircInsertMsg.color);
		}
	      
	      if ( msg.code == 376 )
		{
		  client->SendJoin("#sf");
		  motdRcvd = TRUE;
		}
	      
	      if ( msg.text[0] )
		{
		  channel->AppendLine(msg.text,msg.color[0],msg.color );
		}

	      if ( msg.code == 366 )
		{
		  startOutput = TRUE;
		}	      

	      disconnected = FALSE;

	    } while ( console->runIRC && !disconnected );

	  client->SendPart("#sf");
	  client->SendQuit("Bye all");
	  client->Disconnect();

	}

    } while ( console->runIRC );

  return 0;
}
