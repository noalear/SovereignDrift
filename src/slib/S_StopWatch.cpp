/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
#include "S_OS.h"
#include "S_StopWatch.h"


S_StopWatch::S_StopWatch(void)
{
  dtSec = dtSplitSec = 0;
  dtUsec = dtSplitUsec = 0;
  
  time1LowRes = 0;
  time2LowRes = 0;
  
# ifdef WIN32
  LARGE_INTEGER lhz;
  QueryPerformanceFrequency(&lhz);
  hz = lhz.LowPart;
  MHz = (double)hz/1000000.0;
# endif
# ifdef OS2
  DosTmrQueryFreq(&hz);
  MHz = (double)hz/1000000.0;
# endif
# ifdef UNIX
  hz = 100;
  MHz = 1.0;
# endif
  
  usecPerTick = 1.0/MHz;
  Start();
}


S_StopWatch::~S_StopWatch(void)
{
}


int
S_StopWatch::Start(int res)
{
# ifdef OS2
  DosTmrQueryTime(&time1);
# endif
# ifdef WIN32
  QueryPerformanceCounter(&time1);
  if ( res == S_STOPWATCH_LOWRES ) {
    time1LowRes = clock();
  }
# endif
# ifdef UNIX
  gettimeofday(&time1,&tz);
# endif
  return 0;
}


ulong 
S_StopWatch::Stop(int res)
{
  dtSplitSec = dtSec;
  dtSplitUsec = dtUsec;
# ifdef OS2
  ulong dHigh,dLow;
  DosTmrQueryTime(&time2);
  if ( time2.ulLo >= time1.ulLo )
    {
      dHigh = time2.ulHi - time1.ulHi;
      dLow = time2.ulLo - time1.ulLo;
      dtSec = dHigh*(0xFFFFFFFF/hz);
      dtUsec = (ulong)(dHigh*(0xFFFFFFFF % hz)*usecPerTick);
      dtSec += dtUsec/1000000;
      dtUsec %= 1000000;
      dtUsec += (ulong)(dLow*usecPerTick);
      dtSec += dtUsec/1000000;
      dtUsec %= 1000000;
    }
  else
    {
      dHigh = time2.ulHi - time1.ulHi - 1;
      dLow = time2.ulLo - time1.ulLo + 0xFFFFFFFF;
      dtSec = dHigh*(0xFFFFFFFF/hz);
      dtUsec = (ulong)(dHigh*(0xFFFFFFFF % hz)*usecPerTick);
      dtSec += dtUsec/1000000;
      dtUsec %= 1000000;
      dtUsec += (ulong)(dLow*usecPerTick);
      dtSec += dtUsec/1000000;
      dtUsec %= 1000000;
    }
# endif
# ifdef WIN32
  ulong dHigh,dLow;
  QueryPerformanceCounter(&time2);
  if ( time2.LowPart >= time1.LowPart )
    {
      dHigh = time2.HighPart - time1.HighPart;
      dLow = time2.LowPart - time1.LowPart;
      dtSec = dHigh*(0xFFFFFFFF/hz);
      dtUsec = (ulong)(dHigh*(0xFFFFFFFF % hz)*usecPerTick);
      dtSec += dtUsec/1000000;
      dtUsec %= 1000000;
      dtUsec += (ulong)(dLow*usecPerTick);
      dtSec += dtUsec/1000000;
      dtUsec %= 1000000;
    }
  else
    {
      dHigh = time2.HighPart - time1.HighPart - 1;
      dLow = time2.LowPart - time1.LowPart + 0xFFFFFFFF;
      dtSec = dHigh*(0xFFFFFFFF/hz);
      dtUsec = (ulong)(dHigh*(0xFFFFFFFF % hz)*usecPerTick);
      dtSec += dtUsec/1000000;
      dtUsec %= 1000000;
      dtUsec += (ulong)(dLow*usecPerTick);
      dtSec += dtUsec/1000000;
      dtUsec %= 1000000;
    }
# endif
# ifdef UNIX
  gettimeofday(&time2,&tz);
  if ( time2.tv_usec >= time1.tv_usec )
    {
      dtSec = time2.tv_sec - time1.tv_sec;
      dtUsec = time2.tv_usec - time1.tv_usec;
    }
  else
    {
      dtSec = time2.tv_sec - time1.tv_sec - 1;
      dtUsec = time2.tv_usec - time1.tv_usec + 1000000;
    }
# endif
  
# if defined(WIN32) || defined(OS2)
  if ( res == S_STOPWATCH_LOWRES )
    {
      time2LowRes = clock();
      dtSec  = (time2LowRes - time1LowRes)/CLK_TCK;
      dtUsec = (time2LowRes - time1LowRes) - dtSec*CLK_TCK;
      dtUsec *= (1000000/CLK_TCK);
    }
#  endif
  
  if ( dtSplitUsec <= dtUsec )
    {
      dtSplitUsec = dtUsec - dtSplitUsec;
      dtSplitSec = dtSec - dtSplitSec;
    }
  else
    {
      dtSplitUsec = (dtUsec + 1000000) - dtSplitUsec;
      dtSplitSec = (dtSec - 1) - dtSplitSec;
    }
  return dtSec*1000000 + dtUsec;
}
