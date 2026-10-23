/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
#ifndef S_STOPWATCH
#define S_STOPWATCH

#define S_STOPWATCH_HIGHRES 0
#define S_STOPWATCH_LOWRES  1

class S_StopWatch
{
 public:
  ulong time1LowRes;
  ulong time2LowRes;
  
# ifdef WIN32
  LARGE_INTEGER time1;
  LARGE_INTEGER time2;
# endif
# ifdef OS2
  QWORD time1;
  QWORD time2;
# endif
# ifdef UNIX
  struct timeval time1;
  struct timeval time2;
  struct timezone tz;
# endif
  
  ulong dtSec;
  ulong dtUsec;
  ulong dtSplitSec;
  ulong dtSplitUsec;
  ulong hz;
  double MHz;
  double usecPerTick;
  
  S_StopWatch(void);
  ~S_StopWatch(void);
  
  int Start(int=S_STOPWATCH_HIGHRES);
  ulong Stop(int=S_STOPWATCH_HIGHRES);
};

#endif
