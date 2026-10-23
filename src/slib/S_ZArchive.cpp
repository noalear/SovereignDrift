/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
#include "S_OS.h"
#include "S_ZArchive.h"
#include "S_Directory.h"


S_ZArchive::S_ZArchive()
{
  hdr.hdrSize = sizeof(hdr);
  hdr.compSize = 0;
  hdr.uncompSize = 0;
  ofptr = NULL;
  ifptr = NULL;
  efptr = NULL;
  numBytesRead = 0;
  numBytesWritten = 0;
  exeSize = 0;
}


int
S_ZArchive::Open(char *filename)
{
  if ( filename )
    {
      numBytesRead = 0;
      name = S_strdup(filename);
      ifptr = fopen(name,"rb");
      if ( ifptr )
	{
	  fseek(ifptr,-(int)sizeof(ulong),SEEK_END);
	  fread((char *)&exeSize,1,sizeof(ulong),ifptr);
	  fileSize = ftell(ifptr) + sizeof(ulong) - exeSize;
	  archiveSize = fileSize - exeSize;
	  fseek(ifptr,exeSize,SEEK_SET);
	  int bytesRead;
	  bytesRead = fread((char *)&hdr.hdrSize,1,sizeof(hdr.hdrSize),ifptr);
	  if ( bytesRead == sizeof(hdr.hdrSize) && hdr.hdrSize == sizeof(hdr) )
	    {
	      fseek(ifptr,-bytesRead,SEEK_CUR);
	      bytesRead = fread((char *)&hdr,1,hdr.hdrSize,ifptr);
	      if ( bytesRead == (int)hdr.hdrSize )
		{
		  rc = S_ZARCHIVE_OK;
		  return rc;
		}
	    }
	}
    }
  rc = S_ZARCHIVE_OPENFAILED;
  return rc;
}


int
S_ZArchive::WriteExe(char *exeFilename)
{
  rc = S_ZARCHIVE_EXEWRITEFAILED;
  char *exebin = (char *)S_malloc(exeSize);
  if ( exebin && ifptr )
    {
      fseek(ifptr,0,SEEK_SET);
      fread(exebin,1,exeSize,ifptr);
      efptr = fopen(exeFilename,"wb");
      if ( efptr )
	{
	  fwrite(exebin,1,exeSize,efptr);
	  fclose(efptr);
	  efptr = NULL;
	  rc = S_ZARCHIVE_OK;
	}
      S_free(exebin);
    }
  return rc;
}


ulong 
S_ZArchive::GetFilePosition()
{
  if ( !ifptr )
    return 0;
  int pos = ftell(ifptr);
  if ( pos > 0 )
    return pos;
  return 0;
}



S_ZFile *
S_ZArchive::ReadNextFile(int options)
{
  S_ZFile *zFile = new S_ZFile();
  if ( zFile )
    {
      rc = zFile->Read(ifptr,&numBytesRead,options);
      if ( rc == S_ZFILE_OK )
	{
	  AddChild(zFile);
	  return zFile;
	}
      delete zFile;
    }
  return NULL;
}


S_ZFile *
S_ZArchive::ReadNextChunkedFile(int options)
{
  S_ZFile *zFile = new S_ZFile();
  if ( zFile )
    {
      rc = zFile->ReadHeader(ifptr);
      if ( rc == S_ZFILE_OK )
	{
	  rc = zFile->ReadName(ifptr);
	  if ( rc == S_ZFILE_OK )
	    {
	      if ( options & S_ZFILE_DATA )
		{
		  //
		  // Read all the chunks
		  //
		  S_ZChunk *zChunk = NULL;
		  do
		    {
		      zChunk = zFile->ReadNextChunk(ifptr,
						    &numBytesRead,
						    options);
		    } while ( zChunk && zChunk->rc == S_ZFILE_OK );
		  
		  AddChild(zFile);
		  return zFile;
		}
	      else
		{
		  fseek(ifptr,zFile->hdr.chunkBytes,SEEK_CUR);
		  return zFile;
		}
	    }
	}
      delete zFile;
    }
  return NULL;
}
  

int
S_ZArchive::Write(char *filename)
{

  ofptr = fopen(filename,"wb");
  if ( ofptr )
    {
      S_Tree *tmp;
      hdr.compSize = 0;
      hdr.uncompSize = 0;
      numBytesWritten = 0;
      for ( tmp=child; tmp!=NULL; tmp=tmp->next )
	{
	  hdr.compSize += ((S_ZFile *)tmp)->hdr.compSize;
	  hdr.uncompSize += ((S_ZFile *)tmp)->hdr.uncompSize;
	}
      int bytesWritten = fwrite((char *)&hdr,1,hdr.hdrSize,ofptr);
      if ( bytesWritten == (int)hdr.hdrSize )
	{
	  rc = S_ZFILE_OK;
	  for ( tmp=child; tmp!=NULL && rc==S_ZFILE_OK; tmp=tmp->next )
	    {
	      rc = ((S_ZFile *)tmp)->Write(ofptr);
	      numBytesWritten += ((S_ZFile *)tmp)->hdr.uncompSize;
	    }
	  fwrite((char *)&exeSize,1,sizeof(ulong),ofptr);
	  fclose(ofptr);
	  ofptr = NULL;
	  return rc;
	}
      fclose(ofptr);
      ofptr = NULL;
    }
  return S_ZARCHIVE_WRITEFAILED;
}


