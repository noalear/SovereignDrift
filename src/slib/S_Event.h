/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
#ifndef S_EVENT
#define S_EVENT

#ifdef WIN32
#  define S_EVENT_INFINITEWAIT INFINITE
#  define HEVENT HANDLE
#  define HMUTEX HANDLE
#endif
#ifdef OS2
#  define S_EVENT_INFINITEWAIT SEM_INDEFINITE_WAIT
#  define HEVENT HEV
#  define HMUTEX HMTX
#endif
#ifdef UNIX 
#  define S_EVENT_INFINITEWAIT ULONG_MAX
#  define S_EVENT_UNBLOCKED 0
#  define S_EVENT_BLOCKED 1
#  ifdef PTHREAD
#     define HEVENT pthread_cond_t
#     define HMUTEX pthread_mutex_t
#  endif
#endif

class S_Event
{
 public:
  
  HEVENT handle;
  HMUTEX hMutex;
  ulong postCount;
  int state;
  
  S_Event(void);
  ~S_Event(void);
  
  int WaitFor(ulong=S_EVENT_INFINITEWAIT);
  int Reset(void);
  int Set(void);
};

#endif
