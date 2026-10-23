/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "S_OS.h"
#include "S_DebugMem.h"
#include "S_Mutex.h"

#define S_DEBUGMEM_GUARDPAGESIZE    16
#define S_DEBUGMEM_HEADERSIZE        8
#define S_DEBUGMEM_GUARDPAGEVALUE 0xAA

ulong S_DebugMem_NumPages = 0;
S_DebugMem_StateStruct *S_DebugMem_States = NULL;

int S_DebugMem_ThrowException = 0;
int S_DebugMem_Verbose = 0;
ulong S_DebugMem_Allocated = 0;
ulong S_DebugMem_Freed = 0;
ulong S_DebugMem_GlobalAllocated = 0;
ulong S_DebugMem_GlobalFreed = 0;
ulong S_DebugMem_PagesAllocated = 0;
ulong S_DebugMem_PagesFreed = 0;
ulong S_DebugMem_SavePageStates = 0;
ulong S_DebugMem_PageException = 0xFFFFFFFF;

S_Mutex S_DebugMem_Mutex;

void S_DebugMem_StartLeakCheck() {
   S_DebugMem_GlobalAllocated = S_DebugMem_Allocated;
   S_DebugMem_GlobalFreed = S_DebugMem_Freed;
   S_DebugMem_Allocated = 0;
   S_DebugMem_Freed = 0;
   S_DebugMem_Verbose = 1;
   }

void S_DebugMem_StopLeakCheck() {
   S_DebugMem_Allocated = S_DebugMem_GlobalAllocated;
   S_DebugMem_Freed = S_DebugMem_GlobalFreed;
   S_DebugMem_Verbose = 0;
   }

void S_DebugMem_SetFreed(ulong val) {
   S_DebugMem_Freed = val;
   }

void S_DebugMem_SetAllocated(ulong val) {
   S_DebugMem_Allocated = val;
   }

void S_DebugMem_SetVerbose(int val) {
   S_DebugMem_Verbose = val;
   }

void S_DebugMem_SetSavePageStates(int val) {
   S_DebugMem_SavePageStates = val;
   }

void S_DebugMem_SetThrowException(int val) {
   S_DebugMem_ThrowException = val;
   }

void S_DebugMem_SetPageException(ulong val) {
   S_DebugMem_PageException = val;
   }

void *S_DebugMem_Malloc(size_t numBytes) {

   void *ptr = NULL;
   S_DebugMem_AllocStruct *dptr = (S_DebugMem_AllocStruct *)malloc( numBytes +
                                              2*S_DEBUGMEM_GUARDPAGESIZE + 
                                              S_DEBUGMEM_HEADERSIZE );
   if ( dptr ) {
      S_DebugMem_Allocated += numBytes;
      dptr->numBytes = numBytes;
      dptr->id = S_DebugMem_PagesAllocated++;
      if ( S_DebugMem_Verbose )
         S_DebugMem_Profile(dptr,numBytes);
      if ( S_DebugMem_SavePageStates ) {
         S_DebugMem_Mutex.Request();
         if ( S_DebugMem_States == NULL )
            S_DebugMem_States = (S_DebugMem_StateStruct *)
                                 malloc(sizeof(S_DebugMem_StateStruct));
         else
            S_DebugMem_States = (S_DebugMem_StateStruct *)
                                 realloc(S_DebugMem_States,
                                         sizeof(S_DebugMem_StateStruct)*
                                         (S_DebugMem_NumPages+1));
         if ( S_DebugMem_States ) {
            S_DebugMem_States[S_DebugMem_NumPages].id = dptr->id;
            S_DebugMem_States[S_DebugMem_NumPages].state =S_DEBUGMEM_ALLOCATED;
            S_DebugMem_States[S_DebugMem_NumPages].numBytes = dptr->numBytes;
            S_DebugMem_NumPages++;
            }
         else {
            printf("S_DebugMem_Malloc(): ERROR - Memory allocation failed.");
            S_DebugMem_SavePageStates = FALSE;   
	    }
         S_DebugMem_Mutex.Release();
         }
      S_DebugMem_SetGuardPage(dptr);
      S_DebugMem_Zero(dptr);
      if ( S_DebugMem_PageException == dptr->id ) {
         char *ptr = (char *)0xBBBBBBBB;
         *ptr = 0;
         }
      return ((char *)dptr) + S_DEBUGMEM_GUARDPAGESIZE + S_DEBUGMEM_HEADERSIZE;
      }

   return ptr;
   }

