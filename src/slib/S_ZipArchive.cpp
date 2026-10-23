/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
#include "S_OS.h"
#include "S_ZipArchive.h"


S_ZipArchive::S_ZipArchive(char *ifilename)
{
  filename = NULL;
  if ( ifilename )
    {
      filename = S_strdup(ifilename);
    }
  Open();
  Close();
}


int
S_ZipArchive::Open()
{
  if ( !filename )
    {
      return S_ZIPARCHIVE_NOFILENAME;
    }

  fptr = fopen(filename,"rb");
  fseek(fptr,0,SEEK_END);
  fileSize = ftell(fptr);
  fseek(fptr,0,SEEK_SET);
  if ( fptr )
    {
      
      /* *** Find the start of the zip archive *** */
      int rc = 1;
      do
	{
	  /* *** Read until we come across 'P' *** */
	  char p,k;
	  rc = fread(&p,1,sizeof(p),fptr);
	  if ( rc && p == 'P' )
	    {
	      /* *** Now look for a 'K' *** */
	      rc = fread(&k,1,sizeof(k),fptr);
	      if ( k == 'K' )
		{
		  /* *** Now look for a short of 1027 *** */
		  ushort us;
		  rc = fread(&us,1,sizeof(us),fptr);
		  if ( us == 1027 )
		    {
		      slong offset = sizeof(p) + sizeof(k) + sizeof(us);
		      fseek(fptr,-offset,SEEK_CUR);
		      exeSize = ftell(fptr);
		      archiveSize = fileSize - exeSize;
		      return S_ZIPARCHIVE_OK;
		    }
		}
	    }
        } while ( rc );
      
    }
  
  return S_ZIPARCHIVE_OPENFAILED;
}


int
S_ZipArchive::WriteExe(char *exeFilename)
{
  char *exebin = (char *)S_malloc(exeSize);
  if ( exebin && fptr )
    {
      fseek(fptr,0,SEEK_SET);
      fread(exebin,1,exeSize,fptr);
      FILE *ofptr = fopen(exeFilename,"wb");
      if ( ofptr )
	{
	  fwrite(exebin,1,exeSize,ofptr);
	  fclose(ofptr);
	}
      S_free(exebin);
    }
  return 0;
}


S_ZipFile *S_ZipArchive::GetNextFile()
{
  if ( !fptr )
    {
      return NULL;
    }
  
  int rc = TRUE;
  
  S_ZipFile *zipFile = NULL;
  
  while ( rc ) 
    {
      
      S_ZipRecordHeader zrh;
      rc  = fread( (char *)&zrh,1,sizeof(zrh),fptr );
      
      if ( !rc )
	{
	  return NULL;
	}
      
      switch(zrh.us)
	{
	case 1027 :
	  {
	    zipFile = new S_ZipFile();
	    
	    rc = fread( ((char *)&zipFile->zfh)+2,1,
			sizeof(zipFile->zfh)-2,fptr);
	    if ( !rc )
	      {
		return NULL;
	      }
	    
	    if ( zipFile->zfh.filename_length > 0 )
	      {
		zipFile->name = (char *)
		  S_malloc(zipFile->zfh.filename_length+1);
		fread(zipFile->name,1,zipFile->zfh.filename_length,fptr);
		zipFile->name[zipFile->zfh.filename_length] = 0;
	      }
	    
	    if ( zipFile->zfh.extra_field_length > 0 )
	      {
		zipFile->extra = (char *)
		  S_malloc(zipFile->zfh.extra_field_length+1);
		fread(zipFile->extra,
		      1,
		      zipFile->zfh.extra_field_length,
		      fptr);
		zipFile->name[zipFile->zfh.extra_field_length] = 0;
	      }
	    
	    if ( zipFile->zfh.csize > 0 )
	      {
		zipFile->buffer = (char *)S_malloc(zipFile->zfh.csize);
	      }
	    rc = fread(zipFile->buffer,1,zipFile->zfh.csize,fptr);
	    if ( !rc )
	      {
		return NULL;
	      }
	    return zipFile;
	  }
	case 513 :
	  {
	    cdir_file_hdr cdfh;
	    fread( ((char *)&cdfh),1,
		   sizeof(cdfh)-sizeof(short)-2*sizeof(slong),fptr );
	    fread( ((char *)&cdfh)+sizeof(cdfh)-2*sizeof(slong),1,
		   2*sizeof(slong),fptr );
	    char *filename = (char *)S_malloc(cdfh.filename_length+1);
	    fread(filename,1,cdfh.filename_length,fptr);
	    filename[cdfh.filename_length] = 0;
	    break;
	  }
	case 1541 :
	  {
	    ecdir_rec ecdfh;
	    fread( ((char *)&ecdfh),1,sizeof(ecdfh),fptr );
	    break;
	  }
	  
	case 5 :
	  break;
	  
	default :
	  printf("ERROR: Unknown section id %d\n",zrh.us);
	  return NULL;
	}
    }
  
  return zipFile;
}


