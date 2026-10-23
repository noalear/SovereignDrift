/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
#ifndef S_MUTEX
#define S_MUTEX

#ifdef WIN32
#  define S_MUTEX_INFINITEWAIT INFINITE
#  define HMUTEX HANDLE
#endif
#ifdef OS2
#  define S_MUTEX_INFINITEWAIT SEM_INDEFINITE_WAIT
#  define HMUTEX HMTX
#endif
#ifdef UNIX
#  define S_MUTEX_INFINITEWAIT 0
#  ifdef PTHREAD
#     define HMUTEX pthread_mutex_t
#  endif
#endif

class S_Mutex
{
 public:
  HMUTEX handle;
  
  S_Mutex(void);
  ~S_Mutex(void);
  
  int Request(ulong=S_MUTEX_INFINITEWAIT);
  int Release(void);
};

#endif
