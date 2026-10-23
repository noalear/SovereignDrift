/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
#ifndef SF_GAMEENGINE_INCLUDED
#define SF_GAMEENGINE_INCLUDED

#define SF_GAMEENGINE_NUMSHIELDSPRITES 256

#define SF_GAMEENGINE_OK               0
#define SF_GAMEENGINE_GAMESTATEFAILED -1

char *ReadValueFromScenarioFile( char *,char * );
int SF_DebugMem_Check(void *,char *);

class SF_Player;
class SF_MsgBuffer;
class SF_Sprite;
class SF_SpriteList;
class SF_Engine;
class SF_DisplayEngine;
class SF_NetEngine;

class SF_GameEngine
{
 public:
  
  ulong check;
  
  char *scenarioName;
  
  int rc;
  slong numLives;
  
  ulong dt1,dt2,dt3,dt4,dt5;
  
  slong numObjects;
  slong numShips;
  slong numPlanets;
  slong numMissiles;
  char event;
  char ch;
  short s;
  slong l;
  
  ulong msgHistograph[256][256];
  
  ulong itime;
  ulong frame;
  ushort localSlot;

  int controlsDisabled;
  ulong disableInput;
  ulong disableF9;
  
  int paused;
  S_Event pauseEvent;
  S_Mutex mutex;
  S_StopWatch sw;
  S_StopWatch sw1;
  
  int boostOn[max_players];
  int impulseOn[max_players];
  
  S_SystemBitmap egg;
  SF_Sprite *shldSprites[SF_GAMEENGINE_NUMSHIELDSPRITES];
  
  int CheckObjectCPUWarning(slong,slong,slong,slong,slong);
  slong objectCPUWarning;
  slong objectCPUWarningMsg;
  slong objectCPUWarningSegment;
  slong objectCPUWarningFrame;
  slong objectCPUWarningTime;
  
  int RemapPointers();
  
  SF_SpriteList *sl;
  SF_Engine *e;
  SF_NetEngine *ne;
  SF_DisplayEngine *de;
  
  int OverlayBGSprites();
  
  SF_Sprite *mainFontSprite;
  SF_Sprite *powerupTransSprite;
  SF_Sprite *halftoneSprite;
  SF_Sprite *videoSprite;
  SF_Sprite *debrisSprite;
  SF_Sprite *bgSprite;
  SF_Sprite *bg2Sprite;
  SF_Sprite *mapSprite;
  SF_Sprite *chatSprite;
  SF_Sprite *scrollSprite;
  SF_Sprite *lpanelSprite;
  SF_Sprite *rpanelSprite;
  
  SF_MsgBuffer *msgBuffer;
  SF_MsgBuffer *retMsgBuffer;
  
  SF_GameEngine(SF_Engine *,char *);
  ~SF_GameEngine();
  
  int FilterKeystroke(char,char);
  int Update();
  
  int FillSlot(int,SF_Player *);
  int HomePlanetConquered(int);
  int SystemConqueredBy(int);
  
  int guiInitDone;
  int InitGUIClient();
};

char *CleanString(char *);

#endif