void *S_DebugMem_Realloc(void *ptr,size_t nsize) {
   if ( !ptr ) {
      printf("S_DebugMem_Realloc(): WARNING - Attempt to realloc NULL pointer."
             "  Using S_DebugMem_Malloc() instead.\n");
      if ( S_DebugMem_ThrowException ) {
         char *ptr = (char *)0xAAAAAAAA;
         *ptr = 0;
         }
      return S_DebugMem_Malloc(nsize);
      }

   S_DebugMem_AllocStruct *dptr = (S_DebugMem_AllocStruct *)
                       ((char *)ptr - 
                        S_DEBUGMEM_GUARDPAGESIZE - 
                        S_DEBUGMEM_HEADERSIZE);

   if ( S_DebugMem_Check(ptr) ) {
      printf("dfree error -- Memory %ld address 0x%x is corrupt (%ld bytes)\n",
             dptr->id,(uint)ptr,dptr->numBytes);
      if ( S_DebugMem_ThrowException ) {
         char *ptr = (char *)0xAAAAAAAA;
         *ptr = 0;
         }
      return NULL;
      }

   dptr = (S_DebugMem_AllocStruct *)realloc((char *)dptr,nsize + 
                                              2*S_DEBUGMEM_GUARDPAGESIZE + 
                                              S_DEBUGMEM_HEADERSIZE );
   if ( dptr ) {
      S_DebugMem_Allocated += (nsize-dptr->numBytes);
      if ( S_DebugMem_Verbose )
         S_DebugMem_Profile(dptr,nsize-dptr->numBytes);
      dptr->numBytes = nsize;
      if ( S_DebugMem_SavePageStates )
         S_DebugMem_States[dptr->id].numBytes = dptr->numBytes;
      S_DebugMem_SetGuardPage(dptr);
      return ((char *)dptr) + S_DEBUGMEM_GUARDPAGESIZE + S_DEBUGMEM_HEADERSIZE;
      }
   return NULL;
   }

void S_DebugMem_Zero(S_DebugMem_AllocStruct *dptr) {
   for ( ulong i=S_DEBUGMEM_GUARDPAGESIZE; 
         i<dptr->numBytes+S_DEBUGMEM_GUARDPAGESIZE; 
         i++ )
      dptr->buffer[i] = 0;
   }

void S_DebugMem_SetGuardPage(S_DebugMem_AllocStruct *dptr) {
   ulong i;
   for ( i=0; i<S_DEBUGMEM_GUARDPAGESIZE; i++ )
      dptr->buffer[i] = S_DEBUGMEM_GUARDPAGEVALUE;
   for ( i=dptr->numBytes+S_DEBUGMEM_GUARDPAGESIZE; 
         i<dptr->numBytes+2*S_DEBUGMEM_GUARDPAGESIZE; 
         i++ )
      dptr->buffer[i] = S_DEBUGMEM_GUARDPAGEVALUE;
   }

void _check(void *){}

int S_DebugMem_Check(void *ptr) {
   if ( !ptr ) {
      printf("S_DebugMem_Free(): WARNING - Attempt to check invalid pointer "
             "0x%x\n",(uint)ptr);
      if ( S_DebugMem_ThrowException ) {
         char *ptr = (char *)0xAAAAAAAA;
         *ptr = 0;
         }
      return -1;
      }
   ulong i;
   S_DebugMem_AllocStruct *dptr = (S_DebugMem_AllocStruct *) ( (char *)ptr - 
                                        S_DEBUGMEM_GUARDPAGESIZE - 
                                        S_DEBUGMEM_HEADERSIZE );

   if ( dptr->id >= S_DebugMem_PagesAllocated )
      return -1;
   for ( i=0; i<S_DEBUGMEM_GUARDPAGESIZE; i++ ) {
      if ( dptr->buffer[i] != S_DEBUGMEM_GUARDPAGEVALUE )
         return -1;
      }
   for ( i=S_DEBUGMEM_GUARDPAGESIZE+dptr->numBytes; 
         i<2*S_DEBUGMEM_GUARDPAGESIZE+dptr->numBytes; 
         i++ ) {
      if ( dptr->buffer[i] != S_DEBUGMEM_GUARDPAGEVALUE )
         return -1;
      }
   return 0;
   }

