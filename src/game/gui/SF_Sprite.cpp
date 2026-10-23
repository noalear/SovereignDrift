/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
#include "sf.h"
#include "SF_PortCpp.h"
#include "SF_Externals.h"
#include "SF_Sprite.h"
#include "SF_SpriteList.h"
#include "SF_Globals.h"
#include "SF_ClientWindow.h"
#include "SF_Client.h"
#include "SF_Slot.h"
#include "SF_MsgBuffer.h"
#include "SF_Engine.h"
#include "SF_NetEngine.h"
#include "SF_GameEngine.h"
#include "SF_DisplayEngine.h"

SF_Sprite::SF_Sprite( char *iname,char *ifilename,slong imaskFlags,
                      slong idefaultBlitFlags,slong inumFrames,
                      slong igamma,ulong itColor ) :
   S_SystemBitmap( ifilename,imaskFlags,idefaultBlitFlags,inumFrames,
                   igamma,itColor + (itColor<<8) + (itColor<<16)) {

   name = (char *)S_malloc(strlen(iname)+1);
   filename = (char *)S_malloc(strlen(ifilename)+1);
   strcpy(name,iname);
   strcpy(filename,ifilename);
   schematic3D = NULL;
   schematic = NULL;
   flame = NULL;

   LoadFlame();
   LoadSchematic();
   LoadSchematic3D();

   }


int
SF_Sprite::IsACOnly(char *type)
{
  if ( secretCode == 0x20202020 )
    {
      return TRUE;
    }

  if ( checkSum == 0x59181c || checkSum == 0x37f2ef || checkSum == 0x4b05dc ||
       checkSum == 0x0b1df6 || checkSum == 0x1cd9c3 || checkSum == 0x0cf32d ||
       checkSum == 0x469851 || checkSum == 0x339f9a || checkSum == 0x3f1787 )
    {
      return TRUE;
    }

  if ( strstr(type,"Starbase") || strstr(type,"Marauder") || 
       strstr(type,"BattleCruiser") )
    {
      return TRUE;
    }

  if (
      !strcmp(name,"sf-a-sb") || 
      !strcmp(name,"sf-d-sb") || 
      !strcmp(name,"sf-t-sb") ||
      !strcmp(name,"sf-a-ma") || 
      !strcmp(name,"sf-d-ma") || 
      !strcmp(name,"sf-t-ma") ||
      !strcmp(name,"sf-a-bc") || 
      !strcmp(name,"sf-d-bc") || 
      !strcmp(name,"sf-t-bc")
      )
    {
      return TRUE;
    }

  return FALSE;
}


int
SF_Sprite::LoadSchematic() {
   if ( schematic ) {
      delete schematic;
      schematic = NULL;
      }
   S_String fn("%s-s",filename);
   if ( fn.str ) {
      schematic = new S_SystemBitmap(fn.str,S_BITMAP_MASK_TRANSPARENT,
                                     S_BITMAP_BLIT_USEMASK,1,0,0);
      if ( schematic->rc ) {
         schematic->cx = 64;
         schematic->cy = 64;
         }
      }

   return 0;
   }


int
SF_Sprite::LoadSchematic3D() {
   if ( schematic3D ) {
      delete schematic3D;
      schematic3D = NULL;
      }
   S_String fn("%s-3",filename);
   if ( fn.str ) {
      schematic3D = new S_SystemBitmap(fn.str,S_BITMAP_MASK_TRANSPARENT,
                                       S_BITMAP_BLIT_USEMASK,1,0,0);
      if ( schematic->rc ) {
         schematic3D->cx = 96;
         schematic3D->cy = 96;
         }
      }

   return 0;
   }


int
SF_Sprite::LoadFlame() {
   if ( flame ) {
      delete flame;
      flame = NULL;
      }
   S_String fn("%s-f",filename);
   if ( fn.str ) {
      flame = new S_SystemBitmap(fn.str,S_BITMAP_MASK_TRANSPARENT,
                                 S_BITMAP_BLIT_USEMASK,
                                 numFrames,0,0);
      }

   return 0;
   }


int
SF_Sprite::Reload() {
   rc = 0;
   S_BitmapData bmpData(filename);
   S_SystemBitmap::Destroy();
   S_SystemBitmap::Create(&bmpData,maskFlags,defaultBlitFlags,
                          numFrames,gamma,tColor);
   return 0;
   }

SF_Sprite::~SF_Sprite() {
   if ( filename ) {
      S_free( filename );
      filename = NULL;
      }
   if ( name ) {
      S_free( name );
      name = NULL;
      }
   if ( schematic ) {
      delete schematic;
      schematic = NULL;
      }
   if ( schematic3D ) {
      delete schematic3D;
      schematic3D = NULL;
      }
   if ( flame ) {
      delete flame;
      flame = NULL;
      }
   }


