/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
#include "sf.h"
#include "SF_PortCpp.h"
#include "SF_Externals.h"
#include "SF_DisplayEngine.h"
#include "SF_GameEngine.h"
#include "SF_NetEngine.h"
#include "SF_Profile.h"
#include "SF_WaveList.h"
#include "SF_SpriteList.h"
#include "SF_Engine.h"
#include "SF_Globals.h"
#include "SF_ClientWindow.h"
#include "SF_Client.h"
#include "SF_VideoConsole.h"
#include "SF_MapConsole.h"
#include "SF_ChatConsole.h"
#include "SF_TextBuffer.h"
#include "SF_Sprite.h"
#include "SF_Slot.h"
#include "SF_Button.h"
#include "SF_Font.h"



SF_DisplayEngine::SF_DisplayEngine(SF_Engine *iengine) {
   e = iengine;
   window = e->window;
   ge = e->gameEngine;
   ne = e->netEngine;
   ge->de = this;

   check = 0xAAAAAAAA;
   repaint = TRUE;
   redrawButtons = TRUE;
   displaySkip = 1;
   showDebugString = FALSE;
   firstTime = TRUE;
   waitForLogin = TRUE;

   show = FALSE;

   tacticalZoom = 1;
   tacticalScale = 1;
   strategicZoom = 16;
   strategicScale = 16;

   soundIndex = -1;
   numObjectsOnScreen = 0;
   numEnemyShipsOnScreen = 0;
   numEnemyShipsNearby = 0;
   condition = RGB_DARKGREEN;

   target = NULL;
   targetPower = 0;
   targetShield = 0;
   targetEngine = 0;
   targetScore = 0;
   targetKills = 0;
   targetCrew = 0;
   targetSpeed = 0;

   localPower = 0;
   localShield = 0;
   localEngine = 0;
   localScore = 0;
   localKills = 0;
   localCrew = 0;
   localSpeed = 0;

   localPlayerBitmapIndex = -1;

   c = (SF_Client *)window->puser;
   bb = c->backBuffer;

   console = NULL;
   video = NULL;
   map = NULL;
   chat = NULL;
   mainFont = NULL;

   cyContextButtons = 15;

   video = console = new SF_VideoConsole(bb,ge,
                                         bb->cxCanvas-10-sf->cxVideoConsole,
                                         sf->cyConsoleButtons,
                                         MESSAGE_STRING_LENGTH,
					 SF_VIDEOCONSOLE_NUMROWS);

   map = new SF_MapConsole(bb,
                           bb->cxCanvas-10-sf->cxMapConsole,
                           sf->cyConsoleButtons + sf->margin +
                           cyContextButtons + 5 + 
                           video->buffer->cyCanvas);

   chat = new SF_ChatConsole(bb,
                             bb->cxCanvas - 10 - sf->cxMapConsole,
                             sf->cyConsoleButtons + 2*sf->margin + 
                             cyContextButtons + 5 +
                             video->buffer->cyCanvas + 
                             map->buffer->cyCanvas + 15);

   if ( console )
      console->SetMode(VIDEO_HELP);

   halftoneBuffer = new S_CanvasBuffer(sf->cxHalftone,
                                       sf->cyHalftone,
                                       window->root->bitsPerPixel);

   scrTopMsgBuffer = new SF_TextBuffer(10,10 + bb->lMaxAscender,10);

   scrBotMsgBuffer = new SF_TextBuffer(37,bb->cyCanvas - sf->cyPanel);

   /* ********* Initialize the debris data ********* */
   numDebris = 20;
   debrisDist = (int *) S_malloc( numDebris*sizeof(int) );
   xDebris = (slong *) S_malloc( numDebris*sizeof(slong) );
   yDebris = (slong *) S_malloc( numDebris*sizeof(slong) );

   int i;
   for ( i=0; i<numDebris; i++ )
      debrisDist[i] = (rand() % 3)*3 + 1;

   for ( i=0; i<MAXOBJECTS; i++ )
     {
       onScreen[i] = FALSE;
     }

   for ( i=0; i<SF_DISPLAYENGINE_NUMBUTTONS; i++ )
      button[i] = 0;
   CreateButtons();

   mainFont = new SF_Font(7,10,NULL);
   }

SF_DisplayEngine::~SF_DisplayEngine() {
   if ( mainFont ) {
      delete mainFont;
      mainFont = NULL;
      }
   DestroyButtons();
   if ( debrisDist ) {
      S_free(debrisDist);
      debrisDist = NULL;
      }
   if ( xDebris ) {
      S_free(xDebris);
      xDebris = NULL;
      }
   if ( yDebris ) {
      S_free(yDebris);
      yDebris = NULL;
      }
   if ( scrTopMsgBuffer ) {
      delete scrTopMsgBuffer;
      scrTopMsgBuffer = NULL;
      }
   if ( scrBotMsgBuffer ) {
      delete scrBotMsgBuffer;
      scrBotMsgBuffer = NULL;
      }
   bb->ResetRects();
   bb->numEmbeddedBuffers = 0;
   if ( halftoneBuffer ) {
      delete halftoneBuffer;
      halftoneBuffer = NULL;
      }
   if ( map ) {
      delete map;
      map = NULL;
      }
   if ( chat ) {
      delete chat;
      chat = NULL;
      }
   if ( console ) {
      delete console;
      console = NULL;
      }
   }


int
SF_DisplayEngine::ClearBackBuffer(int flag) {
   if ( bb && ge->bgSprite ) {
     if ( ge->bgSprite->rc ) {
         if ( ge->bg2Sprite && ge->bg2Sprite->rc )
            ge->bg2Sprite->RequestDownload();
         ge->bgSprite->RequestDownload();
         bb->FillInRects(0,flag);
         }
      else
         bb->BlitInRects(ge->bgSprite,flag);
      }
   return 0;
   }


