/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
#include "S_OS.h"
#include "S_WaveData.h"
#include "S_Root.h"
#include "S_ZArchive.h"


int
S_WaveData::Init()
{
  rc = S_WAVEDATA_OK;
  name = NULL;
  volume = 100;
  loop = FALSE;
  priority = 1;
  pAudioData = NULL;
# ifdef WIN32
  lpDsb = NULL;
# endif
  return 0;
}


S_WaveData::S_WaveData()
{
  Init();
}


S_WaveData::S_WaveData(char *filename,int ipriority,int iloop)
{
  Init();
  Create(filename,ipriority,iloop);
}


int
S_WaveData::ZCreate(char *filename,int ipriority,int iloop)
{
  if ( filename )
    name = S_strdup(filename);
  loop = iloop;
  priority = ipriority;
  
  S_ZArchive zArc;
  S_ZFile *zFile;
  zArc.Open(_root->exeFilename);
  do
    {
      zFile = zArc.ReadNextFile();
      if ( zFile )
	{
	  if ( stricmp(zFile->name,filename) == 0 )
	    {
	      zFile->UnCompress();
	      Create(zFile->buffer,zFile->hdr.fileSize,priority,loop);
	    }
	  delete zFile;
	}
    } while (zFile);
  
  return 0;
}


int
S_WaveData::Create(char *filename,int ipriority,int iloop)
{
  if ( filename )
    name = S_strdup(filename);
  loop = iloop;
  priority = ipriority;
  
  rc = S_WAVEDATA_OK;
  
  Load();
  
  return 0;
}



int
S_WaveData::Load()
{
  
  char *fileName = NULL;
  if ( _OsFileExists(name) )
    fileName = S_strdup(name);
  else
    {
      fileName = (char *)S_malloc(strlen(name)+strlen(".wav")+1);
      if ( fileName )
	sprintf(fileName,"%s.wav",name);
    }
  
  if ( fileName )
    {
      
      FILE *fptr = fopen(fileName,"rb");
      
      if ( fptr == NULL )
	{
	  rc = S_WAVEDATA_FILEDOESNOTEXIST;
	  return rc;
	}
      
      fseek(fptr,0,SEEK_END);
      slong fileSize = ftell(fptr);
      fseek(fptr,0,SEEK_SET);

      uchar *buffer = (uchar *)S_malloc(fileSize);
      
      if ( buffer )
	{
	  
	  fread(buffer,1,fileSize,fptr);
	  fclose(fptr);
	  
	  Create(buffer,fileSize,priority,loop);
	  
	  S_free(buffer);
	}
      
      S_free(fileName);
    }
  
  return 0;
}


