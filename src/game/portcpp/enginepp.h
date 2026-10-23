/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
class SF_Msg;
class SF_MsgBuffer;

void EngineAddAIMsgs          ( SF_MsgBuffer * );
void EnginePlusPlus           ( SF_MsgBuffer * );

char* GetDiagnosticString( slong );
void EngineResetObjectList( char * );
void EngineConstructObjectList( char *,slong );
void EnginePlusPlus( SF_Msg * );
void EngineObjectSet( slong,char * );
void EngineSetGameState(char *);
slong EngineGetGameState(char *);
short EngineConstructGameState(char *,char *); 
void EngineDestroyObjectList( void );
slong EngineQueryNumObjects( slong&, slong&, slong& );
void EngineSetNumObjects( const slong&, const slong&, const slong& );
void EngineStartFrame();
int  SendObjectTrackMsg(int);
char* Format(slong);
char* Format(char*);

/* dlh */
SF_MsgBuffer *EngineCloseFrame();
/* dlh */

void InitDir();

#define NUM_UPDATED_OBJECTS -1

#define BITMAP_INDEX      1
#define BITMAP_LINDEX     2
#define BITMAP_OFFSET     3
#define BITMAP_LOFFSET    4
#define BITMAP_SIZE       5
#define POSITION_X        6
#define POSITION_Y        7
#define POSITION_LASTX    8
#define POSITION_LASTY    9
#define OBJECT_DATA_PTR  10
#define OBJECT_SIZE      11
#define PLAYER_ACTIVE    12

#define SMID_FUEL        12
#define SMID_POWER       13

#define EVENT_KEYPRESSED        255
#define EVENT_KEYRELEASED       254
#define EVENT_MESSAGE           253
#define EVENT_MOUSE1PRESSED     252
#define EVENT_MOUSE1RELEASED    251
#define EVENT_MOUSE2PRESSED     250
#define EVENT_MOUSE2RELEASED    249
#define EVENT_MOUSE3PRESSED     248
#define EVENT_MOUSE3RELEASED    247
#define EVENT_GAMEMSG           246
#define EVENT_SETSHIPDATA       245
#define EVENT_NONE              244

#define GAMEMSG_DESTROYOBJECT      1
#define GAMEMSG_STARTMESSAGE       2
#define GAMEMSG_ENDMESSAGE         3
#define GAMEMSG_DATAMESSAGE        4
#define GAMEMSG_PING               5
#define GAMEMSG_SETANGLE           6
#define GAMEMSG_DESTROYSHIP        7
#define GAMEMSG_DESTROYPLANET      8
#define GAMEMSG_DESTROYCOLONY      9
#define GAMEMSG_DESTROYMISSILE    10
#define GAMEMSG_DOWNLOADGAMESTATE 11