int
S_ZArchive::IsValid()
{
  
  rc = S_ZFILE_OPENFAILED;
  if ( !ifptr )
    return rc;
  
  rc = S_ZFILE_OK;
  do
    {
      rc = testFile.Read(ifptr,&numBytesRead,S_ZFILE_NAME);
      if ( rc != S_ZFILE_OK && (ulong)ftell(ifptr) < archiveSize-sizeof(ulong))
	return FALSE;
      testFile.Destroy();
    } while ( rc == S_ZFILE_OK );
  return TRUE;
}


int
S_ZArchive::CanExtract()
{
  
  rc = S_ZFILE_OPENFAILED;
  if ( !ifptr )
    {
      return rc;
    }
  
  rc = S_ZFILE_OK;
  do
    {
      rc = testFile.Read(ifptr,&numBytesRead,S_ZFILE_NAME);
      if ( rc != S_ZFILE_OK && (ulong)ftell(ifptr) < archiveSize-sizeof(ulong))
	return FALSE;
      
#     if defined(WIN32) || defined(OS2)
      char separator = '\\';
#     endif
#     if defined(UNIX)
      char separator = '/';
#     endif
      
      if ( rc == S_ZFILE_OK )
	{
	  int i;
	  S_Directory *dir = NULL;
	  char *dirname = NULL;
	  if ( testFile.name )
            dirname = S_strdup(testFile.name);
         if ( dirname )
	   {
	     for ( i=strlen(testFile.name); dirname[i]!=separator&&i>0; i-- );
	     if ( dirname[i] == separator )
	       {
		 dirname[i] = 0;
		 dir = new S_Directory(dirname);
		 if ( dir )
		   dir->CreateDir();
               }
	     S_free(dirname);
	   }
	 
         if ( !_OsFileExists(testFile.name) )
	   {
	     if ( dir )
	       {
		 dir->RemoveDir();
		 delete dir;
		 dir = NULL;
	       }
	     return TRUE;
	   }
	 
         FILE *fptr = fopen(testFile.name,"a");
         if ( !fptr )
	   {
	     if ( dir )
	       {
		 dir->RemoveDir();
		 delete dir;
		 dir = NULL;
   	       }
	     return FALSE;
	   }
	 
         if ( dir )
	   {
	     dir->RemoveDir();
	     delete dir;
	     dir = NULL;
	   }
         fclose(fptr);
	 
         testFile.Destroy();
	}
      
    } while ( rc == S_ZFILE_OK );
  return TRUE;
}



int
S_ZArchive::Unzap(char *dirName,int ioptions)
{
  
  numBytesWritten = 0;
  
  if ( ioptions == S_ZARCHIVE_CHUNKED )
    {
      int keepReadingFiles;
      S_ZFile *zFile = NULL;
      do
	{
	  keepReadingFiles = FALSE;
	  zFile = new S_ZFile();
	  if ( zFile )
	    {
	      rc = zFile->ReadHeader(ifptr);
	      if ( rc == S_ZFILE_OK )
		{
		  rc = zFile->ReadName(ifptr);
		  if ( rc == S_ZFILE_OK )
		    {
		      
		      //
		      // Create the output filename
		      //
		      S_String filename;
		      if ( dirName )
			{
			  filename.Create("%s\\%s",dirName,zFile->name);
			}
		      else
			{
			  filename.Create("%s",zFile->name);
			}
		      
		      FILE *ofptr = fopen(filename.str,"wb");
		      if ( ofptr )
			{
			  
			  //
			  // Read all the chunks
			  //
			  int keepReadingChunks;
			  S_ZChunk *zChunk = NULL;
			  do
			    {
			      keepReadingChunks = FALSE;
			      zChunk = zFile->ReadNextChunk(ifptr,
							    &numBytesRead,
							    S_ZCHUNK_DATA);
			      if ( zChunk && zFile->rc == S_ZFILE_OK )
				{
				  zChunk->UnCompress();
				  zChunk->WriteData(ofptr,&numBytesWritten);
				  if ( zChunk->rc == S_ZFILE_OK )
				    {
				      keepReadingChunks = TRUE;
				      keepReadingFiles = TRUE;
				    }
				  delete zChunk;
				}
			      
			    } while ( keepReadingChunks );
			  
			  fclose(ofptr);
			}
		    }
		  
		}          	
	      delete zFile;
	    }
	} while ( keepReadingFiles );
    }
  
  if ( ioptions != S_ZARCHIVE_CHUNKED )
    {
      do {
	unzapFile = ReadNextFile();
	if ( unzapFile )
	  {
	    rc = unzapFile->UnCompress();
	    if ( rc == S_ZFILE_OK )
	      {
		char path[1024];
		if ( dirName )
		  {
		    sprintf(path,"%s\\%s",dirName,unzapFile->name);
		  }
		else
		  {
		    sprintf(path,"%s",unzapFile->name);
		  }
		unzapFile->Write(path,&numBytesWritten);
	      }
	    else
	      return S_ZARCHIVE_UNZAPFAILED;
	    delete unzapFile;
	  }
      } while (unzapFile);
    }
  
  return S_ZARCHIVE_OK;
}


