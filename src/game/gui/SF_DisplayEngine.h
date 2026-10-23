/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
#ifndef SF_DISPLAYENGINE_INCLUDED
#define SF_DISPLAYENGINE_INCLUDED

#define MAXOBJECTS 1024

void SetScreenMessage(char *msg,int color);
void SetScreenBottomMessage(char *msg,int color);
void SetShipLag(int,int);
int  GetShipLag(int);

class SF_GameEngine;
class SF_NetEngine;
class SF_Engine;
class SF_MapConsole;
class SF_ChatConsole;
class SF_VideoConsole;
class SF_Sprite;
class SF_Client;
class SF_TextBuffer;
class GenericClass;
class SF_Button;
class ShipClass;
class DesignedClass;
class SF_Font;

#define SF_DISPLAYENGINE_NUMBUTTONS 22

class SF_DisplayEngine {
public:

  ulong check;

   int show;
   int repaint;
   int firstTime;
   int waitForLogin;
   int displaySkip;
   int showDebugString;
   int redrawButtons;

   ulong condition;

   int onScreen[MAXOBJECTS];

   int tacticalZoom;
   int tacticalScale;
   int strategicZoom;
   int strategicScale;

   SF_Client *c;
   S_CanvasBuffer *bb;

   S_Window *window;
   SF_GameEngine *ge;
   SF_NetEngine *ne;
   SF_Engine *e;
   SF_MapConsole *map;
   SF_ChatConsole *chat;
   SF_VideoConsole *video;
   S_CanvasBuffer *halftoneBuffer;
   SF_TextBuffer *scrTopMsgBuffer;
   SF_TextBuffer *scrBotMsgBuffer;
   SF_Font *mainFont;

   S_StopWatch sw;

   SF_DisplayEngine(SF_Engine *);
   ~SF_DisplayEngine();

   int ClearBackBuffer(int);
   int Update();

   slong scrX0,scrY0;
   slong x0,lx0;
   slong y0,ly0;

   int SetVisibilityVariables(int);
   int active;
   int visible;
   int isAWarpingShip;
   int stealth;
   int shouldBeDrawn;

   int BlitRectToWindow(int,int,int,int);
   int BlitPanelBackground(int,int,int,int);

   void DrawInternetCafe();

   int DrawMainWindowDebris();
   int numDebris;
   int *debrisDist;
   slong *xDebris;
   slong *yDebris;

   int MouseClick(int,int,int,int);

   int CreateButtons();
   int DestroyButtons();
   int DrawButtons(int=-1);

   SF_Button *button[SF_DISPLAYENGINE_NUMBUTTONS];

   int cyContextButtons;

   int DrawCondition();

   int numEnemyShipsOnScreen;
   int numEnemyShipsNearby;
   int numObjectsOnScreen;

   int DrawTarget(int);
   int DrawLocalShip(int);
   int DrawGauge(int,int,int,int,int);
   int DrawBottomPanel(int);
   int localPlayerBitmapIndex;

   ShipClass *target;
   int targetPower;
   int targetShield;
   int targetEngine;
   ulong targetScore;
   ulong targetKills;
   ulong targetCrew;
   slong targetSpeed;

   int localPower;
   int localShield;
   int localEngine;
   ulong localScore;
   ulong localKills;
   ulong localCrew;
   slong localSpeed;

   SF_Sprite *SetObjectVariables(int);
   slong gx,ngx;
   slong gy,ngy;
   slong angle;
   int bitmapIndex;
   int race;
   ulong color;
   int isSun;
   int isPlanet;
   int isMinor;
   int isShip;
   int isRobotShip;
   int isColony;
   int isTargetted;
   int isTerminal;
   int isTransitional;
   GenericClass *colonizedPlanet;
   SF_Sprite *sprite;
   int soundIndex;
   int useFlame;
   int localUseFlame;
   int localPlayImpulseSound;
   int localPlayBoostSound;

   int DrawMainScreenObject(int);
   int DrawMapConsoleObject(int);
   };

int IsInInternetCafe(GenericClass *);

#endif
