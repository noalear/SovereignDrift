/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
#include "S_OS.h"
#include "S_Event.h"

S_Event::S_Event(void)
{
# ifdef WIN32
  handle = CreateEvent(NULL,TRUE,FALSE,NULL);
# endif
# ifdef OS2
  DosCreateEventSem(NULL,&handle,0,FALSE);
  postCount = 0;
# endif
# ifdef PTHREAD
  pthread_cond_init(&handle,NULL);
  pthread_mutex_init(&hMutex,NULL);
  state = S_EVENT_UNBLOCKED;
# endif
}


S_Event::~S_Event(void)
{
# ifdef WIN32
  CloseHandle(handle);
# endif
# ifdef OS2
  DosCloseEventSem(handle);
# endif
# ifdef PTHREAD
  pthread_cond_destroy(&handle);
  pthread_mutex_destroy(&hMutex);
# endif
}


int
S_Event::WaitFor(ulong timeOut)
{
# ifdef WIN32
  if ( timeOut == S_EVENT_INFINITEWAIT )
    {
      WaitForSingleObject(handle,timeOut);
    }
  else
    {
      WaitForSingleObject(handle,timeOut/1000);
    }
# endif
# ifdef OS2
  if ( timeOut == S_EVENT_INFINITEWAIT )
    {
      DosWaitEventSem(handle,timeOut);
    }
  else
    {
      DosWaitEventSem(handle,timeOut/1000);
    }
# endif
# ifdef PTHREAD
  struct timezone tz;
  struct timeval now;
  struct timespec tOut;
  gettimeofday(&now,&tz);
  tOut.tv_nsec = now.tv_usec + timeOut;
  tOut.tv_sec = now.tv_sec + tOut.tv_nsec/1000000;
  tOut.tv_nsec = (tOut.tv_nsec%1000000)*1000;
  pthread_mutex_lock(&hMutex);
  if ( state == S_EVENT_BLOCKED )
    {
      pthread_cond_timedwait(&handle,&hMutex,&tOut);
    }
  pthread_mutex_unlock(&hMutex);
# endif
  return 0;
}


int
S_Event::Reset(void)
{
# ifdef WIN32
  ResetEvent(handle);
# endif
# ifdef OS2
  DosResetEventSem(handle,&postCount);
# endif
# ifdef PTHREAD
  state = S_EVENT_BLOCKED;
# endif
  return 0;
}


int
S_Event::Set(void)
{
# ifdef WIN32
  SetEvent(handle);
# endif
# ifdef OS2
  DosPostEventSem(handle);
# endif
# ifdef PTHREAD
  pthread_cond_broadcast(&handle);
  state = S_EVENT_UNBLOCKED;
  pthread_mutex_unlock(&hMutex);
# endif
  return 0;
}
