#ifndef S_DATABASE_INCLUDED
#define S_DATABASE_INCLUDED

#include "S_DBRecord.h"
#include "S_String.h"

class S_Database : public S_DBRecord 
{
 public:
  
  S_String filename;
  
  int Load();
  int Save();
  
  int Init();
  S_Database();
  int Create();
  int Create(char *);
  int Destroy();
  virtual ~S_Database();
};

#endif