int
SF_DisplayEngine::CreateButtons() {

   DestroyButtons();

   int cxButton = 45;
   int cyButton = 15;
   int x0 = bb->cxCanvas-308;
   int y0 = sf->cyConsoleButtons-2-cyButton;

   button[0] = new SF_Button(this,"Navigate",x0,y0,x0+cxButton,y0+cyButton,
                             RGB_GREEN,RGB_DARKGRAY,RGB_BLACK,0xFFFFFFFF,
                             SF_KEY_N,0,0,0,0,
                             SF_MSG_KEYPRESSED_4,0,0,0,0);
   button[6] = new SF_Button(this,"Census",x0,y0-sf->margin-cyButton,
                             x0+cxButton,y0-sf->margin,
                             RGB_GREEN,RGB_DARKGRAY,RGB_BLACK,0xFFFFFFFF,
                             SF_KEY_R,0,0,0,0,
                             SF_MSG_KEYPRESSED_4,0,0,0,0);
   button[16] = new SF_Button(this,"",x0,
                              sf->cyConsoleButtons+console->cy,
                              x0+cxButton,
                              sf->cyConsoleButtons+console->cy+cyButton,
                              RGB_GREEN,RGB_DARKGRAY,RGB_BLACK,0xFFFFFFFF,
                              0,0,0,0,0,
                              0,0,0,0,0);

   x0 += cxButton + sf->margin;
   button[1] = new SF_Button(this,"Target",x0,y0,x0+cxButton,y0+cyButton,
                             RGB_GREEN,RGB_DARKGRAY,RGB_BLACK,0xFFFFFFFF,
                             SF_KEY_T,0,0,0,0,
                             SF_MSG_KEYPRESSED_4,0,0,0,0);
   button[7] = new SF_Button(this,"Ranks",x0,y0-sf->margin-cyButton,
                             x0+cxButton,y0-sf->margin,
                             RGB_GREEN,RGB_DARKGRAY,RGB_BLACK,0xFFFFFFFF,
                             SF_KEY_LSHIFT,SF_KEY_R,SF_KEY_LSHIFT,0,0,
                             SF_MSG_KEYPRESSED_4,SF_MSG_KEYPRESSED_4,
                             SF_MSG_KEYRELEASED_4,0,0);
   button[17] = new SF_Button(this,"",x0,
                              sf->cyConsoleButtons+console->cy,
                              x0+cxButton,
                              sf->cyConsoleButtons+console->cy+cyButton,
                              RGB_GREEN,RGB_DARKGRAY,RGB_BLACK,0xFFFFFFFF,
                              0,0,0,0,0,
                              0,0,0,0,0);

   x0 += cxButton + sf->margin;
   button[2] = new SF_Button(this,"Damage",x0,y0,x0+cxButton,y0+cyButton,
                             RGB_GREEN,RGB_DARKGRAY,RGB_BLACK,0xFFFFFFFF,
                             SF_KEY_S,0,0,0,0,
                             SF_MSG_KEYPRESSED_4,0,0,0,0);
   button[8] = new SF_Button(this,"Planets",x0,y0-sf->margin-cyButton,
                              x0+cxButton,y0-sf->margin,
                             RGB_GREEN,RGB_DARKGRAY,RGB_BLACK,0xFFFFFFFF,
                             SF_KEY_LSHIFT,SF_KEY_P,SF_KEY_LSHIFT,0,0,
                             SF_MSG_KEYPRESSED_4,SF_MSG_KEYPRESSED_4,
                             SF_MSG_KEYRELEASED_4,0,0);
   button[18] = new SF_Button(this,"",x0,
                              sf->cyConsoleButtons+console->cy,
                              x0+cxButton,
                              sf->cyConsoleButtons+console->cy+cyButton,
                              RGB_GREEN,RGB_DARKGRAY,RGB_BLACK,0xFFFFFFFF,
                              0,0,0,0,0,
                              0,0,0,0,0);

   x0 += cxButton + sf->margin;
   button[3] = new SF_Button(this,"Intel",x0,y0,x0+cxButton,y0+cyButton,
                             RGB_GREEN,RGB_DARKGRAY,RGB_BLACK,0xFFFFFFFF,
                             SF_KEY_M,0,0,0,0,
                             SF_MSG_KEYPRESSED_4,0,0,0,0);
   button[9] = new SF_Button(this,"Refit",x0,y0-sf->margin-cyButton,
                              x0+cxButton,y0-sf->margin,
                             RGB_GREEN,RGB_DARKGRAY,RGB_BLACK,0xFFFFFFFF,
                             SF_KEY_D,0,0,0,0,
                             SF_MSG_KEYPRESSED_4,0,0,0,0);
   button[19] = new SF_Button(this,"",x0,
                              sf->cyConsoleButtons+console->cy,
                              x0+cxButton,
                              sf->cyConsoleButtons+console->cy+cyButton,
                              RGB_GREEN,RGB_DARKGRAY,RGB_BLACK,0xFFFFFFFF,
                              0,0,0,0,0,
                              0,0,0,0,0);

   x0 += cxButton + sf->margin;
   button[4] = new SF_Button(this,"Pilots",x0,y0,x0+cxButton,y0+cyButton,
                             RGB_GREEN,RGB_DARKGRAY,RGB_BLACK,0xFFFFFFFF,
                             SF_KEY_P,0,0,0,0,
                             SF_MSG_KEYPRESSED_4,0,0,0,0);
   button[10] = new SF_Button(this,"Save",x0,y0-sf->margin-cyButton,
                             x0+cxButton,y0-sf->margin,
                             RGB_GREEN,RGB_DARKGRAY,RGB_BLACK,0xFFFFFFFF,
                             SF_KEY_F8,0,0,0,0,
                             SF_MSG_KEYPRESSED_4,0,0,0,0);
   button[20] = new SF_Button(this,"",x0,
                              sf->cyConsoleButtons+console->cy,
                              x0+cxButton,
                              sf->cyConsoleButtons+console->cy+cyButton,
                              RGB_GREEN,RGB_DARKGRAY,RGB_BLACK,0xFFFFFFFF,
                              0,0,0,0,0,
                              0,0,0,0,0);

   x0 += cxButton + sf->margin;
   button[5] = new SF_Button(this," ",x0,y0,x0+cxButton,y0+cyButton,
                             RGB_GREEN,RGB_DARKGRAY,RGB_BLACK,0xFFFFFFFF,
                             0,0,0,0,0,
                             0,0,0,0,0);
   button[11] = new SF_Button(this,"Help",x0,y0-sf->margin-cyButton,
                             x0+cxButton,y0-sf->margin,
                             RGB_GREEN,RGB_DARKGRAY,RGB_BLACK,0xFFFFFFFF,
                             SF_KEY_F1,0,0,0,0,
                             SF_MSG_KEYRELEASED_4,0,0,0,0);
   button[11]->mode = SF_BUTTON_DOWN;
   button[21] = new SF_Button(this,"",x0,
                              sf->cyConsoleButtons+console->cy,
                              x0+cxButton,
                              sf->cyConsoleButtons+console->cy+cyButton,
                              RGB_GREEN,RGB_DARKGRAY,RGB_BLACK,0xFFFFFFFF,
                              0,0,0,0,0,
                              0,0,0,0,0);

   x0 = bb->cxCanvas - 330;
   y0 = sf->cyConsoleButtons + 2*sf->margin + 
        video->buffer->cyCanvas + map->buffer->cyCanvas + cyButton +
        cyContextButtons + 5;
   button[12] = new SF_Button(this,"BCast",x0,y0-cyButton,x0+cxButton,y0,
                                 RGB_GREEN,RGB_DARKGRAY,RGB_BLACK,0xFFFFFFFF,
                                 SF_KEY_F2,0,0,0,0,
                                 SF_MSG_KEYPRESSED_4,0,0,0,0);
   x0 += cxButton + sf->margin;
   button[13] = new SF_Button(this,"Team",x0,y0-cyButton,x0+cxButton,y0,
                                 RGB_GREEN,RGB_DARKGRAY,RGB_BLACK,0xFFFFFFFF,
                                 SF_KEY_F3,0,0,0,0,
                                 SF_MSG_KEYPRESSED_4,0,0,0,0);
   x0 += cxButton + sf->margin;
   button[14] = new SF_Button(this,"IRC",x0,y0-cyButton,x0+cxButton,y0,
                                 RGB_GREEN,RGB_DARKGRAY,RGB_BLACK,0xFFFFFFFF,
                                 SF_KEY_F4,0,0,0,0,
                                 SF_MSG_KEYPRESSED_4,0,0,0,0);
   x0 = bb->cxCanvas - 10 - cxButton;
   button[15] = new SF_Button(this,"Lock",x0,y0-cyButton,x0+cxButton,y0,
                                 RGB_GREEN,RGB_DARKGRAY,RGB_BLACK,0xFFFFFFFF,
                                 SF_KEY_F5,0,0,0,0,
                                 SF_MSG_KEYPRESSED_4,0,0,0,0);

   button[12]->mode = SF_BUTTON_DOWN;
   return 0;
   }


/* ********************************************************** */
/* *** Return TRUE if click is handled, else return FALSE *** */
/* ********************************************************** */
int
SF_DisplayEngine::MouseClick(int mbutton,int x,int y,int keyup) {
   if ( mbutton == 1 && !keyup ) {

      int i,j;

      /* *** Video Console buttons *** */
      for ( i=0; i<12; i++ ) {
         if ( button[i]->Contains(x,y) ) {
            for ( j=0; j<12; j++ )
               button[j]->mode = SF_BUTTON_UP;
            button[i]->HandleClick();
            SndCueWav(game->soundEvent[push_button1_event],
                      game->soundEvent[push_button1_event]);
            redrawButtons = TRUE;
            return TRUE;
            }
         }

      /* *** Chat Console buttons *** */
      for ( i=12; i<15; i++ )
	{
	  if ( button[i]->Contains(x,y) )
	    {
	      for ( j=12; j<15; j++ )
		{
		  if ( button[j]->mode == SF_BUTTON_DOWN )
		    {
		      button[j]->mode = SF_BUTTON_UP;
		    }
		}
	      button[i]->HandleClick();
	      SndCueWav(game->soundEvent[push_button2_event],
			game->soundEvent[push_button2_event]);
	      redrawButtons = TRUE;
	      if ( button[i]->mode == SF_BUTTON_DOWN )
		{
		  int index = SF_CHANNEL_BROADCAST + (i-12);
		  chat->channel = &chat->channels[index];
		  chat->redrawAll = TRUE;
		  
		}
	      return TRUE;
            }
	}

      if ( button[15]->Contains(x,y) ) {
         button[15]->HandleClick(button[15]->mode);
         SndCueWav(game->soundEvent[push_button3_event],
                   game->soundEvent[push_button3_event]);
         return TRUE;
         }

      /* *** Video Console buttons *** */
      for ( i=16; i<22; i++ ) {
         if ( button[i]->Contains(x,y) ) {
            for ( j=16; j<22; j++ )
               button[j]->mode = SF_BUTTON_UP;
            button[i]->HandleClick();
            SndCueWav(game->soundEvent[push_button3_event],
                      game->soundEvent[push_button3_event]);
            button[i]->mode = SF_BUTTON_UP;
            redrawButtons = TRUE;
            return TRUE;
            }
         }

      }
   return FALSE;
   }


int
SF_DisplayEngine::DrawButtons(int imode) {
  for ( int i=0; i<SF_DISPLAYENGINE_NUMBUTTONS; i++ )
    {
      button[i]->visible = TRUE;
      button[i]->Draw(imode);
    }
  return 0;
}


int
SF_DisplayEngine::DestroyButtons() {
   for ( int i=0; i<SF_DISPLAYENGINE_NUMBUTTONS; i++ ) {
      if ( button[i] ) {
         delete button[i];
         button[i] = NULL;
         }
      }
   return 0;
   }



