/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
#ifndef S_AUDIODEVICE
#define S_AUDIODEVICE

#include "S_Event.h"
#include "S_StopWatch.h"

typedef struct {
   ulong ulCommand;
   ulong ulOperand1;
   ulong ulOperand2;
   ulong ulOperand3;
   } AudioPlayListStruct;

class S_Thread;
class S_WaveData;

class S_AudioDevice {
public:
   int rc;

#  if (defined(OS2) && defined(IBMMMOS2)) || defined(WIN32)
      AudioPlayListStruct mmPlayList[2]; /* ** Gets modified and reused ** */
      MCI_OPEN_PARMS mmOpenP;            /* ** Gets reused not modified ** */
#  endif

#  ifdef WIN32
      LPDIRECTSOUNDBUFFER lpDsBuffer;
#  endif

   int canMix;

   int volume;
   int SetVolume(int);

   S_AudioDevice();
   ~S_AudioDevice();

   S_Event event;
   int threadContinue;
   S_Thread *waveThread;
   S_WaveData *wavePlaying;
   S_WaveData *waveToBePlayed;
   int Play(S_WaveData *);
   int Stop(S_WaveData *);
   ulong GetPosition();

   S_StopWatch sw;
   };

#endif
