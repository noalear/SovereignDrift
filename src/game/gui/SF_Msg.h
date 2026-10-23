/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
#ifndef SF_MSG_INCLUDED
#define SF_MSG_INCLUDED

class SF_Msg {
public:
   slong msg;
   slong slot;
   slong frame;
   slong scancode;
   slong frameRate;
   slong angle;
   slong bitmap;
   slong vx;
   slong vy;
   slong x;
   slong y;
   slong from;
   slong to;
   slong mode;
   slong id;
   slong type;
   slong bonus;
   slong credit;
   slong itime;
   slong index;
   slong mask;
   slong tslot;
   slong pop1;
   slong pop2;
   slong mp1;
   slong mp2;
   slong ai1;
   slong ai2;
   slong ai3;
   slong ai4;
   slong ai5;
   slong ai6;
   slong port;
   slong dslot;
   slong bslot;
   slong flag;
   slong race;
   slong bSize;
   slong crew;
   S_String str;
   SF_Player player;
   char *buffer;

   int Reset();

   SF_Msg();
   ~SF_Msg();
   };

#endif