int
SF_DisplayEngine::Update() {

   if ( !game )
      return 0;

   if ( ge->itime % displaySkip == 0 && show) {

      sw.Start();

      int blitAll = FALSE;
      if ( repaint ) {
         blitAll = TRUE;
         repaint = FALSE;
         map->redrawAll = TRUE;
         chat->redrawAll = TRUE;
         video->redrawAll = TRUE;
         }

      int blitEntireBufferToScreen = FALSE;
      if ( waitForLogin &&
           e->localPlayerLoggedIn && 
           ge->itime > e->localPlayerLoggedIn + 3*frameRate )
	{
	  waitForLogin = FALSE;
	  blitEntireBufferToScreen = TRUE;
	}

      if ( blitAll ) {
         ClearBackBuffer(S_CANVASBUFFER_BLIT_ALL);
         }
      else
         ClearBackBuffer(S_CANVASBUFFER_BLIT_RECTANGLES);

      ulong lastCondition = condition;
      condition = RGB_DARKGREEN;

      if ( map )
         map->Draw();
      if ( chat )
         chat->Draw();

      if ( !waitForLogin && (redrawButtons || blitAll) )
	{
          if ( !firstTime )
	    {
	      DrawButtons();
	      DrawCondition();
	      redrawButtons = FALSE;
	    }
	}

      if ( video )
	video->buffer->ResetRects();
      bb->ResetRects(FALSE);

      lx0 = x0;
      ly0 = y0;

      x0 = game->ship[ge->localSlot].cX();
      y0 = game->ship[ge->localSlot].cY();

      if ( x0 == 0 && y0 == 0 )
         return 0;

      scrX0 = (window->cxClient - sf->cxPanel)/2;
      scrY0 = (window->cyClient - sf->cyPanel )/2;

      if ( window->MenuBarIsVisible() ) {
         scrX0 = (window->cxClient - sf->cxPanel)/2;
         scrY0 = (window->cyClient - sf->cyPanel +
                  window->root->cyTitleBar +
                  window->root->cyMenu )/2;
         }

      bb->SetClipRect(0,0,bb->cxCanvas-sf->cxPanel,bb->cyCanvas-sf->cyPanel);

      scrBotMsgBuffer->y0 = bb->cyCanvas - sf->cyPanel - 5;
      scrTopMsgBuffer->x0 = 10;

      DrawMainWindowDebris();

      DrawInternetCafe();

      bb->ResetClipRect();

      numObjectsOnScreen = 0;
      numEnemyShipsOnScreen = 0;
      numEnemyShipsNearby = 0;
      for ( int j=0; j<ge->numObjects; j++ ) {

         /* ***** Z-order patch ***** */
         int i = j;
         if ( j < game->N(GameStateClass::planets) )
            i = j + game->N(GameStateClass::ships);
         if ( j >= game->N(GameStateClass::planets) &&
              j < game->N(GameStateClass::statics) )
            i = j - game->N(GameStateClass::planets);
         /* ****** End of Z-order patch ****** */

         shouldBeDrawn = SetVisibilityVariables(i);
         if ( shouldBeDrawn ){
            sprite = SetObjectVariables(i);
            if ( sprite ) {
               bb->SetClipRect(0,0,
                               bb->cxCanvas - sf->cxPanel,
                               bb->cyCanvas - sf->cyPanel);
               DrawMainScreenObject(i);
               bb->ResetClipRect();
               if ( map && map->visible ) {
                  DrawMapConsoleObject(i);
	          }
	       }
            }
         }

      /* *** Draw any lines on the top of the Z-order *** */
      bb->SetClipRect(0,0,bb->cxCanvas-sf->cxPanel,bb->cyCanvas-sf->cyPanel);
      for ( int i=0; i<ge->numObjects; i++ ) {
         shouldBeDrawn = SetVisibilityVariables(i);
         if ( shouldBeDrawn ) {
            sprite = SetObjectVariables(i);
            if ( sprite != NULL ) {

               if ( isShip || isColony ) {

                  /* ****************************************** */
                  /* *** Draw line onscreen for tractor beam *** */
                  /* ****************************************** */
                  ShipClass *src = (ShipClass*)game->objectList[i];
		  if ( IsValidObjectPointer(src) )
		    {
		      if ( src->bus.dockMod && src->bus.dockMod[0] )
			for ( int k=0; k<src->bus.numPorts; k++ ) {
			  
			  if ( src->bus.dockMod[k]->IsActive() &&
			       src->bus.dockMod[k]->workPar[0] &&
			       !src->bus.dockMod[k]->workPar[1] ) {
			    
			    ShipClass *targ = (ShipClass *)
			      src->bus.dockMod[k]->workPar[0];
			    
			    CoordinateClass c;
			    slong *par = src->bus.dockMod[k]->specPar;
			    slong h = par[1];
			    if ( !h )
			      h = src->bitmap.size/2;
			    
			    if ( src->bitmap.IsSpin() ) {
			      c = src->now + 
				src->bitmap.Dir((short)(src->heading+par[0]))*h;
			    } 
			    else {
			      c = src->now + 
				Dir[(src->heading+par[0] +
				     2*max_angles)%max_angles]*h;
			    }
			    
			    slong x1 = scrX0+(c.x/sub_scale - x0)/tacticalZoom;
			    slong y1 = scrY0-(c.y/sub_scale - y0)/tacticalZoom;
			    slong x2 = scrX0+(targ->now.x/sub_scale-x0)/
			      tacticalZoom;
			    slong y2 = scrY0-(targ->now.y/sub_scale-y0)/
			      tacticalZoom;
			    
			    if ( labs(x1-x2) < 240 && labs(y1-y2) < 240 ) {
			      
			      int gamma = min(src->bus.dockMod[k]->func,
					      src->bus.dockMod[k]->XStorage());
			      int width = gamma/25;
			      width = 1;
			      if ( gamma < 50 )
				gamma = 50; 
#                          ifdef WIN32
                              if ( _root->bitsPerPixel == 8 )
				gamma = 100;
#                          endif
			      if ( ge->itime % 2 == 0 )
				gamma = 2*gamma/3;
			      ulong gcolor = RGB_GAMMA(color,gamma);
			      ulong size = targ->bitmap.Size()/2/tacticalScale;
			      ulong color1 = gcolor;
			      ulong color2 = gcolor;
			      ulong color3 = gcolor;
			      ulong color4 = gcolor;
			      if ( ge->itime % 4 == 0 )
				color1 = color;
			      else if ( ge->itime % 4 == 1 )
				color2 = color;
			      else if ( ge->itime % 4 == 2 )
				color3 = color;
			      else if ( ge->itime % 4 == 3 )
				color4 = color;
			      bb->DrawLine(x1,y1,
					   x2-size,y2-size,
					   width,color1,
					   S_CANVASBUFFER_ADDRECTANGLE);
			      bb->DrawLine(x1,y1,
					   x2+size,y2+size,
					   width,color2,
					   S_CANVASBUFFER_ADDRECTANGLE);
			      bb->DrawLine(x1,y1,
					   x2-size,y2+size,
					   width,color3,
					   S_CANVASBUFFER_ADDRECTANGLE);
			      bb->DrawLine(x1,y1,
					   x2+size,y2-size,
					   width,color4,
					   S_CANVASBUFFER_ADDRECTANGLE);
			    }
			  }
			}
		      
		      /* ****************************************** */
		      /* *** Draw line onscreen for weapon beam *** */
		      /* ****************************************** */
		      if ( src->bus.tubeMod && src->bus.tubeMod[0] )
			for ( int k=0; k<src->bus.numTubes; k++ ) {
			  
			  if ( src->bus.tubeMod[k]&&
			       src->bus.tubeMod[k]->moduleFunc==ModuleClass::beam &&
			       src->bus.tubeMod[k]->IsActive() &&
			       src->bus.tubeMod[k]->workPar[0] &&
			       src->bus.tubeMod[k]->workPar[1] ) {
			    
			    ShipClass *targ = (ShipClass *)
			      src->bus.tubeMod[k]->workPar[0];
			    
			    //
			    // Check for bad targ pointer
			    //
			    if ( IsValidObjectPointer(targ,__FILE__,__LINE__) )
			      {
				CoordinateClass c;
				slong *par = src->bus.tubeMod[k]->specPar;
				slong h = par[1];
				if ( !h )
				  h = src->bitmap.size/2;
				
				if ( src->bitmap.IsSpin() ) {
				  c = src->now + 
				    src->bitmap.Dir((short)(src->heading+par[0]))*h;
				} 
				else {
				  c = src->now + 
				    Dir[(src->heading+par[0] +
					 2*max_angles)%max_angles]*h;
				}
				
				CoordinateClass r1( scrX0+(c.x/sub_scale - x0) /
						    tacticalZoom,
						    scrY0-(c.y/sub_scale - y0)/
						    tacticalZoom);
				CoordinateClass r2(scrX0+(targ->now.x/sub_scale-x0)/
						   tacticalZoom,
						   scrY0-(targ->now.y/sub_scale-y0)/
						   tacticalZoom);
				
				slong ran = labs(c.x + targ->now.y + k) % 5 - 2;
				
				slong tOffset = (r1-r2).Heading(game->maxAngles) + ran;
				
				CoordinateClass rOffset = r2 + 
				  ::Dir[(tOffset+game->maxAngles) % game->maxAngles] *
				  sprite->cx / 2 / sub_scale;
				
				int gamma = 33 * ((ge->itime%3)+1);
				if ( (r1-r2).Mag(0) < (ulong) src->bus.tubeMod[k]->par[2] )
				  bb->DrawLine(r1.x, r1.y,
					       rOffset.x,
					       rOffset.y,
					       1,
					       RGB_GAMMA(color,gamma),
					       S_CANVASBUFFER_ADDRECTANGLE);
			      }
			  }
			}
		    }
	          }
	       }
            }
         }

      bb->ResetClipRect();

      if ( showDebugString ) {
         char fpsString[1024];
         sprintf(fpsString,
		 "fr: %ld | sfr: %ld | lag: %ld | dr: %ld | "
		 "e: %3ldms = de: %3ldms + ge: %3ldms --> %ld fps | %ld %d | ",
                 ge->frame,ne->serverFrame,ne->lag,ne->drift,
                 sw.dtUsec/1000+ge->sw.dtUsec/1000,
                 sw.dtUsec/1000,ge->sw.dtUsec/1000,
                 1000000/(sw.dtUsec+ge->sw.dtUsec+1),
                 ge->numObjects,c->mainWindow->scancode);
         bb->DrawString(fpsString,10,10,0xFFFFFF,
                        S_CANVASBUFFER_ADDRECTANGLE); 
         }

      if ( !waitForLogin )
	{
	  DrawBottomPanel(blitAll|blitEntireBufferToScreen);
	}

      if ( video )
         video->Draw();

      if ( condition != lastCondition )
         DrawCondition();

      scrTopMsgBuffer->Draw(bb,ge->itime);
      scrBotMsgBuffer->Draw(bb,ge->itime);

      if ( !waitForLogin )
	{
	  if ( blitAll || blitEntireBufferToScreen ) {
	    bb->Blit(window,0,-c->dy1,S_CANVASBUFFER_BLIT_ALL);
	  }
	  else
	    bb->Blit(window,0,-c->dy1,S_CANVASBUFFER_BLIT_RECTANGLES);
          firstTime = FALSE;
	}

      sw.Stop();

      displaySkip = 1 + (ge->sw.dtUsec + sw.dtUsec)*frameRate/1000000;
      if ( displaySkip > 4 ) displaySkip = 4;
      }

   if ( ge->itime % (3*frameRate) == 0 )
      soundIndex = SndCueWav(soundIndex,game->soundEvent[ping_event]);

   if ( ge->itime % frameRate == 0 &&
        game->ship[ge->localSlot].bus.shieldMod &&
        game->ship[ge->localSlot].bus.shieldMod->XStorage() < 
        module_break_limit )
      soundIndex = SndCueWav(soundIndex,game->soundEvent[klaxon_event]);

   if ( sf->profile->sound ) {
#     ifdef WIN32
         S_WaveData *w;

         if ( game->ship[ge->localSlot].bus.boostMod ) {
            w = c->mwl->Wave(game->ship[ge->localSlot].bus.
                             boostMod->activationSoundIndex);
            if ( w ) {
               if ( !w->rc ) {
                  if ( localPlayBoostSound ) {
                     c->audio->Play(w);
                     w->rc = 1;
   	             }
                  else {
                     c->audio->Stop(w);
	             }
	          }
               }
	    }

         if ( game->ship[ge->localSlot].bus.impulseMod ) {
            w = c->mwl->Wave(game->ship[ge->localSlot].bus.
                             impulseMod->activationSoundIndex);
            if ( w ) {
               if ( !w->rc ) {
                  if ( localPlayImpulseSound || localPlayBoostSound ) {
                     c->audio->Play(w);
                     w->rc = 1;
	             }
                  else {
                     c->audio->Stop(w);
	             }
	          }
	       }
            }
#     endif
      }

   localUseFlame = FALSE;
   localPlayImpulseSound = FALSE;
   localPlayBoostSound = FALSE;

   if ( c->profile->sound ) {
      S_WaveData *w = c->mwl->Wave(soundIndex);
      if (w) {
         c->audio->Play(w);
         soundIndex = -1;
         }
      }

   for ( int i=0; i<c->mwl->numWaves; i++ ) {
      S_WaveData *w = c->mwl->Wave(i);
      if ( w )
         w->rc = 0;
      }

   return 0;
   }

