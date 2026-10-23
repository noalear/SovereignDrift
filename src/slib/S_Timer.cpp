/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
#include "S_OS.h"
#include "S_Timer.h"
#include "S_Thread.h"

void
_DoNothing(void){}

int
_BaseTimerLoop(void *arg)
{
  S_BaseTimer *baseTimer = (S_BaseTimer *)arg;
  
  baseTimer->counter = 0;
  
# ifdef S_BASETIMER_DEBUG
  int i;
  baseTimer->dtUsecAve = 0;
  for ( i=0; i<S_BASETIMER_NUMSAMPLES; i++ )
    baseTimer->dtUsecMeas[i] = 0;
# endif
  
  while (baseTimer->continueRunning) {
    
    baseTimer->counter++;
    
#   ifdef S_BASETIMER_DEBUG
    S_StopWatch sw;
    sw.Stop();
    sw.Start();
    i = baseTimer->counter % S_BASETIMER_NUMSAMPLES;
    baseTimer->dtUsecAve -= baseTimer->dtUsecMeas[i];
    baseTimer->dtUsecMeas[i] = sw.dtUsec;
    baseTimer->dtUsecAve += baseTimer->dtUsecMeas[i];
    if ( i == 0 )
      printf("%d\n",baseTimer->dtUsecAve/S_BASETIMER_NUMSAMPLES);
#    endif
    
    _OsSleep(baseTimer->dtUsec);
    baseTimer->event.Set();
  }
  baseTimer->event.Set();
  return 0;
}


S_BaseTimer::S_BaseTimer(int idtUsec)
{
  dtUsec = idtUsec;
  continueRunning = TRUE;
  event.Reset();
  timerLoop = new S_Thread(_BaseTimerLoop,(void *)this);
  timerLoop->SetPriority(S_THREAD_HIGHESTPRIORITY);
  event.WaitFor(S_EVENT_INFINITEWAIT);
}


S_BaseTimer::~S_BaseTimer()
{
  continueRunning = FALSE;
  event.WaitFor();
  timerLoop->WaitFor();
  delete timerLoop;
}


int _timerCount = 0;
S_BaseTimer *_baseTimer;
S_Timer::S_Timer(ulong idtUsec,int ires)
{
  dtUsec = idtUsec;
  res = ires;
  stopWatch.Start(res);
  if ( _timerCount == 0 )
    _baseTimer = new S_BaseTimer();
  _timerCount++;
  numCountsOfLastWait = 0;
  countAtLastWait = _baseTimer->counter;
  Set(dtUsec);
}


S_Timer::~S_Timer(void)
{
  _timerCount--;
  if ( _timerCount == 0 )
    if ( _baseTimer )
      {
	delete _baseTimer;
	_baseTimer = NULL;
      }
}


int
S_Timer::Set(ulong idtUsec)
{
  dtUsec = idtUsec;
  stopWatch.Stop(res);
  tWaitUsec = stopWatch.dtUsec + dtUsec - _baseTimer->dtUsec;
  tWaitSec  = stopWatch.dtSec + tWaitUsec/1000000;
  tWaitUsec %= 1000000;
  numCountsOfLastWait = 0;
  return 0;
}


int
S_Timer::WaitFor()
{
  ulong keepWaiting = TRUE;
  ulong numCountsWaited = 0;
  do
    {
      _baseTimer->event.WaitFor(dtUsec);
      _baseTimer->event.Reset();
      stopWatch.Stop(res);
      
      /*
      if ( stopWatch.dtSplitSec < 0 )
	{
	  tWaitSec = stopWatch.dtSec;
	  tWaitUsec = stopWatch.dtUsec;
	}
      */
      
      keepWaiting = 
	stopWatch.dtSec < tWaitSec ||
	( stopWatch.dtSec == tWaitSec && 
	  stopWatch.dtUsec < tWaitUsec );
      
      numCountsWaited = _baseTimer->counter - countAtLastWait;
      if ( numCountsOfLastWait > 0 &&
           numCountsWaited > numCountsOfLastWait + 1 )
	{
          keepWaiting = FALSE;
	}
      
    } while ( keepWaiting );
  
  tWaitUsec += dtUsec;
  tWaitSec += tWaitUsec/1000000;
  tWaitUsec %= 1000000;
  
  numCountsOfLastWait = numCountsWaited;
  countAtLastWait = _baseTimer->counter;
  
  return 0;
}


int
S_Timer::Snooze(ulong usec)
{
  Set(usec);
  WaitFor();
  return 0;
}
