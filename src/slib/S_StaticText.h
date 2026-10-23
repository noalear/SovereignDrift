#ifndef S_STATICTEXT
#define S_STATICTEXT

#include "S_Object.h"

class S_StaticText : public S_Object {
public:
   S_StaticText();
   S_StaticText(S_Object *,char* = NULL,int=0,int=0,int=0,int=0);
   int Create(S_Object *,char* = NULL,int=0,int=0,int=0,int=0);
   int Destroy();
   virtual ~S_StaticText();
   };

#endif
