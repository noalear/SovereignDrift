#ifndef S_ZIPARCHIVE
#define S_ZIPARCHIVE

#define S_ZIPARCHIVE_OK                      0
#define S_ZIPARCHIVE_NOFILENAME             -1
#define S_ZIPARCHIVE_OPENFAILED             -2


typedef struct
{
  uchar PK[2];
  ushort us;
} S_ZipRecordHeader;


typedef struct
{
  uchar dummy[2];
  uchar version_needed_to_extract[2];
  ushort general_purpose_bit_flag;
  ushort compression_method;
  ushort last_mod_file_time;
  ushort last_mod_file_date;
  ulong crc32;
  ulong csize;
  ulong ucsize;
  ushort filename_length;
  ushort extra_field_length;
} S_ZipFileHeader;


class S_ZipFile
{
 public:
  char *name;
  char *extra;
  char *buffer;
  
  S_ZipFileHeader zfh;
  
  S_ZipFile(char * =NULL);
  ~S_ZipFile();
};


class S_ZipArchive
{
 public:
  char *filename;
  FILE *fptr;
  
  ulong exeSize;
  ulong fileSize;
  ulong archiveSize;
  ulong GetFilePosition();
  
  int Unzip(char *);
  int Open();
  int Close();
  int WriteExe(char *);
  S_ZipFile *GetNextFile();
  
  S_ZipArchive(char * =NULL);
  ~S_ZipArchive();
};


typedef struct central_directory_file_header
{ /* CENTRAL */
  uchar version_made_by[2];
  uchar version_needed_to_extract[2];
  ushort general_purpose_bit_flag;
  ushort compression_method;
  ushort last_mod_file_time;
  ushort last_mod_file_date;
  ulong crc32;
  ulong csize;
  ulong ucsize;
  ushort filename_length;
  ushort extra_field_length;
  ushort file_comment_length;
  ushort disk_number_start;
  ushort internal_file_attributes;
  ushort dummy;
  ulong external_file_attributes;
  ulong relative_offset_local_header; /* Needs to be adjusted for self-ext */
} cdir_file_hdr;


typedef struct end_central_dir_record 
{ /* END CENTRAL */
  ushort number_this_disk;
  ushort num_disk_start_cdir;
  ushort num_entries_centrl_dir_ths_disk;
  ushort total_entries_central_dir;
  ulong size_central_directory;
  ulong offset_start_central_directory;
  ushort zipfile_comment_length;
} ecdir_rec;

#endif


