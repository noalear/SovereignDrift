/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
#ifndef S_WAVEDATA
#define S_WAVEDATA

#define S_WAVEDATA_OK                0
#define S_WAVEDATA_FILEDOESNOTEXIST -1

typedef struct
{
  char RIFF[4];
  ulong ulRiffDataSize;
} WaveRIFFHeaderStruct;


typedef struct
{
  char WAVE[4];
  char fmt[4];
  ulong wavHeaderDataSize;
  ushort usFormatTag;
  ushort usChannels;
  ulong  ulSamplesPerSec;
  ulong  ulAveBytesPerSec;
  ushort usBlockAlign;
  ushort usBitsPerSample;
} WaveWAVEHeaderStruct;


typedef struct 
{
  char FACT[4];
  ulong ulDum1;
  ulong ulDataSize;
} WaveFACTHeaderStruct;


typedef struct
{
  char DATA[4];
  ulong ulDataSize;
} WaveDATAHeaderStruct;


typedef struct
{
  char WAVE[4];
  char fmt[4];
  ulong wavHeaderDataSize;
  ushort usFormatTag;
  ushort usChannels;
  ulong  ulSamplesPerSec;
  ulong  ulAveBytesPerSec;
  ushort usBlockAlign;
  ushort usBitsPerSample;
  ushort usDum;
} WaveWAVEHeaderStruct18;


class S_WaveData
{
 public:
  
  int rc;
  
# ifdef WIN32
# define DSBCAPS_CTRLDEFAULT 0x000000E0
  LPDIRECTSOUNDBUFFER lpDsb;
# endif
  
# if (defined(OS2) && defined(IBMMMOS2)) || defined(WIN32)
# ifdef OS2
  MMAUDIOHEADER mmAudioHead;
# endif
  MCI_WAVE_SET_PARMS mmWaveSetParm;
# endif
  
  char *name;
  int priority;
  int loop;
  WaveWAVEHeaderStruct header;
  ulong ulDataSize;
  char *pAudioData;
  ulong msec;
  
  int volume;
  int SetVolume(int);
  
  int Load();
  
  int Init();
  S_WaveData();
  S_WaveData(char *,int=0,int=0);
  int Create(char *,int=0,int=0);
  int ZCreate(char *,int=0,int=0);
  int Create(uchar *,slong,int=0,int=0);
  int Destroy();
  ~S_WaveData();
  
};

#endif
