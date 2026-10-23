/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
#ifndef SF_MSGBUFFER_INCLUDED
#define SF_MSGBUFFER_INCLUDED

#define SF_MSG_OK 0

/* ************************************************************************* */
/* ************************ KeyStroke Event Msgs *************************** */
/* ************************************************************************* */
/* uchar msg      = SF_MSG_KEYPRESSED_4 or SF_MSG_KEYRELEASED_4
   ushort slot    = player slot index
   uchar frame    = client frame % 256
   uchar scancode = keyboard scancode */

#define SF_MSG_KEYPRESSED_4  255
#define SF_MSG_KEYRELEASED_4 254
/* ************************************************************************* */


/* ************************************************************************* */
/* **************************** None Msg *********************************** */
/* ************************************************************************* */
/* uchar msg   = SF_MSG_NONE_2
   ushort slot = player slot index */

#define SF_MSG_NONE 0
/* ************************************************************************* */


/* ************************************************************************* */
/* **************************** Ping Msg *********************************** */
/* ************************************************************************* */
/* uchar msg   = SF_MSG_PING_10
   ushort slot = player slot index
   ulong frame = server frame 
   ulong frame = client itime */

#define SF_MSG_PING_10        1
/* ************************************************************************* */


/* ************************************************************************* */
/* ************************* Player Info Msg ******************************* */
/* ************************************************************************* */
/* uchar msg   = SF_MSG_PLAYERINFO
   ushort slot = player slot index
   SF_Player * = player class data (server echos back)*/

#define SF_MSG_PLAYERINFO 2
/* ************************************************************************* */

/* ************************************************************************* */
/* ************************ Gamestate msg ********************************** */
/* ************************************************************************* */
/* uchar msg   = SF_MSG_GAMESTATE
   ushort slot = player slot index
   char * = gamestate data (filled in by server,client passes NULL)*/

#define SF_MSG_GAMESTATE 3
/* ************************************************************************* */


/* ************************************************************************* */
/* ************************** Disconnect msg ******************************* */
/* ************************************************************************* */
/* uchar msg      = SF_MSG_DISCONNECT
   uchar short    = player slot index */

#define SF_MSG_DISCONNECT 4
/* ************************************************************************* */

/* ************************************************************************* */
/* ************************** Framerate msg ******************************** */
/* ************************************************************************* */
/* uchar msg       = SF_MSG_FRAMERATE
   ushort slot     = player slot index
   uchar frameRate = frameRate (filled in by server,client passes 0)*/

#define SF_MSG_FRAMERATE 5
/* ************************************************************************* */

/* ************************************************************************* */
/* ************************* Object bearing info msg *********************** */
/* ************************************************************************* */
/* uchar msg       = SF_MSG_OBJECTTRACK_17
   ushort slot     = player slot index
   uchar frame     = client frame % 256
   uchar angle     = Object angle or offset
   uchar bitmap    = Object bitmap index
   short vx        = X velocity
   short vy        = Y velocity
   long x          = X position
   long y          = Y position */

#define SF_MSG_OBJECTTRACK_17 6
/* ************************************************************************* */


/* ************************************************************************* */
/* ********************************** Chat Msgs **************************** */
/* ************************************************************************* */
/* uchar msg    = SF_MSG_PLAYERCHAT_N
   ushort slot  = player slot index
   uchar mode   = SF_MSG_CHAT_TOALL, SF_MSG_CHAT_TORACE, SF_MSG_CHAR_TOPLAYER
   uchar to     = raceIndex or playerIndex
   char *msg    = msg string */

#define SF_MSG_PLAYERCHAT_N 7

#define SF_MSG_CHAT_TOALL    0
#define SF_MSG_CHAT_TORACE   1
#define SF_MSG_CHAT_TOPLAYER 2
/* ************************************************************************* */


/* ************************************************************************* */
/* ********************************* Mouse Msgs **************************** */
/* ************************************************************************* */
/* uchar msg      = SF_MSG_MOUSEBUTTONDOWN_12,SF_MSG_MOUSEBUTTONUP_12
   ushort slot    = player slot index
   uchar frame    = client frame % 256
   uchar scancode = which button 1,2, or 3
   long  x        = actual game x position
   long  y        = actual game y position */