int
S_ZArchive::Close()
{
  if ( name )
    {
      S_free(name);
      name = NULL;
    }
  if ( ifptr )
    {
      fclose(ifptr);
      ifptr = NULL;
    }
  if ( ofptr )
    {
      fclose(ofptr);
      ofptr = NULL;
    }
  return 0;
}


S_ZArchive::~S_ZArchive()
{
  Close();
  DeleteChildren();
}


char *S_ZArchive::ErrorString()
{
  
  switch ( rc )
    {
    case S_ZARCHIVE_OK :
      error.Create("No error");
      break;
    case S_ZARCHIVE_OPENFAILED :
      error.Create("Open failed");
      break;
    case S_ZARCHIVE_WRITEFAILED :
      error.Create("Write failed");
      break;
    case S_ZARCHIVE_UNZAPFAILED :
      error.Create("Unzap failed");
      break;
    case S_ZARCHIVE_EXEWRITEFAILED :
      error.Create("Executable write failed");
      break;
    case S_ZARCHIVE_ENDOFFILE :
      error.Create("End of file");
      break;
    default :
      error.Create("Unknown error");
    }
  
  return error.str;
}


S_ZFile::S_ZFile(S_ZFile *zFile)
{
  rc = S_ZFILE_OK;
  hdr.hdrSize = sizeof(hdr);
  hdr.nameSize = 0;
  hdr.fileSize = 0;
  hdr.compSize = 0;
  hdr.uncompSize = 0;
  hdr.chunkBytes = 0;
  buffer = NULL;
  
  if ( zFile )
    {
      S_Tree::Create(zFile->name);
      hdr.nameSize = zFile->hdr.nameSize;
      hdr.fileSize = zFile->hdr.fileSize;
      hdr.compSize = zFile->hdr.compSize;
      hdr.uncompSize = zFile->hdr.uncompSize;
      hdr.chunkBytes = zFile->hdr.chunkBytes;
      buffer = (uchar *) S_malloc(hdr.fileSize);
      for ( uint i=0; i<hdr.fileSize; i++ )
	buffer[i] = zFile->buffer[i];
    }
  
}


int
S_ZFile::IsEqualTo(S_ZFile *zFile)
{
  if ( !zFile )
    {
      return FALSE;
    }
  
  int isEqual = hdr.nameSize == zFile->hdr.nameSize &&
    hdr.fileSize == zFile->hdr.fileSize &&
    hdr.compSize == zFile->hdr.compSize &&
    hdr.uncompSize == zFile->hdr.uncompSize &&
    hdr.chunkBytes == zFile->hdr.chunkBytes &&
    !strcmp(name,zFile->name);
  
  /*
    if ( isEqual )
    for ( int i=0; i<hdr.compSize; i++ )
    if ( buffer[i] != zFile->buffer[i] )
    isEqual = FALSE;
  */
  
  return isEqual;
}


S_ZChunk *
S_ZFile::ReadNextChunk(FILE *ifptr,ulong *pnumBytesRead,int ioptions)
{
  if ( ifptr )
    {
      S_ZChunk *zChunk = new S_ZChunk();
      if ( zChunk )
	{
	  rc = zChunk->Read(ifptr,pnumBytesRead,ioptions);
	  if ( rc == S_ZFILE_OK )
	    {
	      AddChild(zChunk);
	      return zChunk;
	    }
	  delete zChunk;
	}
    }
  return NULL;
}


int
S_ZFile::ReadHeader(FILE *fptr)
{
  if ( fptr )
    {
      int bytesRead;
      bytesRead = fread((char *)&hdr.hdrSize,1,sizeof(hdr.hdrSize),fptr);
      fseek(fptr,-bytesRead,SEEK_CUR);
      if ( bytesRead == sizeof(hdr.hdrSize) && hdr.hdrSize == sizeof(hdr) )
	{
	  bytesRead = fread((char *)&hdr,1,hdr.hdrSize,fptr);
	  if ( bytesRead == (int)hdr.hdrSize )
	    {
	      return S_ZFILE_OK;
            }
	}
    }
  return S_ZFILE_READFAILED;
}


int
S_ZFile::ReadName(FILE *fptr)
{
  if ( fptr )
    {
      name = (char *)S_malloc((hdr.nameSize+1)*sizeof(char));
      if ( name )
	{
	  int bytesRead = fread(name,1,hdr.nameSize,fptr);
	  if ( bytesRead == (int)hdr.nameSize )
	    {
	      name[hdr.nameSize] = 0;
              rc = S_ZFILE_OK;
              return rc;
	    }
	}
    }
  rc = S_ZFILE_READFAILED;
  return rc;
}