int
_StatusColor(slong s) {
  if (s <= module_break_limit)
    return RGB_DARKRED;
  else if (s <= module_warning_limit)
    return RGB_BROWN;
  else
    return RGB_DARKGREEN;
  }


int
SF_DisplayEngine::BlitRectToWindow(int x1,int y1,int x2,int y2)
{
  if ( !waitForLogin && !firstTime )
    {
      bb->SystemBlit(window,x1,y1-sf->dy1,x2+1,y2-sf->dy1+1,x1,y1);
    }
  return 0;
}


int
SF_DisplayEngine::BlitPanelBackground(int x1,int y1,int x2,int y2) {
   if ( ge->lpanelSprite )
      ((S_SystemBitmap *)ge->lpanelSprite)->Blit(bb,0,x1,y1,x2,y2,
           x1,(bb->cyCanvas-y2));
   return 0;
   }


int
SF_DisplayEngine::DrawTarget(int blitAll) {

   ShipClass *sh = &game->ship[ge->localSlot];
   GenericClass *tg = sh->targetPP.obj.generic;
   ShipClass *tsh = NULL;
   if ( tg && (tg->Type() == ship || tg->Type() == colony) )
      tsh = sh->targetPP.obj.ship;

   int targetChanged = blitAll || tsh != target;

   target = tsh;
   int targetBitmapIndex = -1;
   if ( tsh )
      targetBitmapIndex = tsh->originalBitmapIndex;

   SF_Sprite *tspr = ge->sl->Sprite(targetBitmapIndex);

   if ( ge->lpanelSprite && ge->lpanelSprite->rc ) {
      ge->lpanelSprite->cx = 460;
      ge->lpanelSprite->cy = 115;
      ge->lpanelSprite->RequestDownload();
      }

   if ( ge->lpanelSprite && tspr && tsh ) {
      char tmp[128];
      SizeStruct size;
      PlayerClass *pl = &game->player[tsh->playerIndex];
      RaceClass *r = &game->race[tsh->raceID];
      int x0 = ge->lpanelSprite->cx/2;
      int y0 = bb->cyCanvas - ge->lpanelSprite->cy + 10;
      int x1 = ge->lpanelSprite->cx - ge->lpanelSprite->cy/2 - 
               tspr->schematic->cx/2 - 5;
      int y1 = bb->cyCanvas - ge->lpanelSprite->cy/2 - 
                              tspr->schematic->cy/2 - 2;

      if ( targetChanged ) {
         BlitPanelBackground(x0+1,y0,
                             x0+ge->lpanelSprite->cx/2,
                             y0+ge->lpanelSprite->cy - 20);
         if ( tspr->schematic->rc )
            tspr->RequestSchematicDownload();
         else
            tspr->schematic->Blit(bb,0,x1,y1);
         bb->DrawString("Target",
                        x1,
                        bb->cyCanvas-ge->lpanelSprite->cy+20,RGB_GREEN);
         }

      if ( targetChanged || (ulong) targetSpeed != tsh->vel.Mag() ) {
         targetSpeed = tsh->vel.Mag();
         sprintf(tmp,"%.2fc",(double)targetSpeed/game->lightSpeed);
         BlitPanelBackground(ge->lpanelSprite->cx-ge->lpanelSprite->cy/2-20,
                             bb->cyCanvas-10-10,
                             ge->lpanelSprite->cx-ge->lpanelSprite->cy/2-20+50,
                             bb->cyCanvas-10);
	 FilterString(tmp);
         bb->DrawString(tmp,ge->lpanelSprite->cx-ge->lpanelSprite->cy/2-20,
                        bb->cyCanvas-10,RGB_GREEN);
         BlitRectToWindow(ge->lpanelSprite->cx-ge->lpanelSprite->cy/2-20,
                          bb->cyCanvas-10-10,
                          ge->lpanelSprite->cx-ge->lpanelSprite->cy/2-20+50,
                          bb->cyCanvas-10);
         }

      x0 += 5;
      y0 += 10;
      if ( targetChanged ) {
         sprintf(tmp,"%s %s",r->name,tsh->design->name);
	 FilterString(tmp);
         bb->DrawString(tmp,x0,y0,raceTextColor[tsh->raceID%MAX_COLORS]);
         }

      y0 += 12;
      if ( targetChanged && tg->Type() == ship )
	{
	  FilterString(pl->shipName);
	  bb->DrawString(pl->shipName,x0,y0,RGB_GREEN);
	}

      y0 += 12;
      if ( targetChanged && tg->Type() == ship ) {
         sprintf(tmp,"%s %s",game->rank[pl->rank].name,pl->name);
	 FilterString(tmp);
         bb->DrawString(tmp,x0,y0,RGB_GREEN);
         }

      y0 += 12;
      if ( targetChanged )
         bb->DrawString("Po:",x0,y0,RGB_GREEN);

      y0 += 10;
      if ( targetChanged )
         bb->DrawString("Sh:",x0,y0,RGB_GREEN);

      y0 += 10;
      if ( targetChanged )
         bb->DrawString("En:",x0,y0,RGB_GREEN);

      int cxRect = 70;

      y0 -= 29;
      if ( tsh->bus.numMod && 
          (targetChanged || targetPower != tsh->bus.powerMod[0]->func) ) {
         targetPower = tsh->bus.powerMod[0]->func;
         DrawGauge(x0+20,y0,cxRect,8,targetPower);
         BlitRectToWindow(x0+20,y0,x0+20+cxRect,y0+8);
         }

      y0 += 10;
      if ( tsh->bus.shieldMod &&
          (targetChanged || targetShield != tsh->bus.shieldMod->XStorage()) ) {
         targetShield = tsh->bus.shieldMod->XStorage();
         DrawGauge(x0+20,y0,cxRect,8,targetShield);
         BlitRectToWindow(x0+20,y0,x0+20+cxRect,y0+8);
         }

      y0 += 10;
      if ( tsh->bus.impulseMod &&
          (targetChanged || targetEngine != tsh->bus.impulseMod->func) ) {
         targetEngine = tsh->bus.impulseMod->func;
         DrawGauge(x0+20,y0,cxRect,8,targetEngine);
         BlitRectToWindow(x0+20,y0,x0+20+cxRect,y0+8);
         }

      y0 += 20;
      if ( tg->Type() == ship &&
          (targetChanged || targetKills != pl->totalScore) ) {
         targetScore = pl->totalScore;
         sprintf(tmp,"Score:%ld",pl->totalScore);
         size = bb->GetStringSize(tmp);
         BlitPanelBackground(x0,y0-size.cy+3,3*ge->lpanelSprite->cx/4,y0+3);
         bb->DrawString(tmp,x0,y0,RGB_GREEN);
         BlitRectToWindow(x0,y0-size.cy+3,3*ge->lpanelSprite->cx/4,y0+3);
         }

      y0 += 12;
      if ( tg->Type() == ship &&
          (targetChanged || targetKills != pl->numKillsTotal ||
                            targetCrew != (ulong)tsh->pop.lBillions ) ) {
         targetKills = pl->numKillsTotal;
         targetCrew = tsh->pop.lBillions;
         sprintf(tmp,"Kills:%ld - Crew:%ld",pl->numKillsTotal,
                 tsh->pop.lBillions);
         size = bb->GetStringSize(tmp);
         BlitPanelBackground(x0,y0-size.cy+3,3*ge->lpanelSprite->cx/4,y0+3);
         bb->DrawString(tmp,x0,y0,RGB_GREEN);
         BlitRectToWindow(x0,y0-size.cy+3,3*ge->lpanelSprite->cx/4,y0+3);
         }

      if ( tg->Type() == colony && (targetChanged || 
                                    targetCrew != (ulong)tsh->pop.lBillions) ){
         targetCrew = tsh->pop.lBillions;
         sprintf(tmp,"Pop:%s",tsh->pop.print());
         size = bb->GetStringSize(tmp);
         BlitPanelBackground(x0,y0-size.cy+3,3*ge->lpanelSprite->cx/4,y0+3);
         bb->DrawString(tmp,x0,y0,RGB_GREEN);
         BlitRectToWindow(x0,y0-size.cy+3,3*ge->lpanelSprite->cx/4,y0+3);
         }

      if ( !waitForLogin && !firstTime && targetChanged && ge->lpanelSprite ) {
         x1 = ge->lpanelSprite->cx/2;
         y1 = bb->cyCanvas - ge->lpanelSprite->cy;
         int x2 = ge->lpanelSprite->cx;
         int y2 = bb->cyCanvas;
         bb->SystemBlit(window,x1,y1-sf->dy1,x2,y2-sf->dy1,x1,y1);
         }
      }

   return 0;
   }


