/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
#ifndef SF_SPRITE_INCLUDED
#define SF_SPRITE_INCLUDED

#define SF_SPRITE_HASBEENREQUESTED -100

class SF_Sprite : public S_SystemBitmap
{
 public:
  
  ulong check;
  
  char *name;
  char *filename;
  
  int RequestDownload();
  
  int RequestSchematicDownload();
  int LoadSchematic();
  S_SystemBitmap *schematic;
  
  int RequestSchematic3DDownload();
  int LoadSchematic3D();
  S_SystemBitmap *schematic3D;
  
  int RequestFlameDownload();
  int LoadFlame();
  S_SystemBitmap *flame;
  
  int IsACOnly(char *);
  
  int Reload();
  
  SF_Sprite(char *,char *,slong,slong,slong,slong,ulong);
  ~SF_Sprite();
  
  int ShipBlit(S_CanvasBuffer *,int,int,int,ulong=0,int=FALSE);
  int Blit(S_CanvasBuffer *,int,int,int,ulong=0,int=FALSE);
  int StretchBlit(S_CanvasBuffer *,int,int,int,int,int,ulong=0);
  int ShipStretchBlit(S_CanvasBuffer *,int,int,int,int,int,ulong=0,int=FALSE);
};

#endif