int
SF_Sprite::RequestSchematicDownload() {

   if ( sf->engine->state != SF_CLIENT_CONNECTED )
      return 0;

   SF_NetEngine *ne = sf->engine->netEngine;
   SF_GameEngine *ge = sf->engine->gameEngine;
   SF_DisplayEngine *de = sf->engine->displayEngine;
   if ( ne && schematic && schematic->rc ) {
      SF_Sprite *msprite = ge->sl->Sprite(name);
      if ( msprite ) {
         char *schFilename = (char *)S_malloc(strlen(msprite->filename)+2+
                                            strlen(".bmp")+1);
         if ( schFilename ) {
            sprintf(schFilename,"%s-s.bmp",msprite->filename);
            if ( msprite->schematic->rc == SF_SPRITE_HASBEENREQUESTED &&
                 _OsFileExists(schFilename) ) {
               msprite->LoadSchematic();
               LoadSchematic();
               de->repaint = TRUE;
               }
            if ( schematic->rc && schematic->rc!=SF_SPRITE_HASBEENREQUESTED ) {
               ne->slot->writeBuffer.mutex.Request();
	       mstate = 70;
               ne->slot->writeBuffer.PushMsg((uchar)SF_MSG_DOWNLOADFILE,
                                             (ushort)ge->localSlot,
                                             (char *)schFilename,
                                             (ulong)0,
                                             (char *)NULL,
                                             (int)S_CLIENT_TCP);
	       mstate = 0;
               ne->slot->writeBuffer.mutex.Release();
               schematic->rc = SF_SPRITE_HASBEENREQUESTED;
               }
            S_free(schFilename);
            }
         }
      }
   return 0;
   }


int
SF_Sprite::RequestSchematic3DDownload() {

   if ( sf->engine->state != SF_CLIENT_CONNECTED )
      return 0;

   SF_NetEngine *ne = sf->engine->netEngine;
   SF_GameEngine *ge = sf->engine->gameEngine;
   SF_DisplayEngine *de = sf->engine->displayEngine;
   if ( ne && schematic3D && schematic3D->rc ) {
      SF_Sprite *msprite = ge->sl->Sprite(name);
      if ( msprite ) {
         char *schFilename = (char *)S_malloc(strlen(msprite->filename)+2+
                                            strlen(".bmp")+1);
         if ( schFilename ) {
            sprintf(schFilename,"%s-s.bmp",msprite->filename);
            if ( msprite->schematic3D->rc == SF_SPRITE_HASBEENREQUESTED &&
                 _OsFileExists(schFilename) ) {
               msprite->LoadSchematic3D();
               LoadSchematic3D();
               de->repaint = TRUE;
               }
            if ( schematic3D->rc && 
                 schematic3D->rc != SF_SPRITE_HASBEENREQUESTED ) {
               ne->slot->writeBuffer.mutex.Request();
	       mstate = 71;
               ne->slot->writeBuffer.PushMsg((uchar)SF_MSG_DOWNLOADFILE,
                                             (ushort)ge->localSlot,
                                             (char *)schFilename,
                                             (ulong)0,
                                             (char *)NULL,
                                             (int)S_CLIENT_TCP);

	       mstate = 0;
               ne->slot->writeBuffer.mutex.Release();
               schematic3D->rc = SF_SPRITE_HASBEENREQUESTED;
               }
            S_free(schFilename);
            }
         }
      }
   return 0;
   }


int
SF_Sprite::RequestFlameDownload() {

   if ( sf->engine->state != SF_CLIENT_CONNECTED )
      return 0;

   SF_NetEngine *ne = sf->engine->netEngine;
   SF_GameEngine *ge = sf->engine->gameEngine;
   SF_DisplayEngine *de = sf->engine->displayEngine;
   if ( ne && flame && flame->rc ) {
      SF_Sprite *msprite = ge->sl->Sprite(name);
      if ( msprite ) {
         char *schFilename = (char *)S_malloc(strlen(msprite->filename)+2+
                                            strlen(".bmp")+1);
         if ( schFilename ) {
            sprintf(schFilename,"%s-f.bmp",msprite->filename);
            if ( msprite->flame->rc == SF_SPRITE_HASBEENREQUESTED &&
                 _OsFileExists(schFilename) ) {
               msprite->LoadFlame();
               LoadFlame();
               de->repaint = TRUE;
               }
            if ( flame->rc && flame->rc!=SF_SPRITE_HASBEENREQUESTED ) {
               ne->slot->writeBuffer.mutex.Request();
	       mstate = 72;
               ne->slot->writeBuffer.PushMsg((uchar)SF_MSG_DOWNLOADFILE,
                                             (ushort)ge->localSlot,
                                             (char *)schFilename,
                                             (ulong)0,
                                             (char *)NULL,
                                             (int)S_CLIENT_TCP);
	       mstate = 0;
               ne->slot->writeBuffer.mutex.Release();
               flame->rc = SF_SPRITE_HASBEENREQUESTED;
               }
            S_free(schFilename);
            }
         }
      }
   return 0;
   }