int
SF_DisplayEngine::DrawGauge(int x,int y,int cx,int cy,int ipercent) {
  //int value = x + cx*ipercent/100;
   for ( int i=x; i<x+cx; i+=2 ) {
      ulong color;
      int percent = (i-x)*100/cx;

      if ( percent <= module_break_limit ) {
         color =  RGB_DARKRED;
         if ( ipercent >= percent )
            color = RGB_RED;
         }
      else if ( percent <= module_warning_limit ) {
         color = RGB_BROWN;
         if ( ipercent >= percent )
            color = RGB_YELLOW;
         }
      else {
         color = RGB_DARKGREEN;
         if ( ipercent > percent )
            color = RGB_GREEN;
         }

      bb->DrawLine(i,y,i,y+cy,1,color);
      bb->DrawLine(i-1,y,i-1,y+cy,1,0);
      }
   return 0;
   }


int
SF_DisplayEngine::DrawLocalShip(int blitAll) {

   ShipClass *sh = &game->ship[ge->localSlot];

   int localChanged = blitAll || 
                    (sh && localPlayerBitmapIndex != 
                     sh->originalBitmapIndex);

   localPlayerBitmapIndex = sh->originalBitmapIndex;

   SF_Sprite *spr = ge->sl->Sprite(localPlayerBitmapIndex);

   if ( spr && sh && ge->lpanelSprite ) {
      char tmp[128];
      SizeStruct size;
      PlayerClass *pl = &game->player[sh->playerIndex];
      RaceClass *r = &game->race[sh->raceID];
      int x0 = 125;
      int y0 = bb->cyCanvas - ge->lpanelSprite->cy + 10;
      int x1 = ge->lpanelSprite->cy/2 - spr->schematic->cx/2 + 5;
      int y1 = bb->cyCanvas - ge->lpanelSprite->cy/2 - 
                              spr->schematic->cy/2 - 2;

      if ( localChanged ) {
         BlitPanelBackground(0,y0,
                             ge->lpanelSprite->cx/2-1,
                             y0+ge->lpanelSprite->cy - 20);
         if ( spr->schematic->rc )
            spr->RequestSchematicDownload();
         else
            spr->schematic->Blit(bb,0,x1,y1);
         }

      if ( localChanged || (ulong) localSpeed != sh->vel.Mag() ) {
         localSpeed = sh->vel.Mag();
         sprintf(tmp,"%.2fc",(double)localSpeed/game->lightSpeed);
         BlitPanelBackground(ge->lpanelSprite->cy/2,bb->cyCanvas-10-10,
                             ge->lpanelSprite->cy/2+50,bb->cyCanvas-10);
         bb->DrawString(tmp,ge->lpanelSprite->cy/2,bb->cyCanvas-10,RGB_GREEN);
         BlitRectToWindow(ge->lpanelSprite->cy/2,bb->cyCanvas-10-10,
                          ge->lpanelSprite->cy/2+50,bb->cyCanvas-10);
         }

      x0 += 5;
      y0 += 10;
      if ( localChanged ) {
         sprintf(tmp,"%s %s",r->name,sh->design->name);
	 FilterString(tmp);
         bb->DrawString(tmp,x0-15,y0,raceTextColor[sh->raceID%MAX_COLORS]);
         }

      y0 += 12;
      if ( localChanged )
	{
	  FilterString(pl->shipName);
	  bb->DrawString(pl->shipName,x0-5,y0,RGB_GREEN);
	}

      y0 += 12;
      if ( localChanged ) {
         sprintf(tmp,"%s %s",game->rank[pl->rank].name,pl->name);
	 FilterString(tmp);
         bb->DrawString(tmp,x0,y0,RGB_GREEN);
         }

      y0 += 12;
      if ( localChanged )
         bb->DrawString("Po:",x0,y0,RGB_GREEN);

      y0 += 10;
      if ( localChanged )
         bb->DrawString("Sh:",x0,y0,RGB_GREEN);

      y0 += 10;
      if ( localChanged )
         bb->DrawString("En:",x0,y0,RGB_GREEN);

      int cxRect = 70;

      y0 -= 29;
      if ( sh->bus.numMod && 
          (localChanged || localPower != sh->bus.powerMod[0]->XStorage()) ) {
         localPower = sh->bus.powerMod[0]->XStorage();
         DrawGauge(x0+20,y0,cxRect,8,localPower);
         BlitRectToWindow(x0+20,y0,x0+20+cxRect,y0+8);
         }

      y0 += 10;
      if ( sh->bus.shieldMod &&
          (localChanged || localShield != sh->bus.shieldMod->XStorage()) ) {
         localShield = sh->bus.shieldMod->XStorage();
         DrawGauge(x0+20,y0,cxRect,8,localShield);
         BlitRectToWindow(x0+20,y0,x0+20+cxRect,y0+8);
         }

      y0 += 10;
      if ( sh->bus.boostMod &&
          (localChanged || localEngine != sh->bus.boostMod->XStorage()) ) {
         localEngine = sh->bus.boostMod->XStorage();
         DrawGauge(x0+20,y0,cxRect,8,localEngine);
         BlitRectToWindow(x0+20,y0,x0+20+cxRect,y0+8);
         }

      y0 += 20;
      if ( (localChanged || localKills != pl->totalScore) ) {
         localScore = pl->totalScore;
         sprintf(tmp,"Score:%ld",pl->totalScore);
         size = bb->GetStringSize(tmp);
         BlitPanelBackground(x0-5,y0-size.cy+3,ge->lpanelSprite->cx/2,y0+3);
         bb->DrawString(tmp,x0-5,y0,RGB_GREEN);
         BlitRectToWindow(x0-5,y0-size.cy+3,ge->lpanelSprite->cx/2,y0+3);
         }

      y0 += 12;
      if ( (localChanged || localKills != pl->numKillsTotal ||
                            localCrew != (ulong)sh->pop.lBillions) ) {
         localKills = pl->numKillsTotal;
         localCrew = sh->pop.lBillions;
         sprintf(tmp,"Kills:%ld - Crew:%ld",pl->numKillsTotal,sh->pop.lBillions);
         size = bb->GetStringSize(tmp);
         BlitPanelBackground(x0-15,y0-size.cy+3,ge->lpanelSprite->cx/2,y0+3);
         bb->DrawString(tmp,x0-15,y0,RGB_GREEN);
         BlitRectToWindow(x0-15,y0-size.cy+3,ge->lpanelSprite->cx/2,y0+3);
         }

      if ( !waitForLogin && !firstTime && localChanged && ge->lpanelSprite ) {
         x1 = 0;
         y1 = bb->cyCanvas - ge->lpanelSprite->cy;
         int x2 = ge->lpanelSprite->cx/2;
         int y2 = bb->cyCanvas;
         bb->SystemBlit(window,x1,y1-sf->dy1,x2,y2-sf->dy1,x1,y1);
         }
      }

   return 0;
   }