int
S_ZFile::Read(FILE *fptr,ulong *pnumBytesRead,int options)
{
  if ( fptr )
    {
      int bytesRead;
      bytesRead = fread((char *)&hdr.hdrSize,1,sizeof(hdr.hdrSize),fptr);
      if ( bytesRead == sizeof(hdr.hdrSize) && hdr.hdrSize == sizeof(hdr) )
	{
	  fseek(fptr,-bytesRead,SEEK_CUR);
	  bytesRead = fread((char *)&hdr,1,hdr.hdrSize,fptr);
	  if ( bytesRead == (int)hdr.hdrSize )
	    {
	      if ( options & S_ZFILE_NAME )
		{
		  name = (char *)S_malloc((hdr.nameSize+1)*sizeof(char));
		  if ( name )
		    {
		      bytesRead = fread(name,1,hdr.nameSize,fptr);
		      if ( bytesRead == (int)hdr.nameSize )
			{
			  name[hdr.nameSize] = 0;
			  if ( options & S_ZFILE_DATA )
			    {
			      buffer =(uchar *) 
				S_malloc(hdr.fileSize*sizeof(char));
			      if ( buffer )
				{
				  int bytes = 0;
				  int bytesRead = 0;
				  do
				    {
				      int bytesToRead = 4096;
				      if ( bytesToRead > 
					   (int)hdr.fileSize-bytesRead )
					bytesToRead = hdr.fileSize-bytesRead;
				      bytes=fread(buffer+bytesRead,
						  1,
						  bytesToRead,
						  fptr);
				      bytesRead += bytes;
				      if ( pnumBytesRead )
					*pnumBytesRead += bytes;
				    } while ( bytes > 0 );
				  if ( bytesRead == (int)hdr.fileSize )
				    {
				      return S_ZFILE_OK;
				    }
				  return S_ZFILE_READFAILED;
				}
			      return S_ZFILE_MALLOCFAILED;
			    }
			  else {
			    fseek(fptr,hdr.fileSize,SEEK_CUR);
			    return S_ZFILE_OK;
			  }
			  return S_ZFILE_READFAILED;
			}
		      return S_ZFILE_MALLOCFAILED;
		    }
		}
	      else
		{
		  fseek(fptr,hdr.nameSize,SEEK_CUR);
		  fseek(fptr,hdr.fileSize,SEEK_CUR);
		  return S_ZFILE_OK;
		}
            }
	}
    }
  return S_ZFILE_READFAILED;
}


int
S_ZFile::Read(char *filename)
{
  if ( filename )
    {
      name = S_strdup(filename);
      hdr.nameSize = strlen(name); 
      FILE *fptr = fopen(name,"rb");
      if ( fptr )
	{
	  fseek(fptr,0,SEEK_END);
	  hdr.fileSize = ftell(fptr);
	  hdr.compSize = hdr.fileSize;
	  hdr.uncompSize = hdr.fileSize;
	  fseek(fptr,0,SEEK_SET);
	  buffer = (uchar *)S_malloc(hdr.fileSize*sizeof(char));
	  if ( buffer )
	    {
	      int bytesRead = fread(buffer,1,hdr.fileSize,fptr);
	      fclose(fptr);
	      if ( bytesRead == (int)hdr.fileSize )
		return S_ZFILE_OK;
	      return S_ZFILE_READFAILED;
	    }
	  return S_ZFILE_MALLOCFAILED;
	}
    }
  return S_ZFILE_OPENFAILED;
}