int
S_ZipArchive::Close() {
  if ( fptr )
    {
      fclose(fptr);
      fptr = NULL;
    }
  return S_ZIPARCHIVE_OK;;
}

#ifdef __cplusplus
extern "C" {
#endif
#ifdef OS2
int EXPENTRY UzpMain(int,char **);
#else
int UzpMain(int,char **);
#endif
#ifdef __cplusplus
}
#endif

#ifdef UNIX
   int _tell(int zipfd){return 0;}
#endif

/*
extern int *_zipfd;
ulong S_ZipArchive::GetFilePosition() {
   if ( !_zipfd )
      return 0;
   int pos = _tell(*_zipfd);
   if ( pos < 0 )
      return 0;
   return pos;
   }
   */


int
_Unzip(char *line) {
  int argc = 0;
  char **argv = NULL;
  
  int i=0;
  int j=0;
  int inQuote = FALSE;
  int len = strlen(line);
  while ( i<len && j<len )
    {
      while ( line[i] == ' ' && i < len ) i++;
      j = i;
      if ( line[i] == '"' )
	{
	  inQuote = TRUE;
	  i++;
	}
      while ( (line[j] != ' ' && j < len) || inQuote )
	{
	  j++;
	  if ( line[j] == '"' )
	    {
	      inQuote = FALSE;
	      line[j] = ' ';
	    }
	}
      
      if ( j > i )
	{
	  if ( argc == 0 )
	    argv = (char **)S_malloc(sizeof(char *)*(argc+1));
	  else
            argv = (char **)S_realloc(argv,sizeof(char *)*(argc+1));
	  argv[argc] = (char *)S_malloc(sizeof(char)*(j-i+1));
	  strncpy(argv[argc],&line[i],j-i);
	  argv[argc][j-i] = 0;
	  argc++;
	}
      i = j;
    }
  
  int rc = UzpMain(argc,argv);
  
  if ( argv )
    {
      for ( i=0; i<argc; i++ )
	{
	  if ( argv[i] )
	    {
	      S_free(argv[i]);
	      argv[i] = 0;
	    }
	}
      S_free(argv);
      argv = NULL;
    }
  
  return rc;
}


int
_Unzip(int argc,char *argv[])
{
  return UzpMain(argc,argv);
}


int
S_ZipArchive::Unzip(char *options)
{
  int rc = S_ZIPARCHIVE_OPENFAILED;
  FILE *fptr = fopen(filename,"rb");
  if ( fptr )
    {
      fclose(fptr);
      char *cmdLine = (char *)S_malloc(strlen(options)+strlen(filename)+
				       strlen("unzip")+3);
      sprintf(cmdLine,"unzip %s %s",options,filename);
      rc = _Unzip(cmdLine);
      S_free(cmdLine);
    }
  return rc;
}


S_ZipArchive::~S_ZipArchive()
{
  if ( filename )
    {
      S_free(filename);
      filename = NULL;
    }
}



S_ZipFile::S_ZipFile(char *iname)
{
  name = NULL;
  if ( iname )
    name = S_strdup(iname);
  extra = NULL;
  buffer = NULL;
}



S_ZipFile::~S_ZipFile()
{
  if ( name )
    {
      S_free(name);
      name = NULL;
    }
  if ( extra )
    {
      S_free(extra);
      extra = NULL;
    }
  if ( buffer )
    {
      S_free(buffer);
      buffer = NULL;
    }
}