int
SF_DisplayEngine::DrawBottomPanel(int blitAll) {

   if ( blitAll && ge->lpanelSprite && ge->lpanelSprite->rc == S_BITMAP_OK ) {
      int x1 = 0;
      int y1 = bb->cyCanvas - ge->lpanelSprite->cy;
      ((S_SystemBitmap *)ge->lpanelSprite)->Blit(bb,0,x1,y1);

      int x0 = ge->lpanelSprite->cx/2;
      int y0 = bb->cyCanvas - ge->lpanelSprite->cy + 10;
      bb->DrawLine(x0,y0,x0,bb->cyCanvas - 10,1,RGB_DARKGREEN);
      }

   DrawTarget(blitAll);
   DrawLocalShip(blitAll);

   return 0;
   }


int
SF_DisplayEngine::DrawCondition() {
  //int cxButton = 45;
   int cyButton = 15;
   int x0 = bb->cxCanvas-308;
   int y0 = sf->cyConsoleButtons-2-cyButton;

   int pX0 = x0 - sf->margin;
   int pY0 = y0 + cyButton;
   int rx = 17;
   int ry = sf->margin + 2*cyButton - 5;

   bb->DrawFilledPieWedge(pX0,pY0,rx,ry,90,180,condition);
   bb->DrawPieWedge(pX0,pY0,rx,ry,90,180,RGB_DARKGRAY);
   bb->DrawLine(x0-sf->margin,y0+cyButton,
                x0-sf->margin,y0-sf->margin-cyButton+5-1,1,RGB_BLACK);
   bb->DrawLine(x0-sf->margin,y0+cyButton,
                x0-sf->margin-18,y0+cyButton,1,RGB_BLACK);

   if ( !waitForLogin && !firstTime )
     {
       bb->SystemBlit(window,pX0-rx,pY0-ry-sf->dy1,pX0,pY0-sf->dy1,pX0-rx,pY0-ry);
     }
   return 0;
   }


int
SF_DisplayEngine::SetVisibilityVariables(int i) {
   active = IsValidObjectPointer(game->objectList[i],__FILE__,__LINE__) && 
	        game->objectList[i]->IsActive();
   if ( active ) {
      visible = i == ge->localSlot;
      if ( i != ge->localSlot )
	{
	  visible=game->objectList[i]->CanBeSeenBy(&game->ship[ge->localSlot]);
	}
      isAWarpingShip = 
	i >= 0 && 
	i < game->numShips &&
	game->objectList[i]->Type()==(ObjectType)ship &&
	game->ship[i].IsWarping();
      stealth = 0;
      shouldBeDrawn = active;
      }
   else {
      visible = 0;
      isAWarpingShip = 0;
      stealth = 0;
      shouldBeDrawn = 0;
      }
   return shouldBeDrawn;
   }

SF_Sprite *SF_DisplayEngine::SetObjectVariables(int i) {
  sprite = NULL;
  if ( IsValidObjectPointer(game->objectList[i],__FILE__,__LINE__) )
    {
      gx = game->objectList[i]->X();
      gy = game->objectList[i]->Y();
      ngx = game->objectList[i]->gnow.x/sub_scale;
      ngy = game->objectList[i]->gnow.y/sub_scale;
      angle = game->objectList[i]->Offset();

      bitmapIndex = game->objectList[i]->Index();
      race = game->objectList[i]->raceID;
      color = raceBlipColor[race];
      isSun = FALSE;
      isMinor = FALSE;
      if ( i >= game->planetObjectIndexStart && 
	   i <= game->planetObjectIndexEnd ) {
	isSun   = game->planet[i-game->planetObjectIndexStart].minor == -1;
	isMinor = game->planet[i-game->planetObjectIndexStart].minor ==  1;
      }
      isPlanet = 
	i>=game->planetObjectIndexStart && 
	i<=game->planetObjectIndexEnd &&
	game->objectList[i]->Type() == (ObjectType)planet;

      isShip = 
	i>=game->shipObjectIndexStart && 
	i<=game->shipObjectIndexEnd &&
	game->objectList[i]->Type() == (ObjectType)ship;

      isRobotShip = 
	isShip &&
	game->IsRobotPlayer((ShipClass *)game->objectList[i]);

      isColony = 
	i>=game->colonyObjectIndexStart && 
	i<=game->colonyObjectIndexEnd &&
	game->objectList[i]->Type() == (ObjectType)colony;

      isTargetted = ( game->ship[ge->localSlot].targetPP.obj.generic &&
		      game->ship[ge->localSlot].targetPP.obj.generic == 
		      game->objectList[i] ) && 
	!game->objectList[i]->IsExploding();
      
      colonizedPlanet = NULL;
      if ( isColony )
	colonizedPlanet=((ColonyClass*)game->objectList[i])->lockObj.generic;
      if ( isPlanet && !((PlanetClass *)game->objectList[i])->colony )
	color = 0xC0C0C0;
      if ( isSun )
	color = RGB_ORANGE;
      sprite = ge->sl->Sprite(bitmapIndex);
      isTerminal = game->objectList[i]->bitmap.IsTerminal();
      isTransitional = game->objectList[i]->bitmap.IsTransitional();
      
      useFlame = FALSE;
      
      if ( isShip ) {
	BusStruct *bus = &game->ship[i].bus;
	
	if ( bus->impulseMod && bus->impulseMod->func > module_break_limit ) {
	  if ( i == ge->localSlot )
            localPlayImpulseSound = ge->impulseOn[i];
	  if ( bus->boostMod && (bus->boostMod->active||ge->boostOn[i]) && 
	       bus->boostMod->func > module_break_limit ) {
            if ( i == ge->localSlot )
	      localPlayBoostSound = TRUE;
            if ( sprite && sprite->flame && !sprite->flame->rc ) {
	      useFlame = TRUE;
	      if ( i == ge->localSlot )
		localUseFlame = TRUE;
	    }
	  }
	}
	
	stealth = game->ship[i].stealthLevel;
      }
    }
  
  return sprite;
}