int
S_ZFile::Write(FILE *fptr) {

  if ( fptr )
    {
      
      //
      // Calclate file size for a chunked file
      //
      if ( child )
	{
	  hdr.chunkBytes = 0;
	  hdr.fileSize = 0;
	  hdr.compSize = 0;
	  hdr.uncompSize = 0;
	  for ( S_Tree *tmp=child; tmp!=NULL; tmp=tmp->next )
	    {
	      S_ZChunk *zChunk = (S_ZChunk *)tmp;
	      hdr.chunkBytes += zChunk->hdr.chunkSize + sizeof(zChunk->hdr);
	      hdr.fileSize +=  zChunk->hdr.chunkSize;
	      hdr.compSize += zChunk->hdr.compSize;
	      hdr.uncompSize += zChunk->hdr.uncompSize;
	    }
	}
      
      int bytesWritten;
      bytesWritten = fwrite((char *)&hdr,1,hdr.hdrSize,fptr);
      if ( bytesWritten != (int)hdr.hdrSize )
	{
	  printf("Failed on header write\n");
	  return S_ZFILE_WRITEFAILED;
	}
      
      bytesWritten = fwrite(name,1,hdr.nameSize,fptr);
      if ( bytesWritten != (int)hdr.nameSize )
	{
	  printf("Failed on name write\n");
	  return S_ZFILE_WRITEFAILED;
	}
      
      //
      // A non-chunked file
      //
      if ( buffer )
	{
	  bytesWritten = fwrite(buffer,1,hdr.fileSize,fptr);
	  if ( bytesWritten == (int)hdr.fileSize )
	    {
	      return S_ZFILE_OK;
	    }
	}
      
      //
      // Empty file is fine
      //
      if ( hdr.fileSize == 0 )
	{
	  return S_ZFILE_OK;
	}

      //
      // A chunked file
      //
      if ( child )
	{
	  int numBytesWritten = 0;

	  if ( hdr.fileSize > 0 )
	    {
	  
	      S_Tree *tmp;
	      
	      rc = S_ZFILE_OK;
	      for ( tmp=child; tmp!=NULL && rc==S_ZFILE_OK; tmp=tmp->next )
		{
		  rc = ((S_ZChunk *)tmp)->Write(fptr);
		  numBytesWritten += ((S_ZChunk *)tmp)->hdr.chunkSize;
		}
	      
	      if ( numBytesWritten != (int)hdr.fileSize )
		{
		  printf("Write failed: numBytesWritten(%d) != hdr.fileSize(%d)\n",
			 numBytesWritten,hdr.fileSize);
		  return S_ZFILE_WRITEFAILED;
		}
	    }
	  
	  return S_ZFILE_OK;
	}
    }
  
  return S_ZFILE_WRITEFAILED;
}


int
S_ZFile::CreateDir(char *filename)
{
  
# if defined(WIN32) || defined(OS2)
  char separator = '\\';
# endif
# if defined(UNIX)
  char separator = '/';
# endif
  
  if ( filename )
    {
      
      //
      // Create any directories in path name
      //
      int i;
      S_String dirname(filename);
      if ( dirname.str )
	{
	  for ( i=strlen(filename); dirname.str[i]!=separator&&i>0; i-- );
	  if ( dirname.str[i] == separator )
	    {
	      dirname.str[i] = 0;
	      S_Directory dir(dirname.str);
	      dir.CreateDir();
	      if ( dir.rc != S_DIRECTORY_OK )
		{
		  rc = S_ZFILE_DIRCREATEFAILED;
		  return rc;
		}
	    }
	}
    }
  
  rc = S_ZFILE_OK;
  return rc;
}


int
S_ZFile::Write(char *filename,ulong *totalBytesWritten)
{
  
  rc = S_ZFILE_OK;
  
# if defined(WIN32) || defined(OS2)
  char separator = '\\';
# endif
# if defined(UNIX)
  //char separator = '/';
# endif
  
  if ( filename )
    {
      
      //
      // Create any directories in path name
      //
      rc = CreateDir(filename);
      if ( rc != S_ZFILE_OK )
	{
	  return rc;
	}
      
      //
      // Now write the file to disk
      //
      FILE *fptr = fopen(filename,"wb");
      if ( fptr )
	{
	  
	  int bytes = 0;
	  int bytesWritten = 0;
	  
	  //
	  // A non-chunked file
	  //
	  if ( buffer )
	    {
	      do
		{
		  int numBytesToWrite = 1024;
		  if ( hdr.fileSize - bytesWritten < 1024 )
		    {
		      numBytesToWrite = hdr.fileSize - bytesWritten;
		    }
		  bytes = fwrite(buffer+bytesWritten,1,numBytesToWrite,fptr);
		  bytesWritten += bytes;
		  if ( totalBytesWritten )
		    {
		      *totalBytesWritten += bytes;
		    }
		} while ( bytes > 0 );
	      
	      if ( bytesWritten == (int)hdr.fileSize )
		{
		  fclose(fptr);
		  rc = S_ZFILE_OK;
		  return rc;
		}
	      
	      fclose(fptr);	  
	      rc = S_ZFILE_WRITEFAILED;
	      return rc;
	      
	    }
	  
	  //
	  // A chunked file
	  //
	  if ( child )
	    {
	      for ( S_Tree *tmp=child; tmp!=NULL; tmp=tmp->next )
		{
		  S_ZChunk *zChunk = (S_ZChunk *)tmp;
		  zChunk->WriteData(fptr,totalBytesWritten);
		  
		  if ( zChunk->rc != S_ZCHUNK_OK )
		    {
		      fclose(fptr);
		      rc = S_ZFILE_WRITEFAILED;
		      return rc;
		    }
		}
	      
	      fclose(fptr);	  
	      rc = S_ZFILE_OK;
	      return rc;
	    }
	  
	  fclose(fptr);	  
          rc = S_ZFILE_WRITEFAILED;
	  return rc;
	  
	}
      
      rc = S_ZFILE_OPENFAILED;
      return rc;
      
    }
  
  rc = S_ZFILE_BADFILENAME;
  return rc;  
}


#ifdef __cplusplus
extern "C" {
#endif
  int compress(       unsigned char *,ulong *,
		      const unsigned char *,ulong);
  int uncompress(       unsigned char *,ulong *,
			const unsigned char *,ulong);
#ifdef __cplusplus
}
#endif


