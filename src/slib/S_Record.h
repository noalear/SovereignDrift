#ifndef S_RECORD
#define S_RECORD

#define S_RECORD_STRSIZE  256

typedef struct
{
  char keyword[S_RECORD_STRSIZE];
  char value[S_RECORD_STRSIZE];
} S_RecordEntry;


class S_Record
{
 public:
  char *name;
  
  int numEntries;
  int maxEntries;
  S_RecordEntry *entries;
  
  int New();
  int Exists();
  int Open();
  int Close();
  
  int   GetIndex(char *);
  char *GetEntry(char *);
  int SetEntry(char *,char *);
  int AddEntry(char *,char *);
  int RemoveEntry(int);
  int InsertEntry(int,char *,char *);
  
  S_Record(char *,int=0);
  ~S_Record();
};


int S_Record_RemoveCharacter(char *,char);
int S_Record_HasWhiteSpace(char *);

#endif