int
SF_DisplayEngine::DrawMapConsoleObject(int i) {
   if ( !map )
      return 0;

   if ( !isShip && !isPlanet && !isColony )
      return 0;

   if ( !visible /*&& !isAWarpingShip*/ )
      return 0;

   int zoom = strategicZoom;
   int scale = strategicScale;

   int cx = sprite->cx/scale;
   int cy = sprite->cy/scale;

   if ( cx < 1 ) cx = 1;
   if ( cy < 1 ) cy = 1;

   if ( isPlanet ) {
      cx *= 2;
      cy *= 2;
      }

   if ( isShip ) {
      cx = 9;
      cy = 9;
      }

   if ( isColony ) {
      cx = 7;
      cy = 7;
      }

   slong x = map->buffer->cxCanvas/2 + (gx - x0)/zoom - cx/2;
   slong y = map->buffer->cyCanvas/2 - (gy - y0)/zoom - cy/2;

   slong x0 = x + cx/2;
   slong y0 = y + cy/2;

   slong nx = map->buffer->cxCanvas/2 + (ngx - x0)/zoom - cx/2;
   slong ny = map->buffer->cyCanvas/2 - (ngy - y0)/zoom - cy/2;

   int onScr = (x > -cx) && (x < map->buffer->cxCanvas ) && 
               (y > -cy) && (y < map->buffer->cyCanvas );

   int dontDrawObject = FALSE;
   if ( zoom >= 64 && (isShip || isColony) && i != ge->localSlot )
      dontDrawObject = TRUE;

   char *text = NULL;
   if ( onScr && !dontDrawObject ) {

      ulong scolor = color;
      if ( i == ge->localSlot )
         scolor = 0xFFFFFF;

      if ( isPlanet )
         map->buffer->DrawFilledCircle(x0,y0,cy/2,color,
                                 S_CANVASBUFFER_ADDRECTANGLE);
      else if ( isShip ) {
         map->buffer->DrawTriangle(x0-5,y0+5,x0+5,y0+5,x0,y0-5,scolor,
                                   S_CANVASBUFFER_ADDRECTANGLE);
         }
      else if ( isColony && !colonizedPlanet ) {
         map->buffer->DrawRect(x0-3,y0-3,x0+4,y0+4,color,
                               S_CANVASBUFFER_ADDRECTANGLE);
         }
      if ( isSun )
         text = ((PlanetClass *)(game->objectList[i]))->name;
      if ( isPlanet && !isMinor && zoom < 128 )
         text = ((PlanetClass *)(game->objectList[i]))->name;
      if ( isMinor && zoom < 32 )
         text = ((PlanetClass *)(game->objectList[i]))->name;
      if ( isShip && zoom < 32 )
         text = game->player[game->ship[i].playerIndex].shipName;

      if ( visible && isTargetted && ge->itime % 10 < 5 )
         map->buffer->DrawRect(x-2,y-2,x+cx+2,y+cy+2,0xFF0000,
                              S_CANVASBUFFER_ADDRECTANGLE);

      if ( mainFont && mainFont->spr ) {
         if ( text ) {
	   FilterString(text);
            mainFont->DrawString(map->buffer,text,x,y+cy+mainFont->cy,race);
            map->buffer->AddRect(x-1,y+cy-1,
                                 x+mainFont->cx*strlen(text)+1,
                                 y+cy+mainFont->cy+1);
            }
         }
      else {
         if ( text )
	   {
	     FilterString(text);
            map->buffer->DrawString(text,x,y+cy+10,color,
                                    S_CANVASBUFFER_ADDRECTANGLE);
	   }
         }

   } /* onScreen */

   return 0;
   }


int
SF_DisplayEngine::DrawMainWindowDebris() {

   if ( tacticalZoom > 1 )
      return 0;

   int numDebrisToShow = max(0,numDebris-numObjectsOnScreen);
   numDebrisToShow = numDebris;

   int cxScreen = bb->clipRect.xRight - bb->clipRect.xLeft;
   int cyScreen = bb->clipRect.yBottom - bb->clipRect.yTop;

   int i;
   for ( i=0; i<numDebrisToShow; i++ ) {
      int j = i % ge->debrisSprite->numFrames;

      int cx = ge->debrisSprite->cx;
      int cy = ge->debrisSprite->cy;

      slong x = scrX0  - cx/2 + (xDebris[i]-x0)/debrisDist[i];
      slong y = scrY0  - cy/2 - (yDebris[i]-y0)/debrisDist[i];

      ge->debrisSprite->Blit(bb,j,x,y);

      /* ******* Wrap around screen ******* */
      if ( x > bb->clipRect.xRight + cx ) {
         xDebris[i] = x0 - (scrX0 + cx)*debrisDist[i];
         yDebris[i] = ( (rand()%(abs(cyScreen)+1))-scrY0 )*debrisDist[i] + y0;
         }
      else if ( x < bb->clipRect.xLeft-2*cx ) {
         xDebris[i] = x0 + (scrX0 + cy)*debrisDist[i];
         yDebris[i] = ( (rand()%(abs(cyScreen)+1))-scrY0 )*debrisDist[i] + y0;
         }
      if ( y > bb->clipRect.yBottom + cy ) {
         yDebris[i] = y0 + (scrY0 + cy)*debrisDist[i];
         xDebris[i] = ( (rand()%(abs(cxScreen)+1))-scrX0 )*debrisDist[i] + x0;
         }
      else if ( y < bb->clipRect.yTop-2*cy ) {
         yDebris[i] = y0 - (scrY0 + cy)*debrisDist[i];
         xDebris[i] = ( (rand()%(abs(cxScreen)+1))-scrX0 )*debrisDist[i] + x0;
         }
      }

   if ( numDebrisToShow==0 || abs(lx0-x0)>cxScreen || abs(ly0-y0)>cyScreen ||
        window->cx == 0 || window->cy == 0 ) {
      for ( i=0; i<numDebris; i++ ) {
         xDebris[i] = ( (rand()%(abs(cxScreen)+1))-scrX0 )*debrisDist[i] + x0;
         yDebris[i] = ( (rand()%(abs(cyScreen)+1))-scrY0 )*debrisDist[i] + y0; 
        }
      }

   numObjectsOnScreen = 0;
   numEnemyShipsOnScreen = 0;
   numEnemyShipsNearby = 0;

   return 0;
   }


int _lag[max_players] = {
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
   };


void 
SetShipLag(int i,int lag)
{
  if ( i >= 0 && i < max_players )
    {
      _lag[i] = lag;
    }
}


int 
GetShipLag(int i)
{
  if ( i >= 0 && i < max_players )
    {
      return _lag[i];
    }
  return 0;
}


