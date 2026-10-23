/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
#ifndef S_TIMER
#define S_TIMER

#define S_BASETIMER_NODEBUG
#define S_BASETIMER_NUMSAMPLES 100

#include "S_StopWatch.h"
#include "S_Event.h"
class S_Thread;

class S_BaseTimer
{
 public:
  ulong dtUsec;
  int continueRunning;
  
  S_Event event;
  S_Thread *timerLoop;
  
  S_BaseTimer(int=1000);
  ~S_BaseTimer(void);
  
  int counter;
  
# ifdef S_BASETIMER_DEBUG
  ulong dtUsecAve;
  ulong dtUsecMeas[S_BASETIMER_NUMSAMPLES];
# endif
  
};


class S_Timer
{
 public:
  ulong numCountsOfLastWait;
  ulong countAtLastWait;
  
  ulong dtUsec;
  ulong tWaitSec;
  ulong tWaitUsec;
  
  int res;
  
  S_StopWatch stopWatch;
  
  S_Timer(ulong=0,int=S_STOPWATCH_HIGHRES);
  ~S_Timer(void);
  
  int Set(ulong);
  int WaitFor();
  int Snooze(ulong);
};

#endif