int
S_ZFile::Compress(void)
{
  
  //
  // A non-chunked file
  //
  if ( buffer )
    {  
      ulong uncompLen = hdr.fileSize;
      unsigned char *uncomp = buffer;
      ulong compLen = uncompLen*11/10 + 8;
      unsigned char *comp = (unsigned char *) S_malloc(compLen);
      
      if ( !comp )
	{
	  return S_ZFILE_MALLOCFAILED;
	}
      rc = compress(comp,&compLen,uncomp,uncompLen);
      if ( rc != 0 )
	{
	  S_free(comp);
	  return S_ZFILE_COMPRESSFAILED;
	}
      
      hdr.uncompSize = uncompLen;
      hdr.compSize = uncompLen;
      hdr.fileSize = uncompLen;
      
      if ( compLen < uncompLen ) {
	hdr.uncompSize = uncompLen;
	hdr.compSize = compLen;
	hdr.fileSize = compLen;
	
	buffer = (uchar *)S_realloc(buffer,compLen*sizeof(char));
	
	if ( buffer )
	  {
	    for ( int i=0; i<(int)hdr.fileSize; i++ )
	      {
		buffer[i] = comp[i];
	      }
	  }
	else
	  {
	    S_free(comp);
	    return S_ZFILE_MALLOCFAILED;
	  }
      }
      
      S_free(comp); 
    }
  
  //
  // A chunked file
  //
  if ( child )
    {
      for ( S_Tree *tmp=child; tmp!=NULL; tmp=tmp->next )
	{
	  ((S_ZChunk *)tmp)->Compress();
	}      
    }
  
  return S_ZFILE_OK;   
}


int
S_ZFile::UnCompress(void)
{
  
  //
  // A non-chunked file
  //
  if ( buffer )
    {
      
      if ( hdr.compSize == hdr.uncompSize )
	{
	  return S_ZFILE_OK;
	}
      
      ulong compLen = hdr.compSize;
      unsigned char *comp = buffer;
      ulong uncompLen = hdr.uncompSize;
      unsigned char *uncomp = (unsigned char *) S_malloc(uncompLen);
      
      if ( !uncomp )
	{
	  return S_ZFILE_MALLOCFAILED;
	}
      
      rc = uncompress(uncomp,&uncompLen,comp,compLen);
      if ( rc != 0 )
	{
	  S_free(uncomp);
	  return S_ZFILE_UNCOMPRESSFAILED;
	}
      
      if ( uncompLen != hdr.uncompSize )
	{
	  S_free(uncomp);
	  return S_ZFILE_UNCOMPRESSWRONGSIZE;
	}
      
      if ( uncompLen > compLen )
	{
	  buffer = (uchar *)S_realloc(buffer,uncompLen*sizeof(char));
	}
      
      if ( buffer )
	{
	  hdr.uncompSize = uncompLen;
	  hdr.compSize = compLen;
	  hdr.fileSize = uncompLen;
	  
	  for ( int i=0; i<(int)hdr.fileSize; i++ )
	    {
	      buffer[i] = uncomp[i];
	    }
	}
      else
	{
	  S_free(uncomp);
	  return S_ZFILE_UNCOMPRESSFAILED;
	}
      
      S_free(uncomp);
    }
  
  //
  // A chunked file
  //
  if ( child )
    {
      for ( S_Tree *tmp=child; tmp!=NULL; tmp=tmp->next )
	{
	  ((S_ZChunk *)tmp)->UnCompress();
	}      
    }
  
  return S_ZFILE_OK;
}


int
S_ZFile::IsCompressed()
{
  return hdr.compSize > 0 && hdr.fileSize == hdr.compSize;
}


S_ZFile::~S_ZFile()
{
  if ( name )
    {
      S_free(name);
      name = NULL;
    }
  if ( buffer )
    {
      S_free(buffer);
      buffer = NULL;
    }
  DeleteChildren();
}


char *
S_ZFile::ErrorString()
{
  
  switch ( rc )
    {
    case S_ZFILE_OK :
      error.Create("No error");
      break;
    case S_ZFILE_OPENFAILED :
      error.Create("File open failed");
      break;
    case S_ZFILE_MALLOCFAILED :
      error.Create("Memory allocation failed");
      break;
    case S_ZFILE_READFAILED :
      error.Create("File read failed");
      break;
    case S_ZFILE_WRITEFAILED :
      error.Create("File write failed");
      break;
    case S_ZFILE_COMPRESSFAILED :
      error.Create("File compress failed");
      break;
    case S_ZFILE_UNCOMPRESSFAILED :
      error.Create("File uncompress failed");
      break;
    case S_ZFILE_UNCOMPRESSWRONGSIZE :
      error.Create("File uncompressed to wrong size");
      break;
    case S_ZFILE_DIRCREATEFAILED :
      error.Create("Failed to create directory in path name");
      break;
    case S_ZFILE_BADFILENAME :
      error.Create("The filename specified was not acceptable!");
      break;
    case S_ZFILE_ENDOFFILE :
      error.Create("End of file");
      break;
    default :
      error.Create("Unknown error");
    }
  
  return error.str;
}