#define SF_MSG_MOUSEBUTTONDOWN_12 8
#define SF_MSG_MOUSEBUTTONUP_12   9
#define SF_MSG_REL_MOUSEBUTTONDOWN_12 10
#define SF_MSG_REL_MOUSEBUTTONUP_12   11
/* ************************************************************************* */

/* ************************************************************************* */
/* ****************************** Explode Ship Msg ************************* */
/* ************************************************************************* */
/* uchar msg    = SF_MSG_EXPLODEME_2,SF_MSG_EXPLODESHIP,SF_MSG_EXPLODECOLONY
   ushort slot  = player slot index = ship index
   ushort dslot = other ship index */

#define SF_MSG_EXPLODEME     12
#define SF_MSG_EXPLODESHIP   13
#define SF_MSG_EXPLODECOLONY 14
/* ************************************************************************* */

/* ************************************************************************* */
/* **************************** Interact msgs ****************************** */
/* ************************************************************************* */
/* uchar msg    = 
   ushort slot  = player slot index = ship index
   ushort tslot = target slot index = ship index */

#define SF_MSG_SETTARGET        16
#define SF_MSG_SETDUTY          17
#define SF_MSG_SEEKTARGETLEFT   18
#define SF_MSG_SEEKTARGETRIGHT  19
#define SF_MSG_SEEKOBJECT       20
#define SF_MSG_SEEKSHIP         21
#define SF_MSG_SEEKCOLONY       22
#define SF_MSG_SEEKPLANET       23
#define SF_MSG_HELP             24
#define SF_MSG_ATTACK           25
#define SF_MSG_SETNAVTARGET     26
/* ************************************************************************* */

/* ************************************************************************* */
/* **************************** Seek Position ****************************** */
/* ************************************************************************* */
/* uchar msg    = 
   ushort slot  = player slot index = ship index
   uchar frame  = client frame % 256
   long x       = x position to seek
   long y       = y position to seek */

#define SF_MSG_SEEKPOSITION             27
/* ************************************************************************* */

/* ************************************************************************* */
/* **************************** Fire Weapons ******************************* */
/* ************************************************************************* */
/* uchar msg    = 
   ushort slot  = player slot index = ship index
   uchar frame  = client frame % 256
   uchar angle  = 
   uchar mask   = tube mask 
   short vx     = 
   short vy     = 
   long x       = 
   long y       = */

#define SF_MSG_FIRETUBES             28
/* ************************************************************************* */


/* ************************************************************************* */
/* ************************** Set Ship/Race Type *************************** */
/* ************************************************************************* */
/* uchar msg    = 
   ushort slot = player slot index = ship index
   ushort dslot = ship index of who's refitting it
   ushort index  = ship index
   uchar flag = build flag
   uchar port = port it is attached to
   */

#define SF_MSG_SETSHIPINDEX          29
/* ************************************************************************* */

/* ************************************************************************* */
/* ************************** Set Ship/Race Type *************************** */
/* ************************************************************************* */
/* uchar msg    = 
   ushort slot = player slot index = ship index
   uchar index  = ship/race index */

#define SF_MSG_SETRACEINDEX          30
#define SF_MSG_SETRANKINDEX          31
/* ************************************************************************* */


/* ************************************************************************* */
/* ************************** Set Ship/Race Type *************************** */
/* ************************************************************************* */
/* uchar msg    = 
   ushort slot  = player slot index = ship index
   long id      = bus slot id */

#define SF_MSG_SETBUSSLOT            32
/* ************************************************************************* */


/* ************************************************************************* */
/* ************************** Spawn a colony ******************************* */
/* ************************************************************************* */
/* uchar msg    = SF_MSG_SPAWNCOLONY
   ushort slot  = objectList index
   ushort dslot = slot of object that will create colony
   ushort bslot = slot of new colony
   ushort race  = race of new colony
   ulong pop1   = 
   ulong pop2   = */

