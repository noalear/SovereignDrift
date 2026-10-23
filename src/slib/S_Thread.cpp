/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
#include "S_OS.h"
#include "S_Thread.h"

S_ThreadRet S_ThreadFunc 
_S_ThreadFunc(void *ithread)
{
  S_Thread *thread = (S_Thread *)ithread;
  if ( thread )
    {
#     ifdef OS2
      HAB hab = WinInitialize(0);
      HMQ hmq = WinCreateMsgQueue (hab,0);
#     endif
      if ( thread->func )
	thread->func(thread->arg);
      else
	thread->Function();
#     ifdef OS2
      WinDestroyMsgQueue (hmq);
      WinTerminate (hab);
#     endif
    }
  if ( thread && thread->check == 0xAAAAAAAA )
    {
      thread->running = FALSE;
    }
  if ( thread && thread->check == 0xAAAAAAAA && thread->selfDelete == TRUE )
    {
      delete thread;
    }
  S_ExitThread();
# ifdef PTHREAD
  return 0;
# endif
}


int
S_Thread::Init()
{
  func = NULL;
  arg = NULL;
  tid = 0;
  handle = 0;
  running = FALSE;
  selfDelete = FALSE;
  check = 0xAAAAAAAA;
  rc = S_THREAD_OK;
  return 0;
}

S_Thread::S_Thread()
{
  Init();
}


S_Thread::S_Thread( int (*ifunc)(void *),
		    void *iarg,
                    ulong stackSize)
{
  Init();
  Start(ifunc,iarg,stackSize);
}


int
S_Thread::Destroy()
{
# ifdef WIN32
  if ( handle )
    {
      CloseHandle(handle);
    }
# endif
  Init();
  return 0;
}


S_Thread::~S_Thread(void)
{
  Destroy();
}


int
S_Thread::Function()
{
  return 0;
}


int
S_Thread::Start(int (*ifunc)(void *),void *iarg,ulong stackSize)
{
  if ( ifunc )
    func = ifunc;
  if ( iarg )
    arg = iarg;
# ifdef WIN32
  handle = (HTHREAD)S_BeginThread( S_ThreadProt _S_ThreadFunc,
				   0,
				   stackSize,
				   (void *)this,
				   &tid);
# ifdef IBMCPP
  if ( (int)handle < 0 )
    rc = (int)handle;
#  endif
#  ifdef MSVCPP
  if ( handle == NULL )
    rc = -1;
# endif
# endif
# ifdef OS2
  handle = (HTHREAD)S_BeginThread(_S_ThreadFunc,0,stackSize,(void *)this,&tid);
  if ( (int)handle < 0 )
    rc = (int)handle;
# endif
# ifdef PTHREAD
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
  rc = pthread_create(&handle,&attr,_S_ThreadFunc,(void *)this);
  pthread_attr_destroy(&attr);
# endif
  if ( !rc )
    running = TRUE;
  return rc;
}


int
S_Thread::Stop()
{
  if ( handle )
    {
#     ifdef WIN32
      TerminateThread(handle,0);
#     endif
#     ifdef OS2
      DosKillThread(handle);
#     endif
#     ifdef PTHREAD
#     ifndef BSD
         pthread_detach(handle);
#     endif
      pthread_kill(handle,-9);
#     endif
      handle = 0;
    }
  running = FALSE;
  return 0;
}


int
S_Thread::WaitFor(ulong timeOut)
{
  if ( handle && running )
    {
#     ifdef WIN32
      if ( timeOut == S_THREAD_INFINITEWAIT )
	WaitForSingleObject(handle,timeOut);
      else
	WaitForSingleObject(handle,timeOut/1000);
      CloseHandle(handle);
#     endif
#     ifdef OS2
      if ( timeOut == S_THREAD_INFINITEWAIT )
	DosWaitThread(&handle,timeOut);
      else
	DosWaitThread(&handle,timeOut/1000);
#     endif
#     ifdef PTHREAD
      pthread_join(handle,NULL);
#     endif
      handle = 0;
    }
  return 0;
}


int
S_Thread::SetPriority(ulong priority)
{
  if ( handle )
    {
#     ifdef WIN32
      SetThreadPriority(handle,priority);
#     endif
#     ifdef OS2
      DosSetPriority(PRTYS_THREAD,priority,PRTYD_MAXIMUM,
		     (ulong)handle);
#     endif
#     ifdef PTHREAD
#     endif
    }
  return 0;
}