S_ZChunk::S_ZChunk(S_ZChunk *zChunk)
{
  rc = S_ZCHUNK_OK;
  hdr.hdrSize = sizeof(hdr);
  hdr.chunkSize = 0;
  hdr.compSize = 0;
  hdr.uncompSize = 0;
  buffer = NULL;
  
  if ( zChunk )
    {
      hdr.chunkSize = zChunk->hdr.chunkSize;
      hdr.compSize = zChunk->hdr.compSize;
      hdr.uncompSize = zChunk->hdr.uncompSize;
      buffer = (uchar *) S_malloc(hdr.chunkSize);
      for ( uint i=0; i<hdr.chunkSize; i++ )
	buffer[i] = zChunk->buffer[i];
    }
}


int
S_ZChunk::IsEqualTo(S_ZChunk *zChunk)
{
  if ( !zChunk )
    {
      return FALSE;
    }
  
  int isEqual = 
    hdr.chunkSize == zChunk->hdr.chunkSize &&
    hdr.compSize == zChunk->hdr.compSize &&
    hdr.uncompSize == zChunk->hdr.uncompSize;
  
  return isEqual;
}


int
S_ZChunk::Read(FILE *fptr,ulong *pnumBytesRead,int options)
{
  
  if ( !fptr )
    {
      return S_ZCHUNK_READFAILED;
    }
  
  int bytesRead = fread((char *)&hdr.hdrSize,1,sizeof(hdr.hdrSize),fptr);
  
  if ( bytesRead > 0 )
    {
      fseek(fptr,-bytesRead,SEEK_CUR);
    }
  
  if ( bytesRead != sizeof(hdr.hdrSize) || hdr.hdrSize != sizeof(hdr) )
    {
      return S_ZCHUNK_READFAILED;
    }
  
  bytesRead = fread((char *)&hdr,1,hdr.hdrSize,fptr);
  
  if ( bytesRead != (int)hdr.hdrSize )
    {
      return S_ZCHUNK_READFAILED;
    }
  
  if ( pnumBytesRead )
    {
      *pnumBytesRead += bytesRead;
    }
  
  if ( options & S_ZCHUNK_DATA )
    {
      buffer =(uchar *) S_malloc(hdr.chunkSize*sizeof(char));
      
      if ( !buffer )
	{
	  return S_ZCHUNK_MALLOCFAILED;
	}
      
      int bytes = 0;
      int bytesRead = 0;
      do
	{
	  int bytesToRead = 4096;
	  if ( bytesToRead > (int)hdr.chunkSize-bytesRead )
	    {
	      bytesToRead = hdr.chunkSize-bytesRead;
	    }
	  
	  bytes = fread(buffer+bytesRead,1,bytesToRead,fptr);
	  bytesRead += bytes;
	  if ( pnumBytesRead )
	    {
	      *pnumBytesRead += bytes;
	    }
	} while ( bytes > 0 );
      
      if ( bytesRead == (int)hdr.chunkSize )
	{
	  return S_ZCHUNK_OK;
	}
      
      return S_ZCHUNK_READFAILED;
    }
  
  fseek(fptr,hdr.chunkSize,SEEK_CUR);
  return S_ZCHUNK_OK;
}


int
S_ZChunk::ReadData(FILE *fptr,int chunkSize)
{
  if ( fptr )
    {
      buffer = (uchar *)S_malloc(chunkSize*sizeof(char));
      if ( buffer )
	{
	  int bytesRead = fread(buffer,1,chunkSize,fptr);
	  
	  hdr.chunkSize = bytesRead;
	  hdr.uncompSize = bytesRead;
	  hdr.compSize = bytesRead;
	  
	  if ( bytesRead > 0 )
	    {
	      rc = S_ZCHUNK_OK;
	      return rc;
	    }
	  
	  rc = S_ZCHUNK_ENDOFFILE;
	  return rc;
	}
      rc = S_ZCHUNK_MALLOCFAILED;
      return rc;
    }
  rc = S_ZCHUNK_OPENFAILED;
  return rc;
}


int
S_ZChunk::WriteData(FILE *fptr,ulong *totalBytesWritten)
{
  if ( fptr )
    {
      int bytes = 0;
      int bytesWritten = 0;
      
      do
	{
	  int numBytesToWrite = 1024;
	  if ( hdr.chunkSize - bytesWritten < 1024 )
	    {
	      numBytesToWrite = hdr.chunkSize - bytesWritten;
	    }
	  bytes = fwrite(buffer+bytesWritten,1,numBytesToWrite,fptr);
	  bytesWritten += bytes;
	  if ( totalBytesWritten )
	    {
	      *totalBytesWritten += bytes;
	    }
	} while ( bytes > 0 );
      
      if ( bytesWritten == (int)hdr.chunkSize )
	{
	  rc = S_ZCHUNK_OK;
	  return rc;
	}
      
    }
  
  rc = S_ZCHUNK_WRITEFAILED;
  return rc;
}