#define SF_MSG_SPAWNCOLONY          33
/* ************************************************************************* */

/* ************************************************************************* */
/* *************************** Cloaking msgs ******************************* */
/* ************************************************************************* */
/* uchar msg = SF_MSG_UNCLOAK
   ushort slot = objectList index */

#define SF_MSG_UNCLOAK       34


/* uchar msg = SF_MSG_CLOAK
   ushort slot = objectList index
   long mp1 = module param 1
   long mp2 = module param 2 */

#define SF_MSG_CLOAK         35
/* ************************************************************************* */


/* ************************************************************************* */
/* ***************************** Docking msgs ****************************** */
/* ************************************************************************* */
/* uchar msg = SF_MSG_ISDOCKED
   ushort slot = objectList index
   ushort dslot = objectListIndex of object with slot
   uchar port = port number */

#define SF_MSG_ISDOCKED   36


/* uchar msg = SF_MSG_DEACTIVATEDOCK
   ushort slot = objectListIndex
   uchar port = port number */

#define SF_MSG_DEACTIVATEDOCK 37 


/* uchar msg = SF_MSG_UNDOCK
   ushort slot = slot of ship who owns dock and sends msgs
   ushort dslot = slot of ship that wants to detach */

#define SF_MSG_UNDOCK     38


/* uchar msg = SF_MSG_DOCK
   ushort slot = objectList index
   ushort dslot = objectListIndex of object with slot
   uchar port = port num
   ulong mp1 = module param 1
   ulong mp2 = module param 2 */

#define SF_MSG_DOCK       39
/* ************************************************************************* */


/* ************************************************************************* */
/* *************************** Build a ship ******************************** */
/* ************************************************************************* */
/* uchar msg = SF_MSG_FILLSLOT
   ushort slot = player slot
   ushort dslot = slot of object that has factory
   ushort index = player index
   ushort race = default race
   ushort flag = immediate build flag
   ushort port */

#define SF_MSG_FILLSLOT 40
/* ************************************************************************* */

/* ************************************************************************* */
/* *************************** Powerups ************************************ */
/* ************************************************************************* */
/* uchar msg = SF_MSG_GETPOWERUP
   ushort slot = player slot
   ushort bslot = bus slot
   long id = module id 
   long id = module id */

#define SF_MSG_GETPOWERUP 41
/* ************************************************************************* */

/* ************************************************************************* */
/* ************************* AI Object bearing info msg ******************** */
/* ************************************************************************* */
/* uchar msg       = SF_MSG_AIOBJECTTRACK_17
   ushort slot     = player slot index
   uchar frame     = client frame % 256
   uchar angle     = Object angle or offset
   uchar bitmap    = Object bitmap index
   short vx        = X velocity
   short vy        = Y velocity
   long x          = X position
   long y          = Y position
   uchar ai1       = AI var 1
   uchar ai2       = AI var 2
   uchar ai3       = AI var 3
   uchar ai4       = AI var 4
   ushort dslot    = AI Target */

#define SF_MSG_AIOBJECTTRACK_17 42
/* ************************************************************************* */


/* ************************************************************************* */
/* ******************************** Warp Msgs ****************************** */
/* ************************************************************************* */
/* uchar msg       = SF_MSG_INITIATEWARP
   ushort slot     = player slot index
   ushort dslot    = AI Target
   uchar ai1       = AI Param 1 */

#define SF_MSG_INITIATEWARP 43

/* uchar msg       = SF_MSG_TERMINATEWARP
   ushort slot     = player slot index 
   ushort dslot    = AI Target
   uchar ai1       = AI Param 1
   long x          = x position
   long y          = y position */

#define SF_MSG_TERMINATEWARP 44
/* ************************************************************************* */

/* ************************************************************************* */
/* **************************** RESETGAME Msg ****************************** */
/* ************************************************************************* */
/* uchar msg   = SF_MSG_RESETGAME
   ushort slot = player slot index
   ulong race  = race index */

#define SF_MSG_RESETGAME     45
/* ************************************************************************* */

