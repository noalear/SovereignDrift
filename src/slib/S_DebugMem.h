/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
#ifndef S_DEBUGMEM
#define S_DEBUGMEM

#  define S_DEBUGMEM_ALLOCATED 1
#  define S_DEBUGMEM_FREED     2

   typedef struct {
      ulong id;
      ulong numBytes;
      ulong state;
      } S_DebugMem_StateStruct;

   typedef struct {
      ulong id;
      ulong numBytes;
      unsigned char buffer[1];
      } S_DebugMem_AllocStruct;

   void S_DebugMem_SetAllocated(ulong=0);
   void S_DebugMem_SetFreed(ulong=0);
   void S_DebugMem_SetThrowException(int=1);
   void S_DebugMem_SetPageException(ulong=0xFFFFFFFF);
   void S_DebugMem_SetVerbose(int=1);
   void S_DebugMem_SetSavePageStates(int=1);
   void S_DebugMem_StartLeakCheck();
   void S_DebugMem_StopLeakCheck();

   void *S_DebugMem_Malloc(size_t);
   void *S_DebugMem_Realloc(void *,size_t);
   char *S_DebugMem_Strdup(char *);
   void  S_DebugMem_SetGuardPage(S_DebugMem_AllocStruct *);
   void  S_DebugMem_Zero(S_DebugMem_AllocStruct *);
   int   S_DebugMem_Check(void *);
   void  S_DebugMem_Free(void *);
   void  S_DebugMem_Profile(S_DebugMem_AllocStruct *,slong=0);

   void  _check(void *);

#  ifdef S_DEBUGMEM_ON
#     define S_malloc  S_DebugMem_Malloc
#     define S_realloc S_DebugMem_Realloc
#     define S_free    S_DebugMem_Free
#     define S_mcheck  S_DebugMem_Check
#     define S_strdup  S_DebugMem_Strdup
#  else
#     define S_malloc  malloc
#     define S_realloc realloc
#     define S_free    free
#     define S_mcheck  _check
#     define S_strdup  strdup
#  endif



#endif
