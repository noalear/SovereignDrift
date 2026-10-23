#ifndef S_ZARCHIVE
#define S_ZARCHIVE

#include "S_Tree.h"
#include "S_String.h"


#define S_ZCHUNK_DATA                 2
#define S_ZCHUNK_OK                   0
#define S_ZCHUNK_OPENFAILED          -1
#define S_ZCHUNK_MALLOCFAILED        -2
#define S_ZCHUNK_READFAILED          -3
#define S_ZCHUNK_WRITEFAILED         -4
#define S_ZCHUNK_COMPRESSFAILED      -5
#define S_ZCHUNK_UNCOMPRESSFAILED    -6
#define S_ZCHUNK_UNCOMPRESSWRONGSIZE -7
#define S_ZCHUNK_ENDOFFILE          -10


typedef struct
{
  ulong hdrSize;
  ulong chunkSize;
  ulong compSize;
  ulong uncompSize;
} S_ZChunkHeaderStruct;


class S_ZChunk : public S_Tree
{
  public :
    int rc;
  
  uchar *buffer;
  S_ZChunkHeaderStruct hdr;
  
  char *ErrorString();
  S_String error;
  
  int WriteData(FILE *,ulong * =NULL);
  int ReadData(FILE *,int chunkSize);
  int Read(FILE *,ulong * =NULL,int options=S_ZCHUNK_DATA);
  int Write(FILE *);
  
  int Compress();
  int UnCompress();
  
  int IsCompressed();
  int IsEqualTo(S_ZChunk *);
  
  S_ZChunk(S_ZChunk * = NULL);
  ~S_ZChunk();
};


#define S_ZFILE_CHUNKED              4
#define S_ZFILE_DATA                 2
#define S_ZFILE_NAME                 1
#define S_ZFILE_OK                   0
#define S_ZFILE_OPENFAILED          -1
#define S_ZFILE_MALLOCFAILED        -2
#define S_ZFILE_READFAILED          -3
#define S_ZFILE_WRITEFAILED         -4
#define S_ZFILE_COMPRESSFAILED      -5
#define S_ZFILE_UNCOMPRESSFAILED    -6
#define S_ZFILE_UNCOMPRESSWRONGSIZE -7
#define S_ZFILE_DIRCREATEFAILED     -8
#define S_ZFILE_BADFILENAME         -9
#define S_ZFILE_ENDOFFILE          -10

typedef struct
{
  ulong hdrSize;
  ulong nameSize;
  ulong fileSize;
  ulong compSize;
  ulong uncompSize;
  ulong chunkBytes;
} S_ZFileHeaderStruct;

class S_ZFile : public S_Tree
{
  public :
    int rc;
  
  uchar *buffer;
  S_ZFileHeaderStruct hdr;
  
  char *ErrorString();
  S_String error;
  
  int CreateDir(char *);
  
  S_ZChunk *ReadNextChunk(FILE *,ulong * =NULL,int=S_ZCHUNK_DATA);
  int ReadHeader(FILE *);
  int ReadName(FILE *);
  int Read(FILE *,ulong * =NULL,int options=S_ZFILE_NAME|S_ZFILE_DATA);
  int Read(char *);
  int Write(FILE *);
  int Write(char *,ulong * =NULL);
  int Compress();
  int UnCompress();
  
  int IsCompressed();
  int IsEqualTo(S_ZFile *);
  
  S_ZFile(S_ZFile * = NULL);
  ~S_ZFile();
};

#define S_ZARCHIVE_CHUNKED           4
#define S_ZARCHIVE_DATA              2
#define S_ZARCHIVE_NAME              1
#define S_ZARCHIVE_OK                0
#define S_ZARCHIVE_OPENFAILED       -1
#define S_ZARCHIVE_WRITEFAILED      -2
#define S_ZARCHIVE_UNZAPFAILED      -3
#define S_ZARCHIVE_EXEWRITEFAILED   -4
#define S_ZARCHIVE_ENDOFFILE       -10

typedef struct
{
  ulong hdrSize;
  ulong compSize;
  ulong uncompSize;
} S_ZArchiveHeaderStruct;


class S_ZArchive : public S_Tree
{
 public:
  int rc;
  
  S_ZArchiveHeaderStruct hdr;
  FILE *ifptr;
  FILE *ofptr;
  FILE *efptr;
  
  char *ErrorString();
  S_String error;
  
  ulong numBytesRead;
  ulong numBytesWritten;
  
  ulong exeSize;
  ulong fileSize;
  ulong archiveSize;
  
  ulong GetFilePosition();
  
  int WriteExe(char *);
  int Open(char *);
  S_ZFile *ReadNextFile(int=S_ZARCHIVE_NAME|S_ZARCHIVE_DATA);
  S_ZFile *ReadNextChunkedFile(int=S_ZARCHIVE_NAME|S_ZARCHIVE_DATA);
  int Write(char *);
  int Close();
  
  S_ZFile testFile;
  int IsValid();
  int CanExtract();
  
  S_ZFile *unzapFile;
  int Unzap(char * =NULL,int=0);
  
  S_ZArchive();
  ~S_ZArchive();
};

#endif
