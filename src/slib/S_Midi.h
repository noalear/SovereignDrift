#ifndef S_MIDI
#define S_MIDI

class S_Midi {
public:
   int rc;
   char *filename;

   S_Midi(char *,int=100);
   ~S_Midi();

   int play;
   int Play(int=-1);
   int Stop();

   int loaded;
   int Load();

   int volume;
   int SetVolume(int);
   };

class S_MidiPlaylist {
public:
   int numMidis;
   int currentMidi;
   S_Midi *midi;

   S_MidiPlaylist();
   ~S_MidiPlaylist();

   int Add(S_Midi *);
   int Remove(int);

   int play;
   int Play();
   int Stop();

   int volume;
   int SetVolume();
   };

#endif