int
SF_Sprite::RequestDownload() {

   if ( sf->engine->state != SF_CLIENT_CONNECTED )
      return 0;

   SF_NetEngine *ne = sf->engine->netEngine;
   SF_GameEngine *ge = sf->engine->gameEngine;
   SF_DisplayEngine *de = sf->engine->displayEngine;
   if ( ne && rc ) {
      SF_Sprite *msprite = ge->sl->Sprite(name);
      if ( msprite ) {
         char *filename = (char *)S_malloc(strlen(msprite->filename)+
                                         strlen(".bmp")+1);
         if ( filename ) {
            sprintf(filename,"%s.bmp",msprite->filename);
            if ( msprite->rc == SF_SPRITE_HASBEENREQUESTED &&
                 _OsFileExists(filename) ) {
               msprite->Reload();
               Reload();
               de->repaint = TRUE;
               }
            if ( rc && rc != SF_SPRITE_HASBEENREQUESTED ) {
               char tmp[128];
               sprintf(tmp,"Downloading %s from server . . .",filename);
               SetScreenMessage(tmp,RGB_WHITE);
               ne->slot->writeBuffer.mutex.Request();
	       mstate = 73;
               ne->slot->writeBuffer.PushMsg((uchar)SF_MSG_DOWNLOADFILE,
                                             (ushort)ge->localSlot,
                                             (char *)filename,
                                             (ulong)0,
                                             (char *)NULL,
                                             (int)S_CLIENT_TCP);
	       mstate = 0;
               ne->slot->writeBuffer.mutex.Release();
               if ( msprite )
                  msprite->rc = SF_SPRITE_HASBEENREQUESTED;
               rc = SF_SPRITE_HASBEENREQUESTED;
               }
            S_free(filename);
            }
         }
      }
   return 0;
   }



int
SF_Sprite::ShipBlit(S_CanvasBuffer *target,int angle, int x,int y,ulong flags,
                    int useFlame) {

  /*
  if ( sf->engine->state == SF_CLIENT_CONNECTED ) {
     if ( rc )
        RequestDownload();
     if ( useFlame && flame && flame->rc )
        RequestFlameDownload();
     if ( schematic && schematic->rc )
        RequestSchematicDownload();
     if ( schematic3D && schematic3D->rc )
        RequestSchematic3DDownload();
     }
     */

   if ( useFlame && flame && !flame->rc ) {
      flame->Blit(target,angle,x,y,flags);
      target->AddRect(x,y,x+cx,y+cy);
      }
   else if ( !rc ) {
      S_SystemBitmap::Blit(target,angle,x,y,flags);
      target->AddRect(x,y,x+cx,y+cy);
      }

   return 0;
   }


int
SF_Sprite::ShipStretchBlit(S_CanvasBuffer *target,int angle,
                           int x,int y,int icx,int icy,ulong flags,
                           int useFlame) {

  /*
  if ( sf->engine->state == SF_CLIENT_CONNECTED ) {
     if ( rc )
        RequestDownload();
     if ( useFlame && flame && flame->rc )
        RequestFlameDownload();
     if ( schematic && schematic->rc )
        RequestSchematicDownload();
     if ( schematic3D && schematic3D->rc )
        RequestSchematic3DDownload();
     }
     */

   if ( useFlame && flame && !flame->rc ) {
      flame->StretchBlit(target,angle,x,y,x+icx,y+icy,flags);
      target->AddRect(x,y,x+icx,y+icy);
      }
   else if ( !rc ) {
      S_SystemBitmap::StretchBlit(target,angle,x,y,x+icx,y+icy,flags);
      target->AddRect(x,y,x+icx,y+icy);
      }

   return 0;
   }


int
SF_Sprite::Blit(S_CanvasBuffer *target,int angle, int x,int y,ulong flags,
                int useFlame) {

   if ( rc ) {
     /*
      RequestDownload();
      */
      }
   else {
      S_SystemBitmap::Blit(target,angle,x,y,flags);
      target->AddRect(x,y,x+cx,y+cy);
      }

   return 0;
   }


int
SF_Sprite::StretchBlit(S_CanvasBuffer *target,int angle,
                       int x,int y,int icx,int icy,ulong flags) {
   if ( rc ) {
     /*
      RequestSchematic3DDownload();
      RequestSchematicDownload();
      RequestFlameDownload();
      RequestDownload();
      */
      }
   else {
      S_SystemBitmap::StretchBlit(target,angle,x,y,x+icx,y+icy,flags);
      target->AddRect(x,y,x+icx,y+icy);
      }
   return 0;
   }