int
S_WaveData::Create(uchar *buffer,slong fileSize,int ipriority,int iloop)
{
  
  if ( buffer )
    {
      priority = ipriority;
      loop = iloop;
      
      int i;
      int pos = 0;
      
      WaveRIFFHeaderStruct riff;
      for ( i=0; i<(int)sizeof(riff); i++ )
         ((char *)&riff)[i] = buffer[pos++];
      for ( i=0; i<(int)sizeof(header); i++ )
	((char *)&header)[i] = buffer[pos++];
      
      if ( header.wavHeaderDataSize == 18 )
	{
	  pos = 0;
	  WaveWAVEHeaderStruct18 header18;
	  for ( i=0; i<(int)sizeof(riff); i++ )
            ((char *)&riff)[i] = buffer[pos++];
         for ( i=0; i<30; i++ )
	   ((char *)&header18)[i] = buffer[pos++];
	 
         header.wavHeaderDataSize = 16;
         header.usFormatTag = header18.usFormatTag;
         header.usChannels = header18.usChannels;
         header.ulSamplesPerSec = header18.ulSamplesPerSec;
         header.ulAveBytesPerSec = header18.ulAveBytesPerSec;
         header.usBlockAlign = header18.usBlockAlign;
         header.usBitsPerSample = header18.usBitsPerSample;
	 
         WaveFACTHeaderStruct fact;
         for ( i=0; i<(int)sizeof(fact); i++ )
	   ((char *)&fact)[i] = buffer[pos++];
	}
      
      WaveDATAHeaderStruct data;
      for ( i=0; i<(int)sizeof(data); i++ )
	((char *)&data)[i] = buffer[pos++];
      
      ulDataSize = data.ulDataSize;
      pAudioData = (char *) S_malloc(data.ulDataSize);
      
      if ( pAudioData )
	{
	  
	  for ( i=0; i<(slong)data.ulDataSize; i++ )
            pAudioData[i] = buffer[pos++];
	  
	  msec = ( data.ulDataSize/header.usChannels/
		   (header.usBitsPerSample/8) )*1000/header.ulSamplesPerSec;
	  
#         if defined(OS2) && defined(IBMMMOS2)
	  mmWaveSetParm.ulSamplesPerSec = header.ulSamplesPerSec;
	  mmWaveSetParm.usBitsPerSample = header.usBitsPerSample;
	  mmWaveSetParm.usChannels = header.usChannels;
	  mmWaveSetParm.ulAudio = MCI_SET_AUDIO_ALL;
#         endif
	  
#         ifdef WIN32
	  lpDsb = NULL;
	  if (_root && _root->lpDs && _root->flags&S_ROOT_USEDIRECTSOUND)
	    {
	      PCMWAVEFORMAT pcmwf;
	      DSBUFFERDESC dsbdesc;
	      HRESULT hr;
	      
	      memset(&pcmwf, 0, sizeof(PCMWAVEFORMAT));
	      pcmwf.wf.wFormatTag = WAVE_FORMAT_PCM;
	      pcmwf.wf.nChannels = header.usChannels;
	      pcmwf.wf.nSamplesPerSec = header.ulSamplesPerSec;
	      pcmwf.wBitsPerSample = header.usBitsPerSample;
	      pcmwf.wf.nBlockAlign = header.usChannels *
		header.usBitsPerSample/8;
	      pcmwf.wf.nAvgBytesPerSec = pcmwf.wf.nSamplesPerSec * 
		pcmwf.wf.nBlockAlign;
	      
	      memset(&dsbdesc, 0, sizeof(DSBUFFERDESC));
	      dsbdesc.dwSize = sizeof(DSBUFFERDESC);
	      dsbdesc.dwFlags = DSBCAPS_CTRLDEFAULT | DSBCAPS_STATIC; 
	      dsbdesc.dwBufferBytes = data.ulDataSize; 
	      dsbdesc.lpwfxFormat = (LPWAVEFORMATEX)&pcmwf;
	      
	      _root->lpDs->CreateSoundBuffer(&dsbdesc,&lpDsb,NULL);
	      
	      LPVOID lpvPtr1;
	      DWORD dwBytes1; 
	      LPVOID lpvPtr2;
	      DWORD dwBytes2;
	      
	      hr = lpDsb->Lock( 0, data.ulDataSize,
				&lpvPtr1, &dwBytes1,
				&lpvPtr2, &dwBytes2, 0);
	      
	      if( DSERR_BUFFERLOST == hr )
		{
		  lpDsb->Restore();
		  hr = lpDsb->Lock( 0, data.ulDataSize,
				    &lpvPtr1, &dwBytes1, 
                                    &lpvPtr2, &dwBytes2, 0);
		}
	      
	      if( DS_OK == hr )
		{
                  CopyMemory( lpvPtr1, pAudioData, dwBytes1 );
                  if( NULL != lpvPtr2 )
		    {
		      CopyMemory( lpvPtr2, pAudioData+dwBytes1, dwBytes2 );
		    }
		  
                  hr = lpDsb->Unlock(lpvPtr1, dwBytes1, lpvPtr2, dwBytes2);
		}
	    }
#        endif
	}
    }
  return rc;
}


int
S_WaveData::Destroy()
{
# ifdef WIN32
  if ( lpDsb ) 
    {
      /* ***************************************************************** */
      /* *** This line can potentially crash if you close the window   *** */
      /* *** that was providing access to the DirectSoundBuffer before *** */
      /* *** Destroy() is called.                                      *** */
      /* ***************************************************************** */
      lpDsb->Release();
      lpDsb = NULL;
    }
# endif
  if ( name )
    {
      S_free(name);
      name = NULL;
    }
  if ( pAudioData )
    {
      S_free(pAudioData);
      pAudioData = NULL;
    }
  return 0;
}


S_WaveData::~S_WaveData()
{
  Destroy();
}


int
S_WaveData::SetVolume(int ivolume)
{
  volume = ivolume;
# ifdef WIN32
  if ( lpDsb )
    lpDsb->SetVolume( (volume*100 - 10000)/2 );
# endif
  return 0;
}
