#include "sf.h"
#include "SF_WaveList.h"

SF_WaveList::SF_WaveList() {
   numWaves = 0;
   wave = NULL;
   volume = 100;
   }

SF_WaveList::~SF_WaveList() {
   DeleteAll();
   if ( wave ) {
      S_free(wave);
      wave = NULL;
      }
   }


int
SF_WaveList::Add(S_WaveData *newWave) {
   if ( numWaves == 0 && wave == NULL )
      wave = (S_WaveData **)S_malloc(sizeof(S_WaveData *));
   else if ( numWaves > 0 && wave != NULL )
      wave=(S_WaveData **)S_realloc(wave,sizeof(S_WaveData *)*(numWaves+1));

   wave[numWaves] = newWave;
   numWaves++;
   return 0;
   }

S_WaveData *
SF_WaveList::Wave(int i) {
   if ( i<0 || i>=numWaves )
      return NULL;
   return wave[i];
   }

S_WaveData *
SF_WaveList::Wave(char *filename) {
   S_WaveData *foundWave = NULL;
   for ( int i=0; i<numWaves; i++ )
      if ( strcmp(filename,wave[i]->name) == 0 )
         foundWave = wave[i];
   return foundWave;
   }


int
SF_WaveList::Remove(char *name) {
   for ( int i=0; i<numWaves; i++ )
      if ( strcmp(name,wave[i]->name) == 0 )
         Remove(i);
   return 0;
   }


int
SF_WaveList::SetVolume(int ivolume) {
   volume = ivolume;
   for ( int i=0; i<numWaves; i++ )
      if ( wave[i] )
         wave[i]->SetVolume(volume);
   return 0;
   }


int
SF_WaveList::Remove(int j) {
   if ( j >= numWaves )
      return 0;
   for ( int i=j; i<numWaves-1; i++ )
      wave[i] = wave[i+1];
   numWaves--;
   return 0;
   }


int
SF_WaveList::RemoveAll() {
   numWaves = 0;
   return 0;
   }


int
SF_WaveList::Delete(char *name) {
   for ( int i=0; i<numWaves; i++ )
      if ( strcmp(name,wave[i]->name) == 0 ) {
         delete wave[i];
         Remove(i);
         }
   return 0;
   }


int
SF_WaveList::Delete(int j) {
   if ( j >= numWaves )
      return 0;
   delete wave[j];
   for ( int i=j; i<numWaves-1; i++ )
      wave[i] = wave[i+1];
   numWaves--;
   return 0;
   }


int
SF_WaveList::DeleteAll() {
   for ( int i=0; i<numWaves; i++ )
     if ( wave[i] ) {
        delete wave[i];
        wave[i] = 0;
        }
   numWaves = 0;
   return 0;
   }
