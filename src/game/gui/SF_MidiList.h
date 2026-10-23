#ifndef SF_MIDILIST_INCLUDED
#define SF_MIDILIST_INCLUDED

class SF_MidiList {
public:
   int numMidis;
   S_Midi **midi;

   int Add(S_Midi *);

   S_Midi *Midi(int);
   S_Midi *Midi(char *);

   int Remove(int);
   int Remove(char *);
   int RemoveAll();

   int Delete(int);
   int Delete(char *);
   int DeleteAll();

   int loop;
   int track;
   int Play(int);
   int Play(char *);
   int PlayLoop();
   int Stop();

   int continueThread;
   S_Thread *playThread;
   S_Event event;
   int StartThread();
   int StopThread();

   int volume;
   int SetVolume(int);

   SF_MidiList();
   ~SF_MidiList();
   };

#endif