void S_DebugMem_Free(void *ptr) {

   if ( !ptr ) {
      printf("S_DebugMem_Free(): WARNING - Attempt to free NULL pointer\n");
      if ( S_DebugMem_ThrowException ) {
         char *ptr = (char *)0xAAAAAAAA;
         *ptr = 0;
         }
      return;
      }

   S_DebugMem_AllocStruct *dptr = (S_DebugMem_AllocStruct *) ( (char *)ptr - 
                                        S_DEBUGMEM_GUARDPAGESIZE - 
                                        S_DEBUGMEM_HEADERSIZE );

   if ( S_DebugMem_Check(ptr) ) {
      printf("dfree error -- Memory %ld address 0x%x is corrupt (%ld bytes)\n",
             dptr->id,(uint)ptr,dptr->numBytes);
      if ( S_DebugMem_ThrowException ) {
         char *ptr = (char *)0xAAAAAAAA;
         *ptr = 0;
         }
      }
   else {
      int proceed = 1;

      if ( S_DebugMem_SavePageStates )
         switch(S_DebugMem_States[dptr->id].state) {
            case S_DEBUGMEM_ALLOCATED :
               S_DebugMem_States[dptr->id].state = S_DEBUGMEM_FREED;
               break;
            case S_DEBUGMEM_FREED :
               proceed = 0;
               printf("Warning: Attempt to free page %ld already marked "
                      "S_DEBUGMEM_FREED\n",dptr->id);
               break;
            default :
               proceed = 0;
               printf("Warning: Attempt to free page %ld with unknown "
                      "state %ld\n",dptr->id,S_DebugMem_States[dptr->id].state);
	    }

      if ( proceed ) {
         S_DebugMem_Freed += dptr->numBytes;
         S_DebugMem_PagesFreed++;
         if ( S_DebugMem_Verbose )
            S_DebugMem_Profile(dptr,-(slong)dptr->numBytes);
	     free((char *)ptr - S_DEBUGMEM_GUARDPAGESIZE - S_DEBUGMEM_HEADERSIZE);
         }
      }

   }


char *S_DebugMem_Strdup(char *istr) {
   if ( !istr ) {
      printf("S_DebugMem_Strdup(): WARNING - Attempt to dup NULL string\n");
      if ( S_DebugMem_ThrowException ) {
         char *ptr = (char *)0xAAAAAAAA;
         *ptr = 0;
         }
      return NULL;
      }
   char *str = (char *)S_DebugMem_Malloc(strlen(istr)+1);
   if ( str )
      strcpy(str,istr);
   return str;
   }

void S_DebugMem_Profile(S_DebugMem_AllocStruct *dptr,slong dmem) {
   if ( dmem > 0 )
   printf("alloc %ld %6ld bytes:%6ld-%6ld=%ld bytes:%4ld-%4ld=%ld pages\n",
                                                dptr->id,dmem,
                                                S_DebugMem_Allocated,
                                                S_DebugMem_Freed,
                                                S_DebugMem_Allocated-
                                                S_DebugMem_Freed,
                                                S_DebugMem_PagesAllocated,
                                                S_DebugMem_PagesFreed,
                                                S_DebugMem_PagesAllocated -
                                                S_DebugMem_PagesFreed);
   if ( dmem < 0 )
   printf("freed %ld %6ld bytes:%6ld-%6ld=%ld bytes:%4ld-%4ld=%ld pages\n",
	  dptr->id,
	  -dmem,
	  S_DebugMem_Allocated,
	  S_DebugMem_Freed,
	  S_DebugMem_Allocated-
	  S_DebugMem_Freed,
	  S_DebugMem_PagesAllocated,
	  S_DebugMem_PagesFreed,
	  S_DebugMem_PagesAllocated -
	  S_DebugMem_PagesFreed);
   if ( S_DebugMem_SavePageStates ) {
      ulong numPagesDisplayed = 0;
      for ( ulong i=0; i<S_DebugMem_NumPages; i++ )
         if ( numPagesDisplayed < 10 &&
              S_DebugMem_States[i].state == S_DEBUGMEM_ALLOCATED ) {
            printf("Page %ld (%ld bytes) is allocated\n",
                   i,S_DebugMem_States[i].numBytes);
            numPagesDisplayed++;
	    }
      }
   }

#if defined(S_DEBUGMEM_ON) && !defined(__IBMCPP__) && !defined(__GNUC__)
   void * ::operator new(size_t size) {
      void *ptr = S_DebugMem_Malloc(size);
      return ptr;
      }

   void ::operator delete(void *ptr) {
      S_DebugMem_Free(ptr);
      }
#endif
