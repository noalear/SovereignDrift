#include "S_OS.h"
#include "S_AudioDevice.h"
#include "S_Root.h"
#include "S_Thread.h"
#include "S_WaveData.h"

int _WaveThread(void *);

S_AudioDevice::S_AudioDevice() {
   rc = 0;

   volume = 100;
   wavePlaying = NULL;
   waveToBePlayed = NULL;

#  if defined(OS2) && defined(IBMMMOS2)

      canMix = FALSE;

      mmPlayList[0].ulCommand = DATA_OPERATION;
      mmPlayList[0].ulOperand1 = NULL;            /* *** pAudioData *** */
      mmPlayList[0].ulOperand2 = 0;               /* *** ulDataSize *** */
      mmPlayList[0].ulOperand3 = 0;
      mmPlayList[1].ulCommand = EXIT_OPERATION;
      mmPlayList[1].ulOperand1 = 0;
      mmPlayList[1].ulOperand2 = 0;
      mmPlayList[1].ulOperand3 = 0;

      mmOpenP.pszDeviceType = 
         (PSZ) MAKEULONG(MCI_DEVTYPE_WAVEFORM_AUDIO,1);
      mmOpenP.pszAlias = "wav1";
      mmOpenP.pszElementName = (PSZ) &mmPlayList;

      rc = mciSendCommand( 0,
                    MCI_OPEN,MCI_WAIT | MCI_OPEN_PLAYLIST | MCI_OPEN_TYPE_ID |
                    MCI_OPEN_SHAREABLE, (PVOID) &mmOpenP,0 );
#  endif

#  ifdef WIN32

      if ( _root&&(!_root->lpDs || !(_root->flags & S_ROOT_USEDIRECTSOUND)) ) {
         canMix = FALSE;
         mmOpenP.lpstrDeviceType = (char *)"waveaudio";
         rc = mciSendCommand(0, MCI_OPEN,MCI_WAIT | MCI_OPEN_TYPE | 
                             MCI_OPEN_SHAREABLE, (DWORD)&mmOpenP);
         }
      else
         canMix = TRUE;

      /* A primary buffer is created automatically by DS so this not needed
      lpDsBuffer = NULL;
      if ( _root && _root->lpDs && _root->flags & S_ROOT_USEDIRECTSOUND ) {
         DSBUFFERDESC dsbdesc;
         memset(&dsbdesc, 0, sizeof(DSBUFFERDESC));
         dsbdesc.dwSize = sizeof(DSBUFFERDESC);
         dsbdesc.dwFlags = DSBCAPS_PRIMARYBUFFER;
         dsbdesc.dwBufferBytes = 0; 
         dsbdesc.lpwfxFormat = NULL;
         _root->lpDs->CreateSoundBuffer(&dsbdesc,&lpDsBuffer,NULL);
         }
	 */
#  endif

   waveThread = NULL;
   if ( !canMix ) {
      threadContinue = TRUE;
      waveThread = new S_Thread(_WaveThread,(void *)this);
      }

   }

S_AudioDevice::~S_AudioDevice() {
   if ( waveThread ) {
      threadContinue = FALSE;
      event.Set();
      waveThread->WaitFor();
      delete waveThread;
      waveThread = NULL;
      }
#  if defined(OS2) && defined(IBMMMOS2)
      rc = mciSendCommand( mmOpenP.usDeviceID,MCI_CLOSE,MCI_WAIT,
                          (PVOID)&mmOpenP,0 );
#  endif
#  ifdef WIN32
      if ( _root&&(!_root->lpDs || !(_root->flags & S_ROOT_USEDIRECTSOUND)) )
         rc = mciSendCommand( mmOpenP.wDeviceID,MCI_CLOSE,MCI_WAIT,
                             (DWORD)&mmOpenP);
      /*
      if ( lpDsBuffer != NULL ) {
         lpDsBuffer->Release();
         lpDsBuffer = NULL;
         }
	 */
#  endif
   }


