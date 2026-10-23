#ifndef SF_WAVELIST_INCLUDED
#define SF_WAVELIST_INCLUDED

class SF_WaveList {
public:
   int numWaves;
   S_WaveData **wave;

   int volume;
   int SetVolume(int);

   int Add(S_WaveData *);

   S_WaveData *Wave(int);
   S_WaveData *Wave(char *);

   int Remove(int);
   int Remove(char *);
   int RemoveAll();

   int Delete(int);
   int Delete(char *);
   int DeleteAll();

   SF_WaveList();
   ~SF_WaveList();
   };

#endif