int
S_ZChunk::Write(FILE *fptr)
{
  if ( fptr && buffer )
    {
      int bytesWritten = fwrite((char *)&hdr,1,hdr.hdrSize,fptr);
      if ( bytesWritten == (int)hdr.hdrSize )
	{
	  bytesWritten = fwrite(buffer,1,hdr.chunkSize,fptr);
	  if ( bytesWritten == (int)hdr.chunkSize )
	    {
	      return S_ZCHUNK_OK;
	    }
	}
    }
  return S_ZCHUNK_WRITEFAILED;
}


int
S_ZChunk::Compress(void)
{
  
  ulong uncompLen = hdr.chunkSize;
  unsigned char *uncomp = buffer;
  ulong compLen = uncompLen*11/10 + 8;
  unsigned char *comp = (unsigned char *) S_malloc(compLen);
  
  if ( !comp )
    {
      rc = S_ZCHUNK_MALLOCFAILED;
      return rc;
    }
  rc = compress(comp,&compLen,uncomp,uncompLen);
  if ( rc != 0 )
    {
      S_free(comp);
      rc = S_ZCHUNK_COMPRESSFAILED;
      return rc;
    }
  
  hdr.uncompSize = uncompLen;
  hdr.compSize = uncompLen;
  hdr.chunkSize = uncompLen;
  
  if ( compLen < uncompLen )
    {
      hdr.uncompSize = uncompLen;
      hdr.compSize = compLen;
      hdr.chunkSize = compLen;
      
      buffer = (uchar *)S_realloc(buffer,compLen*sizeof(char));
      
      if ( buffer )
	{
	  for ( int i=0; i<(int)hdr.chunkSize; i++ )
	    {
	      buffer[i] = comp[i];
	    }
	}
      else
	{
	  S_free(comp);
	  rc = S_ZCHUNK_MALLOCFAILED;
	  return rc;
	}
    }
  
  S_free(comp); 
  rc = S_ZCHUNK_OK;
  return rc;
}


int
S_ZChunk::UnCompress(void)
{
  
  if ( hdr.compSize == hdr.uncompSize )
    {
      return S_ZCHUNK_OK;
    }
  
  ulong compLen = hdr.compSize;
  unsigned char *comp = buffer;
  ulong uncompLen = hdr.uncompSize;
  unsigned char *uncomp = (unsigned char *) S_malloc(uncompLen);
  
  if ( !uncomp )
    {
      return S_ZCHUNK_MALLOCFAILED;
    }
  
  rc = uncompress(uncomp,&uncompLen,comp,compLen);
  if ( rc != 0 )
    {
      S_free(uncomp);
      return S_ZCHUNK_UNCOMPRESSFAILED;
    }
  
  if ( uncompLen != hdr.uncompSize )
    {
      S_free(uncomp);
      return S_ZCHUNK_UNCOMPRESSWRONGSIZE;
    }
  
  if ( uncompLen > compLen )
    {
      buffer = (uchar *)S_realloc(buffer,uncompLen*sizeof(char));
    }
  
  if ( buffer )
    {
      hdr.uncompSize = uncompLen;
      hdr.compSize = compLen;
      hdr.chunkSize = uncompLen;
      
      for ( int i=0; i<(int)hdr.chunkSize; i++ )
	{
	  buffer[i] = uncomp[i];
	}
    }
  else
    {
      S_free(uncomp);
      return S_ZCHUNK_UNCOMPRESSFAILED;
    }
  
  S_free(uncomp);
  return S_ZCHUNK_OK;
}


int
S_ZChunk::IsCompressed()
{
  return hdr.compSize > 0 && hdr.chunkSize == hdr.compSize;
}


S_ZChunk::~S_ZChunk()
{
  if ( buffer )
    {
      S_free(buffer);
      buffer = NULL;
    }
}


char *
S_ZChunk::ErrorString()
 {
   switch ( rc )
     {
     case S_ZCHUNK_OK :
       error.Create("No error");
       break;
     case S_ZCHUNK_OPENFAILED :
       error.Create("Chunk open failed");
       break;
     case S_ZCHUNK_MALLOCFAILED :
       error.Create("Memory allocation failed");
       break;
     case S_ZCHUNK_READFAILED :
       error.Create("Chunk read failed");
       break;
     case S_ZCHUNK_WRITEFAILED :
       error.Create("Chunk write failed");
       break;
     case S_ZCHUNK_COMPRESSFAILED :
       error.Create("Chunk compress failed");
       break;
     case S_ZCHUNK_UNCOMPRESSFAILED :
       error.Create("Chunk uncompress failed");
       break;
     case S_ZCHUNK_UNCOMPRESSWRONGSIZE :
       error.Create("Chunk uncompressed to wrong size");
       break;
     case S_ZCHUNK_ENDOFFILE :
       error.Create("End of file");
       break;
     default :
       error.Create("Unknown error");
     }
   
   return error.str;
 }
