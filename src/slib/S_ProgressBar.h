#ifndef S_PROGRESSBAR
#define S_PROGRESSBAR

#include "S_Object.h"

class S_ProgressBar : public S_Object {
public:

   int Init();
   S_ProgressBar();
   int Create(S_Object *,int=0,int=0,int=0,int=0);
   int Destroy();
   ~S_ProgressBar();

   int SetValue(int);
   int SetRange(int,int);
   };

#endif
