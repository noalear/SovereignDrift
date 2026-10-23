#include "sf.h"
#include "SF_PortCpp.h"
#include "SF_Externals.h"
#include "SF_MidiList.h"
#include "SF_NetEngine.h"
#include "SF_GameEngine.h"
#include "SF_Globals.h"
#include "SF_ClientWindow.h"
#include "SF_Client.h"
#include "SF_Engine.h"
#include "SF_Slot.h"

#define SF_MIDI_HASBEENREQUESTED -100


int
PlayThreadFunc(void *arg) {
   SF_MidiList *ml = (SF_MidiList *)arg;
   while ( ml->continueThread ) {
      S_Midi *mid = ml->Midi(ml->track);
      if ( mid )
         mid->Load();
      ml->event.WaitFor();
      if ( ml->continueThread ) {
         if ( mid ) {
            if ( !_OsFileExists(mid->filename) ) {
               if ( sf->engine ) {
                  SF_NetEngine *ne = sf->engine->netEngine;
                  SF_GameEngine *ge = sf->engine->gameEngine;
                  if ( ne && mid->rc != SF_MIDI_HASBEENREQUESTED ) {
                     mid->rc = SF_MIDI_HASBEENREQUESTED;
                     ne->slot->writeBuffer.mutex.Request();
		     mstate = 40;
                     ne->slot->writeBuffer.PushMsg((uchar)SF_MSG_DOWNLOADFILE,
                                                   (ushort)ge->localSlot,
                                                   (char *)mid->filename,
                                                   (ulong)0,
                                                   (char *)NULL,
                                                   (int)S_CLIENT_TCP);
		     mstate = 0;
                     ne->slot->writeBuffer.mutex.Release();
	             }
	          }
	       }
            else if ( mid->loaded ) {
               mid->Play(ml->volume);
               ml->track++;
	       }
            }
         if ( !ml->loop )
            ml->event.Reset();
         if ( ml->track >= ml->numMidis )
            ml->track = 0;
         }
      _OsSleep(100000);
      }
   return 0;
   }

SF_MidiList::SF_MidiList() {
   numMidis = 0;
   midi = NULL;
   track = -1;
   volume = 75;
   loop = FALSE;
   continueThread = TRUE;
   event.Reset();
   playThread = NULL;
   }


int
SF_MidiList::StartThread() {
   playThread = new S_Thread(PlayThreadFunc,(void *)this);
   return 0;
   }


int
SF_MidiList::StopThread() {
   continueThread = FALSE;
   Stop();
   track = -1;
   event.Set();
   if ( playThread )
      playThread->WaitFor();
   return 0;
   }

SF_MidiList::~SF_MidiList() {
   StopThread();
   DeleteAll();
   if ( playThread ) {
      delete playThread;
      playThread = NULL;
      }
   if ( midi ) {
      S_free(midi);
      midi = FALSE;
      }
   }


int
SF_MidiList::SetVolume(int ivolume ) {
   volume = ivolume;
   S_Midi *mid = Midi(track);
   if ( mid )
      mid->SetVolume(volume);
   return 0;
   }


int
SF_MidiList::Stop() {
   loop = FALSE;
   S_Midi *mid = Midi(track);
   track = -1;
   if ( mid )
      mid->Stop();
   return 0;
   }


int
SF_MidiList::PlayLoop() {
   loop = TRUE;
   track = 0;
   event.Set();
   return 0;
   }


int
SF_MidiList::Play(int i) {
   loop = FALSE;
   track = i;
   event.Set();
   return 0;
   }


int
SF_MidiList::Add(S_Midi *newMidi) {
   if ( numMidis == 0 && midi == NULL )
      midi = (S_Midi **)S_malloc(sizeof(S_Midi *));
   else if ( numMidis > 0 && midi != NULL )
      midi=(S_Midi **)S_realloc(midi,sizeof(S_Midi *)*(numMidis+1));

   midi[numMidis] = newMidi;
   numMidis++;
   return 0;
   }

S_Midi *
SF_MidiList::Midi(int i) {
   if ( i<0 || i>=numMidis )
      return NULL;
   return midi[i];
   }

S_Midi *
SF_MidiList::Midi(char *filename) {
   S_Midi *foundMidi = NULL;
   for ( int i=0; i<numMidis; i++ )
      if ( strcmp(filename,midi[i]->filename) == 0 )
         foundMidi = midi[i];
   return foundMidi;
   }


int
SF_MidiList::Remove(char *name) {
   for ( int i=0; i<numMidis; i++ )
      if ( strcmp(name,midi[i]->filename) == 0 )
         Remove(i);
   return 0;
   }


int
SF_MidiList::Remove(int j) {
   if ( j >= numMidis )
      return 0;
   for ( int i=j; i<numMidis-1; i++ )
      midi[i] = midi[i+1];
   numMidis--;
   return 0;
   }


int
SF_MidiList::RemoveAll() {
   numMidis = 0;
   return 0;
   }


int
SF_MidiList::Delete(char *name) {
   for ( int i=0; i<numMidis; i++ )
      if ( strcmp(name,midi[i]->filename) == 0 ) {
         delete midi[i];
         Remove(i);
         }
   return 0;
   }


int
SF_MidiList::Delete(int j) {
   if ( j >= numMidis )
      return 0;
   delete midi[j];
   for ( int i=j; i<numMidis-1; i++ )
      midi[i] = midi[i+1];
   numMidis--;
   return 0;
   }


int
SF_MidiList::DeleteAll() {
   for ( int i=0; i<numMidis; i++ )
     if ( midi[i] ) {
        delete midi[i];
        midi[i] = 0;
        }
   numMidis = 0;
   return 0;
   }