/* ************************************************************************* */
/* uchar msg   = SF_MSG_COMPLETEBUILD
   ushort slot = player slot index
   ushort dslot = build object slot index */

#define SF_MSG_COMPLETEBUILD 46
/* ************************************************************************* */

/* ************************************************************************* */
/* uchar msg   = SF_MSG_CAPTURE
   ushort slot = player slot index
   ushort dslot = build object slot index
   uchar index = race index */

#define SF_MSG_CAPTURE 47
/* ************************************************************************* */

/* ************************************************************************* */
/* **************************** Ping Msg *********************************** */
/* ************************************************************************* */
/* uchar msg   = SF_MSG_PING_10
   ushort slot = player slot index
   ulong frame = server frame 
   ulong frame = client itime */

#define SF_MSG_PINGUDP 48
#define SF_MSG_PINGTCP 49
/* ************************************************************************* */

/* ************************************************************************* */
/* *********************** Remove colony Msg ******************************* */
/* ************************************************************************* */
/* uchar msg   = SF_MSG_REMOVECOLONY
   ushort slot = player slot index */

#define SF_MSG_REMOVECOLONY 50
/* ************************************************************************* */

/* ************************************************************************* */
/* *********************** Download file msg ******************************* */
/* ************************************************************************* */
/* uchar msg = SF_MSG_DOWNLOADFILE
   ushort slot = player slot index
   char *filename = file name
   char *buffer = file data */

#define SF_MSG_DOWNLOADFILE 51
/* ************************************************************************* */



/* ************************************************************************* */
/* ****************** Request object population msg ************************ */
/* ************************************************************************* */
/* uchar msg      = SF_MSG_REQUESTPOPULATION
   ushort slot    = object index */

#define SF_MSG_REQUESTPOPULATION 55
/* ************************************************************************* */


/* ************************************************************************* */
/* ******************** Set object population msg ************************** */
/* ************************************************************************* */
/* uchar msg      = SF_MSG_SETPOPULATION
   ushort slot    = object index
   ulong pop1     = pop1
   ulong pop2     = pop2
   uchar ai1      = power damage
   uchar ai2      = power storage
   uchar ai3      = shield damage
   uchar ai4      = shield storage
   uchar ai5      = engine damage
   uchar ai6      = engine storage
   long x         = position
   long y         = */

#define SF_MSG_SETPOPULATION 56
/* ************************************************************************* */

/* ************************************************************************* */
/* *************************** Set Player Score **************************** */
/* ************************************************************************* */
/* uchar msg      = SF_MSG_ADDTOPLAYERSCORE
   ushort slot    = sender
   ushort dslot   = receiver
   ulong bonus    = increment
   ulong credit   = Some kind of credit */

#define SF_MSG_ADDTOPLAYERSCORE 57
/* ************************************************************************* */

/* ************************************************************************* */
/* ******************** Set Player Module Pointer ************************** */
/* ************************************************************************* */
/* uchar msg      = SF_MSG_SETMODULEPOINTER
   ushort slot    = sending object
   ushort index   = module index */

#define SF_MSG_SETMODULEPOINTER 58
/* ************************************************************************* */

/* ************************************************************************* */
/* ***************************** Docking msgs ****************************** */
/* ************************************************************************* */
/* uchar msg = SF_MSG_HUMANISDOCKED
   ushort slot = objectList index
   ushort dslot = objectListIndex of object with slot
   uchar port = port number */

#define SF_MSG_HUMANISDOCKED   59
/* ************************************************************************* */


/* ************************************************************************* */
/* uchar msg   = SF_MSG_HUMANCAPTURE
   ushort slot = player slot index
   ushort dslot = build object slot index
   uchar index = race index */

#define SF_MSG_HUMANCAPTURE 60
/* ************************************************************************* */


/* ************************************************************************* */
/* uchar msg = SF_MSG_PLANETSIEGESUCCEEDS
   ushort slot = player slot index
   ulong race = race index of seigers
   */
#define SF_MSG_PLANETSIEGESUCCEEDS 61
/* ************************************************************************* */