inline int _PlayWave(S_AudioDevice *ad,S_WaveData *w) {
   if ( !w || !ad )
      return 0;
   if ( w->rc != S_WAVEDATA_OK )
      return 0;
#  if defined(OS2) && defined(IBMMMOS2)
      mciSendCommand( ad->mmOpenP.usDeviceID,
                      MCI_SET,MCI_WAIT | MCI_WAVE_SET_BITSPERSAMPLE |
                      MCI_WAVE_SET_SAMPLESPERSEC | MCI_WAVE_SET_CHANNELS,
                     (PVOID) &w->mmWaveSetParm,
                      0 );
      ad->mmPlayList[0].ulOperand1 = (ulong)w->pAudioData;
      ad->mmPlayList[0].ulOperand2 = w->ulDataSize;
      mciSendCommand( ad->mmOpenP.usDeviceID,
                      MCI_SEEK,0 | MCI_WAIT | MCI_TO_START,
                     (PVOID) &w->mmAudioHead,
                      0 );
      mciSendCommand( ad->mmOpenP.usDeviceID,
                      MCI_PLAY,0/* | MCI_WAIT*/,
                     (PVOID) &w->mmAudioHead,
                      0 );
      ad->sw.Start();
      ad->wavePlaying = w;
#  endif
#  ifdef WIN32

   if ( w && w->lpDsb &&
        _root && _root->lpDs && _root->flags & S_ROOT_USEDIRECTSOUND ) {
      ulong status = 0;
      w->lpDsb->GetStatus(&status);

      if ( w->loop ) {
         if ( !(status & DSBSTATUS_LOOPING) ) {
            w->lpDsb->Play(0,0,DSBPLAY_LOOPING);
	    }
         }
      else {
         w->lpDsb->SetCurrentPosition(0);
         if ( !(status & DSBSTATUS_PLAYING) ) {
            w->lpDsb->Play(0,0,0);
	    }
         }
      }
#  endif
   return 0;
   }


int
_WaveThread(void *arg) {
   S_AudioDevice *ad = (S_AudioDevice *)arg;

   if ( !ad ) return -1;

   ad->event.Reset();
   while ( ad->threadContinue ) {
      ad->event.WaitFor();
      ad->event.Reset();
      if ( ad->threadContinue )
         _PlayWave(ad,ad->waveToBePlayed);
      }

   return 0;
   }


ulong 
S_AudioDevice::GetPosition() {
  /*
#  if defined(OS2) && defined(IBMMMOS2)
      MCI_STATUS_PARMS mmStatus;
      mmStatus.ulItem = MCI_STATUS_POSITION;
      mciSendCommand(mmOpenP.usDeviceID,
                     MCI_STATUS,MCI_WAIT | MCI_STATUS_ITEM,
                    (PVOID)&mmStatus,0);
      return MSECFROMMM(mmStatus.ulReturn);
#  endif
   */
   sw.Stop();
   return sw.dtSec*1000 + sw.dtUsec/1000;
   }



int
S_AudioDevice::Stop(S_WaveData *w) {
#  if defined(OS2) && defined(IBMMMOS2)
      if ( w )
         mciSendCommand( mmOpenP.usDeviceID,
                         MCI_STOP,0 | MCI_WAIT,
                        (PVOID) &w->mmAudioHead,
                         0 );
#  endif
#  ifdef WIN32
   if ( w && w->lpDsb &&
        _root && _root->lpDs && _root->flags & S_ROOT_USEDIRECTSOUND )
      w->lpDsb->Stop();
#  endif
   return 0;
   }


int
S_AudioDevice::Play(S_WaveData *w) {

   if ( !w )
     return 0;

   if ( !canMix ) {
      if ( wavePlaying ) {
         if ( GetPosition() >= wavePlaying->msec )
            wavePlaying = NULL;
         else if ( w->priority < wavePlaying->priority )
            return 0;
         }
      waveToBePlayed = w;
      event.Set();
      }

   if ( canMix )
      _PlayWave(this,w);

   return 0;
   }


int
S_AudioDevice::SetVolume(int ivolume) {

   volume = ivolume;

#  if defined(OS2) && defined(IBMMMOS2)
      MCI_GENERIC_PARMS mciGenericParms;
      MCI_SET_PARMS msp;
      MciSendCommand( mmOpenP.usDeviceID,
                      MCI_ACQUIREDEVICE,MCI_WAIT, &mciGenericParms,0 );
      msp.ulLevel = volume;
      msp.ulAudio = MCI_SET_AUDIO_ALL;
      MciSendCommand( mmOpenP.usDeviceID,
                      MCI_SET,MCI_WAIT | MCI_SET_AUDIO | MCI_SET_VOLUME,
                      (PVOID) &msp,0 );
#  endif

   return 0;
   }

