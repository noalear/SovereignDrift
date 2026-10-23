#ifndef S_REGCODE
#define S_REGCODE

#include "S_String.h"

#define S_REGCODE_NUMERIC        1
#define S_REGCODE_UPPERCASEALPHA 2
#define S_REGCODE_LOWERCASEALPHA 4
#define S_REGCODE_ALPHA          6
#define S_REGCODE_ALPHANUMERIC   7

class S_RegCode
{
 public:
  int options;
  S_String key;
  S_String code;
  S_String exclude;
  
  int numPiratedSerials;
  ulong *piratedSerials;
  int AddPiratedSerial(ulong);
  int IsPiratedSerial(ulong);
  
  int GenerateCode(ulong,ulong=0);
  int GetDuration(ulong,char *);
  
  int Init();
  S_RegCode();
  int Create(char * ="LmHeDvIlMp",int=S_REGCODE_ALPHANUMERIC,char * =NULL);
  int Destroy();
  ~S_RegCode();
  
  int IsPrintable(char);
  
};

#endif
