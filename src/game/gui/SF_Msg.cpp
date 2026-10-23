#include "sf.h"
#include "SF_Player.h"
#include "SF_Msg.h"

SF_Msg::SF_Msg() {
   buffer = NULL;
   Reset();
   }

SF_Msg::~SF_Msg() {
   }


int
SF_Msg::Reset() {
   msg = -1;
   slot = -1;
   frame = -1;
   scancode = -1;
   frameRate = -1;
   angle = -1;
   bitmap = -1;
   vx = -1;
   vy = -1;
   x = -1;
   y = -1;
   from = -1;
   to = -1;
   mode = -1;
   id = -1;
   type = -1;
   bonus = -1;
   credit = -1;
   itime = -1;
   index = -1;
   mask = -1;
   tslot = -1;
   pop1 = -1;
   pop2 = -1;
   mp1 = -1;
   mp2 = -1;
   ai1 = -1;
   ai2 = -1;
   ai3 = -1;
   ai4 = -1;
   ai5 = -1;
   ai6 = -1;
   port = -1;
   dslot = -1;
   bslot = -1;
   flag = -1;
   race = -1;
   bSize = -1;
   crew = -1;
   if ( buffer ) {
      S_free(buffer);
      buffer = NULL;
      }
   str.Destroy();
   player.Destroy();

   return 0;
   }