int
SF_DisplayEngine::DrawMainScreenObject(int i) {

  if ( !IsValidObjectPointer(game->objectList[i],__FILE__,__LINE__) )
    {
      return 0;
    }

   int zoom = tacticalZoom;
   int scale = tacticalScale;

   int cx = sprite->cx/scale;
   int cy = sprite->cy/scale;

   if ( sprite->cx <= 4 ) cx = sprite->cx;
   if ( sprite->cy <= 4 ) cy = sprite->cy;

   slong x = scrX0 + (gx - x0)/zoom - cx/2;
   slong y = scrY0 - (gy - y0)/zoom - cy/2;

   slong nx = scrX0 + (ngx - x0)/zoom - cx/2;
   slong ny = scrY0 - (ngy - y0)/zoom - cy/2;

   onScreen[i] = (x > -cx) && (x < bb->cxCanvas - sf->cxPanel) && 
                 (y > -cy) && (y < bb->cyCanvas - sf->cyPanel);

   if ( condition == RGB_DARKGREEN && isShip &&
        race != game->ship[ge->localSlot].raceID &&
        x-scrX0>-1024 && x-scrX0<1024 && y-scrY0>-1024 && y-scrY0<1024 ) {
      numEnemyShipsNearby++;
      condition = RGB_YELLOW;
      }


   if ( onScreen[i] ) {
      char *text = NULL;
      char *text2 = NULL;
      char label[1024];
      char label2[1024];

      soundIndex = SndCueWav(soundIndex,game->objectList[i]->currentSound);

      numObjectsOnScreen++;
      if ( isShip && race != game->ship[ge->localSlot].raceID ) {
         numEnemyShipsOnScreen++;
         condition = RGB_RED;
         }
      

      if ( zoom == 1 ) {

         if ( isShip ) {
            if ( (visible && !stealth) || isTerminal || isTransitional ) {
               sprite->ShipBlit(bb,angle,x,y,S_BITMAP_BLIT_USEMASK,useFlame);

               if ( showDebugString && i != ge->localSlot )
                  sprite->ShipBlit(bb,angle,nx,ny,S_BITMAP_BLIT_USEMASK);

	       }
            else if ( !visible && i != ge->localSlot && !isAWarpingShip ) {
               sprite->Blit(bb,angle,x,y,S_BITMAP_BLIT_MASKONLY);
	       }
            else if ( visible && stealth ) {
               sprite->Blit(halftoneBuffer,angle,0,0,
                            S_BITMAP_BLIT_OPAQUE);
#              ifdef OS2
                  ge->halftoneSprite->Blit(halftoneBuffer,8-stealth,0,0,
                                           S_BITMAP_BLIT_AND);
#              else
                  ge->halftoneSprite->Blit(halftoneBuffer,stealth,0,0,
                                           S_BITMAP_BLIT_AND);
#              endif
               sprite->Blit(bb,angle,x,y,S_BITMAP_BLIT_MASKONLY);
               halftoneBuffer->ResetRects();
               halftoneBuffer->SystemBlit(bb,x,y,
                                          x+sprite->cx,y+sprite->cy,0,0,
                                          ROP_SRCINVERT);
               bb->AddRect(x,y,x+sprite->cx,y+sprite->cy);
	       }
	    }
         else {
            sprite->Blit(bb,angle,x,y);
            }

         if ( i == ge->localSlot && !game->ship[i].IsExploding() && 
              !isAWarpingShip ) {


	   //
	   // Draw shield around ship
	   //

	   int hasObjectDocked = 
	     game->ship[i].bus.numPorts == 1 &&
	     game->ship[i].bus.dockMod[0] && 
	     game->ship[i].bus.numPorts > 0 &&
	     game->ship[i].bus.dockMod[0]->workPar[0] &&
	     game->ship[i].bus.dockMod[0]->workPar[1];

            ulong shcolor;
            ulong store;
            if ( !hasObjectDocked &&
		 game->ship[i].bus.shieldMod && !stealth ) {
	      store = game->ship[i].bus.shieldMod->XStorage();

               if ( store < 1 ) store = 1;
               else if ( store > 100 ) store = 100;
               SF_Sprite *spr = ge->shldSprites[sprite->cx];
               if ( spr ) {
                  spr->Blit(bb,((100-store)*spr->numFrames)/100,
                            x+sprite->cx/2-spr->cx/2,
                            y+sprite->cy/2-spr->cy/2);
	          }
               else {
                  shcolor = RGB_DARKGREEN;
                  if ( store < (int)module_warning_limit )
                     shcolor = RGB_BROWN;
                  if ( store < (int)module_break_limit )
                     shcolor = RGB_DARKRED;
                  bb->DrawCircle(x+sprite->cx/2,y+sprite->cy/2,
                                 sprite->cy/2+2,shcolor,
                                 S_CANVASBUFFER_ADDRECTANGLE);
	          }
	       }

            if ( i == ge->localSlot && game->ship[i].bitmap.IsTime() ) {
               ShipClass *s = &game->ship[i];
               int frame = (s->heading + 2*s->bitmap.numFrames) %
                            s->bitmap.numFrames;
               slong rad = sprite->cx/2;
               slong dx = s->bitmap.Dir(frame).x*rad/sub_scale;
               slong dy = s->bitmap.Dir(frame).y*rad/sub_scale;
               slong x0 = x + sprite->cx/2 + dx;
               slong y0 = y + sprite->cy/2 - dy;
               bb->DrawFilledCircle(x0,y0,2,RGB_DARKGREEN);
               }

            if ( game->ship[i].bus.boostMod ) {
               store = game->ship[i].bus.boostMod->XStorage();
               shcolor = RGB_DARKGREEN;
               if ( store < (int)module_warning_limit )
                  shcolor = RGB_BROWN;
               if ( store < (int)module_break_limit )
                  shcolor = RGB_DARKRED;
               bb->DrawLine(x,y+sprite->cy+20+2,
                            x+sprite->cx*store/100,y+sprite->cy+20+2,
                            1,shcolor,
                            S_CANVASBUFFER_ADDRECTANGLE);
               }

            if ( game->ship[i].bus.stealthMod && stealth ) {
               store = game->ship[i].bus.stealthMod->XStorage();
               shcolor = RGB_DARKGREEN;
               if ( store < (int)module_warning_limit )
                  shcolor = RGB_BROWN;
               if ( store < (int)module_break_limit )
                  shcolor = RGB_DARKRED;
               bb->DrawLine(x,y+sprite->cy,
                            x,y+sprite->cy*(100-store)/100,
                            1,shcolor,
                            S_CANVASBUFFER_ADDRECTANGLE);
               }
	    }

         if ( game->objectList[i]->Type() == missile &&
	     ((MissileClass*)game->objectList[i])->duty == plugin &&
	     game->objectList[i]->IsExploding() ) {
            int nmiss =  ( (MissileClass*)game->objectList[i])->
                            bus.moduleList[1].workPar[0];
            char* name = ( (MissileClass*)game->objectList[i])->
                            bus.moduleList[1].name;
            if ( ((MissileClass*)game->objectList[i])->
                   ModuleType(1)!=ModuleClass::tube )
               strcpy(label, name);
            else
               sprintf(label,"%s(%d)",name,nmiss);
            text = label;
	    }

        }
      else {
         if ( visible ) {
            if ( isShip )
               sprite->ShipStretchBlit(bb,angle,x,y,cx,cy,
                                       S_BITMAP_BLIT_USEMASK,useFlame);
            else if ( sprite->cx <= 4 && sprite->cy <= 4 )
               sprite->Blit(bb,angle,x,y);
            else
               sprite->StretchBlit(bb,angle,x,y,cx,cy);
	    }
         }

      if ( visible && isShip ) {
         int pi = game->ship[i].playerIndex;
         int ri = game->player[pi].rank;
         sprintf(label,"%d %s%s",
                 i,game->rank[ri].name,game->player[pi].name);
         if ( i != ge->localSlot && !isRobotShip )
            sprintf(label,"%s [%03dms]",label,_lag[i]*1000/frameRate);
         else
            sprintf(label,"%s [%03ldms]",label,ne->lag*1000/frameRate);
         text = label;
	 sprintf(label2,"%s",game->player[game->ship[i].playerIndex].shipName);
	 if (  ge->controlsDisabled )
	   {
	     sprintf(label2,"%s [Controls Disabled]",
		     game->player[game->ship[i].playerIndex].shipName);
	   }
         text2 = label2;
         }
      else if ( visible && isColony && !colonizedPlanet )
         text = ((ColonyClass *)(game->objectList[i]))->pop.print();

      if ( isPlanet ) {
         text = ((PlanetClass *)(game->objectList[i]))->name;
         if ( game->PlanetIsSpawnable(i-game->FirstPlanetObjectIndex(), game)){
            sprintf(label,"[Colony available]");
            text2 = label;
	    }
         }

      if ( visible && isTargetted && ge->itime % 10 < 5 )
         bb->DrawRect(x-2,y-2,x+cx+2,y+cy+2,0xFF0000,
                      S_CANVASBUFFER_ADDRECTANGLE);

      FilterString(text);
      FilterString(text2);

      if ( mainFont && mainFont->spr ) {
         if ( text && !(isShip && isTerminal) ) {
            mainFont->DrawString(bb,text,x,y+cy+mainFont->cy,race);
            bb->AddRect(x-1,y+cy-1,
                        x+mainFont->cy*strlen(text)+1,y+cy+mainFont->cy+1);
            }
         if ( text2 && !(isShip && isTerminal) ) {
            mainFont->DrawString(bb,text2,x,y+cy+2*mainFont->cy,race);
            bb->AddRect(x-1,y+cy+mainFont->cy-1,
                        x+mainFont->cx*strlen(text2)+1,y+cy+2*mainFont->cy+1);
            }
         }
      else {
         if ( text && !(isShip && isTerminal) )
            bb->DrawString(text,x,y+cy+10,color,S_CANVASBUFFER_ADDRECTANGLE);
         if ( text2 && !(isShip && isTerminal) )
            bb->DrawString(text2,x,y+cy+20,color,S_CANVASBUFFER_ADDRECTANGLE);
         }

      } /* onScreen */

   return 0;
   }



void 
SetScreenMessage(char *msg,int color) {
   if ( !sf || !msg ) return;
   if ( !sf->engine ) return;
   if ( !sf->engine->displayEngine ) return;
   if ( msg[0] == '\0' ) return;
   if ( strcmp(msg," ") == 0 ) return;
   SF_TextBuffer *tb = sf->engine->displayEngine->scrTopMsgBuffer;
   if ( tb ) {
      for ( int i=tb->numLines-1; i>0; i-- )
         tb->SetLine(i,tb->text[i-1],CLR_WHITE,tb->lastUpdate[i-1]);
      tb->SetLine(0,msg,CLR_WHITE,sf->engine->gameEngine->itime);
      }
   }

void 
SetScreenBottomMessage( char *msg, int colorIndex ) {
   if ( !sf ) return;
   if ( !sf->engine ) return;
   if ( !sf->engine->displayEngine ) return;
   SF_TextBuffer *tb = sf->engine->displayEngine->scrBotMsgBuffer;
   if ( tb && sf->engine->gameEngine )
      tb->SetLine(0,msg,raceTextColor[colorIndex%MAX_COLORS],
                  sf->engine->gameEngine->itime);
   }



slong rCafe = 5000;
slong drCafe = 320;

int
IsInInternetCafe(GenericClass *s)
{

  if ( !IsValidObjectPointer(s) )
  {
    return 0;
  }

  for ( int i=-1; i<=1; i++ )
    {
      for ( int j=-1; j<=1; j++ )
	{
	  if ( i != 0 || j != 0 )
	    {
	      slong eggX = rCafe*i;
	      slong eggY = rCafe*j;

	      if ( s->now.x/sub_scale > eggX && s->now.x/sub_scale < eggX+drCafe && 
		   s->now.y/sub_scale > eggY-drCafe && s->now.y/sub_scale < eggY)
		{
		  return TRUE;
		}
	    }
	}
    }
  return 0;
}


void
SF_DisplayEngine::DrawInternetCafe()
{
  //
  // Draw Easter Egg
  //
  if ( tacticalZoom == 1 )
    {
      for ( int i=-1; i<=1; i++ )
	{
	  for ( int j=-1; j<=1; j++ )
	    {
	      if ( i != 0 || j != 0 )
		{
		  slong eggX = scrX0 + (rCafe*i - x0);
		  slong eggY = scrY0 - (rCafe*j - y0);
		  if ( (eggX > -(int)ge->egg.cx) && (eggX < bb->cxCanvas - sf->cxPanel) && 
		       (eggY > -(int)ge->egg.cy) && (eggY < bb->cyCanvas - sf->cyPanel) )
		    {
		      ge->egg.Blit(bb,0,eggX,eggY);
		      bb->AddRect(eggX,eggY,eggX+ge->egg.cx,eggY+ge->egg.cy);
		    }
		}
	    }
	}
    }
}
