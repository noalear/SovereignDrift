#ifndef S_TOKENARRAY
#define S_TOKENARRAY

#include "S_StringArray.h"

#define S_TOKENARRAY_SINGLEDELIMIT          1
#define S_TOKENARRAY_IGNOREQUOTES           2
#define S_TOKENARRAY_KEEPTRAILINGDELIMITER  4

class S_TokenArray : public S_StringArray
{
 public:
  int options;
  
  char delimiter1;
  char delimiter2;
  int IsWhiteSpace(char);
  
  int Init();
  S_TokenArray(char * =NULL,char = ' ',char = '\t',int=0);
  int Create(char *,char = ' ',char = '\t',int=0);
  int Destroy();
  ~S_TokenArray();
};

#endif