/* ************************************************************************* */
/* ************************* Updaterate msg ******************************** */
/* ************************************************************************* */
/* uchar msg       = SF_MSG_UPDATERATE
   ushort slot     = player slot index
   uchar updateRate = updateRate (filled in by server,client passes 0)*/

#define SF_MSG_UPDATERATE 62
/* ************************************************************************* */

/* ************************************************************************* */
/* ************************* RequestDev msg ******************************** */
/* ************************************************************************* */
/* uchar msg       = SF_MSG_REQUESTDEV
   ushort slot     = player slot index
   uchar updateRate = isdev (filled in by server,client passes 0)*/

#define SF_MSG_REQUESTDEV 63
/* ************************************************************************* */


class SF_Player;
class SF_Msg;

class SF_MsgBuffer {
public:
   S_Mutex mutex;

   S_Packet udpPacket;
   S_Packet tcpPacket;

   ulong tcpBytesRcvd;
   ulong tcpBytesSent;
   ulong udpBytesRcvd;
   ulong udpBytesSent;

   SF_MsgBuffer(void);
   ~SF_MsgBuffer(void);

   int ShrinkBuffers();
   int Rewind(void);
   int Flush(int=S_PACKET_FLUSHALL);
   int SeekEnd();
   int Merge(SF_MsgBuffer *);
   int IsEmpty();

   int FilterMsg(ushort,uchar,int);

   int PushMsg(uchar,ushort,int);
   int PushMsg(uchar,ushort,char *,int);
   int PushMsg(uchar,ushort,uchar,int);
   int PushMsg(uchar,ushort,ushort,int);
   int PushMsg(uchar,ushort,uchar,uchar,int);
   int PushMsg(uchar,ushort,ushort,uchar,int);
   int PushMsg(uchar,ushort,ushort,uchar,slong,slong,int);
   int PushMsg(uchar,ushort,ushort,ulong,int);
   int PushMsg(uchar,ushort,slong,slong,int);
   int PushMsg(uchar,ushort,slong,slong,uchar,uchar,uchar,uchar,uchar,uchar,slong,slong,int);
   int PushMsg(uchar,ushort,ushort,slong,slong,int);
   int PushMsg(uchar,ushort,ushort,ushort,ushort,ushort,ushort,int);
   int PushMsg(uchar,ushort,uchar,ushort,int);
   int PushMsg(uchar,ushort,char *,ulong,char *,int);
   int PushMsg(uchar,ushort,uchar,uchar,char *,int);
   int PushMsg(uchar,ushort,ulong,char *,int);
   int PushMsg(uchar,ushort,SF_Player *,int);
   int PushMsg(uchar,ushort,slong,int);
   int PushMsg(uchar,ushort,uchar,slong,int);
   int PushMsg(uchar,ushort,ushort,ushort,uchar,uchar,int);
   int PushMsg(uchar,ushort,uchar,slong,slong,int);
   int PushMsg(uchar,ushort,uchar,uchar,slong,slong,int);
   int PushMsg(uchar,ushort,ushort,uchar,ulong,ulong,int);
   int PushMsg(uchar,ushort,ushort,ushort,ushort,ulong,ulong,int);
   int PushMsg(uchar,ushort,uchar,uchar,uchar,short,short,slong,slong,int);
   int PushMsg(uchar,ushort,uchar,uchar,uchar,short,short,slong,slong,ushort,int);
   int PushMsg(uchar,ushort,uchar,uchar,uchar,short,short,slong,slong,
           uchar,uchar,uchar,uchar,ushort,int);
   int PushMsg(uchar,ushort,uchar,uchar,uchar,short,short,slong,slong,ushort,
           uchar,uchar,uchar,uchar,ushort,int);
   int PushMsg(uchar,ushort,uchar,uchar,uchar,short,short,slong,slong,ulong,ulong,
           uchar,uchar,uchar,uchar,ushort,int);

   int GetNextMsg(slong,SF_Msg *,int,slong = -1,int = TRUE);

   int SendTo(S_Client *,int);
   int RecvFrom(S_Client *,int=S_CLIENT_NONBLOCK);
   };

#endif
