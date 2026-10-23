/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
#include "S_OS.h"
#include "S_Mutex.h"

S_Mutex::S_Mutex(void)
{
# ifdef WIN32
  handle = CreateMutex(NULL,FALSE,NULL);
# endif
# ifdef OS2
  DosCreateMutexSem(NULL,&handle,0,FALSE);
# endif
# ifdef PTHREAD
  pthread_mutex_init(&handle,NULL);
# endif
}


S_Mutex::~S_Mutex(void)
{
# ifdef WIN32
  CloseHandle(handle);
# endif
# ifdef OS2
  DosCloseMutexSem(handle);
# endif
# ifdef PTHREAD
  pthread_mutex_destroy(&handle);
# endif
}


int
S_Mutex::Request(ulong timeOut)
{
# ifdef WIN32
  if ( timeOut == S_MUTEX_INFINITEWAIT )
    WaitForSingleObject(handle,timeOut);
  else
    WaitForSingleObject(handle,timeOut/1000);
# endif
# ifdef OS2
  if ( timeOut == S_MUTEX_INFINITEWAIT )
    DosRequestMutexSem(handle,timeOut);
  else
    DosRequestMutexSem(handle,timeOut/1000);
# endif
# ifdef PTHREAD
  pthread_mutex_lock(&handle);
# endif
  return 0;
}


int
S_Mutex::Release(void)
{
# ifdef WIN32
  ReleaseMutex(handle);
# endif
# ifdef OS2
  DosReleaseMutexSem(handle);
# endif
# ifdef PTHREAD
  pthread_mutex_unlock(&handle);
# endif
  return 0;
}

