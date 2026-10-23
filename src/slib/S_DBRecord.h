#ifndef S_DBRECORD_INCLUDED
#define S_DBRECORD_INCLUDED

#include "S_Tree.h"
#include "S_StringArray.h"

class S_DBRecord : public S_Tree , public S_StringArray 
{
 public:
  
  int Read(FILE *);
  int Write(FILE *);
  
  S_DBRecord *GetRecordNamed(char *);
  char *GetKeywordValue(char *);
  
  int Init();
  S_DBRecord();
  int Create(char * =NULL);
  int Destroy();
  virtual ~S_DBRecord();
};

#endif
