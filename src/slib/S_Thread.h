/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
#ifndef S_THREAD
#define S_THREAD

#define S_THREAD_OK             0

#ifdef WIN32

//#  define S_THREAD_APITHREADS

#  define S_THREAD_INFINITEWAIT INFINITE
#  define S_THREAD_HIGHESTPRIORITY THREAD_PRIORITY_HIGHEST
#  define S_THREAD_HIGHPRIORITY    THREAD_PRIORITY_TIME_CRITICAL
#  define S_THREAD_MEDIUMPRIORITY  THREAD_PRIORITY_NORMAL
#  define S_THREAD_LOWPRIORITY     THREAD_PRIORITY_IDLE
#  define S_THREAD_LOWESTPRIORITY  THREAD_PRIORITY_LOWEST
#  define HTHREAD HANDLE
#  ifdef S_THREAD_APITHREADS
#      define S_ThreadFunc 
#      define S_ThreadProt (LPTHREAD_START_ROUTINE)
#      define S_ThreadRet  void
#      define S_BeginThread(a,b,c,d,e) CreateThread(NULL,(c),(a),(d),0,(e))
#      define S_ExitThread() ExitThread(0)
#      define S_THREAD_STACKSIZE 98304
#  else
#    ifdef IBMCPP
#      define S_ThreadFunc _Optlink
#      define S_BeginThread(a,b,c,d,e) _beginthread((a),(b),(c),(d))
#    endif
#    ifdef MSVCPP
#      define S_ThreadFunc
#      define S_BeginThread(a,b,c,d,e) _beginthread((a),(c),(d))
#    endif
#    define S_ExitThread()    _endthread()
#    define S_ThreadRet  void
#    define S_ThreadProt 
#    define S_THREAD_STACKSIZE 98304
#  endif
#  define HZ 100
   typedef ulong TID;
#endif
#ifdef OS2
#  define S_THREAD_INFINITEWAIT DCWW_WAIT
#  define S_THREAD_HIGHESTPRIORITY PRTYC_FOREGROUNDSERVER
#  define S_THREAD_HIGHPRIORITY    PRTYC_TIMECRITICAL
#  define S_THREAD_MEDIUMPRIORITY  PRTYC_REGULAR
#  define S_THREAD_LOWPRIORITY     PRTYC_IDLETIME
#  define S_THREAD_LOWESTPRIORITY  PRTYC_IDLETIME
#  define HTHREAD TID
#  define S_ThreadFunc _Optlink
#  define S_ThreadRet  void
#  define S_BeginThread(a,b,c,d,e)  _beginthread((a),(b),(c),(d))
#  define S_ExitThread()  _endthread()
#  define S_THREAD_STACKSIZE 98304
#  ifndef HZ
#     define HZ 32
#  endif
#endif
#ifdef UNIX 
#  define S_THREAD_INFINITEWAIT ULONG_MAX
#  define S_THREAD_HIGHESTPRIORITY 0
#  define S_THREAD_HIGHPRIORITY    0
#  define S_THREAD_MEDIUMPRIORITY  0
#  define S_THREAD_LOWPRIORITY     0
#  define S_THREAD_LOWESTPRIORITY  0
#  define S_ThreadFunc
#  define S_ThreadRet void*
#  define S_THREAD_STACKSIZE 98304
#  ifdef PTHREAD
#     define _ExitThread()    pthread_exit(NULL)
      typedef pthread_t HTHREAD;
      typedef pthread_t TID;
#  endif
#endif

class S_Thread
{
 public:
  ulong check;
  int rc;
  TID tid;
  HTHREAD handle;
  int (*func)(void *);
  void *arg;
  int running;
  int selfDelete;
  
  virtual int Function();

  int Init();
  int Destroy();  
  S_Thread();
  S_Thread(int(*)(void *),void * = NULL,ulong = S_THREAD_STACKSIZE);
  virtual ~S_Thread(void);
  
  int Start(int(*)(void *) = NULL,void * = NULL,ulong = S_THREAD_STACKSIZE);
  int Suspend(void);
  int Resume(void);
  int Stop(void);
  int WaitFor(ulong=S_THREAD_INFINITEWAIT );
  int SetPriority(ulong);
};

#endif
