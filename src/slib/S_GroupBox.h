#ifndef S_GROUPBOX
#define S_GROUPBOX

#include "S_Object.h"

class S_GroupBox : public S_Object {
public:
   int Init();
   S_GroupBox();
   S_GroupBox(S_Object *,char* = NULL,int=0,int=0,int=0,int=0);
   int Create(S_Object *,char* = NULL,int=0,int=0,int=0,int=0);
   int Destroy();
   ~S_GroupBox();
   };

#endif
